# Recherche technique — références consultées le 25 septembre 2026

## Arduino-ESP32

- Releases Espressif Arduino core : https://github.com/espressif/arduino-esp32/releases
- Arduino-ESP32 3.3.12 est publié comme release basée sur ESP-IDF 5.5.5.
- API HTTPClient : https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.h
- API ESPmDNS : https://github.com/espressif/arduino-esp32/blob/master/libraries/ESPmDNS/src/ESPmDNS.cpp
- Exemple mDNS HTTP : https://github.com/espressif/arduino-esp32/blob/master/libraries/ESPmDNS/examples/mDNS_Web_Server/mDNS_Web_Server.ino
- Wi-Fi scan asynchrone : https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiScanAsync/WiFiScanAsync.ino
- API Wi-Fi scan : https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html

## Arduino CLI / GitHub Actions

- Arduino CLI : https://github.com/arduino/arduino-cli
- Action officielle setup Arduino CLI : https://github.com/arduino/setup-arduino-cli
- Espressif Arduino package index : https://espressif.github.io/arduino-esp32/package_esp32_index.json

## PlatformIO / pioarduino

- PlatformIO ESP32 platform : https://docs.platformio.org/en/latest/platforms/espressif32.html
- pioarduino / PlatformIO ESP32 fork : https://github.com/pioarduino/platform-espressif32
- Référence d'archive utilisée pour Arduino-ESP32 3.3.12 : `esp32-core-3.3.12.tar.xz`
- Référence d'archive utilisée pour ESP-IDF 5.5.5 : `esp-idf-v5.5.5.tar.xz`

## Simulation en ligne

- Wokwi : https://wokwi.com/
- Wokwi CI / CLI : https://github.com/wokwi/wokwi-ci

## Conclusion de recherche

Pour ce projet précis, la vérification la plus pertinente est le double build : Arduino CLI avec le core exact 3.3.12 pour reproduire le journal utilisateur, puis PlatformIO/pioarduino comme second chemin indépendant. Le MASTER reste séparé avec ESP-IDF 6.1.
