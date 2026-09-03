# Mathis v100 → v101 开发版 OTA 烧录与联调

> **仅限研发测试。** 当前实现校验 CRC-32、文件大小和应用向量表，但没有启用
> RSA-3072 验签和防降级，禁止用于量产或客户设备。

## 1. 两类文件不要混用

运行以下命令生成并验证 v100 → v101 产物：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File .\BuildOtaArtifacts.ps1 -FactoryVersion 100 -OtaVersion 101
python .\tests\test_ota_artifacts.py
```

脚本会生成 `OTA_Artifacts/Release_v100_to_v101/`：

- `Factory_Flash/mathis_factory_v100.hex`
  - 你在 Keil 中首次烧录的文件。
  - 同时包含 `0x08000000` 的 bootloader 和 `0x08001800` 的 v100 App。
  - 烧录前必须执行 Full Chip Erase。
- `Factory_Flash/mathis_factory_v100.bin`
  - 与组合 HEX 对应的连续 BIN；使用其他烧录器时加载地址必须是 `0x08000000`。
- `App_Upload/mathis_ota_v101.ota`
  - 唯一交给 App 工程师内置的升级文件。
  - 内容为真实可执行的重定位 v101 App，加 384 字节开发签名占位区。
- `App_Upload/mathis_ota_v101.manifest.json`
  - App 必须读取其中的 `target_version`、`artifact_size` 和 `crc32_iso_hdlc`。
- `Debug_Only/`
  - 裸 App BIN/HEX，只用于定位问题；不能代替 Factory HEX，也不能上传到 App。

当前已验证的 App 文件参数：

```text
Target version : 101
Artifact size  : 23268 bytes
CRC-32         : 0x119B455F
SHA-256        : 7d72822aa7ad78dde1265e720eed1b003e81f5e70d59e33c9e1a09ce0bb41b8e
```

每次重新构建后应以同目录 manifest 和 `SHA256SUMS.txt` 为准，不要复制本文中的旧值。

## 2. Keil 首次烧录

1. 打开 `MDK-ARM/tw66gw02.uvprojx` 并执行 Rebuild；该 target 会先构建并嵌入 bootloader。
2. 在 Flash 菜单执行 Full Chip Erase。
3. 使用 Download 烧录完整 factory target，或直接选择
   `Factory_Flash/mathis_factory_v100.hex`。
4. 复位设备，确认屏幕、按键、测温和蓝牙均正常。
5. 连接 App，首个合法 telemetry 的 Firmware Version 必须是 100。

禁止只烧 `mathis_app_v100.hex`。它从 `0x08001800` 开始，不包含复位地址上的
bootloader，单独整片烧录后设备无法正常启动。

## 3. App 设置

App 当前的 4096 字节 dummy 文件必须替换为 `mathis_ota_v101.ota`，Target Version
固定使用 manifest 中的 101，不能再手工填写 2 或 105。

OTA 与系数写入使用同一 GATT 通道：

```text
Service: 441C1000-776D-B95D-75A7-496DD1B5BEDA
RX     : FFE1  (App -> Device, Write)
TX     : FFE2  (Device -> App, Notify)
```

截图中的 `BLE Write failed: FFE1` 是 Android GATT 写入在到达 STM32 之前失败。
App 应复用已经能成功发送系数的 FFE1 characteristic 和写入方式，并确保所有写操作
串行执行：收到上一笔写回调后才能提交下一笔。

`OTA_BEGIN` 完整帧只有 17 字节。`OTA_CHUNK` 最多包含 232 字节固件数据，完整帧
最多 244 字节；在默认 MTU 23 下由 App 拆成多个连续的 characteristic writes，
设备按 Mathis Length 字段重新组帧。

## 4. 正常升级验收

1. 电量保持在 30% 以上并连接 App。
2. 点击 Start update；设备返回 `READY`，accepted chunk size 为 232。
3. App 每发送一个 chunk，都必须等到 `ACK + next expected offset` 后再发送下一包。
4. 传输期间设备暂停 1 Hz telemetry。
5. COMMIT 后设备依次发送 `VERIFY_OK`、`APPLYING`，随后自动复位。
6. bootloader 将 v101 安装到运行槽，同时把 v100 保存为回滚副本。
7. App 重连后应收到 Firmware Version 101。
8. 设备稳定运行 10 秒后确认 v101；之后再次复位仍应运行 v101。

## 5. 中断与回滚验收

- BEGIN 到 COMMIT 之间断连、Abort 或断电：重新启动后仍运行 v100。
- CRC 错误、文件不完整或 offset 错误：拒绝安装并继续运行 v100。
- COMMIT 后换页期间断电：下次上电按 journal 从安全步骤继续安装 v101。
- v101 trial 启动后 10 秒内再次复位：bootloader 自动恢复 v100。
- 回滚期间断电：下次上电继续回滚，最终运行 v100。
- v101 稳定超过 10 秒后复位：继续运行已经确认的 v101。

自动测试已覆盖 81 个正向换页中断点和 81 个回滚中断点。实机仍需至少在传输
25%、50%、90%，COMMIT 后换页阶段，以及 v101 确认前分别执行断连或断电测试。
