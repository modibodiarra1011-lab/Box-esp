# 20 - Boucle UART (Serial Loopback)

## But
Valider le fonctionnement d'un port UART matériel en reliant physiquement sa
broche TX à sa broche RX, et en vérifiant que chaque message envoyé revient
identique.

## Matériel
- 1x carte ESP32 DevKit (UART2 matériel, distinct du port USB/debug UART0)
- 1x fil de câblage (jumper) pour relier TX à RX

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| TX2    | GPIO17 |
| RX2    | GPIO16 |

## Câblage
GPIO17 (TX2) ------- jumper ------- GPIO16 (RX2)

(Aucune autre connexion nécessaire ; le test est purement interne à la carte.)

## Avertissements tension / niveaux logiques
- Les deux broches sont en logique **3.3V** côté ESP32 : ce montage est sûr sans diviseur
  puisque TX et RX sont tous deux à 3.3V.
- N'utilisez **jamais** UART0 (GPIO1/GPIO3, utilisé par le port USB/programmation) pour ce test
  en boucle, sous peine de perturber la communication avec l'IDE Arduino pendant le flashage.
- Si vous testez ce montage sur un Arduino Uno, il ne possède qu'un seul UART matériel
  (déjà utilisé par USB) : utilisez la bibliothèque `SoftwareSerial` sur des broches
  numériques libres à la place de `HardwareSerial(2)`.
