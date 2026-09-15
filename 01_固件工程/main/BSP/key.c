/**
  ******************************************************************************
  * @file    key.c
  * @author 
  * @version V1.0
  * @date
  * @brief   按键扫描产生事件，由 MainControl 消费并执行显示、单位和蓝牙操作。
  ******************************************************************************
  */
#include "config.h"

_KeyState KeyState[KeyNumber];
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
/* 低电平按下；计数单位为扫描次数。待机时 KEY0 去抖后在按下阶段触发，其余短按通常在松开时产生事件。 */
void Key_Scan(void)
{
    static uint16_t time_count[KeyNumber];
    static uint8_t quick_sent_on_down[KeyNumber];
    static uint8_t key0_shutdown_sent = 0u;
    static uint8_t key0_press_started_idle = 0u;
    uint8_t key_now[KeyNumber];

    key_now[0] = key0;
    key_now[1] = key1;

    for (uint8_t i = 0; i < KeyNumber; i++)
    {
        if (key_now[i] == 0)
        {
            time_count[i]++;
            if ((i == 0u) && (time_count[i] == 1u)) {
                key0_press_started_idle = (work_process == idle) ? 1u : 0u;
            }
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
                if (!((i == 0u) && (key0_press_started_idle != 0u))) {
                    KeyState[i] = LongPress;
                }
                if ((i == 0u) && (key0_shutdown_sent == 0u) && (key0_press_started_idle == 0u))
                {
                    /* 非待机起按的 KEY0 达到 100 次扫描即请求关机，不等松手。 */
                    work_process = shutdown;
                    UI_NotifyLocalInteraction();
                    key0_shutdown_sent = 1u;
                }
            } else if ((time_count[i] == 1000u) && (i == 1u)) {
                KeyState[i] = VeryLongPress;
            }
            if (time_count[i] > 1000u) {
                time_count[i] = 1000u;
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
                key0_press_started_idle = 0u;
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
            if ((IsDisplayDashOnlyAlarm() == 0u) || (UI_IsProbeConnected() != 0u)) {
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
            (void)UI_SetUnits((system_data.units == unitC) ? unitF : unitC, 1u);
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
    /* KEY0 长按事件请求关机；扫描层负责过滤待机起按的例外。 */
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
            /* 蓝牙长按请求配对或关电；已连接时此请求不生效，由超长按处理。 */
            UI_RequestBluetoothToggle();
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
/* KEY1 达到 1000 次扫描的超长按，可关闭已连接的蓝牙。 */
void Key1_very_long_press(void)
{
    if ((work_process != idle) && (work_process != shutdown) &&
        (UI_GetBluetoothState() == BT_CONNECTED)) {
        UI_RequestBluetoothPowerOff();
        UI_NotifyLocalInteraction();
    }
}
