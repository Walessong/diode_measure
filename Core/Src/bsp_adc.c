#include "bsp_adc.h"

#include "app_config.h"

extern ADC_HandleTypeDef hadc1;

static void BSP_ADC_SelectChannel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

uint16_t BSP_ADC_ReadRaw(uint32_t channel)
{
    BSP_ADC_SelectChannel(channel);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 20U);
    return (uint16_t)HAL_ADC_GetValue(&hadc1);
}

uint16_t BSP_ADC_ReadAvg(uint32_t channel, uint8_t times)
{
    uint32_t sum = 0U;
    uint8_t i;

    if (times == 0U)
    {
        times = 1U;
    }

    for (i = 0U; i < times; i++)
    {
        sum += BSP_ADC_ReadRaw(channel);
    }

    return (uint16_t)(sum / times);
}

float BSP_ADC_ReadVoltage(uint32_t channel)
{
    uint16_t raw = BSP_ADC_ReadAvg(channel, ADC_AVG_TIMES);
    return ((float)raw * ADC_VREF) / ADC_FULL_SCALE;
}

float BSP_ADC_ReadVin(void)
{
    return BSP_ADC_ReadVoltage(ADC_CHANNEL_4) * ADC_INPUT_SCALE;
}

float BSP_ADC_ReadVout(void)
{
    return BSP_ADC_ReadVoltage(ADC_CHANNEL_5) * ADC_INPUT_SCALE;
}

float BSP_ADC_ReadISense(void)
{
    return BSP_ADC_ReadVoltage(ADC_CHANNEL_6);
}
