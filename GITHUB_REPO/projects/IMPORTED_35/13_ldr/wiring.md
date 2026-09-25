# 13 - LDR (photorésistance)

## But
Détecter le niveau de luminosité ambiante grâce à un pont diviseur de tension
formé par une LDR (résistance qui varie avec la lumière) et une résistance fixe.

## Matériel
- 1x LDR (photorésistance, ex: GL5528)
- 1x résistance fixe 10 kΩ
- 1x carte ESP32 DevKit

## Broches
| Signal   | ESP32 (GPIO) |
|----------|--------------|
| Point milieu du pont | GPIO34 |

## Câblage
3V3 --- LDR --- (point milieu) --- résistance 10kΩ --- GND
                       |
                   GPIO34

(Plus il fait sombre, plus la résistance de la LDR augmente, donc plus la
tension lue sur GPIO34 diminue si le montage est câblé comme ci-dessus.)

## Avertissements tension / niveaux logiques
- Toujours alimenter le pont diviseur en **3.3V** (et non 5V) car il attaque directement une
  entrée ADC ESP32 limitée à 3.3V max.
- GPIO34 est une broche "input only" (ADC1) : parfaite pour cet usage, mais ne peut pas être
  utilisée comme sortie ailleurs dans un projet combiné.
- Les seuils `DARK_THRESHOLD` / `LIGHT_THRESHOLD` sont indicatifs : recalibrez-les selon votre
  LDR précise, la valeur de la résistance fixe choisie, et les conditions d'éclairage réelles.
