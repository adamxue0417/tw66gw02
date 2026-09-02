# Windows电脑BLE联调工具（v103→v104）

默认设备名`337910`、地址`04:78:63:33:79:10`。工具连接后枚举完整GATT表，再选择FFE1写特征和FFE2 Notify特征；找不到时不会猜UUID。

```powershell
.\Setup-PC-BLE.ps1
.\Run-PC-BLE.ps1 selftest
.\Run-PC-BLE.ps1 discover
.\Run-PC-BLE.ps1 monitor --seconds 10
```

环境固定为Python 3.11和`bleak==3.0.2`，隔离在`.venv`。关机和OTA必须显式确认：

```powershell
.\Run-PC-BLE.ps1 poweroff-check --confirm-poweroff
.\Run-PC-BLE.ps1 ota --confirm DEV-v104
```

设备始终由电池供电。关机测试要求BLE在3秒内断开、随后30秒不再广播；之后人工记录关机前后电池电流，并确认连续10分钟无可测温升。

OTA前自动校验manifest中的大小、CRC-32、SHA-256、384字节签名尾和DEV标记。逻辑块遵从READY（最大232字节），完整Mathis帧再切为默认20字节GATT片段，片间至少25ms。每块等待ACK；超时仅重发上一个完整块一次。COMMIT只发送一次，最长等待60秒验签，并在120秒内跨越安装和10秒确认两次重启，最终稳定收到v104遥测。

若FFE1/FFE2不是标准UUID，先运行`discover`，复制完整UUID后执行：

```powershell
.\Run-PC-BLE.ps1 --write-uuid <UUID> --notify-uuid <UUID> monitor --seconds 10
```

联网命令同时输出人类可读日志和`logs\*.jsonl`，记录地址、UUID、原始状态帧、ACK偏移、重连和版本，不保存密钥。
