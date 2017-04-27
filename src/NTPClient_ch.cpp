/**
 * The MIT License (MIT)
 * Copyright (c) 2015 by Fabrice Weinberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "NTPClient_ch.h"


NTPClient::NTPClient(UDP& udp) {
  this->_udp            = &udp;
}

NTPClient::NTPClient(UDP& udp, int timeOffset) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName) {
  this->_udp            = &udp;
  this->_poolServerName = poolServerName;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName, int timeOffset) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
  this->_poolServerName = poolServerName;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName, int timeOffset, int updateInterval) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
  this->_poolServerName = poolServerName;
  this->_updateInterval = updateInterval;
}

void NTPClient::begin() {
  this->begin(NTP_DEFAULT_LOCAL_PORT);
}

void NTPClient::begin(int port) {
  this->_port = port;

  this->_udp->begin(this->_port);

  this->_udpSetup = true;
}

void NTPClient::updateDate() {
  unsigned long days, rem;
  unsigned int y;
  char *ip; 
  const unsigned char __mon_lengths[2][12] = 
  {
    /* Normal years.  */
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    /* Leap years.  */
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
  };
  
  days = this->getEpochTime() / SECS_PER_DAY;
  rem = this->getEpochTime() % SECS_PER_DAY;
  while (rem < 0) {
    rem += SECS_PER_DAY;
    --days;
  }
  while (rem >= SECS_PER_DAY) {
    rem -= SECS_PER_DAY;
    ++days;
  }
//  tbuf->tm_hour = rem / SECS_PER_HOUR;
  rem %= SECS_PER_HOUR;
//  tbuf->tm_min = rem / 60;
//  tbuf->tm_sec = rem % 60;
  // January 1, 1970 was a Thursday.
  this->_weekday = (4 + days) % 7;
  if (this->_weekday < 0)
  this->_weekday += 7;
  y = 1970;
  while (days >= (rem = __isleap(y) ? 366 : 365)) {
    ++y;
    days -= rem;
  }

  while (days < 0) {
    --y;
    days += __isleap(y) ? 366 : 365;
  }
  this->_year = y;
  // this->_yday = days; // days since January 1 - [0,365]
  ip = (char *)__mon_lengths[__isleap(y)];
  for (y = 0; days >= ip[y]; ++y)
  days -= ip[y];
  this->_month = y + 1;
  this->_day = days + 1;
}

bool NTPClient::forceUpdate() {
  #ifdef DEBUG_NTPClient
    Serial.println("Update from NTP Server");
  #endif

  this->sendNTPPacket();

  // Wait till data is there or timeout...
  byte timeout = 0;
  int cb = 0;
  do {
    delay ( 10 );
    cb = this->_udp->parsePacket();
    if (timeout > 100) return false; // timeout after 1000 ms
    timeout++;
  } while (cb == 0);

  this->_lastUpdate = millis() - (10 * (timeout + 1)); // Account for delay in reading the time

  this->_udp->read(this->_packetBuffer, NTP_PACKET_SIZE);

  unsigned long highWord = word(this->_packetBuffer[40], this->_packetBuffer[41]);
  unsigned long lowWord = word(this->_packetBuffer[42], this->_packetBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;

  this->_currentEpoc = secsSince1900 - SEVENZYYEARS;

  this->updateDate();

/* Sommerzeit */
 
  // last sunday of march
  int beginDSTDate=  (31 - (5* this->_year /4 + 4) % 7);
  Serial.println("Actual date:");
  Serial.print(this->_day);
  Serial.print(".");
  Serial.print(this->_month);
  Serial.print(".");
  Serial.println(this->_year);
  Serial.println("Summertime from:");
  Serial.print(beginDSTDate);
  Serial.print(".3.");
  Serial.println(this->_year);
//  Serial.println(month);
//  Serial.println(year);
  int beginDSTMonth=3;
  //last sunday of october
  int endDSTDate= (31 - (5 * this->_year /4 + 1) % 7);
  int endDSTMonth=10;
  Serial.println("Summertime to:");
  Serial.print(endDSTDate);
  Serial.print(".10.");
  Serial.println(this->_year);
  // DST is valid as:
  if (((this->_month > beginDSTMonth) && (this->_month < endDSTMonth))
    || ((this->_month == beginDSTMonth) && (this->_day > beginDSTDate)) 
    || ((this->_month == beginDSTMonth) && (this->_day == beginDSTDate) && (this->getHoursUTC() > 0)) 
    || ((this->_month == endDSTMonth) && (this->_day < endDSTDate))
    || ((this->_month == endDSTMonth) && (this->_day == endDSTDate) &&  (this->getHoursUTC() < 1))) {
    this->setTimeOffset(7200);  // DST europe = utc +2 hour
  } else {
    this->setTimeOffset(3600); // nonDST europe = utc +1 hour
  }

  return true;
}

bool NTPClient::update() {
  if ((millis() - this->_lastUpdate >= this->_updateInterval)     // Update after _updateInterval
    || this->_lastUpdate == 0) {                                // Update if there was no update yet.
    if (!this->_udpSetup) this->begin();                         // setup the UDP client if needed
    return this->forceUpdate();
  }
  return true;
}

unsigned long NTPClient::getEpochTime() {
  return this->_timeOffset + // User offset
         this->_currentEpoc + // Epoc returned by the NTP server
         ((millis() - this->_lastUpdate) / 1000); // Time since last update
}

int NTPClient::getDay() {
  return (((this->getEpochTime()  / 86400L) + 4 ) % 7); //0 is Sunday
}
int NTPClient::getHours() {
  return ((this->getEpochTime()  % 86400L) / 3600);
}
int NTPClient::getHoursUTC() {
  return (((this->_currentEpoc + // Epoc returned by the NTP server
         ((millis() - this->_lastUpdate) / 1000)) // Time since last update
         % 86400L) / 3600);
}
int NTPClient::getMinutes() {
  return ((this->getEpochTime() % 3600) / 60);
}
int NTPClient::getSeconds() {
  return (this->getEpochTime() % 60);
}

String NTPClient::getFormattedTime() {
  unsigned long rawTime = this->getEpochTime();
  unsigned long hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  unsigned long seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr + ":" + secondStr;
}

void NTPClient::end() {
  this->_udp->stop();

  this->_udpSetup = false;
}

void NTPClient::setTimeOffset(int timeOffset) {
  this->_timeOffset     = timeOffset;
}

void NTPClient::setUpdateInterval(int updateInterval) {
  this->_updateInterval = updateInterval;
}

void NTPClient::sendNTPPacket() {
  // set all bytes in the buffer to 0
  memset(this->_packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  this->_packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  this->_packetBuffer[1] = 0;     // Stratum, or type of clock
  this->_packetBuffer[2] = 6;     // Polling Interval
  this->_packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  this->_packetBuffer[12]  = 49;
  this->_packetBuffer[13]  = 0x4E;
  this->_packetBuffer[14]  = 49;
  this->_packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  this->_udp->beginPacket(this->_poolServerName, 123); //NTP requests are to port 123
  this->_udp->write(this->_packetBuffer, NTP_PACKET_SIZE);
  this->_udp->endPacket();
}
