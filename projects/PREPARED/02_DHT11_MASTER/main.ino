#include <Arduino.h>

const uint8_t DHT_PIN = 4;

bool waitLevel(int level, uint32_t timeout_us){
  uint32_t start=micros();
  while(digitalRead(DHT_PIN)!=level){
    if(micros()-start>timeout_us) return false;
  }
  return true;
}

bool readDHT11(int &humidity, int &temperature){
  uint8_t data[5]={0,0,0,0,0};
  pinMode(DHT_PIN,OUTPUT); digitalWrite(DHT_PIN,LOW); delay(20);
  pinMode(DHT_PIN,INPUT_PULLUP); delayMicroseconds(40);
  if(!waitLevel(LOW,200)) return false;
  if(!waitLevel(HIGH,200)) return false;
  if(!waitLevel(LOW,200)) return false;
  noInterrupts();
  for(int i=0;i<40;i++){
    if(!waitLevel(HIGH,100)) { interrupts(); return false; }
    uint32_t t=micros();
    if(!waitLevel(LOW,120)) { interrupts(); return false; }
    uint32_t high_us=micros()-t;
    data[i/8] <<= 1;
    if(high_us>45) data[i/8] |= 1;
  }
  interrupts();
  if(uint8_t(data[0]+data[1]+data[2]+data[3])!=data[4]) return false;
  humidity=data[0];
  temperature=data[2];
  return true;
}

void setup(){
  Serial.begin(115200);
  Serial.println("ESP32 LAB - DHT11 test");
}

void loop(){
  int h=0,t=0;
  if(readDHT11(h,t)){
    Serial.printf("Temperature: %d C | Humidity: %d %%\n",t,h);
  }else{
    Serial.println("DHT11 read failed - check VCC/DATA/GND and pull-up");
  }
  delay(2500);
}
