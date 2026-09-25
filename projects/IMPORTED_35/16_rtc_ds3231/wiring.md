# 16 - DS3231 (horloge temps réel)

## But
Maintenir une date/heure précise même hors tension (grâce à une pile de secours
CR2032), et lire un capteur de température interne au module.

## Matériel
- 1x module RTC DS3231 (avec pile CR2032 pour la sauvegarde d'horloge)
- 1x carte ESP32 DevKit

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VCC    | 3V3 |
| GND    | GND |
| SCL    | GPIO22 |
| SDA    | GPIO21 |
| SQW    | non utilisé dans ce template (interruption périodique optionnelle) |

## Câblage
3V3 --- VCC
GND --- GND
GPIO21 --- SDA
GPIO22 --- SCL

## Avertissements tension / niveaux logiques
- Le DS3231 accepte 2.3V-5.5V, mais utilisez **3.3V** pour rester cohérent avec les niveaux
  logiques I2C de l'ESP32.
- **Conflit d'adresse I2C connu avec le MPU6050** : les deux utilisent `0x68` par défaut.
  Si vous combinez RTC + IMU sur le même bus, décalez l'adresse du MPU6050 à `0x69` (broche AD0
  au VCC).
- Vérifiez la pile CR2032 : une pile vide entraîne une réinitialisation de l'horloge à chaque
  coupure d'alimentation (le code détecte ce cas via `rtc.lostPower()`).
