#!/usr/bin/env python3
"""Analyze GPIO/protocol references in project manifests and wiring/source files."""
from __future__ import annotations
from pathlib import Path
import argparse, json, re

GPIO_PATTERNS = [r"GPIO[_ ]?(\d{1,2})", r"pinMode\s*\(\s*(\d{1,2})", r"digitalWrite\s*\(\s*(\d{1,2})", r"analogRead\s*\(\s*(\d{1,2})"]

def analyze(project: Path) -> dict:
    meta = json.loads((project / "project.json").read_text(encoding="utf-8"))
    text = ""
    for f in project.rglob("*"):
        if f.is_file() and f.suffix.lower() in {".ino", ".c", ".cpp", ".h", ".md", ".json"}:
            text += f.read_text(encoding="utf-8", errors="ignore") + "\n"
    pins = sorted({int(m) for pat in GPIO_PATTERNS for m in re.findall(pat, text, re.I)})
    low = text.lower()
    return {
        "project": str(project),
        "board": meta.get("board", meta.get("fqbn", "")),
        "gpio_mentions": pins,
        "i2c": "i2c" in low, "spi": "spi" in low, "i2s": "i2s" in low,
        "uart": "serial" in low or "uart" in low, "wifi": "wifi" in low,
        "ble": "ble" in low or "bluetooth" in low, "usb": "usb" in low,
        "pwm": "pwm" in low, "adc": "adc" in low,
    }

def main() -> int:
    ap = argparse.ArgumentParser(); ap.add_argument("root", type=Path); ap.add_argument("--out", type=Path, default=Path("catalog/DATABASE/range_analysis.json")); a = ap.parse_args()
    rows = [analyze(p) for p in sorted({p.parent for p in a.root.rglob("project.json")})]
    a.out.parent.mkdir(parents=True, exist_ok=True); a.out.write_text(json.dumps(rows, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"Analyzed {len(rows)} projects")
    return 0
if __name__ == "__main__": raise SystemExit(main())
