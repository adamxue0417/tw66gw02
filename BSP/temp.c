/**
  ******************************************************************************
  * @file    temp.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Temperature and battery acquisition implementation.
  ******************************************************************************
  */
#include "config.h"
#include "temp.h"

channel_env_t ch_env[USR_ADC_CH_NR];
Temp_GetTypeDef Temperature;
ErrMessage SystemErrMessage;
volatile uint8_t ADC_Start_DMA_OVER = 0u;
volatile uint8_t g_battery_low = 0u;
volatile uint16_t g_battery_mv = 0u;

static int16_t s_comm_temp_c[3] = {0, 0, 0};
static uint8_t s_comm_temp_valid = 0u;
static int16_t s_probe_c_filt = 0;
static uint8_t s_probe_c_filt_valid = 0u;
static uint8_t s_probe_hi_latched = 0u;
static uint8_t s_probe_hi_pending_cnt = 0u;
static uint8_t s_probe_hi_release_cnt = 0u;
static uint8_t s_probe_lo_latched = 0u;
static uint8_t s_probe_lo_pending_cnt = 0u;
static uint8_t s_probe_lo_release_cnt = 0u;
static uint16_t s_probe_f_prev = 0u;
static uint8_t s_probe_f_prev_valid = 0u;
static uint8_t s_probe_drop_glitch_cnt = 0u;
static uint8_t s_probe_disconnect_pending_cnt = 0u;
static uint8_t s_probe_reconnect_pending_cnt = 0u;
static uint16_t s_probe_temp_hist[10] = {0u};
static uint8_t s_comm_hi_latched[3] = {0u, 0u, 0u};
static uint8_t s_comm_hi_pending_cnt[3] = {0u, 0u, 0u};
static uint8_t s_comm_hi_release_cnt[3] = {0u, 0u, 0u};
static uint8_t s_comm_lo_latched[3] = {0u, 0u, 0u};
static uint8_t s_comm_lo_pending_cnt[3] = {0u, 0u, 0u};
static uint8_t s_comm_lo_release_cnt[3] = {0u, 0u, 0u};
static int16_t s_comm_raw_temp[3][10] = {{0}};
static uint8_t s_comm_raw_temp_inited[3] = {0u, 0u, 0u};
#define PROBE_TEMP_C_LOW_LIMIT   (0)
#define PROBE_TEMP_C_HIGH_LIMIT  (500)
#define PROBE_TEMP_C_HIGH_RELEASE (PROBE_TEMP_C_HIGH_LIMIT)
#define PROBE_TEMP_HI_CONFIRM_COUNT (3u)
#define PROBE_TEMP_HI_RELEASE_CONFIRM_COUNT (2u)
#define PROBE_TEMP_LO_CONFIRM_COUNT (3u)
#define PROBE_TEMP_LO_RELEASE_CONFIRM_COUNT (2u)
#define PROBE_DISCONNECT_CONFIRM_COUNT (3u)
#define PROBE_RECONNECT_CONFIRM_COUNT (2u)
#define BATTERY_LOW_MV_THRESHOLD   (1100u)
#define BATTERY_RECOVER_MV_THRESHOLD (1200u)
#define BATTERY_DEBOUNCE_COUNT     (20u)
/* TempGetTask runs every 500 ms: 120 samples form a 60-second window. */
#define BATTERY_ADC_AVERAGE_SAMPLES (120u)

static uint16_t s_battery_adc_history[BATTERY_ADC_AVERAGE_SAMPLES] = {0u};
static uint32_t s_battery_adc_sum = 0u;
static uint16_t s_battery_adc_index = 0u;
static uint16_t s_battery_adc_count = 0u;

/* PT1000 lookup table: -25C..500C, step 5C, value scaled by 10 (10000 = 1000.0 ohm) */
static const uint16_t RTD_TAB_PT1000[] =
{
    9020,9216,9413,9609,9805,
    10000,10195,10390,10584,10779,10973,11167,11360,11554,11747,
    11939,12132,12324,12516,12707,12898,13089,13280,13470,13660,
    13850,14040,14229,14418,14606,14795,14983,15170,15358,15545,
    15732,15919,16105,16291,16477,16662,16847,17032,17217,17401,
    17585,17769,17952,18135,18318,18501,18683,18865,19047,19228,
    19409,19590,19771,19951,20131,20311,20490,20669,20848,21026,
    21205,21383,21560,21738,21915,22092,22268,22444,22620,22796,
    22971,23146,23321,23495,23670,23844,24017,24191,24364,24536,
    24709,24881,25053,25224,25396,25567,25737,25908,26078,26248,
    26417,26587,26770,26937,27104,27271,27437,27604,27769,27934,
    28098
};

/**
  * @function channel_env_init()
  * ------------------
  * @brief    Initialize ADC channel accumulation context.
  * @param    None
  * @note     None
  */
void channel_env_init(void)
{
    uint8_t i;
    for (i = 0; i < USR_ADC_CH_NR; ++i)
    {
        ch_env[i].max = 0u;
        ch_env[i].min = 0x0FFFu;
        ch_env[i].vol = 0u;
        ch_env[i].sum = 0u;
        adc_value[i] = 0u;
    }
}

/**
  * @function Get_Filter_ADC12bitResult()
  * ---------------------------
  * @brief    Acquire filtered ADC values for all local channels.
  * @param    None
  * @note     None
  */
void Get_Filter_ADC12bitResult(void)
{
    uint16_t i, j;

    channel_env_init();
    HAL_ADCEx_Calibration_Start(&hadc);

    for (j = 0u; j < USR_ADC_COUNT; j++)
    {
        HAL_ADC_Start_DMA(&hadc, (uint32_t *)adc_value, USR_ADC_CH_NR);
        ADC_Start_DMA_OVER = 1u;
        while (ADC_Start_DMA_OVER) {}

        HAL_Delay(1);

        for (i = 0u; i < USR_ADC_CH_NR; i++)
        {
            ch_env[i].max = (adc_value[i] > ch_env[i].max) ? adc_value[i] : ch_env[i].max;
            ch_env[i].min = (adc_value[i] < ch_env[i].min) ? adc_value[i] : ch_env[i].min;
            ch_env[i].sum += adc_value[i];
        }
    }

    for (i = 0u; i < USR_ADC_CH_NR; ++i)
    {
        ch_env[i].sum = ch_env[i].sum - ch_env[i].max - ch_env[i].min;
        ch_env[i].vol = (uint16_t)(ch_env[i].sum / (USR_ADC_COUNT - 2u));
    }
}

/**
  * @function PT1000_CalculateTemperature()
  * -----------------------------
  * @brief    Convert PT1000 resistance to Celsius temperature.
  * @param    fR - input parameter
  * @note     None
  */
float PT1000_CalculateTemperature(uint16_t fR)
{
    const int16_t temp_min = -25;
    const int16_t step = 5;
    const uint8_t tab_last = (uint8_t)(sizeof(RTD_TAB_PT1000) / sizeof(RTD_TAB_PT1000[0]) - 1u);
    uint8_t cBottom, cTop, i;
    float fLowRValue;
    float fHighRValue;
    int16_t iTem;
    float fTem;

    if (fR < RTD_TAB_PT1000[0])
    {
        return -25.0f;
    }

    if (fR > RTD_TAB_PT1000[tab_last])
    {
        return 500.0f;
    }

    cBottom = 0u;
    cTop = tab_last;
    i = (uint8_t)((cTop + cBottom) / 2u);

    for (; (cTop - cBottom) != 1u; )
    {
        if (fR < RTD_TAB_PT1000[i])
        {
            cTop = i;
            i = (uint8_t)((cTop + cBottom) / 2u);
        }
        else if (fR > RTD_TAB_PT1000[i])
        {
            cBottom = i;
            i = (uint8_t)((cTop + cBottom) / 2u);
        }
        else
        {
            iTem = (int16_t)(temp_min + (int16_t)i * step);
            return (float)iTem;
        }
    }

    iTem = (int16_t)(temp_min + (int16_t)cBottom * step);
    fLowRValue  = RTD_TAB_PT1000[cBottom];
    fHighRValue = RTD_TAB_PT1000[cTop];

    fTem = ((((float)fR - fLowRValue) * step) / (fHighRValue - fLowRValue)) + iTem;
    return fTem;
}

/**
  * @function Temp_Get()
  * ------------
  * @brief    Convert raw ADC reading to probe temperature.
  * @param    channel - input parameter
  * @note     None
  */
int16_t Temp_Get(uint8_t channel)
{
	int16_t temp;
	uint32_t R_Value,vol;
	vol=(uint32_t)ch_env[channel].vol; 
  
	if(vol>3723)		 
	{
		temp=TempErr;
	}
	else
	{
		R_Value=20000*(uint32_t)vol/(4096-(uint32_t)vol);
		/* Distinguish open/error from out-of-range temperature. */
		if (R_Value < 9216u)
		{
			temp = TempLow;
		}
		else if (R_Value > 28098u)
		{
			temp = TempHigh;
		}
		else
		{
            int16_t temp_c = (int16_t)PT1000_CalculateTemperature((uint16_t)R_Value);
			temp = (int16_t)(((int32_t)temp_c * 9) / 5 + 32);
		}
	}
	return	temp;
}

/**
  * @function Average_Temp()
  * --------------
  * @brief    Calculate the arithmetic average of a temperature buffer.
  * @param    arr - input parameter
  * @param    Length - input parameter
  * @note     None
  */
uint16_t  Average_Temp(uint16_t arr[],uint8_t Length)
{
	static uint8_t i = 0;
	static uint16_t temp = 0;
	static uint16_t Total_temp = 0;
	uint16_t *q = arr;
	for(i=0;i<Length;i++)
	{
		temp=*(q+i);
		Total_temp+=temp;
	}
	
	temp = Total_temp/Length;
	Total_temp=0;
	return temp;
}

/**
  * @function Add_Temp()
  * ------------
  * @brief    Push one new sample into a fixed-size history buffer.
  * @param    arr - target history buffer
  * @param    Length - buffer length
  * @param    New_Data - new sample to append
  * @note     None
  */
void Add_Temp(uint16_t arr[],uint8_t Length,uint16_t New_Data)
{
	uint16_t *str = arr;
	static uint8_t i = 0;
	for(i=0;i<Length-1;i++)
	{
		*(str+i)=*(str+1+i);
	}
	*(str+Length-1)=New_Data;
}


/**
  * @function Temp_Handle()
  * ---------------
  * @brief    Update moving-average state for probe temperature.
  * @param    arr - history buffer used for smoothing
  * @param    length - window length
  * @param    temp - latest probe sample
  * @note     None
  */
uint16_t Temp_Handle(uint16_t arr[],uint8_t length, uint16_t temp)
{
	static uint8_t Size=0;
	static uint16_t InTtemp=0;
	Size++;
	if(Size<=length)
	{
		arr[Size-1]=temp;
		InTtemp=Average_Temp(arr,Size);
	}
	else
	{
		Size=length+1;
		Add_Temp(arr,length,temp);
		InTtemp=Average_Temp(arr,length);
	}
	
	return InTtemp;
}

/* channel: ADC1=battery voltage */
/**
  * @function Battery_Get_mV()
  * ----------------
  * @brief    Average battery ADC over one minute, then convert it to millivolts.
  * @param    channel - input parameter
  * @note     None
  */
static uint16_t Battery_Get_mV(uint8_t channel)
{
    uint16_t adc = ch_env[channel].vol;
    uint16_t average_adc;

    if (s_battery_adc_count < BATTERY_ADC_AVERAGE_SAMPLES)
    {
        s_battery_adc_count++;
    }
    else
    {
        s_battery_adc_sum -= s_battery_adc_history[s_battery_adc_index];
    }

    s_battery_adc_history[s_battery_adc_index] = adc;
    s_battery_adc_sum += adc;
    s_battery_adc_index++;
    if (s_battery_adc_index >= BATTERY_ADC_AVERAGE_SAMPLES)
    {
        s_battery_adc_index = 0u;
    }

    /* Round to the nearest ADC count. During startup, average all samples
       collected so far instead of delaying battery display for 60 seconds. */
    average_adc = (uint16_t)((s_battery_adc_sum + (s_battery_adc_count / 2u)) /
                             s_battery_adc_count);
    return (uint16_t)(((uint32_t)average_adc * 3300u) / 4095u);
}

/**
  * @function UpdateBatteryLevel()
  * --------------------
  * @brief    Update battery low flag with debounce and hysteresis.
  * @param    batt_mV - input parameter
  * @note     None
  */
static void UpdateBatteryLevel(uint16_t batt_mV)
{
    static uint8_t low_cnt = 0u;
    static uint8_t high_cnt = 0u;

    g_battery_mv = batt_mV;

    /* Requirement: low battery only when voltage is below 1.1 V. */
    if (batt_mV < BATTERY_LOW_MV_THRESHOLD)
    {
        if (low_cnt < BATTERY_DEBOUNCE_COUNT) { low_cnt++; }
        high_cnt = 0u;
        if (low_cnt >= BATTERY_DEBOUNCE_COUNT) { g_battery_low = 1u; }
    }
    else if (batt_mV >= BATTERY_RECOVER_MV_THRESHOLD)
    {
        if (high_cnt < BATTERY_DEBOUNCE_COUNT) { high_cnt++; }
        low_cnt = 0u;
        if (high_cnt >= BATTERY_DEBOUNCE_COUNT) { g_battery_low = 0u; }
    }
}

/**
  * @function parse_be32_raw_c()
  * -----------------------
  * @brief    Parse a big-endian 32-bit temperature payload into Celsius or disconnected sentinel.
  * @param    buf - input parameter
  * @note     None
  */
static int32_t parse_be32_raw_c(const uint8_t *buf)
{
    int32_t raw;

    raw = ((int32_t)buf[0] << 24) |
          ((int32_t)buf[1] << 16) |
          ((int32_t)buf[2] << 8)  |
          ((int32_t)buf[3]);

    /* Source value is scaled by 100. */
    raw /= 100;

    if ((raw == TempDisconnected) || (raw == -10) || (raw == 14)) {
        return TempDisconnected;
    }

    return raw;
}

/**
  * @function normalize_external_temp_c()
  * -------------------------------
  * @brief    Normalize external channel temperature and sentinel values.
  * @param    raw - input parameter
  * @note     None
  */
static int16_t normalize_external_temp_c(int32_t raw)
{
    if (raw == TempDisconnected) {
        return TempDisconnected;
    }
    if (raw > PROBE_TEMP_C_HIGH_LIMIT) { return TempHigh; }
    if (raw < PROBE_TEMP_C_LOW_LIMIT) { return TempLow; }

    return (int16_t)raw;
}

static void raw_temp_sort_asc(int16_t temp_sort[], uint8_t len)
{
    uint8_t i;
    uint8_t j;
    int16_t t;

    for (i = 0u; i < len; i++)
    {
        for (j = (uint8_t)(i + 1u); j < len; j++)
        {
            if (temp_sort[i] > temp_sort[j])
            {
                t = temp_sort[i];
                temp_sort[i] = temp_sort[j];
                temp_sort[j] = t;
            }
        }
    }
}

static int16_t raw_temp_filter(uint8_t idx, int16_t temp)
{
    int16_t temp_sort[10];
    int32_t sum;
    uint8_t i;

    if (idx >= 3u) {
        return temp;
    }

    if ((temp == HaveTempErr) || (temp == TempDisconnected) || (temp == TempHigh) || (temp == TempLow))
    {
        s_comm_raw_temp_inited[idx] = 0u;
        return temp;
    }

    if (s_comm_raw_temp_inited[idx] == 0u)
    {
        s_comm_raw_temp_inited[idx] = 1u;
        for (i = 0u; i < 10u; i++)
        {
            s_comm_raw_temp[idx][i] = temp;
        }
        return temp;
    }

    for (i = 0u; i < 9u; i++)
    {
        s_comm_raw_temp[idx][i] = s_comm_raw_temp[idx][i + 1u];
    }
    s_comm_raw_temp[idx][9] = temp;

    for (i = 0u; i < 10u; i++)
    {
        temp_sort[i] = s_comm_raw_temp[idx][i];
    }

    raw_temp_sort_asc(temp_sort, 10u);

    sum = (int32_t)temp_sort[3] + (int32_t)temp_sort[4] + (int32_t)temp_sort[5] + (int32_t)temp_sort[6];
    if (sum >= 0) {
        return (int16_t)((sum + 2) / 4);
    }
    return (int16_t)((sum - 2) / 4);
}

static int16_t debounce_external_temp(uint8_t idx, int16_t sample)
{
    if (idx >= 3u) {
        return sample;
    }

    if ((sample == HaveTempErr) || (sample == TempDisconnected)) {
        s_comm_hi_latched[idx] = 0u;
        s_comm_hi_pending_cnt[idx] = 0u;
        s_comm_hi_release_cnt[idx] = 0u;
        s_comm_lo_latched[idx] = 0u;
        s_comm_lo_pending_cnt[idx] = 0u;
        s_comm_lo_release_cnt[idx] = 0u;
        return sample;
    }

    if (s_comm_hi_latched[idx] != 0u) {
        if (sample == TempHigh) {
            s_comm_hi_release_cnt[idx] = 0u;
            return TempHigh;
        }
        if (s_comm_hi_release_cnt[idx] < 255u) { s_comm_hi_release_cnt[idx]++; }
        if (s_comm_hi_release_cnt[idx] < PROBE_TEMP_HI_RELEASE_CONFIRM_COUNT) {
            return TempHigh;
        }
        s_comm_hi_latched[idx] = 0u;
        s_comm_hi_pending_cnt[idx] = 0u;
        s_comm_hi_release_cnt[idx] = 0u;
    }

    if (s_comm_lo_latched[idx] != 0u) {
        if (sample == TempLow) {
            s_comm_lo_release_cnt[idx] = 0u;
            return TempLow;
        }
        if (s_comm_lo_release_cnt[idx] < 255u) { s_comm_lo_release_cnt[idx]++; }
        if (s_comm_lo_release_cnt[idx] < PROBE_TEMP_LO_RELEASE_CONFIRM_COUNT) {
            return TempLow;
        }
        s_comm_lo_latched[idx] = 0u;
        s_comm_lo_pending_cnt[idx] = 0u;
        s_comm_lo_release_cnt[idx] = 0u;
    }

    if (sample == TempHigh) {
        s_comm_lo_pending_cnt[idx] = 0u;
        if (s_comm_hi_pending_cnt[idx] < 255u) { s_comm_hi_pending_cnt[idx]++; }
        if (s_comm_hi_pending_cnt[idx] >= PROBE_TEMP_HI_CONFIRM_COUNT) {
            s_comm_hi_latched[idx] = 1u;
            s_comm_hi_release_cnt[idx] = 0u;
            return TempHigh;
        }
        return PROBE_TEMP_C_HIGH_LIMIT;
    }

    if (sample == TempLow) {
        s_comm_hi_pending_cnt[idx] = 0u;
        if (s_comm_lo_pending_cnt[idx] < 255u) { s_comm_lo_pending_cnt[idx]++; }
        if (s_comm_lo_pending_cnt[idx] >= PROBE_TEMP_LO_CONFIRM_COUNT) {
            s_comm_lo_latched[idx] = 1u;
            s_comm_lo_release_cnt[idx] = 0u;
            return TempLow;
        }
        return PROBE_TEMP_C_LOW_LIMIT;
    }

    s_comm_hi_pending_cnt[idx] = 0u;
    s_comm_lo_pending_cnt[idx] = 0u;
    return sample;
}

/**
  * @function update_three_temps_from_uart2()
  * -------------------------------
  * @brief    Request and parse three external PT1000 temperatures by UART2.
  * @param    None
  * @note     None
  */
static void update_three_temps_from_uart2(void)
{
    uint8_t rx[32];
    uint16_t rx_len;
    uint8_t tx_req[8] = {0xAA, 0x55, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    int32_t raw0;
    int32_t raw1;
    int32_t raw2;

    /* Use blocking TX on USART2 to avoid DMA state-machine lockups. */
    (void)HAL_UART_Transmit(&huart2, tx_req, (uint16_t)sizeof(tx_req), 20u);

    uart_dma_poll_check(&huart2, &hdma_usart2_rx, &COM2);

    rx_len = COM_TakeRx(&COM2, rx, sizeof(rx));
    if (rx_len < 15u) {
        return;
    }

    raw0 = parse_be32_raw_c(&rx[3]);
    raw1 = parse_be32_raw_c(&rx[7]);
    raw2 = parse_be32_raw_c(&rx[11]);

    if ((raw0 == 0) && (raw1 == 0) && (raw2 == 0)) {
        s_comm_temp_c[0] = TempDisconnected;
        s_comm_temp_c[1] = TempDisconnected;
        s_comm_temp_c[2] = TempDisconnected;
    } else {
        s_comm_temp_c[0] = normalize_external_temp_c(raw0);
        s_comm_temp_c[1] = normalize_external_temp_c(raw1);
        s_comm_temp_c[2] = normalize_external_temp_c(raw2);
    }

    s_comm_temp_valid = 1u;
}

/**
  * @function Temp_Get_Init()
  * ---------------
  * @brief    Initialize temperature acquisition peripherals.
  * @param    None
  * @note     None
  */
void Temp_Get_Init(void)
{
    MX_ADC_Init();
}

/**
  * @function TempGetTask()
  * -------------
  * @brief    Run one temperature acquisition and state update cycle.
  * @param    None
  * @note     None
  */
void TempGetTask(void)
{
    uint8_t i;
    int32_t t;
    int16_t probe_f;
    int16_t probe_f_raw;
    int16_t probe_c;
    uint16_t battery_mV;

    if (work_process == shutdown) { return; }
    update_three_temps_from_uart2();
    if (s_comm_temp_valid != 0u)
    {
        for (i = 0u; i < 3u; i++)
        {
            t = debounce_external_temp(i, s_comm_temp_c[i]);
            t = raw_temp_filter(i, (int16_t)t);

            if ((t == HaveTempErr) || (t == TempDisconnected) || (t == TempHigh) || (t == TempLow))
            {
                system_data.pt1000_temp[i] = (int16_t)t;
                continue;
            }

            if (t < PROBE_TEMP_C_LOW_LIMIT) { t = PROBE_TEMP_C_LOW_LIMIT; }
            if (t > PROBE_TEMP_C_HIGH_LIMIT) { t = PROBE_TEMP_C_HIGH_LIMIT; }
            system_data.pt1000_temp[i] = (int16_t)t;
        }
    }

    /* Local ADC channels:
     * CH0 -> probe temperature
     * CH1 -> battery level
     */
    Get_Filter_ADC12bitResult();

    probe_f_raw = Temp_Get(0u);
    if ((probe_f_raw != TempErr) && (probe_f_raw != TempHigh) && (probe_f_raw != TempLow) && (probe_f_raw >= 0))
    {
        probe_f = (int16_t)Temp_Handle(s_probe_temp_hist, 10u, (uint16_t)probe_f_raw);
    }
    else
    {
        probe_f = probe_f_raw;
    }

    /* High-temperature ADC drop glitch guard:
     * if previous reading is >=350C and current reading suddenly drops a lot,
     * ignore short transient drops to avoid false plunge. */
    if ((probe_f != TempErr) && (probe_f != TempHigh) && (probe_f != TempLow) && (probe_f >= 0) && (probe_f < 932))
    {
        if ((s_probe_f_prev_valid != 0u) && (s_probe_f_prev >= 662u) &&
            (((probe_f + 120) < s_probe_f_prev) || ((probe_f + 72) < s_probe_f_prev)))
        {
            if (s_probe_drop_glitch_cnt < 255u) { s_probe_drop_glitch_cnt++; }
            if (s_probe_drop_glitch_cnt < 3u)
            {
                probe_f = s_probe_f_prev;
            }
            else
            {
                s_probe_drop_glitch_cnt = 0u;
                s_probe_f_prev = probe_f;
            }
        }
        else
        {
            s_probe_drop_glitch_cnt = 0u;
            s_probe_f_prev = probe_f;
            s_probe_f_prev_valid = 1u;
        }
    }
    battery_mV = Battery_Get_mV(1u);
    UpdateBatteryLevel(battery_mV);
    if (probe_f == TempErr)
    {
        s_probe_reconnect_pending_cnt = 0u;
        if (s_probe_disconnect_pending_cnt < 255u) { s_probe_disconnect_pending_cnt++; }

        if ((g_probe_connected != 0u) && (s_probe_disconnect_pending_cnt < PROBE_DISCONNECT_CONFIRM_COUNT))
        {
            return;
        }

        g_probe_connected = 0u;
        g_probe_over_hi = 0u;
        g_probe_over_lo = 0u;
        s_probe_c_filt_valid = 0u;
        s_probe_hi_latched = 0u;
        s_probe_hi_pending_cnt = 0u;
        s_probe_hi_release_cnt = 0u;
        s_probe_lo_latched = 0u;
        s_probe_lo_pending_cnt = 0u;
        s_probe_lo_release_cnt = 0u;
        s_probe_f_prev_valid = 0u;
        s_probe_drop_glitch_cnt = 0u;
        s_probe_disconnect_pending_cnt = 0u;
        return;
    }

    s_probe_disconnect_pending_cnt = 0u;
    if (g_probe_connected == 0u)
    {
        if (s_probe_reconnect_pending_cnt < 255u) { s_probe_reconnect_pending_cnt++; }
        if (s_probe_reconnect_pending_cnt < PROBE_RECONNECT_CONFIRM_COUNT)
        {
            return;
        }
    }
    s_probe_reconnect_pending_cnt = 0u;
    if ((probe_f == TempHigh) || (probe_f > 932))
    {
        if (s_probe_hi_pending_cnt < 255u) { s_probe_hi_pending_cnt++; }
        if (s_probe_hi_pending_cnt >= PROBE_TEMP_HI_CONFIRM_COUNT)
        {
            s_probe_hi_latched = 1u;
            g_probe_connected = 1u;
            g_probe_over_hi = 1u;
            g_probe_over_lo = 0u;
            s_probe_c_filt_valid = 0u;
            s_probe_hi_release_cnt = 0u;
            s_probe_lo_latched = 0u;
            s_probe_lo_pending_cnt = 0u;
            s_probe_lo_release_cnt = 0u;
            system_data.pt1000_temp[3] = PROBE_TEMP_C_HIGH_LIMIT;
            return;
        }
    }
    else
    {
        s_probe_hi_pending_cnt = 0u;
    }

    g_probe_connected = 1u;
    if (probe_f == TempLow)
    {
        if (s_probe_lo_pending_cnt < 255u) { s_probe_lo_pending_cnt++; }
        if (s_probe_lo_pending_cnt >= PROBE_TEMP_LO_CONFIRM_COUNT)
        {
            s_probe_lo_latched = 1u;
            g_probe_over_hi = 0u;
            g_probe_over_lo = 1u;
            s_probe_c_filt_valid = 0u;
            s_probe_hi_latched = 0u;
            s_probe_hi_pending_cnt = 0u;
            s_probe_hi_release_cnt = 0u;
            s_probe_lo_release_cnt = 0u;
            system_data.pt1000_temp[3] = PROBE_TEMP_C_LOW_LIMIT;
        }
        return;
    }
    else
    {
        s_probe_lo_pending_cnt = 0u;
    }

    probe_c = (int16_t)((((int32_t)probe_f - 32) * 5) / 9);
    /* HI hysteresis: keep -HI until temperature drops below release threshold. */
    if (s_probe_hi_latched != 0u)
    {
        if (probe_c > PROBE_TEMP_C_HIGH_RELEASE)
        {
            g_probe_over_hi = 1u;
            g_probe_over_lo = 0u;
            s_probe_hi_release_cnt = 0u;
            system_data.pt1000_temp[3] = PROBE_TEMP_C_HIGH_LIMIT;
            return;
        }
        if (s_probe_hi_release_cnt < 255u) { s_probe_hi_release_cnt++; }
        if (s_probe_hi_release_cnt < PROBE_TEMP_HI_RELEASE_CONFIRM_COUNT)
        {
            g_probe_over_hi = 1u;
            g_probe_over_lo = 0u;
            system_data.pt1000_temp[3] = PROBE_TEMP_C_HIGH_LIMIT;
            return;
        }
        s_probe_hi_latched = 0u;
        s_probe_hi_pending_cnt = 0u;
        s_probe_hi_release_cnt = 0u;
        s_probe_f_prev_valid = 0u;
        s_probe_drop_glitch_cnt = 0u;
    }
    if (s_probe_lo_latched != 0u)
    {
        if (probe_c < PROBE_TEMP_C_LOW_LIMIT)
        {
            g_probe_over_hi = 0u;
            g_probe_over_lo = 1u;
            s_probe_lo_release_cnt = 0u;
            system_data.pt1000_temp[3] = PROBE_TEMP_C_LOW_LIMIT;
            return;
        }
        if (s_probe_lo_release_cnt < 255u) { s_probe_lo_release_cnt++; }
        if (s_probe_lo_release_cnt < PROBE_TEMP_LO_RELEASE_CONFIRM_COUNT)
        {
            g_probe_over_hi = 0u;
            g_probe_over_lo = 1u;
            system_data.pt1000_temp[3] = PROBE_TEMP_C_LOW_LIMIT;
            return;
        }
        s_probe_lo_latched = 0u;
        s_probe_lo_pending_cnt = 0u;
        s_probe_lo_release_cnt = 0u;
        s_probe_f_prev_valid = 0u;
        s_probe_drop_glitch_cnt = 0u;
    }
    /* Probe anti-jitter with amplitude-adaptive response:
     * small changes -> slow/stable, large changes -> fast tracking. */
    if (s_probe_c_filt_valid == 0u)
    {
        s_probe_c_filt = probe_c;
        s_probe_c_filt_valid = 1u;
    }
    else
    {
        int16_t delta = (int16_t)(probe_c - s_probe_c_filt);
        int16_t abs_delta = (delta >= 0) ? delta : (int16_t)(-delta);
        int16_t step;
        int32_t num_old;
        int32_t num_new;
        int32_t den;

        if (abs_delta <= 2) {
            /* tiny jitter: very slow response */
            step = 1;
            num_old = 7; num_new = 1; den = 8;
        } else if (abs_delta <= 8) {
            /* small movement: moderate smoothing */
            step = 3;
            num_old = 3; num_new = 1; den = 4;
        } else if (abs_delta <= 20) {
            /* medium movement: faster tracking */
            step = 8;
            num_old = 1; num_new = 1; den = 2;
        } else {
            /* large movement: very fast tracking */
            step = 20;
            num_old = 0; num_new = 1; den = 1;
        }

        if (delta > step) {
            probe_c = (int16_t)(s_probe_c_filt + step);
        } else if (delta < -step) {
            probe_c = (int16_t)(s_probe_c_filt - step);
        }

        s_probe_c_filt = (int16_t)(((int32_t)s_probe_c_filt * num_old + (int32_t)probe_c * num_new) / den);
        probe_c = s_probe_c_filt;
    }
    if (probe_c > PROBE_TEMP_C_HIGH_LIMIT)
    {
        if (s_probe_hi_pending_cnt < 255u) { s_probe_hi_pending_cnt++; }
        if (s_probe_hi_pending_cnt >= PROBE_TEMP_HI_CONFIRM_COUNT)
        {
            s_probe_hi_latched = 1u;
            s_probe_hi_release_cnt = 0u;
            g_probe_over_hi = 1u;
            g_probe_over_lo = 0u;
            s_probe_lo_latched = 0u;
            s_probe_lo_pending_cnt = 0u;
            s_probe_lo_release_cnt = 0u;
            system_data.pt1000_temp[3] = PROBE_TEMP_C_HIGH_LIMIT;
            return;
        }
    }
    else
    {
        s_probe_hi_pending_cnt = 0u;
        g_probe_over_hi = 0u;
    }
    if (probe_c < PROBE_TEMP_C_LOW_LIMIT) {
        if (s_probe_lo_pending_cnt < 255u) { s_probe_lo_pending_cnt++; }
        if (s_probe_lo_pending_cnt >= PROBE_TEMP_LO_CONFIRM_COUNT)
        {
            s_probe_lo_latched = 1u;
            s_probe_lo_release_cnt = 0u;
            g_probe_over_hi = 0u;
            g_probe_over_lo = 1u;
            system_data.pt1000_temp[3] = PROBE_TEMP_C_LOW_LIMIT;
            return;
        }
    } else {
        s_probe_lo_pending_cnt = 0u;
        g_probe_over_lo = 0u;
    }

    system_data.pt1000_temp[3] = probe_c;
}

/**
  * @function HAL_ADC_ConvCpltCallback()
  * --------------------------
  * @brief    Handle ADC conversion complete callback.
  * @param    hadc - input parameter
  * @note     None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        ADC_Start_DMA_OVER = 0u;
    }
}



