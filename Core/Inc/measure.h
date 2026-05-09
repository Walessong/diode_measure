#ifndef MEASURE_H
#define MEASURE_H

#include "bsp_relay.h"

typedef struct
{
    float vin;
    float vout;
    float vsense;
    float vdut;
    float vdut_corr;
    uint8_t dir;
    uint8_t range;
} SamplePoint_t;

typedef enum
{
    MEASURE_QUALITY_OPEN = 0,
    MEASURE_QUALITY_WEAK,
    MEASURE_QUALITY_VALID
} MeasureQuality_t;

typedef enum
{
    DISPLAY_STATE_OPEN = 0,
    DISPLAY_STATE_FORWARD_ON,
    DISPLAY_STATE_REVERSE_OFF
} DisplayState_t;

typedef struct
{
    SamplePoint_t dir0_low;
    SamplePoint_t dir1_low;
    SamplePoint_t dir0_high;
    SamplePoint_t dir1_high;
    uint8_t dir0_forward;
    RelayDir_t forward_dir;
    MeasureQuality_t quality;
    DisplayState_t display_state;
    uint8_t valid;
    float vf;
    float vf_low_corrected;
    float vf_high_corrected;
} SingleMeasureResult_t;

typedef struct
{
    uint8_t valid;
    RelayDir_t forward_dir;
    DisplayState_t display_state;
    SamplePoint_t low_pt;
    SamplePoint_t high_pt;
    float step_early_vsense;
    float step_mid_vsense;
    float step_late_vsense;
    float step_early_vdut;
    float step_mid_vdut;
    float step_late_vdut;
} FaultMeasureResult_t;

void Measure_Init(void);
void Measure_GetPoint(RelayDir_t dir, CurrentRange_t range, SamplePoint_t *pt);
void Measure_ScanSingle(SingleMeasureResult_t *result);
void Measure_ScanSingleDetailed(SingleMeasureResult_t *result);
uint8_t Measure_GetForwardLowOnly(SamplePoint_t *low_pt, RelayDir_t *forward_dir, DisplayState_t *display_state);
uint8_t Measure_GetForwardTotal(SamplePoint_t *low_pt, SamplePoint_t *high_pt, RelayDir_t *forward_dir, DisplayState_t *display_state);
uint8_t Measure_GetFaultSample(FaultMeasureResult_t *result);
uint8_t Measure_DetectPolarity(void);
float Measure_SingleVf(void);

#endif
