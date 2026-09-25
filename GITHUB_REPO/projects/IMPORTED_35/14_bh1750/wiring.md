# 14 - BH1750 (capteur de luminosité en lux)

## But
Mesurer précisément la luminosité ambiante en lux (contrairement à la LDR qui
ne donne qu'une valeur relative), via un capteur numérique I2C dédié.

## Matériel
- 1x module BH1750 (breakout I2C)
- 1x carte ESP32 DevKit

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VCC    | 3V3 |
| GND    | GND |
| SCL    | GPIO22 |
| SDA    | GPIO21 |
| ADDR   | GND (0x23) ou 3V3 (0x5C) — laisser flottant = comportement indéfini |

## Câblage
3V3 --- VCC
GND --- GND
GPIO21 --- SDA
GPIO22 --- SCL
GND --- ADDR (pour adresse 0x23, utilisée dans ce code)

## Avertissements tension / niveaux logiques
- Le BH1750 fonctionne en 2.4V-3.6V : alimentez-le **exclusivement en 3.3V**, jamais en 5V,
  car il n'a généralement pas de régulateur sur les breakouts bon marché.
- Ne laissez pas la broche ADDR flottante : elle doit être reliée fermement à GND ou VCC pour
  fixer une adresse I2C stable et éviter des conflits intermittents.
