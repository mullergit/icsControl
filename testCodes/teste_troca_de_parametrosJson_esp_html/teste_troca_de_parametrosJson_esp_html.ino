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

//#include <ESPAsyncTCP.h>
//#include <ESPAsyncWebServer.h>
#include <FS.h>

#define APSSID "ICSControl"
#define APPSK  "12345678"

const char *myHostname = "icscontrol";//???????
const char *softAP_ssid = APSSID;
const char *softAP_password = APPSK;

Ticker ticker;
WiFiManager wifiManager;

ESP8266WebServer server(80);
//AsyncWebServer server(80);
const int ledPin = 15;
String ledState;

String getTemperature() {
  float temperature = 35;
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  Serial.println(temperature);
  return String(temperature);
}
  
String getHumidity() {
  float humidity = 55;
  Serial.println(humidity);
  return String(humidity);
}

String getPressure() {
  float pressure = 99;
  Serial.println(pressure);
  return String(pressure);
}

// Replaces placeholder with LED state value
String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(ledPin)){
      ledState = "ON";
    }
    else{
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  else if (var == "TEMPERATURE"){
    return getTemperature();
  }
  else if (var == "HUMIDITY"){
    return getHumidity();
  }
  else if (var == "PRESSURE"){
    return getPressure();
  }  
}
const byte DNS_PORT = 53;
DNSServer dnsServer;

void tick(){
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode: configModeCallback()");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

#define NLIN 13
#define NCOL 243

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);
        
//  0:off 
//  1:22f1 2:22f2 3:22f3 
//  4:23f1 5:23f2 6:23f3
//  7:24f1 8:24f2 9:24f3
// 10:25f1 11:25f2 12:25f3
union {
    unsigned char buf[sizeof(uint8_t) * NLIN * NCOL * 2];
    uint16_t matriz[NLIN][NCOL];//6318 bytes 
}codes;

int bufLen = NLIN * NCOL * 2;
int addr = 0;

const uint16_t kRecvPin = 14;
const uint32_t kBaudRate = 115200;
const uint16_t kCaptureBufferSize = 512;//1024;
const uint8_t kTimeout = 50;
const uint16_t kMinUnknownSize = 12;
#define LEGACY_TIMING_INFO false
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
decode_results results;  

bool flag=true;
bool flagReadEEPROM = true;
uint16_t count;// = results->rawlen;
int ir_type = 0;
bool flagClone = false, flagAC = false;
uint8_t initDecodeCounter = 2;
int indexMatriz = 0;
int rawSize = 0;

float data[3];
const byte s0 = 5; // low-order bit
const byte s1 = 12;
const byte mux = A0;
int x;
int analog=1023;
int var1, var2;
float adc, n=10, n2=39;
float timer1 ;
bool ACconfig = false;

void openFS();
void createFile();
void handleRoot();

//HTTPClient http;
//StaticJsonBuffer<10000> jsonBuffer;                 // Buffer static contenant le JSON courant - Current JSON static buffer
//JsonObject& root = jsonBuffer.createObject();
DynamicJsonDocument jsonBuffer(1024);
  JsonObject root = jsonBuffer.to<JsonObject>();
  
//JsonArray& timestamp = root.createNestedArray("timestamp");
char json[10000]; 

void updateGpio(){
  String gpio = server.arg("id");
  String etat = server.arg("etat");
  String success = "1";
  int pin = 15;  
 if ( gpio == "15" ) {
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
  String json = "{\"t\":\"" + getTemperature() + "\",";
  json += "\"h\":\"" + getHumidity() + "\",";
  json += "\"pa\":\"" + getPressure() + "\"}";

  server.send(200, "application/json", json);
  Serial.println("Mesures envoyees");
}
void sendTabMesures() {
  double temp = 0;      // Récupère la plus ancienne mesure (temperature) - get oldest record (temperature)
  String json = "[";
  json += "{\"mesure\":\"Température\",\"valeur\":\"" + String(33) + "\",\"unite\":\"°C\",\"glyph\":\"glyphicon-indent-left\",\"precedente\":\"" + String(temp) + "\"},";
  temp = 0;             // Récupère la plus ancienne mesure (humidite) - get oldest record (humidity)
  json += "{\"mesure\":\"Humidité\",\"valeur\":\"" + String(66) + "\",\"unite\":\"%\",\"glyph\":\"glyphicon-tint\",\"precedente\":\"" + String(temp) + "\"},";
  temp = 0;             // Récupère la plus ancienne mesure (pression atmospherique) - get oldest record (Atmospheric Pressure)
  json += "{\"mesure\":\"Pression Atmosphérique\",\"valeur\":\"" + String(99) + "\",\"unite\":\"mbar\",\"glyph\":\"glyphicon-dashboard\",\"precedente\":\"" + String(temp) + "\"}";
  json += "]";
  server.send(200, "application/json", json);
  Serial.println("Tableau mesures envoyees");
}
void sendMessage() {
  DynamicJsonDocument jsonBuffer(1024);
  JsonObject msg = jsonBuffer.to<JsonObject>();

  msg["nameId"] = "nameIdjson";
  msg["temp"] = random(20, 29);
  msg["veloc"] = random(0, 15);
  msg["memory"] = ESP.getFreeHeap();
  msg["RSSI"] = WiFi.RSSI();//String(WiFi.RSSI());
  String json;
  serializeJson(msg, json);
  server.send(200, "application/json", json);        
 }
void setup() {
    irsend.begin();
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(ledPin, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);
  
  Serial.begin(kBaudRate, SERIAL_8N1, SERIAL_TX_ONLY);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.printf("\n" D_STR_IRRECVDUMP_STARTUP "\n", kRecvPin);
//#if DECODE_HASH
  //irrecv.setUnknownThreshold(kMinUnknownSize);
//#endif  // DECODE_HASH
  irrecv.enableIRIn();  // Start the receiver
  //SPIFFS.format();
  openFS();
  createFile();
  EEPROM.begin(4);//(512);
  pinMode(s0, OUTPUT); 
  pinMode(s1, OUTPUT);
  digitalWrite(s0,  LOW);  
  digitalWrite(s1,  LOW); 
  timer1 = millis();
  
  //wifiManager.resetSettings();
  //WiFi.disconnect(true, true);
  //WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.hostname(myHostname);
  //WiFi.setHostname(myHostname);
  
  wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  wifiManager.setAPCallback(configModeCallback);
  
  if (!wifiManager.autoConnect(softAP_ssid, softAP_password)) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  Serial.println("connected...yeey :)");
  ticker.detach();
  digitalWrite(BUILTIN_LED, LOW);
  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", IPAddress(10,0,1,1));
  
  if (MDNS.begin(myHostname)) {
    MDNS.addService("http", "tcp", 80);
    Serial.print(F("Open http://"));
    Serial.print(myHostname);
    Serial.println(F(".local/edit to open the FileSystem Browser"));
  }
  else{
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }


  server.serveStatic("/ics", SPIFFS, "/index.html");
  server.serveStatic("/bonjur", SPIFFS, "/bonjur23.html");
  server.serveStatic("/configAC", SPIFFS, "/index.html");
  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.on("/", handleRoot);
  //server.on("/", HTTP_GET, handleRoot, processor());

  server.serveStatic("/js", SPIFFS, "/js");
  server.serveStatic("/css", SPIFFS, "/css");
  server.serveStatic("/img", SPIFFS, "/img");
  //server.serveStatic("/", SPIFFS, "/index.html");
  server.on("/tabmesures.json", sendTabMesures);
  server.on("/mesures.json", sendMesures);
  server.on("/gpio", updateGpio);
  server.begin(); // Web server start
  
/*void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}*/
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
  /*if(flag){
    if(flagReadEEPROM){
      printMatriz(codes.matriz);
      readFile1();
      printMatriz(codes.matriz);
      testSend();
      flagReadEEPROM = false;
    }
    
  }else{
      if(initDecodeCounter!=0){
        initDecode();
      }else{
        decodeIr();
      }
  }*/
  MDNS.update();
  dnsServer.processNextRequest();
  server.handleClient();
  if(millis()-timer1>5000){
    Serial.print("IP STA: ");
    Serial.println(WiFi.localIP());
    Serial.print("IP softAP: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("WiFi.SSID(): ");
    Serial.println(WiFi.SSID());
    Serial.print("wifiManager.getConfigPortalSSID(): ");
    Serial.println(wifiManager.getConfigPortalSSID().c_str());
    Serial.print("Hostname3: ");
    Serial.println(WiFi.hostname());
    timer1=millis();
  }
   
}


void writeMatrizFS(){
  Serial.println("\tGravando matriz no FS");
    int i;
    for (i = 0; i < sizeof(codes.buf); ++i){     
            EEPROM.write(addr+i, codes.buf[i]);
    }     
    Serial.println("\n Gravou na EEPROM.....: ");Serial.println(i);Serial.println(" bytes");
    if (EEPROM.commit()) {
      Serial.println("\tEEPROM successfully committed");
    } else {
      Serial.println("\tERROR! EEPROM commit failed");
    }
}
void setMUXChannel(const byte which){
  digitalWrite(s0, (which & 1) ? HIGH : LOW); 
  digitalWrite(s1, (which & 2) ? HIGH : LOW);
}
void readMUX(float d[]){   
  for (byte i = 0; i < 3; i++){
        setMUXChannel(i);
        Serial.print("Sensor ");Serial.print (i);Serial.print(" reads: ");
        adc = 0;
        for(int x=0;x<n;x++){
            var1 = analogRead(mux);
            for(int i = 0;i<n2;i++){ 
                 var2 = analogRead(mux);
                 if(var2<var1)var1 = var2;
                 delay(1);
            }
            adc += var1;
        }
        adc /= n;
        d[i] = adc;           
        Serial.print (adc);
        Serial.print (" \tTensão: ");
        Serial.println((double) (adc) * 3.3 / 1023);        
    } 
}
void sendIrCode(){

  
}
void calcRevP(){
  
}
float calcLM35(float adc){
  float temperature = adc*3.3/(1024)/0.01;
  Serial.print("Temperatura LM35: ");Serial.println(temperature);
  return temperature;
}
void createFile(void){
  File wFile; 
  if(SPIFFS.exists("/log.txt")){
    Serial.println("Arquivo ja existe!");
  } else {
    Serial.println("Criando o arquivo...");
    wFile = SPIFFS.open("/log.txt","w+"); 
    if(!wFile){
      Serial.println("Erro ao criar arquivo!");
    } else {
      Serial.println("Arquivo criado com sucesso!");
    }
  }
  wFile.close();
}
void writeFile(String msg) {
  int w;
  File rFile = SPIFFS.open("/log.txt","a+");  
  Serial.println();
  if(!rFile){
    Serial.println("Erro ao abrir arquivo!");
  } else {
    w = rFile.write(codes.buf, bufLen);
    rFile.println();
    Serial.print("\t write: ");Serial.print(w);Serial.println(" Bytes ");
  }
  rFile.close();  
}

void readFile1(void) { 
  int w; 
  Serial.print("\t fazendo leirura 1 ..... ");
  File rFile = SPIFFS.open("/log.txt","r"); 
  if(!rFile){
    Serial.println("Erro ao abrir arquivo!");
  } else {
    w = rFile.read(codes.buf, bufLen);     
    Serial.print("\t read: ");Serial.print(w);Serial.println(" Bytes ");
  }   
  rFile.close();
}

void openFS(void){
  if(!SPIFFS.begin()){
    Serial.println("Erro ao abrir o sistema de arquivos");
  } else {
    Serial.println("Sistema de arquivos aberto com sucesso!");
  }
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
