# WORKER Arduino-ESP32 3.3.12 — build de référence

## Correction de l'erreur signalée

L'erreur remontée par l'IDE était :

```text
'OTA_TIMEOUT_MS' was not declared in this scope
```

Le code du Worker utilisait un ancien nom de constante. Le firmware 5.0.0 utilise désormais :

```cpp
OTA_IDLE_TIMEOUT_MS
OTA_HTTP_TIMEOUT_MS
```

La constante d'inactivité OTA est définie dans `WORKER/Arduino/config.h`.

## Cible identique à l'IDE utilisateur

Le journal fourni utilise Arduino-ESP32 **3.3.12** et la cible `ESP32_DEV` avec 4 MB de flash, CPU 240 MHz et PSRAM désactivée. La CI Arduino CLI reproduit ce FQBN pour le Worker.

Arduino-ESP32 3.3.12 est publié par Espressif avec ESP-IDF 5.5.5. Le dépôt contient aussi une seconde compilation indépendante via PlatformIO/pioarduino afin de détecter les divergences de toolchain.

## Vérification en ligne

Le dépôt contient trois chemins de compilation dans GitHub Actions :

1. `build-master` : ESP-IDF 6.1 pour le MASTER ESP32-S3.
2. `build-worker-arduino-cli` : Arduino CLI + Arduino-ESP32 3.3.12, au plus proche de l'Arduino IDE.
3. `build-worker-platformio` : PlatformIO/pioarduino avec Arduino-ESP32 3.3.12 et ESP-IDF 5.5.5.

Wokwi est une voie de simulation supplémentaire, mais son CLI/CI nécessite un jeton Wokwi fourni par l'utilisateur.

## Build local Arduino CLI

```powershell
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12
arduino-cli compile `
  --fqbn "esp32:esp32:esp32:UploadSpeed=921600,CPUFreq=240,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,DebugLevel=none,PSRAM=disabled,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default" `
  --warnings all `
  WORKER\Arduino
```

## Vérification statique

```powershell
python scripts/verify_worker_arduino.py
```

La vérification finale reste la compilation réelle sur la machine Windows et, ensuite, le flash sur chaque carte.
