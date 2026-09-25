# Wiring - MPU6050 I2C

MPU6050 module -> ESP32 Dev Module

- VCC -> module-compatible 3.3V supply (verify module)
- GND -> GND
- SDA -> GPIO21
- SCL -> GPIO22

The example reads the accelerometer and gyro registers directly, so no external MPU6050 library is required.
