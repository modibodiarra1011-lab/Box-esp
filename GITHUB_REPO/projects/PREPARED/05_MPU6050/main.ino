#include <Arduino.h>
#include <Wire.h>

const int SDA_PIN=21;
const int SCL_PIN=22;
const uint8_t MPU_ADDR=0x68;

void writeReg(uint8_t reg,uint8_t value){
  Wire.beginTransmission(MPU_ADDR); Wire.write(reg); Wire.write(value); Wire.endTransmission();
}

bool readBytes(uint8_t reg,uint8_t *buf,size_t n){
  Wire.beginTransmission(MPU_ADDR); Wire.write(reg);
  if(Wire.endTransmission(false)!=0) return false;
  if(Wire.requestFrom((int)MPU_ADDR,(int)n)!=n) return false;
  for(size_t i=0;i<n;i++) buf[i]=Wire.read();
  return true;
}

void setup(){
  Serial.begin(115200); Wire.begin(SDA_PIN,SCL_PIN);
  writeReg(0x6B,0x00); delay(100);
  uint8_t who=0;
  if(readBytes(0x75,&who,1)) Serial.printf("WHO_AM_I: 0x%02X\n",who); else Serial.println("MPU6050 not found");
}

void loop(){
  uint8_t b[14];
  if(!readBytes(0x3B,b,sizeof(b))){Serial.println("Read failed");delay(1000);return;}
  int16_t ax=(b[0]<<8)|b[1], ay=(b[2]<<8)|b[3], az=(b[4]<<8)|b[5];
  int16_t gx=(b[8]<<8)|b[9], gy=(b[10]<<8)|b[11], gz=(b[12]<<8)|b[13];
  Serial.printf("A[g]: %.2f %.2f %.2f | G[dps]: %.2f %.2f %.2f\n",ax/16384.0,ay/16384.0,az/16384.0,gx/131.0,gy/131.0,gz/131.0);
  delay(500);
}
