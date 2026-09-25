# 22 - Point d'accès WiFi + serveur web

## But
Créer un réseau WiFi autonome (Access Point) directement depuis l'ESP32 et
servir une page web permettant de piloter une LED, sans nécessiter de routeur.

## Matériel
- 1x carte ESP32 DevKit (WiFi intégré)
- Aucun composant externe (utilise LED_BUILTIN)

## Broches
| Signal | ESP32 (GPIO) |
|--------|--------------|
| LED    | LED_BUILTIN |

## Câblage
Aucun câblage externe nécessaire.

## Utilisation
1. Flashez le sketch, ouvrez le moniteur série pour voir l'adresse IP de l'AP.
2. Sur votre téléphone/PC, connectez-vous au WiFi `ESP32_LAB_AP` (mot de passe `esp32lab123`).
3. Ouvrez un navigateur sur `http://192.168.4.1/` pour accéder à la page de contrôle.

## Avertissements tension / niveaux logiques
- **Non applicable** à un Arduino Uno standard (pas de WiFi natif).
- Un mot de passe WPA2 doit faire **au moins 8 caractères** ; en dessous, `softAP()` échouera
  silencieusement ou créera un réseau ouvert selon la version du core.
- Le mode Access Point consomme davantage de courant que le mode Station : privilégiez une
  alimentation USB de qualité (500mA minimum) pour éviter les redémarrages (brownout).
- Changez le mot de passe par défaut de ce template avant tout déploiement réel.
