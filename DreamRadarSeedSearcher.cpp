/*
  Copyright (C) 2012 chiizu
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


#include "DreamRadarSeedSearcher.h"
#include "SeedSearcher.h"

#include "IVSeedCache.h"

namespace pprng
{

namespace
{

struct FrameChecker
{
  FrameChecker(const DreamRadarSeedSearcher::Criteria &criteria)
    : m_criteria(criteria)
  {}
  
  bool operator()(const DreamRadarFrame &frame) const
  {
    return m_criteria.pid.CheckNature(frame.nature) &&
           m_criteria.ivs.CheckIVs(frame.ivs) &&
           m_criteria.ivs.CheckHiddenPower(frame.ivs);
  }
  
  const DreamRadarSeedSearcher::Criteria  &m_criteria;
};


struct SeedMapSearcher
{
  typedef DreamRadarFrame  ResultType;
  
  SeedMapSearcher(const IVSeedMap &seedMap,
                  const DreamRadarSeedSearcher::Criteria &criteria)
    : m_seedMap(seedMap), m_criteria(criteria),
      m_lowIVFrame(criteria.frameParameters.GetBaseIVFrameNumber
                                              (criteria.frame.min)),
      m_highIVFrame(m_lowIVFrame +
                    ((criteria.frame.max - criteria.frame.min + 1) * 2))
  {}
  
  void Search(const HashedSeed &seed, const FrameChecker &frameChecker,
              const DreamRadarSeedSearcher::ResultCallback &resultHandler)
  {
    uint32_t  ivSeed = seed.rawSeed >> 32;
    uint32_t  limit = m_lowIVFrame;
    
    IVSeedMap::const_iterator  i = m_seedMap.lower_bound(ivSeed);
    IVSeedMap::const_iterator  end = m_seedMap.upper_bound(ivSeed);
    while ((i != end) && (i->second.frame < limit))
      ++i;
    
    limit = m_highIVFrame;
    HashedIVFrame  ivFrame(seed);
    while ((i != end) && (i->second.frame <= limit))
    {
      ivFrame.number = i->second.frame;
      ivFrame.ivs.SetFromGameDataWord(i->second.ivWord);
      
      if (((ivFrame.number & 0x1) == (m_lowIVFrame & 0x1)) &&
          frameChecker.m_criteria.ivs.CheckIVs(ivFrame.ivs) &&
          frameChecker.m_criteria.ivs.CheckHiddenPower(ivFrame.ivs))
      {
        DreamRadarFrame  result =
          DreamRadarFrameGenerator::FrameFromIVFrame
            (ivFrame, m_criteria.frameParameters);
        
        if (frameChecker.m_criteria.pid.CheckNature(result.nature))
          resultHandler(result);
      }
      
      ++i;
    }
  }
  
  const IVSeedMap                         &m_seedMap;
  const DreamRadarSeedSearcher::Criteria  &m_criteria;
  const uint32_t                          m_lowIVFrame, m_highIVFrame;
};


struct FrameGeneratorFactory
{
  typedef DreamRadarFrameGenerator  FrameGenerator;
  
  FrameGeneratorFactory(const DreamRadarSeedSearcher::Criteria &criteria)
    : m_criteria(criteria)
  {}
  
  DreamRadarFrameGenerator operator()(const HashedSeed &seed) const
  {
    return DreamRadarFrameGenerator(seed, m_criteria.frameParameters);
  }
  
  const DreamRadarSeedSearcher::Criteria  &m_criteria;
};

}

uint64_t DreamRadarSeedSearcher::Criteria::ExpectedNumberOfResults() const
{
  uint64_t  numSeeds = seedParameters.NumberOfSeeds();
  if (numSeeds == 0)
    return 0;
  
  uint64_t  numFrames = frame.max - frame.min + 1;
  
  uint64_t  numIVs = IVs::CalculateNumberOfCombinations(ivs.min, ivs.max);
  
  uint64_t  natureMultiplier = pid.NumNatures(), natureDivisor = 25;
  
  uint64_t  numResults = numSeeds * numFrames * numIVs * natureMultiplier /
                         (32 * 32 * 32 * 32 * 32 * 32 * natureDivisor);
  
  numResults = IVs::AdjustExpectedResultsForHiddenPower
    (numResults, ivs.min, ivs.max, ivs.hiddenTypeMask, ivs.minHiddenPower);
  
  return numResults + 1;
}

void DreamRadarSeedSearcher::Search
  (const Criteria &criteria, const ResultCallback &resultHandler,
   SearchRunner::StatusHandler &searchStatusHandler,
   const std::vector<uint64_t> &startingSeeds)
{
  HashedSeedGenerator         seedGenerator(criteria.seedParameters);
  FrameGeneratorFactory       frameGeneratorFactory(criteria);
  FrameChecker                frameChecker(criteria);
  SearchRunner                searcher;
  
  if ((criteria.ivs.pattern != IVPattern::CUSTOM) &&
      (criteria.frameParameters.GetBaseIVFrameNumber(criteria.frame.min) <=
        IVSeedMapMaxFrame) &&
      (criteria.frameParameters.GetBaseIVFrameNumber(criteria.frame.max) <=
        IVSeedMapMaxFrame))
  {
    SeedMapSearcher  seedSearcher(GetIVSeedMap(criteria.ivs.pattern), criteria);
    
    if (startingSeeds.size() > 0)
      searcher.ContinueSearchThreaded(seedGenerator, seedSearcher, frameChecker,
                                      resultHandler, searchStatusHandler,
                                      startingSeeds);
    else
      searcher.SearchThreaded(seedGenerator, seedSearcher, frameChecker,
                              resultHandler, searchStatusHandler);
  }
  else
  {
    SeedFrameSearcher<FrameGeneratorFactory> seedSearcher(frameGeneratorFactory,
                                                          criteria.frame);
    
    if (startingSeeds.size() > 0)
      searcher.ContinueSearchThreaded(seedGenerator, seedSearcher, frameChecker,
                                      resultHandler, searchStatusHandler,
                                      startingSeeds);
    else
      searcher.SearchThreaded(seedGenerator, seedSearcher, frameChecker,
                              resultHandler, searchStatusHandler);
  }
}

}
