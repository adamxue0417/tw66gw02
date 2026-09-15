# battery_adjustment 省电实验

只在 `battery_adjustment` 分支维护。源码路径仍为 `01_固件工程/main`，不代表 GitHub `main` 分支。原工作区不切换、不覆盖，实验主计划为本工作树根目录 `plan.md` 的 OTA-041。

## 改动与边界

- 默认 `SCH_IDLE_SLEEP_ENABLED=1`，无待执行任务时浅休眠；设为 `0` 得到忙等待对照组。
- 保留 48 MHz、1 ms SysTick、现有调度周期的实际语义（包括原有 Period+1 tick 间隔）、ADC 连续转换及 DMA、显示、BLE 和电量策略。
- 任务表是中断共享状态；添加、删除、领取任务及最终空闲检查采用短临界区，任务回调在恢复原中断屏蔽状态后执行。单次任务在回调前移除，防止执行期间再次排队。
- `RunMe` 仍为 uint8_t；积压到 255 时饱和并报告新增内部诊断 `ERROR_SCH_PENDING_OVERFLOW`，不回绕为零。超出容量无法保存更多次数，必须排查超长任务；不应将此状态当成正常运行通过。
- 错误保持时间按 `HAL_GetTick()` 计算 60000 ms，支持计数回绕；新错误重新计时。
- `SCH_Go_To_Sleep()` 保持已有公开声明，定义由 static 修正为一致的外部链接；无 App 协议、持久化布局或 Bootloader 修改。
- 实现使用 PRIMASK 包住最终检查和 WFI。依据 [ST PM0215](https://www.st.com/resource/en/programming_manual/pm0215-stm32f0xxx-cortexm0-programming-manual-stmicroelectronics.pdf) §2.5.2 和 WFI 指令说明，已使能中断挂起可唤醒 WFI，即使 PRIMASK=1；恢复 PRIMASK 后服务中断。保留 SysTick，不使用 STOP/Standby。

## 可重复的离线验证

在本工作树根目录运行，以下输出目录必须使用新名字；构建脚本不会烧录设备。

```powershell
python 01_固件工程/main/tests/run_battery_builds.py --output .battery-local/comparison-new
python 01_固件工程/main/tests/run_scheduler_tests.py --output .battery-local/scheduler-new
python 01_固件工程/main/tests/verify_scheduler_codegen.py --output .battery-local/codegen-new
```

依赖：Python、Keil `C:/Keil_v5` 中的 ARM Compiler 5.06u7，以及调度器执行测试所需的 `unicorn==2.1.4`。可用 `--keil-root` 指定 Keil；Unicorn 已在其他本地目录安装时，使用 `--deps <目录>`，不把依赖包提交到 Git。

`run_battery_builds.py` 在新目录复制源码并校验指纹，两组分别调用：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File BuildKeilFactoryTarget.ps1 -IdleSleepEnabled 0
powershell -NoProfile -ExecutionPolicy Bypass -File BuildOtaArtifacts.ps1 -IdleSleepEnabled 0 -FactoryVersion 100 -OtaVersion 101
```

开启组把两条命令的 `-IdleSleepEnabled` 改为 `1`。这两个参数默认为 `1`；Keil 工程直接构建使用配置头文件默认值。对照组也包含相同的调度器竞争修复和错误计时修复，仅 Sleep 开关不同，不能用冻结 v101 代替 OFF 组。

执行 C 测试时仅模拟 HAL tick、中断屏蔽和 WFI 边界；生产 `TaskScheduler.c` 独立编译后在 Cortex-M0 指令模拟器执行。覆盖积压、回调期间新增任务、单次任务、入睡前/临界窗口中断、UART/DMA 类型唤醒、禁中断/异常上下文禁止睡眠、错误计时回绕、容量边界。`unicorn_runner.py` 复用本仓库历史 snapshot-14 的测试执行器，未引入其固件实现。

另用真实 HAL/CMSIS 编译检查公开头文件独立重复包含、非法开关拒绝，并检查 OFF 无 WFI、ON 有 WFI 及屏障/屏蔽指令。以上不能替代真实 STM32 中断、电流或时序验证。

## 实机 TEST-043（当前 NOT_RUN）

1. 登记设备编号、板卡、MCU/BLE 固件、手机/App、探针、电源、仪表精度及采样方式。对每组固件登记源提交、Sleep 开关、文件 SHA-256；从对应 `sleep-0` 或 `sleep-1` 输出取匹配的完整工厂镜像。不要交叉使用不同组的应用/工厂件，也不要覆盖冻结归档。
2. 同一设备、供电电压、温度输入和屏幕内容，对 OFF/ON 分别做短按、长按、模式/单位切换、探针插拔及温度阶跃、显示/低电图标、BLE 配对/遥测/断连重连、参数保存重启、手动及原超时自动关机回归。记录响应时间、更新间隔、UART 错误/丢包和调度器溢出；发生差异登记 ISSUE，不能仅凭“能用”通过。
3. 两组分别执行配套 v100→v101 OTA，确认成功、重启及原版本确认行为；核对原低电准入仍为 30%。未经实测，不宣称深度睡眠或低电策略已验证。
4. 电流测量断开调试器和串口等反向供电路径，保持同一测量回路。在“BLE 关闭”和“BLE 已连接持续遥测”两种状态下，对 OFF/ON 各测三轮、每轮五分钟；等待初始化完成、条件稳定后采集。交替测 OFF/ON，避免电压和温度漂移造成单向偏差。
5. 保留电流时间序列，计算每轮平均值、电荷量、三轮均值/波动和 `(OFF-ON)/OFF`。无仪表记录不填写节电百分比。差值需超过仪表不确定度和重复测量波动，且上述行为回归通过，OTA-041 才能 DONE。

ADC 连续转换和循环 DMA 可能限制省电收益；无显著收益也如实记录，不在本实验中顺便改变采样或显示。

## Git 与恢复

- 基线提交 `6bd626d` 保存当前编码整理、主计划及历史文字证据；其后提交单独记录实现和本轮验证。
- 输出缓存放 `.battery-local/`，本机已加入 Git 本地排除。其他克隆需自行把同一路径加入 `.git/info/exclude`；逐文件暂存，不提交固件、AXF、ZIP 或依赖。
- 每次推送前核对 `git branch --show-current`、暂存差异及 `git status`。首次仅 `git push -u github battery_adjustment`，以后仅推送当前同名分支；不强推、不合入 main。
- 开关设为 0 可关闭 Sleep；完整撤回实现用 `git revert <实现提交>` 产生新提交，保留基线和证据。是否合入其他分支另行决定。
