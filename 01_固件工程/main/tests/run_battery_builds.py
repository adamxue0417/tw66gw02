"""Build Sleep OFF/ON in fresh isolated copies; retain logs and artifact hashes.

Output must be outside the firmware source directory and must not already exist.
Requires Keil/ARM Compiler 5. No flashing, network access or baseline replacement.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--keil-root', type=Path, default=Path('C:/Keil_v5'))
    args = parser.parse_args()
    output = args.output.resolve()
    if output.is_relative_to(ROOT) or output.exists():
        raise ValueError('Use a new output directory outside the source tree')
    tracked = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others',
                                      '--exclude-standard', '--', '.'], cwd=ROOT)
    names = sorted(set(tracked.decode('utf-8').split('\0')) - {''})
    allowed = {'.c', '.h', '.s', '.sct', '.uvprojx', '.py', '.ps1', '.md', '.txt', '.ioc'}
    names = [name for name in names if (ROOT / name).suffix.lower() in allowed]
    fingerprint = {name: sha(ROOT / name) for name in names}
    output.mkdir(parents=True)
    report = {'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT,
                text=True).strip(), 'source_files': fingerprint, 'modes': {}, 'result': 'RUNNING',
              'hardware': 'NOT_RUN'}
    def save():
        (output / 'result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    def run(command, cwd, log):
        print('RUN ' + log.name, flush=True)
        with log.open('wb') as stream:
            result = subprocess.run([str(x) for x in command], cwd=cwd, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=300)
        if result.returncode:
            raise RuntimeError(f'{log}: exit {result.returncode}')
    save()
    try:
        for enabled in (0, 1):
            label = f'sleep-{enabled}'
            mode_dir = output / label
            project = mode_dir / 'project'
            for name in names:
                target = project / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / name, target)
                assert sha(target) == fingerprint[name]
            ps = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File']
            commands = [ps + [project / 'BuildKeilFactoryTarget.ps1', '-KeilRoot', args.keil_root,
                              '-IdleSleepEnabled', enabled],
                        ps + [project / 'BuildOtaArtifacts.ps1', '-KeilRoot', args.keil_root,
                              '-FactoryVersion', 100, '-OtaVersion', 101, '-IdleSleepEnabled', enabled],
                        [sys.executable, project / 'tests/test_ota_artifacts.py']]
            for name, command in zip(('factory', 'ota', 'ota-tests'), commands):
                run(command, project, mode_dir / (name + '.log'))
            artifacts = project / 'OTA_Artifacts'
            for name in ('build_factory_keil.log', 'build_v100.log', 'build_v101.log'):
                log = artifacts / name
                assert b'0 Error(s), 0 Warning(s)' in log.read_bytes(), name
                shutil.copyfile(log, mode_dir / name)
            record = {'define': f'SCH_IDLE_SLEEP_ENABLED={enabled}', 'result': 'PASS',
                      'builds': 'factory/v100/v101: 0 errors, 0 warnings', 'artifacts': {}}
            for file in sorted(artifacts.rglob('*')) + sorted((project / 'Bootloader/build').glob('*')):
                if file.is_file() and file.suffix.lower() in {'.bin', '.hex', '.ota', '.json'}:
                    record['artifacts'][file.relative_to(project).as_posix()] = {
                        'bytes': file.stat().st_size, 'sha256': sha(file)}
            report['modes'][label] = record
            save()
        assert all(sha(ROOT / name) == digest for name, digest in fingerprint.items())
        report['source_unchanged'] = True
        report['result'] = 'PASS'
    except Exception as error:
        report['result'] = 'FAIL'
        report['error'] = str(error)
        raise
    finally:
        save()
    print('PASS: factory/v100/v101 in both modes; 162 OTA recovery cases per mode')

if __name__ == '__main__':
    main()
