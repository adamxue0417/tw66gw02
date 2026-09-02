# v103安全OTA、整体联调与内存优化实施计划

本分支从`87c1d0e`创建，目标是在不改动v102分支和旧产物的前提下，实现RSA-3072双重验签、防降级、25 KiB双槽布局、DEV安全工厂基线、RAM优化、App交付包及联调验收文档。

实施边界：仅修改目录12；目录11只读校验。私钥仅位于`12/private_keys`且不进入Git或App包。App只发送`[应用BIN][384字节签名]`原始文件。

主要里程碑：

1. 从`87c1d0e`建立`test/v103-integration-memory`，确认远端无冲突分支。
2. 固定Bootloader至8 KiB、应用/暂存槽各25 KiB；Boot API固定在`0x08001FC0`。
3. Bootloader固化DEV公钥，以RSA-3072/PKCS#1 v1.5/SHA-256在应用COMMIT和Boot安装前各验一次。
4. 在`0x0800E800`实现双副本单向安全版本日志，基线102；稳定运行10秒确认后提交新安全版本。
5. 调度器、USART、Wireless及Heap优化；保留2304字节栈并验证RSA余量。
6. 构建安全v102工厂镜像和v103签名OTA，生成manifest、日志、SHA-256清单及App包。
7. 离线执行布局/CRC/签名/150个断电交换模型；上板执行WRP、BLE、断电、显示和动态栈水印验收。
8. 分两次提交并推送，禁止强制推送。

详细烧录、App步骤、状态码、故障恢复及验收表见`V103_INTEGRATION_TEST_GUIDE.md`。
