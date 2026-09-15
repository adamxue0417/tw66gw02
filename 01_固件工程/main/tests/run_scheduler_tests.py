"""Run the actual scheduler C on Cortex-M0 with mocked HAL/IRQ/WFI.

Requires ARM Compiler 5 and the Python unicorn package (optional --deps path).
These tests do not model STM32 peripherals, interrupt latency or power usage.
"""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def run(command):
    result = subprocess.run([str(x) for x in command], capture_output=True, timeout=60)
    text = (result.stdout + result.stderr).decode('utf-8', errors='replace')
    if result.returncode or 'warning:' in text.lower():
        raise RuntimeError(f'{command[0]}: exit {result.returncode}\n{text}')
    return text

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--keil-root', type=Path, default=Path('C:/Keil_v5'))
    parser.add_argument('--deps', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.deps:
        sys.path.insert(0, str(args.deps.resolve()))
    sys.path.insert(0, str(ROOT / 'tests/scheduler'))
    from unicorn_runner import execute
    compiler = args.keil_root / 'ARM/ARM_Compiler_5.06u7/Bin'
    for enabled in (0, 1):
        output = args.output.resolve() / f'sleep-{enabled}'
        output.mkdir(parents=True, exist_ok=True)
        run([compiler / 'armasm.exe', '--cpu', 'Cortex-M0', '--pd', '__MICROLIB SETA 0',
             '-o', output / 'startup.o', ROOT / 'MDK-ARM/startup_stm32f030x8.s'])
        objects = [output / 'startup.o']
        for source in (ROOT / 'OS/TaskScheduler.c', ROOT / 'tests/scheduler/test_scheduler.c'):
            obj = output / (source.stem + '.o')
            run([compiler / 'armcc.exe', '-c', '--cpu', 'Cortex-M0', '--c99', '-O2',
                 '--split_sections', f'-DSCH_IDLE_SLEEP_ENABLED={enabled}',
                 '-I', ROOT / 'tests/scheduler', '-I', ROOT / 'OS', '-o', obj, source])
            objects.append(obj)
        executable = output / 'scheduler.axf'
        run([compiler / 'armlink.exe', '--cpu', 'Cortex-M0', '--strict', '--scatter',
             ROOT / 'tests/scheduler/test.sct', '-o', executable, *objects])
        text = execute(executable)
        if 'PASS:' not in text:
            raise RuntimeError(text)
        (output / 'results.log').write_text(text, encoding='utf-8')
        print(text.strip())

if __name__ == '__main__':
    main()
