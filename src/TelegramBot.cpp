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


#include "TelegramBot.h"


TelegramBOT::TelegramBOT(String token)	{
	_token=token;
  _stateGU = 0;
  _stateCONN = 0;
  message[0][0]="0";   // number of received messages
  message[1][0]="";    
  message[0][1]="0";  // code of last read Message
}


void TelegramBOT::setToken(String token)  {
  _token=token;
  _stateGU = 0;
  _stateCONN = 0;
  message[0][0]="0";   // number of received messages
  message[1][0]="";    
  message[0][1]="0";  // code of last read Message
}


  
/**************************************************************************************************
 * function to achieve connection to api.telegram.org and send command to telegram                *
 * (Argument to pass: URL to address to Telegram)                                                 *
 **************************************************************************************************/
byte TelegramBOT::connectToTelegram(unsigned long timeout)  {
  bool avail;
  byte ret = 0;
  //Serial.print("connectToTelegram ");Serial.println(_stateCONN);
  switch (_stateCONN) {
    case 0: {
      _mess = "";
      // Connect with api.telegram.org       
      if (client.connect("api.telegram.org", 443)) {  
        //Serial.println("GET: "+_command);
        client.println("GET /"+_command);
        _now=millis();
        _stateCONN++;
      }
    } break;
    case 1: {
      if ( (millis()-_now) > timeout ) {
        ret = 1;
        _stateCONN = 0;
        client.stop();
        Serial.println("Bot Client Timeout");
      } else {
        if (client.available())
          _stateCONN++;
      }
    } break;
    case 2: {
      uint16_t rcvcnt=0;
      while (client.available()) {
        char c = client.read();
        //Serial.print("Received: ");Serial.println(c);
        if (rcvcnt<700)  {
          _mess=_mess+c;
          rcvcnt++;
        }
      }
      ret = 1;
      _stateCONN = 0;
    } break;
  }
  return ret;
}


/***************************************************************
 * GetUpdates - function to receive all messages from telegram *
 * (Argument to pass: the last+1 message to read)             *
 ***************************************************************/
void TelegramBOT::getUpdates(String offset)  {
  if (_token == "")
    return;
//  Serial.print("Millis:");
//  Serial.println(millis());
  switch (_stateGU) {
    case 0: {
//      Serial.println("GET Update Messages ");
      _command="bot"+_token+"/getUpdates?offset="+offset+"&timeout=30";
      _stateGU++;
    } break;
    case 1: {
      byte finished = connectToTelegram(32000);
      if (finished) {
        Serial.println("Message received:");
        Serial.println(_mess);
        if (_mess == "") {
          _stateGU = 0;
        } else {
          _stateGU++;
        }
      }
    } break;
    case 2: {
      // parsing of reply from Telegram into separate received messages
      int i=0;                //messages received counter
      if (_mess!="") {
        Serial.print("Sent Update request messages up to : ");
        Serial.println(offset);
        String a="";
        int ch_count=0;
        String c;
        for (int n=1; n<_mess.length()+1; n++) {   //Search for each message start
          ch_count ++;
          c =  _mess.substring(n-1,n);
          //Serial.print(c);
          a=a+c;
          if (ch_count>8) {
            if (a.substring(ch_count-9)=="update_id")  {
              if (i>1) break;
              message[i][0]=a.substring(0, ch_count-11);
              a=a.substring(ch_count-11);
              i ++;
              ch_count=11;
            }
          }
        }
        if (i==1)  {
          message[i][0]=a.substring(0, ch_count);   //Assign of parsed message into message matrix if only 1 message)
        }
        if (i>1)  i=i-1;
      }
      //check result of parsing process
      if (_mess=="") {     
        Serial.println("failed to update");
      } else {
        if (i==0) {
          Serial.println("no new messages");
          Serial.println();
          message[0][0]="0";
        } else {
          message[0][0]=String(i);   //returns how many messages are in the array
          //Serial.println();         
          for (int b=1; b<i+1; b++)  {
            Serial.println(message[b][0]);
          }
          Serial.println();
          analyzeMessages();
        }
      }
      _stateGU = 0;
    } break;
  } // case
} 



String convAndURLEncode(String unicodeStr){
  String out = "";
  uint16_t len = unicodeStr.length();
  char iChar;
  for (uint16_t i = 0; i < len; i++){
     iChar = unicodeStr[i];
     if(iChar == '\\'){ // got escape char
       iChar = unicodeStr[++i];
       if(iChar == 'u'){ // got unicode hex
         char unicode[7];
         unicode[0] = '0';
         unicode[1] = 'x';
         unicode[6] = '\0';
         for (uint8_t j = 0; j < 4; j++){
           iChar = unicodeStr[++i];
           unicode[j + 2] = iChar;
         }
         uint16_t unicodeVal = strtol(unicode, 0, 16); //convert the string
         uint8_t conv = unicodeVal;
         switch (unicodeVal) {
           case 196: out += "Ae"; break;  //Ä
           case 214: out += "Oe"; break;  //Ö
           case 220: out += "Ue"; break;  //Ü
           case 223: out += "ss"; break;  //ß
           case 228: out += "ae"; break;  //ä
           case 246: out += "oe"; break;  //ö
           case 252: out += "ue"; break;  //ü
           default:
              out+='-';
         }
         
       } else if(iChar == '/'){
         out += iChar;
       } else if(iChar == 'n'){
         out += '\n';
       }
     } else {
       switch (iChar) {
         case ' ':
           out += "%20";
           break;
         case '&':
           out += " und ";
           break;
         default:
           out += iChar;
       }
     }
  }
  return out;
}


/***********************************************************************
 * SendMessage - function to send message to telegram                  *
 * (Arguments to pass: chat_id, text to transmit and markup(optional)) *
 ***********************************************************************/
void TelegramBOT::sendMessage(String chat_id, String text, String reply_markup)  {
  bool sent=false;
  if (_token == "")
    return;
//  Serial.println("SEND Message ");
  _stateCONN = 0;
  _stateGU = 0;
  client.stop();
  uint32_t sttime=millis();
  if (text!="") {
    while (millis()<sttime+8000) {    // loop for a while to send the message
      String command="bot"+_token+"/sendMessage?chat_id="+chat_id+"&text="+convAndURLEncode(text)+"&reply_markup="+reply_markup;

      if (client.connect("api.telegram.org", 443)) {  
        Serial.println("GET: "+command);
        client.println("GET /"+command);
        uint32_t now=millis();
        bool timeout = true;
        while ((millis()-now) < 1500) {
          if (client.available()) {
            timeout = false; 
            break;
          }
        }
        String mess = "";
        if (timeout) {
          Serial.printf("Timeout while sending telegram message");
          client.stop();
        } else {
          uint16_t rcvcnt = 0;
          while (client.available()) {
            char c = client.read();
            if (rcvcnt<700)  {
              mess=mess+c;
              rcvcnt++;
            }
          }
        }
      
        //Serial.println(mess);
        int messageLength=mess.length();
        for (int m=5; m<messageLength+1; m++)  {
          if (mess.substring(m-10,m)=="{\"ok\":true")     {  //Chek if message has been properly sent
            sent=true;
            break;
          }
        }
        if (sent==true)   {
          //  Serial.print("Message delivered: \"");
          //  Serial.print(text);
          //  Serial.println("\"");
          //  Serial.println();
          break;
        }
        delay(1000);
        Serial.println("Retry");
      }
    }
  }
  if (sent==false) Serial.println("Telegram message not delivered");
}

/******************************************************************************
 * AnalizeMessage - function to achieve message parameters from json structure *
 ******************************************************************************/
void TelegramBOT::analyzeMessages(void)     {

  int rif[6]= {0,0,0,0,0,0}; //pointers in message json (update_id, name_id, name, lastname, chat_id, text)
  for (int i=1; i<message[0][0].toInt()+1; i++)      {
    int messageLenght=message[i][0].length();
    String a=message[i][0];
    message[i][5]="";
    for (int m=5; m<messageLenght+1; m++)             {
        if (a.substring(m-12,m)=="\"update_id\":")     { //Search for "update_id" pointer start
          rif[0]=m;
        }
        if (a.substring(m-13,m)=="\"from\":{\"id\":")  { //Search for "from" pointer start
          rif[1]=m;
        }
        if (a.substring(m-14,m)=="\"first_name\":\"")  { //Search for "first_name" pointer start
          rif[2]=m;
        }
        if (a.substring(m-13,m)=="\"last_name\":\"")   { //Search for "last_name" pointer start
          rif[3]=m;
        }
        if (a.substring(m-13,m)=="\"chat\":{\"id\":")  { //Search for "chat" pointer start
          rif[4]=m;
        }
        if (a.substring(m-8,m)=="\"text\":\"")         { //Search for "text" pointer start
	  rif[5]=m;
        }
        for (int n=0; n<2; n++)     {                    //Search for "update_id" and "from" pointers end
            if (a.substring(m-1,m)==",")  {
                if (rif[n]!=0)  {
                  message[i][n]=a.substring(rif[n],m-1);
                }
            rif[n]=0;
            }
        }
        if (a.substring(m-1,m)==",")  {                 //Search for "first name" pointer end
              if (rif[2]!=0)  {
                message[i][2]=a.substring(rif[2],m-2);  //Write value into dedicated slot
              }
          rif[2]=0;
        }
        if (a.substring(m-1,m)==",")  {                 //Search for "last name" pointer end
              if (rif[3]!=0)  {
                message[i][3]=a.substring(rif[3],m-3);  //Write value into dedicated slot
              }
          rif[3]=0;
        }
        if (a.substring(m-1,m)==",")  {                 //Search for "chat" pointer end
              if (rif[4]!=0)  {
                message[i][4]=a.substring(rif[4],m-1);  //Write value into dedicated slot
              }
          rif[4]=0;
        }
        if (a.substring(m-2,m)=="\",")  {               //Search for "text" pointer end
              if (rif[5]!=0)  {
                message[i][5]=a.substring(rif[5],m-2);    //Write value into dedicated slot
            }
          rif[5]=0;
        }
	if (a.substring(m-2,m)=="\"}")  {               //Search for "text" pointer end
              if (rif[5]!=0)  {
                message[i][5]=a.substring(rif[5],m-2);    //Write value into dedicated slot
            }
          rif[5]=0;
        }    
    }
    int id=message[message[0][0].toInt()][0].toInt()+1;
    message[0][1]=id;                                   //Write id of last read message
    
  //  for (int j=0; j<6; j++)	{
  //	Serial.println(message[i][j]);                                //print parsed data
  //  }
  }
}
