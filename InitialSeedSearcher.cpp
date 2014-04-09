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


#include "InitialSeedSearcher.h"
#include "LinearCongruentialRNG.h"
#include "FrameGenerator.h"
#include <iostream>
#include <iomanip>
#include <set>

namespace pprng
{

uint64_t InitialIVSeedSearcher::Criteria::ExpectedNumberOfResults() const
{
  uint64_t  numSeeds = seedParameters.NumberOfSeeds();
  if (numSeeds == 0)
    return 0;
  
  uint64_t  numIVs = IVs::CalculateNumberOfCombinations(minIVs, maxIVs);
  
  return (numSeeds * numIVs / (32 * 32 * 32 * 32 * 32 * 32)) + 1;
}

void InitialIVSeedSearcher::Search
  (const Criteria &criteria, const ResultCallback &resultHandler,
   SearchRunner::StatusHandler &statusHandler,
   const std::vector<uint64_t> &startingSeeds)
{
  HashedSeedSearcher::Criteria  c;
  
  c.seedParameters = criteria.seedParameters;
  c.ivFrame.min = 1;
  c.ivFrame.max = 1;
  c.ivs.min = criteria.minIVs;
  c.ivs.max = criteria.maxIVs;
  c.ivs.hiddenTypeMask = 0;
  c.ivs.minHiddenPower = 30;
  c.ivs.isRoamer = criteria.isRoamer;
  
  HashedSeedSearcher  searcher;
  
  searcher.Search(c, resultHandler, statusHandler, startingSeeds);
}

uint64_t SpinnerInitialSeedSearcher::Criteria::ExpectedNumberOfResults() const
{
  uint64_t  numSeeds = seedParameters.NumberOfSeeds();
  if (numSeeds == 0)
    return 0;
  
  uint64_t  spinsDenominator = (0x1ULL << (3 * spins.NumSpins()));
  
  return (numSeeds / spinsDenominator) + 1;
}

namespace
{

struct SpinnerInitialSeedChecker
{
  typedef HashedSeed  ResultType;
  
  SpinnerInitialSeedChecker(const SpinnerPositions &spins, bool memoryLinkUsed,
                            SpinnerPositions::Mode mode)
    : m_spins(spins), m_memoryLinkUsed(memoryLinkUsed), m_mode(mode)
  {}
  
  bool operator()(const HashedSeed &seed) const
  {
    SpinnerPositions  seedSpins(seed, m_memoryLinkUsed, m_mode,
                                m_spins.NumSpins());
    
    return seedSpins.word == m_spins.word;
  }
  
  void Search(const HashedSeed &seed, const SpinnerInitialSeedChecker &checker,
              const SpinnerInitialSeedSearcher::ResultCallback &resultHandler)
  {
    if (checker(seed))
      resultHandler(seed);
  }
  
  const SpinnerPositions        m_spins;
  const bool                    m_memoryLinkUsed;
  const SpinnerPositions::Mode  m_mode;
};

}

void SpinnerInitialSeedSearcher::Search
  (const Criteria &criteria, const ResultCallback &resultHandler,
   SearchRunner::StatusHandler &statusHandler,
   const std::vector<uint64_t> &startingSeeds)
{
  HashedSeedGenerator        seedGenerator(criteria.seedParameters);
  SpinnerInitialSeedChecker  seedChecker(criteria.spins,
                                         criteria.memoryLinkUsed,
                                         criteria.mode);
  SearchRunner               searcher;
  
  if (startingSeeds.size() > 0)
    searcher.ContinueSearchThreaded(seedGenerator, seedChecker, seedChecker,
                                    resultHandler, statusHandler,
                                    startingSeeds);
  else
    searcher.SearchThreaded(seedGenerator, seedChecker, seedChecker,
                            resultHandler, statusHandler);
}

}
