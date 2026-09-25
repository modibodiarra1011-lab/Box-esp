# Pipeline Arduino CLI → SD / Worker

## Worker — compilation de référence

Le Worker de ce dépôt cible **Arduino-ESP32 3.3.12**.

```powershell
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12
arduino-cli compile `
  --fqbn "esp32:esp32:esp32:UploadSpeed=921600,CPUFreq=240,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,DebugLevel=none,PSRAM=disabled,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default" `
  --warnings all `
  WORKER\Arduino
```

La CI exécute exactement cette chaîne dans `build-worker-arduino-cli`.

## Projets `.ino`

```powershell
python scripts\build_ino.py projects\IMPORTED_35 --output BUILD_OUTPUT
```

## Préparation SD

```powershell
python scripts\prepare_sd.py --build BUILD_OUTPUT --sd SD_READY --projects projects
```

## Vérification Worker

```powershell
python scripts\verify_worker_arduino.py
```

Cette vérification rejette l'ancien symbole `OTA_TIMEOUT_MS`, vérifie les constantes OTA, les routes API principales, les fonctionnalités annoncées et l'identité exacte des fichiers Worker dupliqués.
