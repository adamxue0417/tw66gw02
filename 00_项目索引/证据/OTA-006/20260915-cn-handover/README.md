# OTA-006 / TEST-045：精简中文注释交接 v3

结果：**PASS（离线）**；实机 **NOT_RUN**，设备验收 **BLOCKED**（待设备/App 组合）。

## 来源和范围

- 基线：`6bd626d3390f025e3b486585e34d397e571613ec`，与精简版 v2 的 164 个工程文件字节一致。
- 源码提交：`8bff5517d895afb6a8c592af9437147daf8f3142`；分支 `docs-main-handover-cn`。
- 既有 `docs` 引用阻止创建 `docs/main-handover-cn`，故使用上述名称并保留旧历史；见主计划 CHG-011 / DEC-012。
- 16 个业务 C/H 文件只改中文注释；严格按原 GBK/UTF-8 解码，保留既有换行类型。C token 比较相同，人工核对状态/单位/边界；Bootloader、厂商库和原构建工具字节不变。Git 差异检查按 `core.whitespace=cr-at-eol` 兼容原 CR 换行。
- 包内保留 v2 的 178 文件白名单，修改注释与唯一使用说明。原工作区、battery_adjustment、旧包及冻结件未改动。

## 验证

| 检查 | 结果与证据 |
| --- | --- |
| 注释边界及 v2 指纹 | PASS；[comment-audit.json](comment-audit.json) |
| ZIP 实际解压工厂构建 | 0 错误、0 警告；[build_factory_keil.log](build_factory_keil.log)、[命令输出](factory-build.log) |
| ZIP 实际解压 v100/v101 构建 | 0 错误、0 警告；[build_v100.log](build_v100.log)、[build_v101.log](build_v101.log)、[命令输出](ota-build.log) |
| OTA、布局、CRC 和 162 项中断恢复模拟 | PASS；[ota-tests.log](ota-tests.log) |
| 六项重建产物逐字节比较冻结件 | PASS；[result.json](result.json) 的 compared_artifacts |
| ZIP 178 文件读回及源码提交关联 | PASS；[逐文件 SHA-256](files.sha256)及 result.json |
| 全部 Markdown 本地链接 19 项 | PASS；[isolation-check.json](isolation-check.json) |
| 原工作区/省电分支/旧包/冻结件 | PASS；1126 个受保护文件、原索引和既有 Git 状态不变 |

## 交付

新包放在原工作区 `05_发布与交付/工程师交接/TW66GW02_工程交接_精简版_v3_中文注释.zip`，大小 2372453 字节。ZIP 不入 Git；[同名校验文件](TW66GW02_工程交接_精简版_v3_中文注释.zip.sha256)随包交付并在此留档。

SHA-256：`0def601441f713ef1df0b36ebf2d0133d6625017004a9f789f74aa5a9bde2b17`。

回退使用保留的 v2，或在当前分支新增 revert 提交撤销注释；不覆盖冻结件、不强推。下一步由接手工程师按包内说明烧录 v100、实测 v101 OTA，记录设备和实际结果。

## 复核入口

`audit_comments.py --workspace-root <原工作区>` 可复核基线和注释。`package_verify.py --workspace-root <原工作区>` 依赖原 v2、冻结目录和本次本地 `.handover-local/before.json` 隔离快照；拒绝覆盖已有 v3 包。它从 ZIP 实际解压后调用原构建工具，并将原 `tests/test_ota_artifacts.py` 的 ROOT 定位到解压工程。完整命令和产物哈希见 result.json；现有包的复核可直接依据 files.sha256 与 ZIP SHA-256，不需重打包。
