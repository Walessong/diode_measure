#include "learn.h"

#include <math.h>
#include <string.h>

#include "app_config.h"
#include "flash_storage.h"

static DiodeModel_t g_single_model;

void Learn_Init(void)
{
    if (FlashStorage_LoadModel(&g_single_model) == 0U)
    {
        memset(&g_single_model, 0, sizeof(g_single_model));
    }
}

void Learn_ClearSingleModel(void)
{
    memset(&g_single_model, 0, sizeof(g_single_model));
}

uint8_t Learn_SaveSingleModel(const DiodeModel_t *model)
{
    if (model == NULL)
    {
        return 0U;
    }

    g_single_model = *model;
    return FlashStorage_SaveModel(model);
}

uint8_t Learn_LoadSingleModel(void)
{
    return FlashStorage_LoadModel(&g_single_model);
}

const DiodeModel_t *Learn_GetSingleModel(void)
{
    return &g_single_model;
}

LearnStatus_t Learn_BuildModel(const SingleMeasureResult_t *scan, DiodeModel_t *model)
{
    float delta;
    float quality;

    if ((scan == NULL) || (model == NULL))
    {
        return LEARN_STATUS_FAIL;
    }

    memset(model, 0, sizeof(*model));
    if (scan->quality == MEASURE_QUALITY_OPEN)
    {
        return LEARN_STATUS_OPEN;
    }
    if (scan->display_state == DISPLAY_STATE_REVERSE_OFF)
    {
        return LEARN_STATUS_REVERSE;
    }
    if ((scan->valid == 0U) || (scan->display_state != DISPLAY_STATE_FORWARD_ON))
    {
        return LEARN_STATUS_UNSTABLE;
    }
    if ((scan->vf_low_corrected < VF_VALID_MIN) || (scan->vf_low_corrected > VF_VALID_MAX) ||
        (scan->vf_high_corrected < VF_VALID_MIN) || (scan->vf_high_corrected > VF_VALID_MAX))
    {
        return LEARN_STATUS_UNSTABLE;
    }

    delta = fabsf(scan->vf_high_corrected - scan->vf_low_corrected);
    if (delta > MODEL_VF_DELTA_MAX)
    {
        return LEARN_STATUS_UNSTABLE;
    }

    model->vf_low = scan->vf_low_corrected;
    model->vf_high = scan->vf_high_corrected;
    model->offset_low = FIXED_DROP_LOW_V;
    model->offset_high = FIXED_DROP_HIGH_V;
    quality = 1.0f - (delta / ((scan->vf_low_corrected > 0.10f) ? scan->vf_low_corrected : 0.10f));
    if (quality < 0.0f)
    {
        quality = 0.0f;
    }
    model->quality_score = quality;
    model->valid = 1U;
    return LEARN_STATUS_OK;
}
