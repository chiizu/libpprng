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


#include "HashedSeedMessage.h"

#include "HashedSeedCalculator.h"

using namespace boost::gregorian;

namespace pprng
{

namespace
{

uint32_t SwapEndianess(uint32_t value)
{
  value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
  return (value << 16) | (value >> 16);
}

uint32_t ToBCD(uint32_t value)
{
  uint32_t  thousands = value / 1000;
  uint32_t  allHundreds = value / 100;
  uint32_t  allTens = value / 10;
  
  uint32_t  hundreds = allHundreds - (thousands * 10);
  uint32_t  tens = allTens - (allHundreds * 10);
  
  return (thousands << 12) |
         (hundreds << 8) |
         (tens << 4) |
         (value - (allTens * 10));
}

enum Nazo
{
  // Black / White
  JPBlackNazo = 0x02215f10,
  JPWhiteNazo = 0x02215f30,
  JPBlackDSiNazo = 0x02761150,
  JPWhiteDSiNazo = 0x02761150,
  
  ENBlackNazo = 0x022160B0,
  ENWhiteNazo = 0x022160D0,
  ENBlackDSiNazo = 0x02760190,
  ENWhiteDSiNazo = 0x027601B0,
  
  SPBlackNazo = 0x02216050,
  SPWhiteNazo = 0x02216070,
  SPBlackDSiNazo = 0x027601f0,
  SPWhiteDSiNazo = 0x027601f0,
  
  FRBlackNazo = 0x02216030,
  FRWhiteNazo = 0x02216050,
  FRBlackDSiNazo = 0x02760230,
  FRWhiteDSiNazo = 0x02760250,
  
  DEBlackNazo = 0x02215FF0,
  DEWhiteNazo = 0x02216010,
  DEBlackDSiNazo = 0x027602f0,
  DEWhiteDSiNazo = 0x027602f0,
  
  ITBlackNazo = 0x02215FB0,
  ITWhiteNazo = 0x02215FD0,
  ITBlackDSiNazo = 0x027601d0,
  ITWhiteDSiNazo = 0x027601d0,
  
  KRBlackNazo = 0x022167B0,
  KRWhiteNazo = 0x022167B0,
  KRBlackDSiNazo = 0x02761150,
  KRWhiteDSiNazo = 0x02761150,
  
  
  // Black2 / White2
  JPBlack2Nazo0 = 0x0209A8DC,
  JPBlack2Nazo1 = 0x02039AC9,
  JPBlack2Nazo2DS = 0x021FF9B0,
  JPBlack2Nazo2DSi = 0x027AA730,
  
  JPWhite2Nazo0 = 0x0209A8FC,
  JPWhite2Nazo1 = 0x02039AF5,
  JPWhite2Nazo2DS = 0x021FF9D0,
  JPWhite2Nazo2DSi = 0x027AA5F0,
  
  ENBlack2Nazo0 = 0x0209AEE8,
  ENBlack2Nazo1 = 0x02039DE9,
  ENBlack2Nazo2DS = 0x02200010,
  ENBlack2Nazo2DSi = 0x027A5F70,
  
  ENWhite2Nazo0 = 0x0209AF28,
  ENWhite2Nazo1 = 0x02039E15,
  ENWhite2Nazo2DS = 0x02200050,
  ENWhite2Nazo2DSi = 0x027A5E90,
  
  FRBlack2Nazo0 = 0x0209AF08,
  FRBlack2Nazo1 = 0x02039df9,
  FRBlack2Nazo2DS = 0x02200030,
  FRBlack2Nazo2DSi = 0x027A5F90,
  
  FRWhite2Nazo0 = 0x0209AF28,
  FRWhite2Nazo1 = 0x02039E25,
  FRWhite2Nazo2DS = 0x02200050,
  FRWhite2Nazo2DSi = 0x027A5EF0,
  
  DEBlack2Nazo0 = 0x0209AE28,
  DEBlack2Nazo1 = 0x02039D69,
  DEBlack2Nazo2DS = 0x021FFF50,
  DEBlack2Nazo2DSi = 0x027A6110,
  
  DEWhite2Nazo0 = 0x0209AE48,
  DEWhite2Nazo1 = 0x02039D95,
  DEWhite2Nazo2DS = 0x021FFF70,
  DEWhite2Nazo2DSi = 0x027A6010,
  
  ITBlack2Nazo0 = 0x0209ADE8,
  ITBlack2Nazo1 = 0x02039D69,
  ITBlack2Nazo2DS = 0x021FFF10,
  ITBlack2Nazo2DSi = 0x027A5F70,
  
  ITWhite2Nazo0 = 0x0209AE28,
  ITWhite2Nazo1 = 0x02039D95,
  ITWhite2Nazo2DS = 0x021FFF50,
  ITWhite2Nazo2DSi = 0x027A5ED0,
  
  SPBlack2Nazo0 = 0x0209AEA8,
  SPBlack2Nazo1 = 0x02039DB9,
  SPBlack2Nazo2DS = 0x021FFFD0,
  SPBlack2Nazo2DSi = 0x027A6070,
  
  SPWhite2Nazo0 = 0x0209AEC8,
  SPWhite2Nazo1 = 0x02039DE5,
  SPWhite2Nazo2DS = 0x021FFFF0,
  SPWhite2Nazo2DSi = 0x027A5FB0,
  
  KRBlack2Nazo0 = 0x0209B60C,
  KRBlack2Nazo1 = 0x0203A4D5,
  KRBlack2Nazo2DS = 0x02200750,
  KRBlack2Nazo2DSi = 0x02200770,
  
  KRWhite2Nazo0 = 0x0209B62C,
  KRWhite2Nazo1 = 0x0203A501,
  KRWhite2Nazo2DS = 0x02200770,
  KRWhite2Nazo2DSi = 0x027A57B0
};


static Nazo NazoForParameters(const HashedSeed::Parameters &parameters)
{
  bool isPlainDS = (parameters.consoleType == Console::DS);
  
  switch (parameters.gameColor)
  {
  case Game::Black:
    switch (parameters.gameLanguage)
    {
    case Game::English:
      return isPlainDS ? ENBlackNazo : ENBlackDSiNazo;
    
    case Game::French:
      return isPlainDS ? FRBlackNazo : FRBlackDSiNazo;
      
    case Game::German:
      return isPlainDS ? DEBlackNazo : DEBlackDSiNazo;
      
    case Game::Italian:
      return isPlainDS ? ITBlackNazo : ITBlackDSiNazo;
      
    case Game::Korean:
      return isPlainDS ? KRBlackNazo : KRBlackDSiNazo;
    
    case Game::Japanese:
      return isPlainDS ? JPBlackNazo : JPBlackDSiNazo;
    
    case Game::Spanish:
      return isPlainDS ? SPBlackNazo : SPBlackDSiNazo;
      
    default:
      break;
    }
    break;
    
  case Game::White:
    switch (parameters.gameLanguage)
    {
    case Game::English:
      return isPlainDS ? ENWhiteNazo : ENWhiteDSiNazo;
      
    case Game::French:
      return isPlainDS ? FRWhiteNazo : FRWhiteDSiNazo;
      
    case Game::German:
      return isPlainDS ? DEWhiteNazo : DEWhiteDSiNazo;
      
    case Game::Italian:
      return isPlainDS ? ITWhiteNazo : ITWhiteDSiNazo;
      
    case Game::Japanese:
      return isPlainDS ? JPWhiteNazo : JPWhiteDSiNazo;
    
    case Game::Korean:
      return isPlainDS ? KRWhiteNazo : KRWhiteDSiNazo;
    
    case Game::Spanish:
      return isPlainDS ? SPWhiteNazo : SPWhiteDSiNazo;
      
    default:
      break;
    }
    break;
    
  case Game::Black2:
    switch (parameters.gameLanguage)
    {
    case Game::English:
      return isPlainDS ? ENBlack2Nazo2DS : ENBlack2Nazo2DSi;
      
    case Game::French:
      return isPlainDS ? FRBlack2Nazo2DS : FRBlack2Nazo2DSi;
      
    case Game::German:
      return isPlainDS ? DEBlack2Nazo2DS : DEBlack2Nazo2DSi;
      
    case Game::Italian:
      return isPlainDS ? ITBlack2Nazo2DS : ITBlack2Nazo2DSi;
    
    case Game::Japanese:
      return isPlainDS ? JPBlack2Nazo2DS : JPBlack2Nazo2DSi;
      
    case Game::Korean:
      return isPlainDS ? KRBlack2Nazo2DS : KRBlack2Nazo2DSi;
      
    case Game::Spanish:
      return isPlainDS ? SPBlack2Nazo2DS : SPBlack2Nazo2DSi;
      
    default:
      break;
    }
    break;
    
  case Game::White2:
    switch (parameters.gameLanguage)
    {
    case Game::English:
      return isPlainDS ? ENWhite2Nazo2DS : ENWhite2Nazo2DSi;
    
    case Game::French:
      return isPlainDS ? FRWhite2Nazo2DS : FRWhite2Nazo2DSi;
    
    case Game::German:
      return isPlainDS ? DEWhite2Nazo2DS : DEWhite2Nazo2DSi;
    
    case Game::Italian:
      return isPlainDS ? ITWhite2Nazo2DS : ITWhite2Nazo2DSi;
      
    case Game::Japanese:
      return isPlainDS ? JPWhite2Nazo2DS : JPWhite2Nazo2DSi;
    
    case Game::Korean:
      return isPlainDS ? KRWhite2Nazo2DS : KRWhite2Nazo2DSi;
    
    case Game::Spanish:
      return isPlainDS ? SPWhite2Nazo2DS : SPWhite2Nazo2DSi;
      
    default:
      break;
    }
    break;
    
  default:
    break;
  }
  
  return static_cast<Nazo>(0);
}


static void SetBlack2White2FirstNazos(uint32_t message[],
                                      const HashedSeed::Parameters &parameters)
{
  switch (parameters.gameColor)
  {
  case Game::Black2:
    switch (parameters.gameLanguage)
    {
    case Game::English:
      message[0] = SwapEndianess(ENBlack2Nazo0);
      message[1] = SwapEndianess(ENBlack2Nazo1);
      return;
      
    case Game::French:
      message[0] = SwapEndianess(FRBlack2Nazo0);
      message[1] = SwapEndianess(FRBlack2Nazo1);
      return;
      
    case Game::German:
      message[0] = SwapEndianess(DEBlack2Nazo0);
      message[1] = SwapEndianess(DEBlack2Nazo1);
      return;
      
    case Game::Italian:
      message[0] = SwapEndianess(ITBlack2Nazo0);
      message[1] = SwapEndianess(ITBlack2Nazo1);
      return;
      
    case Game::Japanese:
      message[0] = SwapEndianess(JPBlack2Nazo0);
      message[1] = SwapEndianess(JPBlack2Nazo1);
      return;
      
    case Game::Korean:
      message[0] = SwapEndianess(KRBlack2Nazo0);
      message[1] = SwapEndianess(KRBlack2Nazo1);
      return;
      
    case Game::Spanish:
      message[0] = SwapEndianess(SPBlack2Nazo0);
      message[1] = SwapEndianess(SPBlack2Nazo1);
      return;
      
    default:
      break;
    }
    break;
  
  case Game::White2:
    switch (parameters.gameLanguage)
    {
    case Game::English:
      message[0] = SwapEndianess(ENWhite2Nazo0);
      message[1] = SwapEndianess(ENWhite2Nazo1);
      return;
    
    case Game::French:
      message[0] = SwapEndianess(FRWhite2Nazo0);
      message[1] = SwapEndianess(FRWhite2Nazo1);
      return;
    
    case Game::German:
      message[0] = SwapEndianess(DEWhite2Nazo0);
      message[1] = SwapEndianess(DEWhite2Nazo1);
      return;
    
    case Game::Italian:
      message[0] = SwapEndianess(ITWhite2Nazo0);
      message[1] = SwapEndianess(ITWhite2Nazo1);
      return;
    
    case Game::Japanese:
      message[0] = SwapEndianess(JPWhite2Nazo0);
      message[1] = SwapEndianess(JPWhite2Nazo1);
      return;
    
    case Game::Korean:
      message[0] = SwapEndianess(KRWhite2Nazo0);
      message[1] = SwapEndianess(KRWhite2Nazo1);
      return;
    
    case Game::Spanish:
      message[0] = SwapEndianess(SPWhite2Nazo0);
      message[1] = SwapEndianess(SPWhite2Nazo1);
      return;
      
    default:
      break;
    }
    break;
    
  default:
    break;
  }
  
  message[0] = 0;
  message[1] = 0;
}


enum
{
  BWFirstNazoOffset = 0xFC,
  BWSecondNazoOffset = BWFirstNazoOffset + 0x4C,
  
  BW2NazoOffset = 0x54,
  
  HardResetGxStat = 0x00000006,
  SoftResetGxStat = 0x00000086,
  
  SoftResetTickCount = 0x01000000,
  
  ButtonMask = 0x2FFF
};

static void SetNazos(uint32_t message[],
                     const HashedSeed::Parameters &parameters)
{
  Nazo  nazo = NazoForParameters(parameters);
  
  if (Game::IsBlack2White2(parameters.gameColor))
  {
    SetBlack2White2FirstNazos(message, parameters);
    message[2] = SwapEndianess(nazo);
    message[3] = message[4] = SwapEndianess(nazo + BW2NazoOffset);
  }
  else
  {
    message[0] = SwapEndianess(nazo);
    message[1] = message[2] = SwapEndianess(nazo + BWFirstNazoOffset);
    message[3] = message[4] = SwapEndianess(nazo + BWSecondNazoOffset);
  }
}

void MakeMessage(uint32_t message[], const HashedSeed::Parameters &parameters)
{
  SetNazos(message, parameters);
  
  message[5] = SwapEndianess((parameters.vcount << 16) | parameters.timer0);
  
  message[6] = parameters.macAddress & 0xffff;
  
  message[7] = uint32_t(parameters.macAddress >> 16) ^
               (parameters.vframe << 24);
  
  if (parameters.softResetted)
  {
    message[6] ^= SoftResetTickCount;
    message[7] ^= SoftResetGxStat;
  }
  else
  {
    message[7] ^= HardResetGxStat;
  }
  
  message[8] = ((ToBCD(parameters.date.year()) & 0xff) << 24) |
               ((ToBCD(parameters.date.month()) & 0xff) << 16) |
               ((ToBCD(parameters.date.day()) & 0xff) << 8) |
               (parameters.date.day_of_week() & 0xff);
  
  message[9] = (((ToBCD(parameters.hour) +
                (((parameters.hour >= 12) &&
                  (parameters.consoleType != Console::_3DS)) ?
                 0x40 : 0)) & 0xff) << 24) |
               ((ToBCD(parameters.minute) & 0xff) << 16) |
               ((ToBCD(parameters.second) & 0xff) << 8);
  
  message[10] = 0;
  message[11] = 0;
  
  message[12] = SwapEndianess(parameters.heldButtons ^ ButtonMask);
  
  message[13] = 0x80000000;
  message[14] = 0x00000000;
  message[15] = 0x000001A0; // 416
}

}


HashedSeedMessage::HashedSeedMessage(const HashedSeed::Parameters &parameters,
                                     uint32_t excludedSeasonMask)
  : m_parameters(parameters), m_message(),
    m_monthDays(parameters.date.end_of_month().day()),
    m_excludedSeasonMask(excludedSeasonMask)
{
  MakeMessage(m_message, parameters);
}

HashedSeed HashedSeedMessage::AsHashedSeed() const
{
  return HashedSeed(m_parameters,
                    HashedSeedCalculator::CalculateRawSeed(*this));
}

void HashedSeedMessage::SetMACAddress(uint64_t macAddress)
{
  m_message[6] ^= (m_parameters.macAddress & 0xffff) ^
                  (macAddress & 0xffff);
  m_message[7] ^= (m_parameters.macAddress >> 16) ^
                  (macAddress >> 16);
  
  m_parameters.macAddress = macAddress;
}

void HashedSeedMessage::SetVCount(uint32_t vcount)
{
  m_message[5] = (m_message[5] & 0xffff0000) |
                 ((vcount & 0xff) << 8) | (vcount >> 8);
  
  m_parameters.vcount = vcount;
}

void HashedSeedMessage::SetVFrame(uint32_t vframe)
{
  m_message[7] ^= (m_parameters.vframe << 24) ^ (vframe << 24);
  
  m_parameters.vframe = vframe;
}

void HashedSeedMessage::SetTimer0(uint32_t timer0)
{
  m_message[5] = (m_message[5] & 0xffff) |
                 ((timer0 & 0xff) << 24) | ((timer0 & 0xff00) << 8);
  
  m_parameters.timer0 = timer0;
}

void HashedSeedMessage::SetDate(boost::gregorian::date d)
{
  m_message[8] = ((ToBCD(d.year()) & 0xff) << 24) |
                 ((ToBCD(d.month()) & 0xff) << 16) |
                 ((ToBCD(d.day()) & 0xff) << 8) |
                 (d.day_of_week() & 0xff);
  
  m_monthDays = d.end_of_month().day();
  m_parameters.date = d;
}

void HashedSeedMessage::NextDay()
{
  if (m_parameters.NextDate(m_excludedSeasonMask))
  {
    // a month is being excluded, so resetting the whole value is simpler
    SetDate(m_parameters.date);
    return;
  }
  
  uint32_t  dayInfo = m_message[8] & 0xffff;
  uint32_t  dayOnesDigit = (dayInfo >> 8) & 0xf;
  uint32_t  dayTensDigit = dayInfo >> 12;
  uint32_t  dayInt = (dayTensDigit * 10) + dayOnesDigit;
  uint32_t  dow = dayInfo & 0xff;
  
  if (++dow > 6)
    dow = 0;
  
  if (dayInt == m_monthDays)
  {
    dayInfo = 0x0100 | dow;
    m_monthDays = m_parameters.date.end_of_month().day();
    
    uint32_t  monthInfo = (m_message[8] >> 16) & 0xff;
    
    if (monthInfo == 0x12)
    {
      monthInfo = 0x01;
      
      uint32_t  yearInfo = (m_message[8] >> 24) & 0xff;
      
      if ((++yearInfo & 0xf) > 9)
      {
        yearInfo = (yearInfo & 0xf0) + 0x10;
        if (yearInfo >= 0xa0)
          yearInfo = 0x00;
      }
      
      m_message[8] = (yearInfo << 24) | (monthInfo << 16) | dayInfo;
    }
    else
    {
      if (++monthInfo == 0xA)
      {
        monthInfo = 0x10;
      }
      
      m_message[8] = (m_message[8] & 0xff000000) | (monthInfo << 16) |
                     dayInfo;
    }
  }
  else if (++dayOnesDigit > 9)
  {
    dayOnesDigit = 0;
    ++dayTensDigit;
    m_message[8] = (m_message[8] & 0xffff0000) |
      (dayTensDigit << 12) | (dayOnesDigit << 8) | dow;
  }
  else
  {
    m_message[8] = (m_message[8] & 0xfffff000) | (dayOnesDigit << 8) | dow;
  }
}

void HashedSeedMessage::SetHour(uint32_t hour)
{
  m_message[9] = (m_message[9] & 0x00ffffff) |
    ((ToBCD(hour) +
      (((hour >= 12) && (m_parameters.consoleType != Console::_3DS)) ?
         0x40 : 0)) << 24);
  
  m_parameters.hour = hour;
}

void HashedSeedMessage::NextHour()
{
  if (m_parameters.hour == 23)
  {
    m_parameters.hour = 0;
    m_message[9] = m_message[9] & 0x00ffffff;
    NextDay();
  }
  else if (++m_parameters.hour == 12)
  {
    m_message[9] = (m_message[9] & 0x00ffffff) | 0x12000000 |
      ((m_parameters.consoleType != Console::_3DS) ? 0x40000000 : 0);
  }
  else
  {
    uint32_t  hour = (m_message[9] & 0xff000000) >> 24;
    uint32_t  onesDigit = hour & 0x0f;
    
    if (++onesDigit > 9)
    {
      onesDigit = 0;
      
      uint32_t  tensDigit = (hour & 0xF0) + 0x10;
      
      m_message[9] = (m_message[9] & 0x00ffffff) |
                     ((tensDigit | onesDigit) << 24);
    }
    else
    {
      m_message[9] = (m_message[9] & 0xf0ffffff) | (onesDigit << 24);
    }
  }
}

void HashedSeedMessage::SetMinute(uint32_t minute)
{
  m_message[9] = (m_message[9] & 0xff00ffff) | (ToBCD(minute) << 16);
  
  m_parameters.minute = minute;
}

void HashedSeedMessage::NextMinute()
{
  if (m_parameters.minute == 59)
  {
    m_parameters.minute = 0;
    m_message[9] = m_message[9] & 0xff00ffff;
    NextHour();
  }
  else
  {
    ++m_parameters.minute;
    uint32_t  minute = (m_message[9] & 0xff0000) >> 16;
    uint32_t  onesDigit = minute & 0x0f;
    
    if (++onesDigit > 9)
    {
      onesDigit = 0;
      
      uint32_t  tensDigit = (minute & 0xf0) + 0x10;
      
      m_message[9] = (m_message[9] & 0xff00ffff) |
                     ((tensDigit | onesDigit) << 16);
    }
    else
    {
      m_message[9] = (m_message[9] & 0xfff0ffff) | (onesDigit << 16);
    }
  }
}

void HashedSeedMessage::SetSecond(uint32_t second)
{
  m_message[9] = (m_message[9] & 0xffff00ff) | (ToBCD(second) << 8);
  
  m_parameters.second = second;
}

void HashedSeedMessage::NextSecond()
{
  if (m_parameters.second == 59)
  {
    m_parameters.second = 0;
    m_message[9] = m_message[9] & 0xffff00ff;
    NextMinute();
  }
  else
  {
    ++m_parameters.second;
    uint32_t  second = (m_message[9] & 0xff00) >> 8;
    uint32_t  onesDigit = second & 0x0f;
    
    if (++onesDigit > 9)
    {
      onesDigit = 0;
      
      uint32_t  tensDigit = (second & 0xf0) + 0x10;
      
      m_message[9] = (m_message[9] & 0xffff00ff) |
                     ((tensDigit | onesDigit) << 8);
    }
    else
    {
      m_message[9] = (m_message[9] & 0xfffff0ff) | (onesDigit << 8);
    }
  }
}

void HashedSeedMessage::SetHeldButtons(uint32_t heldButtons)
{
  m_parameters.heldButtons = heldButtons;
  
  heldButtons ^= ButtonMask;
  m_message[12] = ((heldButtons & 0xff) << 24) | ((heldButtons & 0xff00) << 8);
}

void HashedSeedMessage::SetSoftResetted(bool softResetted)
{
  if (softResetted != m_parameters.softResetted)
    FlipSoftResetted();
}

bool HashedSeedMessage::FlipSoftResetted()
{
  m_parameters.softResetted = !m_parameters.softResetted;
  
  m_message[6] ^= SoftResetTickCount;
  m_message[7] ^= SoftResetGxStat ^ HardResetGxStat;
  
  return m_parameters.softResetted;
}

}
