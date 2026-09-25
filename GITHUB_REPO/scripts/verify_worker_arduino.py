#!/usr/bin/env python3
"""Static contract checks for the Arduino-ESP32 WORKER build.

These checks intentionally target the user's Arduino IDE 3.3.12 environment:
- reject the stale OTA_TIMEOUT_MS symbol that caused the reported build error;
- require every configuration timeout/limit referenced by the worker;
- keep the duplicated worker source/config/platform files identical;
- verify the API/feature contract expected by the MASTER;
- verify the exact CI version pin is present.
"""
from __future__ import annotations
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
WORKER = ROOT / "WORKER" / "Arduino"
MIRROR = ROOT / "firmware" / "worker" / "Arduino"


def fail(msg: str) -> None:
    print("FAIL:", msg)
    raise SystemExit(1)


source = (WORKER / "worker.ino").read_text(encoding="utf-8")
config = (WORKER / "config.h").read_text(encoding="utf-8")
pio = (WORKER / "platformio.ini").read_text(encoding="utf-8")

if "OTA_TIMEOUT_MS" in source or "OTA_TIMEOUT_MS" in config:
    fail("stale OTA_TIMEOUT_MS symbol remains")
if "OTA_IDLE_TIMEOUT_MS" not in source or not re.search(r"#define\s+OTA_IDLE_TIMEOUT_MS\s+", config):
    fail("OTA_IDLE_TIMEOUT_MS contract is incomplete")
for macro in ("JOB_TIMEOUT_MS", "OTA_HTTP_TIMEOUT_MS", "MAX_UPLOAD_BYTES", "FS_TEST_BYTES", "BENCHMARK_OPERATIONS"):
    if not re.search(rf"#define\s+{macro}\s+", config):
        fail(f"missing config macro: {macro}")

required_routes = [
    'server.on("/api/info", HTTP_GET, handleInfo);',
    'server.on("/api/capabilities", HTTP_GET, handleCapabilities);',
    'server.on("/api/job", HTTP_POST, handleJob);',
    'server.on("/api/cancel", HTTP_POST, handleCancel);',
    'server.on("/api/flash", HTTP_POST, handleFlash);',
    'server.on("/api/reboot", HTTP_POST, handleReboot);',
    'server.on("/api/scan", HTTP_GET, handleScan);',
]
for route in required_routes:
    if route not in source:
        fail(f"missing worker route: {route}")

for feature in ("PING", "SYSTEM_TEST", "CHECKUP", "BENCHMARK", "FS_TEST", "OTA", "HEARTBEAT", "MEMORY", "RESET_REASON", "MDNS", "CANCEL", "TELEMETRY"):
    if f'"{feature}"' not in source and f'\\"{feature}\\"' not in source:
        fail(f"missing capability: {feature}")

if "3.3.12" not in pio or "5.5.5" not in pio:
    fail("PlatformIO pin must match Arduino-ESP32 3.3.12 / ESP-IDF 5.5.5")
if "esp32dev" not in pio:
    fail("PlatformIO board pin missing")
if "55.03.312" not in pio or "esp32-core-3.3.12.tar.xz" not in pio:
    fail("PlatformIO cloud pin missing")

for name in ("worker.ino", "config.h", "platformio.ini"):
    a = WORKER / name
    b = MIRROR / name
    if not a.exists() or not b.exists():
        fail(f"missing mirrored file: {name}")
    if a.read_bytes() != b.read_bytes():
        fail(f"worker mirror diverges: {name}")

print("Worker Arduino contract checks: PASS")
