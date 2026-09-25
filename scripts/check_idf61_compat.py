#!/usr/bin/env python3
"""Contrôle les migrations ESP-IDF 6.x utiles à ESP32 LAB."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
MAIN=ROOT/'firmware/master/main'

def ordered_freertos(p: Path) -> bool:
    s=p.read_text(encoding='utf-8')
    a=s.find('#include "freertos/FreeRTOS.h"')
    b=s.find('#include "freertos/semphr.h"')
    return a!=-1 and b!=-1 and a<b

cm=(MAIN/'CMakeLists.txt').read_text(encoding='utf-8')
checks=[
 ('pas de dependance json legacy', not re.search(r'(^|\n)\s*json\s*(\n|$)', cm)),
 ('cjson declare', 'cjson' in cm),
 ('esp_timer declare', 'esp_timer' in cm),
 ('esp_driver_sdspi declare', 'esp_driver_sdspi' in cm),
 ('usb_host_cdc_acm declare', 'usb_host_cdc_acm' in cm),
 ('esp_random.h dans lab_config', '#include "esp_random.h"' in (MAIN/'lab_config.c').read_text(encoding='utf-8')),
 ('driver/sdspi_host.h dans storage', '#include "driver/sdspi_host.h"' in (MAIN/'storage.c').read_text(encoding='utf-8')),
 ('FreeRTOS avant semphr', ordered_freertos(MAIN/'job_engine.c')),
 ('pas de symbole Kconfig obsolete', not re.search(r'CONFIG_(MBEDTLS_DEFAULT_CERTIFICATE_BUNDLE|BOOTLOADER_APP_ROLLBACK|ESP32S3_DEFAULT_CPU_FREQ_240)', (ROOT/'firmware/master/sdkconfig.defaults').read_text(encoding='utf-8'))),
]
for name,ok in checks: print(('PASS' if ok else 'FAIL'), name)
raise SystemExit(0 if all(ok for _,ok in checks) else 2)
