#include "config.h"
#include "temp.h"

channel_env_t ch_env[USR_ADC_CH_NR];
Temp_GetTypeDef Temperature;
ErrMessage SystemErrMessage;
volatile uint8_t ADC_Start_DMA_OVER = 0u;
volatile uint8_t g_battery_low = 0u;

static int16_t s_comm_temp_c[3] = {0, 0, 0};
static uint8_t s_comm_temp_valid = 0u;
static int16_t s_probe_c_filt = 0;
static uint8_t s_probe_c_filt_valid = 0u;
static uint8_t s_probe_hi_latched = 0u;
static uint8_t s_probe_hi_pending_cnt = 0u;
static uint8_t s_probe_hi_release_cnt = 0u;
static uint16_t s_probe_f_prev = 0u;
static uint8_t s_probe_f_prev_valid = 0u;
static uint8_t s_probe_drop_glitch_cnt = 0u;
static uint8_t s_probe_disconnect_pending_cnt = 0u;
static uint8_t s_probe_reconnect_pending_cnt = 0u;
static uint16_t s_probe_temp_hist[10] = {0u};
#define PROBE_TEMP_C_LOW_LIMIT   (-20)
#define PROBE_TEMP_C_HIGH_LIMIT  (455)
#define PROBE_TEMP_C_HIGH_RELEASE (445)
#define PROBE_TEMP_HI_CONFIRM_COUNT (3u)
#define PROBE_TEMP_HI_RELEASE_CONFIRM_COUNT (2u)
#define PROBE_DISCONNECT_CONFIRM_COUNT (3u)
#define PROBE_RECONNECT_CONFIRM_COUNT (2u)
#define BATTERY_LOW_MV_THRESHOLD   (1100u)
#define BATTERY_RECOVER_MV_THRESHOLD (1200u)
#define BATTERY_DEBOUNCE_COUNT     (20u)

/* PT1000 lookup table: -25C..455C, step 5C, value scaled by 10 (10000 = 1000.0 ohm) */
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
    26417,26587
};

/**
 * @brief Initialize ADC channel accumulation context.
 * @details Clears max/min/sum/filtered voltage buffers before a new ADC sampling round.
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
 * @brief Acquire filtered ADC values for all local channels.
 * @details Runs ADC calibration, samples multiple frames via DMA, removes max/min, and keeps mean value.
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
 * @brief Convert PT1000 resistance to temperature in Celsius.
 * @details Uses binary search on a lookup table (-20C to 455C) and linear interpolation between points.
 * @param fR PT1000 resistance value scaled by 10.
 * @return Temperature in Celsius.
 */
float PT1000_CalculateTemperature(uint16_t fR)
{
    const int16_t temp_min = -20;
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
        return 455.0f;
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
 * @brief Convert raw ADC channel reading to probe temperature.
 * @details Translates ADC voltage to PT1000 resistance and returns Fahrenheit, TempErr, or TempHigh sentinel.
 * @param channel ADC channel index, where channel 0 is probe temperature.
 * @return Probe temperature in Fahrenheit or error sentinel.
 */
uint16_t Temp_Get(uint8_t channel) 
{
	uint16_t temp;
	uint32_t R_Value,vol;
	vol=(uint32_t)ch_env[channel].vol; 
  
	if(vol>3723)		 
	{
		temp=TempErr;
	}
	else
	{
		R_Value=20000*(uint32_t)vol/(4096-(uint32_t)vol);
		/* Distinguish unplug/error from over-temperature. */
		if (R_Value < 9020u)
		{
			temp = TempErr;
		}
		else if (R_Value > 26587u)
		{
			temp = TempHigh;
		}
		else
		{
			temp=(uint16_t)PT1000_CalculateTemperature((uint16_t)R_Value);
			temp=temp*9/5+32;
		}
	}
	return	temp;
}

/**
 * @brief Compute arithmetic mean of a temperature buffer.
 * @details Sums all items in the provided array and returns integer average.
 * @param arr Input temperature buffer.
 * @param Length Number of elements to average.
 * @return Averaged temperature value.
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
 * @brief Push one new sample into a fixed-size history buffer.
 * @details Shifts the buffer to the left and appends the latest sample at the tail.
 * @param arr Target history buffer.
 * @param Length Buffer length.
 * @param New_Data New sample to append.
 */
void Add_Temp(uint16_t arr[],uint8_t Length,uint16_t New_Data)		//在数组中加入一个新元素
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
 * @brief Update moving-average state for probe temperature.
 * @details Fills the buffer during startup, then keeps a rolling window and returns current average.
 * @param arr History buffer used for smoothing.
 * @param length Window length.
 * @param temp Latest probe sample.
 * @return Smoothed probe temperature sample.
 */
uint16_t Temp_Handle(uint16_t arr[],uint8_t length, uint16_t temp)	//温度处理
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
 * @brief Convert ADC reading to battery voltage in millivolts.
 * @details Applies linear scaling from 12-bit ADC code to the 3.3V reference domain.
 * @param channel ADC channel index, where channel 1 is battery voltage.
 * @return Battery voltage in mV.
 */
static uint16_t Battery_Get_mV(uint8_t channel)
{
    uint32_t vol = (uint32_t)ch_env[channel].vol;
    return (uint16_t)((vol * 3300u) / 4095u);
}

/**
 * @brief Update low-battery flag with debounce and hysteresis.
 * @details Sets or clears global low-battery state only after consecutive threshold confirmations.
 * @param batt_mV Current battery voltage in mV.
 */
static void UpdateBatteryLevel(uint16_t batt_mV)
{
    static uint8_t low_cnt = 0u;
    static uint8_t high_cnt = 0u;

    /* Requirement: low battery when voltage is continuously below ~1.1V */
    if (batt_mV <= BATTERY_LOW_MV_THRESHOLD)
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
 * @brief Parse one big-endian 32-bit temperature payload into Celsius.
 * @details Converts source scaling (/100), validates protocol sentinels and bounds, then returns value or HaveTempErr.
 * @param buf Pointer to 4-byte big-endian payload.
 * @return Temperature in Celsius or HaveTempErr.
 */
static int16_t parse_be32_to_int16_c(const uint8_t *buf)
{
    int32_t raw;

    raw = ((int32_t)buf[0] << 24) |
          ((int32_t)buf[1] << 16) |
          ((int32_t)buf[2] << 8)  |
          ((int32_t)buf[3]);

    /* Source value is scaled by 100. */
    raw /= 100;

    /* Only this function decides whether source value is unplugged/invalid. */
    if ((raw == -10) || (raw == 14) || (raw > 455) || (raw < -20)) {
        return HaveTempErr;
    }

    return (int16_t)raw;
}

/**
 * @brief Request and parse three external PT1000 temperatures via UART2.
 * @details Sends poll frame, checks RX DMA completion, validates frame length, and updates channel cache.
 */
static void update_three_temps_from_uart2(void)
{
    uint8_t tx_req[8] = {0xAA, 0x55, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};

    /* Use blocking TX on USART2 to avoid DMA state-machine lockups. */
    (void)HAL_UART_Transmit(&huart2, tx_req, (uint16_t)sizeof(tx_req), 20u);

    uart_dma_poll_check(&huart2, &hdma_usart2_rx, &COM2);

    if (COM2.rxFlag == 0u) {
        return;
    }

    COM2.rxFlag = 0u;
    if (COM2.rxLen < 15u) {
        return;
    }

    s_comm_temp_c[0] = parse_be32_to_int16_c(&COM2.rxBuf[3]);
    s_comm_temp_c[1] = parse_be32_to_int16_c(&COM2.rxBuf[7]);
    s_comm_temp_c[2] = parse_be32_to_int16_c(&COM2.rxBuf[11]);

    s_comm_temp_valid = 1u;
}

/**
 * @brief Initialize temperature acquisition peripherals.
 * @details Initializes ADC hardware used by local probe and battery acquisition.
 */
void Temp_Get_Init(void)
{
    MX_ADC_Init();
}

/**
 * @brief Run one temperature acquisition and state-update cycle.
 * @details Updates main channels from UART, probe from ADC with filtering and protections, and battery status.
 */
void TempGetTask(void)
{
    uint8_t i;
    int32_t t;
    uint16_t probe_f;
    uint16_t probe_f_raw;
    int16_t probe_c;
    uint16_t battery_mV;

    update_three_temps_from_uart2();
    if (s_comm_temp_valid != 0u)
    {
        for (i = 0u; i < 3u; i++)
        {
            t = s_comm_temp_c[i];

            if (t == HaveTempErr)
            {
                system_data.pt1000_temp[i] = HaveTempErr;
                continue;
            }

            if (system_data.units == unitC)
            {
                if (t < PROBE_TEMP_C_LOW_LIMIT) { t = PROBE_TEMP_C_LOW_LIMIT; }
                if (t > 455) { t = 455; }
                system_data.pt1000_temp[i] = (int16_t)t;
            }
            else
            {
                t = (t * 9) / 5 + 32;
                if (t < -4) { t = -4; }
                if (t > 851) { t = 851; }
                system_data.pt1000_temp[i] = (int16_t)t;
            }
        }
    }

    /* Local ADC channels:
     * CH0 -> probe temperature
     * CH1 -> battery level
     */
    Get_Filter_ADC12bitResult();

    probe_f_raw = Temp_Get(0u);
    if ((probe_f_raw != TempErr) && (probe_f_raw != TempHigh))
    {
        probe_f = Temp_Handle(s_probe_temp_hist, 10u, probe_f_raw);
    }
    else
    {
        probe_f = probe_f_raw;
    }

    /* High-temperature ADC drop glitch guard:
     * if previous reading is >=350C and current reading suddenly drops a lot,
     * ignore short transient drops to avoid false plunge. */
    if ((probe_f != TempErr) && (probe_f != TempHigh) && (probe_f < 851u))
    {
        if ((s_probe_f_prev_valid != 0u) && (s_probe_f_prev >= 662u) &&
            (((probe_f + 120u) < s_probe_f_prev) || ((probe_f + 72u) < s_probe_f_prev)))
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
    if ((probe_f == TempHigh) || (probe_f >= 851u))
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
            if (system_data.units == unitC) {
                system_data.pt1000_temp[3] = PROBE_TEMP_C_HIGH_LIMIT;
            } else {
                system_data.pt1000_temp[3] = (int16_t)(((int32_t)PROBE_TEMP_C_HIGH_LIMIT * 9) / 5 + 32);
            }
            return;
        }
    }
    else
    {
        s_probe_hi_pending_cnt = 0u;
    }

    probe_c = (int16_t)((((int32_t)probe_f - 32) * 5) / 9);
    g_probe_connected = 1u;
    /* HI hysteresis: keep -HI until temperature drops below release threshold. */
    if (s_probe_hi_latched != 0u)
    {
        if (probe_c > PROBE_TEMP_C_HIGH_RELEASE)
        {
            g_probe_over_hi = 1u;
            g_probe_over_lo = 0u;
            s_probe_hi_release_cnt = 0u;
            if (system_data.units == unitC) {
                system_data.pt1000_temp[3] = PROBE_TEMP_C_HIGH_LIMIT;
            } else {
                system_data.pt1000_temp[3] = (int16_t)(((int32_t)PROBE_TEMP_C_HIGH_LIMIT * 9) / 5 + 32);
            }
            return;
        }
        if (s_probe_hi_release_cnt < 255u) { s_probe_hi_release_cnt++; }
        if (s_probe_hi_release_cnt < PROBE_TEMP_HI_RELEASE_CONFIRM_COUNT)
        {
            g_probe_over_hi = 1u;
            g_probe_over_lo = 0u;
            if (system_data.units == unitC) {
                system_data.pt1000_temp[3] = PROBE_TEMP_C_HIGH_LIMIT;
            } else {
                system_data.pt1000_temp[3] = (int16_t)(((int32_t)PROBE_TEMP_C_HIGH_LIMIT * 9) / 5 + 32);
            }
            return;
        }
        s_probe_hi_latched = 0u;
        s_probe_hi_pending_cnt = 0u;
        s_probe_hi_release_cnt = 0u;
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
    if (probe_c >= PROBE_TEMP_C_HIGH_LIMIT)
    {
        if (s_probe_hi_pending_cnt < 255u) { s_probe_hi_pending_cnt++; }
        if (s_probe_hi_pending_cnt >= PROBE_TEMP_HI_CONFIRM_COUNT)
        {
            s_probe_hi_latched = 1u;
            s_probe_hi_release_cnt = 0u;
            g_probe_over_hi = 1u;
            if (system_data.units == unitC) {
                system_data.pt1000_temp[3] = PROBE_TEMP_C_HIGH_LIMIT;
            } else {
                system_data.pt1000_temp[3] = (int16_t)(((int32_t)PROBE_TEMP_C_HIGH_LIMIT * 9) / 5 + 32);
            }
            return;
        }
    }
    else
    {
        s_probe_hi_pending_cnt = 0u;
        g_probe_over_hi = 0u;
    }
    if (probe_c < PROBE_TEMP_C_LOW_LIMIT) {
        g_probe_over_lo = 1u;
    } else {
        g_probe_over_lo = 0u;
    }

    if (system_data.units == unitC)
    {
        system_data.pt1000_temp[3] = probe_c;
    }
    else
    {
        system_data.pt1000_temp[3] = (int16_t)(((int32_t)probe_c * 9) / 5 + 32);
    }
}

/**
 * @brief ADC conversion complete callback.
 * @details Clears DMA wait flag used by synchronous ADC sampling routine when ADC1 transfer finishes.
 * @param hadc ADC handle provided by HAL.
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        ADC_Start_DMA_OVER = 0u;
    }
}



