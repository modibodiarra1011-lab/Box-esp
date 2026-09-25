# 26 - GPS NEO-6M

## But
Recevoir et décoder les trames NMEA d'un module GPS pour extraire position
(latitude/longitude), altitude, nombre de satellites, date et heure UTC.

## Matériel
- 1x module GPS NEO-6M (breakout avec régulateur 3.3V/5V et antenne céramique)
- 1x carte ESP32 DevKit

## Broches
| Signal GPS | ESP32 (GPIO) |
|------------|--------------|
| VCC        | 3V3 ou 5V (selon module, voir avertissement) |
| TX         | GPIO16 (RX2 de l'ESP32) |
| RX         | GPIO17 (TX2 de l'ESP32) |
| GND        | GND |

## Câblage
3V3 (ou 5V) --- VCC (module GPS)
GND --- GND
GPS TX --- GPIO16 (RX2 ESP32)
GPS RX --- GPIO17 (TX2 ESP32)

(Attention au croisement : le TX du GPS va vers le RX de l'ESP32, et vice-versa.)

## Avertissements tension / niveaux logiques
- La majorité des breakouts NEO-6M ont un régulateur intégré et acceptent 3.3V-5V en VCC,
  mais **vérifiez le niveau logique de sortie TX** de votre module précis : certains sortent
  du 5V sur TX, ce qui nécessiterait un diviseur de tension pour protéger le RX2 de l'ESP32
  (par sécurité, préférez toujours alimenter en 3.3V si le module le permet).
- Un premier "fix" satellite (accrochage GPS) peut prendre 30 secondes à plusieurs minutes en
  extérieur avec vue dégagée sur le ciel ; à l'intérieur, le module peut ne jamais obtenir de fix.
- Le débit par défaut du NEO-6M est 9600 bauds : ne le modifiez pas sans envoyer au préalable
  la commande UBX correspondante au module.
