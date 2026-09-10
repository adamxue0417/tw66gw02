# main 主工程

[返回工程导航](../README.md) · [项目入口](../../README.md)

该工程由原工作区根目录整体迁入，根工作区 `.git` 保持原位。入口为 [Keil 工程](MDK-ARM/tw66gw02.uvprojx)。BSP、COP、Core、Drivers、OS、Bootloader、工程配置、构建脚本和测试保持原相对结构。

构建使用 [BuildOtaArtifacts.ps1](BuildOtaArtifacts.ps1)，说明见 [OTA 开发文档](OTA_DEVELOPMENT_README.md) 和 [烧录联调说明](OTA_烧录与APP联调说明.md)。Windows PowerShell 5.1 环境中，可从当前工程目录执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\BuildOtaArtifacts.ps1
python .\tests\test_ota_artifacts.py
```

2026-09-09 已在迁移后路径生成 v100 工厂镜像及 v101 OTA 包，并通过产物、内存布局、CRC 和 162 项中断交换/回滚检查。本轮没有烧录设备。

这里使用 6 KiB Bootloader 和开发签名占位。现有产物属于开发验证；不能与 12/13 的 8 KiB 安全 OTA 包混用。
