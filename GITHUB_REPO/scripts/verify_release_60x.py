#!/usr/bin/env python3
"""Repeat deterministic release invariants 60 times.

This is intentionally a structural check, not a substitute for a real
Arduino/ESP-IDF compiler or hardware test.
"""
from __future__ import annotations
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
WORKER = ROOT / "WORKER" / "Arduino"
MIRROR = ROOT / "firmware" / "worker" / "Arduino"
EXPECTED = [
    WORKER / "worker.ino",
    WORKER / "config.h",
    WORKER / "platformio.ini",
    ROOT / "firmware/master/main/web_ui.cpp",
    ROOT / "firmware/master/main/web_server.c",
]

def digest(p: Path) -> str:
    h = hashlib.sha256(); h.update(p.read_bytes()); return h.hexdigest()

for run in range(1, 61):
    for p in EXPECTED:
        if not p.is_file() or p.stat().st_size == 0:
            raise SystemExit(f"FAIL run={run}: missing/empty {p}")
    source = (WORKER / "worker.ino").read_text(encoding="utf-8")
    config = (WORKER / "config.h").read_text(encoding="utf-8")
    pio = (WORKER / "platformio.ini").read_text(encoding="utf-8")
    if "OTA_TIMEOUT_MS" in source or "OTA_TIMEOUT_MS" in config:
        raise SystemExit(f"FAIL run={run}: stale OTA_TIMEOUT_MS")
    for macro in ("OTA_IDLE_TIMEOUT_MS", "OTA_HTTP_TIMEOUT_MS", "JOB_TIMEOUT_MS", "MAX_UPLOAD_BYTES", "FS_TEST_BYTES", "BENCHMARK_OPERATIONS"):
        if f"#define {macro} " not in config:
            raise SystemExit(f"FAIL run={run}: missing {macro}")
    if "3.3.12" not in pio or "5.5.5" not in pio or "esp32dev" not in pio:
        raise SystemExit(f"FAIL run={run}: version/board pin drift")
    if "api/scan" not in source or "BENCHMARK" not in source or "MDNS" not in source:
        raise SystemExit(f"FAIL run={run}: feature drift")
    for name in ("worker.ino", "config.h", "platformio.ini"):
        if digest(WORKER / name) != digest(MIRROR / name):
            raise SystemExit(f"FAIL run={run}: mirror drift {name}")
print("60 deterministic release verification passes: PASS")
