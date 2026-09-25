# 15 - VL53L0X (télémètre laser Time-of-Flight)

## But
Mesurer une distance précise (jusqu'à ~2m) par temps de vol d'un faisceau
laser infrarouge, avec une résolution millimétrique et une latence très faible.

## Matériel
- 1x module VL53L0X (breakout I2C, ex: Adafruit ou GY-VL53L0XV2)
- 1x carte ESP32 DevKit

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VIN/VCC| 3V3 |
| GND    | GND |
| SCL    | GPIO22 |
| SDA    | GPIO21 |
| XSHUT  | GPIO19 (optionnel, pour reset/adressage multi-capteurs) |
| GPIO1 (interrupt) | non utilisé dans ce template |

## Câblage
3V3 --- VIN
GND --- GND
GPIO21 --- SDA
GPIO22 --- SCL
(XSHUT laissé flottant/tiré au 3V3 = capteur actif par défaut)

## Avertissements tension / niveaux logiques
- Le VL53L0X est un capteur **3.3V natif** : la plupart des breakouts ont un régulateur
  acceptant 3V-5V en entrée, mais si le vôtre est en "VCC direct" sans régulateur, alimentez
  impérativement en 3.3V.
- Adresse I2C fixe par défaut `0x29`. Pour utiliser **plusieurs VL53L0X sur le même bus**,
  câblez chaque XSHUT sur un GPIO distinct, maintenez-les tous en LOW au démarrage, puis
  activez-les un par un en changeant leur adresse I2C via `setAddress()` avant de les relâcher.
- Le faisceau laser est de classe 1 (sans danger pour l'œil dans un usage normal), mais évitez
  toute utilisation prolongée face à un œil à très courte distance par précaution.
