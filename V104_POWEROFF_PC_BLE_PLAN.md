# v104 PB3关机修复与电脑BLE测试计划

- 基线：提交`913c2b5`，目录13独立分支`test/v104-poweroff-pc-ble`，目录12只读。
- SRAM `0x20001FE0..0x20001FEF`保存带魔数、状态、反码和校验的RUNNING/RESTART_ALLOWED/SHUTDOWN_REQUESTED记录；`0x20001FF0`启动追踪保持不变。
- Bootloader v2仅在KEY0按下、可信软件重启、有效OTA恢复或可信非POR调试复位时保持PB3；关机记录及未知/POR复位且未按键时保持PB3低。
- 正常关机顺序：C8721 Sleep并停刷、清BLE队列并关闭蓝牙、停止USART1/2和ADC DMA、等待KEY0释放、写关机记录、关闭中断并永久拉低PB3。
- SWD基线由Bootloader dev v2和提交`913c2b5`的原始v103应用组成；之后用电脑BLE执行v103→v104。
- 设备测试始终仅由电池供电，没有USB条件。关机后记录电池电流和表面温度，10分钟不得出现可测温升。
