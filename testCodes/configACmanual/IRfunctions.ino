void initDecode(){
  Serial.println("Entrou initDecode!");  
  mstate = DECODE;
  
          DynamicJsonDocument jsonBuffer(1024);
          JsonObject msg = jsonBuffer.to<JsonObject>();
          msg["msg"] = "Aguardando envio dos códigos...";          
          msg["passos"] = passos[countSequencia];
          msg["msgPassos"]=msgPassos[countSequencia];
          msg["mstate"]=mstate;
          msg["indexMatriz"]=indexMatriz;
          msg["countSequencia"]=countSequencia;
          String json;
          serializeJson(msg, json);
          server.send(200, "application/json", json);
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
    for (int i = 0; i < 243; ++i){      
                   rawCode[i] = results.rawbuf[i+1] * kRawTick;
    } 
    f_hasCode = true;  
    Serial.println();    // Blank line between entries
    yield();             // Feed the WDT (again)  
  }
}
void gravar(){
  DynamicJsonDocument jsonBuffer(1024);
  JsonObject msg = jsonBuffer.to<JsonObject>();
  
        if (f_hasCode){
                for (int i = 0; i < 243; ++i){      
                   codes.matriz[sequencia[countSequencia]][i] = rawCode[i];
                }
                Serial.print("countSequencia: ");Serial.println(countSequencia);
                Serial.print("sequencia[countSequencia]: ");Serial.println(sequencia[countSequencia]);
                countSequencia++;
                f_hasCode=false;
                if(countSequencia==13){
                  writeFile("Web Server started");
                  printMatriz(codes.matriz);
                  countSequencia = 0;
                  mstate = IDL;
                  flag = true;
                  msg["msg"] = "Ar condicionado configurado com sucesso!";          
                  msg["passos"] = passos[countSequencia];
                  msg["msgPassos"]=msgPassos[countSequencia];
                }else{
                  msg["msg"] = msgPassos[countSequencia];          
                  msg["passos"] = passos[countSequencia];
                  msg["msgPassos"]=msgPassos[countSequencia];
                }
                
                
                msg["mstate"]=mstate;
                msg["indexMatriz"]=indexMatriz;
                msg["countSequencia"]=countSequencia;
                msg["alerta"]=0;                
        }else{
                Serial.println("Configure o controle!!!.....................");
                msg["msg"] = msgPassos[countSequencia];          
                msg["passos"] = passos[countSequencia];
                msg["msgPassos"]=msgPassos[countSequencia];
                msg["mstate"]=mstate;
                msg["indexMatriz"]=indexMatriz;
                msg["countSequencia"]=countSequencia;
                msg["alerta"]=1;
                msg["msgAlerta"]="  Código não detectado!";
        }
        String json;
        serializeJson(msg, json);
        server.send(200, "application/json", json);
  
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
void sendIrCode(){

  
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
             Serial.print(m[i][j]);Serial.print(", ");
         }        
   }      
}
void decodeIrOrigin(){
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

void initDecodeOrigin(){
  //Serial.println("Entrou initDecode!");
  mstate = DECODE;
  if(initDecodeCounter==3){
          Serial.println("initDecodeCounter==3!");
          DynamicJsonDocument jsonBuffer(1024);
          JsonObject msg = jsonBuffer.to<JsonObject>();
          msg["msg"] = "Aguardando envio dos códigos...";
          msg["mstate"]=mstate;
          String json;
          serializeJson(msg, json);
          server.send(200, "application/json", json);
          initDecodeCounter--;
        }  
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
          f_AC = true;      
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
