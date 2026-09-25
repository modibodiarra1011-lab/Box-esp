# 34 - Data logger environnemental (DHT22 + BH1750 + DS3231)

## But
Échantillonner périodiquement température, humidité et luminosité, avec un
horodatage précis fourni par une horloge temps réel, et produire un flux CSV
exploitable (redirigeable vers une carte SD ou un service cloud dans une
évolution future du projet).

## Matériel
- 1x DHT22
- 1x BH1750 (I2C)
- 1x DS3231 (I2C)
- 1x résistance 10kΩ (pull-up DHT22, si absente du module)
- 1x carte ESP32 DevKit

## Broches
| Module  | Signal | ESP32 (GPIO) |
|---------|--------|--------------|
| DHT22   | DATA   | GPIO4 |
| BH1750  | SDA    | GPIO21 |
| BH1750  | SCL    | GPIO22 |
| DS3231  | SDA    | GPIO21 |
| DS3231  | SCL    | GPIO22 |

## Câblage
3V3 --- VCC (DHT22 via pull-up 10kΩ sur DATA), VCC (BH1750), VCC (DS3231)
GND --- GND (commun aux trois modules)
GPIO4  --- DATA (DHT22)
GPIO21 --- SDA (BH1750 + DS3231, bus partagé)
GPIO22 --- SCL (BH1750 + DS3231, bus partagé)

## Gestion des adresses I2C (vérifiée sans conflit)
| Module | Adresse I2C |
|--------|-------------|
| BH1750 | 0x23 |
| DS3231 | 0x68 |

Adresses distinctes : aucun conflit. Le DHT22 n'utilise pas le bus I2C (protocole
propriétaire un-fil), il ne peut donc jamais entrer en conflit avec les modules I2C.

## Avertissements tension / niveaux logiques
- Alimentez les trois modules en **3.3V uniquement** pour rester cohérent avec les niveaux
  logiques ESP32 sur DATA/SDA/SCL.
- Le format CSV imprimé sur le port série (`id,horodatage,temperature_c,humidite_pct,luminosite_lux`)
  est directement compatible avec un import Excel/Google Sheets ou une redirection vers un
  fichier via un script sur PC (ex: `python -m serial.tools.miniterm > log.csv`).
- L'intervalle d'échantillonnage (3s) respecte la contrainte physique du DHT22 (minimum 2s
  entre deux lectures) : ne descendez pas en dessous de 2.5s pour ce capteur.
- En cas d'erreur de lecture DHT22, la ligne CSV contient `ERR` dans les colonnes concernées
  plutôt que de bloquer ou de fausser silencieusement les données.
