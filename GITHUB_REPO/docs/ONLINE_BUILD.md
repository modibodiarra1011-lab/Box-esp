# Build en ligne — chaîne de vérification croisée

## Pourquoi plusieurs compilateurs ?

Le journal utilisateur provenait d'Arduino IDE + Arduino-ESP32 3.3.12. Un build ESP-IDF du MASTER ne suffit donc pas à valider le Worker : les deux programmes n'utilisent pas la même couche logicielle.

Cette archive prépare trois jobs indépendants dans `.github/workflows/ci.yml` :

| Job | Outil | Cible | Version |
|---|---|---|---|
| `build-master` | ESP-IDF | ESP32-S3 MASTER | 6.1 |
| `build-worker-arduino-cli` | Arduino CLI | ESP32 Dev Module WORKER | Arduino-ESP32 3.3.12 |
| `build-worker-platformio` | PlatformIO/pioarduino | ESP32 Dev Module WORKER | Arduino-ESP32 3.3.12 + IDF 5.5.5 |

## Ce que peut faire GitHub Actions

Après publication du contenu de `GITHUB_REPO/` sur un dépôt GitHub, un push ou une Pull Request déclenche les compilations. Les binaires produits sont conservés comme artefacts de workflow.

Le workflow est volontairement reproductible : les versions du MASTER et du Worker sont épinglées au lieu de suivre silencieusement une version future.

## Pourquoi PlatformIO/pioarduino ?

La documentation PlatformIO décrit l'utilisation d'un package Arduino ESP32 depuis GitHub. Les configurations pioarduino actuellement utilisées dans des projets ESP32 épinglent explicitement `framework-arduinoespressif32` sur l'archive 3.3.12 et le framework ESP-IDF 5.5.5. Cette seconde chaîne sert de contrôle croisé ; elle ne remplace pas le build Arduino CLI qui reste la référence la plus proche de l'IDE du journal utilisateur.

## Wokwi

Wokwi peut fournir une simulation et un workflow CI supplémentaire. Sa chaîne CLI/CI demande cependant un jeton Wokwi. Aucun jeton n'est intégré à cette archive et aucun build Wokwi n'est présenté comme exécuté.

## Limitation de cette session

L'environnement d'analyse actuel n'a ni le toolchain Arduino-ESP32/ESP-IDF installé localement ni un dépôt GitHub authentifié permettant de déclencher un workflow distant. Les workflows sont donc préparés et auditablement définis, mais leur exécution cloud doit être faite sur le dépôt réel.
