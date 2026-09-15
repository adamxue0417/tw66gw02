# TEST-044：省电工作树迁至可见目录

2026-09-15 / OTA-041 / CHG-010 / DEC-011。结果 **PASS**。

- 原路径：`.worktrees/battery_adjustment`。
- 新路径：`01_固件工程/experiments/battery_adjustment`。
- 使用 `git worktree move` 完成迁移，未重新建仓库或更换分支。
- 迁移前后 HEAD 均为 `bd7a7eb9816d9660b479d31fd056f729a290985d`，分支 `battery_adjustment`，upstream 为 `github/battery_adjustment`；本次文档及入口更新将单独提交。
- 文档更新之前，完整 2381 个文件（包含源码、历史证据和本地构建产物）逐文件 SHA-256 完全一致。原隐藏路径已不存在，Git 工作树登记指向新路径。
- 原根工作区 HEAD、索引和未提交状态未改变；原 256 个登记文件及 19 个冻结归档文件重新核验不变。
- 本轮只迁移目录、更新文档和新增工作区入口。未重新编译，未运行实机测试；TEST-042 的历史离线 PASS 保留，TEST-043 仍为 NOT_RUN，OTA-041 仍因实机条件 BLOCKED。

[机器校验记录](result.json) · [实验主计划](../../../plan.md) · [打开省电工作区](../../../battery_adjustment.code-workspace)

## 文件位置

这是根仓库的完整独立工作树，保留仓库目录结构。省电固件位于新工作树内 `01_固件工程/main`，测试报告位于 `04_测试与联调/OTA-041`。原根目录的 main 工程继续保留。

旧构建日志中的 `.worktrees/battery_adjustment` 是执行时的历史路径，不批量替换。后续使用新路径下的测试脚本产生新输出；旧缓存不作为重新构建的证据。

## 下一步

在 IDE 打开新目录的 `battery_adjustment.code-workspace`，可分别查看计划、固件和省电测试证据。实机测试仍需设备、供电/仪表和手机 App 信息，按 TEST-043 操作。
