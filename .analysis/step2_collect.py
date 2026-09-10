"""Collect public maintenance copies after verified step-two backup."""
from pathlib import Path
import csv
import hashlib
import json
import os
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]
BACKUP = ROOT / '90_历史原始包/整理前备份/20260909-step2'


def sha(p):
    with p.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def main():
    report = json.loads((BACKUP / 'backup_report.json').read_text(encoding='utf-8'))
    assert len(report['archives']) == 4
    assert all(x['files'] == x['verified_members'] for x in report['archives'])
    pairs = [
        ('4/260323_Mathis_UI_Operational_Logic-V1.6.xlsx', '02_需求与协议/UI操作逻辑/原始需求/UI_Operational_Logic_V1.6.xlsx'),
        ('3/260323_Mathis_UI_Operational_Logic-V1.5.xlsx', '02_需求与协议/UI操作逻辑/原始需求/UI_Operational_Logic_V1.5.xlsx'),
        ('260323_Mathis_UI_Operational_Logic-V1.6_完整解析.md', '02_需求与协议/UI操作逻辑/解析/260323_Mathis_UI_Operational_Logic-V1.6_完整解析.md'),
        ('Mathis_operation_process.md', '02_需求与协议/UI操作逻辑/解析/Mathis_operation_process.md'),
        ('10/mathis-ble-specification.md', '02_需求与协议/BLE协议/Mathis_BLE_Specification_v0.4_Draft.md'),
        ('10/mathis-ble-specification_old.md', '02_需求与协议/BLE协议/Mathis_BLE_Specification_v0.3_Draft.md'),
        ('3/protocolv1.0.docx', '02_需求与协议/BLE协议/历史协议/protocolv1.0.docx'),
        ('3/protocolv1.0_en.docx', '02_需求与协议/BLE协议/历史协议/protocolv1.0_en.docx'),
        ('3/protocolv1.0_fixed.docx', '02_需求与协议/BLE协议/历史协议/protocolv1.0_fixed.docx'),
        ('5/protocolv1.docx', '02_需求与协议/BLE协议/历史协议/protocolv1.docx'),
        ('EMB1082_蓝牙接口说明.md', '02_需求与协议/模块接口/EMB1082_蓝牙接口说明.md'),
        ('TEST/base command.docx', '02_需求与协议/模块接口/base command.docx'),
        ('TEST/EMB101x系列合家亲蓝牙 AT 指令.docx', '02_需求与协议/模块接口/EMB101x系列合家亲蓝牙 AT 指令.docx'),
        ('TEST/功能指令.docx', '02_需求与协议/模块接口/功能指令.docx'),
        ('1/Pt1000温度阻值对照表.xls', '02_需求与协议/温度与校准/Pt1000温度阻值对照表.xls'),
        ('4/Coefficient proposal MATHIS.pptx', '02_需求与协议/温度与校准/Coefficient proposal MATHIS.pptx'),
        ('TW66GW02-ON.pdf', '03_硬件资料/TW66GW02-ON.pdf'),
        ('ostest/TW66GW02-ON_260408.pdf', '03_硬件资料/历史版本/TW66GW02-ON_260408.pdf'),
        ('Mathis—Operational logic test-20260521.pptx', '04_测试与联调/操作逻辑/Mathis—Operational logic test-20260521.pptx'),
        ('TEST/蓝牙单功能测试链.md', '04_测试与联调/BLE联调/蓝牙单功能测试链.md'),
    ]
    outputs = ROOT / '10/outputs/01a01df3-f219-70e3-823c-0747ab78d408'
    for p in sorted(outputs.rglob('*')):
        if p.is_file() and p.suffix.lower() in {'.xlsx', '.pdf'}:
            pairs.append((p.relative_to(ROOT).as_posix(), '04_测试与联调/测试清单/' + p.relative_to(outputs).as_posix()))
    for source, target in pairs:
        assert (ROOT / source).is_file(), source
        assert not (ROOT / target).exists(), target
    assets_name = '260323_Mathis_UI_Operational_Logic-V1.6_完整解析.assets'
    assets_src = ROOT / assets_name
    assets_dst = ROOT / '02_需求与协议/UI操作逻辑/解析' / assets_name
    rows = []
    for source, target in pairs:
        src, dst = ROOT / source, ROOT / target
        dst.parent.mkdir(parents=True, exist_ok=True)
        original_sha = sha(src)
        shutil.copy2(src, dst)
        assert sha(dst) == original_sha
        note = '逐字节复制；原件保留，公共目录作为后续维护入口'
        if dst.suffix.lower() == '.md':
            text = dst.read_text(encoding='utf-8-sig')
            if src.parent == ROOT and src.name.startswith('260323_'):
                image = f'![Mathis UI 控件编号图]({assets_name}/ui_control_map.png)'
                assert image in text
                text = text.replace(image, f'> 资源待补：Mathis UI 控件编号图 `ui_control_map.png` 当前缺失。对应资源目录为 `{assets_name}/`；补齐后恢复图片引用。')
                note += '；缺失图片引用改为明确的待补说明'
            # Existing links in copied Markdown resolve against their original directory.
            def relocate(m):
                label, raw = m.group(1), m.group(2)
                url = raw.strip('<>')
                if url.startswith(('http:', 'https:', 'mailto:', '#')):
                    return m.group(0)
                path, sep, anchor = url.partition('#')
                origin = (src.parent / path).resolve()
                relative = Path(os.path.relpath(origin, dst.parent)).as_posix()
                return label + '(<'+relative+(sep+anchor if sep else '')+'>)'
            text = re.sub(r'(!?\[[^\]]*\])\((<[^>]+>|[^\s)]+)\)', relocate, text)
            if text != dst.read_text(encoding='utf-8-sig'):
                dst.write_text(text, encoding='utf-8')
                if '待补' not in note:
                    note += '；调整相对链接'
        rows.append({'source': source, 'original_name': src.name, 'source_sha256': original_sha, 'target': target, 'target_sha256': sha(dst), 'action': note})
    shutil.copytree(assets_src, assets_dst)
    # Preserve old IDE/bookmark paths as navigation pages, after the full content is copied.
    for source, target in pairs:
        if source in {'260323_Mathis_UI_Operational_Logic-V1.6_完整解析.md', 'Mathis_operation_process.md'}:
            (ROOT / source).write_text('# 文档已归集\n\n请打开 [' + Path(source).stem + '](' + target + ')。\n\n后续内容在上述公共目录维护。本文件保留原路径作为导航入口；归集前正文已保存在第二步备份中。\n', encoding='utf-8')
    evidence = ROOT / '00_项目索引/证据'
    with (evidence / '第二步资料映射.csv').open('w', encoding='utf-8-sig', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader(); writer.writerows(rows)
    (evidence / '第二步资料映射.json').write_text(json.dumps(rows, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('Collected public files:', len(rows))


if __name__ == '__main__':
    main()
