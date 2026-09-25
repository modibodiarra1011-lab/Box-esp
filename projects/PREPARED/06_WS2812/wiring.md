# Wiring - WS2812 / NeoPixel

Example 8-pixel module -> ESP32 Dev Module:

- DIN -> GPIO5
- VCC -> external suitable 5V supply for the strip/module, according to the exact product
- GND -> common GND between ESP32 and LED supply

Never use an ESP32 GPIO to power the LED strip. For longer strips, size the power supply for the pixel count and current.

The DATA pin in this example can be changed if GPIO5 is not suitable on your board.
