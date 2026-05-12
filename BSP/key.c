#include "config.h"

_KeyState KeyState[KeyNumber];

static int16_t ConvertTempToUnit(int16_t value, uint8_t toUnit)
{
    int32_t t = (int32_t)value;

    if (toUnit == unitF) {
        t = (t * 9) / 5 + 32;
    } else {
        t = (t - 32) * 5 / 9;
    }

    if (t > 999) { t = 999; }
    if (t < -200) { t = -200; }
    return (int16_t)t;
}

static void SyncTempsAfterUnitSwitch(uint8_t toUnit)
{
    uint8_t i;

    for (i = 0u; i < 9u; i++)
    {
        system_data.pt1000_temp[i] = ConvertTempToUnit(system_data.pt1000_temp[i], toUnit);
    }
}

static uint8_t IsDisplayDashOnlyAlarm(void)
{
    uint8_t inv0, inv1, inv2;
    inv0 = (uint8_t)(system_data.pt1000_temp[0] == HaveTempErr);
    inv1 = (uint8_t)(system_data.pt1000_temp[1] == HaveTempErr);
    inv2 = (uint8_t)(system_data.pt1000_temp[2] == HaveTempErr);
    return (uint8_t)(inv0 && inv1 && inv2);
}

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

void Key_Respose_Nothing(void) {}

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
            if ((IsDisplayDashOnlyAlarm() == 0u) && (g_probe_connected != 0u)) {
                UI_CycleDisplayMode();
            }
            UI_NotifyLocalInteraction();
            break;
    }
}

void Key1_short_press(void)
{
    switch (work_process)
    {
        case idle:
        case shutdown:
            break;

        default:
            if ((IsDisplayDashOnlyAlarm() != 0u) || (UI_GetDisplaySpecial() == 3u)) {
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

void Key0_long_press(void)
{
    /* KEY0 >=1s: force shutdown from any state (no release required). */
    work_process = shutdown;
    UI_NotifyLocalInteraction();
}

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

void Key0_very_long_press(void)
{
    Key0_long_press();
}

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
