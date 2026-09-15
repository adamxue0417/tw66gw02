# OTA-010 第一步：迁移前工作区基线

记录日期：2026-09-11。关联 [固定主计划](../../../../plan.md) 的 OTA-010、TEST-002、CHG-002。本文件是证据报告，不是另一份实施计划。

## 结果

第一步已完成：确认源/目标仓库、保存精确文件内容与已有文档改动，并完成 ZIP 读回和工作区逐文件哈希复核。尚未执行构建、代码迁移、烧录或实机测试；OTA-010 整体仍为 IN_PROGRESS。

| 项目 | main（迁移目标） | snapshot-13（迁移来源） |
| --- | --- | --- |
| 工程目录 | `01_固件工程/main` | `01_固件工程/development/snapshot-13_poweroff-v104/tw66gw02` |
| 所属 Git 仓库 | 工作区根目录 | 该工程自己的独立仓库 |
| 当前分支 | `chore/workspace-organization` | `test/v104-poweroff-pc-ble` |
| HEAD | `b964654c6290591b0f4497e63704fa94161df99f` | `d60b6a8654e6f3ac743a122699d7418b9cdc7d98` |
| 提交日期 | 2026-09-10 17:15:17 +0800 | 2026-09-10 15:18:00 +0800 |
| 工程范围 tree | `0d11759c38b69c6bb854f02b9e5c8d2878861af8` | `5e17d38f26db4d701d4b64bf285d7d0eb2522442` |
| 工程范围状态 | CLEAN；暂存/未暂存差异均空 | CLEAN；暂存/未暂存差异均空 |
| 已保存受跟踪文件数 | 175 | 500 |
| 被忽略且未跟踪的文件数 | 441，未纳入恢复包 | 1857，未纳入恢复包 |

这里的 main 是工程目录名，不是当前 Git 分支名。根仓库检查时已有 `README.md` 修改，以及未跟踪的 `AGENTS.md`、`plan.md`；它们是前一轮维护入口改动，已保存为文档覆盖层。没有暂存改动。没有创建分支、提交、标签、推送或修改 Git 索引/配置。

## 恢复点与证据

- [baseline.json](baseline.json)：捕获时间、完整提交/tree、逐文件大小与 SHA-256、归档哈希、排除范围。
- [root-status-before.txt](root-status-before.txt)：生成恢复包前的状态，包含刚新增的本轮捕获脚本；前三个文档是本轮开始前已有改动。
- [root-docs.unstaged.patch](root-docs.unstaged.patch)：原 README 相对 HEAD 的差异；新建 AGENTS/plan 的内容在文档 ZIP 中。
- [main 未暂存差异](main.unstaged.patch)、[main 暂存差异](main.staged.patch)、[snapshot-13 未暂存差异](snapshot13.unstaged.patch)、[snapshot-13 暂存差异](snapshot13.staged.patch)：均为 0 字节，明确记录干净基线。
- [捕获脚本](capture_baseline.py)：生成本轮证据的过程；已有恢复点时拒绝覆盖，不要为了复核再次运行捕获。

| 本地恢复包 | SHA-256 |
| --- | --- |
| [main 受跟踪工作树](../../../../90_历史原始包/整理前备份/20260911-ota010-step1/main.tracked-worktree.zip) | `35084b18195eeb6ceeda66cd0d52505a5112c8c13d334f19a52b9f2f140e559a` |
| [snapshot-13 受跟踪工作树](../../../../90_历史原始包/整理前备份/20260911-ota010-step1/snapshot13.tracked-worktree.zip) | `108169060da81d9473eb477a23d6397bf0a38d18c8b9304025481cab9b8ddf7e` |
| [本轮修改前的三个文档](../../../../90_历史原始包/整理前备份/20260911-ota010-step1/documentation-before-step1.zip) | 见 baseline.json 的 `documentation_before_update.sha256` |

ZIP 保留捕获时的实际工作树字节，逐文件 SHA-256 已读回核对；再次比较当前两边源码也全部一致。main ZIP 内路径含 `01_固件工程/main/` 前缀，snapshot-13 ZIP 内路径相对其独立工程根目录。

恢复包位于已有 Git 忽略的本地备份目录，不随普通源码提交上传。它们不是完整 Git 历史、磁盘或构建环境备份；不含 `.git`、未跟踪/被忽略产物、编译器安装和独立存放的私钥。历史交付件继续使用既有归档，本轮没有改写它们。后续如要修改未覆盖的文件，先补充相应恢复点。

### 如何恢复

1. 先保存恢复时的新改动，按 baseline.json 核对所选 ZIP 的整体 SHA-256。
2. 解压到工作区内一个新建的独立目录，不能直接覆盖当前工程。逐文件按清单检查字节数和 SHA-256。
3. 从新目录对比并按需恢复受影响文件：main 按 ZIP 中的工程前缀定位；snapshot-13 按独立工程根目录定位。后续新增加的文件不在旧 ZIP 中，必须依据后续变更记录单独处理，不能批量清空工作区。
4. 若恢复到本轮开始前的文档状态，从文档 ZIP 取回对应文件；它保留 README 的原有入口、未提交 AGENTS 和 plan。正常继续推进不回退此文档 ZIP。
5. 恢复后重新检查 Git 差异并运行受影响的构建/测试。ZIP 校验通过不等于恢复后的固件构建或设备行为通过。

当前不需要执行恢复，也不应执行 `reset --hard`、`clean` 或跨仓库整目录覆盖。

## 构建入口静态核对

| 范围 | 已发现入口 | 本轮结论 |
| --- | --- | --- |
| main 工厂构建 | `BuildKeilFactoryTarget.ps1` → `PrepareFactoryBootImage.ps1`；Keil target `tw66gw02` 使用 `tw66gw02_factory.sct` | 入口存在；未运行 |
| main OTA | `BuildOtaArtifacts.ps1`，默认 FactoryVersion=100、OtaVersion=101 | 写入工程 `OTA_Artifacts` 及生成工程；第二步执行前隔离构建输出，不覆盖冻结交付目录 |
| snapshot-13 | `BuildV104Delivery.ps1` → Bootloader 构建及 `BuildOtaArtifacts.ps1` | 入口存在；依赖归档 v103 应用与外部 DEV 签名材料；未运行，不表示依赖已齐全 |
| Keil 工具 | `C:/Keil_v5/UV4/UV4.exe`、`ARM/ARM_Compiler_5.06u7/Bin/armcc.exe` 和 `fromelf.exe` | 文件存在；未验证实际编译器输出、许可或构建结果 |

下一步执行主计划中 OTA-010 准备工作的第二步：在隔离的构建工作目录验证 main 当前构建和既有 OTA 检查，保存日志；确认无新增源码差异且冻结交付件哈希不变。构建基线尚未完成，不提前标为通过。
