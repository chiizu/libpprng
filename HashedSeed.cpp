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


#include "HashedSeed.h"
#include "HashedSeedMessage.h"
#include "LinearCongruentialRNG.h"

namespace pprng
{

namespace
{

enum { NumTableRows = 5, NumTableColumns = 4 };

static uint32_t  PercentageTable[NumTableRows][NumTableColumns] =
{
  {  50,100,100,100 },
  {  50, 50,100,100 },
  {  30, 50,100,100 },
  {  25, 30, 50,100 },
  {  20, 25, 33, 50 }
};

static uint32_t ProbabilityTableLoop(LCRNG5 &rng)
{
  uint32_t  count = 0;
  
  for (uint32_t j = 0; j < NumTableRows; ++j)
  {
    for (uint32_t k = 0; k < NumTableColumns; ++k)
    {
      uint32_t  percent = PercentageTable[j][k];
      if (percent == 100)
        break;
      
      ++count;
      
      uint32_t d101 = ((rng.Next() >> 32) * 101) >> 32;
      if (d101 <= percent)
        break;
    }
  }
  
  return count;
}

static uint32_t SkipPIDRNGFrames(LCRNG5 &rng, Game::Color color,
                                 bool memoryLinkUsed)
{
  uint32_t  count = 0;
  
  if (Game::IsBlack2White2(color))
  {
    count += ProbabilityTableLoop(rng);
    
    rng.Next(); // 0
    rng.Next(); // 0xffffffff
    count += 2;
    
    // difference after memory link has been used
    if (!memoryLinkUsed)
    {
      rng.Next(); // 0xffffffff
      ++count;
    }
    
    for (uint32_t i = 0; i < 4; ++i)
      count += ProbabilityTableLoop(rng);
    
    bool      duplicatesFound = true;
    uint32_t  buffer[3];
    for (uint32_t limit = 0; duplicatesFound && (limit < 100); ++limit)
    {
      for (uint32_t i = 0; i < 3; ++i)
        buffer[i] = ((rng.Next() >> 32) * 15) >> 32;
      
      count += 3;
      duplicatesFound = false;
      
      for (uint32_t i = 0; i < 3; ++i)
      {
        for (uint32_t j = 0; j < 3; ++ j)
        {
          if (i != j)
          {
            if (buffer[i] == buffer[j])
              duplicatesFound = true;
          }
        }
      }
    }
  }
  else
  {
    for (uint32_t i = 0; i < 5; ++i)
      count += ProbabilityTableLoop(rng);
  }
  
  return count;
}


static uint32_t SkipTIDRNGFrames(LCRNG5 &rng, Game::Color color,
                                 bool hasSaveFile)
{
  uint32_t  count = 0;
  
  if (Game::IsBlack2White2(color))
  {
    count += ProbabilityTableLoop(rng);
    
    rng.Next(); // 0
    rng.Next(); // 0xffffffff
    count += 2;
    
    count += ProbabilityTableLoop(rng);
    
    rng.Next();
    rng.Next();
    rng.Next();
    rng.Next();
    count += 4;
    
    const uint32_t  rounds = hasSaveFile ? 2 : 1;
    for (uint32_t i = 0; i < rounds; ++i)
      count += ProbabilityTableLoop(rng);
    
    // three final calls
    rng.Next();
    rng.Next();
    rng.Next();
    count += 3;
  }
  else
  {
    const uint32_t  rounds = hasSaveFile ? 2 : 3;
    
    for (uint32_t i = 0; i < rounds; ++i)
      count += ProbabilityTableLoop(rng);
    
    // three final calls
    rng.Next();
    rng.Next();
    count += 2;
  }
  
  return count;
}

}

bool HashedSeed::Parameters::NextDate(uint32_t excludedSeasonMask)
{
  date = date + boost::gregorian::days(1);
  
  if ((excludedSeasonMask != 0) &&
      (date.day() == 1) &&
      (Season::MaskForMonth(date.month()) & excludedSeasonMask) != 0)
  {
    do
    {
      date = date + boost::gregorian::months(1);
    }
    while ((Season::MaskForMonth(date.month()) & excludedSeasonMask) != 0);
    
    return true;
  }
  
  return false;
}

HashedSeed::HashedSeed(const HashedSeed::Parameters &p)
  : parameters(p),
    rawSeed(HashedSeedMessage(parameters, 0).AsHashedSeed().rawSeed),
    m_skippedPIDFramesCalculated(false),
    m_skippedPIDFramesMemoryLinkUsed(false),
    m_skippedPIDFrames(0), m_skippedPIDFramesSeed(0)
{}

uint32_t HashedSeed::SeedAndSkipPIDFrames(LCRNG5 &rng, bool memLinkUsed) const
{
  if (!m_skippedPIDFramesCalculated ||
      (memLinkUsed != m_skippedPIDFramesMemoryLinkUsed))
  {
    rng.Seed(rawSeed);
    
    m_skippedPIDFrames = SkipPIDRNGFrames(rng, parameters.gameColor,
                                          memLinkUsed);
    m_skippedPIDFramesSeed = rng.Seed();
    
    m_skippedPIDFramesCalculated = true;
    m_skippedPIDFramesMemoryLinkUsed = memLinkUsed;
  }
  else
  {
    rng.Seed(m_skippedPIDFramesSeed);
  }
  
  return m_skippedPIDFrames;
}

uint32_t HashedSeed::GetSkippedPIDFrames(bool memoryLinkUsed) const
{
  if (!m_skippedPIDFramesCalculated ||
      (memoryLinkUsed != m_skippedPIDFramesMemoryLinkUsed))
  {
    LCRNG5  rng(0);
    
    return SeedAndSkipPIDFrames(rng, memoryLinkUsed);
  }
  else
  {
    return m_skippedPIDFrames;
  }
}

uint32_t HashedSeed::SeedAndSkipTIDFrames(LCRNG5 &rng, bool hasSaveFile) const
{
  rng.Seed(rawSeed);
  
  return SkipTIDRNGFrames(rng, parameters.gameColor, hasSaveFile);
}

uint32_t HashedSeed::GetSkippedTIDFrames(bool hasSaveFile) const
{
  LCRNG5  rng(0);
  
  return SeedAndSkipTIDFrames(rng, hasSaveFile);
}

}
