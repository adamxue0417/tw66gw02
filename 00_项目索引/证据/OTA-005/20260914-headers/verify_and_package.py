"""Build the refactored main from the v2 ZIP, verify bytes, keep logs local."""
from pathlib import Path
from datetime import datetime
import contextlib
import hashlib
import json
import os
import re
import subprocess
import zipfile

EV = Path(__file__).resolve().parent
ROOT = EV.parents[3]
MAIN = ROOT / '01_固件工程/main'
OUT = ROOT / '05_发布与交付/工程师交接'
OLD = 'TW66GW02_工程交接_精简版'
NEW = OLD + '_v2'


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    original = OUT / (OLD + '.zip')
    old_hash = sha(original)
    final = OUT / (NEW + '.zip')
    temp = OUT / (NEW + '.zip.building')
    assert not final.exists() and not temp.exists()
    local_sources = [p for p in MAIN.rglob('*') if p.is_file() and p.suffix in {'.c', '.h', '.s', '.sct', '.ps1', '.uvprojx', '.md', '.ioc'} and not any(x in {'OTA_Artifacts', 'build', 'build_min', '__pycache__'} for x in p.parts)]
    source_before = {str(p): sha(p) for p in local_sources}
    frozen = json.loads((EV.parents[1] / 'OTA-010/20260911-step3/result.json').read_text(encoding='utf-8'))['frozen_before']
    for name, h in frozen.items():
        assert sha(ROOT / name) == h
    entries = {}
    with zipfile.ZipFile(original) as z:
        for name in z.namelist():
            rel = name[len(OLD) + 1:]
            if rel.startswith('01_主程序/main/'):
                entries[rel] = (MAIN / rel[len('01_主程序/main/'):]).read_bytes()
            else:
                entries[rel] = z.read(name)
    headers = ['BSP/temp_config.h', 'BSP/wireless_internal.h', 'COP/main_control_internal.h',
               'COP/ota_update_internal.h', 'Bootloader/boot_internal.h']
    for name in headers + ['编码规范.md']:
        entries['01_主程序/main/' + name] = (MAIN / name).read_bytes()
    guide = entries['使用说明.md'].decode('utf-8')
    guide = guide.replace('## 编译', '代码维护约定见 [编码规范](01_主程序/main/编码规范.md)。\n\n## 编译', 1)
    entries['使用说明.md'] = guide.encode('utf-8')
    for target in re.findall(r'\]\(([^)]+)\)', guide):
        assert target in entries or any(n.startswith(target + '/') for n in entries)
    for name in entries:
        assert Path(name).name not in {'plan.md', 'AGENTS.md', 'CHANGELOG.md'}
        assert Path(name).suffix not in {'.log', '.key', '.pem'}
    with zipfile.ZipFile(temp, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for name, data in sorted(entries.items()):
            z.writestr(NEW + '/' + name, data)
    work = ROOT / '90_历史原始包/整理前备份' / ('headers-' + datetime.now().strftime('%Y%m%d-%H%M%S'))
    work.mkdir(parents=True, exist_ok=False)
    with zipfile.ZipFile(temp) as z:
        assert z.testzip() is None
        for member in z.infolist():
            assert (work / member.filename).resolve().is_relative_to(work.resolve())
        z.extractall(work)
    project = work / NEW / '01_主程序/main'
    report = {'task': 'OTA-005', 'test': 'TEST-008', 'result': 'IN_PROGRESS', 'hardware': 'NOT_RUN',
              'work': str(work), 'commands': [], 'files': [{'path': name, 'sha256': hashlib.sha256(data).hexdigest()} for name, data in sorted(entries.items())]}

    def save():
        (EV / 'result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

    def run(label, cmd):
        print('RUN ' + label, flush=True)
        with (EV / (label + '.log')).open('wb') as f:
            result = subprocess.run(cmd, cwd=project, stdout=f, stderr=subprocess.STDOUT, timeout=300)
        report['commands'].append({'label': label, 'command': cmd, 'exit_code': result.returncode})
        save()
        assert result.returncode == 0, label

    save()
    try:
        compiler = 'C:/Keil_v5/ARM/ARM_Compiler_5.06u7/Bin/armcc.exe'
        includes = ['BSP', 'COP', 'OS', 'Core/Inc', 'Drivers/CMSIS/Include', 'Drivers/CMSIS/Device/ST/STM32F0xx/Include',
                    'Drivers/STM32F0xx_HAL_Driver/Inc', 'MDK-ARM/RTE/_tw66gw02', 'Bootloader']
        flags = [compiler, '-c', '--cpu', 'Cortex-M0', '--c99', '-DSTM32F030x8', '-DUSE_HAL_DRIVER']
        for include in includes:
            flags += ['-I', str(project / include)]
        for index, name in enumerate(headers + ['COP/ota_update.h', 'OS/TaskScheduler.h']):
            unit = work / ('header_' + str(index) + '.c')
            unit.write_text('#include "' + name + '"\n#include "' + name + '"\nint header_check(void) { return 0; }\n', encoding='ascii')
            run('header-' + str(index), flags + ['-I', str(project), '-o', str(unit.with_suffix('.o')), str(unit)])
            assert 'warning' not in (EV / ('header-' + str(index) + '.log')).read_text(errors='replace').lower()
        ps = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File']
        run('factory-build', ps + [str(project / 'BuildKeilFactoryTarget.ps1')])
        run('ota-build', ps + [str(project / 'BuildOtaArtifacts.ps1'), '-FactoryVersion', '100', '-OtaVersion', '101'])
        for name in ('build_factory_keil.log', 'build_v100.log', 'build_v101.log'):
            data = (project / 'OTA_Artifacts' / name).read_bytes()
            assert b'0 Error(s), 0 Warning(s)' in data, name
            (EV / name).write_bytes(data)
        ns = {'__name__': 'verify_refactor', '__file__': str(project / 'tests/test_ota_artifacts.py')}
        with (EV / 'ota-tests.log').open('w', encoding='utf-8') as f, contextlib.redirect_stdout(f):
            exec(compile((MAIN / 'tests/test_ota_artifacts.py').read_text(encoding='utf-8'), 'test_ota_artifacts.py', 'exec'), ns)
            ns['main']()
        reference = ROOT / '05_发布与交付/MCU固件/main_v100-to-v101_DEV'
        compared = {}
        pairs = [('Bootloader/build/mathis_bootloader.bin', 'Bootloader/mathis_bootloader.bin'),
                 ('OTA_Artifacts/mathis_app_v100.bin', 'Debug_Only/mathis_app_v100.bin'),
                 ('OTA_Artifacts/mathis_app_v101.bin', 'Debug_Only/mathis_app_v101.bin')]
        for name in ('Factory_Flash/mathis_factory_v100.hex', 'App_Upload/mathis_ota_v101.ota', 'App_Upload/mathis_ota_v101.manifest.json'):
            pairs.append(('OTA_Artifacts/Release_v100_to_v101/' + name, name))
        for generated, name in pairs:
            assert (project / generated).read_bytes() == (reference / name).read_bytes(), generated
            compared[generated] = sha(project / generated)
        for name, data in entries.items():
            assert (work / NEW / name).read_bytes() == data, name
        for name, h in source_before.items():
            assert sha(Path(name)) == h, name
        for name, h in frozen.items():
            assert sha(ROOT / name) == h, name
        assert sha(original) == old_hash
        os.replace(temp, final)
        digest = sha(final)
        final.with_suffix('.zip.sha256').write_text(digest + '  ' + final.name + '\n', encoding='utf-8')
        report.update(result='PASS', header_checks=7, build='factory/v100/v101: 0 errors, 0 warnings',
                      tests='162 interrupted swap/rollback and artifact/layout checks PASS', compared_artifacts=compared,
                      zip_path=final.relative_to(ROOT).as_posix(), zip_sha256=digest, zip_bytes=final.stat().st_size,
                      file_count=len(entries), old_zip_unchanged=True, frozen_files_unchanged=len(frozen))
        save()
        print(json.dumps({k: v for k, v in report.items() if k not in {'commands', 'files', 'compared_artifacts'}}, ensure_ascii=True, indent=2), flush=True)
    except Exception as error:
        report.update(result='FAIL', error=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
