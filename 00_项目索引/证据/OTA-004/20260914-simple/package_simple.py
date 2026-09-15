"""Create the minimal handover; validate a real extraction without editing main."""
from pathlib import Path
from datetime import datetime
import contextlib
import hashlib
import json
import os
import re
import subprocess
import zipfile
import zlib

EV = Path(__file__).resolve().parent
ROOT = EV.parents[3]
OUT = ROOT / '05_发布与交付/工程师交接'
NAME = 'TW66GW02_工程交接_精简版'
BASE = ROOT / '01_固件工程/main'
FROZEN = ROOT / '05_发布与交付/MCU固件/main_v100-to-v101_DEV'
GUIDE = '''# TW66GW02 使用说明

## 目录

- [Keil 主工程](01_主程序/main/MDK-ARM/tw66gw02.uvprojx)：当前 main 源码及匹配 Bootloader。
- [客户需求](02_客户需求)：UI V1.6 原始表及解析、BLE v0.4 Draft、模块接口、温度校准资料。
- [硬件原理图](03_硬件原理图/TW66GW02-ON.pdf)。

## 编译

使用 Windows、Keil MDK 5.39 / ARM Compiler **5.06 update 7（build 960）**，默认安装目录 `C:\\Keil_v5`。
打开上述工程执行 Rebuild；预构建步骤会自动生成并嵌入 Bootloader。
也可在 `01_主程序/main` 中执行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\\BuildKeilFactoryTarget.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\\BuildOtaArtifacts.ps1 -FactoryVersion 100 -OtaVersion 101
```

新构建文件输出到 main 内的 `OTA_Artifacts`。包内随附固件保持原交付字节。

## 烧录与 OTA

1. 使用 [v100 完整工厂 HEX](01_主程序/固件/Factory_Flash/mathis_factory_v100.hex)。首次建立测试基线先 Full Chip Erase（清除原固件和保存的参数），再烧录并复位。完整 HEX 包含 Bootloader 和应用；不需要手填 HEX 加载地址。
2. 确认显示、按键、测温和蓝牙正常，App 遥测版本为 **100**。
3. 电量至少 30%，App 传输 [v101 OTA](01_主程序/固件/App_Upload/mathis_ota_v101.ota)，参数读取相邻 [manifest](01_主程序/固件/App_Upload/mathis_ota_v101.manifest.json)。目标版本 101、完整大小 23268 字节、CRC-32 `0x119B455F`。App 按 READY → 分块发送/ACK → COMMIT 流程升级。
4. 升级重连后确认版本 **101**，稳定运行超过 10 秒，再次复位确认仍为 101。

当前为 **无 RSA 验签的开发 OTA**；仍检查大小、CRC 和应用向量。OTA 尾部 384 字节是开发占位，不能删除，也不能用裸应用 BIN 代替 OTA 文件。
Bootloader 为 6 KiB，应用起址 `0x08001800`，烧录文件和 OTA 包须配套使用。

本包已从实际解压工程完成工厂及 v100/v101 构建与文件校验；本次未进行实机烧录验收。客户需求按原文提供，需求文档不是全部功能已验收的证明；UI 解析中注明缺失的控件图原资料未提供。
'''


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    final = OUT / (NAME + '.zip')
    temp = OUT / (NAME + '.zip.building')
    if final.exists() or temp.exists():
        raise RuntimeError('Existing package must be preserved; choose a new run/name.')
    baseline = json.loads((EV.parents[1] / 'OTA-010/20260911-step1/baseline.json').read_text(encoding='utf-8'))
    protected = {}
    for repo in baseline['repositories'].values():
        for row in repo['archive']['files']:
            path = ROOT / repo['repository_root'] / row['path']
            assert sha(path) == row['sha256'], str(path)
            protected[path] = row['sha256']
    previous = json.loads((EV.parents[1] / 'OTA-010/20260911-step3/result.json').read_text(encoding='utf-8'))
    for name, digest in previous['frozen_before'].items():
        path = ROOT / name
        assert sha(path) == digest, name
        protected[path] = digest
    old_zip = OUT / 'TW66GW02_HANDOVER_20260914_v1.zip'
    protected[old_zip] = sha(old_zip)
    selected = {}

    def add(path, destination):
        assert path.is_file() and not path.is_symlink(), str(path)
        assert destination not in selected
        selected[destination] = path

    build_scripts = {'BuildKeilFactoryTarget.ps1', 'BuildOtaArtifacts.ps1', 'PrepareFactoryBootImage.ps1'}
    for row in baseline['repositories']['main']['archive']['files']:
        path = ROOT / row['path']
        rel = path.relative_to(BASE)
        if rel.parts[0] not in {'BSP', 'COP', 'Core', 'Drivers', 'OS', 'MDK-ARM', 'Bootloader'} and rel.name not in build_scripts | {'tw66gw02.ioc'}:
            continue
        if rel.suffix == '.md' or rel.name in {'build_min_bootloader.ps1', 'min_boot_main.c', 'tw66gw02_direct.sct'}:
            continue
        add(path, '01_主程序/main/' + rel.as_posix())
    req = ROOT / '02_需求与协议'
    for name in ('UI操作逻辑/原始需求/UI_Operational_Logic_V1.6.xlsx',
                 'UI操作逻辑/解析/260323_Mathis_UI_Operational_Logic-V1.6_完整解析.md',
                 'BLE协议/Mathis_BLE_Specification_v0.4_Draft.md'):
        add(req / name, '02_客户需求/' + name)
    for folder in ('模块接口', '温度与校准', 'UI操作逻辑/解析/260323_Mathis_UI_Operational_Logic-V1.6_完整解析.assets'):
        for path in (req / folder).rglob('*'):
            if path.is_file() and path.name != 'README.md':
                add(path, '02_客户需求/' + path.relative_to(req).as_posix())
    add(ROOT / '03_硬件资料/TW66GW02-ON.pdf', '03_硬件原理图/TW66GW02-ON.pdf')
    for name in ('Factory_Flash/mathis_factory_v100.hex', 'App_Upload/mathis_ota_v101.ota', 'App_Upload/mathis_ota_v101.manifest.json'):
        add(FROZEN / name, '01_主程序/固件/' + name)
    generated = {'使用说明.md': GUIDE.encode('utf-8')}
    names = set(selected) | set(generated)
    for target in re.findall(r'\]\(([^)]+)\)', GUIDE):
        assert target in names or any(n.startswith(target + '/') for n in names), target
    for name, path in selected.items():
        assert not any(part in {'.git', '.venv', 'snapshot', 'history', 'private_keys'} for part in Path(name).parts)
        assert path.suffix.lower() not in {'.log', '.pem', '.key', '.pfx', '.p12'}
        if path.suffix.lower() in {'.c', '.h', '.md', '.ps1', '.json', '.txt'}:
            assert not re.search(rb'-----BEGIN (?:RSA |EC |OPENSSH |ENCRYPTED )?PRIVATE KEY-----', path.read_bytes())
    rows = [{'path': name, 'source': path.relative_to(ROOT).as_posix(), 'sha256': sha(path), 'bytes': path.stat().st_size} for name, path in sorted(selected.items())]
    rows += [{'path': name, 'source': 'generated', 'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data)} for name, data in generated.items()]
    print('Packaging ' + str(len(rows)) + ' files', flush=True)
    with zipfile.ZipFile(temp, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for name, path in sorted(selected.items()):
            z.write(path, NAME + '/' + name)
        for name, data in generated.items():
            z.writestr(NAME + '/' + name, data)
    work = ROOT / '90_历史原始包/整理前备份' / ('simple-' + datetime.now().strftime('%Y%m%d-%H%M%S'))
    work.mkdir(parents=True, exist_ok=False)
    with zipfile.ZipFile(temp) as z:
        assert z.testzip() is None
        for member in z.infolist():
            assert (work / member.filename).resolve().is_relative_to(work.resolve())
        z.extractall(work)
    extracted = work / NAME
    for row in rows:
        assert sha(extracted / row['path']) == row['sha256']
    project = extracted / '01_主程序/main'
    report = {'task': 'OTA-004', 'test': 'TEST-007', 'result': 'IN_PROGRESS', 'files': rows,
              'extracted_project': str(project), 'hardware': 'NOT_RUN', 'commands': [],
              'attachment_note': 'Original ui_control_map.png absent; source requirement note preserved; all available requested assets included.'}

    def save():
        (EV / 'result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

    save()
    try:
        ps = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File']
        commands = [('factory-build', ps + [str(project / 'BuildKeilFactoryTarget.ps1')]),
                    ('ota-build', ps + [str(project / 'BuildOtaArtifacts.ps1'), '-FactoryVersion', '100', '-OtaVersion', '101'])]
        for label, cmd in commands:
            print('RUN ' + label, flush=True)
            with (EV / (label + '.log')).open('wb') as stream:
                result = subprocess.run(cmd, cwd=project, stdout=stream, stderr=subprocess.STDOUT, timeout=300)
            report['commands'].append({'label': label, 'command': cmd, 'exit_code': result.returncode})
            save()
            assert result.returncode == 0, label
        for name in ('build_factory_keil.log', 'build_v100.log', 'build_v101.log'):
            data = (project / 'OTA_Artifacts' / name).read_bytes()
            assert b'0 Error(s), 0 Warning(s)' in data, name
            (EV / name).write_bytes(data)
        # Run the unchanged existing tests against the extracted project; do not add tests to the deliverable.
        source = (BASE / 'tests/test_ota_artifacts.py').read_text(encoding='utf-8')
        ns = {'__name__': 'handover_validation', '__file__': str(project / 'tests/test_ota_artifacts.py')}
        with (EV / 'ota-tests.log').open('w', encoding='utf-8') as stream, contextlib.redirect_stdout(stream):
            exec(compile(source, str(BASE / 'tests/test_ota_artifacts.py'), 'exec'), ns)
            ns['main']()
        built = project / 'OTA_Artifacts/Release_v100_to_v101'
        for name in ('Factory_Flash/mathis_factory_v100.hex', 'App_Upload/mathis_ota_v101.ota', 'App_Upload/mathis_ota_v101.manifest.json'):
            assert (built / name).read_bytes() == (FROZEN / name).read_bytes(), name
        artifact = (extracted / '01_主程序/固件/App_Upload/mathis_ota_v101.ota').read_bytes()
        manifest = json.loads((extracted / '01_主程序/固件/App_Upload/mathis_ota_v101.manifest.json').read_text(encoding='utf-8-sig'))
        assert len(artifact) == manifest['artifact_size'] == 23268
        assert zlib.crc32(artifact) == int(manifest['crc32_iso_hdlc'], 16)
        assert hashlib.sha256(artifact).hexdigest() == manifest['sha256']
        assert artifact[-384:] == bytes(384)
        for row in rows:
            assert sha(extracted / row['path']) == row['sha256'], row['path']
        for path, digest in protected.items():
            assert sha(path) == digest, str(path)
        os.replace(temp, final)
        digest = sha(final)
        final.with_suffix('.zip.sha256').write_text(digest + '  ' + final.name + '\n', encoding='utf-8')
        report.update(result='PASS', zip_path=final.relative_to(ROOT).as_posix(), zip_sha256=digest,
                      zip_bytes=final.stat().st_size, file_count=len(rows), unchanged_protected_files=len(protected),
                      build='factory/v100/v101: 0 errors, 0 warnings', tests='162 swap/rollback simulations and artifact/layout checks PASS',
                      archived_artifacts='Rebuilt HEX/OTA/manifest byte-identical to frozen files', zip_readback='PASS')
        save()
        print(json.dumps({k: v for k, v in report.items() if k not in ('files', 'commands')}, ensure_ascii=True, indent=2), flush=True)
    except Exception as error:
        report.update(result='FAIL', error=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
