# OTA-041：独立浅休眠实验验证

日期：2026-09-15。分支：`battery_adjustment`。关联 CHG-009 / DEC-010。

**TEST-042 离线 PASS；TEST-043 实机 NOT_RUN；OTA-041 BLOCKED，等待设备、仪表及手机/App 组合。** 没有烧录、真实 BLE/UART/ADC 唤醒验证或电流测量，不给出节电比例。

## Git 与输入来源

- 整理工作区原 HEAD：`b964654c6290591b0f4497e63704fa94161df99f`。
- 实验前基线：`6bd626d`，纳入当前未提交的编码整理和必要维护记录，复制后逐文件核验；[256 文件及冻结件指纹](baseline-fingerprint.json)。
- 实现提交：`9cc2b041a05cff54585eed78bf1fe7d85efd129a`。构建发生在提交之前；已逐文件验证实际构建输入 SHA-256 与该提交的 Git blob 一致，[完整记录](result.json) 保留构建启动时 HEAD，避免把旧提交冒称为构建来源。
- 原根工作区仍为 `chore/workspace-organization`，分支、HEAD、索引和未提交状态未变；登记的 256 个文件以及原 v101 归档全部 19 个文件哈希未变。
- 原 GitHub main 为 `7d163bd7a8b8487ead9e99538f126d071b39d62f`；新分支只推送同名 `battery_adjustment`。推送后引用以 GitHub 和 Git upstream 为准。

## 实际验证结果

| 项目 | 本轮实际结果 | 证据 |
| --- | --- | --- |
| 实验前基线 | 工厂/v100/v101 均 0 错误、0 警告；6 项固件/manifest 与冻结件逐字节一致；162 项换页/回滚中断模拟通过 | [基线构建](baseline/build_factory_keil.log)、[基线 OTA 检查](baseline/ota-tests.log)、result.json |
| Sleep OFF | 工厂/v100/v101 均 0 错误、0 警告；布局/向量/CRC/162 项恢复检查通过 | [工厂](sleep-0/build_factory_keil.log)、[v100](sleep-0/build_v100.log)、[v101](sleep-0/build_v101.log)、[OTA](sleep-0/ota-tests.log) |
| Sleep ON | 工厂/v100/v101 均 0 错误、0 警告；布局/向量/CRC/162 项恢复检查通过 | [工厂](sleep-1/build_factory_keil.log)、[v100](sleep-1/build_v100.log)、[v101](sleep-1/build_v101.log)、[OTA](sleep-1/ota-tests.log) |
| 调度器编译后 C 测试 | OFF 70 项、ON 128 项通过；Cortex-M0 指令模拟器，HAL/IRQ/WFI 为测试替身 | [OFF](sleep-0/scheduler-tests.log)、[ON](sleep-1/scheduler-tests.log) |
| 实际指令及配置 | 真实 HAL/CMSIS 编译；头文件独立重复包含通过；非法值 2 按预期编译拒绝；OFF 无 WFI、ON 有 WFI | [结果](codegen/result.json)、[ON 反汇编](codegen/scheduler-1.asm.txt)、[非法值预期拒绝](codegen/invalid-config.log) |
| Bootloader | OFF/ON 与实验前及冻结 Bootloader 字节一致 | result.json 的制品哈希 |
| 实机操作、OTA、功耗 | NOT_RUN；缺少设备/仪表/手机App组合 | [待执行矩阵](../../../01_固件工程/main/tests/BATTERY_TESTING.md) |

在实际反汇编中检查了 `CPSID` → 待执行任务扫描 → `DSB` → `WFI` → 恢复 `PRIMASK` → `ISB` 顺序。寄存器配置仅清除 SLEEPDEEP/SLEEPONEXIT，其他 SCR 位保留。硬件依据见实验说明链接的 ST PM0215；模拟测试不代表 STM32 外设时序通过。

## 本地实验固件

全部为开发签名占位的实验文件，版本号仍为构建参数 v100/v101。由目录、提交、开关和 SHA-256 识别，不能仅凭版本号区分。

| 组别 | v101 应用大小 | v101 裸应用 SHA-256 |
| --- | --- | --- |
| OFF | 22988 字节 | `2c23ca760aec422b11627511a98ad87083a8369eee7afc819a233df4d936b553` |
| ON | 23076 字节 | `4daa5f6a044a31188f171395ce3d5df89a26e7486cb73971dd436d564c1f3569` |

完整工厂、应用、OTA、manifest 的大小/哈希见 result.json。完整产物仅保留在本工作树 `.battery-local/comparison/sleep-0/project/OTA_Artifacts` 和 `sleep-1/project/OTA_Artifacts`，未进入 Git，也未覆盖冻结归档。GitHub 克隆者按实验说明重建；不把本地二进制缺失当成主机测试未执行。

ON 相比冻结 v101 增加 192 字节；应用仍低于现有 27264 字节上限。此处为文件大小，不是实测功耗；RAM/栈实机水印未测。

## 环境问题和下一步

ISSUE-001：首次启动调度器模拟测试时，沙箱拒绝读取已有本地 Unicorn 依赖，测试未进入 C 执行。使用已授权的执行环境重新运行后 OFF/ON 全部通过；未修改依赖和系统安全设置。采集证据时的 Git ownership 检查使用限定本仓库路径的单命令配置解决，未修改全局 safe.directory。不是实机固件故障，已解除。

下一步：登记 DEC-008 设备与仪表条件，按 TEST-043 执行完整操作回归和两种 BLE 状态下三轮五分钟 OFF/ON 电流对比；记录均值、波动、仪表不确定度和原始曲线。操作回归通过且电流下降超过测量波动后，才能将 OTA-041 标记 DONE。发现回归追加 ISSUE/复测记录，保留本报告。

[主计划](../../../plan.md) · [可重复测试与回退说明](../../../01_固件工程/main/tests/BATTERY_TESTING.md)
