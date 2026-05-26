/**
  ******************************************************************************
  * @file    screen_c8721.c
  * @author 
  * @version V1.0
  * @date
  * @brief   C8721 screen mapping and display task implementation.
  ******************************************************************************
  */
#include "screen_c8721.h"
#include "C8721.h"
#include "config.h"
_screen_data Screen_Data;
DisplayMode_t DisplayMode = DISPLAY_MODE_D_SURFACE;//default
/* 7-segment display table */
const uint8_t DigitalTubeDisplayTable[] = {
    0x3Fu,  /* 0 */
    0x06u,  /* 1 */
    0x5Bu,  /* 2 */
    0x4Fu,  /* 3 */
    0x66u,  /* 4 */
    0x6Du,  /* 5 */
    0x7Du,  /* 6 */
    0x07u,  /* 7 */
    0x7Fu,  /* 8 */
    0x6Fu,  /* 9 */
	  0x00u,  /*   */ 
	  0x40u,   /* "-"  middle bar only*/
	  0x76u,   /* "H"*/
    0x38u,   /* "L"*/
    0x3Fu,   /* "O"*/
    0x06u   /* "I" */
};

/* C thresholds: first segment at 30 C, +40 C per segment, last at >350 C.
 * F thresholds: converted from C values, rounded to nearest integer.
 * Gauge is hidden entirely when the displayed temperature is below 30 C / 86 F.
*/
static const uint16_t s_gaugeThreshC[GAUGE_SEG_COUNT] =
{
    30u, 70u, 110u, 150u, 190u, 230u, 270u, 310u, 350u
};

static const uint16_t s_gaugeThreshF[GAUGE_SEG_COUNT] =
{
    86u, 158u, 230u, 302u, 374u, 446u, 518u, 590u, 662u
};
/**
  * @function SCL_Write()
  * ------------
  * @brief    Drive the C8721 SCL pin to the requested level.
  * @param    x - input parameter
  * @note     None
  */
void SCL_Write(uint8_t x)
{
    if (x)
        C8721_SCL_H();
    else
        C8721_SCL_L();
}
/**
  * @function SDA_Write()
  * ------------
  * @brief    Drive the C8721 SDA pin to the requested level.
  * @param    x - input parameter
  * @note     None
  */
void SDA_Write(uint8_t x)
{
    if (x)
        C8721_SDA_H();
    else
        C8721_SDA_L();
}

/**
  * @function Screen_MapToDisplayBuf()
  * ------------------------
  * @brief    Map logical display bits into the C8721 PWM display buffer.
  * @param    None
  * @note     None
  */
static void Screen_MapToDisplayBuf(void)
{
    uint8_t g, s;
    for (s = 0u; s < 8u; s++) {
        for (g = 0u; g < 16u; g++) {
            CF_DisplayBuf[g + s * CF_SEG_NUM] =
                (Screen_Data.DisplayMapTable[g] & (1u << s)) ? CF_LUMI_FULL : CF_LUMI_OFF;
					
        }
    }
}

/**
  * @function Screen_C8721_Init()
  * -------------------
  * @brief    Initialize the C8721 display driver and clear the screen.
  * @param    None
  * @note     None
  */
void Screen_C8721_Init(void)
{
    CF_Init();
    CF_DisplayClearBuf();
    CF_DisplayBufAutomatic();
}

/**
  * @function ShowTemp3Digits()
  * -----------------
  * @brief    Write a signed temperature value to the three display digits.
  * @param    displayTemp - input parameter
  * @note     None
  */
static void ShowTemp3Digits(int16_t displayTemp)
{
    uint8_t hundred, ten, low;
    uint16_t absTemp;

    if (displayTemp < 0)
    {
        absTemp = (uint16_t)(-displayTemp);
        if (absTemp > 99u) { absTemp = 99u; }

        ten = (uint8_t)((absTemp % 100u) / 10u);
        low = (uint8_t)(absTemp % 10u);

        Screen_Data.DisplayMap.SearHundred0 = DigitalTubeDisplayTable[11]; /* '-' */
        Screen_Data.DisplayMap.SearHundred1 = DigitalTubeDisplayTable[11];
        Screen_Data.DisplayMap.SearTen0     = DigitalTubeDisplayTable[ten];
        Screen_Data.DisplayMap.SearTen1     = DigitalTubeDisplayTable[ten];
        Screen_Data.DisplayMap.SearLow0     = DigitalTubeDisplayTable[low];
        Screen_Data.DisplayMap.SearLow1     = DigitalTubeDisplayTable[low];
        return;
    }

    if (displayTemp > 999) { displayTemp = 999; }

    hundred = (uint8_t)((uint16_t)displayTemp / 100u);
    ten     = (uint8_t)(((uint16_t)displayTemp % 100u) / 10u);
    low     = (uint8_t)((uint16_t)displayTemp % 10u);

    Screen_Data.DisplayMap.SearHundred0 = DigitalTubeDisplayTable[hundred];
    Screen_Data.DisplayMap.SearHundred1 = DigitalTubeDisplayTable[hundred];
    Screen_Data.DisplayMap.SearTen0     = DigitalTubeDisplayTable[ten];
    Screen_Data.DisplayMap.SearTen1     = DigitalTubeDisplayTable[ten];
    Screen_Data.DisplayMap.SearLow0     = DigitalTubeDisplayTable[low];
    Screen_Data.DisplayMap.SearLow1     = DigitalTubeDisplayTable[low];
}

/**
  * @function ShowSpecialChars()
  * ------------------
  * @brief    Write raw segment codes to the three display digits.
  * @param    charH - input parameter
  * @param    charT - input parameter
  * @param    charL - input parameter
  * @note     None
  */
static void ShowSpecialChars(uint8 charH, uint8 charT, uint8 charL)
{
    /* Mask to 7 bits: bit 7 belongs to the icon field, not the digit */
    Screen_Data.DisplayMap.SearHundred0 = charH & 0x7Fu;
    Screen_Data.DisplayMap.SearHundred1 = charH & 0x7Fu;
    Screen_Data.DisplayMap.SearTen0     = charT & 0x7Fu;
    Screen_Data.DisplayMap.SearTen1     = charT & 0x7Fu;
    Screen_Data.DisplayMap.SearLow0     = charL & 0x7Fu;
    Screen_Data.DisplayMap.SearLow1     = charL & 0x7Fu;
}



/**
  * @function ApplyUnitIcons()
  * ----------------
  * @brief    Apply Celsius or Fahrenheit unit icons.
  * @param    None
  * @note     None
  */
static void ApplyUnitIcons(void)
{
    if(system_data.units == unitC)
    {
        Screen_Data.DisplayMap.UnitCIcon = 1u;   /* [9]  C on  */
        Screen_Data.DisplayMap.UnitFIcon = 0u;   /* [10] F off */
    }
    else
    {
        Screen_Data.DisplayMap.UnitCIcon = 0u;   /* [9]  C off */
        Screen_Data.DisplayMap.UnitFIcon = 1u;   /* [10] F on  */
    }
}

/**
  * @function ClearTempSourceIcons()
  * ----------------------
  * @brief    Clear all temperature source icons.
  * @param    None
  * @note     None
  */
static void ClearTempSourceIcons(void)
{
    Screen_Data.DisplayMap.PIcon       = 0u;  /* [6]  Probe icon          */
    Screen_Data.DisplayMap.OIcon       = 0u;  /* [7]  O surface indicator */
    Screen_Data.DisplayMap.DIcon       = 0u;  /* [8]  D surface indicator */
    Screen_Data.DisplayMap.CavityIcon1 = 0u;  /* [13] Cavity indicator    */
    Screen_Data.DisplayMap.CavityIcon2 = 0u;
    Screen_Data.DisplayMap.CavityIcon3 = 0u;
}

/**
  * @function ApplySourceIcon()
  * -----------------
  * @brief    Apply the source icon for the current display mode.
  * @param    None
  * @note     None
  */
static void ApplySourceIcon(void)
{
    ClearTempSourceIcons();

    switch(DisplayMode)
    {
        case DISPLAY_MODE_D_SURFACE:
            Screen_Data.DisplayMap.DIcon = 1u;                              /* [8]  */
            break;

        case DISPLAY_MODE_O_SURFACE:
            Screen_Data.DisplayMap.OIcon = 1u;                              /* [7]  */
            break;

        case DISPLAY_MODE_PROBE:
            Screen_Data.DisplayMap.PIcon = 1u;                              /* [6]  */
            break;

        case DISPLAY_MODE_CAVITY:
            Screen_Data.DisplayMap.CavityIcon1 = 1u;                        /* [13] */
            Screen_Data.DisplayMap.CavityIcon2 = (uint8_t)GAUGE_SEG_ON;
            Screen_Data.DisplayMap.CavityIcon3 = (uint8_t)GAUGE_SEG_ON;
            break;

        default:
            Screen_Data.DisplayMap.DIcon = 1u;
            break;
    }
}

/**
  * @function UpdateGauge()
  * -------------
  * @brief    Update the temperature gauge line according to the display temperature.
  * @param    displayTemp - input parameter
  * @note     None
  */
static void UpdateGauge(uint16_t displayTemp)
{
    const uint16_t *thresh;
    uint8_t segOn = 0u;
    uint8_t i;

    thresh = (system_data.units ==unitC) ? s_gaugeThreshC : s_gaugeThreshF;

    /* Gauge is hidden when temperature is below the first threshold (30 C / 86 F) */
    if(displayTemp >= thresh[0])
    {
        for(i = 0u; i < GAUGE_SEG_COUNT; i++)
        {
            if(displayTemp >= thresh[i])
            {
                segOn = i + 1u;
            }
        }
    }

    /* Write each 2-bit gauge segment field */
    Screen_Data.DisplayMap.TempA = (segOn >= 1u) ? GAUGE_SEG_ON : GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempB = (segOn >= 2u) ? GAUGE_SEG_ON : GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempC = (segOn >= 3u) ? GAUGE_SEG_ON : GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempD = (segOn >= 4u) ? GAUGE_SEG_ON : GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempE = (segOn >= 5u) ? GAUGE_SEG_ON : GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempF = (segOn >= 6u) ? GAUGE_SEG_ON : GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempG = (segOn >= 7u) ? GAUGE_SEG_ON : GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempH = (segOn >= 8u) ? GAUGE_SEG_ON : GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempI = (segOn >= 9u) ? GAUGE_SEG_ON : GAUGE_SEG_OFF;
}

/**
  * @function ClearGauge()
  * ------------
  * @brief    Turn off all temperature gauge segments.
  * @param    None
  * @note     None
  */
static void ClearGauge(void)
{
    Screen_Data.DisplayMap.TempA = GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempB = GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempC = GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempD = GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempE = GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempF = GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempG = GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempH = GAUGE_SEG_OFF;
    Screen_Data.DisplayMap.TempI = GAUGE_SEG_OFF;
}


/*
 * Refresh display from screen_data.DisplayMapTable.
 * Call periodically (e.g. every 10 ms) from the scheduler .
 */
uint8_t  displaystep=0;
/**
  * @function DisplayTask()
  * -------------
  * @brief    Refresh the display buffer and send it to the C8721 driver.
  * @param    None
  * @note     None
  */
void DisplayTask(void)
{
    int16_t displayTemp;
    uint8_t special;

    if (displaystep == 0u)
    {
        CF_DisplayClearBuf();

        if (work_process != shutdown)
        {
            displayTemp = UI_GetDisplayTemp();
            special = UI_GetDisplaySpecial();

            if (special == 1u) {
                ShowSpecialChars(DigitalTubeDisplayTable[11], DigitalTubeDisplayTable[12], DigitalTubeDisplayTable[15]);
                ClearGauge();
            } else if (special == 2u) {
                ShowSpecialChars(DigitalTubeDisplayTable[11], DigitalTubeDisplayTable[13], DigitalTubeDisplayTable[14]);
                ClearGauge();
            } else if (special == 3u) {
                ShowSpecialChars(DigitalTubeDisplayTable[11], DigitalTubeDisplayTable[11], DigitalTubeDisplayTable[11]);
                ClearGauge();
            } else {
                ShowTemp3Digits(displayTemp);
                UpdateGauge((displayTemp < 0) ? 0u : (uint16_t)displayTemp);
            }

            ApplyUnitIcons();
            ApplySourceIcon();
            if (UI_GetBluetoothIconState() != 0u) {
                Screen_Data.DisplayMap.BlutoothIcon = 1u;
            } else {
                Screen_Data.DisplayMap.BlutoothIcon = 0u;
            }
            if (g_battery_low != 0u) {
                Screen_Data.DisplayMap.BatteryLowIcon = 1u;
            } else {
                Screen_Data.DisplayMap.BatteryLowIcon = 0u;
            }
            Screen_MapToDisplayBuf();
        }
    }

    CF_DisplaySegment(displaystep);
    displaystep++;
    if (displaystep > 9u)
    {
        displaystep = 0u;
    }
}







