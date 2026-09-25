# 10 - HC-SR04 (capteur ultrason)

## But
Mesurer une distance (2cm à ~4m) en chronométrant le temps de vol aller-retour
d'une impulsion ultrasonique de 40 kHz.

## Matériel
- 1x module HC-SR04
- 2x résistances pour diviseur de tension : 1kΩ et 2kΩ (ou 2.2kΩ) - **obligatoire**
- 1x carte ESP32 DevKit

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VCC    | 5V (VIN, PAS 3V3 !) |
| TRIG   | GPIO5 |
| ECHO   | GPIO18 (via diviseur de tension) |
| GND    | GND |

## Câblage (CRITIQUE)
- VCC du HC-SR04 → **5V** (le capteur a besoin de 5V pour fonctionner correctement ; les
  broches VIN/5V de l'ESP32 DevKit sont disponibles si alimenté par USB).
- TRIG → GPIO5 directement (c'est une sortie ESP32 en 3.3V vers une entrée TRIG du HC-SR04,
  qui accepte un niveau HIGH dès ~2V : compatible sans diviseur).
- ECHO → **diviseur de tension obligatoire** car ECHO sort du HC-SR04 en 5V :
  ```
  ECHO (HC-SR04) ---[1kΩ]---+---[2kΩ]--- GND
                              |
                          GPIO18 (ESP32)
  ```
  Ce pont diviseur ramène le signal 5V à environ 3.3V (5V x 2kΩ/(1kΩ+2kΩ) ≈ 3.33V).

## Avertissements tension / niveaux logiques
- **Ne jamais relier directement ECHO à un GPIO ESP32 sans diviseur de tension** : un signal
  5V direct sur une entrée 3.3V endommage progressivement ou immédiatement l'entrée GPIO.
- Un timeout logiciel (`ECHO_TIMEOUT_US`) est indispensable dans le code : sans obstacle en
  face, `pulseIn()` peut bloquer indéfiniment sans limite de temps.
- Distance minimale fiable : ~2cm. En dessous, les mesures deviennent erratiques (écho parasite).
