#include <Arduino.h>
#include <Wire.h>

const int SDA_PIN=21;
const int SCL_PIN=22;

void setup(){
  Serial.begin(115200);
  Wire.begin(SDA_PIN,SCL_PIN);
  Serial.println("ESP32 LAB - I2C scanner");
}

void loop(){
  uint8_t found=0;
  for(uint8_t address=1;address<127;address++){
    Wire.beginTransmission(address);
    uint8_t err=Wire.endTransmission();
    if(err==0){
      Serial.printf("I2C device found at 0x%02X\n",address);
      found++;
    }
  }
  if(!found) Serial.println("No I2C device found");
  else Serial.printf("Devices found: %u\n",found);
  delay(2000);
}
