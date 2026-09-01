# Mathis STM32F030C8 development OTA

This build implements the Section 9 BLE OTA transport and a real, recoverable application
replacement using only the STM32F030C8 internal flash. It is a development build:
the 384-byte signature trailer is present but RSA verification and anti-rollback
are intentionally not enabled.

## Build artifacts

Run from PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\BuildOtaArtifacts.ps1 -OtaOnly -OtaVersion 102
```

The script produces:

- Existing `OTA_Artifacts/mathis_factory_v100.hex`: bootloader plus relocated
  baseline v100 app; program this once with SWD before OTA testing. `-OtaOnly`
  deliberately leaves this file unchanged.
- `OTA_Artifacts/mathis_ota_v102.ota`: file to bundle in the app.
- `OTA_Artifacts/mathis_ota_v102.manifest.json`: exact artifact size, CRC-32 and
  target version for `OTA_BEGIN`.

The app engineer must replace the current 4096-byte dummy artifact with the `.ota`
file and use the manifest values. A normal HEX, AXF, or application BIN is not an
OTA artifact.

## First SWD programming

Preferred: erase the device once, then program
`OTA_Artifacts/mathis_factory_v100.hex`. Intel HEX records already contain both
addresses, so do not enter a manual load address:

- bootloader starts at `0x08000000`;
- relocated v100 application starts at `0x08001800`.

If the programmer has trouble with a segmented HEX, program the continuous
`OTA_Artifacts/mathis_factory_v100.bin` once at `0x08000000`.

The checked-in Keil target is now a factory target. Before Make it rebuilds the
bootloader, embeds `boot_image.o`, and links one AXF with a boot region at
`0x08000000` and an application region at `0x08001800`. Normal Keil **Download**
therefore writes both regions. Do not download the standalone
`mathis_app_v100.hex` as a factory image: it deliberately contains only the
relocated application. The repository's earlier J-Link log captured this exact
mixed-image failure: a stale direct-application vector remained at `0x08000000`,
the relocated application overwrote part of the old code, and the CPU ended in
HardFault with PC `0xFFFFFFFE`.

To rebuild and independently verify the combined Keil AXF/HEX/BIN:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\BuildKeilFactoryTarget.ps1
```

Build outputs are isolated so an OTA packaging run cannot replace the Keil
factory AXF:

- `MDK-ARM/tw66gw02/`: complete factory AXF used by the checked-in Keil target;
- `MDK-ARM/tw66gw02_ota_v100/`, `_v101/`, and `_v102/`: relocated app-only OTA builds;
- `MDK-ARM/tw66gw02_direct/`: direct-start diagnostic build.

With J-Link connected, the deterministic factory-programming command is:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\ProgramFactoryWithJLink.ps1
```

It performs a full erase, loads the continuous factory BIN at `0x08000000`,
verifies it, prints both boot/application vectors, resets, and runs. To capture
PC, registers, both vectors, and the SRAM boot trace without another reset, run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\ReadStartupStateWithJLink.ps1
```

PB3 is the board power-hold latch. Both the bootloader and application assert
PB3 from their respective `SystemInit()` routines, before C runtime/application
initialization can release a momentary push-button supply.

### Three-way startup diagnosis

Erase the complete chip before every case and program each BIN at
`0x08000000`. Test with the normal battery/board supply; an ST-Link 3.3 V output
may power the MCU but is not evidence that it can power the display rail.

Before the three application cases, `diagnostic_gpio_alive.bin` is a 560-byte
bare-register test with no HAL, screen driver, relocated vector table, or OTA
code. It holds PB3 high and periodically toggles PC13 (run LED/test point) and
PB8 (buzzer). Program it at `0x08000000`; if neither pin toggles, investigate
reset/BOOT0/option bytes/supply/programming rather than the bootloader.

1. `diagnostic_direct_v100.bin`: application linked directly at flash base; no
   bootloader and no vector remap.
2. `diagnostic_minboot_factory_v100.bin`: 680-byte power-hold/vector-remap/jump
   bootloader plus the relocated application; no OTA metadata or swap code.
3. `mathis_factory_v100.bin`: complete OTA bootloader plus relocated application.

Interpretation:

- case 1 fails: investigate power, BOOT0, reset, option bytes, chip identity or
  programmer settings; the OTA bootloader is not executing in this case;
- case 1 passes and case 2 fails: vector relocation/remap path is at fault;
- cases 1 and 2 pass but case 3 fails: full OTA boot state/recovery path is at fault;
- all three pass: the corrected complete factory image is usable.

For diagnosis they may instead be programmed separately. Program
`Bootloader/build/mathis_bootloader.hex` first, then program
`OTA_Artifacts/mathis_app_v100.hex` using sector/page erase only. Do not perform a
second full-chip erase, because that would remove the bootloader. If raw BIN files
are used, explicitly select `0x08000000` for the bootloader and `0x08001800` for
the application. Keep BOOT0 low for normal flash startup.

### SWD startup trace

This development build reserves `0x20001FF0`. Halt the MCU without resetting it
and read the 32-bit value at that address:

| Value | Last completed stage |
| --- | --- |
| `0xB0070001` | bootloader entered |
| `0xB0070002` | application vector accepted |
| `0xB0070003` | SRAM vector remap completed; application branch is next |
| `0xB00700EE` | invalid application vector |
| `0xB00700EF` | SRAM vector remap failed |
| `0xA9900001` | application `main` entered |
| `0xA9900002` | HAL initialized |
| `0xA9900003` | 48 MHz clock initialized |
| `0xA9900004` | GPIO/DMA/ADC/UART initialized |
| `0xA9900005` | persistent data initialized |
| `0xA9900006` | screen initialized |
| `0xA9900007` | scheduler loop running |
| `0xA99000EE` | application entered `Error_Handler` |

## Memory map and limits

| Region | Address | Size |
| --- | --- | ---: |
| Bootloader | `0x08000000` | 6 KiB |
| Running application | `0x08001800` | 27 KiB |
| Staged/rollback image | `0x08008400` | 27 KiB |
| Swap scratch | `0x0800F000` | 1 KiB |
| OTA journal | `0x0800F400` | 1 KiB |
| Persistent settings A/B | `0x0800F800` | 2 KiB |

The application BIN must not exceed 27264 bytes. The OTA artifact is the application
BIN followed by a 384-byte placeholder signature and must not exceed 27648 bytes.

## Safety boundary

The running application is never modified while downloading. The bootloader swaps
pages with a persistent step journal and resumes after a power interruption. A new
image is trial-booted and confirmed after ten seconds of stable scheduler operation;
another reset before confirmation restores the old image.

This build must never ship to customers. Production requires RSA-3072 verification,
production trust-anchor provisioning and anti-rollback enforcement.
