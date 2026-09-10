import json
import hashlib
import pathlib
import struct
import zlib


ROOT = pathlib.Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "OTA_Artifacts"
PAGE = 0x400
SLOT = 0x6C00
APP_MAX = 0x6A80
APP_BASE = 0x08001800


def parse_hex(path):
    data = {}
    upper = 0
    for number, line in enumerate(path.read_text().splitlines(), 1):
        record = bytes.fromhex(line[1:])
        assert sum(record) & 0xFF == 0, (path, number, "checksum")
        count = record[0]
        address = int.from_bytes(record[1:3], "big")
        kind = record[3]
        payload = record[4 : 4 + count]
        if kind == 0:
            for offset, value in enumerate(payload):
                absolute = upper + address + offset
                assert absolute not in data or data[absolute] == value
                data[absolute] = value
        elif kind == 4:
            upper = int.from_bytes(payload, "big") << 16
    return data


def run_swap(main, stage, incoming_size, trim, interrupted_after=None):
    """Model the bootloader's journaled three-phase page swap."""
    scratch = bytearray(PAGE)
    completed = 0
    interruptions = 0
    while completed < (SLOT // PAGE) * 3:
        page, phase = divmod(completed, 3)
        start = page * PAGE
        end = start + PAGE
        if phase == 0:
            scratch[:] = main[start:end]
        elif phase == 1:
            valid = PAGE
            if trim and start >= incoming_size:
                valid = 0
            elif trim and end > incoming_size:
                valid = incoming_size - start
            main[start:end] = stage[start : start + valid] + bytes([0xFF]) * (PAGE - valid)
        else:
            stage[start:end] = scratch
        interruptions += 1
        if interrupted_after == interruptions:
            # Power failed before journal append: repeat the same phase.
            continue
        completed += 1
    return main, stage


def main():
    boot_source = (ROOT / "Bootloader" / "boot_main.c").read_text(encoding="utf-8")
    assert "ImageVectorValid(OTA_APP_BASE, OTA_MAX_APPLICATION_SIZE)" in boot_source
    assert "ImageVectorValid(OTA_APP_BASE, OTA_APP_SLOT_SIZE)" not in boot_source
    app100 = (ARTIFACTS / "mathis_app_v100.bin").read_bytes()
    app101 = (ARTIFACTS / "mathis_app_v101.bin").read_bytes()
    artifact = (ARTIFACTS / "mathis_ota_v101.ota").read_bytes()
    boot = (ROOT / "Bootloader" / "build" / "mathis_bootloader.bin").read_bytes()
    factory_bin = (ARTIFACTS / "mathis_factory_v100.bin").read_bytes()
    manifest = json.loads((ARTIFACTS / "mathis_ota_v101.manifest.json").read_text(encoding="utf-8-sig"))

    assert len(app100) <= APP_MAX and len(app101) <= APP_MAX
    assert app100 != app101
    stack, reset = struct.unpack_from("<II", app101)
    assert 0x200000C0 <= stack <= 0x20001FF0
    assert APP_BASE <= (reset & ~1) < APP_BASE + len(app101)
    assert len(artifact) == len(app101) + 384
    assert artifact[:-384] == app101 and artifact[-384:] == bytes(384)
    assert manifest["artifact_size"] == len(artifact)
    assert manifest["crc32_iso_hdlc"] == f"0x{zlib.crc32(artifact) & 0xFFFFFFFF:08X}"
    assert manifest["sha256"] == hashlib.sha256(artifact).hexdigest()

    boot_stack, boot_reset = struct.unpack_from("<II", boot)
    assert 0x20000000 < boot_stack <= 0x20002000
    assert 0x08000000 <= (boot_reset & ~1) < 0x08000000 + len(boot)
    assert len(factory_bin) == 0x1800 + len(app100)
    assert factory_bin[: len(boot)] == boot
    assert factory_bin[len(boot) : 0x1800] == bytes([0xFF]) * (0x1800 - len(boot))
    assert factory_bin[0x1800:] == app100
    keil_factory_path = ARTIFACTS / "keil_factory_v100.bin"
    if keil_factory_path.exists():
        keil_factory_bin = keil_factory_path.read_bytes()
        # fromelf --bincombined pads the unused tail of the fixed 6 KiB boot
        # execution region with 0x00, while BuildOtaArtifacts leaves erased
        # flash as 0xFF. Both representations program equivalent content.
        assert keil_factory_bin[: len(boot)] == boot
        assert set(keil_factory_bin[len(boot) : 0x1800]) <= {0x00, 0xFF}
        assert keil_factory_bin[0x1800:] == app100

    direct_path = ARTIFACTS / "diagnostic_direct_v100.bin"
    min_boot_path = ROOT / "Bootloader" / "build_min" / "mathis_min_bootloader.bin"
    min_factory_path = ARTIFACTS / "diagnostic_minboot_factory_v100.bin"
    gpio_path = ARTIFACTS / "diagnostic_gpio_alive.bin"
    if all(path.exists() for path in (direct_path, min_boot_path, min_factory_path, gpio_path)):
        direct = direct_path.read_bytes()
        min_boot = min_boot_path.read_bytes()
        min_factory = min_factory_path.read_bytes()
        gpio_alive = gpio_path.read_bytes()
        direct_stack, direct_reset = struct.unpack_from("<II", direct)
        assert 0x20000000 < direct_stack <= 0x20001FF0
        assert 0x08000000 <= (direct_reset & ~1) < 0x08000000 + len(direct)
        min_stack, min_reset = struct.unpack_from("<II", min_boot)
        assert 0x20000000 < min_stack <= 0x20002000
        assert 0x08000000 <= (min_reset & ~1) < 0x08000000 + len(min_boot)
        assert min_factory[: len(min_boot)] == min_boot
        assert min_factory[len(min_boot) : 0x1800] == bytes([0xFF]) * (0x1800 - len(min_boot))
        assert min_factory[0x1800:] == app100
        gpio_stack, gpio_reset = struct.unpack_from("<II", gpio_alive)
        assert 0x20000000 < gpio_stack <= 0x20002000
        assert 0x08000000 <= (gpio_reset & ~1) < 0x08000000 + len(gpio_alive)

    factory = parse_hex(ARTIFACTS / "mathis_factory_v100.hex")
    assert min(factory) == 0x08000000
    assert max(factory) < 0x08008400
    assert any(address < APP_BASE for address in factory)
    assert any(APP_BASE <= address < 0x08008400 for address in factory)

    old_slot = bytearray(app100 + bytes([0xFF]) * (SLOT - len(app100)))
    new_stage = bytearray(artifact + bytes([0xFF]) * (SLOT - len(artifact)))
    expected_new = bytearray(app101 + bytes([0xFF]) * (SLOT - len(app101)))
    total_phases = (SLOT // PAGE) * 3
    for cut in range(1, total_phases + 1):
        installed, backup = run_swap(bytearray(old_slot), bytearray(new_stage), len(app101), True, cut)
        assert installed == expected_new, ("forward", cut)
        assert backup == old_slot, ("backup", cut)
        restored, failed_new = run_swap(installed, backup, SLOT, False, cut)
        assert restored == old_slot, ("rollback", cut)
        assert failed_new == expected_new, ("rollback-backup", cut)

    print("OTA artifact, memory map, CRC, and 162 interrupted swap/rollback cases passed")


if __name__ == "__main__":
    main()
