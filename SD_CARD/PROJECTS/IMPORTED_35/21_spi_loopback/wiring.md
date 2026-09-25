# 21 - Boucle SPI (SPI Loopback)

## But
Valider le bus SPI matériel VSPI de l'ESP32 en reliant physiquement MOSI à
MISO, ce qui permet à l'ESP32 (en mode maître) de se renvoyer ses propres
octets et de vérifier l'intégrité de la transmission.

## Matériel
- 1x carte ESP32 DevKit
- 1x fil de câblage (jumper) pour relier MOSI à MISO

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| SCK    | GPIO18 |
| MISO   | GPIO19 |
| MOSI   | GPIO23 |
| CS     | GPIO5 (non connecté électriquement, géré en logiciel) |

## Câblage
GPIO23 (MOSI) ------- jumper ------- GPIO19 (MISO)

(SCK et CS restent non connectés à l'extérieur ; le test est interne à la carte.)

## Avertissements tension / niveaux logiques
- Toutes les broches SPI de l'ESP32 sont en logique **3.3V** : câblage direct sans diviseur.
- Ce test valide uniquement l'électronique interne et le driver SPI ; il ne remplace pas un
  test avec un véritable périphérique esclave (ex: RC522, ILI9341, MAX6675 dans ce même lab).
- Si vous ajoutez de vrais périphériques SPI ensuite, retirez impérativement le jumper
  MOSI-MISO de ce test avant de les connecter, sous peine de conflit de bus.
