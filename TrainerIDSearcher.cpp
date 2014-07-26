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


#include "TrainerIDSearcher.h"
#include "SeedSearcher.h"

namespace pprng
{

namespace
{

struct FrameChecker
{
  FrameChecker(const TrainerIDSearcher::Criteria &criteria)
    : m_criteria(criteria)
  {}
  
  bool operator()(const Gen5TrainerIDFrame &frame) const
  {
    return (!m_criteria.hasTID || (frame.tid == m_criteria.tid)) &&
           (!m_criteria.hasSID || (frame.sid == m_criteria.sid)) &&
           (!m_criteria.hasShinyPID ||
            (PID(m_criteria.shinyPID).IsShiny(frame.tid, frame.sid)));
  }
  
  const TrainerIDSearcher::Criteria  &m_criteria;
};

struct FrameGeneratorFactory
{
  typedef Gen5TrainerIDFrameGenerator  FrameGenerator;
  
  FrameGeneratorFactory(bool hasSavedFile)
    : m_hasSavedFile(hasSavedFile)
  {}
  
  Gen5TrainerIDFrameGenerator operator()(const HashedSeed &seed) const
  {
    return Gen5TrainerIDFrameGenerator(seed, m_hasSavedFile);
  }
  
  const bool  m_hasSavedFile;
};

}

uint64_t TrainerIDSearcher::Criteria::ExpectedNumberOfResults() const
{
  uint64_t  numSeeds = seedParameters.NumberOfSeeds();
  if (numSeeds == 0)
    return 0;
  
  uint64_t  numFrames = frame.max - frame.min + 1;
  
  uint64_t  tidDivisor = hasTID ? 65536 : 1;
  uint64_t  sidDivisor = hasSID ? 65536 : 1;
  uint64_t  shinyDivisor = hasShinyPID ? 8192 : 1;
  
  return (numFrames * numSeeds / (tidDivisor * sidDivisor * shinyDivisor)) + 1;
}

void TrainerIDSearcher::Search
  (const Criteria &criteria, const ResultCallback &resultHandler,
   SearchRunner::StatusHandler &statusHandler,
   const std::vector<uint64_t> &startingSeeds)
{
  HashedSeedGenerator    seedGenerator(criteria.seedParameters);
  FrameGeneratorFactory  frameGeneratorFactory(criteria.hasSaveFile);
  
  SeedFrameSearcher<FrameGeneratorFactory>  seedSearcher(frameGeneratorFactory,
                                                         criteria.frame);
  
  FrameChecker  frameChecker(criteria);
  
  SearchRunner  searcher;
  
  if (startingSeeds.size() > 0)
    searcher.ContinueSearchThreaded(seedGenerator, seedSearcher, frameChecker,
                                    resultHandler, statusHandler,
                                    startingSeeds);
  else
    searcher.SearchThreaded(seedGenerator, seedSearcher, frameChecker,
                            resultHandler, statusHandler);
}

}
