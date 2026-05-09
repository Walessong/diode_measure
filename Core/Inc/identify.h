#ifndef IDENTIFY_H
#define IDENTIFY_H

#include "learn.h"

typedef enum
{
    FAULT_NONE = 0,
    FAULT_OPEN,
    FAULT_SERIES_CAP,
    FAULT_SERIES_RES,
    FAULT_UNSTABLE
} FaultType_t;

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
    uint8_t has_fault;
    FaultType_t fault_type;
    CountStatus_t status;
} CountResult_t;

typedef enum
{
    COUNT_ABC_OK = 0,
    COUNT_ABC_NO_MODEL,
    COUNT_ABC_OPEN,
    COUNT_ABC_UNSTABLE,
    COUNT_ABC_OUT_OF_RANGE,
    COUNT_ABC_FAIL
} CountAbcStatus_t;

typedef struct
{
    uint8_t count_a;
    uint8_t count_b;
    uint8_t count_c;
    uint8_t total_count;
    float fit_error;
    float fit_error_low;
    float fit_error_high;
    float fit_error_delta;
    uint8_t votes;
    CountAbcStatus_t status;
} CountAbcResult_t;

float Identify_ReconstructError(float total_vf, float unit_vf, uint8_t count);
FaultType_t Identify_DetectFaultSameType(const DiodeModel_t *model,
                                         const FaultMeasureResult_t *fault_sample);
CountStatus_t Identify_CountSameType(const DiodeModel_t *model, CountResult_t *result);
CountAbcStatus_t Identify_CountABC(const DiodeModel_t *model_a,
                                   const DiodeModel_t *model_b,
                                   const DiodeModel_t *model_c,
                                   CountAbcResult_t *result);

#endif
