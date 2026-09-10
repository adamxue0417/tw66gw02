# Mathis BLE Specification

Version: 0.4 (Draft)
Last edited: 26 August 2026

## Changelog

- **0.4 (26 August 2026)** — OTA (§9) clarifications for the first app + firmware implementation. Chunk acknowledgement is now **per-chunk** in v1 (windowed acknowledgement removed; reserved as a future READY-payload extension). The READY chunk size must not exceed the frame payload cap (**≤ 232 bytes**); the app clamps defensively. Documented `OTA_STATUS` sequence semantics (device-owned counter; correlate by state and payload, never by sequence). Added an MTU note (only Android centrals request an MTU; iOS auto-negotiates). See sections 9.4, 9.5, and 9.6.
- **0.3 (28 July 2026)** — Temperatures on the wire are now **always °C** regardless of the display unit (previously "current device unit"). Mode bit 4 and `SET_UNITS` are redefined as display preference only. Added the display rounding rule so the device display and app show identical digits. See section 7.1.
- **0.2 (11 June 2026)** — Baseline draft.

Mathis BLE temperature module is a Bluetooth Low Energy thermometer and display device. The phone app connects to it, reads live temperatures, occasionally sends settings, and can
deliver firmware updates. There is no WiFi, no cloud service, and no appliance
control.

This document defines the **firmware contract**: how the device advertises, how
the link is secured, how data is framed, what the device sends, and what the
device accepts. It describes what the firmware must implement, not how the app
drives it. Where the spec depends on module capability, firmware should flag
anything it cannot implement.

---

## 1. Connection and Transport

### 1.1 One service, two characteristics

The device exposes a **single BLE service** with **two characteristics**. This is
the standard serial-over-BLE pattern: one characteristic carries data into the
device, the other carries data out.

| Name   | Direction    | Properties | Purpose                                |
| ------ | ------------ | ---------- | -------------------------------------- |
| **RX** | App → Device | Write      | The app writes messages to the device. |
| **TX** | Device → App | Notify     | The device sends messages to the app.  |

This is the entire BLE surface. Telemetry, commands, and firmware updates are
**not** separate characteristics. They are all **messages** (see §4) sent over
these same two characteristics and told apart by a message **Type** byte.

### 1.2 Packet size (MTU)

- The protocol must operate correctly at the default ATT MTU of **23 bytes**.
- At MTU 23, one BLE write or notification carries 20 bytes of characteristic
  value data.
- All non-OTA Mathis messages are sized to fit in one MTU-23 BLE write or
  notification. Receivers should still validate full frames using the Mathis
  start marker, Length field, and CRC, but regular telemetry and commands do not
  require framed-message fragmentation.
- OTA may use larger framed messages split across multiple BLE writes or
  notifications. OTA chunk sizing is defined in §9.
- Larger MTUs, including MTU 247 if the module supports it, may be negotiated and
  used as an OTA throughput optimization.
- No protocol feature requires MTU above 23. Coefficient updates are split into
  three default-MTU-safe command payloads (§6.2), and OTA uses a
  device-selected chunk size that can be small enough for the negotiated link and
  UART path.

### 1.3 Connection rules

- Only **one central may be connected at a time**. While a connection is active,
  the device rejects new connection attempts.
- The device delivers all Device → App data as **notifications on the TX
  characteristic**. Nothing is sent until TX notifications are subscribed.
- After TX notifications are subscribed, the device starts telemetry streaming.

---

## 2. Advertising and Discovery

The device advertises so a central can discover and reconnect to it.

**Address:** the device uses a **Static Random Address** that is unique per
device and stays the same across normal use and factory reset. The two most
significant bits of the address must be `11`, as required for a Static Random
Address. The device must **not** use a Resolvable Private Address — a stable
address is required for the app to recognise the device and for iOS background
reconnection.

**Background reconnection:** the full 128-bit service UUID must appear in the
advertising packet (not only the scan response) so iOS can reconnect while the
app is backgrounded.

### 2.1 Advertising packet

| AD Type | Field        | Content                                | Notes                                                                                                  |
| ------- | ------------ | -------------------------------------- | ------------------------------------------------------------------------------------------------------ |
| `0x01`  | Flags        | `0x06`                                 | General Discoverable, BR/EDR not supported.                                                            |
| `0x07`  | 128-bit UUID | `441C1000-776D-B95D-75A7-496DD1B5BEDA` | Primary service UUID, in full.                                                                         |
| `0x08`  | Short Name   | `mat`                                  | Short form to fit the 31-byte limit. Full name is in the scan response. _(Confirm before production.)_ |

### 2.2 Scan response

| AD Type | Field                 | Content                                               | Notes                                                        |
| ------- | --------------------- | ----------------------------------------------------- | ------------------------------------------------------------ |
| `0x09`  | Complete Name         | `mat-XXX`                                             | Full name. _(Confirm before production.)_                    |
| `0x21`  | Service Data — 128bit | Service UUID + `[model_id]`, where Model ID is `0x04` | Identifies the Mathis model without using Manufacturer Data. |

This fits in one 31-byte scan response: Complete Name uses 9 bytes on air
(`length + type + 7-byte name`) and Service Data uses 19 bytes
(`length + type + 16-byte UUID + 1-byte Model ID`), for 28 bytes total.

**No manufacturer-specific data.** Mathis does **not** advertise a `0xFF`
Manufacturer Specific Data field. That AD type requires a Bluetooth SIG-assigned
Company Identifier in its first two bytes, and Ooni holds no such ID. The device
is identified instead by its 128-bit primary service UUID in the advertising
packet (§2.1), with Model ID carried as Service Data in the scan response.
Serial number is not advertised.

### 2.3 Advertising intervals

- **Fast (100 ms):** for the first 60 seconds after power-on or disconnect.
- **Slow (1000 ms):** when idle.

### 2.4 Compliance guardrails

Mathis is a low-risk thermometer/display product and does not control the grill.
Firmware must still keep the BLE radio within the approved module and regional
settings used for ETSI/RED compliance:

- Do not change RF output power, radio test modes, PHY behaviour, or antenna
  assumptions outside the approved EMB1082-P configuration.
- Advertising content may be changed using the module's supported AT commands,
  but must remain within normal BLE advertising limits.
- Any factory or production RF test mode must be disabled in normal user
  firmware.

---

## 3. Security and Pairing

If the BLE module supports it natively, the device should use **LE Secure
Connections with Just Works pairing**: an encrypted link, with no PIN and no
button press. The device has only a 3-digit display, so it cannot show a 6-digit
pairing code; Just Works is the appropriate BLE pairing method.

**LE Secure Connections is the preferred BLE security mode.** When enabled,
pairing must use LE Secure Connections (the ECDH-based key exchange) and must not
fall back to LE legacy pairing. This matters because Just Works is
unauthenticated: under LESC the encryption still resists a _passive_
eavesdropper (a sniffer cannot derive the key), whereas under legacy Just Works
the key is trivially recoverable and the "encrypted" link gives no
confidentiality.

**No bonding.** The device does **not** store a long-term pairing key. Each time
the app connects, the link is paired and encrypted fresh. There is no saved
bond to reuse, and no bonded-device list to manage.

This is a proportionate BLE security target for a read-mostly thermometer that
exposes no credentials and no safety-critical controls. LESC Just Works protects
against passive eavesdropping, but it does **not** authenticate the central. A
nearby phone that connects first may be able to send settings or maintenance
commands. This residual risk is acceptable only because the Mathis temperature device does not control the grill or expose user credentials.

OTA image authenticity must not depend on BLE link security. Firmware signing
and bootloader verification are the launch-critical security/compliance controls
for firmware updates; the OTA CRC/checksum only protects transfer integrity (§9).

### 3.1 Connection flow

1. The device advertises (§2).
2. A central connects. The link starts unencrypted.
3. If encrypted characteristics are supported and enabled by the module, the
   first protected access (subscribing to TX or writing to RX) triggers **Just
   Works LE Secure Connections pairing**. Both sides derive an encryption key for
   this connection only.
4. The app subscribes to TX notifications.
5. The device starts telemetry streaming. The first valid `TELEMETRY` message
   confirms that the connection is usable and gives the app the current device
   state.

If the module can mark RX/TX as encrypted, firmware should rely on the module to
gate access. If the module cannot enforce encrypted characteristic access or
cleanly expose encryption state to the STM32, do not add a custom STM32-side
UART gate by default; flag the module capability and product risk for review.

On every reconnect the same flow runs again from the start — there is no stored
key to skip pairing when LESC is enabled.

### 3.2 Factory reset

A factory reset clears:

- notification / subscription state,
- user preferences (e.g. temperature units),
- any stored firmware-update result.

A factory reset must **not** change the device's permanent identity: serial
number, Model ID, or firmware signing trust anchors.

---

## 4. Message Format

Everything sent over the link is a **message**. Each message is self-contained
so the receiver can find where one message ends and the next begins, even though
the underlying connection is a raw byte stream that may join or split writes.

### 4.1 Message structure

| Offset | Field        | Size | Description                                                                                                                         |
| ------ | ------------ | ---- | ----------------------------------------------------------------------------------------------------------------------------------- |
| 0      | Start marker | 2    | Fixed value `0x5A 0x5A`.                                                                                                            |
| 2      | Type         | 1    | Message type (§4.4). The top bit shows direction: `0x00`–`0x7F` = App → Device, `0x80`–`0xFF` = Device → App.                       |
| 3      | Sequence     | 1    | A number `0x00`–`0xFF` that wraps around. The matching reply echoes this number.                                                    |
| 4      | Length       | 2    | Payload length `N`, little-endian. `0` to the negotiated/accepted message payload limit; never more than `236`.                     |
| 6      | Payload      | N    | The message body (§5, §6).                                                                                                          |
| 6+N    | CRC-16       | 2    | Checksum over Type, Sequence, Length, and Payload. CRC-16/CCITT-FALSE (polynomial `0x1021`, initial value `0xFFFF`), little-endian. |

Total message size = `8 + N` bytes.

**All multi-byte numbers are little-endian**, including the coefficient float
values in §6.2.

### 4.2 How the receiver reads a message

1. Scan the incoming bytes for the start marker `0x5A 0x5A`.
2. Read Type, Sequence, and Length.
3. Read `N` payload bytes, then the 2-byte checksum.
4. Check the checksum. If it passes, handle the message. If it fails, discard it
   and scan forward for the next start marker.
5. The start marker may appear inside a payload by chance. Use the Length field,
   not the marker, to find the end of a message. The marker is only used to
   recover after an error.

### 4.3 Replies and acknowledgements

- `CMD` messages (§6) do not have a generic application-level acknowledgement.
  BLE write success only means the write reached the BLE stack/module; command
  application is confirmed by later observable device state, such as telemetry,
  disconnect, or reset behaviour.
- Telemetry is sent by the device on its own and is **not** acknowledged. For
  unsolicited messages, the device increments its own Sequence value and wraps
  after `0xFF`.
- OTA uses explicit `OTA_STATUS` messages (§9). `OTA_STATUS` is treated as an
  unsolicited device message for sequence purposes: the device sends it with its
  own incrementing Sequence value (as for telemetry above), **not** the Sequence
  of the `OTA_BEGIN`/`OTA_CHUNK`/`OTA_COMMIT` it responds to. The app must
  correlate an `OTA_STATUS` to the transfer by its position in the update flow
  (§9.4) and its payload (for example the next-offset in an `ACK`), never by
  matching the Sequence field.

### 4.4 Message types

**App → Device** (written to RX)

| Type          | Name         | Purpose                             |
| ------------- | ------------ | ----------------------------------- |
| `0x01`        | `CMD`        | Settings and control commands (§6). |
| `0x10`–`0x13` | OTA messages | Firmware update (§9).               |

**Device → App** (sent on TX)

| Type   | Name         | Purpose                      |
| ------ | ------------ | ---------------------------- |
| `0x81` | `TELEMETRY`  | Live sensor reading (§5.1).  |
| `0x90` | `OTA_STATUS` | Firmware update status (§9). |

---

## 5. Reading Data from the Device (Device → App)

### 5.1 Telemetry message (`TELEMETRY`, type `0x81`)

The live sensor reading. Fixed **12-byte** payload, **20 bytes framed**, so it
fits in one MTU-23 notification.

| Field         | Type      | Length | Description                 |
| ------------- | --------- | ------ | --------------------------- |
| Mode          | `uint8_t` | 1      | Device state bitmap (§5.2). |
| Cavity temp   | `int16_t` | 2      | Cavity / air channel.       |
| Left temp     | `int16_t` | 2      | Left channel.               |
| Right temp    | `int16_t` | 2      | Right channel.              |
| Probe temp    | `int16_t` | 2      | External probe channel.     |
| Battery level | `uint8_t` | 1      | 0–100 (%).                  |
| Firmware ver. | `uint8_t` | 1      | Firmware version (§7.3).    |
| Error code    | `uint8_t` | 1      | Current error (§5.3).       |

Temperature encoding and error values are defined in §7.

_To confirm: how many of the four temperature channels the hardware actually
uses. Any unused channel reports its error value and clears its Mode bit._

### 5.2 Mode bitmap

`Mode` is an 8-bit value. Bit 0 is the least significant bit.

| Bit     | 7–5      | 4     | 3           | 2           | 1          | 0            |
| ------- | -------- | ----- | ----------- | ----------- | ---------- | ------------ |
| Meaning | Reserved | Units | Probe valid | Right valid | Left valid | Cavity valid |

- Bits 0–3 — channel valid: `1` = good reading, `0` = unavailable or faulted.
- Bit 4 — units: `0` = Fahrenheit, `1` = Celsius. Display preference only — indicates the unit shown on the device display; it does not affect wire encoding, which is always °C (§7.1).
- Bits 5–7 — reserved. Firmware sends `0`; app ignores on read.

Left and Right are defined from the user facing the front of the device. If a
channel is unavailable or faulted, clear its Mode bit and report its temperature
as the matching error value (§7.1). For the external probe, a probe that is not
connected clears bit 3. If the firmware cannot tell "not connected" apart from
"faulted", report error code `0x02` and temperature value `4000`.

### 5.3 Error codes

The device owns its error state. The current error code is included in every
telemetry message. There is no separate pushed error event. The state clears only
when the firmware reports `0x00`.

| Error        | Code          | Description                                                               |
| ------------ | ------------- | ------------------------------------------------------------------------- |
| No error     | `0x00`        | No errors.                                                                |
| Brownout     | `0x01`        | Supply voltage dropped below the threshold. Clears on recovery.           |
| Sensor fault | `0x02`        | One or more temperature sensors faulted. Clears when the sensor recovers. |
| Reserved     | `0x03`–`0xFF` | Future use. Treat as a generic error.                                     |

### 5.4 Telemetry streaming

Telemetry streams automatically while the device is connected — there is no
start/stop command:

- Once TX notifications are subscribed, the device begins sending one
  `TELEMETRY` message **every second (1 Hz)**.
- Streaming continues until the link drops, at which point it stops.
- While a firmware update is in progress (from `OTA_BEGIN` accepted until the
  device returns to idle — §9), the device **pauses**
  telemetry so the shared pipe is clear for OTA data, and **resumes** it once the
  update finishes, aborts, or fails.
- _To confirm: the device should disconnect after about 4 hours of continuous
  streaming to protect the battery._ (Length of time TBC)

---

## 6. Sending Commands to the Device (App → Device)

The app controls the device with `CMD` messages (type `0x01`). The payload is
`[sub-command][sub-payload]`. Commands are best-effort application messages:
there is no generic command reply. The only time we might want to acknowledge a response (outside of OTA) is during the multi-part coefficient update messages, but we will see if it works fine without responses first before adding complexity.

### 6.1 Command list

| Sub-command | Name                  | Sub-payload | Length | Description                        |
| ----------- | --------------------- | ----------- | ------ | ---------------------------------- |
| `0x01`      | `SET_UNITS`           | `uint8_t`   | 1      | `0x01` Celsius, `0x00` Fahrenheit. Sets the display unit preference only; telemetry values remain °C (§7.1). |
| `0x02`      | `POWER_OFF`           | —           | 0      | Power off the device.              |
| `0x03`      | `SET_COEFFICIENTS_AB` | see §6.2    | 10     | Coefficient update, part 1 of 3.   |
| `0x04`      | `FACTORY_RESET`       | —           | 0      | Clear preferences (§3.2).          |
| `0x05`      | `SET_COEFFICIENTS_CD` | see §6.2    | 10     | Coefficient update, part 2 of 3.   |
| `0x06`      | `SET_COEFFICIENTS_E`  | see §6.2    | 6      | Coefficient update, part 3 of 3.   |

### 6.2 Temperature correction (`SET_COEFFICIENTS_AB` / `SET_COEFFICIENTS_CD` / `SET_COEFFICIENTS_E`)

> **Note:** The three-part split exists to fit within the 23-byte default ATT MTU. If the app negotiates a higher MTU (e.g. MTU 247) before sending coefficients, all five coefficients plus Model ID and Channel could be sent in a single command, simplifying both the protocol and the firmware's partial-update bookkeeping. Firmware should confirm which approach — keeping the three-part split or consolidating to a single command under a negotiated MTU — is preferable before this is finalised.

The Cavity, Left, and Right readings can be corrected with a fourth-order
polynomial:

```
corrected = a·x⁴ + b·x³ + c·x² + d·x + e
```

The firmware applies this correction before sending telemetry. The default
values give no change: `a=0, b=0, c=0, d=1, e=0`. Each coefficient is a 32-bit
IEEE-754 single-precision float, little-endian.

Coefficient updates are split into three commands so each full framed command
fits in one BLE write at the default ATT MTU of 23:

- `SET_COEFFICIENTS_AB` (`0x03`) carries `a` and `b`.
- `SET_COEFFICIENTS_CD` (`0x05`) carries `c` and `d`.
- `SET_COEFFICIENTS_E` (`0x06`) carries `e`.

With 20 bytes available in one MTU-23 BLE write, the 8-byte Mathis frame
overhead and 1-byte command sub-command leave 11 bytes for the coefficient
sub-payload. The `AB` and `CD` payloads are 10 bytes each, and the `E` payload
is 6 bytes, so no coefficient command requires MTU negotiation or
framed-message fragmentation.

The firmware must apply coefficient updates atomically per channel. It must not
replace a channel's active coefficient set until all three parts for the same
Model ID and channel have been received and validated. Partial updates must be
rejected or discarded without changing the active coefficients.

**`SET_COEFFICIENTS_AB` sub-payload (10 bytes):**

| Field    | Type           | Length | Description                                                |
| -------- | -------------- | ------ | ---------------------------------------------------------- |
| Model ID | `uint8_t`      | 1      | Must equal the device Model ID (`0x04`); reject otherwise. |
| Channel  | `uint8_t`      | 1      | `0x00` Cavity, `0x01` Left, `0x02` Right.                  |
| a        | `float32` (LE) | 4      | Quartic term.                                              |
| b        | `float32` (LE) | 4      | Cubic term.                                                |

**`SET_COEFFICIENTS_CD` sub-payload (10 bytes):**

| Field    | Type           | Length | Description                                                |
| -------- | -------------- | ------ | ---------------------------------------------------------- |
| Model ID | `uint8_t`      | 1      | Must equal the device Model ID (`0x04`); reject otherwise. |
| Channel  | `uint8_t`      | 1      | `0x00` Cavity, `0x01` Left, `0x02` Right.                  |
| c        | `float32` (LE) | 4      | Quadratic term.                                            |
| d        | `float32` (LE) | 4      | Linear term.                                               |

**`SET_COEFFICIENTS_E` sub-payload (6 bytes):**

| Field    | Type           | Length | Description                                                |
| -------- | -------------- | ------ | ---------------------------------------------------------- |
| Model ID | `uint8_t`      | 1      | Must equal the device Model ID (`0x04`); reject otherwise. |
| Channel  | `uint8_t`      | 1      | `0x00` Cavity, `0x01` Left, `0x02` Right.                  |
| e        | `float32` (LE) | 4      | Constant term.                                             |

These commands do not require an MTU larger than the 23-byte default (§1.2) and
do not require one coefficient command to be split across multiple BLE writes.

---

## 7. Data Encoding

### 7.1 Temperature (2 bytes)

The device sends whole-number temperatures, **always in °C**, regardless of the
display unit. Negative values are valid.

|                      |                                                        |
| -------------------- | ------------------------------------------------------ |
| Type                 | `int16_t`, little-endian                               |
| Scale                | Whole numbers, no decimal places                       |
| Valid range          | `-50` to `550` (°C)                                    |
| Units                | Always °C. Mode bit 4 (§5.2) is display preference only |
| Error — too high     | `2000`                                                 |
| Error — too low      | `3000`                                                 |
| Error — sensor fault | `4000`                                                 |

The firmware applies any sensor correction before encoding the value. The app
must not apply calibration or offset math to telemetry values. Converting the
Celsius wire value for display in the user's preferred unit is expected, using
the display rounding rule below.

> **Note — display rounding rule:** when the device display is set to
> Fahrenheit, the device must derive its displayed value from the **same integer
> Celsius value it transmits**, using `F = round(C × 9 / 5 + 32)` (round half
> away from zero). The app applies the identical formula, so the app and the
> device display always show the same digits.

### 7.2 Battery level

`uint8_t`, 0–100 (%).

### 7.3 Firmware version

A single `uint8_t` value, `0`–`255`.

The value is a monotonically increasing firmware release number for Mathis. The
app uses it for diagnostics and update eligibility; it is not a semantic
major/minor version.

---

## 8. UUID Reference

The device uses one service. The service UUID is a custom 128-bit value. The two
characteristics under it use 16-bit UUIDs, which is what the BLE module's AT
command set supports (`AT+RXUUID` / `AT+TXUUID` take 16-bit values only;
`AT+SERVUUID` takes the 128-bit service UUID).

| Item                                     | UUID                                             |
| ---------------------------------------- | ------------------------------------------------ |
| Service                                  | `441C1000-776D-B95D-75A7-496DD1B5BEDA` (128-bit) |
| RX characteristic (App → Device, Write)  | `FFE1` (16-bit)                                  |
| TX characteristic (Device → App, Notify) | `FFE2` (16-bit)                                  |

Mathis is identified in advertising by this 128-bit service UUID (§2.1). Model
ID `0x04` is carried in the scan response as Service Data (§2.2), not
Manufacturer Data.

---

## 9. Firmware Updates (OTA)

Firmware updates use the same two characteristics as everything else. They are
framed messages, not a separate channel.

### 9.1 Storage model

The STM32 flash supports a **safe staged update**:

1. The new firmware image downloads into a separate staging area.
2. The bootloader checks the image.
3. The image is installed into the main slot only on the next reset.

Because the running firmware is never overwritten during download, a failed or
interrupted transfer must leave the existing firmware bootable.

Cryptographic image authenticity is a launch requirement. CRC-32/checksum values
are transfer-integrity checks only; they prove that bytes arrived as expected,
not that the image is genuine. The signing model, trust anchors, and verification
rules are defined in §9.2.

If transfer integrity or image authenticity validation fails, the device must
reject the staged image before install and continue running the previous valid
firmware. If installation fails after validation, the device must either roll
back automatically or remain on the previous valid image, depending on bootloader
capability.

### 9.2 Firmware signing and image authenticity

OTA image authenticity must not depend on BLE link security (§3). The app delivers
a **pre-signed release binary** produced by the Ooni CI/CD pipeline. The device
never accepts a trust anchor, private key, or detached signature over BLE.

#### 9.2.1 Signing algorithm and signed image format

- **Algorithm:** RSA-3072, PKCS#1 v1.5, SHA-256.
- **Signed artifact layout:** the OTA file is one contiguous byte stream:

  ```
  [application image][384-byte RSA-3072 signature]
  ```

  | Field             | Length    | Description                                                                   |
  | ----------------- | --------- | ----------------------------------------------------------------------------- |
  | Application image | Variable  | The firmware bytes that will be installed into the main slot.                 |
  | Signature         | 384 bytes | RSA-3072 PKCS#1 v1.5 signature over the SHA-256 hash of the application image |

- **Image size in `OTA_BEGIN`:** the `Image size` field is the total byte length
  of this signed artifact (application image + signature).
- **CRC-32 in `OTA_BEGIN` / `OTA_COMMIT`:** covers the full signed artifact for
  transfer-integrity checking only. It does not replace signature verification.
  Suggested algorithm: **CRC-32/ISO-HDLC** — polynomial `0x04C11DB7`, reflected input and
  output, initial value `0xFFFFFFFF`, final XOR `0xFFFFFFFF` (the variant used
  in Ethernet, ZIP, and PNG).
- **Install target:** after verification, the bootloader installs only the
  application image bytes into the main slot. The trailing 384-byte signature is
  verification metadata and must not be executed.

#### 9.2.2 Key custody and release workflow

| Key                | Holder                       | Use                                                                                  |
| ------------------ | ---------------------------- | ------------------------------------------------------------------------------------ |
| `PROD_PRIVATE_KEY` | Ooni (AWS KMS)               | Signs customer-release binaries in CI/CD. Never shared with the manufacturer or app. |
| `PROD_PUBLIC_KEY`  | Factory-programmed on device | Verifies production-signed images on customer hardware.                              |
| `DEV_PRIVATE_KEY`  | Manufacturer                 | Signs development and QA builds for manufacturer test units only.                    |
| `DEV_PUBLIC_KEY`   | Manufacturer test units only | Allows DEV-signed images to boot on non-shippable fused test hardware.               |

Release workflow:

1. The manufacturer develops firmware and signs test builds with `DEV_PRIVATE_KEY`.
2. When a build is ready for release, the manufacturer submits the candidate binary
   to Ooni for production signing (same submission process as GG3 WiFi/cloud OTA).
3. Ooni CI/CD produces the **PROD-signed binary** that the app distributes to users.
4. The app transfers that PROD-signed binary verbatim over BLE OTA. It must not
   re-sign, strip, or modify the artifact.

#### 9.2.3 Trust anchor storage (STM32)

Production units must store `PROD_PUBLIC_KEY` in **factory-programmed read-only
flash** (or an equivalent one-time-programmed store). Requirements:

- Programmed during manufacturing before shipment.
- Not writable from application firmware, BLE commands, or OTA.
- Survives factory reset (§3.2). Factory reset must not erase or replace firmware
  signing trust anchors.
- Customer production units must contain **only** `PROD_PUBLIC_KEY`. `DEV_PUBLIC_KEY`
  must be present only on manufacturer test units that are never shipped to customers.

The manufacturer must provide a **validation script** that confirms the programmed
`PROD_PUBLIC_KEY` matches the official production key before a unit leaves the line.
An incorrect trust anchor bricks secure-update capability.

#### 9.2.4 Verification timing

Verification happens in two stages:

1. **Application firmware (`VERIFYING` state, §9.6):** on `OTA_COMMIT`, verify
   transfer integrity (CRC-32), then verify the RSA-3072 signature over the staged
   signed artifact using the installed public key. Reply `VERIFY_OK` or
   `VERIFY_FAIL` before scheduling install.
2. **Bootloader (on reset):** before swapping the staged image into the main slot,
   verify the production signature again. If bootloader verification fails, the
   device must boot the previous valid firmware.

The staged image must not be installed unless both stages pass.

#### 9.2.5 Anti-rollback

To satisfy secure-update compliance requirements, the bootloader must refuse to
install or boot firmware with a security version lower than the highest version
already committed on the device. The exact storage mechanism (monotonic counter,
version field in image metadata, or option-byte SVN) is owned by the manufacturer
bootloader design, but the behaviour is required: once a security fix is committed,
older vulnerable images must not be installable.

#### 9.2.6 Manufacturer test hardware

| Class                     | Trust anchor configuration           | Purpose                                                                   |
| ------------------------- | ------------------------------------ | ------------------------------------------------------------------------- |
| Development kits          | No secure-boot enforcement required  | Fast iteration and local flashing.                                        |
| Manufacturer test units   | `PROD_PUBLIC_KEY` + `DEV_PUBLIC_KEY` | Full OTA and signature-path validation. **Must never ship to customers.** |
| Customer production units | `PROD_PUBLIC_KEY` only               | Accept only Ooni PROD-signed release binaries.                            |

### 9.3 OTA messages

**App → Device**

| Type   | Name         | Payload                 | Purpose                           |
| ------ | ------------ | ----------------------- | --------------------------------- |
| `0x10` | `OTA_BEGIN`  | see §9.4                | Start an update.                  |
| `0x11` | `OTA_CHUNK`  | `[offset: u32][data]`   | A piece of the firmware image.    |
| `0x12` | `OTA_COMMIT` | `[image checksum: u32]` | All pieces sent; check and apply. |
| `0x13` | `OTA_ABORT`  | —                       | Cancel the update in progress.    |

**Device → App**

| Type   | Name         | Purpose                   |
| ------ | ------------ | ------------------------- |
| `0x90` | `OTA_STATUS` | Status and errors (§9.5). |

All OTA multi-byte numbers are little-endian.

### 9.4 Update flow

1. **Begin.** The app sends `OTA_BEGIN` with:

   | Field                   | Type       | Length | Description                                                                                     |
   | ----------------------- | ---------- | ------ | ----------------------------------------------------------------------------------------------- |
   | Image size              | `uint32_t` | 4      | Total byte length of the signed artifact (application image + 384-byte signature).              |
   | Image checksum (CRC-32) | `uint32_t` | 4      | CRC-32/ISO-HDLC of the full signed artifact. Re-sent in `OTA_COMMIT` as confirmation.           |
   | Target firmware version | `uint8_t`  | 1      | Firmware version the app expects after a successful update. Used to verify the post-OTA result. |

2. **Ready.** The device runs pre-flight checks (battery, storage, not busy). On
   success it replies `OTA_STATUS / READY` with the **chunk size** it will
   accept. The app must size its chunks to this value. The device may choose a
   small chunk size so the flow works at ATT MTU 23, or a larger size when the
   negotiated MTU, UART flow control, and flash-write constraints allow it.

   The accepted chunk size is the number of **image data** bytes in one
   `OTA_CHUNK` (the `[data]` portion), not the framed message size. Because an
   `OTA_CHUNK` payload is `[offset: u32][data]` and the frame payload limit is
   `236` bytes (§4.1), the chunk data cannot exceed `236 − 4 = 232` bytes. The
   device **must not** advertise a chunk size above `232`, and the app clamps the
   accepted value to `232` defensively before sizing its chunks.

3. **Transfer.** The app sends `OTA_CHUNK` messages, each `[offset][data]`. The
   device replies `OTA_STATUS / ACK` with the next offset it expects. In v1 the
   device **must acknowledge every chunk**, and the app sends the next chunk only
   after receiving the `ACK` for the previous one (lockstep). Every `ACK` must
   report the next expected offset; the app uses that offset as authoritative, so
   an `ACK` that repeats the current offset is a re-request the app honours by
   resending from there. Chunk data must not exceed the accepted chunk size or
   the per-message payload limit.

   > Windowed acknowledgement (the device acknowledging once per fixed group of
   > chunks) is intentionally out of scope for v1: the device has no way to tell
   > the app the window size, so a windowing device and a lockstep app would
   > deadlock. If windowing is added later, the window size must be carried in the
   > `READY` payload alongside the chunk size so the app can adopt it.

   At the default ATT MTU of 23 bytes, each BLE write carries 20 bytes of
   characteristic data. The framed `OTA_CHUNK` message (8-byte frame overhead +
   4-byte offset + chunk data) spans multiple BLE writes for any chunk size above
   8 bytes. The app fragments large messages across sequential writes; the device
   must buffer incoming BLE writes and reassemble complete frames before
   processing.

   To reduce the write count the app should raise the ATT MTU before starting
   OTA. Only an **Android** central can request a specific MTU; an **iOS** central
   auto-negotiates the MTU at connect time and exposes only the negotiated
   result, so firmware must not expect an MTU-exchange request from every central.
   The transfer must work correctly at the default MTU 23 regardless.

4. **Commit.** The app sends `OTA_COMMIT` with the whole-image checksum — the
   same CRC-32/ISO-HDLC value sent in `OTA_BEGIN`. The device verifies this
   matches the value stored from `OTA_BEGIN`, checks transfer integrity against
   the accumulated data, and then verifies image authenticity. The staged image
   must not be installed unless both checks pass.

5. **Apply.** On success the device replies `OTA_STATUS / VERIFY_OK`, then
   `OTA_STATUS / APPLYING`, then reboots. After the reboot, the app reconnects
   and waits for the first valid `TELEMETRY` message to confirm the device is
   running the expected firmware version. If the version in that message does not
   match the target version from `OTA_BEGIN`, the install failed or was rolled
   back; the app must surface an error and offer to retry from step 1.

6. **Failure or cancel.** On any error, or on `OTA_ABORT`, the device discards
   the staged image and returns to idle with the previous valid firmware still
   bootable.

**Pre-flight rule:** reject an update when the device battery is below the safe
level (about 30%). The firmware owns this check using its local battery
measurement; the app-visible battery level is reported in telemetry.

**Telemetry during an update:** from the moment `OTA_BEGIN` is accepted until the
device returns to idle (update applied, aborted, or failed), the device **pauses**
the 1 Hz telemetry stream (§5.4) so the shared pipe carries only OTA traffic,
and **resumes** it afterward. The app should expect a gap in telemetry for the
duration of the update.

### 9.5 Status codes

`OTA_STATUS` (type `0x90`) payload is `[status code][optional data]`.

| Status             | Code   | Extra                         |
| ------------------ | ------ | ----------------------------- |
| Idle               | `0x00` | —                             |
| Ready              | `0x01` | `[accepted chunk size: u16]`  |
| Transferring       | `0x02` | —                             |
| Acknowledge        | `0x03` | `[next expected offset: u32]` |
| Verify OK          | `0x04` | —                             |
| Verify failed      | `0x05` | `[reason]`                    |
| Applying           | `0x06` | —                             |
| Aborted            | `0x07` | `[reason]`                    |
| Error — power      | `0x08` | —                             |
| Error — storage    | `0x09` | —                             |
| Error — busy       | `0x0A` | —                             |
| Error — bad offset | `0x0B` | —                             |

`Transferring` (`0x02`) is an optional progress signal. The device may send it
during chunk receipt; it is not required on every chunk. The app must not block
waiting for `Transferring` — it only blocks on the per-chunk `ACK` (`0x03`)
before sending the next chunk (§9.4 step 3).

**`VERIFY_FAIL` reason byte:**

| Reason | Meaning                               |
| ------ | ------------------------------------- |
| `0x01` | CRC-32 mismatch (transfer corruption) |
| `0x02` | Production signature invalid (§9.2)   |
| `0x03` | Image too large for staging area      |
| `0x04` | Transfer timeout (device-side)        |
| `0x05` | Anti-rollback rejection (§9.2.5)      |
| `0xFF` | Unknown / unclassified                |

### 9.6 Device states

| State        | Entered when                           | Behaviour                                                         | Leaves when                                        |
| ------------ | -------------------------------------- | ----------------------------------------------------------------- | -------------------------------------------------- |
| Idle         | power-on, abort, reboot                | rejects chunks                                                    | `OTA_BEGIN` accepted                               |
| Ready        | `OTA_BEGIN` passes pre-flight          | replies Ready with chunk size                                     | first chunk received, or 60 s with no chunk → Idle |
| Transferring | first chunk received                   | accepts chunks, sends a per-chunk `ACK` with the next offset       | `OTA_COMMIT`                                       |
| Verifying    | `OTA_COMMIT` received                  | checks transfer integrity and image authenticity                  | result ready                                       |
| Applying     | checks passed                          | schedules install on reset                                        | reboot                                             |
| Failed       | check, authenticity, or transfer error | reports error, discards staged image, keeps old firmware bootable | back to Idle                                       |

**Transfer timeout:** if no chunk is received for 60 seconds while in the
Transferring state, or `OTA_COMMIT` is not received within 60 seconds of the
last chunk, the device sends `OTA_STATUS / Aborted` (reason `0x04`) and returns
to Idle. The app must restart from `OTA_BEGIN`.

### 9.7 Throughput

Update speed is limited by the **UART link (115200 baud)** between the BLE module
and the STM32, not only by BLE itself. The accepted chunk size should be tuned
during bring-up against ATT MTU 23, larger negotiated MTUs, UART speed/flow
control, flash-write constraints, and buffer size. The values used at first are
starting points, not final figures. v1 acknowledges every chunk (§9.4 step 3); a
windowed acknowledgement scheme is a possible later optimisation, but only if the
window size is added to the `READY` payload so the app can adopt it.
