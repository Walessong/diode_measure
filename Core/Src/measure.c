#include "measure.h"

#include <math.h>
#include <string.h>

#include "app_config.h"
#include "bsp_adc.h"

#define MEASURE_EXTRA_SETTLE_MS 12U
#define MEASURE_SAMPLE_GAP_MS   2U

static void Measure_ZeroPoint(SamplePoint_t *pt)
{
    memset(pt, 0, sizeof(*pt));
}

static void Measure_AddPoint(SamplePoint_t *acc, const SamplePoint_t *pt)
{
    acc->vin += pt->vin;
    acc->vout += pt->vout;
    acc->vsense += pt->vsense;
    acc->vdut += pt->vdut;
    acc->vdut_corr += pt->vdut_corr;
    acc->dir = pt->dir;
    acc->range = pt->range;
}

static void Measure_DivPoint(SamplePoint_t *pt, float div)
{
    pt->vin /= div;
    pt->vout /= div;
    pt->vsense /= div;
    pt->vdut /= div;
    pt->vdut_corr /= div;
}

static uint8_t Measure_IsPointWeak(const SamplePoint_t *pt)
{
    if ((pt->vdut_corr >= DIODE_PRESENT_MIN_V) || (pt->vsense >= WEAK_SENSE_MIN_V))
    {
        return 1U;
    }

    return 0U;
}

static float Measure_GetFixedDrop(RelayDir_t dir, CurrentRange_t range)
{
    float drop = (range == RANGE_LOW) ? FIXED_DROP_LOW_V : FIXED_DROP_HIGH_V;

    if (dir == DIR_1)
    {
        drop += (range == RANGE_LOW) ? FIXED_DROP_DIR1_LOW_EXTRA_V
                                     : FIXED_DROP_DIR1_HIGH_EXTRA_V;
    }

    return drop;
}

static uint8_t Measure_IsPointValid(const SamplePoint_t *pt)
{
    if ((pt->vsense >= SENSE_VALID_MIN_V) &&
        (pt->vdut_corr >= VF_VALID_MIN) &&
        (pt->vdut_corr <= VF_VALID_MAX))
    {
        return 1U;
    }

    return 0U;
}

static void Measure_FillPoint(SamplePoint_t *pt, RelayDir_t dir, CurrentRange_t range)
{
    memset(pt, 0, sizeof(*pt));
    pt->dir = (uint8_t)dir;
    pt->range = (uint8_t)range;
    pt->vin = BSP_ADC_ReadVin();
    pt->vout = BSP_ADC_ReadVout();
    pt->vsense = BSP_ADC_ReadISense();
    pt->vdut = fabsf(pt->vin - pt->vout);
    pt->vdut_corr = pt->vdut - Measure_GetFixedDrop(dir, range);
    if (pt->vdut_corr < 0.0f)
    {
        pt->vdut_corr = 0.0f;
    }
}

static void Measure_AveragePoint(RelayDir_t dir, CurrentRange_t range, SamplePoint_t *pt)
{
    uint8_t i;
    SamplePoint_t temp;

    memset(pt, 0, sizeof(*pt));
    for (i = 0U; i < MEASURE_REPEAT_TIMES; i++)
    {
        Measure_FillPoint(&temp, dir, range);
        pt->vin += temp.vin;
        pt->vout += temp.vout;
        pt->vsense += temp.vsense;
        pt->vdut += temp.vdut;
        pt->vdut_corr += temp.vdut_corr;
        if (i + 1U < MEASURE_REPEAT_TIMES)
        {
            HAL_Delay(MEASURE_SAMPLE_GAP_MS);
        }
    }

    pt->vin /= (float)MEASURE_REPEAT_TIMES;
    pt->vout /= (float)MEASURE_REPEAT_TIMES;
    pt->vsense /= (float)MEASURE_REPEAT_TIMES;
    pt->vdut /= (float)MEASURE_REPEAT_TIMES;
    pt->vdut_corr /= (float)MEASURE_REPEAT_TIMES;
    pt->dir = (uint8_t)dir;
    pt->range = (uint8_t)range;
}

static float Measure_ReadAverageISense(void)
{
    uint8_t i;
    float sum = 0.0f;

    for (i = 0U; i < MEASURE_REPEAT_TIMES; i++)
    {
        sum += BSP_ADC_ReadISense();
        if (i + 1U < MEASURE_REPEAT_TIMES)
        {
            HAL_Delay(MEASURE_SAMPLE_GAP_MS);
        }
    }

    return sum / (float)MEASURE_REPEAT_TIMES;
}

static float Measure_ReadAverageVdut(void)
{
    uint8_t i;
    float sum = 0.0f;

    for (i = 0U; i < MEASURE_REPEAT_TIMES; i++)
    {
        float vin = BSP_ADC_ReadVin();
        float vout = BSP_ADC_ReadVout();
        sum += fabsf(vin - vout);
        if (i + 1U < MEASURE_REPEAT_TIMES)
        {
            HAL_Delay(MEASURE_SAMPLE_GAP_MS);
        }
    }

    return sum / (float)MEASURE_REPEAT_TIMES;
}

static uint8_t Measure_SelectForwardDirWeak(const SingleMeasureResult_t *scan, RelayDir_t *dir_out)
{
    float score0 = scan->dir0_low.vsense + (0.10f * scan->dir0_low.vdut_corr);
    float score1 = scan->dir1_low.vsense + (0.10f * scan->dir1_low.vdut_corr);
    uint8_t weak0 = Measure_IsPointWeak(&scan->dir0_low);
    uint8_t weak1 = Measure_IsPointWeak(&scan->dir1_low);

    if ((weak0 == 0U) && (weak1 == 0U))
    {
        return 0U;
    }

    *dir_out = (score0 >= score1) ? DIR_0 : DIR_1;
    return 1U;
}

static uint8_t Measure_EvaluatePolarity(const SamplePoint_t *pt0, const SamplePoint_t *pt1)
{
    float score0;
    float score1;
    uint8_t valid0 = Measure_IsPointValid(pt0);
    uint8_t valid1 = Measure_IsPointValid(pt1);

    score0 = pt0->vsense - (0.05f * pt0->vdut_corr);
    score1 = pt1->vsense - (0.05f * pt1->vdut_corr);

    if ((valid0 == 0U) && (valid1 == 0U))
    {
        return 0xFFU;
    }
    if ((valid0 != 0U) && (valid1 == 0U))
    {
        return 1U;
    }
    if ((valid0 == 0U) && (valid1 != 0U))
    {
        return 0U;
    }
    if (fabsf(score0 - score1) < POLARITY_SCORE_MARGIN)
    {
        return 0xFFU;
    }

    return (score0 >= score1) ? 1U : 0U;
}

static void Measure_FinalizeSingle(SingleMeasureResult_t *result)
{
    SamplePoint_t *forward_low;
    SamplePoint_t *forward_high;
    uint8_t weak0;
    uint8_t weak1;

    result->vf = -1.0f;
    result->vf_low_corrected = -1.0f;
    result->vf_high_corrected = -1.0f;
    result->quality = MEASURE_QUALITY_OPEN;
    result->display_state = DISPLAY_STATE_OPEN;
    result->valid = 0U;
    result->forward_dir = DIR_0;

    weak0 = Measure_IsPointWeak(&result->dir0_low);
    weak1 = Measure_IsPointWeak(&result->dir1_low);
    result->dir0_forward = Measure_EvaluatePolarity(&result->dir0_low, &result->dir1_low);
    if (result->dir0_forward == 0xFFU)
    {
        result->quality = ((weak0 != 0U) || (weak1 != 0U)) ? MEASURE_QUALITY_WEAK : MEASURE_QUALITY_OPEN;
        return;
    }

    result->forward_dir = (result->dir0_forward != 0U) ? DIR_0 : DIR_1;
    result->display_state = (result->dir0_forward != 0U) ? DISPLAY_STATE_FORWARD_ON : DISPLAY_STATE_REVERSE_OFF;
    forward_low = (result->dir0_forward != 0U) ? &result->dir0_low : &result->dir1_low;
    forward_high = (result->dir0_forward != 0U) ? &result->dir0_high : &result->dir1_high;

    if (Measure_IsPointValid(forward_low) == 0U)
    {
        result->quality = MEASURE_QUALITY_WEAK;
        return;
    }

    result->quality = MEASURE_QUALITY_VALID;
    result->valid = 1U;
    result->vf = forward_low->vdut_corr;
    result->vf_low_corrected = forward_low->vdut_corr;
    if (Measure_IsPointValid(forward_high) != 0U)
    {
        result->vf_high_corrected = forward_high->vdut_corr;
    }
}

void Measure_Init(void)
{
}

void Measure_GetPoint(RelayDir_t dir, CurrentRange_t range, SamplePoint_t *pt)
{
    uint8_t relay_changed = 0U;

    if (Relay_GetDirection() != dir)
    {
        Relay_SetDirection(dir);
        relay_changed = 1U;
    }

    if (Relay_GetRange() != range)
    {
        Relay_SetRange(range);
        relay_changed = 1U;
    }

    if (relay_changed != 0U)
    {
        HAL_Delay(RELAY_SETTLE_MS);
    }

    Relay_EnableCurrent(1U);
    HAL_Delay(ANALOG_SETTLE_MS + MEASURE_EXTRA_SETTLE_MS);
    Measure_AveragePoint(dir, range, pt);
    Relay_EnableCurrent(0U);
}

void Measure_ScanSingleDetailed(SingleMeasureResult_t *result)
{
    uint8_t i;
    SamplePoint_t temp;

    memset(result, 0, sizeof(*result));
    Measure_ZeroPoint(&result->dir0_low);
    Measure_ZeroPoint(&result->dir1_low);
    Measure_ZeroPoint(&result->dir0_high);
    Measure_ZeroPoint(&result->dir1_high);

    for (i = 0U; i < MEASURE_SCAN_AVG_TIMES; i++)
    {
        Measure_GetPoint(DIR_0, RANGE_LOW, &temp);
        Measure_AddPoint(&result->dir0_low, &temp);
        HAL_Delay(5U);

        Measure_GetPoint(DIR_1, RANGE_LOW, &temp);
        Measure_AddPoint(&result->dir1_low, &temp);
        HAL_Delay(5U);

        Measure_GetPoint(DIR_0, RANGE_HIGH, &temp);
        Measure_AddPoint(&result->dir0_high, &temp);
        HAL_Delay(5U);

        Measure_GetPoint(DIR_1, RANGE_HIGH, &temp);
        Measure_AddPoint(&result->dir1_high, &temp);
        HAL_Delay(5U);
    }

    Measure_DivPoint(&result->dir0_low, (float)MEASURE_SCAN_AVG_TIMES);
    Measure_DivPoint(&result->dir1_low, (float)MEASURE_SCAN_AVG_TIMES);
    Measure_DivPoint(&result->dir0_high, (float)MEASURE_SCAN_AVG_TIMES);
    Measure_DivPoint(&result->dir1_high, (float)MEASURE_SCAN_AVG_TIMES);

    Measure_FinalizeSingle(result);
}

void Measure_ScanSingle(SingleMeasureResult_t *result)
{
    Measure_ScanSingleDetailed(result);
}

uint8_t Measure_GetForwardLowOnly(SamplePoint_t *low_pt, RelayDir_t *forward_dir, DisplayState_t *display_state)
{
    SamplePoint_t dir0_low;
    SamplePoint_t dir1_low;
    uint8_t dir0_forward;
    uint8_t weak0;
    uint8_t weak1;

    Measure_GetPoint(DIR_0, RANGE_LOW, &dir0_low);
    HAL_Delay(5U);
    Measure_GetPoint(DIR_1, RANGE_LOW, &dir1_low);

    weak0 = Measure_IsPointWeak(&dir0_low);
    weak1 = Measure_IsPointWeak(&dir1_low);
    dir0_forward = Measure_EvaluatePolarity(&dir0_low, &dir1_low);

    if (display_state != NULL)
    {
        if (dir0_forward == 0xFFU)
        {
            *display_state = ((weak0 != 0U) || (weak1 != 0U)) ? DISPLAY_STATE_REVERSE_OFF : DISPLAY_STATE_OPEN;
        }
        else
        {
            *display_state = (dir0_forward != 0U) ? DISPLAY_STATE_FORWARD_ON : DISPLAY_STATE_REVERSE_OFF;
        }
    }

    if (dir0_forward == 0xFFU)
    {
        return 0U;
    }

    if (forward_dir != NULL)
    {
        *forward_dir = (dir0_forward != 0U) ? DIR_0 : DIR_1;
    }
    if (low_pt != NULL)
    {
        *low_pt = (dir0_forward != 0U) ? dir0_low : dir1_low;
    }
    return Measure_IsPointValid((dir0_forward != 0U) ? &dir0_low : &dir1_low);
}

uint8_t Measure_GetForwardTotal(SamplePoint_t *low_pt, SamplePoint_t *high_pt, RelayDir_t *forward_dir, DisplayState_t *display_state)
{
    SingleMeasureResult_t result;

    Measure_ScanSingleDetailed(&result);
    if (result.valid == 0U)
    {
        if (display_state != NULL)
        {
            *display_state = result.display_state;
        }
        return 0U;
    }

    if (low_pt != NULL)
    {
        *low_pt = (result.forward_dir == DIR_0) ? result.dir0_low : result.dir1_low;
    }
    if (high_pt != NULL)
    {
        *high_pt = (result.forward_dir == DIR_0) ? result.dir0_high : result.dir1_high;
    }
    if (forward_dir != NULL)
    {
        *forward_dir = result.forward_dir;
    }
    if (display_state != NULL)
    {
        *display_state = result.display_state;
    }
    return 1U;
}

uint8_t Measure_GetFaultSample(FaultMeasureResult_t *result)
{
    SingleMeasureResult_t scan;
    RelayDir_t dir;

    if (result == NULL)
    {
        return 0U;
    }

    memset(result, 0, sizeof(*result));
    Measure_ScanSingleDetailed(&scan);
    result->display_state = scan.display_state;
    if (scan.valid != 0U)
    {
        dir = scan.forward_dir;
    }
    else if (Measure_SelectForwardDirWeak(&scan, &dir) != 0U)
    {
        result->display_state = DISPLAY_STATE_FORWARD_ON;
    }
    else
    {
        return 0U;
    }

    result->valid = 1U;
    result->forward_dir = dir;
    result->low_pt = (dir == DIR_0) ? scan.dir0_low : scan.dir1_low;
    result->high_pt = (dir == DIR_0) ? scan.dir0_high : scan.dir1_high;

    if (Relay_GetDirection() != dir)
    {
        Relay_SetDirection(dir);
        HAL_Delay(RELAY_SETTLE_MS);
    }
    if (Relay_GetRange() != RANGE_LOW)
    {
        Relay_SetRange(RANGE_LOW);
        HAL_Delay(RELAY_SETTLE_MS);
    }

    Relay_EnableCurrent(1U);
    HAL_Delay(FAULT_STEP_EARLY_MS);
    result->step_early_vsense = Measure_ReadAverageISense();
    result->step_early_vdut = Measure_ReadAverageVdut();
    HAL_Delay(FAULT_STEP_MID_MS - FAULT_STEP_EARLY_MS);
    result->step_mid_vsense = Measure_ReadAverageISense();
    result->step_mid_vdut = Measure_ReadAverageVdut();
    HAL_Delay(FAULT_STEP_LATE_MS - FAULT_STEP_MID_MS);
    result->step_late_vsense = Measure_ReadAverageISense();
    result->step_late_vdut = Measure_ReadAverageVdut();
    Relay_EnableCurrent(0U);

    return 1U;
}

uint8_t Measure_DetectPolarity(void)
{
    SingleMeasureResult_t result;

    Measure_ScanSingle(&result);
    return result.dir0_forward;
}

float Measure_SingleVf(void)
{
    SingleMeasureResult_t result;

    Measure_ScanSingle(&result);
    return result.vf;
}
