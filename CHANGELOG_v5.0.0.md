# ESP32 LAB FUTURE CONTROL v5.0.0

## Fix critique

- Suppression du dernier appel actif à `OTA_TIMEOUT_MS`.
- Utilisation cohérente de `OTA_IDLE_TIMEOUT_MS` et `OTA_HTTP_TIMEOUT_MS`.
- Miroirs Worker maintenus byte-for-byte identiques.

## Upgrade moteur

- Worker : jobs non bloquants et annulables.
- Worker : benchmark CPU réel basé sur une boucle d'opérations mesurée.
- Worker : test LittleFS avec écriture, lecture et vérification.
- Worker : compteur de boot, reset reason, heap minimum, max allocation, stack libre, PSRAM et flash.
- Worker : radar Wi-Fi asynchrone.
- Worker : mDNS et tableau de bord local.
- Worker : OTA contrôlée par taille, timeout, SHA-256 et finalisation avant reboot.
- MASTER : scheduler parallèle, retries, priorité, timeout et STOP ALL.
- MASTER : reboot individuel de Worker.
- MASTER UI : mode clair/sombre, gauges, graphique, palette de commandes, BENCH FLEET, STRESS ×9, mission.

## Validation

- Contrôles statiques du projet : PASS.
- Contrat Arduino Worker : PASS.
- 100000 invariants déterministes : PASS.
- 60 passes déterministes release : PASS.
- Analyse de 41 projets : PASS.
- JavaScript embarqué : syntaxe valide.
- C++ Worker : syntaxe validée avec un harness de stubs hors toolchain matérielle.

## Transparence

Aucune compilation réelle Arduino/ESP-IDF n'a été exécutée dans cet environnement. Les workflows cloud sont inclus pour effectuer la validation avec les toolchains exactes sur GitHub Actions.
