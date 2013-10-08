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

#ifndef ENCOUNTER_SEED_SEARCHER_H
#define ENCOUNTER_SEED_SEARCHER_H


#include "HashedSeedSearcher.h"

namespace pprng
{

class EncounterSeedSearcher
{
public:
  struct Criteria : public HashedSeedSearcher::Criteria
  {
    Gen5PIDFrameGenerator::Parameters  frameParameters;
    
    SearchCriteria::PIDCriteria  pid;
    SearchCriteria::FrameRange   pidFrame;
    bool                         shinyOnly;
    
    uint32_t  leadAbilityMask;
    uint32_t  esvMask[4];
    
    Criteria()
      : HashedSeedSearcher::Criteria(),
        frameParameters(), pid(), pidFrame(),
        shinyOnly(false), leadAbilityMask(0)
    {
      esvMask[0] = esvMask[1] = esvMask[2] = esvMask[3] = 0;
    }
    
    uint64_t ExpectedNumberOfResults() const;
  };
  
  typedef Gen5EncounterFrame                         ResultType;
  typedef boost::function<void (const ResultType&)>  ResultCallback;
  
  EncounterSeedSearcher() {}
  
  void Search(const Criteria &criteria, const ResultCallback &resultHandler,
              SearchRunner::StatusHandler &statusHandler,
              const std::vector<uint64_t> &startingSeeds =
                std::vector<uint64_t>());
};

}

#endif
