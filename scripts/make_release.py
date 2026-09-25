#!/usr/bin/env python3
"""Create an OTA manifest compatible with ESP32 LAB. No secrets are embedded."""
from __future__ import annotations
import argparse, hashlib, json, datetime
from pathlib import Path

def sha(p: Path) -> str:
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('artifacts',type=Path)
    ap.add_argument('--version',required=True)
    ap.add_argument('--base-url',required=True)
    ap.add_argument('--notes',default='')
    ap.add_argument('--out',type=Path,default=Path('release-manifest.json'))
    args=ap.parse_args()
    bins=sorted(p for p in args.artifacts.rglob('*.bin') if p.is_file())
    master=next((p for p in bins if 'MASTER' in p.name.upper()), None)
    if master is None:
        raise SystemExit('Aucun binaire MASTER trouve dans les artefacts')
    doc={
      'version':args.version,
      'generated_at':datetime.datetime.now(datetime.timezone.utc).isoformat(),
      'notes':args.notes,
      'master_url':args.base_url.rstrip('/')+'/'+master.name,
      'master_sha256':sha(master),
      'artifacts':[{'name':p.name,'size':p.stat().st_size,'sha256':sha(p),'url':args.base_url.rstrip('/')+'/'+p.name} for p in bins],
    }
    args.out.write_text(json.dumps(doc,indent=2,ensure_ascii=False),encoding='utf-8')
    print(f'Manifest OTA: {args.out} ({len(bins)} .bin)')
if __name__=='__main__': raise SystemExit(main())
