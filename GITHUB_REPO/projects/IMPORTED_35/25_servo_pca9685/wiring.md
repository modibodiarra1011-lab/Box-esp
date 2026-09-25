# 25 - PCA9685 (driver PWM 16 canaux pour servos)

## But
Piloter jusqu'à 16 servomoteurs indépendamment via un seul bus I2C, en
déchargeant l'ESP32 de la génération directe des signaux PWM.

## Matériel
- 1x module PCA9685
- Jusqu'à 16x servomoteurs (SG90 ou compatibles)
- **Alimentation externe 5V-6V dédiée pour les servos** (bornier V+ du PCA9685)
- 1x carte ESP32 DevKit

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VCC (logique) | 3V3 |
| GND    | GND (commun avec alimentation servos) |
| SDA    | GPIO21 |
| SCL    | GPIO22 |
| V+ (bornier) | Alimentation externe 5V-6V (PAS depuis l'ESP32) |

## Câblage
3V3 --- VCC (logique I2C du PCA9685)
GND --- GND (commun avec l'alimentation servos ET l'ESP32)
GPIO21 --- SDA
GPIO22 --- SCL
Alimentation externe 5-6V --- bornier V+ (puissance des servos, séparé de la logique)
Chaque servo --- un canal PWM (0 a 15) du PCA9685

## Avertissements tension / niveaux logiques
- **Ne jamais alimenter le bornier V+ (puissance servos) depuis la broche 5V/3V3 de l'ESP32** :
  plusieurs servos en mouvement simultané peuvent consommer plusieurs ampères, très au-delà de
  ce qu'un régulateur de devkit peut fournir.
- La logique I2C du PCA9685 (VCC) est en 3.3V-5V tolérante : alimentez-la en 3.3V depuis
  l'ESP32 pour rester cohérent avec les niveaux logiques du bus.
- Le **GND doit être commun** entre l'ESP32, l'alimentation servos et le PCA9685, sinon les
  signaux PWM n'auront pas de référence de tension stable.
- Adresse I2C par défaut `0x40` (configurable de 0x40 à 0x7F via les pontages A0-A5 sur le
  module, utile pour chaîner plusieurs PCA9685 sur le même bus).
