/*
  Copyright (C) 2011-2012 chiizu
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


#include "CGearSeedSearcher.h"
#include "SeedSearcher.h"

#include "IVSeedCache.h"

namespace pprng
{

namespace
{

class FastSearchSeedGenerator
{
public:
  typedef uint32_t  SeedType;
  typedef uint32_t  SeedCountType;
  
  enum { SeedsPerChunk = 1000 };
  
  FastSearchSeedGenerator()
    : m_dayMonthMinuteSecond(0xff000000), m_hour(0x00170000)
  {}
  
  SeedCountType NumberOfSeeds() const
  {
    return 256 * 24;
  }
  
  void SkipSeeds(uint64_t numSeeds)
  {
    while (numSeeds-- > 0)
      Next();
  }
  
  SeedType Next()
  {
    m_hour += 0x00010000;
    
    if (m_hour > 0x00170000)
    {
      m_hour = 0x00000000;
      m_dayMonthMinuteSecond += 0x01000000;
    }
    
    return m_dayMonthMinuteSecond | m_hour;
  }
  
private:
  uint32_t  m_dayMonthMinuteSecond;
  uint32_t  m_hour;
};

struct FrameChecker
{
  FrameChecker(const CGearSeedSearcher::Criteria &criteria)
    : m_criteria(criteria)
  {}
  
  bool operator()(const CGearIVFrame &frame) const
  {
    return m_criteria.ivs.CheckIVs(frame.ivs) &&
           m_criteria.ivs.CheckHiddenPower(frame.ivs);
  }
  
  const CGearSeedSearcher::Criteria  &m_criteria;
};

class FastSeedSearcher
{
public:
  typedef CGearIVFrame  ResultType;
  
  FastSeedSearcher(const IVSeedMap &seedMap,
                   const CGearSeedSearcher::Criteria &criteria)
    : m_seedMap(seedMap), m_frameRange(criteria.frameRange),
      m_minDelay(criteria.minDelay), m_maxDelay(criteria.maxDelay),
      m_macAddressLow(criteria.macAddressLow)
  {}
  
  void Search(uint32_t baseSeed, const FrameChecker &frameChecker,
              const CGearSeedSearcher::ResultCallback &resultHandler)
  {
    uint32_t  lowSeed = baseSeed + (m_minDelay & 0xFFFF) + m_macAddressLow;
    uint32_t  highSeed = baseSeed + (m_maxDelay & 0xFFFF) + m_macAddressLow;
    
    if (lowSeed <= highSeed)
    {
      SearchUntil(m_seedMap.lower_bound(lowSeed),
                  m_seedMap.upper_bound(highSeed), frameChecker, resultHandler);
    }
    else
    {
      SearchUntil(m_seedMap.lower_bound(lowSeed), m_seedMap.end(),
                  frameChecker, resultHandler);
      SearchUntil(m_seedMap.begin(), m_seedMap.upper_bound(highSeed),
                  frameChecker, resultHandler);
    }
  }
  
private:
  void SearchUntil(IVSeedMap::const_iterator i, IVSeedMap::const_iterator end,
                   const FrameChecker &frameChecker,
                   const CGearSeedSearcher::ResultCallback &resultHandler)
  {
    while (i != end)
    {
      if ((i->second.frame >= (m_frameRange.min + 2)) &&
          (i->second.frame <= (m_frameRange.max + 2)))
      {
        CGearIVFrame  frame;
        
        frame.seed = i->first;
        frame.number = i->second.frame - 2;
        frame.ivs = i->second.ivWord;
        
        if (frameChecker(frame))
          resultHandler(frame);
      }
      
      ++i;
    }
  }
  
  const IVSeedMap                   &m_seedMap;
  const SearchCriteria::FrameRange  &m_frameRange;
  const uint32_t                    m_minDelay, m_maxDelay, m_macAddressLow;
};


struct FrameGeneratorFactory
{
  typedef CGearIVFrameGenerator  FrameGenerator;
  
  FrameGeneratorFactory(CGearIVFrameGenerator::FrameType frameType)
    : m_frameType(frameType)
  {}
  
  CGearIVFrameGenerator operator()(uint32_t seed) const
  {
    return CGearIVFrameGenerator(seed, m_frameType);
  }
  
  const CGearIVFrameGenerator::FrameType m_frameType;
};

}

uint64_t CGearSeedSearcher::Criteria::ExpectedNumberOfResults() const
{
  uint64_t  delays = maxDelay - minDelay + 1;
  
  uint64_t  numSeeds = delays * 256 * 24;
  
  uint64_t  numFrames = frameRange.max - frameRange.min + 1;
  
  uint64_t  numIVs = IVs::CalculateNumberOfCombinations(ivs.min, ivs.max);
  
  uint64_t  numResults = numFrames * numSeeds * numIVs /
                           (32 * 32 * 32 * 32 * 32 * 32);
  
  numResults = IVs::AdjustExpectedResultsForHiddenPower
    (numResults, ivs.min, ivs.max, ivs.hiddenTypeMask, ivs.minHiddenPower);
  
  return numResults;
}

void CGearSeedSearcher::Search
  (const Criteria &criteria, const ResultCallback &resultHandler,
   SearchRunner::StatusHandler &statusHandler)
{
  FrameChecker  frameChecker(criteria);
  SearchRunner  searcher;
  
  if ((criteria.ivs.pattern == IVPattern::CUSTOM) ||
      ((criteria.frameRange.min + 2) > IVSeedMapMaxFrame) ||
      ((criteria.frameRange.max + 2) > IVSeedMapMaxFrame) ||
      criteria.ivs.isRoamer)
  {
    CGearSeedGenerator     seedGenerator(criteria.minDelay,
                                         criteria.maxDelay,
                                         criteria.macAddressLow);
    
    FrameGeneratorFactory  frameGenFactory(criteria.ivs.isRoamer ?
                                           CGearIVFrameGenerator::Roamer :
                                           CGearIVFrameGenerator::Normal);
  
    SeedFrameSearcher<FrameGeneratorFactory>  seedSearcher(frameGenFactory,
                                                           criteria.frameRange);
    
    searcher.Search(seedGenerator, seedSearcher, frameChecker, resultHandler,
                    statusHandler);
  }
  else
  {
    FastSearchSeedGenerator  seedGenerator;
    FastSeedSearcher         seedSearcher(GetIVSeedMap(criteria.ivs.pattern),
                                          criteria);
    
    searcher.Search(seedGenerator, seedSearcher, frameChecker, resultHandler,
                    statusHandler);
  }
}

}
