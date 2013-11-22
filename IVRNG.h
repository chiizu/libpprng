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

#ifndef IV_RNG_H
#define IV_RNG_H

#include "PPRNGTypes.h"
#include "RNGWrappers.h"

namespace pprng
{

template <int Method, class RNG>
class Gen34IVRNG
{
public:
  Gen34IVRNG(RNG &rng)
    : m_RNG(rng)
  {}
  
  uint32_t NextIVWord()
  {
    IVs  result;
    result.SetFromGameDataWord((m_RNG.Next() >> 16) |
                               (m_RNG.Next() & 0xffff0000));
    return result.ivWord;
  }
  
private:
  RNG  &m_RNG;
};

// specialize for method 4 due to extra RNG call between IV calls
template <class RNG>
class Gen34IVRNG<4, RNG>
{
public:
  Gen34IVRNG(RNG &rng)
    : m_RNG(rng)
  {}
  
  uint32_t NextIVWord()
  {
    uint32_t  temp = m_RNG.Next() >> 16;
    
    // skip one call
    m_RNG.Next();
    
    IVs  result;
    result.SetFromGameDataWord(temp | (m_RNG.Next() & 0xffff0000));
    
    return result.ivWord;
  }
  
private:
  RNG  &m_RNG;
};


// standard IVRNG for 5th gen
template <class RNG>
class Gen5BufferingIVRNG
{
public:
  enum { LowBitOffset = (sizeof(typename RNG::ReturnType) * 8) - 5 };
  
  enum FrameType
  {
    Normal = 0,
    Roamer
  };
  
  // will have undefined behavior if all IVs are set
  Gen5BufferingIVRNG(RNG &rng, FrameType frameType,
                     const OptionalIVs &setIVs = OptionalIVs())
    : m_RNG(rng), m_setIVs(setIVs), m_word(0),
      m_IVWordGenerator((frameType == Normal) ?
                        (setIVs.anySet() ?
                         &Gen5BufferingIVRNG::NextSetIVsNormalIVWord :
                         &Gen5BufferingIVRNG::NextNormalIVWord) :
                        &Gen5BufferingIVRNG::NextRoamerIVWord)
  {
    if (frameType == Roamer)
      m_RNG.Next();  // unknown call
    
    const uint32_t  numSetIVs = setIVs.numSet();
    
    uint32_t  word = 0;
    for (uint32_t i = 0; i < (5 - numSetIVs); ++i)
      word = NextRawWord(word);
    
    m_word = word;
  }
  
  uint32_t NextIVWord()
  {
    uint32_t  word = NextRawWord(m_word);
    m_word = word;
    
    return (this->*m_IVWordGenerator)(word);
  }
  
//private:
  uint32_t NextRawWord(uint32_t currentWord)
  {
    return (currentWord >> 5) | (uint32_t(m_RNG.Next() >> LowBitOffset) << 25);
  }
  
  typedef uint32_t (Gen5BufferingIVRNG::*IVWordGenerator)(uint32_t buffer);
  
  const IVWordGenerator  m_IVWordGenerator;
  
  uint32_t NextNormalIVWord(uint32_t buffer)
  {
    return buffer;
  }
  
  uint32_t NextRoamerIVWord(uint32_t buffer)
  {
    uint32_t  result = (buffer & 0x7fff) |
                       ((buffer & 0x000f8000) << 10) |
                       ((buffer & 0x3ff00000) >> 5);
    return result;
  }
  
  uint32_t NextSetIVsNormalIVWord(uint32_t buffer)
  {
    IVs  current;
    current.ivWord = buffer;
    
    IVs  result;
    int  ivIdx = IVs::SP;
    
    result.sp(m_setIVs.isSet(IVs::SP) ? m_setIVs.sp() : current.iv(ivIdx--));
    result.sd(m_setIVs.isSet(IVs::SD) ? m_setIVs.sd() : current.iv(ivIdx--));
    result.sa(m_setIVs.isSet(IVs::SA) ? m_setIVs.sa() : current.iv(ivIdx--));
    result.df(m_setIVs.isSet(IVs::DF) ? m_setIVs.df() : current.iv(ivIdx--));
    result.at(m_setIVs.isSet(IVs::AT) ? m_setIVs.at() : current.iv(ivIdx--));
    result.hp(m_setIVs.isSet(IVs::HP) ? m_setIVs.hp() : current.iv(ivIdx--));
    
    return result.ivWord;
  }
  
  RNG          &m_RNG;
  OptionalIVs  m_setIVs;
  uint32_t     m_word;
};

}

#endif
