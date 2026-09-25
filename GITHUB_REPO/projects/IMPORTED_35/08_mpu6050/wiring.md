# 08 - MPU6050 (IMU 6 axes)

## But
Lire l'accélération 3 axes, la vitesse angulaire (gyroscope) 3 axes, et la
température interne du capteur, via I2C.

## Matériel
- 1x module MPU6050 (breakout I2C)
- 1x carte ESP32 DevKit

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VCC    | 3V3 |
| GND    | GND |
| SCL    | GPIO22 |
| SDA    | GPIO21 |
| AD0    | GND (adresse 0x68) ou 3V3 (adresse 0x69) |

## Câblage
3V3 --- VCC
GND --- GND
GPIO21 --- SDA
GPIO22 --- SCL
GND (ou 3V3) --- AD0 (selon adresse souhaitee)

## Avertissements tension / niveaux logiques
- Le MPU6050 est nativement en 3.3V : ne l'alimentez **jamais** en 5V, même si le breakout
  possède un régulateur (certains clones n'en ont pas).
- **Conflit d'adresse I2C connu avec le DS3231** (les deux utilisent `0x68` par défaut).
  Si vous combinez les deux (voir projet 31_station_meteo_complete si applicable, ou tout
  montage personnalisé), tirez AD0 du MPU6050 vers 3V3 pour passer à l'adresse `0x69`.
- Manipulez le capteur avec précaution lors des tests : les valeurs du gyroscope dérivent
  légèrement dans le temps (bias thermique) ; pour une IMU de précision, prévoir une calibration
  logicielle au démarrage (moyenne des lectures immobiles).
