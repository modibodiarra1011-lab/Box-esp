# 11 - DS18B20 (température OneWire)

## But
Lire une température précise (±0.5°C) via le protocole numérique 1-Wire,
qui permet de chaîner plusieurs capteurs sur un seul fil de données.

## Matériel
- 1x (ou plusieurs) DS18B20, format TO-92 ou sonde étanche
- 1x résistance 4.7 kΩ (pull-up), **obligatoire**
- 1x carte ESP32 DevKit

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VDD    | 3V3 |
| DQ (data) | GPIO4 |
| GND    | GND |

## Câblage
3V3 ---+--- VDD (DS18B20)
       |
    [4.7kΩ pull-up]
       |
GPIO4 -+--- DQ (DS18B20)
GND -------- GND (DS18B20)

Pour plusieurs capteurs : reliez tous les DQ ensemble sur GPIO4 (une seule
résistance de pull-up suffit pour tout le bus), chaque capteur a une adresse
64 bits unique lue automatiquement par la bibliothèque.

## Avertissements tension / niveaux logiques
- Le DS18B20 fonctionne en 3.0V-5.5V ; utilisez 3.3V pour rester compatible avec la logique
  GPIO de l'ESP32 sans diviseur de tension supplémentaire.
- **Mode "parasite power"** (alimentation par la seule ligne data, sans VDD) : non recommandé
  avec l'ESP32 dans ce template — préférez le câblage normal 3 fils ci-dessus pour la fiabilité.
- La résistance de pull-up 4.7kΩ est indispensable : sans elle, le bus reste flottant et les
  lectures échoueront ou seront erratiques.
- Le temps de conversion (750ms en 12 bits) est géré ici en mode non bloquant
  (`setWaitForConversion(false)`) pour ne jamais geler la boucle principale.
