#!/usr/bin/env python3
"""Synchronise le miroir GITHUB_REPO avec le contenu publiable du dépôt."""
from pathlib import Path
import shutil
ROOT=Path(__file__).resolve().parents[1]
DST=ROOT/'GITHUB_REPO'
KEEP=['firmware','WORKER','projects','catalog','scripts','docs','PROJECT_BUILDER','.github','CONFIG','README.md','SECURITY.md','PORTFOLIO.md','LICENSE','.gitignore','.gitattributes','.editorconfig']
if DST.exists(): shutil.rmtree(DST)
DST.mkdir(parents=True)
for rel in KEEP:
    src=ROOT/rel; dst=DST/rel
    if src.is_dir(): shutil.copytree(src,dst)
    elif src.exists(): dst.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(src,dst)
print('GITHUB_REPO synchronisé')
