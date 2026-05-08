#ifndef BSP_RELAY_H
#define BSP_RELAY_H

#include "main.h"

typedef enum
{
    DIR_0 = 0,
    DIR_1
} RelayDir_t;

typedef enum
{
    RANGE_LOW = 0,
    RANGE_HIGH
} CurrentRange_t;

void Relay_InitSafe(void);
void Relay_SetDirection(RelayDir_t dir);
void Relay_SetRange(CurrentRange_t range);
void Relay_EnableCurrent(uint8_t en);
RelayDir_t Relay_GetDirection(void);
CurrentRange_t Relay_GetRange(void);
uint8_t Relay_IsCurrentEnabled(void);

#endif
