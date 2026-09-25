# 24 - Diagnostic système (Worker Diagnostic)

## But
Fournir un rapport de santé matérielle/logicielle périodique (mémoire, CPU,
uptime, MAC, température interne) et démontrer l'usage d'un watchdog logiciel
qui redémarre automatiquement la carte si la boucle principale se bloque.

## Matériel
- 1x carte ESP32 DevKit
- Aucun composant externe requis

## Broches
Aucun câblage nécessaire (diagnostic 100% logiciel/interne au SoC).

## Avertissements tension / niveaux logiques
- **Spécifique ESP32** : les fonctions de diagnostic interne (`ESP.getChipModel()`, capteur de
  température intégré, watchdog logiciel `esp_task_wdt`) n'existent pas sur un Arduino Uno
  classique (AVR).
- Le capteur de température interne du SoC (lu via `temperatureRead()`, fonction native du core
  Arduino-ESP32) mesure la température de la puce elle-même (souvent 5-10°C au-dessus de
  l'ambiante à cause de l'auto-échauffement), **pas** une mesure ambiante fiable — pour cela,
  utilisez plutôt le DHT22, BME280 ou DS18B20 de ce laboratoire.
- **Compatibilité de core** : l'API du Task Watchdog a changé de signature entre le core
  Arduino-ESP32 v2.x et v3.x (ce dernier étant basé sur ESP-IDF 5.x). Ce code cible le core v3.x
  (`esp_task_wdt_config_t` + `esp_task_wdt_init(&config)`), qui est la version actuellement
  proposée par défaut dans le Board Manager Arduino IDE. Si votre installation utilise encore un
  core v2.x, remplacez l'appel par `esp_task_wdt_init(10, true); esp_task_wdt_add(NULL);` (sans
  structure de configuration). L'appel `esp_task_wdt_deinit()` avant l'initialisation évite une
  erreur si un watchdog par défaut est déjà actif.
- Le watchdog logiciel ici est configuré pour un timeout de 10 secondes : si vous ajoutez du
  code bloquant (ex: `delay()` long ou boucle `while` infinie) dans `loop()` sans jamais rappeler
  `esp_task_wdt_reset()`, la carte redémarrera automatiquement — comportement volontaire et
  attendu pour la robustesse en production.
