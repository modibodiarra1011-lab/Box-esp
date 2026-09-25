# 01 - Blink LED

## But
Faire clignoter une LED pour valider la chaîne complète : IDE -> compilation ->
flash -> exécution sur la carte. C'est le "Hello World" du GPIO.

## Matériel
- 1x carte ESP32 DevKit (ou Arduino Uno)
- 1x LED (si vous n'utilisez pas la LED embarquée)
- 1x résistance 220 Ω à 330 Ω (si LED externe)
- Câbles Dupont

## Broches
| Signal      | ESP32 (GPIO) | Arduino Uno |
|-------------|--------------|-------------|
| LED (anode) | GPIO2 (LED_BUILTIN sur la plupart des devkits) | Pin 13 (LED_BUILTIN) |
| LED (cathode) | GND | GND |

## Câblage LED externe (optionnel)
GPIO2 → résistance 220 Ω → anode LED → cathode LED → GND

## Avertissements tension / niveaux logiques
- L'ESP32 fonctionne en logique **3.3V**, contrairement à l'Arduino Uno qui est en **5V**.
- Ne jamais appliquer plus de 3.3V sur une broche GPIO ESP32 (pas de tolérance 5V comme sur certains Uno).
- Le courant max recommandé par GPIO ESP32 est d'environ 12 mA en continu (40 mA en pic absolu) :
  toujours utiliser une résistance série avec une LED, jamais de connexion directe.
- LED_BUILTIN peut ne pas exister ou être câblée en logique inversée selon le fabricant de la carte :
  vérifiez le schéma de votre carte spécifique si le comportement semble inversé.
