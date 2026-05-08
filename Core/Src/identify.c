#include "identify.h"

#include <math.h>
#include <string.h>

#include "app_config.h"
#include "measure.h"

static uint8_t Identify_EstimateCountAffine(float total_vf, float single_vf, uint8_t *count_out, float *err_out)
{
    float step_vf;
    int32_t count;
    float predicted;
    float abs_err;

    if ((count_out == NULL) || (err_out == NULL) || (single_vf <= 0.0f) || (total_vf <= 0.0f))
    {
        return 0U;
    }

    step_vf = single_vf - SERIES_FIT_BIAS_V;
    if (step_vf < SERIES_STEP_MIN_V)
    {
        step_vf = SERIES_STEP_MIN_V;
    }

    if (total_vf < (single_vf - SERIES_FIT_TOL_V))
    {
        return 0U;
    }

    count = 1 + (int32_t)lroundf((total_vf - single_vf) / step_vf);
    if (count < 1)
    {
        count = 1;
    }
    if (count > (int32_t)COUNT_MAX_SERIES)
    {
        return 0U;
    }

    predicted = single_vf + ((float)(count - 1) * step_vf);
    abs_err = fabsf(total_vf - predicted);
    if (abs_err > SERIES_FIT_TOL_V)
    {
        return 0U;
    }

    *count_out = (uint8_t)count;
    *err_out = abs_err / ((total_vf > 0.10f) ? total_vf : 0.10f);
    return 1U;
}

static uint8_t Identify_VoteLowCount(const DiodeModel_t *model, CountResult_t *result)
{
    uint8_t i;
    uint8_t count;
    uint8_t best_count = 0U;
    uint8_t votes[COUNT_MAX_SERIES + 1U] = {0};
    uint8_t valid_samples = 0U;
    float err_sum[COUNT_MAX_SERIES + 1U] = {0};
    float best_avg_err = 999.0f;
    SamplePoint_t low_pt;
    RelayDir_t forward_dir;
    DisplayState_t display_state;
    uint8_t candidate;
    float err;

    (void)forward_dir;
    for (i = 0U; i < COUNT_SCAN_AVG_TIMES; i++)
    {
        if (Measure_GetForwardLowOnly(&low_pt, &forward_dir, &display_state) == 0U)
        {
            continue;
        }
        if (display_state != DISPLAY_STATE_FORWARD_ON)
        {
            continue;
        }
        if (Identify_EstimateCountAffine(low_pt.vdut_corr, model->vf_low, &candidate, &err) == 0U)
        {
            continue;
        }

        votes[candidate]++;
        err_sum[candidate] += err;
        valid_samples++;
    }

    if (valid_samples == 0U)
    {
        return 0U;
    }

    for (count = 1U; count <= COUNT_MAX_SERIES; count++)
    {
        if (votes[count] == 0U)
        {
            continue;
        }
        if ((best_count == 0U) ||
            (votes[count] > votes[best_count]) ||
            ((votes[count] == votes[best_count]) && ((err_sum[count] / (float)votes[count]) < best_avg_err)))
        {
            best_count = count;
            best_avg_err = err_sum[count] / (float)votes[count];
        }
    }

    if (best_count == 0U)
    {
        return 0U;
    }

    result->count_low = best_count;
    result->count_high = best_count;
    result->final_count = best_count;
    result->err_low = best_avg_err;
    result->err_high = best_avg_err;
    return 1U;
}

float Identify_ReconstructError(float total_vf, float unit_vf, uint8_t count)
{
    float denom = unit_vf * (float)((count == 0U) ? 1U : count);

    if ((unit_vf <= 0.0f) || (total_vf <= 0.0f) || (count == 0U))
    {
        return 1.0f;
    }

    return fabsf(total_vf - ((float)count * unit_vf)) / denom;
}

CountStatus_t Identify_CountSameType(const DiodeModel_t *model, CountResult_t *result)
{
    if (result == NULL)
    {
        return COUNT_FAIL;
    }

    memset(result, 0, sizeof(*result));
    result->status = COUNT_FAIL;
    if ((model == NULL) || (model->valid == 0U))
    {
        result->status = COUNT_NO_MODEL;
        return result->status;
    }

    if (Identify_VoteLowCount(model, result) == 0U)
    {
        result->status = COUNT_OUT_OF_RANGE;
        return result->status;
    }

    result->count_high = result->count_low;
    result->err_high = result->err_low;
    result->final_count = result->count_low;
    result->status = COUNT_OK;
    return result->status;
}
