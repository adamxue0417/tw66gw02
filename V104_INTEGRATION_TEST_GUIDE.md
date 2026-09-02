# v104整体联调和电池关机测试

## 1. 首次SWD烧录（必须先做）

1. 设备断电，连接SWD；全片擦除以解除旧Bootloader页WRP。
2. 将`OTA_Artifacts/mathis_secure_factory_v103_bootv2_dev.bin`从`0x08000000`烧录并校验。
3. 执行`ProgramSecureFactoryWithJLink.ps1 -Image '.\OTA_Artifacts\mathis_secure_factory_v103_bootv2_dev.bin' -EnableBootWriteProtection`，重新启用页0～7 WRP并保持RDP Level 0；再运行`VerifySecureProvisioningWithJLink.ps1`读取验证DEV公钥指纹。
4. 断开调试供电。整机始终只接电池；未按KEY0的冷启动不得拉高PB3。按住KEY0启动，看到v103遥测后释放。

旧v103设备的Bootloader页已受保护，仅发送v104 OTA不能修复早期PB3逻辑，必须执行本节。

## 2. 电脑BLE顺序

```powershell
cd Tools\PC_BLE_Test
.\Setup-PC-BLE.ps1
.\Run-PC-BLE.ps1 selftest
.\Run-PC-BLE.ps1 discover
.\Run-PC-BLE.ps1 monitor --seconds 10
.\Run-PC-BLE.ps1 poweroff-check --confirm-poweroff
# 按KEY0重新开机
.\Run-PC-BLE.ps1 ota --confirm DEV-v104
.\Run-PC-BLE.ps1 monitor --seconds 10
```

OTA状态：READY=`0x01`、ACK=`0x03`、VERIFY_OK=`0x04`、VERIFY_FAILED=`0x05`、APPLYING=`0x06`、ABORTED=`0x07`、POWER=`0x08`、STORAGE=`0x09`、BUSY=`0x0A`、BAD_OFFSET=`0x0B`。失败原因SIGNATURE=`0x02`，防降级=`0x05`。

## 3. PB3与电池验收

- KEY0冷启动：释放前PB3已稳定为高；未按键冷启动PB3保持低。
- OTA提交、交换恢复和10秒确认的软件复位期间PB3持续保持，不掉电。
- 本地/远程关机：屏幕Sleep→BLE断开/蓝牙关闭→DMA停止→KEY0释放→PB3低；示波器确认PB3不反跳，30秒无广播。
- 在同一块电池上记录关机前稳定电流、关机后稳定电流、环境温度和电池表面温度。关机电流应显著下降，连续10分钟不得有可测温升。
- 若仍发热、异味、鼓包或异常大电流，立即断开电池，停止软件测试，排查电池/BMS、充放电MOS、反接或板级短路。

## 4. 尚需上板的项目

当前自动验收覆盖Keil 0错误/0警告、Flash/RAM边界、静态栈、CRC/拆包粘包、manifest/签名和文件哈希。SWD实烧、WRP实读、BLE射频联调、PB3示波器、真实断电、动态栈水印、显示及电流/温升必须在目标控制器上记录，不能由静态测试替代。
