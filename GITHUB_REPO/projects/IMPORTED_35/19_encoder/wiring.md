# 19 - Encodeur rotatif incrémental

## But
Détecter le sens et le nombre de crans de rotation d'un encodeur mécanique
(type KY-040) via interruption matérielle, plus lecture de son bouton intégré.

## Matériel
- 1x module encodeur rotatif KY-040 (ou équivalent)
- 1x carte ESP32 DevKit

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| CLK    | GPIO32 |
| DT     | GPIO33 |
| SW     | GPIO25 |
| +      | 3V3 |
| GND    | GND |

## Câblage
3V3 --- + (VCC du module)
GND --- GND
GPIO32 --- CLK
GPIO33 --- DT
GPIO25 --- SW

## Avertissements tension / niveaux logiques
- Alimentez le module en **3.3V** : la plupart des KY-040 fonctionnent bien en 3.3V comme en 5V
  (logique tout ou rien), mais restez cohérent avec les GPIO ESP32.
- Les pull-up internes (`INPUT_PULLUP`) sont utilisées sur CLK, DT et SW : ne câblez pas de
  résistances de tirage supplémentaires sauf si votre module l'exige explicitement.
- Les encodeurs mécaniques génèrent des rebonds électriques même sur le signal CLK utilisé en
  interruption : si vous observez des sauts de plusieurs crans par déclic, envisagez un
  encodeur avec sortie déjà filtrée (Schmitt trigger) ou ajoutez un filtre RC (100nF + 1kΩ).
