#ifndef BSP_ADC_H
#define BSP_ADC_H

#include "main.h"

uint16_t BSP_ADC_ReadRaw(uint32_t channel);
uint16_t BSP_ADC_ReadAvg(uint32_t channel, uint8_t times);
float BSP_ADC_ReadVoltage(uint32_t channel);
float BSP_ADC_ReadVin(void);
float BSP_ADC_ReadVout(void);
float BSP_ADC_ReadISense(void);

#endif
