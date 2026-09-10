"""Update navigation and provenance after the second organization step."""
from pathlib import Path
import csv
import hashlib
import json
import os

ROOT = Path(__file__).resolve().parents[1]
INDEX = ROOT / '00_项目索引'
BACKUP = ROOT / '90_历史原始包/整理前备份/20260909-step2'


def write(rel, text):
    p = ROOT / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text, encoding='utf-8')


def link(base, target):
    return '<' + Path(os.path.relpath(ROOT / target, ROOT / base)).as_posix() + '>'


def main():
    rows = json.loads((INDEX / '证据/第二步资料映射.json').read_text(encoding='utf-8'))
    for folder, description in [
        ('02_需求与协议', '原始需求、解析、协议和校准资料在此维护。UI V1.6 和 BLE v0.4 Draft 是各自的资料版本，不是固件版本。'),
        ('03_硬件资料', '两份 PDF 分别保留原文件名和历史日期标识。未重新核对图纸内部修订号，不凭修改时间判定硬件兼容性。'),
        ('04_测试与联调', '集中操作逻辑测试资料与测试清单。复制历史测试资料不代表本轮重新执行测试，也不证明各固件已通过验收。'),
    ]:
        lines = ['# ' + folder.split('_', 1)[1], '', '[返回项目入口](../README.md) · [文件用途索引](../00_项目索引/文件用途索引.md)', '', description, '', '公共目录是后续维护入口；历史工程中的原件继续保留。原路径、原名、原始与归集后 SHA-256 见 [资料映射](../00_项目索引/证据/第二步资料映射.csv)。', '', '| 资料 | 分类 |', '| --- | --- |']
        for row in rows:
            if row['target'].startswith(folder + '/'):
                p = Path(row['target'])
                lines.append(f'| [{p.name}]({link(folder, row["target"])}) | {p.parent.relative_to(folder).as_posix()} |')
        if folder == '02_需求与协议':
            lines += ['', 'UI 解析的控件图原本缺失，已在正文明确标注待补；资源目录与解析正文放在一起。模块接口公共副本以根主线版本为来源，版本专用接口说明仍留在各工程中。']
        if folder == '04_测试与联调':
            lines += ['', '版本专用工具与报告继续跟随工程：', '', '- [v104 电脑 BLE 工具](../13/tw66gw02/Tools/PC_BLE_Test/README.md)', '- [v104 构建与验证报告](../13/tw66gw02/V104_BUILD_REPORT.md)', '- [v103 联调指南](../12/tw66gw02/V103_INTEGRATION_TEST_GUIDE.md)', '- [TEST 实验工程](../TEST/)、[ostest](../ostest/)、[eide](../eide/)', '', '费用报销工作簿不属于产品测试资料，本轮未归入此目录。']
        write(folder + '/README.md', '\n'.join(lines) + '\n')

    artifacts = []
    for folder in ['apk', 'ble', 'emb1061']:
        for p in sorted((ROOT / folder).iterdir()):
            if not p.is_file():
                continue
            with p.open('rb') as f:
                digest = hashlib.file_digest(f, 'sha256').hexdigest()
            item = {'path': p.relative_to(ROOT).as_posix(), 'original_name': p.name, 'bytes': p.stat().st_size, 'sha256': digest, 'origin': '本地既有文件；提供方与接收日期待核实', 'compatible_firmware': '待联调核实', 'validation': '仅文件哈希/可读取性；未安装、烧录或进行设备联调', 'version': '待核实', 'build': '待核实'}
            if p.suffix == '.apk':
                item.update(platform='Android', version='3.6.9', build='368', package='com.ooni.app', evidence='包内 AndroidManifest.xml 二进制 XML 只读解析')
            elif p.suffix == '.ipa':
                item.update(platform='iOS', version='3.6.9', build='98', package='com.goodbarber.uuni', minimum_os='16.4', evidence='包内 Payload/*.app/Info.plist 只读解析')
            else:
                item.update(platform='BLE模块固件或烧录配置', evidence='本地文件名及所在目录；文件名日期/型号未作为实际版本证据')
            artifacts.append(item)
    write('05_发布与交付/现有安装包与模块固件.json', json.dumps(artifacts, ensure_ascii=False, indent=2) + '\n')
    lines = ['# 发布与交付入口', '', '[返回项目入口](../README.md) · [版本总表](../00_项目索引/版本总表.md)', '', '本轮建立登记，不移动或重新发布安装包、MCU 产物和模块固件。Android/iOS 版本由包内元数据读取，不能据此推断它们与某一快照兼容。来源、适配关系和实机验证状态待补。', '', '| 文件 | 平台/作用 | 实际版本 | 构建号 |', '| --- | --- | --- | --- |']
    for a in artifacts:
        lines.append(f'| [{a["original_name"]}]({link("05_发布与交付", a["path"])}) | {a["platform"]} | {a["version"]} | {a["build"]} |')
    lines += ['', '详细大小、SHA-256、包标识及元数据依据见 [登记清单](现有安装包与模块固件.json)。两个平台的包标识不同，按实际读取值分别记录。', '', 'MCU 开发交付仍使用对应工程入口：', '', '- [主线 OTA 说明](../OTA_DEVELOPMENT_README.md)', '- [v103 App_Delivery](../12/tw66gw02/App_Delivery/)', '- [v104 App_Delivery](../13/tw66gw02/App_Delivery/)', '', '不要混用不同 Bootloader 分区的 Factory、App OTA 包。正式交付目录的分层与验证流程留待第四步实施。']
    write('05_发布与交付/README.md', '\n'.join(lines) + '\n')

    report = json.loads((BACKUP / 'backup_report.json').read_text(encoding='utf-8'))
    text = '# 整理前备份与恢复说明\n\n[返回项目入口](../../../README.md) · [备份报告](backup_report.json)\n\n这些是同磁盘上的本地恢复副本，包含文件内容，不只是指纹。每份 ZIP 保留相对于原工作区根目录的路径。根主线/共享文件及 12、13 内的 Git 元数据一并归档，未提交文件也在各自范围内保留。备份时间点在第二步资料归集和索引修改之前。\n\n| 归档 | 文件数 | 原始字节 | ZIP 字节 |\n| --- | ---: | ---: | ---: |\n'
    for a in report['archives']:
        text += f'| [{a["archive"]}]({a["archive"]}) | {a["files"]} | {a["source_bytes"]} | {a["zip_bytes"]} |\n'
    text += '\n每份 ZIP 都已逐成员解压读取校验 SHA-256，并在备份完成时重新读取源文件核对。各 `*.files.csv` 保存逐文件大小和指纹；总包指纹见 backup_report.json。总计 11,734 个文件。\n\n## 恢复方法\n\n1. 新建一个空的恢复目录，先把所需 ZIP 解压到这个目录，不直接覆盖当前工作区。\n2. main-and-shared.zip 的内容对应原工作区根；snapshot-12/13/14.zip 内分别已有 12/13/14 前缀，应解压到同一个恢复根，不再套同名版本目录。\n3. 按对应 files.csv 核对解压后的相对路径、大小和 SHA-256。打开恢复目录中的工程进行检查；根和 12/13 的 .git 可用于核对本地历史与未提交文件。\n4. 确认恢复副本无误后，再选择需要替换的文件。不要整包覆盖当前工作区，以免覆盖备份后新增工作。\n\n## 明确未包含的内容\n\n- 原有 1–11 快照未进入本组备份，本轮保持原样。共享目录中的 TEST、ostest、eide 等已包含在 main-and-shared.zip 中。\n- 12/private_keys 未复制；13/14 的 Python 环境、deps、__pycache__ 按报告排除。私钥与依赖目录均保留原位。\n- 因此本组可以恢复归档范围内的源码、配置、脚本、资料、产物和 Git 元数据，但不是可在新电脑直接完成签名/运行的整机环境备份。\n- 13 交付脚本仍引用原 12/private_keys 绝对路径；恢复到其他目录后需另行配置 KeyRoot。\n- ZIP 校验不代表固件重新编译或实机测试通过。\n'
    write('90_历史原始包/整理前备份/20260909-step2/README.md', text)
    write('90_历史原始包/README.md', '# 历史原始包\n\n[返回项目入口](../README.md)\n\n[第二步整理前备份与恢复说明](整理前备份/20260909-step2/README.md)包含主线/共享文件及 12/13/14 的恢复包和校验范围。此子目录已加入根 .gitignore，备份不会随普通源码提交加入仓库。\n')

    # Rewrite local references in the maintained navigation pages only.
    for doc in [ROOT / 'README.md', INDEX / '文件用途索引.md']:
        content = doc.read_text(encoding='utf-8')
        for row in rows:
            old = Path(os.path.relpath(ROOT / row['source'], doc.parent)).as_posix()
            new = Path(os.path.relpath(ROOT / row['target'], doc.parent)).as_posix()
            content = content.replace('](' + old + ')', '](' + new + ')').replace('](<' + old + '>)', '](<' + new + '>)')
        content = content.replace('第一步已建立索引；物理迁移尚未执行', '第二步已完成备份和公共资料归集；工程位置保持不变')
        content = content.replace('14 的修改需要单独保全。', '14 的当前副本已完成第二步本地备份，后续修改仍需单独版本管理。')
        content = content.replace('所有链接指向现有位置。公共资料未来可以集中存放，工程专用源码、脚本、测试及验证记录继续跟随版本。', '公共资料已归集到 02/03/04 分类目录，本索引优先指向公共维护副本。工程专用源码、脚本、测试及验证记录继续跟随版本；原件与公共副本的关系见第二步资料映射。')
        content = content.replace('(../260323_Mathis_UI_Operational_Logic-V1.6_完整解析.assets/)', '(../02_需求与协议/UI操作逻辑/解析/260323_Mathis_UI_Operational_Logic-V1.6_完整解析.assets/)')
        content = content.replace('当前为空；文档引用的 ui_control_map.png 缺失', '与解析同组归集；控件图缺失，正文已明确标注待补')
        content = content.replace('PT1000 表、早期要求以及参考工程；表格原文件名存在显示编码问题，暂不改名', '早期要求及参考工程；PT1000 表已按原名归集到公共温度与校准目录，此前的乱码是终端显示问题')
        content = content.replace('Android APK 和 iOS IPA；应用版本与兼容固件尚待核实，不从 UUID 文件名推断', 'Android/iOS 均为 3.6.9，构建号分别 368/98；兼容固件待核实，详见交付登记')
        content = content.replace('- 根目录 UI 完整解析与 8、9 内同名文件相同。', '- 归集前根目录 UI 完整解析与 8、9 内同名文件相同；根目录现为导航页，公共正文增加了缺图待补说明。')
        content = content.replace('当前只是统一入口，不删除副本。后续集中公共资料时保留来源与校验记录，历史快照的完整性另行维护。', '第二步已建立公共维护副本及源/目标 SHA-256 映射。两份根目录 Markdown 改为导航入口，其他来源原件保留；1–14 工程目录未因归集而修改。')
        content += '\n## 第二步归集入口\n\n'
        for title, target in [('需求与协议', '02_需求与协议/README.md'), ('硬件资料', '03_硬件资料/README.md'), ('测试与联调', '04_测试与联调/README.md'), ('安装包与固件登记', '05_发布与交付/README.md'), ('备份与恢复说明', '90_历史原始包/整理前备份/20260909-step2/README.md'), ('资料来源与指纹映射', '00_项目索引/证据/第二步资料映射.csv')]:
            content += f'- [{title}]({link(doc.parent.relative_to(ROOT), target)})\n'
        doc.write_text(content, encoding='utf-8')

    p = INDEX / '架构与迁移计划.md'
    content = p.read_text(encoding='utf-8').replace('## 第一步：建立可用入口（本轮）', '## 第一步：建立可用入口（已完成）').replace('## 第二步：保全开发线并归集公共资料（待执行）', '## 第二步：保全开发线并归集公共资料（已完成）')
    content = content.replace('完成标准：公共资料可从 README 打开；所有新链接有效；原文件与副本可追溯；开发线能够恢复到迁移前状态。', '完成情况：已生成四份经逐文件校验的本地恢复包，归集 24 份公共资料，保留来源和源/目标指纹，并核实 APP 包内版本。源码、配置、产物和仓库元数据可按归档范围恢复；私钥、依赖环境及未变更的 1–11 快照不包含在这组备份中，详见 [恢复说明](../90_历史原始包/整理前备份/20260909-step2/README.md)。工程位置未调整。')
    content = content.replace('目前除 `00_项目索引` 外尚未创建这些分类目录。', '00、02、03、04 已实际建立；05 已建立交付登记入口，90 已保存备份。01 固件工程和 06 工具分类尚未迁移。')
    p.write_text(content, encoding='utf-8')

    p = INDEX / '版本总表.md'
    content = p.read_text(encoding='utf-8').replace('**指纹用于核对变化，不是源码备份，也不证明构建产物与当前源码匹配。**', '**指纹用于核对变化，不是源码备份，也不证明构建产物与当前源码匹配。** 第二步另外建立了含文件内容的 [本地恢复备份](../90_历史原始包/整理前备份/20260909-step2/README.md)，保全范围和环境排除项以恢复说明为准。')
    content = content.replace('先建立可恢复备份或独立版本管理，再考虑移动', '已建立第二步本地恢复包；后续新增修改仍需独立版本管理，工程尚未移动')
    p.write_text(content, encoding='utf-8')


if __name__ == '__main__':
    main()
