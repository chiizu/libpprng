/*
  Copyright (C) 2013 chiizu
  chiizu.pprng@gmail.com
  
  This file is part of libpprng.
  
  libpprng is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  libpprng is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with libpprng.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "EncounterSeedSearcher.h"
#include "SeedSearcher.h"

namespace pprng
{

namespace
{

struct IVFrameResultHandler
{
  IVFrameResultHandler
    (const EncounterSeedSearcher::Criteria &criteria,
     const EncounterSeedSearcher::ResultCallback &resultHandler)
    : m_criteria(criteria), m_resultHandler(resultHandler)
  {}
  
  void operator()(const HashedIVFrame &frame) const
  {
    Gen5PIDFrameGenerator::Parameters  parameters = m_criteria.frameParameters;
    
    if (Encounter::UsesLeadAbility(parameters.encounterType))
    {
      uint32_t  leadAbilityMask = m_criteria.leadAbilityMask;
      uint32_t  leadAbilityIndex = 0;
      
      while (leadAbilityMask > 0)
      {
        if ((leadAbilityMask & 0x1) != 0)
        {
          parameters.leadAbility = LeadAbility::Type(leadAbilityIndex);
          
          if ((parameters.leadAbility == LeadAbility::CUTE_CHARM) &&
              (parameters.setGender == Gender::ANY))
          {
            parameters.setGender = Gender::FEMALE;
            CheckPIDFrames(frame, parameters);
            
            parameters.setGender = Gender::MALE;
            CheckPIDFrames(frame, parameters);
            
            parameters.setGender = Gender::ANY;
          }
          else
          {
            CheckPIDFrames(frame, parameters);
          }
        }
        
        ++leadAbilityIndex;
        leadAbilityMask >>= 1;
      }
    }
    else
    {
      parameters.leadAbility = LeadAbility::OTHER;
      CheckPIDFrames(frame, parameters);
    }
  }
  
  void CheckPIDFrames(const HashedIVFrame &frame,
                      const Gen5PIDFrameGenerator::Parameters &parameters) const
  {
    Gen5PIDFrameGenerator  generator(frame.seed, parameters);
    
    uint32_t  frameNum = m_criteria.pidFrame.min - 1;
    
    generator.SkipFrames(frameNum);
    
    while (frameNum < m_criteria.pidFrame.max)
    {
      generator.AdvanceFrame();
      ++frameNum;
      
      Gen5PIDFrame  pidFrame = generator.CurrentFrame();
      
      if (pidFrame.isEncounter && pidFrame.abilityActivated &&
          CheckShiny(pidFrame.pid) && CheckNature(pidFrame) &&
          CheckAbility(pidFrame) && CheckGender(pidFrame) &&
          CheckESV(pidFrame))
      {
        Gen5EncounterFrame  encFrame(pidFrame, frame.number, frame.ivs);
        
        m_resultHandler(encFrame);
        
        // only accept first matching frame if not searching for shinies
        if (!m_criteria.shinyOnly)
          break;
      }
    }
  }
  
  bool CheckShiny(PID pid) const
  {
    return !m_criteria.shinyOnly ||
           pid.IsShiny(m_criteria.frameParameters.tid,
                       m_criteria.frameParameters.sid);
  }
  
  bool CheckNature(const Gen5PIDFrame &frame) const
  {
    return ((frame.leadAbility == LeadAbility::SYNCHRONIZE) ||
             m_criteria.pid.CheckNature(frame.nature));
  }
  
  bool CheckAbility(const Gen5PIDFrame &frame) const
  {
    return ((m_criteria.pid.ability == Ability::ANY) ||
            (m_criteria.pid.ability == frame.ability));
  }
  
  bool CheckGender(const Gen5PIDFrame &frame) const
  {
    return Gender::GenderValueMatches(frame.pid.GenderValue(),
                                      m_criteria.pid.gender,
                                      m_criteria.pid.genderRatio);
  }
  
  bool CheckESV(const Gen5PIDFrame &frame) const
  {
    switch (frame.encounterType)
    {
    case Encounter::GRASS_CAVE:
    case Encounter::SURFING:
    case Encounter::FISHING:
    case Encounter::DOUBLES_GRASS:
    case Encounter::SHAKING_GRASS:
    case Encounter::SWIRLING_DUST:
    case Encounter::BRIDGE_SHADOW:
    case Encounter::WATER_SPOT_SURFING:
    case Encounter::WATER_SPOT_FISHING:
      return ((m_criteria.esvMask[frame.seed.Season() - 1] &
              (0x1 << ESV::Slot(frame.esv))) != 0);
      
    case Encounter::SWARM:
      return (frame.esv == ESV::SWARM);
      
    case Encounter::STARTER_FOSSIL_GIFT:
    case Encounter::STATIONARY:
    case Encounter::ROAMER:
    case Encounter::LARVESTA_EGG:
    case Encounter::ENTRALINK:
    case Encounter::HIDDEN_GROTTO:
      return true;
      
    default:
      return false;
    }
  }
  
  const EncounterSeedSearcher::Criteria        &m_criteria;
  const EncounterSeedSearcher::ResultCallback  &m_resultHandler;
};

}

uint64_t EncounterSeedSearcher::Criteria::ExpectedNumberOfResults() const
{
  uint64_t  result = HashedSeedSearcher::Criteria::ExpectedNumberOfResults();
  if (result == 0)
    return 0;
  
  uint64_t  numNatures = pid.NumNatures();
  uint64_t  natureMultiplier = numNatures, natureDivisor = 25;
  
  uint64_t  numLeadAbilities = 0, mask = leadAbilityMask;
  if (mask > 0)
  {
    while (mask > 0)
    {
      if ((mask & 0x1) != 0)
        ++numLeadAbilities;
      mask >>= 1;
    }
  }
  else
  {
    numLeadAbilities = 1;
  }
  
  // if either gender is acceptable, cute charm generates double results
  if (((leadAbilityMask & (0x1 << LeadAbility::CUTE_CHARM)) != 0) &&
      (frameParameters.setGender == Gender::ANY))
    ++numLeadAbilities;
  
  if ((leadAbilityMask & (0x1 << LeadAbility::SYNCHRONIZE)) != 0)
  {
    // scale multiplier & divider by total number of lead abilities
    natureMultiplier *= numLeadAbilities;
    natureDivisor *= numLeadAbilities;
    
    // non-interesting natures will be synchronized half the time when
    // using synchronize, so bump multiplier appropriately
    natureMultiplier += (25 - numNatures) / 2;
  }
  
  uint64_t  abilityDivisor = (pid.ability < 2) ? 2 : 1;
  
  uint64_t  shinyMultiplier, shinyDivisor, pidFrameMultiplier;
  if (shinyOnly)
  {
    pidFrameMultiplier = pidFrame.max - pidFrame.min + 1;
    shinyMultiplier = frameParameters.hasShinyCharm ? 3 : 1;
    shinyDivisor = 8192;
  }
  else
  {
    pidFrameMultiplier = shinyMultiplier = shinyDivisor = 1;
  }
  
  result *= pidFrameMultiplier * natureMultiplier * shinyMultiplier *
            numLeadAbilities;
  result /= (natureDivisor * abilityDivisor * shinyDivisor);
  
  return result + 1;
}


void EncounterSeedSearcher::Search
  (const Criteria &criteria, const ResultCallback &resultHandler,
   SearchRunner::StatusHandler &statusHandler,
   const std::vector<uint64_t> &startingSeeds)
{
  HashedSeedSearcher  searcher;
  
  searcher.Search(criteria, IVFrameResultHandler(criteria, resultHandler),
                  statusHandler, startingSeeds);
}

}
