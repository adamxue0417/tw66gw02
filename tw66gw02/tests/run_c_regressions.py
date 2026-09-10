"""Compile production C with ARM Compiler 5 and execute it on the Cortex-M0 model.

Peripheral/flash calls are mocked; this is not an STM32 peripheral or power-loss test.
No debug probe or hardware is used. Requires the Keil VHT Cortex-M0 model.
"""
import argparse
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]


def run(command, timeout=60):
    result = subprocess.run([str(x) for x in command], capture_output=True, timeout=timeout)
    text = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    if result.returncode:
        raise RuntimeError(f"{command[0]} exited {result.returncode}\n{text}")
    return text


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--keil-root", type=pathlib.Path, default=pathlib.Path("C:/Keil_v5"))
    parser.add_argument("--engine", choices=("unicorn", "vht"), default="unicorn")
    args = parser.parse_args()
    compiler = args.keil_root / "ARM/ARM_Compiler_5.06u7/Bin"
    output = ROOT / "tests/build"
    output.mkdir(parents=True, exist_ok=True)
    incs = [ROOT / name for name in (
        "Core/Inc", "BSP", "COP", "OS", "Drivers/STM32F0xx_HAL_Driver/Inc",
        "Drivers/CMSIS/Include", "Drivers/CMSIS/Device/ST/STM32F0xx/Include")]
    options = ["-c", "--cpu", "Cortex-M0", "--c99", "-O1", "--split_sections", "--debug",
               "-DSTM32F030x8", "-DUSE_HAL_DRIVER"]
    for include in incs:
        options += ["-I", include]
    objects = [output / "startup.o"]
    run([compiler / "armasm.exe", "--cpu", "Cortex-M0", "--pd", "__MICROLIB SETA 0",
         "-o", objects[0], ROOT / "MDK-ARM/startup_stm32f030x8.s"])
    diagnostics = []
    for source in sorted((ROOT / "tests/simulator").glob("*.c")) + [ROOT / "BSP/mathis_util.c", ROOT / "BSP/key.c"]:
        obj = output / (source.stem + ".o")
        diagnostics.append(run([compiler / "armcc.exe", *options, "-o", obj, source]))
        objects.append(obj)
    # Each public header must compile without relying on config.h include order.
    headers = ["temp.h", "bh66f5242.h", "key.h", "wireless.h", "main_control.h", "mathis_util.h",
               "system_types.h", "TaskScheduler.h", "usart.h", "C8721.h", "warning.h", "pid.h", "gagent_md5.h"]
    for header in headers:
        source = output / "header_check.c"
        source.write_text(f'#include "{header}"\n', encoding="ascii")
        diagnostics.append(run([compiler / "armcc.exe", *options, "-o", output / "header_check.o", source]))
    (output / "compile.log").write_text("".join(diagnostics), encoding="utf-8")
    if any("warning:" in text.lower() for text in diagnostics):
        raise RuntimeError("C regression build emitted warnings; see tests/build/compile.log")
    executable = output / "regression.axf"
    run([compiler / "armlink.exe", "--cpu", "Cortex-M0", "--strict", "--scatter",
         ROOT / "tests/simulator/test.sct", "-o", executable, *objects])
    if args.engine == "unicorn":
        deps = ROOT.parent / "outputs/optimization/deps"
        if deps.exists():
            sys.path.insert(0, str(deps))
        sys.path.insert(0, str(ROOT / "tests/simulator"))
        from unicorn_runner import execute
        text = execute(executable)
        (output / "results.log").write_text(text, encoding="utf-8")
        print(text.strip())
        print(f"{len(headers)} public headers compiled independently.")
        return
    simulator = args.keil_root / "ARM/VHT/VHT_MPS2_Cortex-M0.exe"
    command = [simulator, "-a", executable, "--timelimit", "30", "-C",
               "fvp_mps2.mps2_visualisation.disable-visualisation=1"]
    for port in range(3):
        command += ["-C", f"fvp_mps2.telnetterminal{port}.start_telnet=0"]
    text = run(command, timeout=45)
    (output / "results.log").write_text(text, encoding="utf-8")
    if "PASS:" not in text or "FAIL " in text:
        raise RuntimeError(f"C tests did not finish successfully:\n{text}")
    print(text.strip())
    print(f"{len(headers)} public headers compiled independently.")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.TimeoutExpired) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
