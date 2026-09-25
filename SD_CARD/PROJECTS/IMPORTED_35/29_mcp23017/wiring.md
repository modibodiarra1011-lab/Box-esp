# 29 - MCP23017 (extension GPIO I2C)

## But
Ajouter 16 broches numériques (entrées ou sorties) supplémentaires à l'ESP32
via un seul bus I2C, utile quand tous les GPIO natifs sont déjà occupés.

## Matériel
- 1x module MCP23017
- 1x carte ESP32 DevKit
- LEDs/boutons selon votre montage de test

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VDD    | 3V3 |
| VSS    | GND |
| SCL    | GPIO22 |
| SDA    | GPIO21 |
| A0,A1,A2 | GND (adresse 0x20) |
| RESET  | 3V3 (jamais laissé flottant) |

## Câblage
3V3 --- VDD, RESET, A0, A1, A2 (tous a GND en realite pour A0-A2, voir tableau ci-dessus)
GND --- VSS, A0, A1, A2
GPIO21 --- SDA
GPIO22 --- SCL

Note : RESET doit être tiré au VDD (actif à l'état bas), tandis que A0/A1/A2
doivent être tirés à GND pour fixer l'adresse 0x20 utilisée dans ce code.

## Avertissements tension / niveaux logiques
- Le MCP23017 fonctionne en 1.8V-5.5V : alimentez-le en **3.3V** pour que ses broches GPA/GPB
  restent compatibles avec vos autres périphériques 3.3V (LEDs, boutons, etc.).
- **Ne laissez jamais RESET flottant** : un niveau LOW inattendu réinitialiserait le composant
  de façon intempestive. Reliez-le fermement à VDD.
- Les broches GPA/GPB du MCP23017 peuvent piloter directement des LEDs avec résistance série,
  mais ne fournissez jamais plus de ~25mA par broche (limite absolue du composant).
- Adresse I2C configurable de 0x20 à 0x27 selon le câblage de A0/A1/A2, ce qui permet de
  chaîner jusqu'à 8 MCP23017 sur le même bus (soit 128 GPIO supplémentaires au total).
