
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <Ticker.h> //for LED status
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <EEPROM.h>
#include "LM75.h"
//#include "fonts/pixel3x4.h"
//#include <Fonts/FreeSans9pt7b.h>
#include "fonts/pixel5pt7b.h"
#include "fonts/pixel3x5.h"
#include "NTPClient_ch.h"
#include "TelegramBot.h"


/* Todo:
 *  EEPROM
 *  Umlaute in Telegram-Nachrichten
 *  Umbau Telegram auf asynchron
 *  Nachrichten unendlich durchscrollen
 *  ds-tools.local umbenennen (webseite u.s.w.)
 *  Knöpfe auswerten + Bedienkonzept
 *   
 *   
 *  Telegram Konfiguration (Botfather)
 *  Farbsets definieren
 *  Farbwahl auf HSB
 *  
 *  SSID lokal konfigurieren
 *  OTA Update
 *   
*/
WiFiUDP ntpUDP;


int show = 1;

int16_t sensor_temperature=0;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 3600000); // update 1x pro Stunde
//NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 5000);

Ticker ticker;
ESP8266WebServer server ( 80 );
LM75 sensor;
#define PIN D5 // was: 4, was: 6

// Taster: D3 (GPIO0), D6 (GPIO12), D7 (GPIO13), LDR=A0

#define KEY1 D3
#define KEY2 D6
#define KEY3 D7

// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0
#define WHITE    0xFFFF

#define MAXCLOCKFACE 7

#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 500;
DynamicJsonBuffer jsonBuffer(bufferSize);
const char* sessionToken = "";

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, PIN,
                            NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
                            NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
                            NEO_GRB            + NEO_KHZ800);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255)
};

//#define BOTtoken "334204835:AAG-lutKIHNsSFkztYbLF-lozD-3_ONbZQ0"  //token of BOT

TelegramBOT bot("");
#define SCROLL_RESET 16380
String scrollmessage;
int16_t _scrmsgcnt = SCROLL_RESET;
uint8_t _repeatmessage = 0;

uint16_t color1;
uint16_t color2;
uint16_t color3;
uint16_t color4;
uint16_t color5;
uint16_t color6;

#define STARTMARKER 0x98765432
#define ENDMARKER   0xd570015

union _EEP {
  struct __attribute__((packed)) {
    uint32_t startmarker; // magic number
    uint8_t minBrightness;
    uint8_t maxBrightness;
    uint8_t clockface;
    int8_t  temperatureOffset;
    uint16_t p1i;
    uint16_t p2i;
    uint16_t p3i;
    uint16_t p4i;
    uint16_t p5i;
    uint16_t p6i;
    uint8_t  selectedcolor;
    char     BotToken[60];  // normal size: 45 characters
    uint32_t endmarker; // magic number
  } data;
  byte buf[0];
} eep;

void initEEPROM() {
  eep.data.startmarker = STARTMARKER;
  eep.data.endmarker = ENDMARKER;
  eep.data.minBrightness = 1;
  eep.data.maxBrightness = 250;
  eep.data.clockface = 7;
  eep.data.temperatureOffset = -40; // -52 = 5,2 Grad abziehen
  eep.data.p1i = matrix.Color(255, 0, 255);
  eep.data.p2i = matrix.Color(255, 0, 150);
  eep.data.p3i = matrix.Color(255, 0, 50);
  eep.data.p4i = matrix.Color(255, 255, 0);
  eep.data.p5i = matrix.Color(255, 150, 0);
  eep.data.p6i = matrix.Color(0, 255, 0);
  eep.data.BotToken[0] = 0;
  eep.data.selectedcolor = 0;
}

void loadEEPROM(void) {
  uint16_t i;
  uint16_t seep = sizeof(eep);
  
  for(i=0; i<seep; i++) {
    eep.buf[i] = EEPROM.read(i); 
  }
}

void saveEEPROM(void) {
  uint16_t i;
  uint16_t seep = sizeof(eep);
  
  for(i=0; i<seep; i++) {
    EEPROM.write(i, eep.buf[i]); 
  }
  EEPROM.commit();
}

String convertUnicodeToASCII(String unicodeStr){
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
           case 196: conv = 142; break;  //Ä
           case 214: conv = 153; break;  //Ö
           case 220: conv = 154; break;  //Ü
           case 223: conv = 224; break;  //ß
           case 228: conv = 132; break;  //ä
           case 246: conv = 148; break;  //ö
           case 252: conv = 129; break;  //ü
         }
         
         out += (char)conv;
       } else if(iChar == '/'){
         out += iChar;
       } else if(iChar == 'n'){
         out += '\n';
       }
     } else {
       out += iChar;
     }
  }
  return out;
}

/********************************************
   EchoMessages - function to Echo messages
 ********************************************/
void Bot_ExecMessages() {
  for (int i = 1; i < bot.message[0][0].toInt() + 1; i++)      {
    Serial.print("Bot-Nachricht ");
    Serial.println(i);
    Serial.println(bot.message[i][5]);
    //    bot.message[i][5]=bot.message[i][5].substring(1,bot.message[i][5].length());
    String cmd = bot.message[i][5].substring(0, 2);
    if (cmd == "/f") {
      String val = bot.message[i][5].substring(3, bot.message[i][5].length());
      bot.sendMessage(bot.message[i][4], "Clockface " + val, "");
      eep.data.clockface = val.toInt();
      saveEEPROM();
    }
    if (cmd == "/c") {
      String space = bot.message[i][5].substring(2, 3);
      String colNo,rgb,val;
      if (space != " ") {
        colNo = bot.message[i][5].substring(2, 3);
        rgb = bot.message[i][5].substring(3, 4);
        val = bot.message[i][5].substring(4, bot.message[i][5].length());
      } else {
        colNo = bot.message[i][5].substring(3, 4);
        rgb = bot.message[i][5].substring(4, 5);
        val = bot.message[i][5].substring(5, bot.message[i][5].length());
      }
      uint16_t col;
      switch (colNo.toInt()) {
        case 1: col = eep.data.p1i; break;
        case 2: col = eep.data.p2i; break;
        case 3: col = eep.data.p3i; break;
        case 4: col = eep.data.p4i; break;
        case 5: col = eep.data.p5i; break;
        case 6: col = eep.data.p6i; break;
      }
      uint16_t col_r = (col & 0xf800) >> 8;
      uint16_t col_g = (col & 0x7e0) >> 3;
      uint16_t col_b = (col & 0x1f) << 3;
      if (rgb == "r")
        col_r = val.toInt();
      if (rgb == "g")
        col_g = val.toInt();
      if (rgb == "b")
        col_b = val.toInt();
      col = matrix.Color(col_r, col_g, col_b);
      
      bot.sendMessage(bot.message[i][4], "set color " + colNo + " type "+rgb+" to "+val+", color is R="+String(col_r)+", G="+String(col_g)+", B="+String(col_b), "");

      switch (colNo.toInt()) {
        case 1: eep.data.p1i = col; break;
        case 2: eep.data.p2i = col; break;
        case 3: eep.data.p3i = col; break;
        case 4: eep.data.p4i = col; break;
        case 5: eep.data.p5i = col; break;
        case 6: eep.data.p6i = col; break;
      }
      eep.data.selectedcolor = 0;
      selectColor();
      saveEEPROM();
    }
    if (cmd == "/m") {
      String msg = bot.message[i][5].substring(3, bot.message[i][5].length());
      if (msg == "") {
        _repeatmessage = 0;
        show = 1;
        bot.sendMessage(bot.message[i][4], "Message deleted", "");
      } else {
        Serial.println("Nachricht: "+msg);
        scrollmessage = convertUnicodeToASCII(msg);
        _repeatmessage = 1;
        Serial.println("nach Uni.: "+scrollmessage);
        bot.sendMessage(bot.message[i][4], "Message: " + msg, "");
        _scrmsgcnt=SCROLL_RESET;
        show = 2;
      }
    }
    if ( (cmd == "/s") || (cmd == "/h") ) {
      bot.sendMessage(bot.message[i][4], "Welcome to LED Matrix Clock", "");
      bot.sendMessage(bot.message[i][4], "/f x: switch to clockface x [1.."+String(MAXCLOCKFACE)+"]", "");
      bot.sendMessage(bot.message[i][4], "/c : change color. 6 colors can be changed, each has red (r), green (r) and blue (r) components. To change color 1 red to value 200, type: /c 1r200. Value range [0..255]", "");
      bot.sendMessage(bot.message[i][4], "/m msg: send message msg", "");
      bot.sendMessage(bot.message[i][4], "/m : /m without any text deletes current message", "");
    }
  }
  bot.message[0][0] = "";   // All messages have been replied - reset new messages
}

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
  matrix.fillScreen(0);
  matrix.setCursor(0, 0);
  matrix.setTextColor(RED);
  matrix.setFont(0);
  matrix.print("WLAN");
  matrix.show();
}

void handleTelegram() {
  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "LED Matrix Clock");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_HEAD_END);
  page += "<h1>";
  page += "LED Matrix Clock";
  page += "</h1>";
  page += F("<h3>Telegram Bot</h3>");
  
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    if (server.argName(i)[0] == 't') {
      strncpy(eep.data.BotToken, server.arg(i).c_str(), 59);
      page += "Bot token saved!<br/><br/>";
      saveEEPROM();
      bot.setToken(eep.data.BotToken);
    }
  }

  page += "<br/>Enter Telegram Bot token here. Leave blank to disable Bot.<br/>If you need help on how to create a Bot token: <a href='https://ds-tools.net/ledmatrix' target='_blank'>click here</a>.<br/>";

  page += "<br/><form method='post' action='telegram'><input id='t' name='t' maxlength=58 placeholder='Telegram Bot token' value='"+String(eep.data.BotToken)+"'><br/>";
  page += "<br/><button type='submit'>save</button></form><br/><hr>";

  page += "<br/><form action=\"/\" method=\"get\"><button>Go back</button></form><br/>";

  page += FPSTR(HTTP_END);

  server.send(200, "text/html", page);
}

void handleOptions() {
  String p1r = "255";
  String p1g = "0";
  String p1b = "0";
  String p2r = "0";
  String p2g = "255";
  String p2b = "0";
  String p3r = "0";
  String p3g = "0";
  String p3b = "255";
  String wf = "1"; // clockface
  String bright = "255";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    if (server.argName(i)[0] == 'w')
      wf = server.arg(i);
    if (server.argName(i)[0] == 'b')
      bright = server.arg(i);
    if (server.argName(i)[0] == 'p') {
      if (server.argName(i)[1] == '1') {
        if (server.argName(i)[2] == 'r')
          p1r = server.arg(i);
        if (server.argName(i)[2] == 'g')
          p1g = server.arg(i);
        if (server.argName(i)[2] == 'b')
          p1b = server.arg(i);
      }
      if (server.argName(i)[1] == '2') {
        if (server.argName(i)[2] == 'r')
          p2r = server.arg(i);
        if (server.argName(i)[2] == 'g')
          p2g = server.arg(i);
        if (server.argName(i)[2] == 'b')
          p2b = server.arg(i);
      }
      if (server.argName(i)[1] == '3') {
        if (server.argName(i)[2] == 'r')
          p3r = server.arg(i);
        if (server.argName(i)[2] == 'g')
          p3g = server.arg(i);
        if (server.argName(i)[2] == 'b')
          p3b = server.arg(i);
      }
    }
  }

  //  matrix.fillScreen(0);
  matrix.setBrightness(bright.toInt());

  color1 = ((p1r.toInt() >> 3) << 11) | ((p1g.toInt() >> 2) << 5) | (p1b.toInt() >> 3);
  color2 = ((p2r.toInt() >> 3) << 11) | ((p2g.toInt() >> 2) << 5) | (p2b.toInt() >> 3);
  color3 = ((p3r.toInt() >> 3) << 11) | ((p3g.toInt() >> 2) << 5) | (p3b.toInt() >> 3);
  eep.data.clockface = wf.toInt();
  saveEEPROM();
  /*
    Serial.print("p1:");
    Serial.printlncolor1);
    Serial.print("p1r-Int:");
    Serial.println(p1r.toInt());
    Serial.print("p1r:");
    Serial.println(p1r);

    Serial.print("p2:");
    Serial.printlncolor2);
    Serial.print("p3:");
    Serial.printlncolor3);
  */
  /*  matrix.drawPixel(0,0,p1i);
    matrix.drawPixel(1,0,p2i);
    matrix.drawPixel(0,1,p3i);
    matrix.show();
  */

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "LED Matrix Clock - colors");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_HEAD_END);
  page += "<h1>";
  page += "LED Matrix Clock - colors";
  page += "</h1>";
  page += F("<h3>LEDs</h3>");

  page += "<br/><form method='post' action='options'>";
  page += "Clockface: <input id='wf' name='wf' length=1 value='" + wf + "'><br/>";
  page += "Helligkeit: <input id='bright' name='bright' length=3 value='" + bright + "'><br/>";
  page += "<hr>";
  page += "Farbe 1 rot: <input id='p1r' name='p1r' length=3 value='" + p1r + "'><br/>";
  page += "Farbe 1 gr&uuml;n: <input id='p1g' name='p1g' length=3 value='" + p1g + "'><br/>";
  page += "Farbe 1 blau: <input id='p1b' name='p1b' length=3 value='" + p1b + "'><br/>";
  page += "<hr>";
  page += "Farbe 2 rot: <input id='p2r' name='p2r' length=3 value='" + p2r + "'><br/>";
  page += "Farbe 2 gr&uuml;n: <input id='p2g' name='p2g' length=3 value='" + p2g + "'><br/>";
  page += "Farbe 2 blau: <input id='p2b' name='p2b' length=3 value='" + p2b + "'><br/>";
  page += "<hr>";
  page += "Farbe 3 rot: <input id='p3r' name='p3r' length=3 value='" + p3r + "'><br/>";
  page += "Farbe 3 gr&uuml;n: <input id='p3g' name='p3g' length=3 value='" + p3g + "'><br/>";
  page += "Farbe 3 blau: <input id='p3b' name='p3b' length=3 value='" + p3b + "'><br/>";
  page += "<hr>";
  page += "<br/><button type='submit'>setzen</button></form><br/>";

  page += FPSTR(HTTP_END);

  server.send(200, "text/html", page);
}

void handleRoot() {
  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "LED Matrix Clock Configuration");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_HEAD_END);
  page += "<h1>";
  page += "LED Matrix Clock Configuration";
  page += "</h1>";
  page += F("<h3>Configuration</h3>");
  page += "<form action=\"/telegram\" method=\"get\"><button>Telegram Bot configuration</button></form><br/>\
           <form action=\"/options\" method=\"get\"><button>Display options</button></form><br/>";
  page += FPSTR(HTTP_END);

  server.send(200, "text/html", page);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

#define MAXTEMPHIST 16
int _tempHistory[MAXTEMPHIST] = {0}; /* history of the last values used for averaging */
int _tempHP = 0; /* actual write position in history buffer */

void setup() {
  String ssid = "ledclock_" + String(ESP.getChipId());
  matrix.begin();
  matrix.setTextWrap(false);
  //  matrix.setFont(&pixel3x4);
  //  matrix.setFont(&FreeSans9pt7b);
  //  matrix.setFont(&pixel5pt7b);

  matrix.setBrightness(160);
  matrix.fillScreen(0);
  matrix.setCursor(0, 0);
  matrix.setTextColor(GREEN);
  matrix.setFont(0);
  matrix.print("start");
  matrix.show();

  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();


  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);

  pinMode(KEY1, INPUT_PULLUP);
  pinMode(KEY2, INPUT_PULLUP);
  pinMode(KEY3, INPUT_PULLUP);

  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.setConnectTimeout(60);
  if (!wifiManager.autoConnect(ssid.c_str())) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  ticker.detach();
  //keep LED off
  digitalWrite(BUILTIN_LED, HIGH);
  //  getURL();
  //  getURL();

  if ( MDNS.begin ( "ledclock" ) ) {
    Serial.println ( "MDNS responder started" );
  }
  MDNS.addService("http", "tcp", 80);

  server.on ( "/", handleRoot );
  server.on ( "/telegram", handleTelegram );
  server.on ( "/options", handleOptions );
  server.on ( "/inline", []() {
    server.send ( 200, "text/plain", "this works as well" );
  } );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );

  timeClient.begin();
  scrollmessage = WiFi.localIP().toString();
  /*
    Serial.println("Flash drive start.");
    SPIFFS.begin();
    if (!SPIFFS.exists("confi.txt")) {
      Serial.println("Config not found, formatting...");
      SPIFFS.format();
      Serial.println("Formatting finished.");
      File f = SPIFFS.open("config.txt", "w");
      if (!f) {
        Serial.println("File open for write failed");
      } else {
        f.println("test");
        f.close();
      }
    }
    SPIFFS.end();
  */
  /*
    Serial.println("Read config:");
    File f = SPIFFS.open("config.txt", "r");
    if (!f) {
      Serial.println("File open for read failed");
    } else {
    //    while (f.available()) {
    //      Serial.write(f.read());
    //    }
      f.close();
    }
  */
  EEPROM.begin(512);
  Serial.println("Lese EEPROM");
  loadEEPROM();
  if ( (eep.data.startmarker != STARTMARKER) || (eep.data.endmarker != ENDMARKER) ) {
    Serial.println("EEPROM muss initialisiert werden");
    initEEPROM();
    saveEEPROM();
  }

  int16_t tmp = round(sensor.temp() * 10) + eep.data.temperatureOffset;
  for (uint8_t i = 0; i < MAXTEMPHIST; i++)
    _tempHistory[i] = tmp; // first read: fill buffer
  sensor_temperature = tmp;

  selectColor();
  bot.setToken(eep.data.BotToken);

/*  
  Serial.println("EEPROM:");
  Serial.println(eep.data.minBrightness);
  Serial.println(eep.data.maxBrightness);
  Serial.println(sizeof(eep));
  Serial.println("Ausgabe:");
  for (int i = 0; i < 10; i++) {
    char buf[2];
    char *p = 0;
    int zahl;
    p = (char *)eep.buf;
    buf[1] = '\0';
    buf[0] = *(p + i);
    zahl=buf[0];
    Serial.println(buf);
    Serial.println(zahl);
  }
*/


/*
  // ASCII Tabelle ausgeben
  matrix.setTextColor(color1);
  matrix.setFont(0);
  for (int i = 0; i<255; i++) {
    matrix.fillScreen(0);
    matrix.setCursor(0, 0);
    char s[2];
    s[1] = '\0';
    s[0] = i;
    matrix.print(s);
    Serial.print("(");
    Serial.print(i);
    Serial.print(") Buchstabe ");
    Serial.println(s);
    for (int y=0;y<8; y++) {
      for (int x=0; x<8; x++) {
        uint32_t pixel=matrix.getPixel(x,y);
        if (pixel > 0) {
          Serial.print('X');
        } else {
          Serial.print(' ');
        }
      }
      Serial.println("");
    }
    Serial.println("------------");
  }
  
  matrix.show();
*/ 
}

int old_ts = 60;

void clockface1(int th, int tm, int ts) {  // Uhr gross mit blinkendem Doppelpunkt
  matrix.setFont(0);

  matrix.fillScreen(0);
  char str[2] = " ";

  matrix.setTextColor(color1);
  matrix.setCursor(2, 0);
  str[0] = (th / 10) + '0';
  matrix.print(str);
  matrix.setCursor(8, 0);
  str[0] = (th % 10) + '0';
  matrix.print(str);

  matrix.setTextColor(color2);
  matrix.setCursor(18, 0);
  str[0] = (tm / 10) + '0';
  matrix.print(str);
  matrix.setCursor(24, 0);
  str[0] = (tm % 10) + '0';
  matrix.print(str);

  if ((ts % 2) == 0) {
    matrix.setTextColor(color3);
    matrix.setCursor(13, 0);
    matrix.print(":");
  }

  messageReminder();
  matrix.show();
}

void clockface2(int th, int tm, int ts) { // Uhr gross ohne Doppelpunkt
  matrix.setFont(0);

  matrix.fillScreen(0);
  char str[2] = " ";

  matrix.setTextColor(color1);
  matrix.setCursor(3, 0);
  str[0] = (th / 10) + '0';
  matrix.print(str);
  matrix.setCursor(9, 0);
  str[0] = (th % 10) + '0';
  matrix.print(str);

  matrix.setTextColor(color2);
  matrix.setCursor(18, 0);
  str[0] = (tm / 10) + '0';
  matrix.print(str);
  matrix.setCursor(24, 0);
  str[0] = (tm % 10) + '0';
  matrix.print(str);

  messageReminder();
  matrix.show();
}

void clockface3(int th, int tm, int ts) { // Uhr groß mit Sekundenpunkt, ohne Doppelpunkt
  matrix.setFont(0);

  matrix.fillScreen(0);
  char str[2] = " ";

  matrix.setTextColor(color1);
  matrix.setCursor(3, 0);
  str[0] = (th / 10) + '0';
  matrix.print(str);
  matrix.setCursor(9, 0);
  str[0] = (th % 10) + '0';
  matrix.print(str);

  matrix.setTextColor(color2);
  matrix.setCursor(18, 0);
  str[0] = (tm / 10) + '0';
  matrix.print(str);
  matrix.setCursor(24, 0);
  str[0] = (tm % 10) + '0';
  matrix.print(str);

  if ((ts % 2) == 0) {
    matrix.drawPixel(1 + (ts >> 1), 7, color3);
  }

  messageReminder();
  matrix.show();
}

void clockface4(int th, int tm, int ts) {
  matrix.setFont(&pixel5pt7b);

  matrix.fillScreen(0);
  char str[2] = " ";

  matrix.setTextColor(color1);
  matrix.setCursor(0, 1);
  str[0] = (th / 10) + '0';
  matrix.print(str);
  matrix.setCursor(5, 1);
  str[0] = (th % 10) + '0';
  matrix.print(str);

  matrix.setTextColor(color2);
  matrix.setCursor(11, 1);
  str[0] = (tm / 10) + '0';
  matrix.print(str);
  matrix.setCursor(16, 1);
  str[0] = (tm % 10) + '0';
  matrix.print(str);

  matrix.setTextColor(color3);
  matrix.setCursor(22, 1);
  str[0] = (ts / 10) + '0';
  matrix.print(str);
  matrix.setCursor(27, 1);
  str[0] = (ts % 10) + '0';
  matrix.print(str);

  messageReminder();
  matrix.show();
}

void clockface5(int th, int tm, int ts) {
  matrix.setFont(&pixel3x5);

  matrix.fillScreen(0);
  char str[2] = " ";

  matrix.setTextColor(color1);
  matrix.setCursor(1, 1);
  str[0] = (th / 10) + '0';
  matrix.print(str);
  matrix.setCursor(5, 1);
  str[0] = (th % 10) + '0';
  matrix.print(str);

  matrix.drawPixel(10, 2, color4);
  matrix.drawPixel(10, 4, color4);

  matrix.setTextColor(color2);
  matrix.setCursor(11, 1);
  str[0] = (tm / 10) + '0';
  matrix.print(str);
  matrix.setCursor(15, 1);
  str[0] = (tm % 10) + '0';
  matrix.print(str);

  matrix.drawPixel(20, 2, color5);
  matrix.drawPixel(20, 4, color5);

  matrix.setTextColor(color3);
  matrix.setCursor(21, 1);
  str[0] = (ts / 10) + '0';
  matrix.print(str);
  matrix.setCursor(25, 1);
  str[0] = (ts % 10) + '0';
  matrix.print(str);

  messageReminder();
  matrix.show();
}

void clockface6(int th, int tm, int ts) {
  matrix.setFont(&pixel3x5);

  matrix.fillScreen(0);
  char str[2] = " ";

  matrix.setTextColor(color4);
  matrix.setCursor(-1, 1);
  int16_t avgtmp = sensor_temperature;
  if (avgtmp < 0) avgtmp = 0;
  // zehner
  str[0] = (avgtmp / 100) + '0';
  if (str[0] != '0')
    matrix.print(str);
  matrix.setCursor(3, 1);
  // einer
  str[0] = ((avgtmp % 100) / 10) + '0';
  matrix.print(str);
  matrix.drawPixel(7, 5, color5);
  matrix.setCursor(7, 1);
  // nachkomma
  str[0] = (avgtmp % 10) + '0';
  matrix.print(str);

  /*
      matrix.drawPixel(12,1,p3i);
      matrix.drawPixel(12,2,p3i);
      matrix.drawPixel(13,1,p3i);
      matrix.drawPixel(13,2,p3i);
  */

  matrix.drawPixel(12, 1, color5);

  matrix.drawPixel(12, 3, color5);
  matrix.drawPixel(13, 3, color5);
  matrix.drawPixel(12, 4, color5);
  matrix.drawPixel(12, 5, color5);
  matrix.drawPixel(13, 5, color5);


  matrix.setTextColor(color1);
  matrix.setCursor(14, 1);
  str[0] = (th / 10) + '0';
  matrix.print(str);
  matrix.setCursor(18, 1);
  str[0] = (th % 10) + '0';
  matrix.print(str);

  if ((ts % 2) == 0) {
    matrix.drawPixel(23, 2, color3);
    matrix.drawPixel(23, 4, color3);
  }

  matrix.setTextColor(color2);
  matrix.setCursor(24, 1);
  str[0] = (tm / 10) + '0';
  matrix.print(str);
  matrix.setCursor(28, 1);
  str[0] = (tm % 10) + '0';
  matrix.print(str);

  messageReminder();
  matrix.show();
}

void clockface7(int th, int tm, int ts) {
  matrix.setFont(&pixel3x5);

  matrix.fillScreen(0);
  char str[2] = " ";

  int16_t avgtmp = sensor_temperature + 5; // round to full degrees
  if (avgtmp < 0) avgtmp = 0;

  matrix.setTextColor(color4);
  matrix.setCursor(-1, 1);
  str[0] = (avgtmp / 100) + '0';
  if (str[0] != '0')
    matrix.print(str);
  matrix.setCursor(3, 1);
  str[0] = ((avgtmp % 100) / 10) + '0';
  matrix.print(str);

  /*
      matrix.drawPixel(12,1,p3i);
      matrix.drawPixel(12,2,p3i);
      matrix.drawPixel(13,1,p3i);
      matrix.drawPixel(13,2,p3i);
  */

  matrix.drawPixel(8, 1, color5);

  matrix.drawPixel(9, 3, color5);
  matrix.drawPixel(10, 3, color5);
  matrix.drawPixel(9, 4, color5);
  matrix.drawPixel(9, 5, color5);
  matrix.drawPixel(10, 5, color5);


  matrix.setTextColor(color1);
  matrix.setCursor(14, 1);
  str[0] = (th / 10) + '0';
  matrix.print(str);
  matrix.setCursor(18, 1);
  str[0] = (th % 10) + '0';
  matrix.print(str);

  if ((ts % 2) == 0) {
    matrix.drawPixel(23, 2, color3);
    matrix.drawPixel(23, 4, color3);
  }

  matrix.setTextColor(color2);
  matrix.setCursor(24, 1);
  str[0] = (tm / 10) + '0';
  matrix.print(str);
  matrix.setCursor(28, 1);
  str[0] = (tm % 10) + '0';
  matrix.print(str);
  messageReminder();
  matrix.show();
}

void messageReminder() {
  static uint8_t cnt=0;
  if (_repeatmessage) {
    cnt++;
    if (cnt>127) cnt = 0;
    uint8_t val;
    if (cnt>63) {
      val = 255-((cnt-64)*4);
    } else {
      val = cnt*4;
    }
    uint32_t col = matrix.Color(val, val, val);
    for (uint8_t i=0;i<32;i++) {
      matrix.drawPixel(i, 7, col);
    }
    matrix.show();
  }
}


// ACHTUNG: MAXCLOCKFACE oben im DEFINE anpassen!!!
void do_clock() {
  static uint8_t oldclockface = 0;
  static uint8_t oldselectedcolor = 0;
  timeClient.update();
  int th = timeClient.getHours();
  int tm = timeClient.getMinutes();
  int ts = timeClient.getSeconds();
  if (eep.data.clockface != oldclockface) {
    oldclockface = eep.data.clockface;
    old_ts = 0; // display immediately
  }
  if (eep.data.selectedcolor != oldselectedcolor) {
    oldselectedcolor = eep.data.selectedcolor;
    old_ts = 0; // display immediately
  }
  if (old_ts != ts) {
    old_ts = ts;
    switch (eep.data.clockface) {
      case 1:
        clockface1(th, tm, ts);
        break;
      case 2:
        clockface2(th, tm, ts);
        break;
      case 3:
        clockface3(th, tm, ts);
        break;
      case 4:
        clockface4(th, tm, ts);
        break;
      case 5:
        clockface5(th, tm, ts);
        break;
      case 6:
        clockface6(th, tm, ts);
        break;
      case 7:
        clockface7(th, tm, ts);
        break;
      default:
        clockface1(th, tm, ts);
    }
  } else {
    messageReminder();
  }
}

void do_scrollmessage() {
  static uint16_t scrwdth;
  static uint32_t last=1000;
  if ((millis()-last) > 100) {
    last = millis();
    matrix.setFont(0);
    if (_scrmsgcnt == SCROLL_RESET) {
      int16_t  x1, y1;
      uint16_t h;
      char *s = (char*)scrollmessage.c_str();
      matrix.getTextBounds(s, 0, 0, &x1, &y1, &scrwdth, &h);
      Serial.print("Textbreite: ");
      Serial.println(scrwdth);
      _scrmsgcnt = matrix.width();
    }
    matrix.fillScreen(0);
    matrix.setCursor(_scrmsgcnt, 0);
    matrix.setTextColor(color6);
    matrix.print(scrollmessage);
    if (--_scrmsgcnt < (0 - scrwdth)) {
      _scrmsgcnt = SCROLL_RESET;
      show = 1;
    }
    matrix.show();
  }
}

void doLDR() {
  static uint32_t last=0;
  if ((millis()-last) > 1000) {
    last = millis();
    uint16_t ldr = analogRead(A0);
//    Serial.print("LDR: ");
//    Serial.println(ldr);
    int b = ldr / 4;
    if (b < eep.data.minBrightness)
      b = eep.data.minBrightness;
    matrix.setBrightness(b);
  }
}

void doTemperature() {
  static uint32_t last=0;
  if ((millis()-last) > 1000) {
    last = millis();
//    Serial.print("Temp raw: ");
//    Serial.println(sensor.temp());
    int16_t tmp = round(sensor.temp() * 10) + eep.data.temperatureOffset;
    _tempHistory[_tempHP] = tmp; /* store value in history*/
    _tempHP = (_tempHP + 1) % MAXTEMPHIST; /* and increment history pointer */
  
    sensor_temperature = 0;
    for (int i = 0; i < MAXTEMPHIST; i++) {
      sensor_temperature += _tempHistory[i]; /* calculate average. Value range: e.g. 400 = 40.0° * 16 values = 6400, fits in 16 bit integer */
    }
    sensor_temperature /= MAXTEMPHIST;
  
//    Serial.print("Temp sensor: ");
//    Serial.println(sensor_temperature);
  }
}

void checkRepeat() {
  static uint32_t last=0;
  static uint8_t oldshow = 0;
  if (show != oldshow) {
    oldshow = show;
    last = millis();
  }
  if ((show == 1) && (_repeatmessage)) {
    if ((millis()-last) > 5000) {
      show = 2;
    }
  }
}

int16_t hsv_h, hsv_s, hsv_v;
int16_t rgb_r, rgb_g, rgb_b;

void RGBtoHSV(void) {
   // calc min/max
   int16_t cmaxt = 0;
   int16_t cmax = rgb_r;
   int16_t cmint = 0;
   int16_t cmin = rgb_r;

   if (rgb_g > cmax) {
     cmaxt = 1;
     cmax = rgb_g;
   }
   if (rgb_b > cmax) {
     cmaxt = 2;
     cmax = rgb_b;
   }
   if (rgb_g < cmin) {
     cmint = 1;
     cmin = rgb_g;
   }
   if (rgb_b < cmin) {
     cmint = 2;
     cmin = rgb_b;
   }
   Serial.print("cmax=");
   Serial.println(cmax);
   Serial.print("cmaxt=");
   Serial.println(cmaxt);
   Serial.print("cmin=");
   Serial.println(cmin);
   Serial.print("cmint=");
   Serial.println(cmint);

   if (cmax == cmin) {
     hsv_h = 0;
   } else {
      switch (cmaxt) {
         case 0:  // cmax = R?
            hsv_h = 60*(0+(rgb_g-rgb_b)/(cmax-cmin));
            break;
         case 1:  // cmax = G?
            hsv_h = 60*(2+(rgb_b-rgb_r)/(cmax-cmin));
            break;
         case 2:  // cmax = B?
            hsv_h = 60*(4+(rgb_r-rgb_g)/(cmax-cmin));
            break;
      }
   }

   if (hsv_h < 0)
      hsv_h+=360;

   if (cmax == 0) {
      hsv_s = 0;
   } else {
      hsv_s = (cmax-cmin) / cmax;
   }
   hsv_v = cmax;
}

void HSVtoRGB() {
   int16_t hi = floor(hsv_h / 60);
   int16_t f  = (hsv_h / 60 -  hi);
   int16_t p  =  hsv_v * (1 -  hsv_s);
   int16_t q  =  hsv_v * (1 - (hsv_s*f));
   int16_t t  =  hsv_v * (1 - (hsv_s*(1-f)));
/*   
   Serial.print("hi=");
   Serial.println(hi);
   Serial.print("f=");
   Serial.println(f);
   Serial.print("p=");
   Serial.println(p);
   Serial.print("q=");
   Serial.println(q);
   Serial.print("t=");
   Serial.println(t);
*/
   switch (hi) {
      case 0:
      case 6:
         rgb_r = hsv_v;
         rgb_g = t;
         rgb_b = p;
         break;
      case 1:
         rgb_r = q;
         rgb_g = hsv_v;
         rgb_b = p;
         break;
      case 2:
         rgb_r = p;
         rgb_g = hsv_v;
         rgb_b = t;
         break;
      case 3:
         rgb_r = p;
         rgb_g = q;
         rgb_b = hsv_v;
         break;
      case 4:
         rgb_r = t;
         rgb_g = p;
         rgb_b = hsv_v;
         break;
      case 5:
         rgb_r = hsv_v;
         rgb_g = p;
         rgb_b = q;
         break;
   }
}

void setColorToRGB(uint16_t col) {
// rrrrrggggggbbbbb
  rgb_r = (col & 0xf800) >> 8;
  rgb_g = (col & 0x7e0) >> 3;
  rgb_b = (col & 0x1f) << 3;
  Serial.print("R = ");
  Serial.print(rgb_r);
  Serial.print(", G = ");
  Serial.print(rgb_g);
  Serial.print(", B = ");
  Serial.println(rgb_b);
/*  RGBtoHSV();
  Serial.print(",     H = ");
  Serial.print(hsv_h);
  Serial.print(", S = ");
  Serial.print(hsv_s);
  Serial.print(", V = ");
  Serial.println(hsv_v);*/
}

#define MAXSELECTEDCOLOR 5 // max color number+1
void selectColor() {
  Serial.print("Selected Colorset = ");
  Serial.println(eep.data.selectedcolor);
  switch (eep.data.selectedcolor) {
    case 0:
      color1 = eep.data.p1i;
      color2 = eep.data.p2i;
      color3 = eep.data.p3i;
      color4 = eep.data.p4i;
      color5 = eep.data.p5i;
      color6 = eep.data.p6i;
      break;
    case 1:
      color1 = matrix.Color(0xD3, 0xF8, 0xE2);
      color2 = matrix.Color(0xE4, 0xC1, 0xF9);
      color3 = matrix.Color(0xF6, 0x94, 0xC1);
      color4 = matrix.Color(0xED, 0xE7, 0xB1);
      color5 = matrix.Color(0xA9, 0xDE, 0xF9);
      color6 = matrix.Color(0, 255, 0);
      break;
    case 2:
      color1 = matrix.Color(255, 0, 255);
      color2 = matrix.Color(255, 0, 150);
      color3 = matrix.Color(255, 0, 50);
      color4 = matrix.Color(255, 255, 0);
      color5 = matrix.Color(255, 150, 0);
      color6 = matrix.Color(0, 255, 0);
      break;
    case 3:
      color1 = matrix.Color(0x9b, 0xe5, 0x64);
      color2 = matrix.Color(0xd7, 0xf7, 0x5b);
      color3 = matrix.Color(0xd1, 0x9c, 0x1d);
      color4 = matrix.Color(0x7d, 0x45, 0x1b);
      color5 = matrix.Color(0x47, 0x2c, 0x1b);
      color6 = matrix.Color(0, 255, 0);
      break;
    case 4:
      color1 = matrix.Color(255, 255, 255);
      color2 = matrix.Color(255, 255, 255);
      color3 = matrix.Color(255, 255, 255);
      color4 = matrix.Color(255, 255, 255);
      color5 = matrix.Color(255, 255, 255);
      color6 = matrix.Color(255, 255, 255);
      break;
/*    case :
      color1 = matrix.Color(0x, 0x, 0x);
      color2 = matrix.Color(0x, 0x, 0x);
      color3 = matrix.Color(0x, 0x, 0x);
      color4 = matrix.Color(0x, 0x, 0x);
      color5 = matrix.Color(0x, 0x, 0x);
      color6 = matrix.Color(0, 255, 0);*/
      break;
  }

  Serial.println("Color1");
  setColorToRGB(color1);
  Serial.println("Color2");
  setColorToRGB(color2);
  Serial.println("Color3");
  setColorToRGB(color3);
  Serial.println("Color4");
  setColorToRGB(color4);
  Serial.println("Color5");
  setColorToRGB(color5);
  Serial.println("Color6");
  setColorToRGB(color6);
  
}

void button1Press() {
  if (_repeatmessage) {
    _repeatmessage = 0;
    show = 1;
  } else {
    eep.data.selectedcolor++;
    if (eep.data.selectedcolor >= MAXSELECTEDCOLOR)
      eep.data.selectedcolor = 0;
    selectColor();
    saveEEPROM();
  }
}

void button2Press() {
  if (_repeatmessage) {
    _repeatmessage = 0;
    show = 1;
  } else {
    scrollmessage = "IP: "+WiFi.localIP().toString()+" http://ledclock.local";
    _scrmsgcnt=SCROLL_RESET;
    show = 2;
  }
}

void button3Press() {
  if (_repeatmessage) {
    _repeatmessage = 0;
    show = 1;
  } else {
    eep.data.clockface++;
    if (eep.data.clockface>MAXCLOCKFACE)
      eep.data.clockface = 1;
    saveEEPROM();
  }
}

void doButtons() {
  static uint8_t key1buf = 0x07, key2buf = 0x07, key3buf = 0x07;
  static uint8_t oldkey1 = 0, oldkey2 = 0, oldkey3 = 0;
  key1buf = ((key1buf << 1) & 0x07) | digitalRead(KEY1);
  key2buf = ((key2buf << 1) & 0x07) | digitalRead(KEY2);
  key3buf = ((key3buf << 1) & 0x07) | digitalRead(KEY3);

  uint8_t key1=0,key2=0,key3=0;
  if (key1buf == 0) key1 = 1;
  if (key2buf == 0) key2 = 1;
  if (key3buf == 0) key3 = 1;
/*
  Serial.print("Keyauswertung: ");
  Serial.print(key1);
  Serial.print(",");
  Serial.print(key2);
  Serial.print(",");
  Serial.println(key3);
 */

  if (key1 != oldkey1) {
    oldkey1 = key1;
    if (key1)
       button1Press();
  }
 
  if (key2 != oldkey2) {
    oldkey2 = key2;
    if (key2)
       button2Press();
  }

  if (key3 != oldkey3) {
    oldkey3 = key3;
    if (key3)
       button3Press();
  }
}

void loop() {
  static uint8_t cnt = 0;
  cnt++;
  if (cnt == 10) {
    cnt = 0;
    server.handleClient();
  }

  switch (show) {
    case 1: // clock
      do_clock();
      break;
    case 2: // scrollmessage
      do_scrollmessage();
      break;
  }

  doButtons();
  doLDR();
  doTemperature();
  checkRepeat();

  bot.getUpdates(bot.message[0][1]);   // launch API GetUpdates up to xxx message
  Bot_ExecMessages();

  
  delay(10);
}

