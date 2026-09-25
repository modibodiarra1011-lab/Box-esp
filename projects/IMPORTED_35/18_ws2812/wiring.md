# 18 - WS2812 (bandeau LED adressable / NeoPixel)

## But
Piloter individuellement chaque LED d'un bandeau WS2812 pour créer une
animation arc-en-ciel non bloquante.

## Matériel
- 1x bandeau/anneau WS2812 (ex: 8 LEDs)
- 1x condensateur 1000µF (recommandé, entre + et - d'alimentation du bandeau)
- 1x résistance 300-500Ω en série sur DATA (recommandé)
- 1x carte ESP32 DevKit
- Alimentation externe 5V si plus de ~8-10 LEDs

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| DIN (data) | GPIO5 (via résistance série 300-500Ω recommandée) |
| VCC    | 5V (externe si nombreuses LEDs) |
| GND    | GND (commun avec ESP32) |

## Câblage
5V (alim) ---+--- VCC (bandeau)
             |
        [1000µF entre + et -]
             |
GND (commun)-+--- GND (bandeau)

GPIO5 ---[résistance 300-500Ω]--- DIN (bandeau)

## Avertissements tension / niveaux logiques
- **Incompatibilité de niveau logique potentielle** : le WS2812 est spécifié pour un signal
  DATA à 5V (seuil HIGH ≈ 0.7×VCC = 3.5V si alimenté en 5V), alors que l'ESP32 sort du 3.3V.
  Sur de courtes distances (<50cm) et avec un bandeau neuf, cela fonctionne généralement, mais
  pour un montage fiable/production, insérez un level-shifter (ex: 74AHCT125) entre GPIO5 et DIN.
- Chaque LED WS2812 peut consommer jusqu'à ~60mA à pleine luminosité blanche : pour 8 LEDs,
  prévoyez jusqu'à 480mA — **ne jamais alimenter directement depuis la broche 3V3 de l'ESP32**.
- Le condensateur 1000µF en entrée d'alimentation absorbe les pics de courant à l'allumage et
  protège les premières LED du bandeau.
- La résistance série sur DATA protège la première LED des transitoires électriques.
