# 32 - Domotique : contrôle d'accès RFID + serrure servo + dashboard WiFi

## But
Système d'accès complet : un badge RFID autorisé déclenche l'ouverture
temporisée d'une serrure motorisée (servo), avec un tableau de bord web
accessible en WiFi pour suivi et déverrouillage manuel.

## Matériel
- 1x RC522 (SPI)
- 1x servomoteur SG90 (verrou)
- 1x carte ESP32 DevKit
- Alimentation externe 5V pour le servo (voir avertissements)

## Broches
| Module | Signal | ESP32 (GPIO) |
|--------|--------|--------------|
| RC522  | SCK    | GPIO18 |
| RC522  | MISO   | GPIO19 |
| RC522  | MOSI   | GPIO23 |
| RC522  | SDA/CS | GPIO5  |
| RC522  | RST    | GPIO27 |
| RC522  | VCC    | **3V3 (strict, voir avertissement)** |
| Servo  | Signal | GPIO16 |
| Servo  | VCC    | 5V externe |

## Câblage
GPIO18 --- SCK (RC522)
GPIO19 --- MISO (RC522)
GPIO23 --- MOSI (RC522)
GPIO5  --- SDA/SS (RC522)
GPIO27 --- RST (RC522)
3V3    --- VCC (RC522)
GND    --- GND (RC522, servo, alimentation externe - masse commune)
GPIO16 --- Signal (servo)
5V externe --- VCC (servo)

## Gestion des conflits de bus SPI
Le bus SPI (VSPI) est **partagé** entre le RC522 et tout autre périphérique SPI
que vous ajouteriez (ex: écran ILI9341 du projet 28). La règle est simple :
- SCK, MISO, MOSI sont **communs** à tous les périphériques SPI.
- **Chaque périphérique a sa propre broche CS (Chip Select)**, jamais partagée.
- Le RC522 utilise ici CS=GPIO5 ; si vous ajoutez un écran SPI, choisissez un
  autre GPIO libre (ex: GPIO15) pour son CS.

## Avertissements tension / niveaux logiques
- **RC522 = 3.3V STRICT** : ne jamais alimenter en 5V (voir détails dans le projet 17_rc522).
- **Le servo nécessite une alimentation 5V externe séparée** avec masse commune (GND) reliée à
  l'ESP32 : ne jamais l'alimenter depuis la broche 3V3/5V de l'ESP32 seule (risque de brownout
  lors du mouvement du servo, qui peut couper/redémarrer l'ESP32 en pleine lecture RFID).
- La machine à états du servo (`LOCKED → UNLOCKING → UNLOCKED → LOCKING → LOCKED`) est gérée
  entièrement via `millis()`, sans jamais bloquer le scan RFID ou le serveur web pendant les
  4 secondes d'ouverture.
- **Sécurité** : ce template stocke un UID codé en dur dans le firmware à titre pédagogique.
  Pour un déploiement réel, chiffrez/stockez les UID autorisés de façon plus robuste (ex: NVS
  chiffré, ou vérification côté serveur distant).
