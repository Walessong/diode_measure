#include "bsp_relay.h"

#include "app_config.h"

static RelayDir_t g_dir = (RelayDir_t)0xFFU;
static CurrentRange_t g_range = (CurrentRange_t)0xFFU;
static uint8_t g_current_enabled = 0xFFU;

static void Relay_WritePin(GPIO_TypeDef *port, uint16_t pin, uint8_t active)
{
    HAL_GPIO_WritePin(port, pin, active ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL);
}

void Relay_InitSafe(void)
{
    g_dir = (RelayDir_t)0xFFU;
    g_range = (CurrentRange_t)0xFFU;
    g_current_enabled = 0xFFU;
    Relay_SetDirection(DIR_0);
    Relay_SetRange(RANGE_LOW);
    Relay_EnableCurrent(0U);
}

void Relay_SetDirection(RelayDir_t dir)
{
    if (g_dir == dir)
    {
        return;
    }

    g_dir = dir;
    if (dir == DIR_0)
    {
        Relay_WritePin(REL_POL_A_GPIO_Port, REL_POL_A_Pin, 0U);
        Relay_WritePin(REL_POL_B_GPIO_Port, REL_POL_B_Pin, 0U);
    }
    else
    {
        Relay_WritePin(REL_POL_A_GPIO_Port, REL_POL_A_Pin, 1U);
        Relay_WritePin(REL_POL_B_GPIO_Port, REL_POL_B_Pin, 1U);
    }
}

void Relay_SetRange(CurrentRange_t range)
{
    if (g_range == range)
    {
        return;
    }

    g_range = range;
    Relay_WritePin(REL_RANGE_GPIO_Port, REL_RANGE_Pin, (range == RANGE_HIGH) ? 1U : 0U);
}

void Relay_EnableCurrent(uint8_t en)
{
    uint8_t next_state = (en != 0U) ? 1U : 0U;

    if (g_current_enabled == next_state)
    {
        return;
    }

    g_current_enabled = next_state;
    Relay_WritePin(I_EN_GPIO_Port, I_EN_Pin, g_current_enabled);
}

RelayDir_t Relay_GetDirection(void)
{
    return g_dir;
}

CurrentRange_t Relay_GetRange(void)
{
    return g_range;
}

uint8_t Relay_IsCurrentEnabled(void)
{
    return g_current_enabled;
}
