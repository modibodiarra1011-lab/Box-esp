# 03 - Potentiomètre

## But
Lire une tension analogique variable (0 à 3.3V) et la convertir en valeur
numérique (0-4095 sur ESP32) puis en volts / pourcentage.

## Matériel
- 1x potentiomètre linéaire (10 kΩ typique)
- 1x carte ESP32 DevKit

## Broches
| Signal            | ESP32 (GPIO) |
|-------------------|--------------|
| Curseur (wiper)   | GPIO34 (ADC1_CH6, entrée uniquement) |
| Patte 1           | 3V3 |
| Patte 2           | GND |

## Câblage
3V3 --- patte1(potentiomètre) --- curseur → GPIO34
                      patte2 --- GND

## Avertissements tension / niveaux logiques
- **Ne jamais** alimenter le potentiomètre en 5V si le curseur est lu par un GPIO ESP32 :
  la tension max en entrée ADC est 3.3V (au-delà, risque de dommage permanent).
- GPIO34 à GPIO39 sont des broches "input only" (pas de pull-up/pull-down interne, pas de sortie) :
  idéales pour l'ADC mais inutilisables comme sorties numériques.
- L'ADC2 de l'ESP32 (GPIO0, 2, 4, 12-15, 25-27) est indisponible pendant l'utilisation du WiFi :
  toujours préférer l'ADC1 (GPIO32-39) pour des lectures analogiques fiables, comme fait ici.
- L'ADC de l'ESP32 n'est pas parfaitement linéaire aux extrémités (proche de 0V et proche de 3.3V) :
  pour des mesures de précision, prévoir un étalonnage logiciel (fonction `adc_calibration` d'Espressif).
