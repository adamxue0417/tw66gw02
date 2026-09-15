"""Build the verified main snapshot in a fresh, local ignored directory."""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import shutil
import subprocess
import sys
import zipfile

EVIDENCE = Path(__file__).resolve().parent
ROOT = EVIDENCE.parents[3]
BASELINE = EVIDENCE.parent / '20260911-step1/baseline.json'
WORK = ROOT / '90_历史原始包/整理前备份/ota010-step2/main'
KEIL = Path('C:/Keil_v5')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(data):
    (EVIDENCE / 'result.json').write_text(
        json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def run(label, command, cwd, timeout=300):
    log = EVIDENCE / (label + '.log')
    started = datetime.now().astimezone().isoformat()
    print('RUN ' + label, flush=True)
    with log.open('wb') as stream:
        process = subprocess.run(command, cwd=cwd, stdout=stream,
                                 stderr=subprocess.STDOUT, timeout=timeout)
    result = {'command': [str(p) for p in command], 'cwd': str(cwd),
              'started': started, 'finished': datetime.now().astimezone().isoformat(),
              'exit_code': process.returncode, 'log': log.name,
              'log_sha256': digest(log)}
    print(f'END {label}: exit {process.returncode}', flush=True)
    return result


def check_originals(baseline, frozen):
    checked = 0
    for data in baseline['repositories'].values():
        repo = ROOT / data['repository_root']
        assert subprocess.check_output(['git', '-C', str(repo), 'rev-parse', 'HEAD']).decode().strip() == data['head']
        for row in data['archive']['files']:
            assert digest(repo / row['path']) == row['sha256'], row['path']
            checked += 1
    for name, hash_value in frozen.items():
        assert digest(ROOT / name) == hash_value, name
    return {'source_files_unchanged': checked, 'frozen_delivery_files_unchanged': len(frozen),
            'git_heads_unchanged': True}


def main():
    if WORK.exists() or (EVIDENCE / 'result.json').exists():
        raise RuntimeError('Existing run must not be overwritten; use a new run directory')
    baseline = json.loads(BASELINE.read_text(encoding='utf-8'))
    entry = baseline['repositories']['main']['archive']
    archive = ROOT / entry['path']
    assert digest(archive) == entry['sha256']
    delivery = ROOT / '05_发布与交付/MCU固件/main_v100-to-v101_DEV'
    frozen = {p.relative_to(ROOT).as_posix(): digest(p) for p in delivery.rglob('*') if p.is_file()}
    assert digest(delivery / 'App_Upload/mathis_ota_v101.ota') == '7d72822aa7ad78dde1265e720eed1b003e81f5e70d59e33c9e1a09ce0bb41b8e'
    report = {'task': 'OTA-010', 'step': 2, 'result': 'IN_PROGRESS',
              'started': datetime.now().astimezone().isoformat(),
              'source_head': baseline['repositories']['main']['head'],
              'source_archive_sha256': entry['sha256'], 'isolated_project': str(WORK),
              'frozen_files_before': frozen, 'commands': {}}
    report['before'] = check_originals(baseline, frozen)
    WORK.mkdir(parents=True)
    prefix = '01_固件工程/main/'
    with zipfile.ZipFile(archive) as z:
        for row in entry['files']:
            assert row['path'].startswith(prefix)
            target = WORK / row['path'][len(prefix):]
            assert target.resolve().is_relative_to(WORK.resolve())
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(z.read(row['path']))
            assert digest(target) == row['sha256']
    save(report)
    try:
        compiler = KEIL / 'ARM/ARM_Compiler_5.06u7/Bin/armcc.exe'
        report['commands']['compiler-version'] = run('compiler-version', [str(compiler), '--vsn'], WORK)
        save(report)
        ps = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File']
        for label, args in [
            ('factory-build', [str(WORK / 'BuildKeilFactoryTarget.ps1')]),
            ('ota-build', [str(WORK / 'BuildOtaArtifacts.ps1'), '-FactoryVersion', '100', '-OtaVersion', '101']),
            ('ota-tests', [sys.executable, str(WORK / 'tests/test_ota_artifacts.py')]),
        ]:
            cmd = args if label == 'ota-tests' else ps + args
            report['commands'][label] = run(label, cmd, WORK)
            save(report)
            if report['commands'][label]['exit_code'] != 0:
                raise RuntimeError(f'{label} failed; inspect saved log')
        outputs = []
        for p in sorted((WORK / 'OTA_Artifacts').rglob('*')):
            if p.is_file():
                outputs.append({'path': p.relative_to(WORK).as_posix(),
                                'bytes': p.stat().st_size, 'sha256': digest(p)})
        report['outputs'] = outputs
        report['rebuilt_manifest'] = json.loads((WORK / 'OTA_Artifacts/mathis_ota_v101.manifest.json').read_text(encoding='utf-8-sig'))
        report['rebuilt_artifact_equals_frozen'] = digest(WORK / 'OTA_Artifacts/mathis_ota_v101.ota') == digest(delivery / 'App_Upload/mathis_ota_v101.ota')
        report['optional_diagnostic_checks'] = 'NOT_RUN: diagnostic binaries not generated by these two baseline build commands'
        report['hardware'] = 'NOT_RUN'
        report['result'] = 'PASS'
    except Exception as exc:
        report['result'] = 'FAIL'
        report['error'] = str(exc)
        raise
    finally:
        copied = []
        for name in ('build_factory_keil.log', 'build_v100.log', 'build_v101.log'):
            source = WORK / 'OTA_Artifacts' / name
            if source.is_file():
                shutil.copyfile(source, EVIDENCE / name)
                copied.append({'path': name, 'sha256': digest(EVIDENCE / name)})
        report['native_build_logs'] = copied
        report['after'] = check_originals(baseline, frozen)
        altered = []
        for row in entry['files']:
            target = WORK / row['path'][len(prefix):]
            if not target.exists() or digest(target) != row['sha256']:
                altered.append(row['path'][len(prefix):])
        report['isolated_tracked_files_changed_by_build'] = altered
        if altered:
            report['result'] = 'FAIL'
        report['finished'] = datetime.now().astimezone().isoformat()
        save(report)
        print('RESULT ' + report['result'], flush=True)
        print(json.dumps(report['after']), flush=True)


if __name__ == '__main__':
    main()
