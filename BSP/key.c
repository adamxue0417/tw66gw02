/**
  ******************************************************************************
  * @file    key.c
  * @author 
  * @version V1.0
  * @date
  * @brief   Key scan, key event dispatch and key action implementation.
  ******************************************************************************
  */
#include "config.h"

_KeyState KeyState[KeyNumber];
/**
  * @function ConvertTempToUnit()
  * -------------------
  * @brief    Convert a temperature value between Celsius and Fahrenheit units.
  * @param    value - input parameter
  * @param    toUnit - input parameter
  * @note     None
  */
static int16_t ConvertTempToUnit(int16_t value, uint8_t toUnit)
{
    int32_t t = (int32_t)value;

    if ((value == HaveTempErr) || (value == TempDisconnected) || (value == TempHigh) || (value == TempLow)) {
        return value;
    }

    if (toUnit == unitF) {
        t = (t * 9) / 5 + 32;
    } else {
        t = (t - 32) * 5 / 9;
    }

    if (t > 999) { t = 999; }
    if (t < -200) { t = -200; }
    return (int16_t)t;
}
/**
  * @function SyncTempsAfterUnitSwitch()
  * --------------------------
  * @brief    Synchronize cached temperatures after a unit switch.
  * @param    toUnit - input parameter
  * @note     None
  */
static void SyncTempsAfterUnitSwitch(uint8_t toUnit)
{
    uint8_t i;

    for (i = 0u; i < 9u; i++)
    {
        system_data.pt1000_temp[i] = ConvertTempToUnit(system_data.pt1000_temp[i], toUnit);
    }
}
/**
  * @function IsDisplayDashOnlyAlarm()
  * ------------------------
  * @brief    Check whether the main display should keep dash-only alarm state.
  * @param    None
  * @note     None
  */
static uint8_t IsDisplayDashOnlyAlarm(void)
{
    uint8_t inv0, inv1, inv2;
    inv0 = (uint8_t)((system_data.pt1000_temp[0] == HaveTempErr) ||
                     (system_data.pt1000_temp[0] == TempDisconnected));
    inv1 = (uint8_t)((system_data.pt1000_temp[1] == HaveTempErr) ||
                     (system_data.pt1000_temp[1] == TempDisconnected));
    inv2 = (uint8_t)((system_data.pt1000_temp[2] == HaveTempErr) ||
                     (system_data.pt1000_temp[2] == TempDisconnected));
    return (uint8_t)(inv0 && inv1 && inv2);
}
/**
  * @function Key_Scan()
  * ------------
  * @brief    Scan keys and update key press states.
  * @param    None
  * @note     None
  */
void Key_Scan(void)
{
    static uint16_t time_count[KeyNumber];
    static uint8_t quick_sent_on_down[KeyNumber];
    static uint8_t key0_shutdown_sent = 0u;
    uint8_t key_now[KeyNumber];

    key_now[0] = key0;
    key_now[1] = key1;

    for (uint8_t i = 0; i < KeyNumber; i++)
    {
        if (key_now[i] == 0)
        {
            time_count[i]++;
            /* Power-on requirement: in idle state, KEY0 triggers on press (no release needed). */
            if ((i == 0u) &&
                (work_process == idle) &&
                (time_count[i] == 2u) &&
                (quick_sent_on_down[i] == 0u))
            {
                KeyState[i] = QuickPress;
                quick_sent_on_down[i] = 1u;
            }
            if (time_count[i] == 100u) {
                KeyState[i] = LongPress;
                if ((i == 0u) && (key0_shutdown_sent == 0u))
                {
                    /* Force shutdown immediately at >=1s while still pressed. */
                    work_process = shutdown;
                    UI_NotifyLocalInteraction();
                    key0_shutdown_sent = 1u;
                }
            } else if (time_count[i] == 300u) {
                KeyState[i] = VeryLongPress;
            }
            if (time_count[i] > 300u) {
                time_count[i] = 300u;
            }
        }
        else
        {
            if ((time_count[i] >= 2u) && (time_count[i] < 100u) && (quick_sent_on_down[i] == 0u)) {
                KeyState[i] = QuickPress;
            }
            time_count[i] = 0u;
            quick_sent_on_down[i] = 0u;
            if (i == 0u) {
                key0_shutdown_sent = 0u;
            }
        }
    }
}
/**
  * @function KeyRespose()
  * ------------
  * @brief    Dispatch key events to the configured callback functions.
  * @param    Key0ShortPress - input parameter
  * @param    Key1ShortPress - input parameter
  * @param    Key0LongPress - input parameter
  * @param    Key1LongPress - input parameter
  * @param    Key0VeryLongPress - input parameter
  * @param    Key1VeryLongPress - input parameter
  * @note     None
  */
void KeyRespose(void(*Key0ShortPress)(),
                void(*Key1ShortPress)(),
                void(*Key0LongPress)(),
                void(*Key1LongPress)(),
                void(*Key0VeryLongPress)(),
                void(*Key1VeryLongPress)())
{
    for (uint8_t i = 0; i < KeyNumber; i++)
    {
        switch (KeyState[i])
        {
            case QuickPress:
                if (i == 0u) { (*Key0ShortPress)(); }
                else { (*Key1ShortPress)(); }
                KeyState[i] = Idle;
                break;

            case LongPress:
                if (i == 0u) { (*Key0LongPress)(); }
                else { (*Key1LongPress)(); }
                KeyState[i] = Idle;
                break;

            case VeryLongPress:
                if (i == 0u) { (*Key0VeryLongPress)(); }
                else { (*Key1VeryLongPress)(); }
                KeyState[i] = Idle;
                break;

            default:
                break;
        }
    }
}
/**
  * @function Key_Respose_Nothing()
  * ---------------------
  * @brief    Provide an empty key response callback.
  * @param    None
  * @note     None
  */
void Key_Respose_Nothing(void) {}
/**
  * @function Key0_short_press()
  * ------------------
  * @brief    Handle KEY0 short press action.
  * @param    None
  * @note     None
  */
void Key0_short_press(void)
{
    switch (work_process)
    {
        case idle:
            work_process = start_up;
            UI_NotifyLocalInteraction();
            break;

        case shutdown:
            break;

        default:
            if (IsDisplayDashOnlyAlarm() == 0u) {
                UI_CycleDisplayMode();
            }
            UI_NotifyLocalInteraction();
            break;
    }
}
/**
  * @function Key1_short_press()
  * ------------------
  * @brief    Handle KEY1 short press action.
  * @param    None
  * @note     None
  */
void Key1_short_press(void)
{
    switch (work_process)
    {
        case idle:
        case shutdown:
            break;

        default:
            if (((IsDisplayDashOnlyAlarm() != 0u) || (UI_GetDisplaySpecial() == 3u)) &&
                (UI_IsProbeConnected() == 0u)) {
                UI_NotifyLocalInteraction();
                break;
            }
            if (system_data.units == unitC) {
                system_data.units = unitF;
                SyncTempsAfterUnitSwitch(unitF);
            } else {
                system_data.units = unitC;
                SyncTempsAfterUnitSwitch(unitC);
            }
            UI_NotifyLocalInteraction();
            break;
    }
}
/**
  * @function Key0_long_press()
  * -----------------
  * @brief    Handle KEY0 long press action.
  * @param    None
  * @note     None
  */
void Key0_long_press(void)
{
    /* KEY0 >=1s: force shutdown from any state (no release required). */
    work_process = shutdown;
    UI_NotifyLocalInteraction();
}
/**
  * @function Key1_long_press()
  * -----------------
  * @brief    Handle KEY1 long press action.
  * @param    None
  * @note     None
  */
void Key1_long_press(void)
{
    switch (work_process)
    {
        case idle:
        case shutdown:
            break;

        default:
            /* Bluetooth button >=1s: enter pairing (triggered while pressed). */
            UI_RequestBluetoothPairing();
            UI_NotifyLocalInteraction();
            break;
    }
}
/**
  * @function Key0_very_long_press()
  * ----------------------
  * @brief    Handle KEY0 very long press action.
  * @param    None
  * @note     None
  */
void Key0_very_long_press(void)
{
    Key0_long_press();
}
/**
  * @function Key1_very_long_press()
  * ----------------------
  * @brief    Handle KEY1 very long press action.
  * @param    None
  * @note     None
  */
void Key1_very_long_press(void)
{
    switch (work_process)
    {
        case idle:
        case shutdown:
            break;

        default:
            /* Bluetooth button >=3s: power off BT (triggered while pressed). */
            UI_RequestBluetoothPowerOff();
            UI_NotifyLocalInteraction();
            break;
    }
}
