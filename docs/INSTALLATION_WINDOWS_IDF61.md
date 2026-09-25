# Installation Windows — ESP-IDF 6.1 + VS Code

## 1. Ouvrir l'environnement ESP-IDF

```powershell
cd C:\esp\v6.1\esp-idf
.\export.bat
idf.py --version
```

Le résultat doit indiquer ESP-IDF **v6.1.x**.

## 2. Ouvrir le projet

Dans VS Code :

```text
ESP32_LAB_IDF6.1_PRO/firmware/master
```

Installer l'extension officielle Espressif ESP-IDF si elle ne l'est pas déjà et sélectionner `C:\esp\v6.1\esp-idf` comme installation active.

## 3. Cible

```powershell
idf.py set-target esp32s3
```

`set-target` régénère la configuration de cible et nettoie le build précédent ; vérifiez donc `sdkconfig` après l'appel.

## 4. Configuration

```powershell
idf.py menuconfig
```

Le projet est prévu pour :

```text
Target                 ESP32-S3
Flash                  16 MB
CPU                    240 MHz
PSRAM                  activée si présente/configurée
HTTPD WebSocket        activé
Certificate Bundle     activé
OTA rollback           activé
```

## 5. Build

```powershell
idf.py build
```

## 6. Flash via CH343

Le premier flash du MASTER se fait avec le **port USB-UART/CH343** :

```powershell
idf.py -p COM7 flash
idf.py -p COM7 monitor
```

ou :

```powershell
idf.py -p COM7 flash monitor
```

Le port est volontairement distinct de l'USB OTG réservé au rôle hôte au runtime.

## 7. Première mise sous tension

Le moniteur série doit afficher les informations de première configuration privée si NVS ne contient pas encore de configuration :

```text
FIRST BOOT PRIVATE SETUP: AP SSID=...
FIRST BOOT PRIVATE SETUP: AP PASSWORD=...
FIRST BOOT PRIVATE SETUP: CONTROL PATH=...
FIRST BOOT PRIVATE SETUP: ADMIN PASSWORD=...
```

Conservez ces informations hors du dépôt Git.

## 8. Composants gérés

Le projet demande automatiquement :

```powershell
idf.py add-dependency "espressif/usb_host_cdc_acm^2.4.1"
idf.py add-dependency "espressif/led_strip^3.0.1"
```

## 9. MCP ESP-IDF

```powershell
idf.py mcp-server
```

Cette commande est facultative et destinée à un client MCP compatible.
