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

#ifndef SEARCH_CRITERIA_H
#define SEARCH_CRITERIA_H

#include "PPRNGTypes.h"
#include <boost/lexical_cast.hpp>
#include <sstream>

namespace pprng
{

struct SearchCriteria
{
  struct PIDCriteria
  {
    uint32_t       natureMask;
    Ability::Type  ability;
    Gender::Type   gender;
    Gender::Ratio  genderRatio;
    
    PIDCriteria()
      : natureMask(0), ability(Ability::ANY),
        gender(Gender::ANY), genderRatio(Gender::ANY_RATIO)
    {}
    
    bool CheckNature(Nature::Type nature) const
    {
      return (natureMask == 0) || (((0x1 << nature) & natureMask) != 0);
    }
    
    uint32_t NumNatures() const
    {
      uint32_t  c = 0, n = natureMask;
      while (n != 0)
      {
        n &= n - 1;
        ++c;
      }
      
      return (c == 0) ? 25 : c;
    }
    
    bool CheckAbility(Ability::Type a) const
    {
      return (ability == Ability::ANY) || (a == ability);
    }
    
    bool CheckGender(const PID &pid) const
    {
      return Gender::GenderValueMatches(pid.GenderValue(), gender, genderRatio);
    }
  };
  
  struct IVCriteria
  {
    IVPattern::Type  pattern;
    
    IVs       min, max;
    uint32_t  hiddenTypeMask;
    uint32_t  minHiddenPower;
    bool      isRoamer;
    
    IVCriteria()
      : pattern(IVPattern::CUSTOM), min(), max(),
        hiddenTypeMask(0), minHiddenPower(30), isRoamer(false)
    {}
    
    void DeterminePattern()
    {
      pattern = IVPattern::Get(min, max, (hiddenTypeMask != 0), minHiddenPower);
      
      switch (pattern)
      {
      case IVPattern::CUSTOM: default:
        break;
        
      case IVPattern::HEX_FLAWLESS:
        mask = IVs::Perfect.word; test = IVs::Perfect.word;
        break;
        
      case IVPattern::PHYSICAL_FLAWLESS:
        mask = IVs::PhysPerfect.word; test = IVs::PhysPerfect.word;
        break;
        
      case IVPattern::SPECIAL_FLAWLESS:
        mask = IVs::SpecPerfect.word; test = IVs::SpecPerfect.word;
        break;
        
      case IVPattern::SPECIAL_HIDDEN_POWER_FLAWLESS:
        mask = IVs::HpPerfectLow.word; test = IVs::HpPerfectLow.word;
        break;
        
      case IVPattern::HEX_FLAWLESS_TRICK:
        mask = IVs::Perfect.word; test = IVs::PerfectTrick.word;
        break;
        
      case IVPattern::PHYSICAL_FLAWLESS_TRICK:
        mask = IVs::PhysPerfect.word; test = IVs::PhysPerfectTrick.word;
        break;
        
      case IVPattern::SPECIAL_FLAWLESS_TRICK:
        mask = IVs::SpecPerfect.word; test = IVs::SpecPerfectTrick.word;
        break;
        
      case IVPattern::SPECIAL_HIDDEN_POWER_FLAWLESS_TRICK:
        mask = IVs::HpPerfectLow.word; test = IVs::HpPerfectTrickLow.word;
        break;
      }
    }
    
    bool CheckPattern(const IVs &ivs) const
    {
      return (ivs.word & mask) == test;
    }
    
    bool CheckIVs(const IVs &ivs) const
    {
      return ((pattern == IVPattern::CUSTOM) ||
              CheckPattern(ivs)) &&
             ivs.betterThanOrEqual(min) &&
             (max.isMax() || ivs.worseThanOrEqual(max));
    }
    
    bool CheckHiddenPower(const IVs &ivs) const
    {
      return (hiddenTypeMask == 0) ||
             ((((0x1 << (ivs.HiddenType() - 1)) & hiddenTypeMask) != 0) &&
              (ivs.HiddenPower() >= minHiddenPower));
    }
    
    uint32_t NumHiddenPowers() const
    {
      uint32_t  c = 0, n = hiddenTypeMask;
      while (n != 0)
      {
        n &= n - 1;
        ++c;
      }
      
      return (c == 0) ? 16 : c;
    }
    
  private:
    uint32_t  mask, test;
  };
  
  struct NumberRange
  {
    NumberRange() : min(0), max(0) {}
    NumberRange(uint32_t mi, uint32_t ma) : min(mi), max(ma) {}
    
    uint32_t  min;
    uint32_t  max;
  };
  
  typedef NumberRange  FrameRange;
  typedef NumberRange  DelayRange;
  
  class ImpossibleMinMaxFrameRangeException : public Exception
  {
  public:
    ImpossibleMinMaxFrameRangeException
      (uint32_t minFrame, uint32_t maxFrame, const std::string &frameType)
      : Exception
        ("Minimum " + frameType + " frame " +
         boost::lexical_cast<std::string>(minFrame) +
         " is not less than or equal to maximum " + frameType + " frame " +
         boost::lexical_cast<std::string>(maxFrame))
    {}
  };
  
  virtual uint64_t ExpectedNumberOfResults() const = 0;
};

}

#endif
