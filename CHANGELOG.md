# Changelog

## Unreleased - 2026-09-02

### Added

- Added the Mathis BLE specification v0.4 development OTA transport with 232-byte lockstep chunks, CRC-32 verification, timeout/abort handling, and device-owned OTA status sequences.
- Added a 6 KiB bootloader, dual 27 KiB application/staging slots, persistent page-swap journal, ten-second trial confirmation, and interrupted rollback recovery for STM32F030C8.
- Added reproducible v100/v101 factory and App artifact generation, validation of 162 interrupted swap/rollback points, and physically separated Factory/App/Debug delivery folders.

### Security note

- Development OTA artifacts contain a 384-byte signature placeholder; RSA-3072 verification and anti-rollback remain intentionally disabled, so this build must not ship to customers.

### Fixed

- Made BLE telemetry temperature values remain in whole degrees Celsius regardless of the device display-unit preference, as required by Mathis BLE Specification v0.3.
- Updated Fahrenheit display conversion to round to the nearest whole degree with half values away from zero, keeping the device display consistent with the app.
- Synchronized the generated HEX file to both `MDK-ARM/tw66gw02.hex` and the project-root `tw66gw02.hex` after every successful Keil build so production flashing cannot accidentally use a stale firmware image.
