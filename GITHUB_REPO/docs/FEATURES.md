# ESP32 LAB — fonctionnalités v5.0.0

## Noyau MASTER

- ESP32-S3 N16R8, orchestration locale et fonctionnement offline-first.
- Worker Pool dynamique avec découverte UDP, attribution automatique W1/W2/W3 et heartbeat.
- Scheduler parallèle : plusieurs jobs peuvent progresser en même temps sur des workers différents.
- Priorités, retries, timeout, annulation individuelle et **STOP ALL**.
- Jobs : `PING`, `SYSTEM_TEST`, `CHECKUP`, `BENCHMARK`, `FS_TEST`.
- Reboot individuel d'un worker depuis l'interface administrateur.
- Télémetrie temps réel : CPU, cœurs, RAM libre/min, PSRAM, flash, RSSI, uptime, progression.
- Logs multi-worker conservés sur SD.

## Worker v5.0.0

- Arduino-ESP32 3.3.12, protocole worker v2.
- Correction définitive de l'ancien symbole `OTA_TIMEOUT_MS` : remplacement par `OTA_IDLE_TIMEOUT_MS` + `OTA_HTTP_TIMEOUT_MS`.
- Jobs asynchrones et annulables depuis l'API.
- Benchmark CPU avec mesure du débit d'opérations.
- Self-test LittleFS écriture/lecture/vérification sans formatage automatique pendant le test.
- Diagnostics CPU/flash/Wi-Fi/LittleFS.
- Télémétrie avancée : heap, minimum de heap, allocation maximale, PSRAM, flash, sketch, stack libre, SDK et version Arduino.
- Compteur de boots et motif de reset (`PANIC`, watchdog, brownout, etc.).
- mDNS `esp32-lab-w<ID>.local` + service HTTP.
- **Radar Wi-Fi asynchrone** avec SSID, RSSI, canal et type de sécurité.
- Dashboard autonome local mobile-first.
- OTA HTTP avec limites de taille, timeout d'inactivité, SHA-256, vérification avant redémarrage.

## Interface FUTURE CONTROL

- Mode sombre / clair.
- Orbite animée et micro-animations.
- Gauges circulaires par worker.
- KPI master et flotte.
- Historique graphique RAM dans le navigateur.
- Console live et fallback polling lorsque WebSocket n'est pas disponible.
- Command palette `Ctrl/Cmd + K`.
- Presets : `TURBO ALL`, `BENCH FLEET`, `STRESS ×9`, `MISSION`.
- Interface mobile responsive et prise en charge de `prefers-reduced-motion`.

## Plateformes de compilation croisées

### Chemin A — le plus proche de l'IDE utilisateur
Arduino CLI installe **Arduino-ESP32 3.3.12** puis compile `WORKER/Arduino` avec le FQBN exact du journal fourni.

### Chemin B — vérification croisée
PlatformIO/pioarduino compile le même Worker avec Arduino-ESP32 **3.3.12** et ESP-IDF **5.5.5**.

### Chemin C — MASTER
GitHub Actions installe ESP-IDF **6.1** et construit le MASTER ESP32-S3.

## Ce qui n'est pas prétendu comme déjà validé

- Aucune compilation matérielle n'est déclarée comme réussie tant qu'elle n'a pas été exécutée dans une vraie chaîne Arduino/ESP-IDF.
- Aucun flash physique n'est considéré comme validé par une vérification statique.
- Wokwi/CI nécessite un compte ou un secret de service lorsqu'un token est demandé.
