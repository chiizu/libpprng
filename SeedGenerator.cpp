/*
  Copyright (C) 2011-2014 chiizu
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


#include "SeedGenerator.h"

#include "HashedSeedCalculator.h"
#include "LinearCongruentialRNG.h"

using namespace boost::posix_time;
using namespace boost::gregorian;

namespace pprng
{

namespace
{

static bool ShouldIterateHpAtkDef(IVs minIVs, IVs maxIVs)
{
  IVs tempMin, tempMax;
  
  tempMin.SetFromGameDataWord(minIVs.GameDataWord() & 0x7fff0000);
  tempMax.SetFromGameDataWord(maxIVs.GameDataWord() & 0x7fff0000);
  
  uint32_t  numCombosHi = IVs::CalculateNumberOfCombinations(tempMin, tempMax);
  
  tempMin.SetFromGameDataWord(minIVs.GameDataWord() & 0x00007fff);
  tempMax.SetFromGameDataWord(maxIVs.GameDataWord() & 0x00007fff);
  
  uint32_t  numCombosLo = IVs::CalculateNumberOfCombinations(tempMin, tempMax);
  
  return numCombosLo < numCombosHi;
}

}

Gen34IVSeedGenerator::Gen34IVSeedGenerator(IVs minIVs, IVs maxIVs,
                                           uint32_t method)
  : m_minIVs(minIVs), m_maxIVs(maxIVs), m_method(method),
    m_iteratingHpAtDef(ShouldIterateHpAtkDef(minIVs, maxIVs)),
    m_numRollbacks(m_iteratingHpAtDef ?
                    (((m_method == 2) || (m_method == 3)) ? 1 : 0) :
                    ((m_method == 4) ? 2 : 1)),
    m_iv0(0), m_iv1(0), m_iv2(0),
    m_iv0Low(0), m_iv0High(0), m_iv1Low(0), m_iv1High(0),
    m_iv2Low(0), m_iv2High(0),
    m_otherHalfCounter(0x1ffff)
{
  if (m_iteratingHpAtDef)
  {
    m_iv0Low = minIVs.hp(); m_iv0High = maxIVs.hp();
    m_iv1Low = minIVs.at(); m_iv1High = maxIVs.at();
    m_iv2Low = minIVs.df(); m_iv2High = maxIVs.df();
  }
  else
  {
    m_iv0Low = minIVs.sp(); m_iv0High = maxIVs.sp();
    m_iv1Low = minIVs.sa(); m_iv1High = maxIVs.sa();
    m_iv2Low = minIVs.sd(); m_iv2High = maxIVs.sd();
  }
  
  m_iv0 = m_iv0High;
  m_iv1 = m_iv1High;
  m_iv2 = m_iv2High;
}

Gen34IVSeedGenerator::SeedCountType Gen34IVSeedGenerator::NumberOfSeeds() const
{
  uint32_t  mask = m_iteratingHpAtDef ? 0x00007fff : 0x7fff0000;
  
  IVs  halfMin, halfMax;
  
  halfMin.SetFromGameDataWord(m_minIVs.GameDataWord() & mask);
  halfMax.SetFromGameDataWord(m_maxIVs.GameDataWord() & mask);
  
  return IVs::CalculateNumberOfCombinations(halfMin, halfMax) * (0x1ffff + 1);
}

void Gen34IVSeedGenerator::SkipSeeds(uint64_t numSeeds)
{
  while (numSeeds > 0)
  {
    if (++m_otherHalfCounter > 0x1ffff)
    {
      m_otherHalfCounter = 0;
      if (++m_iv0 > m_iv0High)
      {
        m_iv0 = m_iv0Low;
        if (++m_iv1 > m_iv1High)
        {
          m_iv1 = m_iv1Low;
          if (++m_iv2 > m_iv2High)
          {
            m_iv2 = m_iv2Low;
          }
        }
      }
    }
    
    --numSeeds;
  }
}

Gen34IVSeedGenerator::SeedType Gen34IVSeedGenerator::Next()
{
  SkipSeeds(1);
  
  uint32_t  seed = ((m_otherHalfCounter & 0x10000) << 15) | // unused bit
    (m_iv2 << 26) | (m_iv1 << 21) | (m_iv0 << 16) | // ivs from upper 16 bits
    (m_otherHalfCounter & 0xffff); // lower 16 bits unknown
  
  // back up a number of frames to account for which IVs are being generated
  // and the generation method
  uint32_t  i = m_numRollbacks;
  while (i > 0)
  {
    seed = LCRNG34_R::NextForSeed(seed);
    --i;
  }
  
  // back up 2 frames for PID generation
  seed = LCRNG34_R::NextForSeed(seed);
  seed = LCRNG34_R::NextForSeed(seed);
  
  // backup 1 frame to the 'seed'
  seed = LCRNG34_R::NextForSeed(seed);
  
  return seed;
}


HashedSeedGenerator::HashedSeedGenerator
  (const HashedSeedGenerator::Parameters &parameters,
   SeedCountType seedsPerChunk)
: m_parameters(parameters), m_seedsPerChunk(seedsPerChunk),
  m_seedMessage(parameters.ToInitialSeedParameters(),
                parameters.excludedSeasonMask),
  m_timer0(parameters.timer0High), m_vcount(parameters.vcountHigh),
  m_vframe(parameters.vframeHigh),
  m_heldButtonsIter(m_parameters.heldButtons.end() - 1)
#ifdef __ARM_NEON__
  ,
  m_seedParameters(parameters.ToInitialSeedParameters()),
  m_paramsHeldButtonsIter(m_heldButtonsIter),
  m_index(4)
#endif
{}

HashedSeedGenerator::HashedSeedGenerator(const HashedSeedGenerator &other)
: m_parameters(other.m_parameters), m_seedsPerChunk(other.m_seedsPerChunk),
  m_seedMessage(other.m_seedMessage),
  m_timer0(other.m_timer0), m_vcount(other.m_vcount), m_vframe(other.m_vframe),
  m_heldButtonsIter(m_parameters.heldButtons.begin() +
                    (other.m_heldButtonsIter -
                     other.m_parameters.heldButtons.begin()))
#ifdef __ARM_NEON__
  ,
  m_seedParameters(other.m_seedParameters),
  m_paramsHeldButtonsIter(m_parameters.heldButtons.begin() +
                          (other.m_paramsHeldButtonsIter -
                           other.m_parameters.heldButtons.begin())),
  m_index(other.m_index)
#endif
{
#ifdef __ARM_NEON__
  for (int i = 0; i < 4; ++i)
    m_rawSeeds[i] = other.m_rawSeeds[i];
#endif
}

HashedSeed::Parameters
  HashedSeedGenerator::Parameters::ToInitialSeedParameters() const
{
  HashedSeed::Parameters  parameters;
  
  parameters.gameColor = gameColor;
  parameters.gameLanguage = gameLanguage;
  parameters.consoleType = consoleType;
  parameters.macAddress = macAddress;
  parameters.timer0 = timer0High;
  parameters.vcount = vcountHigh;
  parameters.vframe = vframeHigh;
  
  ptime  dt = fromTime;
  
  if (excludedSeasonMask != 0)
  {
    date   d = dt.date();
    
    while (((Season::MaskForMonth(d.month()) & excludedSeasonMask) != 0) &&
           (dt < toTime))
    {
      ptime  nextDT(d.end_of_month() + days(1), seconds(0));
      
      if (nextDT > toTime)
        nextDT = toTime + seconds(1);
      
      dt = nextDT;
      d = dt.date();
    }
  }
  
  dt = dt - seconds(1);
  time_duration  t = dt.time_of_day();
  
  parameters.date = dt.date();
  parameters.hour = t.hours();
  parameters.minute = t.minutes();
  parameters.second = t.seconds();
  
  return parameters;
}

uint32_t HashedSeedGenerator::Parameters::NumberOfSeconds() const
{
  SeedCountType  totalSeconds = 0;
  
  if (excludedSeasonMask != 0)
  {
    ptime  dt = fromTime;
    
    while (dt < toTime)
    {
      date   d = dt.date();
      ptime  nextDT(d.end_of_month() + days(1), seconds(0));
      
      if (nextDT > toTime)
        nextDT = toTime + seconds(1);
      
      if ((Season::MaskForMonth(d.month()) & excludedSeasonMask) == 0)
        totalSeconds += (nextDT - dt).total_seconds();
      
      dt = nextDT;
    }
  }
  else
  {
    totalSeconds = (toTime - fromTime).total_seconds() + 1;
  }
  
  return totalSeconds;
}

HashedSeedGenerator::SeedCountType
  HashedSeedGenerator::Parameters::NumberOfSeeds() const
{
  SeedCountType  seconds = NumberOfSeconds();
  SeedCountType  buttonCombos = heldButtons.size();
  SeedCountType  timer0Values = (timer0High - timer0Low) + 1;
  SeedCountType  vcountValues = (vcountHigh - vcountLow) + 1;
  SeedCountType  vframeValues = (vframeHigh - vframeLow) + 1;
  
  return seconds * buttonCombos * timer0Values * vcountValues * vframeValues;
}

HashedSeedGenerator::SeedCountType HashedSeedGenerator::NumberOfSeeds() const
{
  return m_parameters.NumberOfSeeds();
}


static ptime SkipSeconds(ptime dt, uint32_t skippedSeconds,
                         uint32_t excludedSeasonMask)
{
  if (excludedSeasonMask != 0)
  {
    while (skippedSeconds > 0)
    {
      date      d = dt.date();
      ptime     nextDT(d.end_of_month() + days(1), seconds(0));
      uint32_t  monthSeconds = (nextDT - dt).total_seconds();
      
      if ((Season::MaskForMonth(d.month()) & excludedSeasonMask) == 0)
      {
        if (monthSeconds > skippedSeconds)
          monthSeconds = skippedSeconds;
        
        skippedSeconds -= monthSeconds;
      }
      
      dt = dt + seconds(monthSeconds);
    }
  }
  else
  {
    dt = dt + seconds(skippedSeconds);
  }
  
  return dt;
}


void HashedSeedGenerator::SkipSeeds(uint64_t numSeeds)
{
  uint64_t  numHeldButtons = m_parameters.heldButtons.size();
  uint64_t  numTimer0 = (m_parameters.timer0High - m_parameters.timer0Low) + 1;
  uint64_t  numVcount = (m_parameters.vcountHigh - m_parameters.vcountLow) + 1;
  uint64_t  numVframe = (m_parameters.vframeHigh - m_parameters.vframeLow) + 1;
  
  uint64_t  divisor = numHeldButtons * numTimer0 * numVcount * numVframe;
  uint64_t  skippedSeconds = numSeeds / divisor;
  
  numSeeds = numSeeds % divisor;
  
  {
    ptime  dt(m_seedMessage.GetDate(), hours(m_seedMessage.GetHour()) +
                                       minutes(m_seedMessage.GetMinute()) +
                                       seconds(m_seedMessage.GetSecond()));
    
    dt = SkipSeconds(dt, skippedSeconds, m_parameters.excludedSeasonMask);
    
    m_seedMessage.SetDate(dt.date());
    
    time_duration  t = dt.time_of_day();
    m_seedMessage.SetHour(t.hours());
    m_seedMessage.SetMinute(t.minutes());
    m_seedMessage.SetSecond(t.seconds());
  }
  
  divisor = numHeldButtons * numTimer0 * numVcount;
  
  uint64_t  skippedVframes = numSeeds / divisor;
  
  numSeeds = numSeeds % divisor;
  divisor = numHeldButtons * numTimer0;
  
  uint32_t  vframeChange = skippedVframes % numVframe;
  uint64_t  skippedVcounts = numSeeds / divisor;
  
  numSeeds = numSeeds % divisor;
  divisor = numHeldButtons;
  
  m_vframe += vframeChange;
  if (m_vframe > m_parameters.vframeHigh)
    m_vframe -= numVframe;
  
  uint32_t  vcountChange = skippedVcounts % numVcount;
  
  m_seedMessage.SetVFrame(m_vframe);
  
  uint64_t  skippedTimer0s = numSeeds / divisor;
  uint64_t  skippedButtons = numSeeds % divisor;
  
  m_vcount += vcountChange;
  if (m_vcount > m_parameters.vcountHigh)
    m_vcount -= numVcount;
  
  uint32_t  timer0Change = skippedTimer0s % numTimer0;
  
  m_seedMessage.SetVCount(m_vcount);
  
  uint32_t  newButtonPos =
    (m_heldButtonsIter - m_parameters.heldButtons.begin()) + skippedButtons;
  if (newButtonPos >= numHeldButtons)
    newButtonPos -= numHeldButtons;
  
  m_timer0 += timer0Change;
  if (m_timer0 > m_parameters.timer0High)
    m_timer0 -= numTimer0;
  
  m_seedMessage.SetTimer0(m_timer0);
  
  m_heldButtonsIter = m_parameters.heldButtons.begin() + newButtonPos;
  
  m_seedMessage.SetHeldButtons(*m_heldButtonsIter);
}


void HashedSeedGenerator::AdvanceMessageParameters()
{
  if (++m_heldButtonsIter == m_parameters.heldButtons.end())
  {
    m_heldButtonsIter = m_parameters.heldButtons.begin();
    
    if (++m_timer0 > m_parameters.timer0High)
    {
      m_timer0 = m_parameters.timer0Low;
      
      if (++m_vcount > m_parameters.vcountHigh)
      {
        m_vcount = m_parameters.vcountLow;
        
        if (++m_vframe > m_parameters.vframeHigh)
        {
          m_vframe = m_parameters.vframeLow;
          
          m_seedMessage.NextSecond();
        }
        
        m_seedMessage.SetVFrame(m_vframe);
      }
      
      m_seedMessage.SetVCount(m_vcount);
    }
    
    m_seedMessage.SetTimer0(m_timer0);
  }
  
  m_seedMessage.SetHeldButtons(*m_heldButtonsIter);
}


HashedSeedGenerator::SeedType HashedSeedGenerator::Next()
{
#ifdef __ARM_NEON__
  if (m_index > 3)
  {
    // calculate next four seeds
    HashedSeedCalculator::MessageVariables  vars;
    
    for (int i = 0; i < 4; ++i)
    {
      AdvanceMessageParameters();
      
      const HashedSeedMessage::MessageData  &d(m_seedMessage.GetMessageData());
      
      vars.m5[i] = d[5];
      vars.m7[i] = d[7];
      vars.m8[i] = d[8];
      vars.m9[i] = d[9];
      vars.m12[i] = d[12];
    }
    
    HashedSeedCalculator::CalculateRawSeeds(m_seedMessage, vars, m_rawSeeds);
    m_index = 0;
  }
  
  // update parameters for next seed
  if (++m_paramsHeldButtonsIter == m_parameters.heldButtons.end())
  {
    m_paramsHeldButtonsIter = m_parameters.heldButtons.begin();
    
    if (++m_seedParameters.timer0 > m_parameters.timer0High)
    {
      m_seedParameters.timer0 = m_parameters.timer0Low;
      
      if (++m_seedParameters.vcount > m_parameters.vcountHigh)
      {
        m_seedParameters.vcount = m_parameters.vcountLow;
        
        if (++m_seedParameters.vframe > m_parameters.vframeHigh)
        {
          m_seedParameters.vframe = m_parameters.vframeLow;
          
          if (++m_seedParameters.second > 59)
          {
            m_seedParameters.second = 0;
            
            if (++m_seedParameters.minute > 59)
            {
              m_seedParameters.minute = 0;
              
              if (++m_seedParameters.hour > 23)
              {
                m_seedParameters.hour = 0;
                
                m_seedParameters.date = m_seedParameters.date +
                  boost::gregorian::days(1);
              }
            }
          }
        }
      }
    }
  }
  
  m_seedParameters.heldButtons = *m_paramsHeldButtonsIter;
  
  // return cached seed with parameters
  return HashedSeed(m_seedParameters, m_rawSeeds[m_index++]);
  
#else
  
  AdvanceMessageParameters();
  
  HashedSeed  seed = m_seedMessage.AsHashedSeed();
  
  return seed;
#endif
}


std::list<HashedSeedGenerator> HashedSeedGenerator::Split(uint32_t parts)
{
  std::list<HashedSeedGenerator>  result;
  
  HashedSeedGenerator::Parameters  p = m_parameters;
  
  uint32_t  totalSeconds = m_parameters.NumberOfSeconds();
  
  if (parts > totalSeconds)
    parts = totalSeconds;
  
  if (parts == 0)
    return result;
  
  uint32_t  partSeconds = (totalSeconds + parts - 1) / parts;
  ptime     fromTime = m_parameters.fromTime;
  ptime     toTime = SkipSeconds(fromTime, partSeconds - 1,
                                 m_parameters.excludedSeasonMask);
  
  for (uint32_t i = 0; i < parts; ++i)
  {
    p.fromTime = fromTime;
    p.toTime = toTime;
    
    HashedSeedGenerator  part(p, m_seedsPerChunk);
    
    result.push_back(part);
    
    fromTime = SkipSeconds(toTime, 1, m_parameters.excludedSeasonMask);
    toTime = (i == (parts - 1)) ? m_parameters.toTime :
               SkipSeconds(toTime, partSeconds,
                           m_parameters.excludedSeasonMask);
  }
  
  return result;
}

}
