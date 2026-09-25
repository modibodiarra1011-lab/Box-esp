#!/usr/bin/env python3
"""Build all Arduino sketches and regenerate the SD staging tree."""
from __future__ import annotations
import argparse, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
def run(cmd):
    print("$", " ".join(map(str, cmd)))
    return subprocess.call([str(x) for x in cmd])

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--projects",type=Path,default=Path("projects"))
    ap.add_argument("--output",type=Path,default=Path("BUILD_OUTPUT"))
    ap.add_argument("--sd",type=Path,default=Path("SD_READY"))
    args=ap.parse_args()
    r=run([sys.executable, ROOT/"scripts/build_ino.py", args.projects, "--output", args.output])
    if r: return r
    return run([sys.executable, ROOT/"scripts/prepare_sd.py", "--build", args.output, "--projects", args.projects, "--sd", args.sd])
if __name__=="__main__": raise SystemExit(main())
