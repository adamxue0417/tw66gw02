from pathlib import Path
import re

root = Path('tw66gw02')
def read(name):
    p=root/name
    try: return p.read_text(encoding='utf-8')
    except UnicodeDecodeError: return p.read_text(encoding='gb18030')
def write(name,s): (root/name).write_text(s,encoding='utf-8')
def function(s,name,new):
    pattern=r'(?m)^(?:static )?[\w *]+\b'+re.escape(name)+r'\([^;]*?\)\n\{'
    m=re.search(pattern,s)
    assert m,name
    start=m.start(); end=m.end(); depth=1
    while depth:
        if s[end]=='{': depth+=1
        if s[end]=='}': depth-=1
        end+=1
    return s[:start]+new+s[end:]

s=read('Core/Src/main.c')
s=s.replace('  HAL_Init();','  SCH_Init();\n  HAL_Init();')
s=re.sub(r' SCH_Add_Task\(([^;]+)\);',r' if (SCH_Add_Task(\1) == SCH_MAX_TASKS) { Error_Handler(); }',s)
trace='\t*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = OTA_BOOT_TRACE_LOOP_RUNNING;'
s=s.replace(trace,'').replace('  while (1)\n  {',trace+'\n  while (1)\n  {',1)
write('Core/Src/main.c',s)

s=read('BSP/temp.c')
s=s.replace('volatile uint16_t g_battery_mv = 0u;','''volatile uint16_t g_battery_mv = 0u;
volatile uint8_t g_battery_sample_valid = 0u;
static uint8_t s_adc_running;
static volatile uint8_t s_adc_error;
static volatile uint16_t s_adc_sample[USR_ADC_CH_NR];
#define ADC_SAMPLE_TIMEOUT_MS (10u)
typedef char AdcSampleCountValid[(USR_ADC_COUNT > 2u) ? 1 : -1];''')
s=function(s,'Get_Filter_ADC12bitResult','''static uint8_t AcquireAdcBatch(void)
{
    uint16_t i, j;
    uint32_t started;
    channel_env_init();
    g_battery_sample_valid = 0u;
    if (s_adc_running == 0u) {
        s_adc_error = 0u;
        if (HAL_ADCEx_Calibration_Start(&hadc) != HAL_OK) { goto failed; }
        ADC_Start_DMA_OVER = 1u;
        if (HAL_ADC_Start_DMA(&hadc, (uint32_t *)adc_value, USR_ADC_CH_NR) != HAL_OK) { goto failed; }
        s_adc_running = 1u;
        /* Only a complete scan publishes a coherent two-channel sample. */
        __HAL_DMA_DISABLE_IT(hadc.DMA_Handle, DMA_IT_HT);
    }
    for (j = 0u; j < USR_ADC_COUNT; j++) {
        ADC_Start_DMA_OVER = 1u;
        started = HAL_GetTick();
        while (ADC_Start_DMA_OVER != 0u) {
            if ((s_adc_error != 0u) || ((uint32_t)(HAL_GetTick() - started) >= ADC_SAMPLE_TIMEOUT_MS)) { goto failed; }
        }
        if (s_adc_error != 0u) { goto failed; }
        HAL_Delay(1u);
        {
            uint32_t mask = __get_PRIMASK();
            __disable_irq();
            for (i = 0u; i < USR_ADC_CH_NR; i++) {
                uint16_t sample = s_adc_sample[i];
                if (sample > ch_env[i].max) { ch_env[i].max = sample; }
                if (sample < ch_env[i].min) { ch_env[i].min = sample; }
                ch_env[i].sum += sample;
            }
            __set_PRIMASK(mask);
        }
    }
    if (s_adc_error != 0u) { goto failed; }
    for (i = 0u; i < USR_ADC_CH_NR; i++) {
        ch_env[i].vol = (uint16_t)((ch_env[i].sum - ch_env[i].max - ch_env[i].min) / (USR_ADC_COUNT - 2u));
    }
    g_battery_sample_valid = 1u;
    return 1u;
failed:
    (void)HAL_ADC_Stop_DMA(&hadc);
    s_adc_running = 0u;
    ADC_Start_DMA_OVER = 0u;
    return 0u;
}

void Get_Filter_ADC12bitResult(void)
{
    (void)AcquireAdcBatch();
}''')
s=s.replace('vol=(uint32_t)ch_env[channel].vol;', 'if (channel >= USR_ADC_CH_NR) { return TempErr; }\n\tvol=(uint32_t)ch_env[channel].vol;')
s=function(s,'Average_Temp','''uint16_t Average_Temp(uint16_t arr[], uint8_t length)
{
    uint32_t sum = 0u;
    uint8_t i;
    if ((arr == 0) || (length == 0u)) { return 0u; }
    for (i = 0u; i < length; i++) { sum += arr[i]; }
    return (uint16_t)(sum / length);
}''')
s=function(s,'Add_Temp','''void Add_Temp(uint16_t arr[], uint8_t length, uint16_t sample)
{
    if ((arr == 0) || (length == 0u)) { return; }
    memmove(arr, arr + 1, (length - 1u) * sizeof(*arr));
    arr[length - 1u] = sample;
}''')
# Keep the legacy helper and ordered-buffer contract; use private ring state only for the probe.
s=function(s,'Temp_Handle','''uint16_t Temp_Handle(uint16_t arr[], uint8_t length, uint16_t sample)
{
    static uint16_t count;
    static uint16_t *previous_buffer;
    static uint8_t previous_length;
    if ((arr == 0) || (length == 0u)) { return 0u; }
    if ((arr != previous_buffer) || (length != previous_length)) {
        count = 0u; previous_buffer = arr; previous_length = length;
    }
    if (count < length) { arr[count++] = sample; }
    else { Add_Temp(arr, length, sample); }
    return Average_Temp(arr, (uint8_t)count);
}

static uint16_t ProbeAverage(uint16_t sample)
{
    static uint32_t sum;
    static uint8_t count;
    static uint8_t index;
    if (count < 10u) { count++; }
    else { sum -= s_probe_temp_hist[index]; }
    s_probe_temp_hist[index] = sample;
    sum += sample;
    if (++index == 10u) { index = 0u; }
    return (uint16_t)(sum / count);
}''')
s=s.replace('Temp_Handle(s_probe_temp_hist, 10u, (uint16_t)probe_f_raw)','ProbeAverage((uint16_t)probe_f_raw)')
s=function(s,'raw_temp_sort_asc','''static void raw_temp_sort_asc(int16_t values[], uint8_t length)
{
    uint8_t i;
    for (i = 1u; i < length; i++) {
        int16_t value = values[i];
        uint8_t j = i;
        while ((j != 0u) && (values[j - 1u] > value)) {
            values[j] = values[j - 1u]; j--;
        }
        values[j] = value;
    }
}''')
s=function(s,'Temp_Get_Init','''void Temp_Get_Init(void)
{
    /* MX_ADC_Init is owned by main; compatibility callers may initialize first. */
    if (hadc.Instance != ADC1) { MX_ADC_Init(); }
}''')
s=s.replace('    Get_Filter_ADC12bitResult();\n\n    probe_f_raw = Temp_Get(0u);','    probe_f_raw = (AcquireAdcBatch() != 0u) ? Temp_Get(0u) : TempErr;')
s=s.replace('    battery_mV = Battery_Get_mV(1u);\n    UpdateBatteryLevel(battery_mV);','    if (g_battery_sample_valid != 0u) {\n        battery_mV = Battery_Get_mV(1u);\n        UpdateBatteryLevel(battery_mV);\n    }')
s=function(s,'HAL_ADC_ConvCpltCallback','''void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc)
{
    if ((adc != 0) && (adc->Instance == ADC1)) {
        uint8_t i;
        for (i = 0u; i < USR_ADC_CH_NR; i++) { s_adc_sample[i] = adc_value[i]; }
        __DMB();
        ADC_Start_DMA_OVER = 0u;
    }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *adc)
{
    if ((adc != 0) && (adc->Instance == ADC1)) {
        s_adc_error = 1u;
        ADC_Start_DMA_OVER = 0u;
        g_battery_sample_valid = 0u;
    }
}''')
write('BSP/temp.c',s)
s=read('Core/Src/stm32f0xx_it.c').replace('  ADC_Start_DMA_OVER=0;','  /* Completion/error is published by the ADC callbacks, not half-transfer IRQs. */')
write('Core/Src/stm32f0xx_it.c',s)
s=read('BSP/temp.h').replace('void TempGetTask(void);\nvoid Temp_Get_Init(void);\nvoid TempGetTask(void);','void TempGetTask(void);\nvoid Temp_Get_Init(void);\nextern volatile uint8_t g_battery_sample_valid;')
write('BSP/temp.h',s)
s=read('BSP/C8721.c')
s=function(s,'CF_DisplaySegment','''void CF_DisplaySegment(uint8_t step)
{
    static uint8_t checksum;
    if (step == 0u) {
        CF_SendCommandPackage(CmdCommand);
        CF_SendSetPackage();
        CF_SendCommandPackage(CmdData);
        checksum = 0u;
    } else if (step <= 8u) {
        uint8_t i;
        uint8_t begin = (uint8_t)((step - 1u) * 16u);
        for (i = 0u; i < 16u; i++) {
            uint8_t value = CF_DisplayBuf[begin + i];
            CF_SendByte(value);
            checksum += value;
        }
    } else if (step == 9u) {
        CF_SendByte(checksum);
        CF_SendCommandPackage(CmdDataUpdate);
        CF_SendSCK();
    }
}''')
write('BSP/C8721.c',s)
