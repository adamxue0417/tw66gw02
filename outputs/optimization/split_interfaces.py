exec(open('outputs/optimization/edit_firmware.py',encoding='utf-8').read().split("s=read('Core/Src/main.c')")[0])
s=read('BSP/mathis_util.h')
start=s.index('/* Pure helpers')
body=s[start:s.rfind('#endif')].replace('static __inline ','')
prototypes=[]
for m in re.finditer(r'(?m)^(?:uint\d+_t) [\w]+\([^;]*?\)\n\{',body):
    prototypes.append(m.group(0)[:-2]+';')
write('BSP/mathis_util.c','#include "mathis_util.h"\n#include "ota_layout.h"\n#include <string.h>\n\n'+body)
write('BSP/mathis_util.h','#ifndef MATHIS_UTIL_H\n#define MATHIS_UTIL_H\n\n#include <stdint.h>\n\n/* CRC, finite-value and image-boundary helpers. Implementation: mathis_util.c. */\n'+'\n\n'.join(prototypes)+'\n#endif\n')
s=read('BSP/config.h')
type_start=s.index('typedef enum \n')
type_end=s.index('extern ErrMessage SystemErrMessage;')+len('extern ErrMessage SystemErrMessage;')
types=s[type_start:type_end]
types=re.sub(r'(?m)^extern .*\n?','',types)
write('BSP/system_types.h','#ifndef MATHIS_SYSTEM_TYPES_H\n#define MATHIS_SYSTEM_TYPES_H\n#include <stdint.h>\n\n/* Shared application data layout; no peripheral or module includes. */\n'+types+'\n#endif\n')
s=s[:type_start]+s[type_end:]
s=s.replace('#include <string.h>','#include "system_types.h"\n#include <string.h>').replace('#include "stdio.h"\n','')
write('BSP/config.h',s)
for folder in ['BSP','COP']:
    for p in (root/folder).glob('*.h'):
        if p.name in ['config.h','bh66f5242.h']: continue
        text=read(str(p.relative_to(root)))
        if '#include "config.h"' in text:
            text=text.replace('#include "config.h"','#include <stdint.h>')
            write(str(p.relative_to(root)),text)
s=read('Core/Inc/usart.h').replace('#include "config.h"','#include <stdint.h>')
write('Core/Inc/usart.h',s)
s=read('COP/main_control.h').replace('#include <stdint.h>','#include <stdint.h>\n#include "system_types.h"\n\nextern _work_process work_process, work_process_backups;\nextern _system_data system_data;\nextern volatile uint8_t g_probe_connected, g_probe_over_hi, g_probe_over_lo;')
write('COP/main_control.h',s)
s=read('BSP/temp.h').replace('#include <stdint.h>','#include <stdint.h>\n#include "system_types.h"\n\nextern ErrMessage SystemErrMessage;\nextern volatile uint8_t g_battery_low;\nextern volatile uint16_t g_battery_mv;')
s=s.replace('void TempGetTask(void);','''void channel_env_init(void);
void Get_Filter_ADC12bitResult(void);
float PT1000_CalculateTemperature(uint16_t resistance);
int16_t Temp_Get(uint8_t channel);
uint16_t Average_Temp(uint16_t values[], uint8_t length);
void Add_Temp(uint16_t values[], uint8_t length, uint16_t sample);
uint16_t Temp_Handle(uint16_t values[], uint8_t length, uint16_t sample);
void TempGetTask(void);''')
write('BSP/temp.h',s)
write('BSP/bh66f5242.h','''#ifndef MATHIS_BH66F5242_H
#define MATHIS_BH66F5242_H
#include "temp.h"
/* Compatibility entry points use the canonical temperature types and constants. */
extern uint8_t TempUnplugeErr[11];
void TempGet_Init(void);
void TempInit(void);
void TempStabDis(void);
Temp_GetTypeDef ThermocoupleTempGet_Task(void);
void LCDDisplayCalculation(Temp_GetTypeDef *temperature);
uint32_t adc_filter(uint8_t channel);
#endif
''')
s=read('BSP/bh66f5242.c').replace('#include "config.h"','#include "bh66f5242.h"')
write('BSP/bh66f5242.c',s)
s=read('BSP/C8721.h').replace('#define __C8721_H__','#define __C8721_H__\n#include <stdint.h>')
write('BSP/C8721.h',s)
for name in ['key.c','key.h']:
    s=read('BSP/'+name)
    s=re.sub(r'(void\(\*Key\w+\))\(\)',r'\1(void)',s)
    if name.endswith('.c'):
        s=re.sub(r'\(\*(Key\w+)\)\(\);',r'if (\1 != 0) { \1(); }',s)
    write('BSP/'+name,s)
s=read('COP/start_up.h').replace('void start_up_control(void);','void start_up_control(void);\nvoid ResetStartUpControl(void);\nvoid StartUpTimeFillIrq(void);')
write('COP/start_up.h',s)
s=read('COP/warning.h').replace('void WarningDetect(void);','void WarningDetect(void);\nvoid Task_AlarmPeriodic100ms(void);')
write('COP/warning.h',s)
# Include each module's own declaration in its implementation, including inactive legacy sources.
for p in list((root/'COP').glob('*.c'))+list((root/'BSP').glob('*.c')):
    header=p.with_suffix('.h')
    if not header.exists(): continue
    name=str(p.relative_to(root)); s=read(name)
    if '#include "config.h"' in s and f'#include "{header.name}"' not in s:
        s=s.replace('#include "config.h"',f'#include "{header.name}"\n#include "config.h"',1)
        write(name,s)
s=read('MDK-ARM/tw66gw02.uvprojx')
needle='''            <File>
              <FileName>wireless.c</FileName>'''
assert needle in s
entry='''            <File>
              <FileName>mathis_util.c</FileName>
              <FileType>1</FileType>
              <FilePath>..\\BSP\\mathis_util.c</FilePath>
            </File>
'''
write('MDK-ARM/tw66gw02.uvprojx',s.replace(needle,entry+needle))
