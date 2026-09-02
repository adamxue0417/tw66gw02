# v104构建、内存与验证报告

## 构建结果

- Keil ARMCC 5.06u7：应用和Bootloader均为0错误、0警告。
- v104应用：RO 24,184字节，RW+ZI 5,120字节，ROM 24,476字节；应用上限25,216字节，剩余740字节。
- v103的RW+ZI为5,112字节；v104增加8字节。保留电源记录位于链接区之外，不占普通RW/ZI。
- Bootloader主区使用`0x1D0C/0x1FC0`，Boot API使用`0x3C/0x40`；RSA最大静态栈1,688字节。应用栈仍为2,304字节，按v103组合调用分析至少留616字节；动态水印需上板确认。

## 最终文件

| 文件 | 字节 | SHA-256 |
|---|---:|---|
| `mathis_secure_bootloader_dev_v2.bin` | 8,188 | `DB88E94FAB70BCEA08AD03EC66D628B15DCAC8482E2D85FB0624F9F18C86D8D9` |
| `mathis_secure_factory_v103_bootv2_dev.bin` | 31,432 | `9DDA30BE007F7D037E465F95A08E0F438A043EC444ED297A6AB0A03FB9B4B882` |
| 原始`mathis_app_v103.bin` | 23,240 | `C3AD66C2302ACA9CFCC19BFD8E77197EB7204CA1FE13DB7CFA4FE37F7869ACAE` |
| `mathis_app_v104.bin` | 24,476 | `E4FE846BD6E31FF963CD236C09EF3458C07A99D2BCFB5257EE11AF6BFEBD4107` |
| `mathis_ota_v104_dev_signed.ota` | 24,860 | `4EA2F3917C8461C6D37ABAE1C45E33476706BCB9670B8AA18AF7B9D5DA0E5468` |

OTA CRC-32为`0xC8BCBCC5`，目标/安全版本104，签名格式RSA-3072/PKCS#1 v1.5/SHA-256，DEV公钥指纹`3FCCDCB727514F80A264FEC76933C305262D3CE92F91F8A9711E4EEC1A84C0D4`。

## 自动验证

- Python自测和4项unittest通过：CRC标准向量、帧编解码、拆包/粘包/坏CRC重同步、manifest变异拒绝。
- OpenSSL验证正确签名通过；修改应用、修改签名、全零签名和错误密钥签名均拒绝。
- Windows环境成功安装Python 3.11隔离环境和`bleak==3.0.2`。只读discover已实际运行，但目标`337910/04:78:63:33:79:10`当时未广播，因此没有执行GATT枚举。
- 没有执行真实关机或OTA，避免旧Bootloader尚未经SWD替换时测试错误路径。

SWD烧录、WRP读回、目标板RSA、真实BLE传输、PB3波形、动态栈水印、显示回归以及仅电池供电下的电流/10分钟温升仍需上板完成。
