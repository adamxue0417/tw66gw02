"""Read-only comparison of numbered project snapshots and full v10/v14 verification."""
from pathlib import Path
import csv
import difflib
import hashlib
import io
import json
import os
import re
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / '.analysis'
EXTENSIONS = {'.c', '.h', '.s', '.sct', '.ld', '.uvprojx', '.ioc', '.ps1', '.py', '.bat', '.cmd', '.cmake'}
EXCLUDED = {'.git', '.git.disabled', '.venv', 'venv', '__pycache__', 'build', 'build_min', 'ota_artifacts', 'app_delivery', 'outputs', '.pytest_cache', 'rte'}

def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def decode(path):
    return decode_bytes(path.read_bytes())

def decode_bytes(raw):
    for encoding in ('utf-8-sig', 'gb18030'):
        try:
            return raw.decode(encoding).replace('\r\n', '\n').replace('\r', '\n')
        except UnicodeError:
            pass
    return raw.decode('latin-1').replace('\r\n', '\n').replace('\r', '\n')

def selected(root):
    result = {}
    for folder, dirs, files in os.walk(root):
        dirs[:] = sorted(d for d in dirs if d.lower() not in EXCLUDED)
        for name in sorted(files):
            path = Path(folder) / name
            rel = path.relative_to(root).as_posix()
            if path.suffix.lower() not in EXTENSIONS:
                continue
            if path.name.lower() == 'boot_image.s':
                continue
            if len(Path(rel).parts) > 2 and (rel.startswith('MDK-ARM/tw66gw02/') or rel.startswith('MDK-ARM/tw66gw02_')):
                continue
            result[rel] = decode(path)
    return result

projects = {str(n): selected(ROOT / str(n) / 'tw66gw02') for n in range(1, 14)}
summary = {'method': {'extensions': sorted(EXTENSIONS), 'excluded_directories': sorted(EXCLUDED), 'text_newlines_normalized': True}, 'versions': {}, 'comparisons': {}}
archive = subprocess.check_output(['git', 'archive', '--format=tar', 'v2'], cwd=ROOT)
with tarfile.open(fileobj=io.BytesIO(archive), mode='r:') as tar:
    projects['2_git'] = {
        member.name: decode_bytes(tar.extractfile(member).read())
        for member in tar.getmembers()
        if member.isfile() and Path(member.name).suffix.lower() in EXTENSIONS
        and not any(part.lower() in EXCLUDED for part in Path(member.name).parts[:-1])
        and not (len(Path(member.name).parts) > 2 and member.name.startswith(('MDK-ARM/tw66gw02/', 'MDK-ARM/tw66gw02_')))
        and Path(member.name).name.lower() != 'boot_image.s'
    }
summary['v2_git_source'] = subprocess.check_output(['git', 'rev-parse', 'v2'], cwd=ROOT, text=True).strip()
comment_pattern = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/')
def c_tokens(text):
    text = comment_pattern.sub(lambda match: '' if match[0].startswith('/') else match[0], text)
    return re.findall(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[a-zA-Z_0-9]+|[^\s]', text)
summary['v1_to_v2_c_token_differences'] = [
    name for name in sorted(projects['1'].keys() & projects['2_git'].keys())
    if Path(name).suffix.lower() in {'.c', '.h'} and c_tokens(projects['1'][name]) != c_tokens(projects['2_git'][name])
]
for version, files in projects.items():
    summary['versions'][version] = {'selected_files': len(files), 'files': sorted(files)}

pairs = [(str(n), str(n+1)) for n in range(1, 13)] + [('1', '3'), ('5', '7'), ('8', '10'), ('10', '12'), ('10', '13'), ('1', '2_git'), ('2_git', '3')]
for a, b in pairs:
    left, right = projects[a], projects[b]
    added = sorted(right.keys() - left.keys())
    deleted = sorted(left.keys() - right.keys())
    changed = sorted(k for k in left.keys() & right.keys() if left[k] != right[k])
    byte_only = sorted(k for k in left.keys() & right.keys() if left[k] == right[k] and sha(ROOT / a / 'tw66gw02' / k) != sha(ROOT / b / 'tw66gw02' / k)) if '_git' not in a + b else []
    label = f'{a}_to_{b}'
    details = {}
    with (OUT / f'{label}.diff').open('w', encoding='utf-8', newline='\n') as stream:
        for name in changed + added + deleted:
            before, after = left.get(name, '').splitlines(True), right.get(name, '').splitlines(True)
            patch = list(difflib.unified_diff(before, after, fromfile=f'{a}/tw66gw02/{name}', tofile=f'{b}/tw66gw02/{name}', n=3))
            stream.writelines(patch)
            details[name] = {'added_lines': sum(line.startswith('+') and not line.startswith('+++') for line in patch), 'deleted_lines': sum(line.startswith('-') and not line.startswith('---') for line in patch)}
    summary['comparisons'][label] = {'added': added, 'deleted': deleted, 'changed': changed, 'byte_only': byte_only, 'line_counts': details}

with (OUT / 'version_comparison.json').open('w', encoding='utf-8') as stream:
    json.dump(summary, stream, ensure_ascii=False, indent=2)

def full_manifest(root):
    files, dirs = {}, []
    for folder, names, filenames in os.walk(root):
        for name in names:
            dirs.append((Path(folder) / name).relative_to(root).as_posix())
        for name in filenames:
            path = Path(folder) / name
            files[path.relative_to(root).as_posix()] = (path.stat().st_size, sha(path))
    return files, sorted(dirs)

source, source_dirs = full_manifest(ROOT / '10')
target, target_dirs = full_manifest(ROOT / '14')
verification = {'source': str(ROOT / '10'), 'destination': str(ROOT / '14'), 'files': len(source), 'bytes': sum(v[0] for v in source.values()), 'directories_including_root': len(source_dirs) + 1, 'missing_files': sorted(source.keys() - target.keys()), 'extra_files': sorted(target.keys() - source.keys()), 'different_files': sorted(k for k in source.keys() & target.keys() if source[k] != target[k]), 'directory_structure_equal': source_dirs == target_dirs}
verification['passed'] = source == target and source_dirs == target_dirs
(OUT / 'v14_copy_verification.json').write_text(json.dumps(verification, ensure_ascii=False, indent=2), encoding='utf-8')
with (OUT / 'v14_sha256.csv').open('w', encoding='utf-8-sig', newline='') as stream:
    writer = csv.writer(stream)
    writer.writerow(['relative_path', 'bytes', 'v10_sha256', 'v14_sha256', 'match'])
    for key in sorted(source):
        writer.writerow([key, source[key][0], source[key][1], target.get(key, (0, ''))[1], source[key] == target.get(key)])
print(json.dumps({'copy_verification': verification, 'versions': {k: v['selected_files'] for k, v in summary['versions'].items()}, 'comparisons': {k: {t: len(v[t]) for t in ('added', 'deleted', 'changed', 'byte_only')} for k, v in summary['comparisons'].items()}}, ensure_ascii=False, indent=2))
if not verification['passed']:
    raise SystemExit('Version 14 verification failed')
