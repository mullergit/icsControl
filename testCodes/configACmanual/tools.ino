/** Is this an IP? */
boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}
void printStatus(){
  if(millis()-timer1>10000){
    Serial.print("WiFi.SSID(): ");Serial.print(WiFi.SSID());Serial.print("IP STA: ");Serial.println(WiFi.localIP());
    Serial.print("IP softAP: ");Serial.println(WiFi.softAPIP());
    
    Serial.print("wifiManager.getConfigPortalSSID(): ");Serial.println(wifiManager.getConfigPortalSSID().c_str());
    Serial.print("Hostname3: ");Serial.println(WiFi.hostname());
    
    Serial.print("mstate: ");Serial.println(mstate);
    if(mstate==1){Serial.print("initDecodeCounter: ");Serial.println(initDecodeCounter);}
    timer1=millis();
  }
}
void tick(){
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode: configModeCallback()");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  ticker.attach(0.2, tick);
}
String getTemperature() {
  float temperature = 35;
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
