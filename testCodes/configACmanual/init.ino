void initsetup(){
  irsend.begin();
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(ledPin, OUTPUT);
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
  WiFi.hostname(myHostname);
  
  wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  wifiManager.setAPCallback(configModeCallback);
  
  if (!wifiManager.autoConnect(softAP_ssid, softAP_password)) {
    Serial.println("failed to connect and hit timeout");
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
}
