#!/usr/bin/env python3
"""Prépare une carte SD offline-first à partir des builds Arduino et des projets."""
from __future__ import annotations
import argparse, hashlib, json, shutil
from pathlib import Path

def sha(p: Path) -> str:
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()

def safe_name(s: str) -> str:
    return ''.join(c if c.isalnum() or c in '-_.' else '_' for c in s)

def copy_tree(src: Path, dst: Path) -> None:
    if not src.exists(): return
    for item in sorted(src.iterdir()):
        target=dst/item.name
        if item.is_dir(): shutil.copytree(item,target,dirs_exist_ok=True)
        elif item.is_file(): shutil.copy2(item,target)

def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--build',type=Path,default=Path('BUILD_OUTPUT'))
    ap.add_argument('--sd',type=Path,default=Path('SD_READY'))
    ap.add_argument('--projects',type=Path,default=Path('projects'))
    args=ap.parse_args()
    if args.sd.exists(): shutil.rmtree(args.sd)
    dirs=['FIRMWARE/MASTER','FIRMWARE/WORKER','FIRMWARE/ESP32','FIRMWARE/ESP32S3','FIRMWARE/AVR/UNO','FIRMWARE/AVR/NANO','PROJECTS/PREPARED','PROJECTS/IMPORTED_35','PROJECTS/MY_PROJECTS','COMPONENTS','LIBRARIES','TESTS','REPORTS','LOGS','BACKUPS','CONFIG','DATABASE','UPDATES','AI']
    for d in dirs: (args.sd/d).mkdir(parents=True,exist_ok=True)

    manifest=[]
    build_manifest=args.build/'build_manifest.json'
    if build_manifest.exists():
        try: manifest=json.loads(build_manifest.read_text(encoding='utf-8'))
        except Exception: manifest=[]
    for p in sorted(args.build.rglob('*')):
        if not p.is_file() or p.suffix.lower() not in {'.bin','.hex'}: continue
        info=None
        stem=p.name.rsplit('.',1)[0]
        side=p.with_suffix('.json')
        if side.exists():
            try: info=json.loads(side.read_text(encoding='utf-8'))
            except Exception: info=None
        fqbn=str((info or {}).get('fqbn','')).lower()
        name=p.name.upper()
        if p.suffix.lower()=='.hex' or fqbn.startswith('arduino:avr:') or 'UNO' in name or 'NANO' in name:
            dest=args.sd/'FIRMWARE'/'AVR'/('NANO' if 'NANO' in name or ':nano' in fqbn else 'UNO')
        elif 'esp32s3' in fqbn or 'ESP32S3' in name:
            dest=args.sd/'FIRMWARE'/'ESP32S3'
        else:
            dest=args.sd/'FIRMWARE'/'ESP32'
        dest.mkdir(parents=True,exist_ok=True); shutil.copy2(p,dest/safe_name(p.name))
        if side.exists(): shutil.copy2(side,dest/safe_name(side.name))

    for src_rel,dst_rel in [('PREPARED','PROJECTS/PREPARED'),('IMPORTED_35','PROJECTS/IMPORTED_35')]:
        copy_tree(args.projects/src_rel,args.sd/dst_rel)
    (args.sd/'DATABASE/project_index.json').write_text(json.dumps([
        {'path':str(p.parent),'name':p.parent.name,'board':json.loads(p.read_text(encoding='utf-8')).get('board',json.loads(p.read_text(encoding='utf-8')).get('fqbn',''))}
        for p in sorted(args.projects.rglob('project.json'))
    ],indent=2,ensure_ascii=False),encoding='utf-8')
    info=[]
    for p in sorted(args.sd.rglob('*')):
        if p.is_file(): info.append({'path':str(p.relative_to(args.sd)).replace('\\','/'),'size':p.stat().st_size,'sha256':sha(p)})
    (args.sd/'DATABASE/sd_manifest.json').write_text(json.dumps({'format':'ESP32 LAB SD 1','files':info},indent=2,ensure_ascii=False),encoding='utf-8')
    print(f'SD_READY créée : {len(info)} fichiers')
    return 0
if __name__=='__main__': raise SystemExit(main())
