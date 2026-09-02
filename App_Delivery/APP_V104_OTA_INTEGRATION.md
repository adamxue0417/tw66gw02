# App端v104 DEV OTA联调说明

交付文件为`mathis_ota_v104_dev_signed.ota`和同名manifest。App不签名、不解析、不截掉末尾384字节，也不持有公钥或私钥；只需先校验manifest，再把OTA原始字节按现有Mathis BLE协议发送。

1. `OTA_BEGIN(0x10)`：payload为`artifact_size(uint32 LE)`、`crc32(uint32 LE)`、`target_version(uint8=104)`。
2. 等待READY(`0x01`)给出的逻辑块上限，当前最大232字节。
3. `OTA_CHUNK(0x11)`：payload为`offset(uint32 LE)+原始OTA数据`；每块等ACK(`0x03`)及下一偏移。超时只重发上一完整逻辑块一次。
4. `OTA_COMMIT(0x12)`：payload为同一CRC-32，只发送一次。最长等60秒，状态顺序应为VERIFY_OK(`0x04`)→APPLYING(`0x06`)。
5. 等待断开、安装重启、v104遥测；约10秒确认后还会再次重启。最多120秒完成重连，最终连续确认版本104。

失败原因：签名`0x02`、防降级`0x05`。全OTA大小、CRC和SHA-256必须使用manifest中的实际值，不可用应用BIN大小代替。该包是DEV签名测试件，不可出货。
