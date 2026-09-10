"""Publish the current architecture after relocation and verification."""
from pathlib import Path
import json,os
ROOT=Path(__file__).resolve().parents[1]
INDEX=ROOT/'00_项目索引';OUT=INDEX/'证据/第三步'
maps=json.loads((OUT/'路径映射.json').read_text(encoding='utf-8'))
def write(rel,text):
    p=ROOT/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text,encoding='utf-8')
def relative(base,target):return '<'+Path(os.path.relpath(ROOT/target,ROOT/base)).as_posix()+'>'
def translate(rel):
    for m in sorted(maps,key=lambda x:len(x['source']),reverse=True):
        if rel==m['source'] or rel.startswith(m['source']+'/'):return m['target']+rel[len(m['source']):]
    return rel

write('README.md','''# TW66GW02 项目入口

第三步已完成工程目录迁移。主工程、开发线、历史快照及实验工具按用途分区；目录编号仍表示快照，不能当作固件上报版本或量产发布顺序。

## 常用入口

| 要做的事 | 入口 |
| --- | --- |
| 打开主线工程 | [main 工程说明](01_固件工程/main/README.md)、[Keil 工程](01_固件工程/main/MDK-ARM/tw66gw02.uvprojx) |
| 选择开发线或历史版本 | [固件工程导航](01_固件工程/README.md)、[版本总表](00_项目索引/版本总表.md) |
| 在 VS Code 中按工程浏览 | [多文件夹工作区](TW66GW02.code-workspace) |
| 查看需求、协议与操作流程 | [需求与协议](02_需求与协议/README.md) |
| 查看硬件资料 | [硬件资料](03_硬件资料/README.md) |
| 查看测试与联调资料 | [测试与联调](04_测试与联调/README.md) |
| 查看安装包与固件登记 | [发布与交付入口](05_发布与交付/README.md) |
| 查看 J-Link/J-Flash 工具 | [工具入口](06_工具与参考/README.md) |
| 查询全部资料和迁移结果 | [文件用途索引](00_项目索引/文件用途索引.md)、[第三步报告](00_项目索引/第三步迁移与验证报告.md) |

## 仓库与版本边界

根 `.git` 继续管理主工程和公共索引，主工程现位于 `01_固件工程/main`。历史快照及开发线继续排除在根仓库普通提交之外。

12、13 保留各自 `tw66gw02/.git`；14 在快照外层建立独立 Git 仓库，覆盖固件、工作簿脚本和回归基线。14 尚无首次提交，当前可恢复副本由整理前备份保全。本轮没有暂存、提交、推送或改变主线选择。

源码、固件、UI 需求、BLE 协议分别维护自己的版本号。12/13 的安全 OTA 与 main/10/14 的开发 OTA 使用不同分区，交付时必须核对对应 Bootloader 和 manifest。

## 本轮验证与已知限制

主线和 14 的构建、OTA 产物检查及各 162 个中断恢复用例通过；14 的 43 项脚本检查和 260 项 C 检查通过。v103/v104 应用按各自安全 OTA 流程中的应用构建函数编译通过，13 的电脑 BLE 离线自检通过。

12 的旧工厂构建入口与其当前 Bootloader 分区不兼容；14 的工作簿数据测试在原备份与迁移后均失败。详见第三步报告。这些问题没有被标记为通过；本轮未做硬件烧录、真实 BLE 或签名交付验证。

后续安排见 [架构与迁移计划](00_项目索引/架构与迁移计划.md)。恢复入口见 [第二步备份说明](90_历史原始包/整理前备份/20260909-step2/README.md) 和 [第三步迁移恢复说明](90_历史原始包/整理前备份/20260909-step3/README.md)。
''')

lines=['# 固件工程导航','','[返回项目入口](../README.md) · [版本总表](../00_项目索引/版本总表.md)','','| 工程/快照 | 当前路径 | 仓库边界 |','| --- | --- | --- |', '| main | [主线说明](main/README.md) | 根工作区 .git |']
for m in maps:
    if m['group'] not in {'history','development'}:continue
    target=m['target'];boundary='历史快照，根仓库忽略'
    if m['source'] in ['12','13']:boundary='tw66gw02 内独立 .git；外层资料由备份保全'
    if m['source']=='14':boundary='快照外层独立 .git；尚无首次提交'
    lines.append(f'| snapshot-{int(m["source"]):02d} | [{Path(target).name}]({relative("01_固件工程",target)}) | {boundary} |')
lines+=['','2 不具备完整本地工程；11 缺少历史构建配置，均保持原样归档。12/13 应使用安全 OTA 流程，不使用继承的旧 6 KiB 工厂构建入口。14 是基于 10 的重构线，不包含所有 12/13 功能。','','路径与逐文件迁移证据见 [第三步报告](../00_项目索引/第三步迁移与验证报告.md)。']
write('01_固件工程/README.md','\n'.join(lines)+'\n')
write('01_固件工程/main/README.md','''# main 主工程

[返回工程导航](../README.md) · [项目入口](../../README.md)

该工程由原工作区根目录整体迁入，根工作区 `.git` 保持原位。入口为 [Keil 工程](MDK-ARM/tw66gw02.uvprojx)。BSP、COP、Core、Drivers、OS、Bootloader、工程配置、构建脚本和测试保持原相对结构。

构建使用 [BuildOtaArtifacts.ps1](BuildOtaArtifacts.ps1)，说明见 [OTA 开发文档](OTA_DEVELOPMENT_README.md) 和 [烧录联调说明](OTA_烧录与APP联调说明.md)。Windows PowerShell 5.1 环境中，可从当前工程目录执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\BuildOtaArtifacts.ps1
python .\tests\test_ota_artifacts.py
```

2026-09-09 已在迁移后路径生成 v100 工厂镜像及 v101 OTA 包，并通过产物、内存布局、CRC 和 162 项中断交换/回滚检查。本轮没有烧录设备。

这里使用 6 KiB Bootloader 和开发签名占位。现有产物属于开发验证；不能与 12/13 的 8 KiB 安全 OTA 包混用。
''')

for n in [12,13,14]:
    target=next(m['target'] for m in maps if m['source']==str(n))
    if n in [12,13]:
        ver=103 if n==12 else 104
        text=f'''# snapshot-{n} 开发线

[工程导航](../../README.md) · [Keil 工程](tw66gw02/MDK-ARM/tw66gw02.uvprojx)

原 {n} 目录整体迁入；`tw66gw02/.git` 保持原有历史和工作区文件。外层资料不属于这个嵌套仓库，根仓库也忽略此开发线，请结合整理前备份保存。

应用版本线为 v{ver}，使用 8 KiB Bootloader。安全流程见 [联调指南](tw66gw02/V{ver}_INTEGRATION_TEST_GUIDE.md)。本轮单独执行原 BuildOtaArtifacts.ps1 中的应用构建函数，应用编译通过；没有访问私钥或重新签名交付。

继承的 BuildKeilFactoryTarget.ps1 及默认工厂链接配置仍属于旧 6 KiB 流程。12 上已实际复现链接重叠，13 也保留同类旧配置；不要将该旧入口当作当前安全版本的工厂交付入口。本轮仅记录，未更改固件分区。
'''
        if n==12:text+='\n`private_keys` 随原目录整体迁入，保持相对于本工程的位置；未读取内容，也没有复制到公共资料或备份中。\n'
        else:text+='\nBuildV104Delivery.ps1 的默认 KeyRoot 已改为相对于开发目录定位 snapshot-12_secure-ota/private_keys，仍可显式传入其他 KeyRoot。原未提交的 App_Delivery/program_test.txt 保留。电脑 BLE 工具已通过迁移后的离线 selftest，未连接真实设备。\n'
    else:
        text='''# snapshot-14 重构开发线

[工程导航](../../README.md) · [Keil 工程](tw66gw02/MDK-ARM/tw66gw02.uvprojx)

原 14 目录整体迁入。独立 Git 仓库位于此层，分支名 refactor-from-10，覆盖固件工程、外层工作簿脚本、公共脚本和测试所需重构基线。当前尚无首次提交；原内容由整理前 ZIP 备份保全，后续新增改动仍需提交到此独立仓库。

`.gitignore` 排除构建产物、运行缓存及依赖环境，保留 outputs/optimization/baseline 与辅助 Python 脚本，不把源码基线当作缓存删除。

迁移后验证：工厂/OTA 构建通过；启动诊断产物已按当前源码重新生成；OTA 产物与 162 项中断恢复检查通过；43 项脚本检查、260 项编译后 C 检查和 13 个头文件独立编译通过。

Test-WorkbookData.ps1 在“每个用例应有 13 个单元格”处失败，原始备份隔离复测得到相同结果。本轮没有将它修成通过，也未生成新的工作簿；后续需要单独排查其数据或测试逻辑。

CHANGLEOG 的原有内容未在本轮重写；它仍不覆盖全部重构工作。实际源码差异和整理验证见工作区版本总表与第三步报告。
'''.replace('CHANGLEOG','CHANGELOG')
    write(target+'/README.md',text)

write('06_工具与参考/README.md','''# 工具与参考

[返回项目入口](../README.md)

- [J-Link 工具目录](JLink/)：由根目录 JLink 整体迁入；现有烧录脚本默认使用 C:\\Keil_v5\\ARM\\Segger\\JLink.exe，未改变该工具路径。
- [J-Flash 配置](JFlash配置/tw66gw02_snapshot10.jflash)：原根目录 tw66gw02.jflash 实际选择 10 版镜像。配置已改名并更新该镜像的绝对路径，目标仍为 snapshot-10，未执行烧录。

历史参考工程继续跟随各快照归档，入口见固件工程导航。手机安装包及 BLE 模块固件保留原位，待第四步整理交付目录。
''')
write('04_测试与联调/实验工程/README.md','''# 实验工程

[返回测试与联调](../README.md)

- [TEST](TEST/)：历史联调与测试工程。
- [ostest](ostest/)：调度/系统实验工程。
- [eide](eide/)：引用相邻 ostest 源码的 EIDE 配置。

三个目录整体迁入，eide 和 ostest 保持相邻。原实验内容和 Git.disabled 元数据保留；本轮核对配置路径，不声称已重新编译所有历史实验工程。
''')

folders=[{'name':'主线 main','path':'01_固件工程/main'}, {'name':'版本与索引','path':'00_项目索引'}]
for n in [12,13,14]:
    target=next(m['target'] for m in maps if m['source']==str(n));folders.append({'name':f'开发 snapshot-{n}','path':target})
for name in ['02_需求与协议','03_硬件资料','04_测试与联调','05_发布与交付','06_工具与参考']:folders.append({'path':name})
write('TW66GW02.code-workspace',json.dumps({'folders':folders,'settings':{'files.autoGuessEncoding':True}},ensure_ascii=False,indent=2)+'\n')

# Keep historical evidence immutable; provide a separate current-path projection.
projection=[]
for x in json.loads((INDEX/'证据/第二步资料映射.json').read_text(encoding='utf-8')):
    projection.append({'original_source':x['source'],'current_source':translate(x['source']),'public_target':x['target'],'original_source_sha256':x['source_sha256'],'public_target_sha256':x['target_sha256']})
(OUT/'公共资料当前路径.json').write_text(json.dumps(projection,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
write('.analysis/README.md','''# 分析与整理脚本

本目录保存历史差异和本轮分阶段整理脚本。早期 compare_versions.py、step2_* 脚本按迁移前目录布局编写，是历史复核/一次性整理记录，不能直接针对当前布局重跑。需要复现时先在隔离目录恢复对应备份。

第三步的原路径→新路径与迁移前指纹位于 00_项目索引/证据/第三步。Move-Step3Projects.ps1 也不是日常构建命令：迁移已经完成，重复运行会因源路径不存在而停止。

日常开发从根 README 和各工程 README 进入。历史 CSV/JSON 的旧路径保留为取证记录，当前公共资料路径另见第三步的公共资料当前路径.json。
''')

p=INDEX/'版本总表.md';t=p.read_text(encoding='utf-8')
t=t.replace('现有数字目录尚未改名','数字目录已迁到 history/development 下对应的 snapshot 目录')
t=t.replace('当前工程入口','当前工程入口（迁移后）').replace('[根目录]','[main]')
t=t.replace('根目录主线与 10 各选取','迁移前的根目录主线与 10 各选取')
t=t.replace('| 14 | 被根仓库忽略；本次未发现独立 .git | 已建立第二步本地恢复包；后续新增修改仍需独立版本管理，工程尚未移动 |','| 14 | 已迁入 development；快照外层新建独立 .git，尚无首次提交 | 根仓库继续忽略；恢复包保全原内容，后续改动在该独立仓库维护 |')
t+='\n## 第三步当前状态\n\n主工程现位于 01_固件工程/main；12/13 的嵌套仓库整体保留，14 的独立仓库边界位于快照外层。主仓库没有提交路径迁移，因此 Git 会显示原路径删除和新目录未跟踪，这不代表文件丢失。逐文件校验、构建结果和原有失败项见 [第三步报告](第三步迁移与验证报告.md)。\n'
p.write_text(t,encoding='utf-8')
p=INDEX/'文件用途索引.md';t=p.read_text(encoding='utf-8').replace('## 主工程结构','## 主工程结构（现位于 01_固件工程/main）').replace('根目录脚本：','main 工程脚本：').replace('1–14 工程目录未因归集而修改。','第二步未修改工程目录；第三步已完成目录迁移，旧来源路径到当前位置见第三步路径映射。')
t+='\n## 第三步工程入口\n\n[固件工程导航](../01_固件工程/README.md) · [实验工程](../04_测试与联调/实验工程/README.md) · [工具目录](../06_工具与参考/README.md) · [迁移报告](第三步迁移与验证报告.md)\n\n第二步资料映射的 source 字段保留历史路径；当前来源位置见 [公共资料当前路径](证据/第三步/公共资料当前路径.json)。\n'
p.write_text(t,encoding='utf-8')
p=INDEX/'架构与迁移计划.md';t=p.read_text(encoding='utf-8').replace('## 第三步：整理工程和仓库边界（待执行）','## 第三步：整理工程和仓库边界（已完成迁移，原有验证失败项见报告）').replace('01 固件工程和 06 工具分类尚未迁移。','01 固件工程、实验工程和 06 工具分类现已完成迁移。')
t+='\n## 第三步执行结果\n\n42 项目录/文件迁移完成，迁移后修改链接和构建前，15,919 个纳入指纹的文件全部一致。根 .git 留在原位；12/13 保留嵌套仓库；14 初始化独立仓库但没有提交。主线、14 和 v103/v104 的适用构建验证已执行，原有不兼容入口与失败测试见 [第三步报告](第三步迁移与验证报告.md)。第三步没有改变固件分区，也没有清理历史快照。\n'
p.write_text(t,encoding='utf-8')
print('Published current architecture navigation')
