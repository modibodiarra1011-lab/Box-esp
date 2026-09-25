# 17 - RC522 (lecteur RFID 13.56MHz)

## But
Détecter et lire l'identifiant unique (UID) de badges/cartes RFID/NFC au
format Mifare, via le bus SPI.

## Matériel
- 1x module RC522
- 1x carte ESP32 DevKit
- 1x badge/carte RFID Mifare (fourni généralement avec le module)

## Broches
| Signal RC522 | ESP32 (GPIO) |
|--------------|--------------|
| SDA (=SS)    | GPIO5 |
| SCK          | GPIO18 |
| MOSI         | GPIO23 |
| MISO         | GPIO19 |
| RST          | GPIO27 |
| VCC          | **3V3 (impératif, voir avertissement)** |
| GND          | GND |

## Câblage
GPIO18 (SCK)  --- SCK
GPIO19 (MISO) --- MISO
GPIO23 (MOSI) --- MOSI
GPIO5  (SS)   --- SDA
GPIO27 (RST)  --- RST
3V3           --- VCC
GND           --- GND

## Avertissements tension / niveaux logiques
- **Le RC522 est un module 3.3V STRICT** : contrairement à beaucoup de modules SPI/I2C, il
  n'a **pas** de régulateur 5V→3.3V à bord sur la plupart des breakouts vendus. Alimenter le
  RC522 en 5V le **détruit définitivement**. C'est en réalité un avantage pour l'ESP32 (contrairement
  à l'Arduino Uno qui doit ajouter un régulateur/diviseur externe pour l'utiliser en toute sécurité).
- Vérifiez systématiquement avec un multimètre que votre alimentation VCC est bien à 3.3V avant
  la première mise sous tension.
- Le bus SPI de l'ESP32 (VSPI par défaut : SCK18/MISO19/MOSI23) peut être partagé avec d'autres
  périphériques SPI (ex: ILI9341, MAX6675) à condition d'utiliser une broche SS (Slave Select)
  distincte pour chaque module — voir le projet combiné 32_domotique_rfid_wifi pour un exemple
  de partage de bus SPI géré proprement.
