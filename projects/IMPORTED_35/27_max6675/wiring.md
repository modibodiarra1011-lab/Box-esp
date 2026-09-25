# 27 - MAX6675 (amplificateur thermocouple type K)

## But
Lire une température élevée (jusqu'à 1024°C) via un thermocouple type K
amplifié et numérisé par le circuit MAX6675, en SPI lecture seule.

## Matériel
- 1x module MAX6675
- 1x thermocouple type K
- 1x carte ESP32 DevKit

## Broches
| Signal MAX6675 | ESP32 (GPIO) |
|-----------------|--------------|
| VCC             | 3V3 |
| GND             | GND |
| SCK             | GPIO18 |
| CS              | GPIO15 |
| SO (data out)   | GPIO19 |

## Câblage
3V3 --- VCC
GND --- GND
GPIO18 --- SCK
GPIO15 --- CS
GPIO19 --- SO

## Avertissements tension / niveaux logiques
- Le MAX6675 fonctionne en 3.0V-5.5V, mais utilisez **3.3V** pour rester dans la plage de
  tolérance native des GPIO ESP32 sur la sortie SO (qui est en logique VCC, donc 3.3V si
  alimenté en 3.3V).
- **Ce circuit est en lecture SEULE** (pas de MOSI/DIN) : ne tentez pas d'écrire sur ce bus.
- Le MAX6675 a une résolution de 0.25°C mais une précision réelle de ±3°C environ ; pour une
  précision plus fine, envisagez le MAX31855 (compatible thermocouple type K également).
- Vérifiez la polarité du thermocouple (fil + et fil -) : une inversion donnera des lectures
  de température erronées (souvent négatives ou aberrantes) sans message d'erreur explicite.
