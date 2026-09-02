# v103安全OTA烧录、联调与验收指南

## 版本与安全边界

- 分支：`test/v103-integration-memory`
- 安全工厂基线：102
- OTA目标/安全版本：103
- 签名：RSA-3072、PKCS#1 v1.5、SHA-256；签名尾长384字节
- DEV公钥DER SHA-256：`3FCCDCB727514F80A264FEC76933C305262D3CE92F91F8A9711E4EEC1A84C0D4`
- 此控制器必须贴“DEV密钥、不可出货”标识；PROD槽未启用。

旧`mathis_factory_v100.bin`仅用于旧v102流程，SHA-256保持为`55DF837097652CD8C04CC0E8902BEC5D2A74F1683AACDFA0F23011F3E20AD05F`。v103安全联调不得再烧录它，必须烧录新的安全v102工厂镜像。

## 第一次烧录顺序

1. 连接SWD、正常板级供电并保持BOOT0为低。
2. 执行以下命令；脚本会全片擦除、把连续工厂BIN烧到`0x08000000`、校验Boot/应用向量和Boot API，再启用Bootloader页0～7的WRP。参数是防误操作确认，不能省略。

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\ProgramSecureFactoryWithJLink.ps1 -EnableBootWriteProtection
   ```

3. 断电重上电，使Option Bytes重新装载。
4. 运行只读校验：

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\VerifySecureProvisioningWithJLink.ps1
   ```

5. 必须同时通过：RDP Level 0、WRP页0～7、Boot API魔数、公钥指纹、启动遥测版本102。

如果不用脚本：全片擦除后仅将`OTA_Artifacts/mathis_secure_factory_v102_dev.bin`烧到`0x08000000`；不要再单独烧应用，也不要把OTA文件当作SWD BIN。随后通过编程器设置WRP0的bit0/bit1为保护态并保持RDP Level 0。

## App升级顺序

App端使用`App_Delivery/mathis_v103_dev_ota_package.zip`，流程如下：

1. 校验ZIP内SHA-256并读取manifest。最终OTA总长23624字节、CRC-32/ISO-HDLC为`0xF05462D8`、目标版本103。
2. BEGIN payload：`23624`小端32位、`0xF05462D8`小端32位、`103`单字节。
3. 收到READY和块上限232后，从offset 0分块；每块等待ACK给出的下一偏移。
4. 最后ACK应为23624；COMMIT发送同一个CRC。
5. 最长等待60秒完成RSA验签，不重发COMMIT。
6. 收到`VERIFY_OK → APPLYING`后等待断开、重启、重连。
7. 确认遥测版本103；稳定运行10秒后设备确认并再次重启，Bootloader提交安全版本103。

完整帧格式和状态码见ZIP内`APP_V103_OTA_INTEGRATION.md`。

## 安全负向测试

每项先从正确OTA复制测试文件，只改指定内容；失败后重新发送正确BEGIN。

| 用例 | 操作 | 预期 |
| --- | --- | --- |
| 应用被改 | 签名区前任意一字节翻转，按新文件重算传输CRC | `VERIFY_FAILED + 0x02` |
| 签名被改 | 末384字节任意一位翻转，重算传输CRC | `VERIFY_FAILED + 0x02` |
| 全零签名 | 末384字节清零，重算传输CRC | `VERIFY_FAILED + 0x02` |
| 错误密钥 | 用另一个RSA-3072私钥签同一应用 | `VERIFY_FAILED + 0x02` |
| 二次验签 | 应用验签通过后、重启前用调试器破坏暂存签名 | Bootloader拒绝安装并保留v102 |
| 防降级 | v103确认并提交后发送安全版本102 | `VERIFY_FAILED + 0x05` |

主机离线验签已验证：正确签名通过；改应用、改签名、全零签名、错误密钥全部拒绝。设备端返回码仍须上板记录。

## 掉电恢复

分别在下载、COMMIT验签、正向交换、v103试运行未确认、确认重启/安全版本提交阶段断电。重上电后只能出现已确认的旧版本或可继续确认的新版本，不允许无应用启动。重新连接后先读遥测版本，不沿用App内存中的旧offset。

## 功能与内存回归

- 连续BLE命令、最大244字节BLE帧、温度15字节响应并发；确认无丢帧、覆盖和DMA死锁。
- D/O/Probe/Cavity、摄氏/华氏、温度条、蓝牙、低电图标、`---`、高低温异常显示全部覆盖。
- 开机、OTA重启、稳定10秒后的屏幕亮度一致；无闪烁、重影、缺段。
- 复测v102节电参数（8.5 mA、PWM `0xD0`）和功耗，确认内存优化没有改变显示控制逻辑。
- 用栈水印测正常、最大BLE帧、RSA验签及中断叠加；动态剩余必须至少256字节。链接器静态最坏RSA栈为1688字节，2304字节栈的静态余量616字节。

## 当前自动验收结果

- v102安全应用、v103应用：Keil 0错误、0警告。
- v103应用BIN 23240字节，小于25216字节上限。
- RAM：v102旧基线5336字节，v103为5112字节，减少224字节。
- Bootloader有效代码区6984字节，Boot API固定60字节，均未越界。
- 产物布局、CRC、manifest和150个正向/回滚断电点模型通过。
- 主机RSA正向及四类负向测试通过。

尚未在本环境执行：SWD实际烧录、WRP实读、BLE手机联调、真实掉电、动态栈水印、显示/功耗回归。这些不能用模拟结果替代，验收时在下表记录日期、板号、操作者和日志路径。

| 项目 | 结果 | 记录 |
| --- | --- | --- |
| 安全工厂烧录/WRP | 待上板 | |
| App完整升级 | 待联调 | |
| 六项安全负测 | 待联调 | |
| 五阶段掉电 | 待上板 | |
| 动态栈水印 | 待上板 | |
| 显示/功耗回归 | 待上板 | |
