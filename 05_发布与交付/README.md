# 发布与交付入口

[返回项目入口](../README.md) · [版本总表](../00_项目索引/版本总表.md)

第四步已完成归集。MCU 固件均为 DEV 开发件；手机 APP、BLE 固件与板卡的适配关系仍待联调确认。工程内原始输出保留，后续构建不会自动覆盖这里的交付快照。

## MCU 开发固件

| 来源与交付快照 | 工厂 → OTA | Bootloader / App 起址 | 验证依据 |
| --- | --- | --- | --- |
| [main](MCU固件/main_v100-to-v101_DEV/README.md) | v100 → v101 | 6 KiB / 0x08001800 | 本次重建、文件校验及 162 项恢复检查通过；零签名占位 |
| [snapshot14](MCU固件/snapshot14_v100-to-v101_DEV/README.md) | v100 → v101 | 6 KiB / 0x08001800 | 本次重建、文件校验及 162 项恢复检查通过；零签名占位 |
| [snapshot12](MCU固件/snapshot12_v102-to-v103_DEV/README.md) | v102 → v103 | 8 KiB / 0x08002000，DEV Bootloader v1 | 既有 DEV 签名包；本次文件及分区校验通过 |
| [snapshot13](MCU固件/snapshot13_v103-to-v104_bootv2_DEV/README.md) | v103 → v104 | 8 KiB / 0x08002000，DEV Bootloader v2 | 既有 DEV 签名包；本次文件及分区校验通过 |

同为 v101 的 main 与 snapshot14 是不同构建，不可只按版本号选包。每组分为 Factory_Flash、App_Upload、Debug_Only、Bootloader、Verification，并附源码指纹、交付 manifest 和 SHA-256 清单。Bootloader 迭代号与 boot_api_version 分别记录。

v103/v104 应用与第三步从当前源码重编译的应用逐字节一致；历史 Bootloader/Factory 的原构建提交未记录。本次没有重新签名或重新执行其签名验签测试，原有日志放在 Verification/Historical。详见各包 delivery-manifest.json 和 [MCU 目录清单](MCU交付目录.json)。

## 手机 APP 与 BLE 模块

| 文件 | 平台/作用 | 实际版本 | 构建号 |
| --- | --- | --- | --- |
| [Android APK](手机APP/Android/v3.6.9_build368/) | Android | 3.6.9 | 368 |
| [iOS IPA](手机APP/iOS/v3.6.9_build98/) | iOS | 3.6.9 | 98 |
| [oven_ble.all.bin](BLE模块固件/oven_ble_版本待核实/) | BLE 模块固件 | 待核实 | 待核实 |
| [EMB1061 两份 HEX 与 J-Flash 配置](BLE模块固件/EMB1061_版本待核实/) | 模块固件与烧录配置 | 待核实 | 待核实 |

详细大小、SHA-256、包标识及元数据依据见 [登记清单](现有安装包与模块固件.json)。两个平台的包标识不同，按实际读取值分别记录。

六个文件均完成迁移前后 SHA-256 核对，保留原文件名。提供方和接收日期待补；J-Flash 配置原本未选择 CurrentFile，本次未修改配置或执行烧录。

开发与重新构建使用对应工程入口：

- [主线 OTA 说明](<../01_固件工程/main/OTA_DEVELOPMENT_README.md>)
- [v103 App_Delivery](<../01_固件工程/development/snapshot-12_secure-ota/tw66gw02/App_Delivery>)
- [v104 App_Delivery](<../01_固件工程/development/snapshot-13_poweroff-v104/tw66gw02/App_Delivery>)

不要混用不同 Bootloader 分区的 Factory、App OTA 包。新增交付使用新目录，按[交付检查表](交付检查表.md)记录来源、适配组合和验证结果，并更新本页及 JSON 清单。整理与恢复依据见[第四步报告](../00_项目索引/第四步交付整理与清理报告.md)。
