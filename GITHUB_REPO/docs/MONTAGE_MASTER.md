# Montage MASTER

## Réseau logique

```text
Téléphone
   │ Wi-Fi
   ▼
ESP32-S3 MASTER
 ├── microSD SPI
 ├── DHT11
 ├── RGB
 └── USB OTG Host
        │
        └── cible USB/AVR
```

## DHT11

```text
DHT11 VCC  → 3V3
DHT11 DATA → GPIO4
DHT11 GND  → GND
```

Module 3 broches : la résistance pull-up est souvent déjà présente. Capteur nu : prévoir la pull-up selon la fiche technique du composant.

## SD SPI

```text
SD VCC  → 3V3
SD GND  → GND
SD CS   → GPIO10
SD SCK  → GPIO12
SD MISO → GPIO13
SD MOSI → GPIO11
```

Pour la carte 64 GB, préparer une partition FAT32 pour la configuration actuelle du projet.

## RGB DevKitC-1

```text
v1.0 → GPIO48
v1.1 → GPIO38
```

## Alimentation

Les moteurs/servos externes doivent avoir leur propre alimentation adaptée. Ne jamais connecter un moteur ou un servo puissant directement à une GPIO du S3.
