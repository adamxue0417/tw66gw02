"""Compile real CMSIS/HAL scheduler objects and inspect Sleep code generation."""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--keil-root', type=Path, default=Path('C:/Keil_v5'))
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    compiler = args.keil_root / 'ARM/ARM_Compiler_5.06u7/Bin'
    includes = ['OS', 'Core/Inc', 'Drivers/CMSIS/Include',
                'Drivers/CMSIS/Device/ST/STM32F0xx/Include', 'Drivers/STM32F0xx_HAL_Driver/Inc']
    flags = ['--cpu', 'Cortex-M0', '--c99', '-O2', '--split_sections',
             '-DSTM32F030x8', '-DUSE_HAL_DRIVER']
    for include in includes:
        flags += ['-I', str(ROOT / include)]
    def run(command, expected_success=True):
        result = subprocess.run([str(x) for x in command], capture_output=True, timeout=60)
        text = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        if expected_success:
            assert result.returncode == 0 and 'warning:' not in text.lower(), text
        else:
            assert result.returncode != 0 and 'SCH_IDLE_SLEEP_ENABLED_must_be_0_or_1' in text, text
        return text
    header = out / 'header_check.c'
    header.write_text('#include "TaskScheduler.h"\n#include "TaskScheduler_config.h"\n'
                      '#include "TaskScheduler.h"\n#include "TaskScheduler_config.h"\n', encoding='ascii')
    report = {'result': 'PASS', 'modes': {}}
    for enabled in (0, 1):
        define = f'-DSCH_IDLE_SLEEP_ENABLED={enabled}'
        run([compiler / 'armcc.exe', '-c', *flags, define, '-o', out / 'header.o', header])
        obj = out / f'scheduler-{enabled}.o'
        run([compiler / 'armcc.exe', '-c', *flags, define, '-o', obj, ROOT / 'OS/TaskScheduler.c'])
        asm = run([compiler / 'fromelf.exe', '--text', '-c', obj])
        (out / f'scheduler-{enabled}.asm.txt').write_text(asm, encoding='utf-8')
        wfi = len(re.findall(r'\bWFI\b', asm, re.IGNORECASE))
        assert wfi == enabled, (enabled, wfi)
        if enabled:
            for instruction in ('CPSID', 'DSB', 'MSR', 'ISB'):
                assert instruction in asm.upper(), instruction
        report['modes'][str(enabled)] = {'header_check': 'PASS', 'compile': 'PASS', 'wfi_count': wfi}
    rejection = run([compiler / 'armcc.exe', '-c', *flags, '-DSCH_IDLE_SLEEP_ENABLED=2',
                     '-o', out / 'invalid.o', header], expected_success=False)
    (out / 'invalid-config.log').write_text(rejection, encoding='utf-8')
    report['invalid_define_rejected'] = True
    (out / 'result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('PASS: real HAL/CMSIS compile, repeated headers, WFI OFF=0/ON=1, invalid define rejected')

if __name__ == '__main__':
    main()
