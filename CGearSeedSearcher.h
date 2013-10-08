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

#ifndef C_GEAR_SEED_SEARCHER_H
#define C_GEAR_SEED_SEARCHER_H

#include "PPRNGTypes.h"
#include "SearchCriteria.h"
#include "SearchRunner.h"
#include "SeedGenerator.h"
#include "FrameGenerator.h"

#include <boost/function.hpp>

namespace pprng
{

class CGearSeedSearcher
{
public:
  struct Criteria : public SearchCriteria
  {
    uint32_t                    macAddressLow;
    uint32_t                    minDelay, maxDelay;
    SearchCriteria::FrameRange  frameRange;
    SearchCriteria::IVCriteria  ivs;
    
    uint64_t ExpectedNumberOfResults() const;
  };
  
  typedef CGearIVFrameGenerator::Frame               ResultType;
  typedef boost::function<void (const ResultType&)>  ResultCallback;
  
  CGearSeedSearcher() {}
  
  void Search(const Criteria &criteria, const ResultCallback &resultHandler,
              SearchRunner::StatusHandler &statusHandler);
};


}

#endif
