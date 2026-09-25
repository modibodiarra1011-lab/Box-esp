# Wiring - DHT11

DHT11 module / sensor -> ESP32 Dev Module

- VCC -> 3V3
- DATA -> GPIO4
- GND -> GND

A 3-pin DHT11 module often already includes the pull-up resistor. A bare 4-pin sensor normally needs an external pull-up on DATA; verify the module you own before adding parts.

## Test
Open Serial Monitor at 115200. A valid read prints temperature and humidity every 2.5 s.
