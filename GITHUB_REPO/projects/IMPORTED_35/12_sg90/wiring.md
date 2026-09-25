# 12 - Servomoteur SG90

## But
Faire pivoter un servomoteur entre 0° et 180° en balayage continu et non
bloquant, via un signal PWM 50Hz.

## Matériel
- 1x servomoteur SG90 (ou compatible 5V)
- 1x carte ESP32 DevKit
- **Alimentation externe 5V recommandée** pour le servo (voir avertissements)

## Broches
| Signal            | ESP32 (GPIO) |
|-------------------|--------------|
| Signal (orange/jaune) | GPIO18 |
| VCC (rouge)       | 5V (externe recommandé) |
| GND (marron/noir) | GND (commun avec l'ESP32) |

## Câblage
GPIO18 --- fil Signal (orange) du servo
5V (alim externe ou VIN) --- fil VCC (rouge) du servo
GND (commun ESP32 + alim) --- fil GND (marron) du servo

## Avertissements tension / niveaux logiques
- **Le signal PWM à 3.3V (sortie GPIO ESP32) est généralement suffisant** pour être reconnu
  comme un niveau HIGH par l'électronique interne du SG90 (seuil ~2.5V à 5V d'alim), mais
  cela reste à la limite selon les clones : pour un montage robuste et professionnel,
  utilisez un level-shifter ou testez avec votre servo précis.
- **Ne jamais alimenter le servo directement depuis la broche 3V3 de l'ESP32** : le SG90 peut
  tirer 100-250 mA en pointe (mouvement + charge), ce qui dépasse largement la capacité du
  régulateur 3.3V embarqué sur la plupart des devkits (souvent limité à ~500mA partagés).
- Utilisez une **alimentation 5V externe dédiée** avec **masse commune (GND)** reliée à l'ESP32,
  pour éviter les redémarrages intempestifs (brownout) causés par les appels de courant du servo.
- Reliez toujours le GND de l'alimentation externe au GND de l'ESP32, sinon le signal PWM n'aura
  pas de référence commune et le servo se comportera de façon erratique.
