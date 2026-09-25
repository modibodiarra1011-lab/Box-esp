# Wiring - OLED SSD1306 128x64 I2C

Typical 4-pin module -> ESP32 Dev Module:

- VCC -> module-compatible supply (often 3.3V, verify yours)
- GND -> GND
- SDA -> GPIO21
- SCL -> GPIO22

Default example address is 0x3C. Some modules use 0x3D; confirm with the I2C scanner.
