#!/usr/bin/env python3
from pathlib import Path
import argparse,hashlib,json,zipfile

def sha(p):
 h=hashlib.sha256();
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--version',required=True);ap.add_argument('--bin',type=Path,required=True);ap.add_argument('--url',required=True);ap.add_argument('--out',type=Path,default=Path('release'));a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=True);dst=a.out/f'ESP32_LAB_MASTER_v{a.version}.bin';dst.write_bytes(a.bin.read_bytes());manifest={'version':a.version,'target':'esp32-lab-master','master_url':a.url,'master_sha256':sha(dst),'notes':'Generated release'};(a.out/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8');print(json.dumps(manifest,indent=2))
if __name__=='__main__':main()
