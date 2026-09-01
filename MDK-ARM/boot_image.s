                AREA    BOOT_IMAGE, DATA, READONLY, ALIGN=2
                EXPORT  __boot_image_start
                EXPORT  __boot_image_end
__boot_image_start
                INCBIN  Bootloader\build\mathis_bootloader.bin
__boot_image_end
                END
