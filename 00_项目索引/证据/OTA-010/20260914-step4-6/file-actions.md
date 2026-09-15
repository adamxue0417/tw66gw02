# 逐文件迁移分类

[主计划](../../../../plan.md) · [模块与批次说明](README.md)

仅比较受跟踪源码、配置和脚本；以下是后续实施分类，并非已迁移结果。

| 文件 | 比较结果 | 处理 |
| --- | --- | --- |
| `BSP/C8721.c` | CHANGED | 保留 main；不引入显示/关机策略变化 |
| `BSP/C8721.h` | CHANGED | 保留 main；不引入显示/关机策略变化 |
| `BSP/battery_level.c` | SAME | 保留 main（相同） |
| `BSP/battery_level.h` | SAME | 保留 main（相同） |
| `BSP/bh66f5242.c` | SAME | 保留 main（相同） |
| `BSP/bh66f5242.h` | SAME | 保留 main（相同） |
| `BSP/boot_api.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `BSP/buzz.c` | SAME | 保留 main（相同） |
| `BSP/buzz.h` | SAME | 保留 main（相同） |
| `BSP/config.h` | CHANGED | 保留 main；不引入显示/关机策略变化 |
| `BSP/key.c` | SAME | 保留 main（相同） |
| `BSP/key.h` | SAME | 保留 main（相同） |
| `BSP/ota_layout.h` | CHANGED | 需适配；见模块清单 |
| `BSP/power_latch.h` | SOURCE_ONLY | 保留 main；不引入显示/关机策略变化 |
| `BSP/screen_c8721.c` | CHANGED | 保留 main；不引入显示/关机策略变化 |
| `BSP/screen_c8721.h` | CHANGED | 保留 main；不引入显示/关机策略变化 |
| `BSP/temp.c` | CHANGED | 保留 main；不引入显示/关机策略变化 |
| `BSP/temp.h` | SAME | 保留 main（相同） |
| `BSP/wireless.c` | CHANGED | 需适配；见模块清单 |
| `BSP/wireless.h` | CHANGED | 需适配；见模块清单 |
| `Bootloader/boot_main.c` | CHANGED | 需适配；见模块清单 |
| `Bootloader/boot_security.c` | SOURCE_ONLY | 需适配；见模块清单 |
| `Bootloader/boot_security.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `Bootloader/bootloader.sct` | CHANGED | 需适配；见模块清单 |
| `Bootloader/build_bootloader.ps1` | CHANGED | 需适配；见模块清单 |
| `Bootloader/build_min_bootloader.ps1` | SAME | 保留 main（相同） |
| `Bootloader/min_boot_main.c` | SAME | 保留 main（相同） |
| `BuildKeilFactoryTarget.ps1` | SAME | 保留 main（相同） |
| `BuildOtaArtifacts.ps1` | CHANGED | 需适配；见模块清单 |
| `BuildStartupDiagnostic.ps1` | CHANGED | 需适配；见模块清单 |
| `BuildV104Delivery.ps1` | SOURCE_ONLY | 保留 main 生成/工程入口；参考件不直接复制 |
| `COP/gagent_md5.c` | SAME | 保留 main（相同） |
| `COP/gagent_md5.h` | SAME | 保留 main（相同） |
| `COP/idle.c` | SAME | 保留 main（相同） |
| `COP/idle.h` | SAME | 保留 main（相同） |
| `COP/main_control.c` | CHANGED | 保留 main；不引入显示/关机策略变化 |
| `COP/main_control.h` | SAME | 保留 main（相同） |
| `COP/ota.c` | SAME | 保留 main（相同） |
| `COP/ota.h` | CHANGED | 迁移；按批次验证 |
| `COP/ota_boot.c` | CHANGED | 需适配；见模块清单 |
| `COP/ota_boot.h` | SAME | 保留 main（相同） |
| `COP/ota_update.c` | CHANGED | 需适配；见模块清单 |
| `COP/ota_update.h` | SAME | 保留 main（相同） |
| `COP/pid.c` | SAME | 保留 main（相同） |
| `COP/pid.h` | SAME | 保留 main（相同） |
| `COP/run.c` | SAME | 保留 main（相同） |
| `COP/run.h` | SAME | 保留 main（相同） |
| `COP/set.c` | SAME | 保留 main（相同） |
| `COP/set.h` | SAME | 保留 main（相同） |
| `COP/shutdown.c` | SAME | 保留 main（相同） |
| `COP/shutdown.h` | SAME | 保留 main（相同） |
| `COP/start_up.c` | SAME | 保留 main（相同） |
| `COP/start_up.h` | SAME | 保留 main（相同） |
| `COP/test.c` | SAME | 保留 main（相同） |
| `COP/test.h` | SAME | 保留 main（相同） |
| `COP/warning.c` | SAME | 保留 main（相同） |
| `COP/warning.h` | SAME | 保留 main（相同） |
| `Core/Inc/adc.h` | SAME | 保留 main（相同） |
| `Core/Inc/dma.h` | SAME | 保留 main（相同） |
| `Core/Inc/gpio.h` | SAME | 保留 main（相同） |
| `Core/Inc/main.h` | SAME | 保留 main（相同） |
| `Core/Inc/stm32f0xx_hal_conf.h` | SAME | 保留 main（相同） |
| `Core/Inc/stm32f0xx_it.h` | SAME | 保留 main（相同） |
| `Core/Inc/tim.h` | SAME | 保留 main（相同） |
| `Core/Inc/usart.h` | CHANGED | 迁移；按批次验证 |
| `Core/Src/adc.c` | SAME | 保留 main（相同） |
| `Core/Src/dma.c` | SAME | 保留 main（相同） |
| `Core/Src/gpio.c` | CHANGED | 需适配；见模块清单 |
| `Core/Src/main.c` | CHANGED | 保留 main；不引入显示/关机策略变化 |
| `Core/Src/stm32f0xx_hal_msp.c` | SAME | 保留 main（相同） |
| `Core/Src/stm32f0xx_it.c` | SAME | 保留 main（相同） |
| `Core/Src/system_stm32f0xx.c` | CHANGED | 保留 main；不引入显示/关机策略变化 |
| `Core/Src/tim.c` | SAME | 保留 main（相同） |
| `Core/Src/usart.c` | CHANGED | 迁移；按批次验证 |
| `Diagnostics/build_gpio_alive.ps1` | SAME | 保留 main（相同） |
| `Diagnostics/gpio_alive.c` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Device/ST/STM32F0xx/Include/stm32f030x8.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Device/ST/STM32F0xx/Include/stm32f0xx.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Device/ST/STM32F0xx/Include/system_stm32f0xx.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/cmsis_armcc.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/cmsis_armclang.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/cmsis_compiler.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/cmsis_gcc.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/cmsis_iccarm.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/cmsis_version.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_armv8mbl.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_armv8mml.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_cm0.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_cm0plus.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_cm1.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_cm23.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_cm3.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_cm33.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_cm4.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_cm7.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_sc000.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/core_sc300.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/mpu_armv7.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/mpu_armv8.h` | SAME | 保留 main（相同） |
| `Drivers/CMSIS/Include/tz_context.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/Legacy/stm32_hal_legacy.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_adc.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_adc_ex.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_cortex.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_def.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_dma.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_dma_ex.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_exti.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_flash.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_flash_ex.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_gpio.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_gpio_ex.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_i2c.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_i2c_ex.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_pwr.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_pwr_ex.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_rcc.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_rcc_ex.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_tim.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_tim_ex.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_uart.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_hal_uart_ex.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_adc.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_bus.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_cortex.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_crs.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_dma.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_exti.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_gpio.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_pwr.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_rcc.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_system.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_tim.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_usart.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Inc/stm32f0xx_ll_utils.h` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_adc.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_adc_ex.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_cortex.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_dma.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_exti.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_flash.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_flash_ex.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_gpio.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_i2c.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_i2c_ex.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_pwr.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_pwr_ex.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_rcc.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_rcc_ex.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_tim.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_tim_ex.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_uart.c` | SAME | 保留 main（相同） |
| `Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_uart_ex.c` | SAME | 保留 main（相同） |
| `MDK-ARM/RTE/_TEST2/RTE_Components.h` | SOURCE_ONLY | 保留 main 生成/工程入口；参考件不直接复制 |
| `MDK-ARM/RTE/_TW66GW02/RTE_Components.h` | SOURCE_ONLY | 保留 main 生成/工程入口；参考件不直接复制 |
| `MDK-ARM/RTE/_tw66gw02/RTE_Components.h` | MAIN_ONLY | 保留 main 生成/工程入口；参考件不直接复制 |
| `MDK-ARM/boot_image.s` | SOURCE_ONLY | 保留 main 生成/工程入口；参考件不直接复制 |
| `MDK-ARM/startup_stm32f030x8.s` | CHANGED | 迁移；按批次验证 |
| `MDK-ARM/tw66gw02.sct` | SOURCE_ONLY | 保留 main 生成/工程入口；参考件不直接复制 |
| `MDK-ARM/tw66gw02.uvprojx` | SAME | 保留 main（相同） |
| `MDK-ARM/tw66gw02/TEST2.sct` | SOURCE_ONLY | 保留 main 生成/工程入口；参考件不直接复制 |
| `MDK-ARM/tw66gw02/tw66gw02.sct` | SOURCE_ONLY | 保留 main 生成/工程入口；参考件不直接复制 |
| `MDK-ARM/tw66gw02_app.sct` | CHANGED | 需适配；见模块清单 |
| `MDK-ARM/tw66gw02_direct.sct` | CHANGED | 保留 main；不引入显示/关机策略变化 |
| `MDK-ARM/tw66gw02_factory.sct` | CHANGED | 需适配；见模块清单 |
| `OS/TaskScheduler.c` | CHANGED | 迁移；按批次验证 |
| `OS/TaskScheduler.h` | CHANGED | 迁移；按批次验证 |
| `PrepareFactoryBootImage.ps1` | CHANGED | 需适配；见模块清单 |
| `ProgramBootloaderOnlyWithJLink.ps1` | SAME | 保留 main（相同） |
| `ProgramFactoryWithJLink.ps1` | SAME | 保留 main（相同） |
| `ProgramSecureFactoryWithJLink.ps1` | SOURCE_ONLY | 需适配；见模块清单 |
| `ReadStartupStateWithJLink.ps1` | SAME | 保留 main（相同） |
| `ThirdParty/BearSSL/inc/bearssl.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_aead.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_block.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_ec.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_hash.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_hmac.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_kdf.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_pem.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_prf.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_rand.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_rsa.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_ssl.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/inc/bearssl_x509.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/codec/ccopy.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/codec/dec32be.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/codec/enc32be.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/config.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/hash/sha2small.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/inner.h` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_add.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_bitlen.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_decmod.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_decode.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_encode.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_fmont.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_modpow2.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_montmul.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_muladd.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_ninv15.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_sub.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/int/i15_tmont.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/rsa/rsa_i15_pkcs1_vrfy.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/rsa/rsa_i15_pub.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `ThirdParty/BearSSL/src/rsa/rsa_pkcs1_sig_unpad.c` | SOURCE_ONLY | 迁移；按批次验证 |
| `Tools/PC_BLE_Test/Run-PC-BLE.ps1` | SOURCE_ONLY | 迁移为可选联调工具；不打包虚拟环境 |
| `Tools/PC_BLE_Test/Setup-PC-BLE.ps1` | SOURCE_ONLY | 迁移为可选联调工具；不打包虚拟环境 |
| `Tools/PC_BLE_Test/mathis_ble.py` | SOURCE_ONLY | 迁移为可选联调工具；不打包虚拟环境 |
| `Tools/PC_BLE_Test/protocol.py` | SOURCE_ONLY | 迁移为可选联调工具；不打包虚拟环境 |
| `Tools/PC_BLE_Test/tests/test_protocol.py` | SOURCE_ONLY | 迁移为可选联调工具；不打包虚拟环境 |
| `VerifySecureProvisioningWithJLink.ps1` | SOURCE_ONLY | 需适配；见模块清单 |
| `tests/test_ota_artifacts.py` | CHANGED | 需适配；见模块清单 |
| `tests/verify_signed_ota.ps1` | SOURCE_ONLY | 需适配；见模块清单 |
| `tw66gw02.ioc` | SAME | 保留 main（相同） |
