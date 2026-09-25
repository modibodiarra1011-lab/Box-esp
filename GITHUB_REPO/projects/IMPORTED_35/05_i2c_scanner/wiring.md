# 05 - Scanner I2C

## But
Balayer les 127 adresses possibles du bus I2C et lister les périphériques
connectés. Outil de diagnostic indispensable avant tout projet combiné
utilisant plusieurs modules I2C (BME280, SSD1306, DS3231, MPU6050, etc.).

## Matériel
- 1x carte ESP32 DevKit
- 1 ou plusieurs modules I2C à tester
- 2x résistances de tirage (pull-up) 4.7 kΩ (souvent déjà présentes sur les breakouts)

## Broches
| Signal | ESP32 (GPIO) | Arduino Uno |
|--------|--------------|-------------|
| SDA    | GPIO21       | A4          |
| SCL    | GPIO22       | A5          |
| VCC    | 3V3          | 5V          |
| GND    | GND          | GND         |

## Câblage
GPIO21 (SDA) --- SDA du module (+ pull-up 4.7kΩ vers 3V3 si absente sur le module)
GPIO22 (SCL) --- SCL du module (+ pull-up 4.7kΩ vers 3V3 si absente sur le module)
3V3          --- VCC du module
GND          --- GND du module

## Avertissements tension / niveaux logiques
- La majorité des modules I2C bon marché (BME280, SSD1306, DS3231...) acceptent 3.3V-5V en
  alimentation mais **le bus SDA/SCL doit rester en 3.3V** côté ESP32. Si un module n'a pas de
  régulateur/level-shifter à bord (rare) et impose du 5V sur SDA/SCL, insérez un level-shifter
  bidirectionnel I2C.
- **Conflit d'adresses connu** : le DS3231 (RTC) et le MPU6050 partagent tous deux l'adresse
  par défaut `0x68`. Si les deux sont sur le même bus, tirez la broche AD0 du MPU6050 au VCC
  pour passer à l'adresse `0x69`.
- Ne dépassez pas ~400 kHz d'horloge I2C avec des câbles longs (>20cm) sans pull-up adaptées,
  sous peine d'erreurs de communication.
