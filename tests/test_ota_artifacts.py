import hashlib
import json
import pathlib
import struct
import zlib


ROOT = pathlib.Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "OTA_Artifacts"
PAGE = 0x400
SLOT = 0x6400
APP_MAX = 0x6280
APP_BASE = 0x08002000
BOOT_API_OFFSET = 0x1FC0


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
            continue
        completed += 1
    return main, stage


def main():
    boot_source = (ROOT / "Bootloader" / "boot_main.c").read_text(encoding="utf-8")
    update_source = (ROOT / "COP" / "ota_update.c").read_text(encoding="utf-8")
    assert "BootSecurity_VerifyArtifact(OTA_STAGE_BASE" in boot_source
    assert "SecurityAdvance(header->target_version)" in boot_source
    assert "api->verify_artifact(OTA_STAGE_BASE" in update_source
    assert "OTA_REASON_ROLLBACK" in update_source

    boot = (ARTIFACTS / "mathis_secure_bootloader_dev_v1.bin").read_bytes()
    app102 = (ARTIFACTS / "mathis_secure_app_v102.bin").read_bytes()
    app103 = (ARTIFACTS / "mathis_app_v103.bin").read_bytes()
    artifact = (ARTIFACTS / "mathis_ota_v103_dev_signed.ota").read_bytes()
    factory = (ARTIFACTS / "mathis_secure_factory_v102_dev.bin").read_bytes()
    manifest = json.loads(
        (ARTIFACTS / "mathis_ota_v103_dev_signed.manifest.json").read_text(encoding="utf-8-sig")
    )

    assert len(boot) <= 0x2000
    assert struct.unpack_from("<I", boot, BOOT_API_OFFSET)[0] == 0x4950414D
    assert len(app102) <= APP_MAX and len(app103) <= APP_MAX
    for app in (app102, app103):
        stack, reset = struct.unpack_from("<II", app)
        assert 0x200000C0 <= stack <= 0x20001FF0
        assert APP_BASE <= (reset & ~1) < APP_BASE + len(app)

    assert len(factory) == 0x2000 + len(app102)
    assert factory[: len(boot)] == boot
    assert factory[len(boot) : 0x2000] == bytes([0xFF]) * (0x2000 - len(boot))
    assert factory[0x2000:] == app102

    assert artifact[:-384] == app103
    assert artifact[-384:] != bytes(384)
    assert manifest["format"] == "mathis-signed-ota-v1"
    assert manifest["target_version"] == manifest["security_version"] == 103
    assert manifest["application_size"] == len(app103)
    assert manifest["artifact_size"] == len(artifact)
    assert manifest["crc32_iso_hdlc"] == f"0x{zlib.crc32(artifact) & 0xFFFFFFFF:08X}"
    assert manifest["application_sha256"] == hashlib.sha256(app103).hexdigest().upper()
    assert manifest["artifact_sha256"] == hashlib.sha256(artifact).hexdigest().upper()
    assert manifest["development_signature_placeholder"] is False
    assert manifest["signature_algorithm"] == "RSA-3072-PKCS1-v1_5-SHA256"

    factory_hex = parse_hex(ARTIFACTS / "mathis_secure_factory_v102_dev.hex")
    assert min(factory_hex) == 0x08000000
    assert max(factory_hex) < 0x08008400
    assert any(address < APP_BASE for address in factory_hex)
    assert any(APP_BASE <= address < 0x08008400 for address in factory_hex)

    old_slot = bytearray(app102 + bytes([0xFF]) * (SLOT - len(app102)))
    new_stage = bytearray(artifact + bytes([0xFF]) * (SLOT - len(artifact)))
    expected_new = bytearray(app103 + bytes([0xFF]) * (SLOT - len(app103)))
    total_phases = (SLOT // PAGE) * 3
    for cut in range(1, total_phases + 1):
        installed, backup = run_swap(bytearray(old_slot), bytearray(new_stage), len(app103), True, cut)
        assert installed == expected_new, ("forward", cut)
        assert backup == old_slot, ("backup", cut)
        restored, failed_new = run_swap(installed, backup, SLOT, False, cut)
        assert restored == old_slot, ("rollback", cut)
        assert failed_new == expected_new, ("rollback-backup", cut)

    print("secure OTA layout, CRC, manifest, and 150 interrupted swap/rollback cases passed")


if __name__ == "__main__":
    main()
