"""Capture OTA-010 step 1 without modifying source files or Git refs/index.

Run once from any directory. Archives contain exact tracked working-tree bytes;
they are local recovery material, not release binaries or Git history backups.
"""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import subprocess
import zipfile

EVIDENCE = Path(__file__).resolve().parent
ROOT = EVIDENCE.parents[3]
MAIN = ROOT / '01_固件工程/main'
SOURCE = ROOT / '01_固件工程/development/snapshot-13_poweroff-v104/tw66gw02'
BACKUP = ROOT / '90_历史原始包/整理前备份/20260911-ota010-step1'


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])


def decode(raw):
    return raw.decode('utf-8').strip()


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def write_json(path, data):
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def relative(path):
    return path.relative_to(ROOT).as_posix()


def archive_files(path, entries):
    records = []
    with zipfile.ZipFile(path, 'x', compression=zipfile.ZIP_DEFLATED) as archive:
        for source, name in entries:
            if source.is_symlink() or not source.is_file():
                raise RuntimeError(f'Unexpected non-regular file: {name}')
            raw = source.read_bytes()
            # Refuse obvious private signing material without logging contents.
            if b'PRIVATE KEY-----' in raw:
                raise RuntimeError(f'Private key marker in capture input: {name}')
            archive.writestr(name, raw)
            records.append({'path': name, 'bytes': len(raw), 'sha256': sha(raw)})
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None
        assert len(archive.namelist()) == len(records)
        for row in records:
            assert sha(archive.read(row['path'])) == row['sha256']
    return {'path': relative(path), 'bytes': path.stat().st_size,
            'sha256': sha(path.read_bytes()), 'files': records,
            'archive_readback': 'PASS'}


def capture_repo(name, repo, scope):
    prefix = ['--', scope] if scope else []
    files = [p.decode('utf-8') for p in git(repo, 'ls-files', '-z', *prefix).split(b'\0') if p]
    status = git(repo, 'status', '--porcelain=v1', '--untracked-files=all', *prefix)
    if status:
        raise RuntimeError(f'{name} source scope changed; inspect before capture')
    ignored = [p for p in git(repo, 'ls-files', '--others', '--ignored', '--exclude-standard', '-z', *prefix).split(b'\0') if p]
    patches = {}
    for label, args in [('unstaged', []), ('staged', ['--cached'])]:
        raw = git(repo, 'diff', *args, '--binary', '--full-index', '--no-ext-diff', '--no-textconv', *prefix)
        file = EVIDENCE / f'{name}.{label}.patch'
        file.write_bytes(raw)
        patches[label] = {'path': relative(file), 'bytes': len(raw), 'sha256': sha(raw)}
    archive = archive_files(BACKUP / f'{name}.tracked-worktree.zip', [(repo / p, p) for p in files])
    git_tree = decode(git(repo, 'rev-parse', f'HEAD:{scope}' if scope else 'HEAD^{tree}'))
    return {'repository_root': relative(repo) if repo != ROOT else '.',
            'project_scope': scope or '.',
            'branch': decode(git(repo, 'branch', '--show-current')),
            'head': decode(git(repo, 'rev-parse', 'HEAD')),
            'scope_tree': git_tree,
            'head_date': decode(git(repo, 'log', '-1', '--format=%cI')),
            'head_subject': decode(git(repo, 'log', '-1', '--format=%s')),
            'scope_status': 'CLEAN', 'tracked_file_count': len(files),
            'ignored_untracked_file_count_not_archived': len(ignored),
            'patches': patches, 'archive': archive}


def main():
    assert (ROOT / 'AGENTS.md').is_file() and MAIN.is_dir() and SOURCE.is_dir()
    if BACKUP.exists() or (EVIDENCE / 'baseline.json').exists():
        raise RuntimeError('Baseline already exists; never overwrite a recovery point')
    before_root = git(ROOT, 'status', '--porcelain=v1', '--untracked-files=all')
    # Save only the known pre-existing documentation modifications.
    docs = [ROOT / name for name in ('README.md', 'AGENTS.md', 'plan.md')]
    docs_patch = git(ROOT, 'diff', '--binary', '--full-index', '--no-ext-diff', '--no-textconv', '--', *(p.name for p in docs))
    staged = git(ROOT, 'diff', '--cached', '--binary', '--full-index', '--no-ext-diff', '--no-textconv')
    if staged:
        raise RuntimeError('Index changed; inspect before capture')
    BACKUP.mkdir(parents=True)
    (EVIDENCE / 'root-status-before.txt').write_bytes(before_root)
    (EVIDENCE / 'root-docs.unstaged.patch').write_bytes(docs_patch)
    report = {'capture_time_local': datetime.now().astimezone().isoformat(),
              'task': 'OTA-010', 'step': 1,
              'scope': 'Tracked working-tree bytes, Git identities and existing documentation overlay; no build/hardware verification',
              'git_index_and_refs_modified': False,
              'documentation_before_update': archive_files(BACKUP / 'documentation-before-step1.zip', [(p, p.name) for p in docs]),
              'repositories': {}}
    report['repositories']['main'] = capture_repo('main', ROOT, relative(MAIN))
    report['repositories']['snapshot13'] = capture_repo('snapshot13', SOURCE, '')
    # Check all archived source bytes again against the still-live workspaces.
    for name, repo in [('main', ROOT), ('snapshot13', SOURCE)]:
        for row in report['repositories'][name]['archive']['files']:
            assert sha((repo / row['path']).read_bytes()) == row['sha256']
    report['live_source_recheck'] = 'PASS'
    report['excluded'] = ['Git object history and config', 'ignored/untracked build outputs',
                          'compiler and tool installation', 'private signing material outside tracked scope',
                          'unrelated workspace directories']
    write_json(EVIDENCE / 'baseline.json', report)
    print(json.dumps({name: {'branch': data['branch'], 'head': data['head'],
                            'files': data['tracked_file_count'],
                            'archive_sha256': data['archive']['sha256']}
                      for name, data in report['repositories'].items()}, indent=2))
    print('PASS: source archives read back; all live source hashes unchanged; Git refs/index untouched.')


if __name__ == '__main__':
    main()
