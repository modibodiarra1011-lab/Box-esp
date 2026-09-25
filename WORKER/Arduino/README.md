# WORKER ESP32 — Arduino

Ce firmware est le worker générique utilisé par W1, W2 et W3.

## Installation

Arduino IDE :

- Carte : ESP32 Dev Module
- Port : port USB de la carte
- Moniteur série : 115200 bauds

Ouvrir `worker.ino`, vérifier puis téléverser.

Le même programme est utilisé pour les trois cartes. Le MASTER attribue l'identifiant W1/W2/W3 par le protocole de découverte.

## Fonctions

- découverte UDP
- heartbeat
- état READY/BUSY/TESTING/FLASHING/ERROR/OFFLINE/RECONNECTING
- jobs PING / SYSTEM_TEST
- OTA depuis le MASTER avec SHA-256
- mémoire opérationnelle LittleFS
- reconnexion Wi-Fi
- mise à jour des identifiants AP depuis le MASTER
- journalisation série et vers le MASTER

Le worker ne nécessite pas d'identifiants Internet du propriétaire.
