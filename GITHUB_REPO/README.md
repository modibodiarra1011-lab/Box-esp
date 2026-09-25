# ESP32 LAB — FUTURE CONTROL v5.0.0

ESP32 LAB est une base de laboratoire embarqué locale et offline-first pour un **ESP32-S3 N16R8 comme MASTER** et trois **ESP32 Dev Module comme WORKERS**.

Le MASTER utilise **ESP-IDF 6.1**. Le Worker Arduino est aligné sur le même environnement que le journal de compilation fourni : **Arduino-ESP32 3.3.12**, cible `ESP32_DEV`, flash 4 MB, CPU 240 MHz, PSRAM désactivée. Arduino-ESP32 3.3.12 est basé sur ESP-IDF 5.5.5.

## Ce qui a changé en v5.0.0

Le défaut qui bloquait le Worker a été identifié précisément : `OTA_TIMEOUT_MS` était encore référencé alors que la configuration ne définissait plus ce nom. Il est remplacé par `OTA_IDLE_TIMEOUT_MS`, avec `OTA_HTTP_TIMEOUT_MS` pour la couche HTTP.

Le Worker a aussi été transformé en nœud de diagnostic : jobs asynchrones et annulables, benchmark CPU, test LittleFS, radar Wi-Fi, télémétrie mémoire/stack/flash, compteur de boots, reset reason, mDNS et OTA renforcée.

Le MASTER ajoute l'orchestration parallèle, `STOP ALL`, reboot d'un worker, `BENCH FLEET`, `STRESS ×9`, palette de commandes, visualisations et télémetrie étendue.

## Arborescence

```text
ESP32_LAB_FUTURE_5.0.0/
├── firmware/
│   ├── master/              # ESP-IDF 6.1 — ESP32-S3 MASTER
│   └── worker/              # copie synchronisée du Worker Arduino
├── WORKER/Arduino/          # source Worker à ouvrir dans Arduino IDE
├── projects/                # projets Arduino préparés/importés
├── catalog/                 # composants + cartes + recettes
├── SD_CARD/                 # arborescence à copier sur la microSD
├── scripts/                 # build / analyse / vérification / release
├── docs/                    # documentation française
├── PROJECT_BUILDER/
├── .github/workflows/       # CI croisée MASTER + Worker
└── GITHUB_REPO/             # miroir prêt à publier
```

## MASTER sous Windows / ESP-IDF 6.1

```powershell
cd C:\esp\v6.1\esp-idf
.\export.ps1
cd C:\chemin\ESP32_LAB_FUTURE_5.0.0\firmware\master
idf.py set-target esp32s3
idf.py reconfigure
idf.py -j 1 build
```

Flash initial :

```powershell
idf.py -p COM7 flash monitor
```

## Worker — Arduino IDE 3.3.12

Ouvre :

```text
WORKER\Arduino\worker.ino
```

Paramètres correspondant au journal fourni :

```text
Carte            : ESP32 Dev Module
CPU              : 240 MHz
Flash            : 4 MB
Partition        : default
PSRAM            : Disabled
Upload speed     : 921600
USB CDC on boot  : Disabled
Moniteur série   : 115200
```

Le build de référence exact est aussi décrit dans `docs/WORKER_ARDUINO_3.3.12.md`.

## Vérification Worker

```powershell
python scripts\verify_worker_arduino.py
```

Ce contrôle vérifie notamment que le symbole obsolète `OTA_TIMEOUT_MS` n'existe plus, que les macros OTA obligatoires sont présentes et que les copies `WORKER/Arduino` et `firmware/worker/Arduino` sont identiques.

## CI en ligne

`.github/workflows/ci.yml` contient trois builds :

- MASTER : ESP-IDF 6.1.
- Worker : Arduino CLI + Arduino-ESP32 3.3.12, au plus proche d'Arduino IDE.
- Worker : PlatformIO/pioarduino avec Arduino-ESP32 3.3.12 + ESP-IDF 5.5.5.

GitHub Actions est donc la plateforme de build cloud prévue pour le dépôt. Le lancement réel d'un workflow exige un dépôt GitHub connecté/accessible ; aucun run cloud n'est déclaré ici comme déjà exécuté depuis cet environnement.

## GitHub

Le dossier `GITHUB_REPO/` est une copie publiable.

```powershell
cd GITHUB_REPO
git init
git branch -M main
git add .
git commit -m "feat: ESP32 LAB FUTURE CONTROL v5.0.0"
git remote add origin https://github.com/<COMPTE>/<DEPOT>.git
git push -u origin main
```

Une fois le dépôt poussé, l'onglet **Actions** exécutera les trois chaînes de compilation. L'intégration GitHub dédiée permet également de suivre la CI et les changements du dépôt.

Ne committe jamais de vrai mot de passe Wi-Fi, clé IA, clé WhatsApp, dump NVS ou secret de service.

## Limites honnêtement suivies

- Aucun binaire n'est présenté comme matériellement validé sans compilation réelle.
- Aucune réussite de flash physique n'est déduite des contrôles statiques.
- USB Host AVR reste dépendant du périphérique et du profil série réellement rencontrés.
