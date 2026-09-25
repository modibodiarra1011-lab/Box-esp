# 07 - BME280 (température / humidité / pression)

## But
Mesurer température, humidité relative et pression atmosphérique via I2C,
avec estimation d'altitude par rapport à une pression de référence.

## Matériel
- 1x module BME280 (breakout I2C, généralement déjà régulé en 3.3V avec pull-up)
- 1x carte ESP32 DevKit

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VCC    | 3V3 |
| GND    | GND |
| SCL    | GPIO22 |
| SDA    | GPIO21 |

## Câblage
3V3   --- VCC
GND   --- GND
GPIO21 -- SDA
GPIO22 -- SCL

## Avertissements tension / niveaux logiques
- **Vérifiez le régulateur de votre module** : certains breakouts BME280 génériques n'ont pas
  de régulateur 3.3V et exigent une alimentation directe en 3.3V uniquement (jamais 5V).
- Adresse I2C par défaut `0x76` (broche SDO à GND) ou `0x77` (SDO à VCC) : à adapter dans le
  code si votre module diffère.
- Ne pas confondre avec le BMP280 (sans capteur d'humidité) qui utilise la même bibliothèque
  de base mais ne renverra pas de valeur d'humidité valide.
- Placez le capteur à l'écart de toute source de chaleur (régulateur, LED) pour éviter un biais
  de mesure de température de +1 à +3°C.
