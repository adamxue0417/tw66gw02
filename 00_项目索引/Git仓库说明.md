# Git 仓库与 GitHub 分支

[返回项目入口](../README.md)

GitHub 项目：`https://github.com/adamxue0417/tw66gw02.git`。各工作副本使用独立分支，目录层次不同，不应直接互相覆盖。

| 本地位置 | GitHub 分支 | 范围 |
| --- | --- | --- |
| 工作区根目录（本地 master） | chore/workspace-organization | 整理后的主工程、公共资料、索引、交付元数据和整理脚本 |
| snapshot-12_secure-ota/tw66gw02 | test/v103-integration-memory | v103 开发线；本次无新增改动 |
| snapshot-13_poweroff-v104/tw66gw02 | test/v104-poweroff-pc-ble | v104 开发线；修正迁移后的默认密钥目录并保存已有联调说明 |
| snapshot-14_refactor-from-10 | refactor-from-10 | 14 的完整源码、构建脚本与重构基线；首次建立提交历史 |

主仓库 origin 继续指向原 Gitee；新增 github 远端用于 GitHub 整理分支。本次不改变 GitHub main，不推送其他历史分支，不强制覆盖远端历史。

根仓库忽略 history、development 中的独立工程、实验工程和 J-Link 工具；克隆整理分支不会自动带回这些本地目录。12/13/14 需分别按上表克隆到对应位置；1–11 历史快照及工具可从本地归档或原 Gitee 历史分支恢复。它们不是 Git submodule。

交付二进制、手机安装包、本地恢复 ZIP、密钥和依赖环境不随本次源码提交上传；交付 manifest、源码指纹、SHA-256、说明和验证记录纳入主仓库。恢复实际交付件仍需本地交付目录或另行分发的制品，GitHub 中部分指向本地归档的链接因此只在完整工作区可用。

前四步报告中的“尚未提交”等表述是当时执行记录。当前提交与推送结果以各分支 Git 历史为准。
