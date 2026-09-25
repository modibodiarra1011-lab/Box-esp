# ESP32 LAB FUTURE CONTROL — build/package report

Generated: 2026-09-25

## Cible

- MASTER : ESP32-S3 N16R8, ESP-IDF 6.1
- WORKERS : ESP32 Dev Module, Arduino-ESP32 3.3.12 / ESP-IDF 5.5.5
- Worker FQBN checked against the user's Arduino IDE log: `ESP32_DEV`, 4 MB flash, 240 MHz, PSRAM disabled.

## Corrections incluses

- Correction du symbole `OTA_TIMEOUT_MS` vers `OTA_IDLE_TIMEOUT_MS` + `OTA_HTTP_TIMEOUT_MS`.
- Mirror `GITHUB_REPO/` resynchronisé avec le Worker corrigé.
- Jobs Worker asynchrones et annulables.
- Benchmark CPU, FS self-test, reset reason, stack telemetry, LittleFS statistics.
- Radar Wi-Fi asynchrone.
- mDNS Worker.
- OTA Worker avec validation SHA-256 et limites de taille/timeout.
- MASTER : `STOP ALL`, `BENCH FLEET`, `STRESS ×9`, reboot Worker, gauges et télémetrie étendue.

## Vérifications réellement exécutées dans cette session

- `verify_project.py` : PASS
- `check_idf61_compat.py` : PASS
- `ci_static.py` : PASS
- `verify_worker_arduino.py` : PASS
- `verify_100k.py` : PASS — 100000 invariants déterministes
- `verify_release_60x.py` : PASS — 60 passes déterministes
- `range_analyzer.py` : PASS — 41 projets analysés
- `python -m compileall` : PASS
- Node `--check` du JavaScript embarqué MASTER/Worker : PASS
- Syntaxe C++ Worker via harness de stubs Arduino : PASS
- aucun dossier vide : PASS
- absence du symbole actif `OTA_TIMEOUT_MS` dans le Worker : PASS
- `GITHUB_REPO` synchronisé : PASS

## Build cloud préparé mais non exécuté ici

`.github/workflows/ci.yml` contient trois chemins de compilation :

1. MASTER avec ESP-IDF 6.1.
2. WORKER avec Arduino CLI + Arduino-ESP32 3.3.12.
3. WORKER avec PlatformIO/pioarduino + Arduino-ESP32 3.3.12 + ESP-IDF 5.5.5.

L'environnement de cette session n'a pas de toolchain Arduino/ESP-IDF local et n'est pas authentifié sur le dépôt GitHub réel ; aucun run cloud n'est donc présenté comme déjà exécuté.

## Non validé matériellement

- flash réel sur le MASTER
- flash réel des trois WORKERS
- comportement électrique SD/USB
- enumeration USB AVR sur le matériel concret
- OTA sur un réseau réel
