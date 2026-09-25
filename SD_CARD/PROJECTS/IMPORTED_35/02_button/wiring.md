# 02 - Bouton poussoir (avec anti-rebond logiciel)

## But
Lire un état numérique de façon robuste, en éliminant les rebonds mécaniques
du bouton grâce à une temporisation logicielle basée sur `millis()`.

## Matériel
- 1x bouton poussoir (tactile switch)
- 1x carte ESP32 DevKit
- Câbles Dupont
- (Optionnel) résistance externe 10 kΩ si vous n'utilisez pas le pull-up interne

## Broches
| Signal        | ESP32 (GPIO) |
|---------------|--------------|
| Bouton (une patte) | GPIO4 |
| Bouton (autre patte) | GND |
| LED (statut)  | LED_BUILTIN |

## Câblage
GPIO4 --- bouton --- GND
(Le pull-up interne de l'ESP32 est activé via `INPUT_PULLUP`, donc GPIO4 est
au niveau HIGH au repos et passe à LOW lors de l'appui.)

## Avertissements tension / niveaux logiques
- Toujours câbler le bouton entre le GPIO et GND, jamais entre le GPIO et 3.3V/5V directement
  sans résistance de protection, pour éviter tout court-circuit accidentel.
- Ne connectez jamais un bouton relié au +5V (ex. rail 5V d'une alimentation externe) directement
  sur un GPIO ESP32 : cela dépasserait la tolérance 3.3V et pourrait détruire l'entrée.
- Certains GPIO ESP32 (0, 2, 5, 12, 15) ont un rôle au boot (strapping pins) : évitez-les pour un
  bouton si vous risquez de perturber la séquence de démarrage. GPIO4 est un choix sûr.
