#ifndef BSP_KEY_H
#define BSP_KEY_H

#include "main.h"

typedef enum
{
    KEY_EVENT_NONE = 0,
    KEY_EVENT_MODE,
    KEY_EVENT_LEARN,
    KEY_EVENT_MEAS
} KeyEvent_t;

void Key_Init(void);
void Key_Task1ms(void);
KeyEvent_t Key_GetEvent(void);

#endif
