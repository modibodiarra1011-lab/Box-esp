# USB Host / AVR

## Connexion physique recommandée

Pour la première utilisation, éviter de fabriquer un câble directement soudé au connecteur USB du S3. Utiliser :

```text
ESP32-S3 USB-C OTG
        ↓
adaptateur/hub OTG alimenté
        ↓
USB-A
        ↓
câble adapté à la carte cible
        ↓
Arduino Uno / autre cible CDC
```

## Signaux ESP32-S3

```text
GPIO19 → USB D-
GPIO20 → USB D+
GND    → GND
VBUS   → alimentation 5 V hôte protégée
```

Le S3 possède un contrôleur USB-OTG et un contrôleur USB-Serial-JTAG partageant le PHY. Le design ne suppose donc pas deux contrôleurs USB indépendants utilisables simultanément avec le même PHY.

## VBUS

Le projet prévoit `USB_HOST_VBUS_EN_GPIO = -1` par défaut, ce qui signifie que le VBUS est fourni par l'alimentation hôte externe. Si un montage matériel possède un interrupteur high-side/limiteur de courant pilotable, le GPIO peut être renseigné dans la configuration et le firmware activera la sortie au démarrage.

**Ne jamais alimenter le VBUS USB depuis une GPIO ESP32.**

## Noyau AVR actuel

Le firmware contient un programmeur série basé sur le bootloader de type STK500v1 pour :

- ATmega328P Optiboot ;
- ATmega168 STK500.

Le programme :

```text
parse Intel HEX
→ valider checksum
→ synchroniser
→ entrer en programmation
→ lire signature
→ charger l'adresse
→ programmer pages
→ relire pages
→ comparer
→ quitter le mode programmation
```

Le reset automatique via DTR/RTS dépend du câble/interface USB-série de la cible. Toutes les cartes AVR ne se comportent pas de façon identique.

## Exemple UNO

Arduino IDE :

```text
Sketch → Export Compiled Binary
```

Puis copier le `.hex` dans :

```text
SD:/FIRMWARE/AVR/UNO/
```

Le panneau privé peut lancer le programmeur lorsqu'un périphérique USB CDC compatible est présent.
