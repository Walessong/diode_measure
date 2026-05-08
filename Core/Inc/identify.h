#ifndef IDENTIFY_H
#define IDENTIFY_H

#include "learn.h"

typedef enum
{
    COUNT_OK = 0,
    COUNT_NO_MODEL,
    COUNT_OPEN,
    COUNT_UNSTABLE,
    COUNT_OUT_OF_RANGE,
    COUNT_FAIL
} CountStatus_t;

typedef struct
{
    uint8_t final_count;
    uint8_t count_low;
    uint8_t count_high;
    float err_low;
    float err_high;
    CountStatus_t status;
} CountResult_t;

float Identify_ReconstructError(float total_vf, float unit_vf, uint8_t count);
CountStatus_t Identify_CountSameType(const DiodeModel_t *model, CountResult_t *result);

#endif
