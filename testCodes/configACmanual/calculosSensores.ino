void calcRevP(){
  
}
float calcLM35(float adc){
  float temperature = adc*3.3/(1024)/0.01;
  Serial.print("Temperatura LM35: ");Serial.println(temperature);
  return temperature;
}
