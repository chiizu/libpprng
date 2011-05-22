/*
  Copyright (C) 2011 chiizu
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


#include "EggSeedSearcher.h"
#include "SeedGenerator.h"

#include <vector>

namespace pprng
{

namespace
{

struct IVRange
{
  IVs       minIVs, maxIVs;
  uint32_t  inheritancePatterns;
  
  IVRange(IVs min, IVs max, uint32_t i)
    : minIVs(min), maxIVs(max), inheritancePatterns(i)
  {}
};

std::vector<IVRange> GenerateIVRanges(IVs parent1IVs, IVs parent2IVs,
                                      IVs minEggIVs, IVs maxEggIVs)
{
  std::vector<IVRange>  result;
  
  uint32_t  minIV, maxIV, iv1, iv2;
  uint32_t  i, j, k, ic, jc, kc;
  
  for (i = 0; i < 4; ++i)
  {
    ic = 0;
    
    minIV = minEggIVs.iv(i);  maxIV = maxEggIVs.iv(i);
    iv1 = parent1IVs.iv(i);   iv2 = parent2IVs.iv(i);
    
    ic += ((iv1 >= minIV) && (iv1 <= maxIV)) ? 1 : 0;
    ic += ((iv2 >= minIV) && (iv2 <= maxIV)) ? 1 : 0;
    if (ic > 0)
    {
      for (j = i + 1; j < 5; ++j)
      {
        jc = 0;
        
        minIV = minEggIVs.iv(j);  maxIV = maxEggIVs.iv(j);
        iv1 = parent1IVs.iv(j);   iv2 = parent2IVs.iv(j);
        
        jc += ((iv1 >= minIV) && (iv1 <= maxIV)) ? 1 : 0;
        jc += ((iv2 >= minIV) && (iv2 <= maxIV)) ? 1 : 0;
        
        if (jc > 0)
        {
          for (k = j + 1; k < 6; ++k)
          {
            kc = 0;
            
            minIV = minEggIVs.iv(k);  maxIV = maxEggIVs.iv(k);
            iv1 = parent1IVs.iv(k);   iv2 = parent2IVs.iv(k);
            
            kc += ((iv1 >= minIV) && (iv1 <= maxIV)) ? 1 : 0;
            kc += ((iv2 >= minIV) && (iv2 <= maxIV)) ? 1 : 0;
            
            if (kc > 0)
            {
              IVs  min = minEggIVs, max = maxEggIVs;
              
              min.setIV(i, 0);  min.setIV(j, 0);  min.setIV(k, 0);
              max.setIV(i, 31); max.setIV(j, 31); max.setIV(k, 31);
              
              result.push_back(IVRange(min, max, ic * jc * kc));
            }
          }
        }
      }
    }
  }
  
  return result;
}


struct IVFrameChecker
{
  IVFrameChecker(const EggSeedSearcher::Criteria &criteria)
    : m_possibleIVs(GenerateIVRanges(criteria.femaleIVs, criteria.maleIVs,
                                     criteria.minIVs,
                                     criteria.shouldCheckMaxIVs ?
                                       criteria.maxIVs : IVs(0x7fff7fff)))
  {}
  
  bool operator()(const HashedIVFrame &frame) const
  {
    std::vector<IVRange>::const_iterator  i;
    for (i = m_possibleIVs.begin(); i != m_possibleIVs.end(); ++i)
    {
      if (frame.ivs.betterThanOrEqual(i->minIVs) &&
          frame.ivs.worseThanOrEqual(i->maxIVs))
      {
        return true;
      }
    }
    
    return false;
  }
  
  const std::vector<IVRange>  m_possibleIVs;
};




struct IVFrameResultHandler
{
  IVFrameResultHandler(const EggSeedSearcher::Criteria &criteria,
                       const EggSeedSearcher::ResultCallback &resultHandler)
    : m_criteria(criteria), m_resultHandler(resultHandler)
  {}
  
  void operator()(const HashedIVFrame &frame) const
  {
    Gen5BreedingFrameGenerator  generator
      (frame.seed, m_criteria.internationalParents,
       m_criteria.usingEverstone, m_criteria.usingDitto,
       m_criteria.tid, m_criteria.sid);
    
    uint32_t  frameNum = 0, limitFrame = m_criteria.minPIDFrame - 1;
    while (frameNum < limitFrame)
    {
      generator.AdvanceFrame();
      ++frameNum;
    }
    
    while (frameNum < m_criteria.maxPIDFrame)
    {
      generator.AdvanceFrame();
      ++frameNum;
      
      Gen5BreedingFrame  breedingFrame = generator.CurrentFrame();
      
      if (CheckShiny(breedingFrame.pid) && CheckNature(breedingFrame) &&
          CheckAbility(breedingFrame))
      {
        Gen5EggFrame  eggFrame(generator.CurrentFrame(),
                               frame.number, frame.ivs,
                               m_criteria.femaleIVs, m_criteria.maleIVs);
        
        if (CheckIVs(eggFrame.ivs) && CheckHiddenPower(eggFrame.ivs))
        {
          m_resultHandler(eggFrame);
        }
      }
    }
  }
  
  bool CheckShiny(PID pid) const
  {
    return !m_criteria.shinyOnly || pid.IsShiny(m_criteria.tid, m_criteria.sid);
  }
  
  bool CheckNature(const Gen5BreedingFrame &frame) const
  {
    return (m_criteria.nature == Nature::ANY) ||
           (m_criteria.usingEverstone && frame.everstoneActivated) ||
           (m_criteria.nature == frame.nature);
  }
  
  bool CheckAbility(const Gen5BreedingFrame &frame) const
  {
    return ((m_criteria.ability > 1) ||
            (m_criteria.ability == frame.pid.Gen5Ability())) &&
           (!m_criteria.inheritsDreamworldAbility ||
            frame.dreamWorldAbilityPassed);
  }
  
  bool CheckIVs(const IVs &ivs) const
  {
    return ivs.betterThanOrEqual(m_criteria.minIVs) &&
           (!m_criteria.shouldCheckMaxIVs ||
            ivs.worseThanOrEqual(m_criteria.maxIVs));
  }

  bool CheckHiddenPower(const IVs &ivs) const
  {
    if (m_criteria.hiddenType == Element::UNKNOWN)
    {
      return true;
    }
    
    if ((m_criteria.hiddenType == Element::ANY) ||
        (m_criteria.hiddenType == ivs.HiddenType()))
    {
      return ivs.HiddenPower() >= m_criteria.minHiddenPower;
    }
    
    return false;
  }
  
  const EggSeedSearcher::Criteria        &m_criteria;
  const EggSeedSearcher::ResultCallback  &m_resultHandler;
};


struct IVFrameGeneratorFactory
{
  HashedIVFrameGenerator operator()(const HashedSeed &seed) const
  {
    return HashedIVFrameGenerator(seed, HashedIVFrameGenerator::Normal);
  }
};

}

uint64_t EggSeedSearcher::Criteria::ExpectedNumberOfResults()
{
  IVs  maxIVs = shouldCheckMaxIVs ? this->maxIVs : IVs(0x7FFF7FFF);
  
  std::vector<IVRange>  ivRanges =
    GenerateIVRanges(femaleIVs, maleIVs, minIVs, maxIVs);
  
  if (ivRanges.size() == 0)
    return 0;
  
  
  uint64_t  seconds = (toTime - fromTime).total_seconds() + 1;
  uint64_t  keyCombos = buttonPresses.size();
  uint64_t  timer0Values = (timer0High - timer0Low) + 1;
  uint64_t  vcountValues = (vcountHigh - vcountLow) + 1;
  uint64_t  vframeValues = (vframeHigh - vframeLow) + 1;
  
  uint64_t  numSeeds =
    seconds * keyCombos * timer0Values * vcountValues * vframeValues;
  
  uint64_t  numIVFrames = maxIVFrame - minIVFrame + 1;
  
  uint64_t  hpDivisor = 1;
  if (hiddenType != Element::UNKNOWN)
  {
    hpDivisor = 1; // number of power levels is 40, but...
    
    if (hiddenType != Element::ANY)
    {
      hpDivisor *= 16;
    }
  }
  
  uint64_t  natureMultiplier, natureDivisor;
  if (nature != Nature::ANY)
  {
    natureMultiplier = usingEverstone ? 13 : 1;
    natureDivisor = 25;
  }
  else
  {
    natureMultiplier = natureDivisor = 1;
  }
  
  uint64_t  abilityDivisor = (ability < 2) ? 2 : 1;
  uint64_t  dwMultiplier, dwDivisor;
  if (inheritsDreamworldAbility && !usingDitto)
  {
    dwMultiplier = 3;
    dwDivisor = 5;
  }
  else
  {
    dwMultiplier = dwDivisor = 1;
  }
  uint64_t  shinyMultiplier, shinyDivisor;
  if (shinyOnly)
  {
    shinyMultiplier = internationalParents ? 6 : 1;
    shinyDivisor = 8192;
  }
  else
  {
    shinyMultiplier = shinyDivisor = 1;
  }
  
  uint64_t  numPIDFrames = maxPIDFrame - minPIDFrame + 1;
  
  uint64_t  multiplier = numIVFrames * numSeeds * numPIDFrames *
                         natureMultiplier * dwMultiplier * shinyMultiplier;
  
  uint64_t  divisor = hpDivisor * natureDivisor * abilityDivisor * dwDivisor *
                      shinyDivisor * 32UL * 32UL * 32UL * 32UL * 32UL * 32UL;
  
  
  std::vector<IVRange>::const_iterator  i;
  uint64_t                              ivMatches = 0UL;
  for (i = ivRanges.begin(); i != ivRanges.end(); ++i)
  {
    uint64_t  ivSets = (i->maxIVs.hp() - i->minIVs.hp() + 1) *
                       (i->maxIVs.at() - i->minIVs.at() + 1) *
                       (i->maxIVs.df() - i->minIVs.df() + 1) *
                       (i->maxIVs.sa() - i->minIVs.sa() + 1) *
                       (i->maxIVs.sd() - i->minIVs.sd() + 1) *
                       (i->maxIVs.sp() - i->minIVs.sp() + 1);
    
    ivMatches += ivSets * i->inheritancePatterns /
                 /* total inheritance patterns * 6c3 */ 160UL;
  }
  
  uint64_t  result = ivMatches * multiplier / divisor;
  
  return result + 1;
}

void EggSeedSearcher::Search(const Criteria &criteria,
                             const ResultCallback &resultHandler,
                             const ProgressCallback &progressHandler)
{
  HashedSeedGenerator   seedGenerator(criteria.version,
                                      criteria.macAddressLow,
                                      criteria.macAddressHigh,
                                      criteria.timer0Low, criteria.timer0High,
                                      criteria.vcountLow, criteria.vcountHigh,
                                      criteria.vframeLow, criteria.vframeHigh,
                                      criteria.fromTime, criteria.toTime,
                                      criteria.buttonPresses);
  
  IVFrameGeneratorFactory   ivFrameGenFactory;
  
  IVFrameChecker            ivFrameChecker(criteria);
  
  IVFrameResultHandler      ivFrameResultHandler(criteria, resultHandler);
  
  typedef SeedSearcher<HashedIVFrameGenerator>  SearcherType;
  
  SearcherType::FrameRange  ivFrameRange
                              (criteria.minIVFrame, criteria.maxIVFrame);
  
  SearcherType              searcher;
  
  searcher.Search(seedGenerator, ivFrameGenFactory,
                  ivFrameRange, ivFrameChecker,
                  ivFrameResultHandler, progressHandler);
}

}
