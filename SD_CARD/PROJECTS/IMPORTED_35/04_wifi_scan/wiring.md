# 04 - Scan WiFi

## But
Utiliser le module radio intégré de l'ESP32 pour détecter les points d'accès
WiFi environnants (SSID, puissance du signal RSSI, canal, type de sécurité).

## Matériel
- 1x carte ESP32 DevKit (WiFi 802.11 b/g/n intégré)
- Aucun composant externe nécessaire

## Broches
Aucun câblage requis : le module WiFi est interne au SoC ESP32 (antenne PCB ou
connecteur IPEX selon le modèle de carte).

## Avertissements tension / niveaux logiques
- N'est **pas** applicable à un Arduino Uno standard (pas de WiFi natif) ; nécessiterait
  un module externe type ESP8266 ou WiFi shield avec sa propre logique 3.3V.
- L'antenne WiFi consomme des pics de courant pouvant atteindre 300-500 mA : une alimentation
  USB de mauvaise qualité ou un câble trop fin peut causer des redémarrages intempestifs (brownout).
- Éviter de placer la carte contre du métal ou dans un boîtier métallique fermé qui atténuerait
  fortement le signal WiFi.
