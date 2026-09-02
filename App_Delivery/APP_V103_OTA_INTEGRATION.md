# v103 DEV安全OTA：App联调说明

## App拿到的文件

- `mathis_ota_v103_dev_signed.ota`：按原始字节发送，不解包、不改写，也不能截掉末尾384字节。
- `mathis_ota_v103_dev_signed.manifest.json`：读取总大小、CRC、版本和校验值。
- `SHA256SUMS.txt`：下载或拷贝后先校验。

App不签名、不持有公钥或私钥。此包使用一次性DEV密钥，只能用于标记为DEV、不可出货的控制器。

## BLE帧

帧为：`5A 5A | type(1) | sequence(1) | payload_length_le16(2) | payload | crc16_le(2)`。
CRC-16/CCITT-FALSE参数为初值`0xFFFF`、多项式`0x1021`，覆盖从`type`到payload末尾。

OTA类型：

| Type | 含义 | Payload |
| --- | --- | --- |
| `0x10` | BEGIN | `artifact_size_le32 + crc32_le32 + target_version_u8` |
| `0x11` | CHUNK | `offset_le32 + 原始OTA数据`，数据最多232字节 |
| `0x12` | COMMIT | `crc32_le32` |
| `0x13` | ABORT | 空 |
| `0x90` | 设备状态 | 首字节为状态，后随状态参数 |

## 升级顺序

1. 确认电量至少30%，设备当前没有未确认的试运行固件。
2. 从manifest读取`artifact_size`、`crc32_iso_hdlc`和`target_version=103`，发送BEGIN。
3. 收到`READY(0x01)`；其后两个字节为设备允许的块长，本版应为232。
4. 从offset 0开始发送CHUNK。每块只在收到`ACK(0x03)+next_offset_le32`后继续。
5. 最后一块ACK的下一偏移必须等于manifest中的`artifact_size`。
6. 发送COMMIT，CRC必须与BEGIN相同。RSA验签期间最多等待60秒，不要重发COMMIT。
7. 收到`VERIFY_OK(0x04)`、`APPLYING(0x06)`后等待BLE断开、设备重启并重新连接。
8. 遥测版本应先变为103。新固件稳定运行10秒后会确认并再次重启，Bootloader随后提交安全版本103。

CHUNK可安全重发“上一块完全相同的数据”；设备会返回已有的下一偏移。其他错误偏移返回`BAD_OFFSET`。

## 状态与故障处理

| 状态 | 值 | 处理 |
| --- | ---: | --- |
| READY | `0x01` | 按返回块长开始发送 |
| ACK | `0x03` | 读取下一偏移 |
| VERIFY_OK | `0x04` | 继续等待APPLYING |
| VERIFY_FAILED | `0x05` | 第二字节为原因，停止并提示 |
| APPLYING | `0x06` | 不再发包，等待断开/重启 |
| ABORTED | `0x07` | 可从BEGIN重新开始 |
| POWER | `0x08` | 充电至30%以上 |
| STORAGE | `0x09` | 停止升级，保留日志并重试一次 |
| BUSY | `0x0A` | 等待当前流程完成 |
| BAD_OFFSET | `0x0B` | 使用最后ACK偏移恢复 |

`VERIFY_FAILED`原因：`0x01` CRC错误、`0x02` RSA签名错误、`0x03`大小错误、`0x04`超时、`0x05`防降级拒绝、`0xFF`其他格式错误。

任何下载、安装或确认阶段断电后，App都应重新连接、读取遥测版本，再决定是否重新发BEGIN；不要假设断电前的本地offset仍有效。
