# snapshot12_v102-to-v103_DEV

[返回交付入口](../../README.md)

本目录是本地开发交付快照，保留 DEV 标记，未进行硬件烧录、真实 BLE 联调或量产签核。

| 项目 | 说明 |
| --- | --- |
| 源工程 | `01_固件工程/development/snapshot-12_secure-ota/tw66gw02` |
| 版本 | 工厂 v102 → OTA v103 |
| 分区 | Bootloader 8 KiB；App `0x08002000`；槽 25 KiB |
| Bootloader | 1；manifest boot_api_version = 1，API 号与 Bootloader 迭代号分别记录 |
| 硬件/协议适配 | 工程目标 STM32F030C8；板卡修订号及 MCU/BLE/手机 APP 适配组合待实机核实；待核实；公共 BLE 文档 v0.4 Draft 不等同于本包已验证协议版本 |
| 编译工具 | Keil uVision / ARM Compiler 5.06 update 7；历史 Bootloader 原构建环境仅有现存脚本/日志依据 |
| 本次校验 | OTA 大小、SHA-256、CRC、应用内容、工厂镜像和分区一致性通过 |

既有 DEV 签名交付件；第三步当前源码应用重编译与本包应用字节一致：True。Bootloader/Factory 原始构建提交未记录，当前源码指纹仅供追溯，不能证明其历史构建来源。

- [Factory_Flash](Factory_Flash/)：本包对应 Bootloader 的完整工厂镜像。
- [App_Upload](App_Upload/)：OTA 原始包及相邻 manifest；不可截掉签名尾部。
- [Debug_Only](Debug_Only/)：应用裸镜像，仅供调试，不用于工厂烧录或手机上传。
- [Bootloader](Bootloader/)：与本包工厂镜像匹配的引导程序参考副本。
- [验证记录](Verification/)：本次文件校验和带来源的构建记录；Historical 下是原有日志，不表示本次重新执行签名验证。
- [交付 manifest](delivery-manifest.json)、[源码指纹](source-fingerprint.json)、[全包 SHA-256](SHA256SUMS.txt)。

不同源工程即使版本号相同也不能互换；6 KiB 与 8 KiB 引导分区不能混用。后续正式交付按[交付检查表](../../交付检查表.md)补齐实机验证和适配记录。
