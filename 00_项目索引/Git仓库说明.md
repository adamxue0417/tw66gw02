# Git 仓库与 GitHub 分支

[返回项目入口](../README.md)

GitHub 项目：`https://github.com/adamxue0417/tw66gw02.git`。各工作副本使用独立分支，目录层次不同，不应直接互相覆盖。

| 本地位置 | GitHub 分支 | 范围 |
| --- | --- | --- |
| 工作区根目录 | chore/workspace-organization | 本地与远端同名；整理后的主工程、公共资料、索引、交付元数据和整理脚本 |
| snapshot-12_secure-ota/tw66gw02 | test/v103-integration-memory | v103 开发线；本次无新增改动 |
| snapshot-13_poweroff-v104/tw66gw02 | test/v104-poweroff-pc-ble | v104 开发线；修正迁移后的默认密钥目录并保存已有联调说明 |
| snapshot-14_refactor-from-10 | refactor-from-10 | 14 的完整源码、构建脚本与重构基线；首次建立提交历史 |

主仓库 origin 继续指向原 Gitee；新增 github 远端用于 GitHub 整理分支。本次不改变 GitHub main，不推送其他历史分支，不强制覆盖远端历史。

## 分支用途与后续命名

| 分支 | 定位与维护方式 |
| --- | --- |
| GitHub main | 原有应用工程基线，保持现状；与整理工作区的目录布局不同，不直接合并整个目录迁移 |
| chore/workspace-organization | 整理工作区基线；新主线功能从这里分出，源码入口为 01_固件工程/main |
| test/v102-display-power | 已有显示节电测试线；不是拟开发的 v102 安全 OTA 测试线 |
| test/v103-integration-memory | 12 工程现有集成测试线 |
| test/v104-poweroff-pc-ble | 13 工程关机与电脑 BLE 测试线 |
| refactor-from-10 | 14 独立重构线；目录根包含 tw66gw02 与外层测试脚本 |
| 本地 docs、legacy-folder-snapshot、v1～v11 | 原 Gitee 历史引用，保留供查询，不随本次 GitHub 推送批量发布 |

后续主线 v102 安全升级建议使用 `feature/v102-secure-ota`，从整理工作区基线创建；该分支尚未创建，也未开始固件修改。其他新分支使用 `feature/<主题>`、`fix/<问题>` 或 `test/<版本>-<目的>`。发布版本以测试通过的明确提交为依据，不把快照目录号当成固件发布号。

主仓库本地分支与 GitHub 整理分支同名；默认推送远端为 github，push.default 为 simple。12/13/14 默认推送远端为 origin，同样采用 simple。首次推送时建立同名 upstream；这样日常 `git push` 会推送当前分支，不会批量推送所有分支，也不会把整理分支误推到 Gitee master。

## 日常提交与推送

在对应仓库内先执行 `git status` 和 `git diff`，只暂存确认过的改动，提交后执行 `git push`。新增分支首次推送使用 `git push -u github <分支名>`；12/13/14 的远端名称改用 origin。不要使用 `--all`、`--mirror` 或强制推送来同步这组不同布局的开发线。

需要跨开发线复用改动时，先检查共同历史及文件路径，再移植具体改动；14 的首次提交是独立历史，不能默认通过普通分支合并接入主线。设备验证通过后再选择合入目标，本次不修改 GitHub 默认分支、不删除旧分支。

根仓库忽略 history、development 中的独立工程、实验工程和 J-Link 工具；克隆整理分支不会自动带回这些本地目录。12/13/14 需分别按上表克隆到对应位置；1–11 历史快照及工具可从本地归档或原 Gitee 历史分支恢复。它们不是 Git submodule。

交付二进制、手机安装包、本地恢复 ZIP、密钥和依赖环境不随本次源码提交上传；交付 manifest、源码指纹、SHA-256、说明和验证记录纳入主仓库。恢复实际交付件仍需本地交付目录或另行分发的制品，GitHub 中部分指向本地归档的链接因此只在完整工作区可用。

前四步报告中的“尚未提交”等表述是当时执行记录。当前提交与推送结果以各分支 Git 历史为准。


## battery_adjustment 独立实验（2026-09-15 / CHG-009）

- 根仓库独立工作树 `.worktrees/battery_adjustment`，分支名为 `battery_adjustment`；来源 b964654 + 当前未提交维护内容，实验前基线为 6bd626d。
- 在该工作树的 `01_固件工程/main` 开发浅休眠；目录名 main 不代表 GitHub main。原工作区仍为 chore/workspace-organization，原索引、文件、未提交状态保留。
- 基线、实现、验证分开提交；仅使用 `git push -u github battery_adjustment` 建立同名 upstream，不合入 main、不强推、不批量推送。
- `.worktrees/` 和 `.battery-local/` 为本机 Git 本地排除目录。源码、必要维护资料和文字证据入库，固件/ZIP/依赖不入库。历史安全线保持独立。
- 对应主计划 OTA-041 和 [实验操作说明](../01_固件工程/main/tests/BATTERY_TESTING.md)；实机未通过前不作为省电量产交付。
