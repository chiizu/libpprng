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

#include "HashedSeedCalculator.h"

#include "LinearCongruentialRNG.h"

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

namespace pprng
{

namespace
{

static uint32_t SwapEndianess(uint32_t value)
{
  value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
  return (value << 16) | (value >> 16);
}

}


// SHA1 constants
#define K0  0x5A827999
#define K1  0x6ED9EBA1
#define K2  0x8F1BBCDC
#define K3  0xCA62C1D6

#define H0  0x67452301
#define H1  0xEFCDAB89
#define H2  0x98BADCFE
#define H3  0x10325476
#define H4  0xC3D2E1F0


#define CalcW(I) \
  temp = w[(I - 3) & 0xf] ^ w[(I - 8) & 0xf] ^ w[(I - 14) & 0xf] ^ w[(I - 16) & 0xf]; \
  w[I & 0xf] = temp = (temp << 1) | (temp >> 31)


#define Section1Calc() \
  temp = ((a << 5) | (a >> 27)) + ((b & c) | (~b & d)) + e + K0 + temp

#define Section2Calc() \
  temp = ((a << 5) | (a >> 27)) + (b ^ c ^ d) + e + K1 + temp

#define Section3Calc() \
  temp = ((a << 5) | (a >> 27)) + ((b & c) | ((b | c) & d)) + e + K2 + temp

#define Section4Calc() \
  temp = ((a << 5) | (a >> 27)) + (b ^ c ^ d) + e + K3 + temp

#define UpdateVars() \
  e = d; \
  d = c; \
  c = (b << 30) | (b >> 2); \
  b = a; \
  a = temp


uint64_t HashedSeedCalculator::CalculateRawSeed(const HashedSeedMessage &m)
{
  const uint32_t  *message = m.GetMessageData();
  
  uint32_t  w[16], temp;
  
  uint32_t  a = H0;
  uint32_t  b = H1;
  uint32_t  c = H2;
  uint32_t  d = H3;
  uint32_t  e = H4;
  
  // Section 1: 0-19
  w[0] = temp = message[0]; Section1Calc(); UpdateVars();
  w[1] = temp = message[1]; Section1Calc(); UpdateVars();
  w[2] = temp = message[2]; Section1Calc(); UpdateVars();
  w[3] = temp = message[3]; Section1Calc(); UpdateVars();
  w[4] = temp = message[4]; Section1Calc(); UpdateVars();
  w[5] = temp = message[5]; Section1Calc(); UpdateVars();
  w[6] = temp = message[6]; Section1Calc(); UpdateVars();
  w[7] = temp = message[7]; Section1Calc(); UpdateVars();
  w[8] = temp = message[8]; Section1Calc(); UpdateVars();
  w[9] = temp = message[9]; Section1Calc(); UpdateVars();
  w[10] = temp = message[10]; Section1Calc(); UpdateVars();
  w[11] = temp = message[11]; Section1Calc(); UpdateVars();
  w[12] = temp = message[12]; Section1Calc(); UpdateVars();
  w[13] = temp = message[13]; Section1Calc(); UpdateVars();
  w[14] = temp = message[14]; Section1Calc(); UpdateVars();
  w[15] = temp = message[15]; Section1Calc(); UpdateVars();
  
  CalcW(16); Section1Calc(); UpdateVars();
  CalcW(17); Section1Calc(); UpdateVars();
  CalcW(18); Section1Calc(); UpdateVars();
  CalcW(19); Section1Calc(); UpdateVars();
  
  // Section 2: 20 - 39
  CalcW(20); Section2Calc(); UpdateVars();
  CalcW(21); Section2Calc(); UpdateVars();
  CalcW(22); Section2Calc(); UpdateVars();
  CalcW(23); Section2Calc(); UpdateVars();
  CalcW(24); Section2Calc(); UpdateVars();
  CalcW(25); Section2Calc(); UpdateVars();
  CalcW(26); Section2Calc(); UpdateVars();
  CalcW(27); Section2Calc(); UpdateVars();
  CalcW(28); Section2Calc(); UpdateVars();
  CalcW(29); Section2Calc(); UpdateVars();
  CalcW(30); Section2Calc(); UpdateVars();
  CalcW(31); Section2Calc(); UpdateVars();
  CalcW(32); Section2Calc(); UpdateVars();
  CalcW(33); Section2Calc(); UpdateVars();
  CalcW(34); Section2Calc(); UpdateVars();
  CalcW(35); Section2Calc(); UpdateVars();
  CalcW(36); Section2Calc(); UpdateVars();
  CalcW(37); Section2Calc(); UpdateVars();
  CalcW(38); Section2Calc(); UpdateVars();
  CalcW(39); Section2Calc(); UpdateVars();
  
  // Section 3: 40 - 59
  CalcW(40); Section3Calc(); UpdateVars();
  CalcW(41); Section3Calc(); UpdateVars();
  CalcW(42); Section3Calc(); UpdateVars();
  CalcW(43); Section3Calc(); UpdateVars();
  CalcW(44); Section3Calc(); UpdateVars();
  CalcW(45); Section3Calc(); UpdateVars();
  CalcW(46); Section3Calc(); UpdateVars();
  CalcW(47); Section3Calc(); UpdateVars();
  CalcW(48); Section3Calc(); UpdateVars();
  CalcW(49); Section3Calc(); UpdateVars();
  CalcW(50); Section3Calc(); UpdateVars();
  CalcW(51); Section3Calc(); UpdateVars();
  CalcW(52); Section3Calc(); UpdateVars();
  CalcW(53); Section3Calc(); UpdateVars();
  CalcW(54); Section3Calc(); UpdateVars();
  CalcW(55); Section3Calc(); UpdateVars();
  CalcW(56); Section3Calc(); UpdateVars();
  CalcW(57); Section3Calc(); UpdateVars();
  CalcW(58); Section3Calc(); UpdateVars();
  CalcW(59); Section3Calc(); UpdateVars();
  
  // Section 3: 60 - 79
  CalcW(60); Section4Calc(); UpdateVars();
  CalcW(61); Section4Calc(); UpdateVars();
  CalcW(62); Section4Calc(); UpdateVars();
  CalcW(63); Section4Calc(); UpdateVars();
  CalcW(64); Section4Calc(); UpdateVars();
  CalcW(65); Section4Calc(); UpdateVars();
  CalcW(66); Section4Calc(); UpdateVars();
  CalcW(67); Section4Calc(); UpdateVars();
  CalcW(68); Section4Calc(); UpdateVars();
  CalcW(69); Section4Calc(); UpdateVars();
  CalcW(70); Section4Calc(); UpdateVars();
  CalcW(71); Section4Calc(); UpdateVars();
  CalcW(72); Section4Calc(); UpdateVars();
  CalcW(73); Section4Calc(); UpdateVars();
  CalcW(74); Section4Calc(); UpdateVars();
  CalcW(75); Section4Calc(); UpdateVars();
  CalcW(76); Section4Calc(); UpdateVars();
  CalcW(77); Section4Calc(); UpdateVars();
  CalcW(78); Section4Calc(); UpdateVars();
  CalcW(79); Section4Calc(); //UpdateVars();  don't need full update
  
  // If doing a true SHA1
  // Digest  d;
  
  //d.h[0] = H0 + a;
  //d.h[1] = H1 + b;
  //d.h[2] = H2 + c;
  //d.h[3] = H3 + d;
  //d.h[4] = H4 + e;
  
  // interesting part from UpdateVars
  b = a;
  a = temp;
  
  // build pre-seed
  uint64_t  preSeed = SwapEndianess(H1 + b);
  preSeed = (preSeed << 32) | SwapEndianess(H0 + a);
  
  // advance once and return
  return LCRNG5::NextForSeed(preSeed);
}



#ifdef __ARM_NEON__

// temp = w[(I - 3) & 0xf] ^ w[(I - 8) & 0xf] ^ w[(I - 14) & 0xf] ^
//        w[(I - 16) & 0xf];
// w[I & 0xf] = temp = (temp << 1) | (temp >> 31)
#define V_CalcW(I) \
  t0 = veorq_u32(w[(I - 3) & 0xf], w[(I - 8) & 0xf]); \
  t0 = veorq_u32(t0, w[(I - 14) & 0xf]); \
  t0 = veorq_u32(t0, w[(I - 16) & 0xf]); \
  t1 = vshlq_n_u32(t0, 1); \
  t0 = vshrq_n_u32(t0, 31); \
  t0 = vorrq_u32(t0, t1); \
  w[I & 0xf] = temp = t0

// ((a << 5) | (a >> 27)) + V + e + K + temp
// t2 is V
#define V_CalcBase(K) \
  t0 = vshlq_n_u32(a, 5); \
  t1 = vshrq_n_u32(a, 27); \
  t0 = vorrq_u32(t0, t1); \
  t0 = vaddq_u32(t2, t0); \
  t0 = vaddq_u32(e, t0); \
  t0 = vaddq_u32(K, t0); \
  temp = vaddq_u32(temp, t0);

// temp = ((a << 5) | (a >> 27)) + ((b & c) | (~b & d)) + e + K0 + temp
#define V_Section1Calc() \
  t0 = vandq_u32(b, c); \
  t1 = vbicq_u32(d, b); \
  t2 = vorrq_u32(t0, t1); \
  V_CalcBase(vK0)

// temp = ((a << 5) | (a >> 27)) + (b ^ c ^ d) + e + K1 + temp
#define V_Section2Calc() \
  t0 = veorq_u32(b, c); \
  t2 = veorq_u32(t0, d); \
  V_CalcBase(vK1)

// temp = ((a << 5) | (a >> 27)) + ((b & c) | ((b | c) & d)) + e + K2 + temp
#define V_Section3Calc() \
  t0 = vorrq_u32(b, c); \
  t0 = vandq_u32(t0, d); \
  t1 = vandq_u32(b, c); \
  t2 = vorrq_u32(t0, t1); \
  V_CalcBase(vK2)

// temp = ((a << 5) | (a >> 27)) + (b ^ c ^ d) + e + K3 + temp
#define V_Section4Calc() \
  t0 = veorq_u32(b, c); \
  t2 = veorq_u32(t0, d); \
  V_CalcBase(vK3)

#define V_UpdateVars() \
  e = d; \
  d = c; \
  t0 = vshlq_n_u32(b, 30); \
  t1 = vshrq_n_u32(b, 2); \
  c = vorrq_u32(t0, t1); \
  b = a; \
  a = temp


void HashedSeedCalculator::CalculateRawSeeds
  (const HashedSeedMessage &baseMessage,
   const MessageVariables &variables,
   Result &result)
{
  const uint32_t  *baseData = baseMessage.GetMessageData();
  
  uint32x4_t  w[16], temp, t0, t1, t2;
  
  uint32x4_t  a = vdupq_n_u32(H0);
  uint32x4_t  b = vdupq_n_u32(H1);
  uint32x4_t  c = vdupq_n_u32(H2);
  uint32x4_t  d = vdupq_n_u32(H3);
  uint32x4_t  e = vdupq_n_u32(H4);
  
  // Section 1: 0-19
  {
    const uint32x4_t  vK0 = vdupq_n_u32(K0);
    
    w[0] = temp = vdupq_n_u32(baseData[0]); V_Section1Calc(); V_UpdateVars();
    w[1] = temp = vdupq_n_u32(baseData[1]); V_Section1Calc(); V_UpdateVars();
    w[2] = temp = vdupq_n_u32(baseData[2]); V_Section1Calc(); V_UpdateVars();
    w[3] = temp = vdupq_n_u32(baseData[3]); V_Section1Calc(); V_UpdateVars();
    w[4] = temp = vdupq_n_u32(baseData[4]); V_Section1Calc(); V_UpdateVars();
    
    w[5] = temp = vld1q_u32(variables.m5); V_Section1Calc(); V_UpdateVars();
    
    w[6] = temp = vdupq_n_u32(baseData[6]); V_Section1Calc(); V_UpdateVars();
    
    w[7] = temp = vld1q_u32(variables.m7); V_Section1Calc(); V_UpdateVars();
    w[8] = temp = vld1q_u32(variables.m8); V_Section1Calc(); V_UpdateVars();
    w[9] = temp = vld1q_u32(variables.m9); V_Section1Calc(); V_UpdateVars();
    
    w[10] = temp = vdupq_n_u32(baseData[10]); V_Section1Calc(); V_UpdateVars();
    w[11] = temp = vdupq_n_u32(baseData[11]); V_Section1Calc(); V_UpdateVars();
    
    w[12] = temp = vld1q_u32(variables.m12); V_Section1Calc(); V_UpdateVars();
    
    w[13] = temp = vdupq_n_u32(baseData[13]); V_Section1Calc(); V_UpdateVars();
    w[14] = temp = vdupq_n_u32(baseData[14]); V_Section1Calc(); V_UpdateVars();
    w[15] = temp = vdupq_n_u32(baseData[15]); V_Section1Calc(); V_UpdateVars();
    
    V_CalcW(16); V_Section1Calc(); V_UpdateVars();
    V_CalcW(17); V_Section1Calc(); V_UpdateVars();
    V_CalcW(18); V_Section1Calc(); V_UpdateVars();
    V_CalcW(19); V_Section1Calc(); V_UpdateVars();
  }
  
  // Section 2: 20 - 39
  {
    const uint32x4_t  vK1 = vdupq_n_u32(K1);
    
    V_CalcW(20); V_Section2Calc(); V_UpdateVars();
    V_CalcW(21); V_Section2Calc(); V_UpdateVars();
    V_CalcW(22); V_Section2Calc(); V_UpdateVars();
    V_CalcW(23); V_Section2Calc(); V_UpdateVars();
    V_CalcW(24); V_Section2Calc(); V_UpdateVars();
    V_CalcW(25); V_Section2Calc(); V_UpdateVars();
    V_CalcW(26); V_Section2Calc(); V_UpdateVars();
    V_CalcW(27); V_Section2Calc(); V_UpdateVars();
    V_CalcW(28); V_Section2Calc(); V_UpdateVars();
    V_CalcW(29); V_Section2Calc(); V_UpdateVars();
    V_CalcW(30); V_Section2Calc(); V_UpdateVars();
    V_CalcW(31); V_Section2Calc(); V_UpdateVars();
    V_CalcW(32); V_Section2Calc(); V_UpdateVars();
    V_CalcW(33); V_Section2Calc(); V_UpdateVars();
    V_CalcW(34); V_Section2Calc(); V_UpdateVars();
    V_CalcW(35); V_Section2Calc(); V_UpdateVars();
    V_CalcW(36); V_Section2Calc(); V_UpdateVars();
    V_CalcW(37); V_Section2Calc(); V_UpdateVars();
    V_CalcW(38); V_Section2Calc(); V_UpdateVars();
    V_CalcW(39); V_Section2Calc(); V_UpdateVars();
  }
  
  // Section 3: 40 - 59
  {
    const uint32x4_t  vK2 = vdupq_n_u32(K2);
    
    V_CalcW(40); V_Section3Calc(); V_UpdateVars();
    V_CalcW(41); V_Section3Calc(); V_UpdateVars();
    V_CalcW(42); V_Section3Calc(); V_UpdateVars();
    V_CalcW(43); V_Section3Calc(); V_UpdateVars();
    V_CalcW(44); V_Section3Calc(); V_UpdateVars();
    V_CalcW(45); V_Section3Calc(); V_UpdateVars();
    V_CalcW(46); V_Section3Calc(); V_UpdateVars();
    V_CalcW(47); V_Section3Calc(); V_UpdateVars();
    V_CalcW(48); V_Section3Calc(); V_UpdateVars();
    V_CalcW(49); V_Section3Calc(); V_UpdateVars();
    V_CalcW(50); V_Section3Calc(); V_UpdateVars();
    V_CalcW(51); V_Section3Calc(); V_UpdateVars();
    V_CalcW(52); V_Section3Calc(); V_UpdateVars();
    V_CalcW(53); V_Section3Calc(); V_UpdateVars();
    V_CalcW(54); V_Section3Calc(); V_UpdateVars();
    V_CalcW(55); V_Section3Calc(); V_UpdateVars();
    V_CalcW(56); V_Section3Calc(); V_UpdateVars();
    V_CalcW(57); V_Section3Calc(); V_UpdateVars();
    V_CalcW(58); V_Section3Calc(); V_UpdateVars();
    V_CalcW(59); V_Section3Calc(); V_UpdateVars();
  }
  
  // Section 3: 60 - 79
  {
    const uint32x4_t  vK3 = vdupq_n_u32(K3);
    
    V_CalcW(60); V_Section4Calc(); V_UpdateVars();
    V_CalcW(61); V_Section4Calc(); V_UpdateVars();
    V_CalcW(62); V_Section4Calc(); V_UpdateVars();
    V_CalcW(63); V_Section4Calc(); V_UpdateVars();
    V_CalcW(64); V_Section4Calc(); V_UpdateVars();
    V_CalcW(65); V_Section4Calc(); V_UpdateVars();
    V_CalcW(66); V_Section4Calc(); V_UpdateVars();
    V_CalcW(67); V_Section4Calc(); V_UpdateVars();
    V_CalcW(68); V_Section4Calc(); V_UpdateVars();
    V_CalcW(69); V_Section4Calc(); V_UpdateVars();
    V_CalcW(70); V_Section4Calc(); V_UpdateVars();
    V_CalcW(71); V_Section4Calc(); V_UpdateVars();
    V_CalcW(72); V_Section4Calc(); V_UpdateVars();
    V_CalcW(73); V_Section4Calc(); V_UpdateVars();
    V_CalcW(74); V_Section4Calc(); V_UpdateVars();
    V_CalcW(75); V_Section4Calc(); V_UpdateVars();
    V_CalcW(76); V_Section4Calc(); V_UpdateVars();
    V_CalcW(77); V_Section4Calc(); V_UpdateVars();
    V_CalcW(78); V_Section4Calc(); V_UpdateVars();
    V_CalcW(79); V_Section4Calc(); // V_UpdateVars();
  }
  
  // interesting part from UpdateVars
  b = a;
  a = temp;
  
  t0 = vdupq_n_u32(H0);
  t0 = vaddq_u32(t0, a);
  
  t1 = vdupq_n_u32(H1);
  t1 = vaddq_u32(t1, b);
  
  // swap endianness
  t0 = (uint32x4_t)vrev32q_u8((uint8x16_t)t0);
  t1 = (uint32x4_t)vrev32q_u8((uint8x16_t)t1);
  
  uint64_t  preSeed = vgetq_lane_u32(t1, 0);
  preSeed = (preSeed << 32) | vgetq_lane_u32(t0, 0);
  result[0] = LCRNG5::NextForSeed(preSeed);
  
  preSeed = vgetq_lane_u32(t1, 1);
  preSeed = (preSeed << 32) | vgetq_lane_u32(t0, 1);
  result[1] = LCRNG5::NextForSeed(preSeed);
  
  preSeed = vgetq_lane_u32(t1, 2);
  preSeed = (preSeed << 32) | vgetq_lane_u32(t0, 2);
  result[2] = LCRNG5::NextForSeed(preSeed);
  
  preSeed = vgetq_lane_u32(t1, 3);
  preSeed = (preSeed << 32) | vgetq_lane_u32(t0, 3);
  result[3] = LCRNG5::NextForSeed(preSeed);
}

#endif

}
