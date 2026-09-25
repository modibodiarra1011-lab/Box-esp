# 28 - Écran TFT ILI9341 (320x240, SPI)

## But
Afficher du texte et des graphiques colorés sur un écran TFT haute résolution
piloté en SPI, avec une mise à jour partielle non bloquante de l'affichage.

## Matériel
- 1x module ILI9341 (breakout SPI, souvent avec régulateur 3.3V intégré)
- 1x carte ESP32 DevKit

## Broches
| Signal ILI9341 | ESP32 (GPIO) |
|-----------------|--------------|
| VCC             | 3V3 |
| GND             | GND |
| CS              | GPIO5 |
| RESET           | GPIO4 |
| DC (ou D/C, RS) | GPIO2 |
| SDI (MOSI)      | GPIO23 |
| SCK             | GPIO18 |
| SDO (MISO)      | GPIO19 (optionnel selon module) |
| LED (backlight) | 3V3 (ou GPIO dédié via transistor pour gradation) |

## Câblage
3V3 --- VCC, LED (rétroéclairage)
GND --- GND
GPIO5  --- CS
GPIO4  --- RESET
GPIO2  --- DC
GPIO23 --- SDI/MOSI
GPIO18 --- SCK
GPIO19 --- SDO/MISO

## Avertissements tension / niveaux logiques
- La plupart des breakouts ILI9341 grand public sont conçus pour être pilotés en **3.3V
  directement** (logique native du contrôleur), ce qui les rend particulièrement bien adaptés
  à l'ESP32 sans diviseur de tension.
- Vérifiez néanmoins la sérigraphie de votre module : certains kits "5V tolerant" ajoutent un
  régulateur mais imposent alors 5V en VCC avec logique SPI toujours en 3.3V — lisez la
  documentation de votre revendeur en cas de doute.
- GPIO2 (utilisé ici pour DC) est une "strapping pin" au boot sur l'ESP32 : si l'écran perturbe
  le démarrage (boot loop), déplacez DC vers un GPIO neutre comme GPIO27 ou GPIO33.
- Le rétroéclairage (LED) peut consommer un courant non négligeable : pour une gradation
  logicielle, pilotez-le via un GPIO en PWM et un transistor NPN plutôt qu'en direct.
