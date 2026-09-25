# ESP32 LAB — Portfolio technique

ESP32 LAB est un laboratoire embarqué local basé sur ESP-IDF 6.1 pour ESP32-S3, avec Worker Pool ESP32, WebSocket, gestion microSD, OTA avec rollback, USB Host CDC-ACM, programmation AVR, Project Builder, pipeline Arduino CLI, automatisation Python et CI GitHub.

## Compétences démontrées

- C/C++ embarqué et architecture multi-tâches FreeRTOS
- ESP-IDF / CMake / Component Manager
- Wi-Fi AP+STA, découverte des workers, heartbeat
- Web embarqué mobile-first et WebSocket
- microSD/FATFS/SDSPI
- OTA, SHA-256, rollback
- USB Host CDC-ACM et programmation AVR profilée
- Python CLI, génération de packages et métadonnées
- Git/GitHub Actions/CI
- sécurité des secrets, validation des chemins, journalisation

## Limites connues

Les builds et flashes physiques restent à exécuter dans l’environnement cible. Les profils AVR sont limités aux bootloaders documentés par le projet.
