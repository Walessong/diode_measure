#ifndef BSP_OLED_H
#define BSP_OLED_H

#include "main.h"

void OLED_AppInit(void);
void OLED_ClearAll(void);
void OLED_ShowStringLine(uint8_t line, const char *str);
void OLED_ShowStringRow16(uint8_t row, const char *str);
void OLED_ShowMenu(uint8_t index);
void OLED_ShowMeasureResult(const char *title, const char *value);

#endif
