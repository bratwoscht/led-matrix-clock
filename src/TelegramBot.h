/*
  Copyright (c) 2015 Giancarlo Bacchio. All right reserved.
  https://github.com/Gianbacchio/ESP8266-TelegramBot

  TelegramBot - Library to create your own Telegram Bot using 
  ESP8266 on Arduino IDE.
  Ref. Library at https:github/esp8266/Arduino

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  */


#ifndef TelegramBOT_h
#define TelegramBOT_h

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

class TelegramBOT
{
  public:
    TelegramBOT (String);
  	String message[3][6];  // amount of messages read per time  (update_id, name_id, name, lastname, chat_id, text)
    void setToken(String);
  	void analyzeMessages(void);
  	void sendMessage(String chat_id, String text, String reply_markup);
  	void getUpdates(String offset);

  private:
    byte connectToTelegram(unsigned long timeout);
    String _token;
    byte   _stateGU;
    byte   _stateCONN;
    String _command;
    String _mess;
    unsigned long   _now;
    WiFiClientSecure client;
};

#endif
