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
