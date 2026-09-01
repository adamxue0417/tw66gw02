# Changelog

## Unreleased - 2026-08-19

### Fixed

- Made BLE telemetry temperature values remain in whole degrees Celsius regardless of the device display-unit preference, as required by Mathis BLE Specification v0.3.
- Updated Fahrenheit display conversion to round to the nearest whole degree with half values away from zero, keeping the device display consistent with the app.
- Synchronized the generated HEX file to both `MDK-ARM/tw66gw02.hex` and the project-root `tw66gw02.hex` after every successful Keil build so production flashing cannot accidentally use a stale firmware image.
