# OTA-010 第三步：snapshot-13 安全构建基线

记录日期：2026-09-11。主要构建/测试执行时间 15:38:28～15:39:10（+08:00），随后完成 v104 工厂组合和公钥一致性检查。关联 [固定计划](../../../../plan.md) 的 OTA-010、TEST-004、CHG-004。本文件为证据报告，不替代主计划。

## 结果与范围

**PASS：来源开发线的安全构建与本轮离线检查通过。** 实机烧录、MCU 执行验签、PROD 公钥、真实防回滚与掉电、BLE 联调均未执行。

- v104 官方脚本完成 Bootloader v2、应用、DEV 签名 OTA 及以原 v103 为底包的工厂镜像；v104 应用为 0 错误、0 警告。
- 同一 snapshot-13 源码在另一个隔离目录按 102/103 版本参数完成兼容构建，两应用均为 0 错误、0 警告；运行原有安全产物检查及 150 个换页/回滚中断主机模拟通过。
- v103/v104 两组主机签名检查均通过：合法签名接受，修改应用、修改签名、全零签名和错误公钥全部拒绝。
- 4 项协议单元测试和 PC BLE 工具 `selftest` 通过，无真实 BLE 连接。
- v104 工厂 BIN/HEX、Boot API、编译进 Bootloader 的 i15 公钥模数与实际 DEV 公钥一致性检查通过；公钥为 RSA-3072。
- 原两套工程 675 个受跟踪文件及 main v101/snapshot-13 v104 两组归档共 40 个文件前后哈希不变。两套隔离工程各 388 个输入源码文件也未改变。

## 输入与构建隔离

| 项目 | 基线 |
| --- | --- |
| 来源仓库 | snapshot-13 独立仓库，分支 `test/v104-poweroff-pc-ble` |
| HEAD | `d60b6a8654e6f3ac743a122699d7418b9cdc7d98` |
| 源码恢复包 SHA-256 | `108169060da81d9473eb477a23d6397bf0a38d18c8b9304025481cab9b8ddf7e` |
| 原 v103 输入 SHA-256 | `c3ad66c2302aca9cfcc19bfd8e77197eb7204ca1fe13db7cfa4fe37f7869acae` |
| 环境 | MDK Plus 5.39；ARM Compiler 5.06 update 7 build 960；OpenSSL 3.5.7；本机 Python |
| 隔离根目录 | [ota010-step3](../../../../90_历史原始包/整理前备份/ota010-step3)，位于已有 Git 忽略范围 |

从第一步源码包为 `v104`、`compat-v103` 各提取 388 个输入文件，排除历史 OTA_Artifacts/App_Delivery 及已有 BIN/HEX/AXF/MAP/HTM/LOG 等生成物，避免用旧产物冒充新构建结果。v104 流程额外恢复经校验的原 v103 BIN/HEX，作为 `BuildV104Delivery.ps1` 明确要求的历史输入；它不是从当前 v104 源码重新生成的 v103。

`compat-v103` 中的 102/103 是当前 snapshot-13 源码使用不同版本宏构建的测试组合，不代表恢复了历史 v102/v103 源码。通用脚本输出名 `mathis_secure_bootloader_dev_v1.*` 是沿用的名称，实际来自本轮 Bootloader v2 源码；迁移不能仅凭此文件名判断 Bootloader 迭代。

完整输入、命令、退出码、日志哈希及产物清单在 [result.json](result.json)。两个隔离目录均保留，未将生成的交付件复制回原工程或正式归档。

## DEV 签名材料核验

最初普通权限访问外部 DEV 密钥目录返回拒绝；随后受控执行获准，OpenSSL 在原位置使用已有 DEV 私钥，未复制或打印其内容。生成到隔离目录的只有公开 DER 公钥。签名负向测试按原测试脚本在临时目录生成错误测试密钥并清理，不属于生产密钥。

私钥导出的 DER SubjectPublicKeyInfo、已有 PEM 规范化后的 DER、已有 DER 文件和 Boot API 公钥指纹一致：

```text
3fccdcb727514f80a264fec76933c305262d3ce92f91f8a9711e4eec1a84c0d4
```

额外解码源码 i15 模数并与 OpenSSL 公钥模数比较，确认 3072 位且相等；该 i15 数组实际存在于新编译 Bootloader 内。此项验证避免只比较自报指纹，仍不能代替 MCU 上的实际验签路径测试。

当前没有遗留 DEV 密钥访问阻塞；正式 PROD 公钥仍未提供，DEC-006 的生产部分保持待输入。

## 执行入口与测试证据

由 [run_secure_baseline.py](run_secure_baseline.py) 顺序执行；脚本不覆盖已有运行结果，命令绝对路径见 result.json：

1. 核对 ARMCC/OpenSSL 版本，导出并比对 DEV 公钥。
2. 在 `v104` 执行原 `BuildV104Delivery.ps1 -KeyRoot <现有 DEV 密钥目录>`。
3. 在 `compat-v103` 执行原 `BuildOtaArtifacts.ps1 -FactoryVersion 102 -OtaVersion 103`，显式传入原位置 DEV 密钥/公钥路径。
4. 执行原 `tests/test_ota_artifacts.py` 和 `tests/verify_signed_ota.ps1`。
5. v104 签名检查使用原 verifier 的副本，仅把文件名中的 v103 替换为 v104；原脚本不改，适配副本保存在隔离目录。
6. 执行 `python -m unittest discover` 的 4 项协议测试与 `mathis_ble.py selftest`。
7. 运行 [check_v104.py](check_v104.py)，补充 v104 专用产物、Boot API 和编译公钥验证，结果为 [v104-validation.json](v104-validation.json)。

所有实际执行命令退出码均为 0。为密钥不可用情形准备的 [build_unsigned.ps1](build_unsigned.ps1) 未执行；本轮没有采用跳过签名的备用路径。

| 验证 | 日志/证据 |
| --- | --- |
| 工具与公钥预检 | [compiler-version.log](compiler-version.log)、[openssl-version.log](openssl-version.log)、[derive-dev-public.log](derive-dev-public.log)、[normalize-dev-public.log](normalize-dev-public.log)；公钥预检两份日志为空为正常成功结果，退出码见 result.json |
| v104 交付构建 | [v104-delivery.log](v104-delivery.log)、[v104 应用构建日志](v104-build_v104.log) |
| 102/103 兼容构建 | [构建控制台](compat-v102-v103-build.log)、[102 应用日志](compat-v103-build_secure_v102.log)、[103 应用日志](compat-v103-build_v103.log) |
| 安全布局、CRC、manifest、150 个中断模拟 | [secure-artifacts.log](secure-artifacts.log) |
| 合法/篡改应用/篡改签名/零签名/错误公钥 | [signature-v103.log](signature-v103.log)、[signature-v104.log](signature-v104.log) |
| 4 项协议测试、PC BLE 离线自检 | [protocol-tests.log](protocol-tests.log)、[pc-ble-selftest.log](pc-ble-selftest.log) |
| Boot 区大小及静态调用栈 | [Bootloader map](v104-mathis_bootloader.map)、[调用图](v104-mathis_bootloader.htm) |

150 个用例是 75 个正向换页中断点加 75 个回滚中断点的模型测试；测试中的源码字符串检查只证明存在相关调用，不等于已验证 MCU 防回滚行为。

## v104 产物及容量

| 产物 | 字节 | SHA-256 |
| --- | ---: | --- |
| Bootloader v2 | 8188 | `db88e94fab70bcea08ad03ec66d628b15dcac8482e2d85fb0624f9f18c86d8d9` |
| 原 v103 + Bootloader v2 工厂 BIN | 31432 | `9dda30be007f7d037e465f95a08e0f438a043ec444ed297a6ab0a03fb9b4b882` |
| v104 裸应用 | 24476 | `e4fe846bd6e31ff963cd236c09ef3458c07a99d2bcfb5257ee11af6bfebd4107` |
| v104 DEV OTA | 24860 | `4ea2f3917c8461c6d37abae1c45e33476706bcb9670b8aa18af7b9d5da0e5468` |

OTA CRC-32 为 `0xC8BCBCC5`，与冻结 v104 OTA 哈希一致。工厂 BIN/HEX 一致；原 v103 输入未变。

| 容量/接口 | 本轮测得 |
| --- | --- |
| v104 应用上限/实际/余量 | 25216 / 24476 / **740 字节** |
| 应用 Code / RO / RW / ZI | 23560 / 624 / 292 / 4828 字节；RW+ZI=5120 |
| Boot 主区使用/上限/余量 | `0x1D0C` / `0x1FC0` / **692 字节** |
| Boot API 使用/预留 | 60 / 64 字节 |
| Boot API | ABI=1、layout=2、capabilities=0x05（DEV + anti-rollback），PROD 位未启用 |
| 静态验签调用深度 | 链接器报告 1688 字节；报告同时提示循环和不可追踪函数指针存在 Unknown 项 |

上述栈数值不能当作完整最坏运行时上限，动态水印、中断叠加和真实 MCU RSA 必须后续验证。当前容量有限，双公钥与签名版本描述的新增空间需在迁移时重新测量，不能沿用本轮通过结论。

## 下一步

第三步没有留下新的构建失败项；OTA-010 整体仍为 IN_PROGRESS。下一步执行第四步：形成 main 与 snapshot-13 的模块迁移清单，逐项标明迁移/保留/适配、依赖、验证方法及恢复点，特别记录 DEV-only、公钥扩展、版本绑定、容量余量和 Bootloader 文件命名差异。本轮不执行源代码迁移。
