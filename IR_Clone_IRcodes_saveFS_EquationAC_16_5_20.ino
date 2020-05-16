#include <Arduino.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>
#include <IRsend.h>
#include <EEPROM.h>
#include "FS.h"

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

void setup() {
    irsend.begin();

  Serial.begin(kBaudRate, SERIAL_8N1, SERIAL_TX_ONLY);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.printf("\n" D_STR_IRRECVDUMP_STARTUP "\n", kRecvPin);
#if DECODE_HASH
  irrecv.setUnknownThreshold(kMinUnknownSize);
#endif  // DECODE_HASH
  irrecv.enableIRIn();  // Start the receiver
  //SPIFFS.format();
  openFS();
  createFile();
  EEPROM.begin(4);//(512);
}

void loop() {
  if(flag){
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
  }
   
}
void testSend(){
  Serial.println(" \n\t Iniciando testeSendIR........ ");
   for(int i=0; i < NLIN; i++) {
          Serial.print(" \n\t Enviando linha: ");Serial.println(i);
          delay(1000);
         irsend.sendRaw(codes.matriz[i], NCOL, 38);
         delay(4000);        
   } 
  Serial.println(" \n\t testeSendIR finalizado!..Desligando Ar condicionado...: ");
  delay(2000);
  irsend.sendRaw(codes.matriz[0], NCOL, 38);
}
void readMatrizEEPROM(){
  Serial.println("\n\tLendo matriz na EEPROM");
    int i;
    for (i = 0; i < sizeof(codes.buf); ++i){     
            codes.buf[i] = EEPROM.read(addr+i);
    }     
    Serial.println("\n\t Leu matriz na EEPROM.....: ");Serial.println(i);Serial.println(" bytes");
}
void resultDecode(){
     Serial.print(resultToHumanReadableBasic(&results));
    String description = IRAcUtils::resultAcToString(&results);
    if (description.length()) Serial.println(D_STR_MESGDESC ": " + description);
    yield();  // Feed the WDT as the text output can take a while to print.

    count = results.rawlen;
    Serial.print("Raw (");Serial.print(count, DEC);Serial.print(")\n");
    Serial.println(resultToSourceCode(&results));
    
    Serial.println();    // Blank line between entries
    yield();             // Feed the WDT (again)

    for (uint16_t i = 1; i < count; i++) {
    if (i % 100 == 0)
      yield();  // Preemptive yield every 100th entry to feed the WDT.
    if (i & 1) {
      Serial.print(", ");
      Serial.print(results.rawbuf[i] * kRawTick, DEC);
    } else {
      Serial.print(", ");
      Serial.print((uint32_t) results.rawbuf[i] * kRawTick, DEC);
    }
  }
  Serial.println("};");
        yield();  // Preemptive yield every 100th entry to feed the WDT.
}

void printMatriz(uint16_t m[][NCOL]){
  Serial.println();
   for(int i=0; i < NLIN; i++) {
         Serial.print(" \nindexMatriz: ");Serial.print(i);Serial.print("\t");
         for(int j=0; j < NCOL; j++){
             //Serial.print(readcodes.matriz[i][j]);Serial.print(", ");
             Serial.print(m[i][j]);Serial.print(", ");
         }        
   }      
}
void initDecode(){  
  if (irrecv.decode(&results)) {
    Serial.println("  initDecode()");
    if(initDecodeCounter==2){
      ir_type = results.decode_type;
      rawSize = results.rawlen - 1;
      initDecodeCounter--;
    }else{
      if(ir_type == results.decode_type & (rawSize == results.rawlen - 1)){
        initDecodeCounter--;
        if (results.decode_type > 0) {
          Serial.println("AC configurado!");
          flagAC = true;      
        }
      }else{
        Serial.println("RawSize ou decode_type inconsistente!!! Repita a operação.....................");
        initDecodeCounter=2;
      }
    }    
    
    if (results.decode_type == UNKNOWN) {
      Serial.println("2.....................");      
    } 
    Serial.print("initDecodeCounter:");
    Serial.println(initDecodeCounter); 
    resultDecode(); 
  }
}
void decodeIr(){
  if (irrecv.decode(&results)) {
    Serial.println("  decodeIr()");
    if (results.overflow)
      Serial.printf(D_WARN_BUFFERFULL "\n", kCaptureBufferSize);
    Serial.print(resultToHumanReadableBasic(&results));
    String description = IRAcUtils::resultAcToString(&results);
    if (description.length()) Serial.println(D_STR_MESGDESC ": " + description);
    yield();  // Feed the WDT as the text output can take a while to print.

    count = results.rawlen;
    Serial.print("Raw (");Serial.print(count, DEC);Serial.print(")\n");
    Serial.println(resultToSourceCode(&results));
    
    Serial.println();    // Blank line between entries
    yield();             // Feed the WDT (again)

    if(indexMatriz < 13){
        if (results.rawlen-1 == rawSize){
                for (int i = 0; i < 243; ++i){      
                   codes.matriz[indexMatriz][i] = results.rawbuf[i+1] * kRawTick;
                }
                Serial.print("indexMatriz: ");Serial.println(indexMatriz);
                indexMatriz++;
                if(indexMatriz==13){
                  writeFile("Web Server started");
                  //writeMatrizEEPROM();
                  flag = true;
                }
        }else{
                Serial.println("RawSize inconsistente!!!.....................");
        }
    }else{
      Serial.println("\tMatriz de códigos preenchida!!!.....................");
      //printMatriz();
    }
        yield();  // Preemptive yield every 100th entry to feed the WDT.  
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
void readMux(){
  
}
void sendIrCode(){

  
}
void calcRevP(){
  
}
void calcLM35(){
  
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
