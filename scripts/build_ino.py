#!/usr/bin/env python3
"""Compile Arduino sketches recursively and stage .bin/.hex + SHA-256 metadata for ESP32 LAB SD."""
from __future__ import annotations
import argparse, hashlib, json, shutil, subprocess, tempfile
from pathlib import Path

EXTENSIONS = {".bin", ".hex"}
SKETCH_DIR_NAMES = {"main", "src"}

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()

def fqbn_for(meta: dict, default: str) -> str:
    if isinstance(meta.get("fqbn"), str) and meta["fqbn"]:
        return meta["fqbn"]
    board = str(meta.get("board", meta.get("board_family", ""))).lower()
    if "uno" in board or "atmega328p" in board:
        return "arduino:avr:uno"
    if "nano" in board and "328" in board:
        return "arduino:avr:nano"
    if "esp32s3" in board or "esp32-s3" in board:
        return "esp32:esp32:esp32s3"
    if "esp32" in board:
        return "esp32:esp32:esp32"
    return default

def family(fqbn: str) -> str:
    if fqbn.startswith("arduino:avr:"):
        return "AVR"
    if "esp32s3" in fqbn:
        return "ESP32S3"
    return "ESP32"

def sketch_roots(project: Path) -> list[Path]:
    roots = []
    for name in SKETCH_DIR_NAMES:
        p = project / name
        if p.is_dir():
            roots.append(p)
    if not roots:
        roots.append(project)
    return roots

def ino_files(project: Path) -> list[Path]:
    found = []
    for root in sketch_roots(project):
        found.extend(p for p in sorted(root.glob("*.ino")) if p.is_file())
    return sorted(found)

def copy_sketch_project(project: Path, ino: Path, work: Path) -> Path:
    work.mkdir(parents=True, exist_ok=True)
    source_root = ino.parent
    for item in source_root.iterdir():
        dest = work / item.name
        # A sketch folder may contain several .ino files. Compile one sketch at a time.
        if item.suffix.lower() == ".ino":
            if item.name == ino.name:
                continue
            continue
        if item.is_dir():
            shutil.copytree(item, dest, dirs_exist_ok=True)
        else:
            shutil.copy2(item, dest)
    # Copy common Arduino project assets alongside the sketch.
    for name in ("lib", "libraries", "data", "src", "include"):
        src = project / name
        if src.exists() and src.resolve() != source_root.resolve():
            dst = work / name
            if src.is_dir():
                shutil.copytree(src, dst, dirs_exist_ok=True)
    main = work / "main.ino"
    shutil.copy2(ino, main)
    return main

def compile_one(project: Path, ino: Path, out: Path, fqbn: str, cli: str) -> tuple[int, str, list[Path]]:
    with tempfile.TemporaryDirectory(prefix="esp32lab_ino_") as td:
        work = Path(td) / "sketch"
        temp_out = Path(td) / "out"
        temp_out.mkdir(parents=True, exist_ok=True)
        main = copy_sketch_project(project, ino, work)
        cmd = [cli, "compile", "--fqbn", fqbn, "--output-dir", str(temp_out), "--warnings", "all", "--export-binaries", str(work)]
        proc = subprocess.run(cmd, text=True, capture_output=True)
        artifacts = sorted(p for p in temp_out.rglob("*") if p.is_file() and p.suffix.lower() in EXTENSIONS)
        return proc.returncode, (proc.stdout + "\n" + proc.stderr)[-12000:], artifacts

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("root", type=Path, help="Root containing project.json files")
    ap.add_argument("--output", type=Path, default=Path("BUILD_OUTPUT"))
    ap.add_argument("--cli", default="arduino-cli")
    ap.add_argument("--default-fqbn", default="esp32:esp32:esp32s3")
    args = ap.parse_args()
    root, out = args.root.resolve(), args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    records = []
    projects = sorted({p.parent for p in root.rglob("project.json")})
    for project in projects:
        meta = json.loads((project / "project.json").read_text(encoding="utf-8"))
        fqbn = fqbn_for(meta, args.default_fqbn)
        fam = family(fqbn)
        for ino in ino_files(project):
            stage = out / fam
            stage.mkdir(parents=True, exist_ok=True)
            rc, log, artifacts = compile_one(project, ino, stage, fqbn, args.cli)
            # Arduino CLI may emit multiple target artifacts; keep only .bin/.hex.
            new_artifacts = [p for p in artifacts if p.suffix.lower() in EXTENSIONS]
            record = {"project": project.name, "source": ino.name, "fqbn": fqbn, "family": fam, "board": meta.get("board", meta.get("board_family", fqbn)),
                      "status": "PASS" if rc == 0 else "FAIL", "log": log, "artifacts": []}
            if rc == 0:
                for artifact in sorted(new_artifacts):
                    final = stage / f"{project.name}_{ino.stem}{artifact.suffix.lower()}"
                    if final.exists(): final.unlink()
                    shutil.copy2(artifact, final)
                    record["artifacts"].append({"path": str(final), "sha256": sha256(final)})
                meta_copy = {**meta, "fqbn": fqbn, "source": ino.name, "artifacts": record["artifacts"]}
                (stage / f"{project.name}_{ino.stem}.json").write_text(json.dumps(meta_copy, indent=2, ensure_ascii=False), encoding="utf-8")
            records.append(record)
            print(f"[{record['status']}] {project.name}/{ino.name} -> {fqbn}")
    (out / "build_manifest.json").write_text(json.dumps(records, indent=2, ensure_ascii=False), encoding="utf-8")
    return 0 if records and all(r["status"] == "PASS" for r in records) else (2 if records else 1)

if __name__ == "__main__":
    raise SystemExit(main())
