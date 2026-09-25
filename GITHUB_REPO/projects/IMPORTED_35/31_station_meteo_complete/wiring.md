# 31 - Station météo complète (BME280 + SSD1306 + DS3231)

## But
Combiner trois modules I2C sur un seul bus pour afficher en temps réel la
température, l'humidité, la pression atmosphérique et l'heure exacte, sur un
écran OLED local. Architecture entièrement non bloquante (`millis()`).

## Matériel
- 1x BME280 (I2C)
- 1x écran SSD1306 128x64 (I2C)
- 1x module RTC DS3231 (I2C)
- 1x carte ESP32 DevKit

## Broches (bus I2C unique et partagé)
| Signal | ESP32 (GPIO) |
|--------|--------------|
| SDA (commun aux 3 modules) | GPIO21 |
| SCL (commun aux 3 modules) | GPIO22 |
| VCC (commun aux 3 modules) | 3V3 |
| GND (commun aux 3 modules) | GND |

## Câblage
Tous les modules se connectent **en parallèle** sur le même bus :
```
                 +--- BME280  (VCC, GND, SDA, SCL)
3V3 --- VCC ------
GND --- GND ------+--- SSD1306 (VCC, GND, SDA, SCL)
GPIO21 --- SDA ---+
GPIO22 --- SCL ---+--- DS3231  (VCC, GND, SDA, SCL)
```

## Gestion des adresses I2C (vérifiée sans conflit)
| Module  | Adresse I2C |
|---------|-------------|
| BME280  | 0x76 (SDO à GND) |
| SSD1306 | 0x3C |
| DS3231  | 0x68 |

Ces trois adresses sont **distinctes**, donc aucun conflit sur ce montage.
⚠️ Si vous ajoutez plus tard un MPU6050 (0x68 par défaut) à ce même bus, il
entrerait en conflit avec le DS3231 : décalez alors l'adresse du MPU6050 à
0x69 en tirant sa broche AD0 au VCC. Utilisez le projet **05_i2c_scanner**
pour valider l'absence de conflit avant tout ajout de module.

## Avertissements tension / niveaux logiques
- Les trois modules sont conçus pour du 3.3V-5V en alimentation, mais toujours privilégier
  **3.3V unique** pour la cohérence des niveaux logiques SDA/SCL avec l'ESP32.
- Limitez la longueur des câbles I2C partagés à moins de 30-40cm pour un bus fiable à 100kHz
  avec trois périphériques ; au-delà, envisagez des pull-up plus fortes (2.2kΩ au lieu de 4.7kΩ)
  ou un buffer de bus I2C.
- L'architecture logicielle sépare volontairement les intervalles de lecture capteur (2s),
  rafraîchissement écran (1s) et log série (5s) : c'est le modèle à suivre pour tout projet
  combiné robuste, en évitant tout `delay()` qui bloquerait les autres sous-systèmes.
