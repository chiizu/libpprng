/*
  Copyright (C) 2014 chiizu
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

#ifndef HASHED_SEED_CALCULATOR
#define HASHED_SEED_CALCULATOR

#include "PPRNGTypes.h"
#include "HashedSeedMessage.h"

namespace pprng
{

struct HashedSeedCalculator
{
  static uint64_t CalculateRawSeed(const HashedSeedMessage &message);
  
#ifdef __ARM_NEON__
  struct MessageVariables
  {
    uint32_t  m5[4];
    uint32_t  m7[4];
    uint32_t  m8[4];
    uint32_t  m9[4];
    uint32_t  m12[4];
  };
  
  typedef uint64_t  Result[4];
  
  static void CalculateRawSeeds(const HashedSeedMessage &baseMessage,
                                const MessageVariables &variables,
                                Result &result);
#endif
};

}

#endif
