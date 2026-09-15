# OTA-010 第二步：main 隔离构建基线

2026-09-11，执行时间 15:28:18～15:28:48（+08:00）。关联 [固定计划](../../../../plan.md) 的 OTA-010、TEST-003、CHG-003。本文件是验证报告，不是替代主计划。

## 结果

**PASS。** 在新建隔离目录从第一步校验过的 175 个 main 受跟踪文件构建，未借用原工程已有编译产物。工厂目标、v100/v101 应用均为 0 错误、0 警告；现有 OTA 检查通过，包括 81 个正向换页中断点和 81 个回滚中断点的主机模拟。

原 main/snapshot-13 共 675 个受跟踪文件以及冻结 main v101 交付目录内的 19 个文件，执行前后 SHA-256 全部一致。隔离目录内的 175 个原始受跟踪文件也未被构建修改；新增编译产物仅留在隔离目录。两边 HEAD 未改变，未进行固件迁移、签名、烧录或实机验证。

## 输入、环境与执行

| 项目 | 记录 |
| --- | --- |
| 源仓库提交 | `b964654c6290591b0f4497e63704fa94161df99f`，分支 `chore/workspace-organization` |
| 源码恢复包 SHA-256 | `35084b18195eeb6ceeda66cd0d52505a5112c8c13d334f19a52b9f2f140e559a` |
| 源码清单 | [第一步 baseline.json](../20260911-step1/baseline.json) 中 main 的 175 个逐文件记录 |
| 隔离工程 | [本地构建目录](../../../../90_历史原始包/整理前备份/ota010-step2/main)；在已有 Git 忽略范围内 |
| 工具版本 | MDK Plus 5.39；ARM Compiler 5.06 update 7（build 960）；armcc `[4d365d]` |
| 编译器安装 | `C:/Keil_v5`，只使用既有安装，不修改工具配置 |
| 第一阶段原交付件 | 仍使用 `05_发布与交付/MCU固件/main_v100-to-v101_DEV`；本轮重建件不替换它 |

命令均在隔离工程根目录运行，详细绝对路径、时间、退出码及日志哈希见 [result.json](result.json)。

```powershell
C:\Keil_v5\ARM\ARM_Compiler_5.06u7\Bin\armcc.exe --vsn
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\BuildKeilFactoryTarget.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\BuildOtaArtifacts.ps1 -FactoryVersion 100 -OtaVersion 101
python .\tests\test_ota_artifacts.py
```

四条命令退出码均为 0。执行过程由 [run_main_baseline.py](run_main_baseline.py) 保存；该脚本拒绝覆盖已存在的运行目录和结果，后续复测应建立新的运行编号。

| 构建项 | Code | RO-data | RW-data | ZI-data | Keil 结果 |
| --- | ---: | ---: | ---: | ---: | --- |
| 工厂目标 v100 | 22020 | 2792 | 240 | 5096 | 0 Error(s), 0 Warning(s) |
| OTA 应用 v100 | 22020 | 624 | 240 | 5096 | 0 Error(s), 0 Warning(s) |
| OTA 应用 v101 | 22020 | 624 | 240 | 5096 | 0 Error(s), 0 Warning(s) |

Bootloader BIN 为 2168 字节，使用现有 6 KiB Bootloader 分区；v101 应用为 22884 字节。本报告记录的是当前开发版静态构建结果，不表示未来安全版的 Flash/RAM/动态栈需求已验证。

## 产物与检查

| 项目 | 本轮结果 |
| --- | --- |
| v101 OTA 大小 | 23268 字节 |
| 签名区域 | 384 字节零值开发占位，未启用 RSA 验签/防降级 |
| CRC-32/ISO-HDLC | `0x119B455F` |
| 完整 OTA SHA-256 | `7d72822aa7ad78dde1265e720eed1b003e81f5e70d59e33c9e1a09ce0bb41b8e` |
| 与冻结包比较 | 完整文件 SHA-256 一致；原归档未覆盖 |
| 产物检查 | 应用前缀、签名占位、manifest 大小/CRC/哈希、向量表、HEX 地址和工厂 Bootloader/应用布局通过 |
| Keil 工厂 BIN | 可选检查已实际执行；与 Bootloader 和 v100 应用一致，允许空闲 Boot 区 0x00/0xFF 填充差别 |
| 断电恢复模拟 | 162 个正向换页/回滚中断用例通过 |
| 启动诊断可选检查 | NOT_RUN；本轮未生成 direct/minboot/GPIO 诊断镜像，现有测试中的对应条件分支未执行 |
| 真实硬件 | NOT_RUN；模拟用例不替代真实掉电、BLE 或升级验收 |

隔离目录内 `OTA_Artifacts` 的全部产物大小和 SHA-256 记录在 result.json 的 `outputs`；保留隔离工作目录供后续对比。不要把其中自动生成的 `Release_v100_to_v101` 复制覆盖冻结交付目录。

## 日志与后续入口

- [编译器版本](compiler-version.log)
- [工厂构建控制台](factory-build.log)、[Keil 工厂原始日志](build_factory_keil.log)
- [OTA 构建控制台](ota-build.log)、[v100 原始日志](build_v100.log)、[v101 原始日志](build_v101.log)
- [OTA 自动检查结果](ota-tests.log)、[完整运行结果与文件哈希](result.json)

无需修改或恢复原工程。已有 [第一步恢复点](../20260911-step1/README.md) 保持不变；新的构建目录作为本地验证材料保留，不作为新发布候选。

**下一步为第三步：验证 snapshot-13 安全构建基线。** 先核对源码包、DEV 签名材料及构建依赖，在独立目录验证；实际存在的依赖和通过结果另行记录，不能沿用本次 main 的通过结论。OTA-010 整体继续 IN_PROGRESS。
