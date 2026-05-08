#include "bsp_key.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t stable_level;
    uint8_t debounce_count;
    uint8_t press_reported;
} KeyContext_t;

static KeyContext_t g_keys[3];
static volatile KeyEvent_t g_key_event = KEY_EVENT_NONE;

void Key_Init(void)
{
    g_keys[0].port = KEY_MODE_GPIO_Port;
    g_keys[0].pin = KEY_MODE_Pin;
    g_keys[1].port = KEY_LEARN_GPIO_Port;
    g_keys[1].pin = KEY_LEARN_Pin;
    g_keys[2].port = KEY_MEAS_GPIO_Port;
    g_keys[2].pin = KEY_MEAS_Pin;
    g_keys[0].stable_level = (uint8_t)HAL_GPIO_ReadPin(g_keys[0].port, g_keys[0].pin);
    g_keys[1].stable_level = (uint8_t)HAL_GPIO_ReadPin(g_keys[1].port, g_keys[1].pin);
    g_keys[2].stable_level = (uint8_t)HAL_GPIO_ReadPin(g_keys[2].port, g_keys[2].pin);
    g_key_event = KEY_EVENT_NONE;
}

void Key_Task1ms(void)
{
    uint8_t i;

    for (i = 0U; i < 3U; i++)
    {
        uint8_t level = (uint8_t)HAL_GPIO_ReadPin(g_keys[i].port, g_keys[i].pin);
        if (level == g_keys[i].stable_level)
        {
            g_keys[i].debounce_count = 0U;
        }
        else
        {
            g_keys[i].debounce_count++;
            if (g_keys[i].debounce_count >= 20U)
            {
                g_keys[i].stable_level = level;
                g_keys[i].debounce_count = 0U;
                if (level == GPIO_PIN_RESET)
                {
                    g_keys[i].press_reported = 1U;
                }
                else if (g_keys[i].press_reported != 0U)
                {
                    g_keys[i].press_reported = 0U;
                    g_key_event = (KeyEvent_t)(i + 1U);
                }
            }
        }
    }
}

KeyEvent_t Key_GetEvent(void)
{
    KeyEvent_t event = g_key_event;
    g_key_event = KEY_EVENT_NONE;
    return event;
}
