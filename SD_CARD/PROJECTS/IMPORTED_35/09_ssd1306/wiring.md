# 09 - Écran OLED SSD1306 (128x64, I2C)

## But
Afficher du texte et des graphiques simples sur un petit écran OLED
monochrome piloté en I2C.

## Matériel
- 1x écran OLED SSD1306 0.96" (128x64, interface I2C, adresse 0x3C généralement)
- 1x carte ESP32 DevKit

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| VCC    | 3V3 |
| GND    | GND |
| SCL    | GPIO22 |
| SDA    | GPIO21 |

## Câblage
3V3 --- VCC
GND --- GND
GPIO21 --- SDA
GPIO22 --- SCL

## Avertissements tension / niveaux logiques
- La plupart des modules SSD1306 sont conçus pour fonctionner de 3.3V à 5V, mais **privilégiez
  toujours 3.3V** avec l'ESP32 pour rester cohérent avec le niveau logique des lignes I2C.
- Adresse I2C typique `0x3C` (parfois `0x3D` selon le fabricant) : vérifiez avec le projet
  05_i2c_scanner en cas de doute.
- Ne jamais alimenter le module par la broche 5V du bus USB si vous alimentez également l'ESP32
  via USB, sauf si le régulateur du module accepte cette tension — cela peut créer un conflit
  d'alimentation entre deux sources 5V.
