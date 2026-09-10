# 固件工程导航

[返回项目入口](../README.md) · [版本总表](../00_项目索引/版本总表.md)

| 工程/快照 | 当前路径 | 仓库边界 |
| --- | --- | --- |
| main | [主线说明](main/README.md) | 根工作区 .git |
| snapshot-01 | [snapshot-01_early-baseline](<history/snapshot-01_early-baseline>) | 历史快照，根仓库忽略 |
| snapshot-02 | [snapshot-02_incomplete](<history/snapshot-02_incomplete>) | 历史快照，根仓库忽略 |
| snapshot-03 | [snapshot-03_temperature-errors](<history/snapshot-03_temperature-errors>) | 历史快照，根仓库忽略 |
| snapshot-04 | [snapshot-04_calibration](<history/snapshot-04_calibration>) | 历史快照，根仓库忽略 |
| snapshot-05 | [snapshot-05_units-display](<history/snapshot-05_units-display>) | 历史快照，根仓库忽略 |
| snapshot-06 | [snapshot-06_early-ota](<history/snapshot-06_early-ota>) | 历史快照，根仓库忽略 |
| snapshot-07 | [snapshot-07_direct-boot](<history/snapshot-07_direct-boot>) | 历史快照，根仓库忽略 |
| snapshot-08 | [snapshot-08_mathis-ble](<history/snapshot-08_mathis-ble>) | 历史快照，根仓库忽略 |
| snapshot-09 | [snapshot-09_secure-ota-experiment](<history/snapshot-09_secure-ota-experiment>) | 历史快照，根仓库忽略 |
| snapshot-10 | [snapshot-10_development-ota](<history/snapshot-10_development-ota>) | 历史快照，根仓库忽略 |
| snapshot-11 | [snapshot-11_powerhold-incomplete](<history/snapshot-11_powerhold-incomplete>) | 历史快照，根仓库忽略 |
| snapshot-12 | [snapshot-12_secure-ota](<development/snapshot-12_secure-ota>) | tw66gw02 内独立 .git；外层资料由备份保全 |
| snapshot-13 | [snapshot-13_poweroff-v104](<development/snapshot-13_poweroff-v104>) | tw66gw02 内独立 .git；外层资料由备份保全 |
| snapshot-14 | [snapshot-14_refactor-from-10](<development/snapshot-14_refactor-from-10>) | 快照外层独立 .git；refactor-from-10 分支 |

2 不具备完整本地工程；11 缺少历史构建配置，均保持原样归档。12/13 应使用安全 OTA 流程，不使用继承的旧 6 KiB 工厂构建入口。14 是基于 10 的重构线，不包含所有 12/13 功能。

路径与逐文件迁移证据见 [第三步报告](../00_项目索引/第三步迁移与验证报告.md)。
