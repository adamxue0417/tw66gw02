"""One-time step 4 inventory and verified backup; deletion is a separate PS action."""
from pathlib import Path
import csv, hashlib, json, os, shutil, subprocess, zipfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / '00_项目索引/证据/第四步'
BACK = ROOT / '90_历史原始包/整理前备份/20260910-step4'
MAIN = ROOT / '01_固件工程/main'
V14 = ROOT / '01_固件工程/development/snapshot-14_refactor-from-10/tw66gw02'

def sha(p):
    with p.open('rb') as f: return hashlib.file_digest(f, 'sha256').hexdigest()

def save(p, obj):
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(json.dumps(obj, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')

def safe(p):
    p = p.resolve()
    assert p.is_relative_to(ROOT) and p != ROOT, p
    return p

def prepare():
    assert not (OUT/'中间文件备份.json').exists(), 'Already prepared; do not overwrite evidence'
    OUT.mkdir(parents=True, exist_ok=True); BACK.mkdir(parents=True, exist_ok=True)
    # Preserve current maintained sources/configs, excluding generated build probes.
    protected = []
    exts = {'.c','.h','.s','.sct','.ld','.uvprojx','.ioc','.ps1','.py','.jflash','.yml'}
    for project in [MAIN, V14]:
        for parent, dirs, files in os.walk(project):
            dirs[:] = [d for d in dirs if d not in {'.git','.git.disabled','.venv','__pycache__','build','build_min','runs','.build-runs','OTA_Artifacts'} and not d.startswith('tw66gw02_ota_')]
            for name in files:
                p = Path(parent)/name
                if p.suffix.lower() in exts and '.generated.' not in name and name != 'boot_image.s':
                    protected.append({'path':p.relative_to(ROOT).as_posix(),'sha256':sha(p)})
    save(OUT/'清理前源码配置指纹.json', protected)
    tracked = set()
    for repo in [ROOT, V14.parent]:
        result = subprocess.run(['git','-C',str(repo),'ls-files','-z'],capture_output=True,check=True)
        tracked.update((repo/os.fsdecode(p)).resolve() for p in result.stdout.split(b'\0') if p)
    rows = []; excluded = []
    for project in [MAIN, V14]:
        # Only known compiler output directories, never arbitrary matching suffixes.
        bases = [project/'Bootloader/build', project/'Bootloader/build_min']
        bases += [p for p in (project/'MDK-ARM').iterdir() if p.is_dir() and (p.name.startswith('tw66gw02') or p.name in {'runs','.build-runs'})]
        for base in bases:
            if not base.exists(): continue
            for p in base.rglob('*'):
                if not p.is_file() or p.suffix.lower() not in {'.o','.crf','.d'}: continue
                safe(p)
                if p.resolve() in tracked:
                    excluded.append(p.relative_to(ROOT).as_posix()); continue
                rows.append({'path':p.relative_to(ROOT).as_posix(),'bytes':p.stat().st_size,'sha256':sha(p)})
    rows.sort(key=lambda x:x['path']); assert rows and len({x['path'] for x in rows}) == len(rows)
    archive = BACK/'compiler-intermediates.zip'
    assert not archive.exists()
    with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for row in rows: z.write(ROOT/row['path'],row['path'])
    with zipfile.ZipFile(archive) as z:
        assert len(z.infolist()) == len(rows)
        for row in rows:
            with z.open(row['path']) as f: assert hashlib.file_digest(f,'sha256').hexdigest() == row['sha256'], row['path']
    result = {'archive':archive.relative_to(ROOT).as_posix(),'archive_sha256':sha(archive),'archive_bytes':archive.stat().st_size,'file_count':len(rows),'original_bytes':sum(x['bytes'] for x in rows),'verified_decompressed_files':len(rows),'tracked_excluded':excluded,'files':rows}
    save(OUT/'中间文件备份.json',result)
    shutil.copy2(OUT/'中间文件备份.json',BACK/'manifest.json')
    # Capture preimages for the public documents and registry changed by this step.
    for rel in ['README.md','05_发布与交付/README.md','05_发布与交付/现有安装包与模块固件.json','00_项目索引/文件用途索引.md','00_项目索引/架构与迁移计划.md','90_历史原始包/README.md','.analysis/README.md']:
        dest=BACK/'edited-files'/rel;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/rel,dest)
    print(json.dumps({k:v for k,v in result.items() if k!='files'},ensure_ascii=False))

if __name__ == '__main__': prepare()
