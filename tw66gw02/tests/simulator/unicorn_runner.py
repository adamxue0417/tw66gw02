"""Minimal ARM ELF + semihosting runner for the isolated C regression image.

Only test-image RAM, Cortex-M system registers and stdout semihosting are exposed.
This intentionally does not model STM32 peripherals or provide host file access.
"""
import pathlib
import struct


def execute(path, instruction_limit=10_000_000):
    import unicorn
    from unicorn import arm_const as arm

    image = pathlib.Path(path).read_bytes()
    if image[:6] != b"\x7fELF\x01\x01":
        raise ValueError("Expected a little-endian ELF32 image")
    uc = unicorn.Uc(unicorn.UC_ARCH_ARM, unicorn.UC_MODE_THUMB | unicorn.UC_MODE_MCLASS)
    uc.ctl_set_cpu_model(arm.UC_CPU_ARM_CORTEX_M0)
    uc.mem_map(0, 0x100000)
    uc.mem_map(0x20000000, 0x100000)
    uc.mem_map(0xE0000000, 0x100000)
    phoff = struct.unpack_from("<I", image, 28)[0]
    phsize, phcount = struct.unpack_from("<HH", image, 42)
    for i in range(phcount):
        kind, offset, virtual, physical, size, _, _, _ = struct.unpack_from("<8I", image, phoff + i * phsize)
        if kind == 1 and size:
            uc.mem_write(physical, image[offset:offset + size])
    stack, reset = struct.unpack("<II", uc.mem_read(0, 8))
    uc.reg_write(arm.UC_ARM_REG_MSP, stack)
    uc.reg_write(arm.UC_ARM_REG_SP, stack)
    uc.reg_write(arm.UC_ARM_REG_XPSR, 0x01000000)
    output = []
    completed = False
    exit_code = 0

    def words(address, count):
        return struct.unpack("<" + "I" * count, uc.mem_read(address, count * 4))

    def trap(machine, number, _):
        nonlocal completed, exit_code
        pc = machine.reg_read(arm.UC_ARM_REG_PC)
        if bytes(machine.mem_read(pc, 2)) != b"\xab\xbe":
            raise RuntimeError(f"Unexpected exception {number} at 0x{pc:08x}")
        op = machine.reg_read(arm.UC_ARM_REG_R0)
        argument = machine.reg_read(arm.UC_ARM_REG_R1)
        result = 0
        if op == 1:  # SYS_OPEN: only the semihosting terminal
            name, mode, length = words(argument, 3)
            if bytes(machine.mem_read(name, length)) != b":tt":
                raise RuntimeError("Host file access is disabled")
            result = 1 if mode < 4 else (2 if mode < 8 else 3)
        elif op == 3:
            output.append(bytes(machine.mem_read(argument, 1)).decode("ascii", errors="replace"))
        elif op == 4:
            data = bytearray()
            while len(data) < 16384:
                value = bytes(machine.mem_read(argument + len(data), 1))[0]
                if not value:
                    break
                data.append(value)
            output.append(data.decode("utf-8", errors="replace"))
        elif op == 5:
            handle, pointer, length = words(argument, 3)
            if handle not in (1, 2, 3) or length > 65536:
                raise RuntimeError("Invalid semihosting write")
            output.append(bytes(machine.mem_read(pointer, length)).decode("utf-8", errors="replace"))
        elif op in (2, 10, 12, 19):
            pass
        elif op == 9:
            result = 1
        elif op == 0x15:
            pointer, capacity = words(argument, 2)
            if capacity:
                machine.mem_write(pointer, b"\0")
            machine.mem_write(argument + 4, struct.pack("<I", 0))
        elif op == 0x16:
            machine.mem_write(argument, struct.pack("<4I", 0x200C0000, 0x200D0000, 0x200FF000, 0x200E0000))
        elif op in (0x18, 0x20):
            if op == 0x20:
                reason, exit_code = words(argument, 2)
            else:
                reason = argument
            if reason != 0x20026:
                raise RuntimeError(f"Abnormal semihosting exit: 0x{reason:x}")
            completed = True
            machine.emu_stop()
        else:
            raise RuntimeError(f"Unsupported semihosting operation 0x{op:x}")
        machine.reg_write(arm.UC_ARM_REG_R0, result)
        machine.reg_write(arm.UC_ARM_REG_PC, (pc + 2) | 1)

    uc.hook_add(unicorn.UC_HOOK_INTR, trap)
    try:
        uc.emu_start(reset, 0, timeout=30_000_000, count=instruction_limit)
    except unicorn.UcError as error:
        pc = uc.reg_read(arm.UC_ARM_REG_PC)
        raise RuntimeError(f"Emulator error at 0x{pc:08x}: {error}; output={''.join(output)}") from error
    text = "".join(output)
    if not completed or exit_code or "FAIL " in text:
        raise RuntimeError(f"C regression did not pass (completed={completed}, exit={exit_code}):\n{text}")
    return text
