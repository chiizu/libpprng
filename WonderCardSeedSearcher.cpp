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


#include "WonderCardSeedSearcher.h"
#include "SeedSearcher.h"

namespace pprng
{

namespace
{

struct FrameChecker
{
  FrameChecker(const WonderCardSeedSearcher::Criteria &criteria)
    : m_criteria(criteria)
  {}
  
  bool operator()(const WonderCardFrame &frame) const
  {
    return m_criteria.ivs.CheckIVs(frame.ivs) &&
           m_criteria.ivs.CheckHiddenPower(frame.ivs) &&
           CheckShiny(frame.pid) &&
           CheckNature(frame.nature) &&
           CheckAbility(frame.ability) &&
           CheckGender(frame.pid);
  }
  
  bool CheckShiny(PID pid) const
  {
    return (m_criteria.frameParameters.cardShininess !=
            Shininess::MAY_BE_SHINY) ||
           !m_criteria.shinyOnly ||
           pid.IsShiny(m_criteria.frameParameters.cardTID,
                       m_criteria.frameParameters.cardSID);
  }
  
  bool CheckNature(Nature::Type nature) const
  {
    return (m_criteria.frameParameters.cardNature != Nature::ANY) ||
           m_criteria.pid.CheckNature(nature);
  }
  
  bool CheckAbility(Ability::Type ability) const
  {
    return (m_criteria.frameParameters.cardAbility != Ability::ANY) ||
           m_criteria.pid.CheckAbility(ability);
  }
  
  bool CheckGender(const PID &pid) const
  {
    return (m_criteria.frameParameters.cardGender != Gender::ANY) ||
           m_criteria.pid.CheckGender(pid);
  }
  
  const WonderCardSeedSearcher::Criteria  &m_criteria;
};

struct FrameGeneratorFactory
{
  typedef WonderCardFrameGenerator  FrameGenerator;
  
  FrameGeneratorFactory(const WonderCardSeedSearcher::Criteria &criteria)
    : m_criteria(criteria)
  {}
  
  WonderCardFrameGenerator operator()(const HashedSeed &seed) const
  {
    return WonderCardFrameGenerator(seed, m_criteria.frameParameters);
  }
  
  const WonderCardSeedSearcher::Criteria  &m_criteria;
};

struct AggressiveSeedSearcher
{
  typedef WonderCardFrame  ResultType;
  
  AggressiveSeedSearcher(const WonderCardFrameGenerator::Parameters &p,
                         const SearchCriteria::FrameRange &frameRange,
                         const SearchCriteria::IVCriteria &ivCriteria,
                         const SearchCriteria::PIDCriteria &pidCriteria)
    : m_frameParameters(p), m_frameRange(frameRange), m_ivCriteria(ivCriteria),
      m_pidCriteria(pidCriteria)
  {}
  
  void Search(const HashedSeed &seed, const FrameChecker &frameChecker,
              const WonderCardSeedSearcher::ResultCallback &resultHandler)
  {
    WonderCardFrameGenerator  generator(seed, m_frameParameters);
    
    generator.SkipFrames(m_frameRange.min - 1);
    
    while (generator.CurrentFrame().number < m_frameRange.max)
    {
      if (generator.AdvanceFrameWithCriteria(m_ivCriteria, m_pidCriteria) &&
          frameChecker(generator.CurrentFrame()))
        resultHandler(generator.CurrentFrame());
    }
  }
  
  const WonderCardFrameGenerator::Parameters  &m_frameParameters;
  const SearchCriteria::FrameRange            &m_frameRange;
  const SearchCriteria::IVCriteria            &m_ivCriteria;
  const SearchCriteria::PIDCriteria           &m_pidCriteria;
};

}

uint64_t WonderCardSeedSearcher::Criteria::ExpectedNumberOfResults() const
{
  uint64_t  numSeeds = seedParameters.NumberOfSeeds();
  if (numSeeds == 0)
    return numSeeds;
  
  uint64_t  numFrames = frameRange.max - frameRange.min + 1;
  
  uint64_t  numIVs = IVs::CalculateNumberOfCombinations(ivs.min, ivs.max);
  uint64_t  ivDivisor = 1;
  
  for (uint32_t i = (6 - frameParameters.cardIVs.numSet()); i > 0; --i)
    ivDivisor *= 32;
  
  uint64_t  natureMultiplier = pid.NumNatures(), natureDivisor = 25;
  if (frameParameters.cardNature != Nature::ANY)
  {
    natureMultiplier = 1;
    natureDivisor = 1;
  }
  
  uint64_t  abilityDivisor = (pid.ability > 1) ? 1 : 2;
  if (frameParameters.cardAbility != Ability::ANY)
    abilityDivisor = 1;
  
  uint64_t  shinyDivisor = (shinyOnly &&
                            (frameParameters.cardShininess ==
                             Shininess::MAY_BE_SHINY)) ? 8192 : 1;
  
  uint64_t  numResults = numSeeds * numFrames * numIVs * natureMultiplier /
                         (ivDivisor *
                          natureDivisor * abilityDivisor * shinyDivisor);
  
  numResults = IVs::AdjustExpectedResultsForHiddenPower
    (numResults, ivs.min, ivs.max, ivs.hiddenTypeMask, ivs.minHiddenPower);
  
  return numResults + 1;
}

void WonderCardSeedSearcher::Search
  (const Criteria &criteria, const ResultCallback &resultHandler,
   SearchRunner::StatusHandler &statusHandler,
   const std::vector<uint64_t> &startingSeeds)
{
  HashedSeedGenerator         seedGenerator(criteria.seedParameters);
  
#if 0
  FrameGeneratorFactory       frameGeneratorFactory(criteria);
  
  SeedFrameSearcher<FrameGeneratorFactory>  seedSearcher(frameGeneratorFactory,
                                                         criteria.frameRange);
#else
  AggressiveSeedSearcher  seedSearcher(criteria.frameParameters,
                                       criteria.frameRange, criteria.ivs,
                                       criteria.pid);
#endif
  FrameChecker                frameChecker(criteria);
  
  SearchRunner                searcher;
  
  if (startingSeeds.size() > 0)
    searcher.ContinueSearchThreaded(seedGenerator, seedSearcher, frameChecker,
                                    resultHandler, statusHandler,
                                    startingSeeds);
  else
    searcher.SearchThreaded(seedGenerator, seedSearcher, frameChecker,
                            resultHandler, statusHandler);
}

}
