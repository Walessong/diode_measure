#ifndef APP_H
#define APP_H

#include "main.h"

typedef enum
{
    APP_MODE_MENU = 0,
    APP_MODE_BASIC_POL,
    APP_MODE_BASIC_VF,
    APP_MODE_LEARN_SINGLE,
    APP_MODE_COUNT_SINGLE,
    APP_MODE_DIAG_INFO
} AppMode_t;

void App_Init(void);
void App_Task(void);
void App_Tick1ms(void);

#endif
