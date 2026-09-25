# 35 - Système d'alarme de sécurité (HC-SR04 + WS2812 + Buzzer + WiFi)

## But
Détecter une intrusion par mesure de distance ultrason, déclencher une alerte
visuelle (LED clignotante rouge) et sonore (buzzer), avec un tableau de bord
web consultable à distance. Séquence d'armement temporisée pour éviter les
fausses alertes au démarrage.

## Matériel
- 1x HC-SR04
- 1x LED WS2812 (ou anneau/bandeau, ici 1 seule LED utilisée comme indicateur)
- 1x buzzer actif (2 fils, pas besoin de fréquence PWM précise)
- 1x carte ESP32 DevKit

## Broches
| Module   | Signal | ESP32 (GPIO) |
|----------|--------|--------------|
| HC-SR04  | TRIG   | GPIO5 |
| HC-SR04  | ECHO   | GPIO18 (via diviseur 1kΩ/2kΩ) |
| WS2812   | DIN    | GPIO15 (via résistance série 300-500Ω recommandée) |
| Buzzer   | +      | GPIO26 |
| Buzzer   | -      | GND |

## Câblage
5V --- VCC (HC-SR04)
5V (ou 3.3V selon modèle) --- VCC (WS2812)
GND (commun) --- GND (HC-SR04, WS2812, buzzer)
GPIO5  --- TRIG
GPIO18 --- diviseur de tension --- ECHO
GPIO15 --- résistance série --- DIN (WS2812)
GPIO26 --- + (buzzer actif)

## Séquence de fonctionnement
1. **ARMEMENT** (5s, LED jaune fixe) : temps de s'éloigner du capteur après la mise sous tension.
2. **ARMÉ / SURVEILLANCE** (LED verte fixe) : le système mesure en continu la distance.
3. **ALARME DÉCLENCHÉE** (LED rouge clignotante + buzzer actif) : si un obstacle est détecté à
   moins de 30cm, jusqu'à ce que la zone soit dégagée, puis retour automatique à l'état armé.

## Avertissements tension / niveaux logiques
- **Diviseur de tension obligatoire sur ECHO** (le HC-SR04 est un capteur 5V, voir projet 10).
- Le buzzer actif utilisé ici se pilote en simple HIGH/LOW (il génère son propre son en interne) ;
  **ne confondez pas avec un buzzer passif**, qui nécessiterait un signal PWM à une fréquence
  audible précise (~2-4 kHz) pour émettre un son.
- La LED WS2812 suit les mêmes recommandations que le projet 18 (résistance série sur DATA,
  éventuellement un level-shifter pour un montage de production robuste).
- Ce système est un **prototype pédagogique** : pour un usage réel de sécurité, prévoyez une
  alimentation de secours (batterie), une notification hors-ligne (SMS/GSM) en plus du WiFi
  local, et un boîtier anti-sabotage.
