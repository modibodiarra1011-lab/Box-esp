# 33 - Robot d'évitement d'obstacles (HC-SR04 + 2 servos)

## But
Implémenter une logique de navigation autonome : un servo oriente un capteur
ultrason pour scanner trois directions (centre, gauche, droite), puis un
second servo (représentant la direction) réagit à l'obstacle détecté. Le tout
sans aucun `delay()` bloquant, via une machine à états pilotée par `millis()`.

## Matériel
- 1x HC-SR04 (voir avertissement diviseur de tension)
- 2x servomoteurs SG90
- 1x carte ESP32 DevKit
- Alimentation externe 5V pour les servos

## Broches
| Module              | Signal | ESP32 (GPIO) |
|---------------------|--------|--------------|
| HC-SR04             | TRIG   | GPIO5 |
| HC-SR04             | ECHO   | GPIO18 (via diviseur 1kΩ/2kΩ, voir projet 10) |
| Servo "tête" (scan) | Signal | GPIO16 |
| Servo "direction"   | Signal | GPIO17 |

## Câblage
5V externe --- VCC (HC-SR04), VCC (les 2 servos)
GND (commun)--- GND (HC-SR04), GND (les 2 servos), GND (ESP32)
GPIO5  --- TRIG (HC-SR04)
GPIO18 --- diviseur de tension --- ECHO (HC-SR04)
GPIO16 --- Signal servo tête
GPIO17 --- Signal servo direction

## Gestion des conflits de broches
- Les deux servos utilisent des timers PWM matériels distincts (`ESP32PWM::allocateTimer(0)`
  et `allocateTimer(1)`) pour éviter toute interférence de fréquence entre eux.
- Le capteur ultrason (TRIG/ECHO) n'utilise pas de PWM et ne rentre donc pas en conflit avec
  les timers des servos.

## Avertissements tension / niveaux logiques
- **Diviseur de tension obligatoire sur ECHO** (voir projet 10_hcsr04) : sans lui, risque de
  dommage sur GPIO18.
- **Alimentation externe 5V impérative pour les deux servos** (masse commune avec l'ESP32) :
  le mouvement simultané de deux servos peut consommer plusieurs centaines de mA en pointe,
  largement au-delà de ce que la broche 3V3/5V de l'ESP32 peut fournir en toute sécurité.
- Le délai `SERVO_SETTLE_MS` (300ms) laisse le temps au servo d'atteindre physiquement sa
  position avant de déclencher une mesure ultrason : le réduire trop provoquerait des mesures
  prises pendant que le capteur est encore en mouvement (bruit de mesure).
- Pour un robot à roues réel, remplacez `steeringServo` par un driver de moteurs (L298N, TB6612)
  piloté en PWM avec la même logique non bloquante — ne jamais alimenter des moteurs DC
  directement depuis un GPIO ESP32.
