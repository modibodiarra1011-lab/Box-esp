# 23 - ESP-NOW (communication pair-à-pair)

## But
Faire communiquer deux cartes ESP32 directement, sans routeur WiFi, avec une
latence très faible, via le protocole propriétaire ESP-NOW d'Espressif.

## Matériel
- 2x cartes ESP32 DevKit (une "émetteur", une "récepteur")
- Aucun composant externe

## Broches
Aucun câblage requis : communication radio interne (2.4 GHz).

## Fichiers fournis
- `23_espnow_sender.ino` : à flasher sur la carte émettrice.
- `23_espnow_receiver.ino` : à flasher sur la carte réceptrice.

⚠️ Ce dossier `main/` contient deux sketches car ESP-NOW nécessite deux rôles
distincts sur deux cartes physiques différentes. **Ne compilez qu'un seul
fichier .ino à la fois par carte** (créez un dossier de projet Arduino IDE
séparé pour chacun, ou copiez le fichier voulu seul dans un dossier).

## Procédure de mise en service
1. Flashez `23_espnow_receiver.ino` sur la première carte, ouvrez le moniteur série,
   notez l'adresse MAC affichée.
2. Reportez cette adresse MAC dans le tableau `receiverMac[]` de `23_espnow_sender.ino`.
3. Flashez `23_espnow_sender.ino` sur la seconde carte.
4. Les deux moniteurs série doivent afficher les messages envoyés/reçus en continu.

## Avertissements tension / niveaux logiques
- ESP-NOW est **spécifique aux puces Espressif** (ESP32, ESP8266) : non applicable à un
  Arduino Uno sans module radio compatible.
- Le canal WiFi (`peerInfo.channel = 0` = canal courant) doit être identique entre émetteur et
  récepteur : en cas de comportement instable, fixez explicitement le même canal des deux côtés.
- ESP-NOW peut coexister avec une connexion WiFi classique (mode STA connecté à un routeur),
  mais les deux doivent alors utiliser le même canal radio.
