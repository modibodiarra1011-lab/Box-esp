# ESP-IDF 6.1 — corrections importantes utilisées dans ESP32 LAB

## 1. JSON

Ne pas utiliser `json` comme composant. Le projet utilise `espressif/cjson`.

## 2. RNG

Inclure explicitement :

```c
#include "esp_random.h"
```

## 3. SDSPI

Utiliser :

```c
#include "driver/sdspi_host.h"
```

Et déclarer `esp_driver_sdspi` dans `REQUIRES`.

## 4. Timer

Les fichiers qui utilisent `esp_timer.h` déclarent `esp_timer` comme dépendance.

## 5. FreeRTOS

`FreeRTOS.h` doit être inclus avant `semphr.h`, `task.h` ou `queue.h`.

## 6. Kconfig

Les symboles obsolètes rencontrés sur la première archive ne sont plus présents dans `sdkconfig.defaults`.

Le dépôt fournit `scripts/check_idf61_compat.py` pour détecter ces régressions avant compilation.
