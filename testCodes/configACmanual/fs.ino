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
