"""沿用 v2 文件白名单，实际解压验证 v3；不覆盖已有包或冻结固件。"""
from pathlib import Path
from datetime import datetime
import argparse
import contextlib
import hashlib
import json
import re
import subprocess
import zipfile
from audit_comments import EV, WT, MAIN, BASE, git, sha, audit

NAME = 'TW66GW02_工程交接_精简版_v3_中文注释'

def main(workspace):
    result = audit(workspace)
    commit = git('rev-parse', 'HEAD').decode().strip()
    assert git('diff', '--name-only', 'HEAD', '--', '01_固件工程/main') == b''
    assert git('branch', '--show-current').decode().strip() == 'docs-main-handover-cn'
    assert commit != result['base_commit'], 'Commit comments before packaging'
    before = json.loads((WT / '.handover-local/before.json').read_text('utf8'))
    out = workspace / '05_发布与交付/工程师交接'
    final = out / (NAME + '.zip')
    pending = out / (NAME + '.zip.building')
    checksum = out / (NAME + '.zip.sha256')
    assert not any(p.exists() for p in [final, pending, checksum]), 'Existing delivery must be preserved'
    old = out / 'TW66GW02_工程交接_精简版_v2.zip'
    entries = {}
    with zipfile.ZipFile(old) as z:
        for info in z.infolist():
            if info.is_dir():
                continue
            rel = info.filename.split('/', 1)[1]
            data = z.read(info)
            if rel.startswith('01_主程序/main/'):
                source = rel[len('01_主程序/main/'):]
                data = (MAIN / source).read_bytes()
                assert data == git('show', commit + ':01_固件工程/main/' + source), source
            entries[rel] = data
    guide = (EV / '使用说明.template.md').read_text('utf8').replace('@SOURCE_COMMIT@', commit).replace('@BASE_COMMIT@', result['base_commit'])
    entries['使用说明.md'] = guide.encode('utf8')
    assert len(entries) == 178
    for name in entries:
        path = Path(name)
        assert not path.is_absolute() and '..' not in path.parts
        assert not set(path.parts) & {'.git', '.codex', '.agents', '__pycache__', 'Objects', 'Listings', 'OTA_Artifacts'}
        assert path.name not in {'plan.md', 'AGENTS.md', 'CHANGELOG.md'}
        assert path.suffix.lower() not in {'.log', '.key', '.pem', '.zip', '.axf', '.o', '.bak'}
    links = re.findall(r'\]\(([^)]+)\)', guide)
    for target in links:
        assert target in entries or any(n.startswith(target + '/') for n in entries), target
    with zipfile.ZipFile(pending, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for name, data in sorted(entries.items()):
            z.writestr(NAME + '/' + name, data)
    work = workspace / '.handover-local' / ('cn-v3-' + datetime.now().strftime('%Y%m%d-%H%M%S'))
    work.mkdir(parents=True, exist_ok=False)
    with zipfile.ZipFile(pending) as z:
        assert z.testzip() is None
        assert len(z.infolist()) == len(entries)
        for member in z.infolist():
            assert (work / member.filename).resolve().is_relative_to(work.resolve())
        z.extractall(work)
    project = work / NAME / '01_主程序/main'
    report = {'task': 'OTA-006', 'test': 'TEST-045', 'result': 'IN_PROGRESS', 'hardware': 'NOT_RUN',
              'source_commit': commit, 'baseline_commit': result['base_commit'], 'branch': 'docs-main-handover-cn',
              'v2_zip_sha256': result['v2_zip_sha256'], 'extracted_project': str(project), 'commands': [],
              'files': [{'path': name, 'bytes': len(data), 'sha256': sha(data)} for name, data in sorted(entries.items())]}

    def save():
        (EV / 'result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf8')

    def run(label, command):
        print('RUN ' + label, flush=True)
        with (EV / (label + '.log')).open('wb') as log:
            proc = subprocess.run(command, cwd=project, stdout=log, stderr=subprocess.STDOUT, timeout=360)
        report['commands'].append({'label': label, 'command': command, 'exit_code': proc.returncode})
        save()
        assert proc.returncode == 0, label

    try:
        save()
        ps = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File']
        run('factory-build', ps + [str(project / 'BuildKeilFactoryTarget.ps1')])
        run('ota-build', ps + [str(project / 'BuildOtaArtifacts.ps1'), '-FactoryVersion', '100', '-OtaVersion', '101'])
        for name in ['build_factory_keil.log', 'build_v100.log', 'build_v101.log']:
            data = (project / 'OTA_Artifacts' / name).read_bytes()
            assert b'0 Error(s), 0 Warning(s)' in data, name
            (EV / name).write_bytes(data)
        # 复用既有测试，实现不改；ROOT 指向本次实际解压工程。
        ns = {'__name__': 'verify_handover', '__file__': str(project / 'tests/test_ota_artifacts.py')}
        with (EV / 'ota-tests.log').open('w', encoding='utf8') as f, contextlib.redirect_stdout(f):
            exec(compile((MAIN / 'tests/test_ota_artifacts.py').read_text('utf8'), 'test_ota_artifacts.py', 'exec'), ns)
            ns['main']()
        test_text = (EV / 'ota-tests.log').read_text('utf8')
        assert '162' in test_text, 'Recovery simulation count missing'
        reference = workspace / '05_发布与交付/MCU固件/main_v100-to-v101_DEV'
        pairs = [('Bootloader/build/mathis_bootloader.bin', 'Bootloader/mathis_bootloader.bin'),
                 ('OTA_Artifacts/mathis_app_v100.bin', 'Debug_Only/mathis_app_v100.bin'),
                 ('OTA_Artifacts/mathis_app_v101.bin', 'Debug_Only/mathis_app_v101.bin')]
        for rel in ['Factory_Flash/mathis_factory_v100.hex', 'App_Upload/mathis_ota_v101.ota', 'App_Upload/mathis_ota_v101.manifest.json']:
            pairs.append(('OTA_Artifacts/Release_v100_to_v101/' + rel, rel))
        compared = []
        for built, frozen in pairs:
            data = (project / built).read_bytes()
            assert data == (reference / frozen).read_bytes(), built
            compared.append({'built': built, 'frozen': frozen, 'bytes': len(data), 'sha256': sha(data), 'equal': True})
        for name, data in entries.items():
            assert (work / NAME / name).read_bytes() == data, name
        for name, digest in before['protected'].items():
            assert sha((workspace / name).read_bytes()) == digest, 'Protected file changed: ' + name
        assert sha((workspace / '.git/index').read_bytes()) == before['root_index_sha256']
        assert sha(old.read_bytes()) == result['v2_zip_sha256']
        pending.rename(final)
        digest = sha(final.read_bytes())
        checksum.write_text(digest + '  ' + final.name + '\n', encoding='utf8')
        report.update(result='PASS', build='factory/v100/v101: 0 errors, 0 warnings',
                      tests='Existing OTA checks and 162 interrupted swap/rollback simulations PASS',
                      compared_artifacts=compared, zip_path=final.relative_to(workspace).as_posix(), zip_sha256=digest,
                      zip_bytes=final.stat().st_size, file_count=len(entries), guide_links_checked=len(links),
                      protected_files_unchanged=len(before['protected']), original_index_unchanged=True)
        save()
        (EV / 'files.sha256').write_text(''.join(item['sha256'] + '  ' + item['path'] + '\n' for item in report['files']), encoding='utf8')
        # 校验信息入 Git，固件与 ZIP 只保存在交付/本地构建目录。
        (EV / checksum.name).write_bytes(checksum.read_bytes())
        print(json.dumps({k: v for k, v in report.items() if k not in {'commands', 'files', 'compared_artifacts'}}, ensure_ascii=False, indent=2), flush=True)
    except Exception as error:
        report.update(result='FAIL', error=str(error))
        save()
        raise

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--workspace-root', type=Path, required=True)
    main(parser.parse_args().workspace_root.resolve())
