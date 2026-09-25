# 30 - ADS1115 (ADC externe 16 bits, 4 canaux)

## But
Obtenir une conversion analogique-numérique bien plus précise (16 bits) et
sur 4 canaux indépendants que l'ADC interne de l'ESP32 (12 bits, non linéaire
aux extrémités), particulièrement utile pour des capteurs analogiques précis.

## Matériel
- 1x module ADS1115
- 1x carte ESP32 DevKit
- Sources de tension/capteurs analogiques à mesurer (0-4 signaux)

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VDD    | 3V3 |
| GND    | GND |
| SCL    | GPIO22 |
| SDA    | GPIO21 |
| ADDR   | GND (adresse 0x48) |
| A0-A3  | signaux analogiques à mesurer |

## Câblage
3V3 --- VDD
GND --- GND, ADDR (pour adresse 0x48)
GPIO21 --- SDA
GPIO22 --- SCL
Signal 1..4 --- A0..A3

## Avertissements tension / niveaux logiques
- Le gain `GAIN_TWOTHIRDS` configuré ici accepte des signaux jusqu'à **±6.144V** en entrée,
  mais **VDD du module reste à 3.3V** : ne dépassez jamais VDD+0.3V sur une entrée analogique
  (soit ~3.6V max en pratique) si vous alimentez le module en 3.3V, sous peine de dommage.
  Pour mesurer des tensions plus élevées en toute sécurité, alimentez le module en 5V (accepté
  par l'ADS1115) tout en gardant le bus I2C (SDA/SCL) compatible 3.3V (la plupart des modules
  gèrent cela nativement via des broches séparées).
- L'adresse I2C dépend du câblage de ADDR : GND=0x48, VDD=0x49, SDA=0x4A, SCL=0x4B — pratique
  pour chaîner jusqu'à 4 modules ADS1115 sur le même bus.
- Ne laissez jamais une entrée A0-A3 non utilisée totalement flottante si vous lisez ce canal :
  cela produirait des valeurs aléatoires ; reliez les canaux inutilisés à GND.
