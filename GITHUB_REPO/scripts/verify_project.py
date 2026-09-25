#!/usr/bin/env python3
"""Repository integrity checks; no hardware result is inferred."""
from __future__ import annotations
from pathlib import Path
import ast, hashlib, json, re, sys

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = [
    "firmware/master/CMakeLists.txt", "firmware/master/main/idf_component.yml",
    "firmware/master/main/app_main.c", "firmware/master/main/web_ui.cpp",
    "firmware/master/main/web_admin.cpp", "firmware/master/partitions_16mb_ota.csv",
    "scripts/build_ino.py", "scripts/prepare_sd.py", "scripts/range_analyzer.py",
    ".github/workflows/ci.yml"
]

def fail(msg): print("FAIL:", msg); return 1

for rel in REQUIRED:
    p = ROOT / rel
    if not p.exists() or not p.is_file() or p.stat().st_size == 0: sys.exit(fail(f"missing/empty {rel}"))
for p in ROOT.rglob("*.py"):
    if "__pycache__" not in p.parts: ast.parse(p.read_text(encoding="utf-8"))
for p in ROOT.rglob("*.json"): json.loads(p.read_text(encoding="utf-8"))
for p in ROOT.glob(".github/workflows/*.yml"):
    if "name:" not in p.read_text(encoding="utf-8"): sys.exit(fail(f"workflow metadata missing: {p}"))
for p in ROOT.rglob("*.c"):
    t=p.read_text(encoding="utf-8",errors="ignore")
    if re.search(r'(?<![A-Za-z0-9_])R\"', t) or '[](' in t: sys.exit(fail(f"C++ construct in C source: {p}"))
    if "USBHost.h" in t: sys.exit(fail(f"legacy Arduino USBHost include remains: {p}"))
for p in ROOT.rglob("*.cpp"):
    if "R\"HTML(" in p.read_text(encoding="utf-8",errors="ignore"): pass
secret_patterns = [r"(?:api[_-]?key|apikey)\s*[:=]\s*[A-Za-z0-9_-]{12,}", r"sk-[A-Za-z0-9]{16,}", r"Authorization:\s*Bearer\s+[A-Za-z0-9._-]{16,}"]
for p in ROOT.rglob("*"):
    if not p.is_file() or ".git" in p.parts or p.suffix.lower() in {".bin", ".hex"}: continue
    if "projects" in p.parts and "IMPORTED_35" in p.parts: continue
    if "SD_CARD" in p.parts and "PROJECTS" in p.parts: continue
    text=p.read_text(encoding="utf-8",errors="ignore")
    if p.name in {"local_secrets.example.json", "lab.example.json", "verify_project.py"}: continue
    for pat in secret_patterns:
        if re.search(pat, text, re.I):
            sys.exit(fail(f"possible secret in {p}: {pat}"))
parts = (ROOT / "firmware/master/partitions_16mb_ota.csv").read_text(encoding="utf-8").splitlines()
assert parts and any("ota_0" in x for x in parts) and any("ota_1" in x for x in parts)
print("Repository integrity checks: PASS")
