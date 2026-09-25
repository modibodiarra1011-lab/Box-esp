#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

const uint8_t DATA_PIN=5;
const uint16_t NUM_PIXELS=8;
Adafruit_NeoPixel pixels(NUM_PIXELS,DATA_PIN,NEO_GRB+NEO_KHZ800);

void setAll(uint32_t color){for(uint16_t i=0;i<NUM_PIXELS;i++)pixels.setPixelColor(i,color);pixels.show();}

void setup(){
  Serial.begin(115200);
  pixels.begin(); pixels.clear(); pixels.show();
  Serial.println("WS2812 ready");
}

void loop(){
  setAll(pixels.Color(255,0,0)); delay(500);
  setAll(pixels.Color(0,255,0)); delay(500);
  setAll(pixels.Color(0,0,255)); delay(500);
  setAll(pixels.Color(0,0,0)); delay(500);
}
