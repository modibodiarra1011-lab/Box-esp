from pathlib import Path
import ast,json,re
ROOT=Path(__file__).resolve().parents[1]
for p in ROOT.rglob('*.py'):
    if '__pycache__' in p.parts: continue
    ast.parse(p.read_text(encoding='utf-8'))
for p in ROOT.rglob('*.json'):
    json.loads(p.read_text(encoding='utf-8'))
for p in ROOT.glob('.github/workflows/*.yml'):
    t=p.read_text(encoding='utf-8'); assert 'name:' in t and 'jobs:' in t
bad=[]
for p in ROOT.rglob('*'):
    if p.is_file() and p.suffix.lower() in ('.c','.h','.cpp'):
        t=p.read_text(encoding='utf-8',errors='ignore')
        if 'TODO' in t or 'FIXME' in t: bad.append(str(p))
assert not bad,bad
print('Static project checks: PASS')
