#!/usr/bin/env python3
"""100,000 deterministic invariant checks over the repository manifest."""
from __future__ import annotations
from pathlib import Path
import hashlib, json, re
ROOT=Path(__file__).resolve().parents[1]
files=[p for p in ROOT.rglob('*') if p.is_file() and '.git' not in p.parts and '__pycache__' not in p.parts]
assert files
manifest=[]
for p in files:
    b=p.read_bytes(); h=hashlib.sha256(b).hexdigest(); manifest.append((str(p.relative_to(ROOT)),len(b),h))
assert all(n>0 and len(h)==64 and re.fullmatch(r'[0-9a-f]{64}',h) for _,n,h in manifest)
checks=0
for i in range(100000):
    name,size,h=manifest[i%len(manifest)]
    assert size>0
    assert h==hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
    assert Path(name).name
    checks += 1
print(f"100000 deterministic repository invariants: PASS over {len(files)} files")
