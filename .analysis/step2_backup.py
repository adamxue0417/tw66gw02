"""Create and verify local, non-overwriting pre-organization archives."""
from pathlib import Path
import csv
import hashlib
import json
import os
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / '90_历史原始包' / '整理前备份' / '20260909-step2'
EXCLUDED = {'private_keys', 'deps', '.venv', '__pycache__'}


def digest(stream):
    h = hashlib.sha256()
    while chunk := stream.read(1024 * 1024):
        h.update(chunk)
    return h.hexdigest()


def fail(error):
    raise error


def collect(starts):
    files, empty, excluded = [], [], []
    for start in starts:
        if start.is_file():
            files.append(start)
            continue
        for folder, dirs, names in os.walk(start, onerror=fail):
            p = Path(folder)
            for name in list(dirs):
                if name in EXCLUDED:
                    excluded.append((p / name).relative_to(ROOT).as_posix())
                    dirs.remove(name)
            if not dirs and not names:
                empty.append(p)
            files.extend(p / name for name in names)
    return sorted(files), sorted(empty), sorted(excluded)


def main():
    DEST.mkdir(parents=True, exist_ok=False)
    main_paths = [p for p in ROOT.iterdir()
                  if p.name not in {str(n) for n in range(1, 15)} | {'90_历史原始包'}]
    scopes = {'main-and-shared': main_paths, **{f'snapshot-{n}': [ROOT / str(n)] for n in (12, 13, 14)}}
    report = {'date': '2026-09-09', 'scope': 'Main/shared working files including Git metadata, and snapshots 12/13/14. Archives retain workspace-relative paths.',
              'limitations': 'Local same-disk recovery copies, not offsite backup. Signing private_keys, deps, .venv and __pycache__ directories are excluded and left untouched. Unchanged snapshots 1-11 are not archived.', 'archives': []}
    for name, starts in scopes.items():
        files, empty, excluded = collect(starts)
        rows = []
        archive = DEST / (name + '.zip')
        with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=1, allowZip64=True) as z:
            for p in empty:
                z.writestr(p.relative_to(ROOT).as_posix() + '/', b'')
            for p in files:
                rel = p.relative_to(ROOT).as_posix()
                before = p.stat()
                h = hashlib.sha256()
                with p.open('rb') as source, z.open(rel, 'w', force_zip64=True) as target:
                    while chunk := source.read(1024 * 1024):
                        h.update(chunk)
                        target.write(chunk)
                after = p.stat()
                if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
                    raise RuntimeError('File changed during backup: ' + rel)
                rows.append({'path': rel, 'bytes': before.st_size, 'sha256': h.hexdigest()})
        # Decompress every member and verify its content, not just the ZIP header.
        with zipfile.ZipFile(archive) as z:
            for row in rows:
                with z.open(row['path']) as source:
                    if digest(source) != row['sha256']:
                        raise RuntimeError('Archive verification failed: ' + row['path'])
                with (ROOT / row['path']).open('rb') as source:
                    if digest(source) != row['sha256']:
                        raise RuntimeError('Source changed after backup: ' + row['path'])
        with (DEST / (name + '.files.csv')).open('w', encoding='utf-8-sig', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=['path', 'bytes', 'sha256'])
            writer.writeheader()
            writer.writerows(rows)
        with archive.open('rb') as f:
            sha = digest(f)
        item = {'archive': archive.name, 'files': len(rows), 'source_bytes': sum(r['bytes'] for r in rows), 'zip_bytes': archive.stat().st_size, 'sha256': sha, 'excluded_directories': excluded, 'verified_members': len(rows), 'empty_directories': len(empty)}
        report['archives'].append(item)
        print(json.dumps(item, ensure_ascii=False), flush=True)
    (DEST / 'backup_report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
