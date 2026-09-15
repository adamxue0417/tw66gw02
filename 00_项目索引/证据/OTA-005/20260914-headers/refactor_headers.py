"""One-time byte-preserving relocation of main module definitions."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[4] / '01_固件工程/main'


def header(name, guard, includes, body, note):
    p = ROOT / name
    assert not p.exists(), name
    text = ('#ifndef ' + guard + '\r\n#define ' + guard + '\r\n\r\n' + note + '\r\n' + includes + '\r\n').encode('ascii')
    p.write_bytes(text + body.strip(b'\r\n') + b'\r\n\r\n#endif\r\n')


def cut(data, first, last):
    start = data.index(first)
    end = data.index(last, start)
    return data[:start] + data[end:], data[start:end]


p = ROOT / 'COP/ota_update.c'
data = p.read_bytes()
data, public = cut(data, b'#define OTA_TYPE_BEGIN', b'#define OTA_TIMEOUT_TICKS_20MS')
data, private = cut(data, b'#define OTA_TIMEOUT_TICKS_20MS', b'static OtaSession s_ota;')
data = data.replace(b'#include "ota_update.h"', b'#include "ota_update.h"\r\n#include "ota_update_internal.h"', 1)
p.write_bytes(data)
p = ROOT / 'COP/ota_update.h'
data = p.read_bytes()
assert b'#include <stdint.h>' in data
data = data.replace(b'#include <stdint.h>', b'#include <stdint.h>\n\n/* Wire protocol values; shared by transport users. */\n' + public.rstrip(), 1)
p.write_bytes(data)
header('COP/ota_update_internal.h', 'MATHIS_OTA_UPDATE_INTERNAL_H', '#include <stdint.h>\r\n', private,
       '/* Private session layout and timing; include only from ota_update.c. */')

p = ROOT / 'BSP/wireless.c'
data, private = cut(p.read_bytes(), b'#define TX_QUEUE_DEPTH', b'static uint8_t s_rx_frame')
data = data.replace(b'#include "ota_update.h"', b'#include "ota_update.h"\r\n#include "wireless_internal.h"', 1)
p.write_bytes(data)
header('BSP/wireless_internal.h', 'MATHIS_WIRELESS_INTERNAL_H', '#include <stdint.h>\r\n#include "main_control.h"\r\n', private,
       '/* Private queue/assembly types and timing; include only from wireless.c. */')

p = ROOT / 'BSP/temp.c'
data, config = cut(p.read_bytes(), b'#define PROBE_TEMP_C_LOW_LIMIT', b'static uint16_t s_battery_adc_history')
p.write_bytes(data)
header('BSP/temp_config.h', 'MATHIS_TEMP_CONFIG_H', '', config,
       '/* Existing acquisition thresholds, in the current measurement domain.\r\n * Relocation only: do not reinterpret these as confirmed pack voltages. */')
p = ROOT / 'BSP/temp.h'
data = p.read_bytes().replace(b'#include "config.h"', b'#include "config.h"\r\n#include "temp_config.h"', 1)
# Remove an exact duplicate declaration while retaining the existing API.
needle = b'void TempGetTask(void);\r\n'
assert data.count(needle) == 2
where = data.rindex(needle)
p.write_bytes(data[:where] + data[where + len(needle):])

p = ROOT / 'COP/main_control.c'
data, macros = cut(p.read_bytes(), b'#define IDLE_AUTO_SHUTDOWN_TICKS_100MS', b'static int16_t s_display_temp')
data, record = cut(data, b'typedef struct {', b'static uint32_t s_config_sequence')
data = data.replace(b'#include "ota_layout.h"', b'#include "ota_layout.h"\r\n#include "main_control_internal.h"', 1)
p.write_bytes(data)
header('COP/main_control_internal.h', 'MATHIS_MAIN_CONTROL_INTERNAL_H',
       '#include <stdint.h>\r\n#include "main_control.h"\r\n#include "ota_layout.h"\r\n', macros + record,
       '/* Private persistence ABI and idle timing; include only from main_control.c. */')

p = ROOT / 'Bootloader/boot_main.c'
data, private = cut(p.read_bytes(), b'#define BOOT_ERROR_MASK', b'/* PB3 drives')
data = data.replace(b'#include "ota_layout.h"', b'#include "ota_layout.h"\r\n#include "boot_internal.h"', 1)
p.write_bytes(data)
header('Bootloader/boot_internal.h', 'MATHIS_BOOT_INTERNAL_H',
       '#include "stm32f030x8.h"\r\n#include "ota_layout.h"\r\n', private,
       '/* Bootloader-only flash/log/trace definitions, not an application API. */')

# Preserve the legacy GBK comments and CRLF in the scheduler files.
p = ROOT / 'OS/TaskScheduler.c'
data = p.read_bytes()
for name in (b'RETURN_ERROR', b'RETURN_NORMAL', b'SCH_REPORT_ERRORS'):
    data, n = re.subn(rb'^#define ' + name + rb'[^\r\n]*\r?\n', b'', data, flags=re.M)
    assert n == 1
data = data.replace(b'RETURN_ERROR', b'SCH_RETURN_ERROR').replace(b'RETURN_NORMAL', b'SCH_RETURN_NORMAL')
p.write_bytes(data)
p = ROOT / 'OS/TaskScheduler.h'
data = p.read_bytes().replace(b'#include "stdint.h"\r\n', b'#include "stdint.h"\r\n\r\n/* SCH_Delete_Task return values; error reporting uses #ifdef (presence). */\r\n#define SCH_RETURN_ERROR  (1u)\r\n#define SCH_RETURN_NORMAL (0u)\r\n#define SCH_REPORT_ERRORS 1\r\n', 1)
p.write_bytes(data)
print('Relocated definitions in six active modules; implementation data remain in C.')
