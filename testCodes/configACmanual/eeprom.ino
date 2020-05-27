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
void readMatrizEEPROM(){
  Serial.println("\n\tLendo matriz na EEPROM");
    int i;
    for (i = 0; i < sizeof(codes.buf); ++i){     
            codes.buf[i] = EEPROM.read(addr+i);
    }     
    Serial.println("\n\t Leu matriz na EEPROM.....: ");Serial.println(i);Serial.println(" bytes");
}
