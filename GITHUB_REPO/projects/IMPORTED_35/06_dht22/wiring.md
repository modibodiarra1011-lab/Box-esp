# 06 - DHT22 (température / humidité)

## But
Lire la température (°C) et l'humidité relative (%) ambiantes via un capteur
numérique à un seul fil de données.

## Matériel
- 1x module DHT22 (AM2302), idéalement en version breakout avec pull-up intégrée
- 1x résistance 10 kΩ (pull-up), sauf si déjà présente sur le module
- 1x carte ESP32 DevKit

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VCC    | 3V3 |
| DATA   | GPIO4 |
| GND    | GND |

## Câblage
3V3 ---+--- VCC (DHT22)
       |
      [10kΩ pull-up]
       |
GPIO4 -+--- DATA (DHT22)
GND  ------ GND (DHT22)

## Avertissements tension / niveaux logiques
- Le DHT22 fonctionne en 3.3V-5.5V : alimentez-le en **3.3V** depuis l'ESP32 pour rester
  cohérent avec la logique de la broche DATA (3.3V), et éviter tout stress sur l'entrée GPIO.
- Si vous utilisez un module DHT22 5V avec pull-up interne câblée vers 5V, la ligne DATA peut
  dépasser 3.3V et endommager l'entrée GPIO ESP32 : utilisez un module 3.3V ou un diviseur de tension.
- Respecter un délai minimum de ~2 secondes entre deux lectures (le capteur est physiquement
  limité à 0.5 Hz) : c'est géré ici par `READ_INTERVAL_MS`.
- Câble DATA de plus de 20m : ajouter un condensateur de découplage 100nF entre VCC et GND
  proche du capteur.
