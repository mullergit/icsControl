#include <Arduino.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>
#include <IRsend.h>
#include <EEPROM.h>
#include "FS.h"
#include <TimeLib.h>
//#include <NtpClientLib.h>
#include <ArduinoJson.h>
//#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <Ticker.h>
#include <FS.h>

#define NLIN 13
#define NCOL 243

#define APSSID "ICSControl"
#define APPSK  "12345678"
const char *myHostname = "icscontrol";//???????
const char *softAP_ssid = APSSID;
const char *softAP_password = APPSK;

Ticker ticker;
WiFiManager wifiManager;
ESP8266WebServer server(80);
const byte DNS_PORT = 53;
DNSServer dnsServer;

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);
const uint16_t kRecvPin = 14;
const uint32_t kBaudRate = 115200;
const uint16_t kCaptureBufferSize = 512;//1024;
const uint8_t kTimeout = 50;
const uint16_t kMinUnknownSize = 12;
#define LEGACY_TIMING_INFO false
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
decode_results results;

const int ledPin = 15;
String ledState;        
 
//  0:22f1  1:22f2  2:22f3 
//  5:23f3  4:23f2  5:23f1
//  6:24f1  7:24f2  8:24f3
//  9:25f3 10:25f2 11:25f1
// 12:off
int sequencia[13]={0,1,2,5,3,4,7,8,6,9,10,11,12};
int countSequencia = 0;
String passos[13]={"Configure o controle remoto em Temperatura 22° e Ventilação(FAN) mínimo","Configure o controle em Ventilação(FAN) médio","Configure em Ventilação(FAN) máximo",
                   "Configure em Temperatura 23°","Configure em Ventilação(FAN) mínimo","Configure em Ventilação(FAN) médio",
                   "Configure em Temperatura 24°","Configure em Ventilação(FAN) máximo","Configure em Ventilação(FAN) mínimo",
                   "Configure em Temperatura 25°","Configure em Ventilação(FAN) médio","Configure em Ventilação(FAN) máximo",
                   "Desligue o controle remoto"};
String msgPassos[13]={"Temp:22° FAN:mín","Temp:22° FAN:méd","Temp:22° FAN:máx",
                      "Temp:23° FAN:máx","Temp:23° FAN:mín","Temp:23° FAN:méd",
                      "Temp:24° FAN:méd","Temp:24° FAN:máx","Temp:24° FAN:mín",
                      "Temp:25° FAN:mín","Temp:25° FAN:méd","Temp:25° FAN:máx",
                      "Desligar   Split"};                  
union {
    unsigned char buf[sizeof(uint8_t) * NLIN * NCOL * 2];
    uint16_t matriz[NLIN][NCOL];//6318 bytes 
}codes;
uint16_t rawCode[NCOL];
int bufLen = NLIN * NCOL * 2;
int addr = 0;  

// Flags:
bool  f_Clone = false, 
      f_AC = false,
      flag=true,
      flagReadEEPROM=true,
      f_hasCode = false,
      ACconfig = false;

//Decode IR:      
uint16_t count;// = results->rawlen;
int ir_type = 0;
uint8_t initDecodeCounter = 3;
int indexMatriz = 0;
int rawSize = 0;

//MUX:
float data[3];
const byte s0 = 5; // low-order bit
const byte s1 = 12;
const byte mux = A0;
int x;
int analog=1023;
int var1, var2;
float adc, n=10, n2=39;

//Timer:
float timer1, timerConfig;


void openFS();
void createFile();
void handleRoot();

DynamicJsonDocument jsonBuffer(1024);
JsonObject root = jsonBuffer.to<JsonObject>();

void updateGpio(){
  String gpio = server.arg("id");
  String etat = server.arg("etat");
  String success = "1";
  int pin = 15;  
 if ( gpio == "D8" ) {
      pin = 15;
 } else if ( gpio == "D7" ) {
     pin = D7;
 } Serial.println(pin);
  if ( etat == "1" ) {
    digitalWrite(pin, HIGH);
  } else if ( etat == "0" ) {
    digitalWrite(pin, LOW);
  } else {
    success = "1";
    Serial.println("Err Led Value");
  }  
  String json = "{\"gpio\":\"" + String(gpio) + "\",";
  json += "\"etat\":\"" + String(etat) + "\",";
  json += "\"success\":\"" + String(success) + "\"}";    
  server.send(200, "application/json", json);
  Serial.println("GPIO mis a jour");
}
void sendMesures() {
  DynamicJsonDocument jsonBuffer(1024);
  JsonObject msg = jsonBuffer.to<JsonObject>();

  msg["t"] = getTemperature();
  msg["h"] = getHumidity();
  msg["pa"] = String(random(0, 15));
  String json;
  serializeJson(msg, json);
  server.send(200, "application/json", json);
  Serial.println("Mesures envoyees");
}
void sendMessage() {
  DynamicJsonDocument jsonBuffer(1024);
  JsonObject msg = jsonBuffer.to<JsonObject>();
  msg["nameId"] = "nameIdjson";
  msg["temp"] = random(20, 29);
  msg["memory"] = ESP.getFreeHeap();
  msg["RSSI"] = WiFi.RSSI();//String(WiFi.RSSI());
  String json;
  serializeJson(msg, json);
  server.send(200, "application/json", json);
          
}
enum State_enum {IDL, DECODE, WIFICONFIG};
enum Sensors_enum {NONE, SENSOR_RIGHT, SENSOR_LEFT, BOTH};
uint8_t mstate = IDL;

void setup() {  
  
  initsetup();
  
  server.serveStatic("/ics", SPIFFS, "/index.html");
  server.serveStatic("/bonjur", SPIFFS, "/bonjur23.html");
  server.serveStatic("/configAC", SPIFFS, "/configAC.html");
  server.serveStatic("/configACmanual", SPIFFS, "/configACmanual.html");
  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.on("/", handleRoot);
  //server.on("/", HTTP_GET, handleRoot, processor());

  server.serveStatic("/js", SPIFFS, "/js");
  server.serveStatic("/css", SPIFFS, "/css");
  server.serveStatic("/img", SPIFFS, "/img");
  //server.serveStatic("/", SPIFFS, "/index.html");
  server.on("/tabmesures.json", sendTabMesures);
  server.on("/mesures.json", sendMesures);
  server.on("/mstate.json", sendMstate);
  server.on("/gravar.json", gravar);
  server.on("/gpio", updateGpio);
  server.on("/initACconfig.json", initDecode);
  server.begin(); // Web server start
}

void sendMstate(){
    Serial.println("sendMstate!");
          DynamicJsonDocument jsonBuffer(1024);
          JsonObject msg = jsonBuffer.to<JsonObject>();
          if(millis()-timerConfig>120000){            
            timer1=millis();
          }else{            
          }
          msg["msg"] = "Aguardando envio dos códigos...";
          msg["mstate"]=mstate;
          String json;
          serializeJson(msg, json);
          server.send(200, "application/json", json);
}
void handleRoot() {
    ACconfig = true;
  if (ACconfig){ // If caprive portal redirect instead of displaying the page.    
   server.sendHeader("Location", "/ics");
   server.send(303);   // Empty content inhibits Content-length header so we have to close the socket ourselves.
    //server.client().stop();
  }else{
    server.sendHeader("Location", "/configAC");
    server.send(303);
  }
  
}
void loop() {
  
  switch(mstate){
    case IDL:
      break;       
    case DECODE:
      
          decodeIr();
      
      break; 
    case WIFICONFIG:
      break;
  }
  MDNS.update();
  dnsServer.processNextRequest();
  server.handleClient();  
  printStatus();
   
}

/*
void inicio(){
    ledpin = D1;
    button = D2;
    buttonState=0;
    delay(200); 

    timeClient.begin();
    timeClient.update();

    Serial.begin(115200);
    digitalWrite(button, LOW);
    pinMode(LED_BUILTIN,OUTPUT);
    pinMode(ledpin,OUTPUT);
    pinMode(button,INPUT);    
    
    EEPROM.begin(512); 
    EEPROM.get(addr,data.buf);          
      
    Serial.println("Time Epoch: "+ String(timeClient.getEpochTime()));//printf
    Serial.println("Time UTC: "+ timeClient.getFormattedTime()+ "  Brazil: UTC-2:00 ");
    
    WiFiManager wifiManager;    

    if(data.field.ipFlag == 0){
        wifiManager.resetSettings(); 
        
        WiFiManagerParameter wifiParameter_IpBanco("ipbanco", "ipbanco2", "ip servidor", 20);
        wifiManager.addParameter(&wifiParameter_IpBanco);  
        
        const char* ipStr = wifiParameter_IpBanco.getValue();//ipbanco;
        char ip[4];
        parseBytes(ipStr, '.', ip, 4, 10);

        data.field.ipFlag = 1;
         
        // Grava Flag, IP, porta, id sala e id bancada na EEPROM
        for (int i = 0; i < sizeof(data.buf); ++i)     
                EEPROM.write(addr+i, data.buf[i]);

        EEPROM.commit();
        Serial.println("\n Gravou flag (modo STA) e dados do servidor na EEPROM......");    

        Serial.print("\n Verificando leitura......");
        EEPROM.get(addr,data.buf);           
        
    }else{
        Serial.println("\n autoConnect......");
        wifiManager.autoConnect("ESP RFID AP");
        
    }
    
    Serial.print("softAPIP address1: "+WiFi.softAPIP());          
    Serial.print("localIP address: ");  Serial.println(WiFi.localIP());
    Serial.print("GatewayIP address: ");  Serial.println(WiFi.gatewayIP().toString());    
    
    //if you get here you have connected to the WiFi
    Serial.println("Conectado na rede WiFi local!");

        
    while ( !timeClient.update() ) {
          if (digitalRead(button)){       
            Serial.print("  Reset na função init() por D2 nível alto: "+digitalRead(button));  //Serial.println(buttonState);
                   
            for (int i = 0; i < sizeof(data.buf); ++i){ 
                    data.buf[i] = 0;    
                    EEPROM.write(addr+i, data.buf[i]);
            }     
            Serial.println("\n Gravou flag (modo AP) na EEPROM......");              
            EEPROM.commit();
            EEPROM.end();
            ESP.reset();
         }
          Serial.println("Time Epoch: "+ String(timeClient.getEpochTime()));//printf
          Serial.println("Time UTC: "+ timeClient.getFormattedTime()+ "  Brazil: UTC-2:00 ");
          delay ( 3000 );
    }   
}*/
