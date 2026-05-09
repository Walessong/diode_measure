#include "identify.h"

#include <math.h>
#include <string.h>

#include "app_config.h"
#include "measure.h"

typedef struct
{
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t total;
    uint8_t votes;
    float score_sum;
    float err_low_sum;
    float err_high_sum;
    float err_delta_sum;
} AbcCandidate_t;

static FaultType_t Identify_ClassifyCapacitor(const FaultMeasureResult_t *fault_sample)
{
    float early;
    float mid;
    float late;
    float decay_ratio;
    float vdut_swing;

    if ((fault_sample == NULL) || (fault_sample->valid == 0U))
    {
        return FAULT_UNSTABLE;
    }

    early = fault_sample->step_early_vsense;
    mid = fault_sample->step_mid_vsense;
    late = fault_sample->step_late_vsense;
    decay_ratio = (early > 0.01f) ? (late / early) : 1.0f;
    vdut_swing = fabsf(fault_sample->step_early_vdut - fault_sample->step_late_vdut);

    if ((vdut_swing >= FAULT_CAP_VDUT_SWING_MIN) ||
        ((early >= FAULT_CAP_MIN_EARLY_V) &&
         (mid <= early) &&
         (late <= mid) &&
         ((early - late) >= FAULT_CAP_DROP_MIN_V) &&
         ((early - fault_sample->low_pt.vsense) >= FAULT_CAP_STEADY_DELTA_V) &&
         (decay_ratio <= FAULT_CAP_DECAY_RATIO)))
    {
        return FAULT_SERIES_CAP;
    }

    return FAULT_NONE;
}

static uint8_t Identify_EstimateCountSimple(float total_vf, float single_vf, uint8_t *count_out, float *err_out)
{
    int32_t count;
    float predicted;
    float rel_err;

    if ((count_out == NULL) || (err_out == NULL) || (single_vf <= 0.0f) || (total_vf <= 0.0f))
    {
        return 0U;
    }

    count = (int32_t)lroundf(total_vf / single_vf);
    if (count < 1)
    {
        count = 1;
    }
    if (count > (int32_t)COUNT_MAX_SERIES)
    {
        return 0U;
    }

    predicted = (float)count * single_vf;
    rel_err = fabsf(total_vf - predicted) / ((total_vf > 0.10f) ? total_vf : 0.10f);
    if ((count <= 2) && (rel_err > COUNT_TOL_ONE_TWO))
    {
        return 0U;
    }
    if ((count >= 7) && (rel_err > COUNT_TOL_HIGH_RELAX))
    {
        return 0U;
    }
    if ((count >= 5) && (count <= 6) && (rel_err > COUNT_TOL_HIGH_SERIES))
    {
        return 0U;
    }
    if ((count >= 3) && (count <= 4) && (rel_err > COUNT_TOL_NORMAL))
    {
        return 0U;
    }

    *count_out = (uint8_t)count;
    *err_out = rel_err;
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
        if (display_state == DISPLAY_STATE_OPEN)
        {
            continue;
        }
        if (Identify_EstimateCountSimple(low_pt.vdut_corr, model->vf_low, &candidate, &err) == 0U)
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

static float Identify_AbcPredictLow(const DiodeModel_t *model_a,
                                    const DiodeModel_t *model_b,
                                    const DiodeModel_t *model_c,
                                    uint8_t count_a,
                                    uint8_t count_b,
                                    uint8_t count_c)
{
    return ((float)count_a * model_a->vf_low) +
           ((float)count_b * model_b->vf_low) +
           ((float)count_c * model_c->vf_low);
}

static float Identify_AbcPredictHigh(const DiodeModel_t *model_a,
                                     const DiodeModel_t *model_b,
                                     const DiodeModel_t *model_c,
                                     uint8_t count_a,
                                     uint8_t count_b,
                                     uint8_t count_c)
{
    return ((float)count_a * model_a->vf_high) +
           ((float)count_b * model_b->vf_high) +
           ((float)count_c * model_c->vf_high);
}

static float Identify_NormalizedError(float measured, float predicted, float min_denom)
{
    float denom = (fabsf(measured) > min_denom) ? fabsf(measured) : min_denom;

    return fabsf(measured - predicted) / denom;
}

static uint8_t Identify_FindBestAbcCandidate(const DiodeModel_t *model_a,
                                             const DiodeModel_t *model_b,
                                             const DiodeModel_t *model_c,
                                             float vlow,
                                             float vhigh,
                                             uint8_t *count_a,
                                             uint8_t *count_b,
                                             uint8_t *count_c,
                                             float *best_score,
                                             float *best_err_low,
                                             float *best_err_high,
                                             float *best_err_delta)
{
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t total;
    uint8_t found = 0U;
    float best = 999.0f;
    float second = 999.0f;
    float chosen_low = 0.0f;
    float chosen_high = 0.0f;
    float chosen_delta = 0.0f;
    uint8_t best_a = 0U;
    uint8_t best_b = 0U;
    uint8_t best_c = 0U;

    for (a = 0U; a <= COUNT_MAX_SERIES; a++)
    {
        for (b = 0U; b <= COUNT_MAX_SERIES; b++)
        {
            for (c = 0U; c <= COUNT_MAX_SERIES; c++)
            {
                float pred_low;
                float pred_high;
                float err_low;
                float err_high;
                float err_delta;
                float score;

                total = (uint8_t)(a + b + c);
                if ((total == 0U) || (total > COUNT_MAX_SERIES))
                {
                    continue;
                }

                pred_low = Identify_AbcPredictLow(model_a, model_b, model_c, a, b, c);
                pred_high = Identify_AbcPredictHigh(model_a, model_b, model_c, a, b, c);
                err_low = Identify_NormalizedError(vlow, pred_low, 0.10f);
                err_high = Identify_NormalizedError(vhigh, pred_high, 0.10f);
                err_delta = 0.0f;
                score = (0.55f * err_low) + (0.45f * err_high);

                if ((err_low > COUNT_ABC_ERR_LOW_MAX) ||
                    (err_high > COUNT_ABC_ERR_HIGH_MAX) ||
                    (score > COUNT_ABC_SCORE_MAX))
                {
                    continue;
                }

                if (score < best)
                {
                    second = best;
                    best = score;
                    best_a = a;
                    best_b = b;
                    best_c = c;
                    chosen_low = err_low;
                    chosen_high = err_high;
                    chosen_delta = err_delta;
                    found = 1U;
                }
                else if (score < second)
                {
                    second = score;
                }
            }
        }
    }

    if (found == 0U)
    {
        return 0U;
    }

    *count_a = best_a;
    *count_b = best_b;
    *count_c = best_c;
    *best_score = best;
    *best_err_low = chosen_low;
    *best_err_high = chosen_high;
    *best_err_delta = chosen_delta;
    return 1U;
}

static uint8_t Identify_VoteAbcCount(const DiodeModel_t *model_a,
                                     const DiodeModel_t *model_b,
                                     const DiodeModel_t *model_c,
                                     CountAbcResult_t *result)
{
    uint8_t i;
    uint8_t idx;
    uint8_t best_idx = 0xFFU;
    uint8_t candidate_count = 0U;
    AbcCandidate_t candidates[COUNT_ABC_MAX_CANDIDATES];
    SamplePoint_t low_pt;
    SamplePoint_t high_pt;
    RelayDir_t forward_dir;
    DisplayState_t display_state;
    uint8_t valid_votes = 0U;

    memset(candidates, 0, sizeof(candidates));

    for (i = 0U; i < COUNT_ABC_SCAN_TIMES; i++)
    {
        uint8_t count_a;
        uint8_t count_b;
        uint8_t count_c;
        float score;
        float err_low;
        float err_high;
        float err_delta;
        uint8_t matched = 0U;

        if (Measure_GetForwardTotal(&low_pt, &high_pt, &forward_dir, &display_state) == 0U)
        {
            continue;
        }
        if (display_state == DISPLAY_STATE_OPEN)
        {
            continue;
        }

        if ((high_pt.vdut_corr < VF_VALID_MIN) || (high_pt.vdut_corr > VF_VALID_MAX))
        {
            continue;
        }

        if (Identify_FindBestAbcCandidate(model_a, model_b, model_c,
                                          low_pt.vdut_corr,
                                          high_pt.vdut_corr,
                                          &count_a, &count_b, &count_c,
                                          &score, &err_low, &err_high, &err_delta) == 0U)
        {
            continue;
        }

        valid_votes++;

        for (idx = 0U; idx < candidate_count; idx++)
        {
            if ((candidates[idx].a == count_a) &&
                (candidates[idx].b == count_b) &&
                (candidates[idx].c == count_c))
            {
                candidates[idx].votes++;
                candidates[idx].score_sum += score;
                candidates[idx].err_low_sum += err_low;
                candidates[idx].err_high_sum += err_high;
                candidates[idx].err_delta_sum += err_delta;
                matched = 1U;
                break;
            }
        }

        if ((matched == 0U) && (candidate_count < COUNT_ABC_MAX_CANDIDATES))
        {
            candidates[candidate_count].a = count_a;
            candidates[candidate_count].b = count_b;
            candidates[candidate_count].c = count_c;
            candidates[candidate_count].total = (uint8_t)(count_a + count_b + count_c);
            candidates[candidate_count].votes = 1U;
            candidates[candidate_count].score_sum = score;
            candidates[candidate_count].err_low_sum = err_low;
            candidates[candidate_count].err_high_sum = err_high;
            candidates[candidate_count].err_delta_sum = err_delta;
            candidate_count++;
        }
    }

    if (candidate_count == 0U)
    {
        return 0U;
    }

    for (idx = 0U; idx < candidate_count; idx++)
    {
        float avg_score = candidates[idx].score_sum / (float)candidates[idx].votes;
        float best_avg_score;

        if (best_idx == 0xFFU)
        {
            best_idx = idx;
            continue;
        }

        best_avg_score = candidates[best_idx].score_sum / (float)candidates[best_idx].votes;
        if ((candidates[idx].votes > candidates[best_idx].votes) ||
            ((candidates[idx].votes == candidates[best_idx].votes) && (avg_score < best_avg_score)))
        {
            best_idx = idx;
        }
    }

    if (best_idx == 0xFFU)
    {
        return 0U;
    }

    if ((valid_votes < COUNT_ABC_MIN_VOTES) || (candidates[best_idx].votes < COUNT_ABC_MIN_VOTES))
    {
        return 0U;
    }
    if ((candidates[best_idx].votes < (COUNT_ABC_SCAN_TIMES / 2U)) &&
        (candidates[best_idx].votes < COUNT_ABC_RELAXED_VOTES))
    {
        return 0U;
    }
    if ((candidates[best_idx].score_sum / (float)candidates[best_idx].votes) > COUNT_ABC_SCORE_MAX)
    {
        return 0U;
    }

    result->count_a = candidates[best_idx].a;
    result->count_b = candidates[best_idx].b;
    result->count_c = candidates[best_idx].c;
    result->total_count = candidates[best_idx].total;
    result->votes = candidates[best_idx].votes;
    result->fit_error = candidates[best_idx].score_sum / (float)candidates[best_idx].votes;
    result->fit_error_low = candidates[best_idx].err_low_sum / (float)candidates[best_idx].votes;
    result->fit_error_high = candidates[best_idx].err_high_sum / (float)candidates[best_idx].votes;
    result->fit_error_delta = candidates[best_idx].err_delta_sum / (float)candidates[best_idx].votes;
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

FaultType_t Identify_DetectFaultSameType(const DiodeModel_t *model,
                                         const FaultMeasureResult_t *fault_sample)
{
    (void)model;
    if ((fault_sample == NULL) || (fault_sample->valid == 0U))
    {
        return FAULT_OPEN;
    }
    return Identify_ClassifyCapacitor(fault_sample);
}

CountStatus_t Identify_CountSameType(const DiodeModel_t *model, CountResult_t *result)
{
    SingleMeasureResult_t raw_scan;

    if (result == NULL)
    {
        return COUNT_FAIL;
    }

    memset(result, 0, sizeof(*result));
    result->status = COUNT_FAIL;
    result->fault_type = FAULT_NONE;
    if ((model == NULL) || (model->valid == 0U))
    {
        result->status = COUNT_NO_MODEL;
        return result->status;
    }

    if (Identify_VoteLowCount(model, result) == 0U)
    {
        Measure_ScanSingleDetailed(&raw_scan);
        if (raw_scan.quality == MEASURE_QUALITY_OPEN)
        {
            result->status = COUNT_OPEN;
        }
        else if (raw_scan.valid == 0U)
        {
            result->status = COUNT_UNSTABLE;
        }
        else
        {
            result->status = COUNT_OUT_OF_RANGE;
        }
        return result->status;
    }

    result->has_fault = 0U;
    result->fault_type = FAULT_NONE;
    result->count_high = result->count_low;
    result->err_high = result->err_low;
    result->final_count = result->count_low;
    result->status = COUNT_OK;
    return result->status;
}

CountAbcStatus_t Identify_CountABC(const DiodeModel_t *model_a,
                                   const DiodeModel_t *model_b,
                                   const DiodeModel_t *model_c,
                                   CountAbcResult_t *result)
{
    if (result == NULL)
    {
        return COUNT_ABC_FAIL;
    }

    memset(result, 0, sizeof(*result));
    result->status = COUNT_ABC_FAIL;
    if ((model_a == NULL) || (model_b == NULL) || (model_c == NULL) ||
        (model_a->valid == 0U) || (model_b->valid == 0U) || (model_c->valid == 0U))
    {
        result->status = COUNT_ABC_NO_MODEL;
        return result->status;
    }

    if (Identify_VoteAbcCount(model_a, model_b, model_c, result) == 0U)
    {
        result->status = COUNT_ABC_OUT_OF_RANGE;
        return result->status;
    }

    result->status = COUNT_ABC_OK;
    return result->status;
}
