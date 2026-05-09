#include "learn.h"

#include <math.h>
#include <string.h>

#include "app_config.h"
#include "flash_storage.h"

static DiodeModel_t g_models[MODEL_SLOT_COUNT];

static uint8_t Learn_IsSlotValid(ModelSlot_t slot)
{
    return (slot < MODEL_SLOT_COUNT) ? 1U : 0U;
}

void Learn_Init(void)
{
    if (FlashStorage_LoadModels(g_models, MODEL_SLOT_COUNT) == 0U)
    {
        memset(g_models, 0, sizeof(g_models));
    }
}

void Learn_ClearAllModels(void)
{
    memset(g_models, 0, sizeof(g_models));
}

void Learn_ClearModel(ModelSlot_t slot)
{
    if (Learn_IsSlotValid(slot) == 0U)
    {
        return;
    }

    memset(&g_models[slot], 0, sizeof(g_models[slot]));
    (void)FlashStorage_SaveModels(g_models, MODEL_SLOT_COUNT);
}

uint8_t Learn_SaveModel(ModelSlot_t slot, const DiodeModel_t *model)
{
    if ((Learn_IsSlotValid(slot) == 0U) || (model == NULL))
    {
        return 0U;
    }

    g_models[slot] = *model;
    return FlashStorage_SaveModels(g_models, MODEL_SLOT_COUNT);
}

uint8_t Learn_LoadAllModels(void)
{
    return FlashStorage_LoadModels(g_models, MODEL_SLOT_COUNT);
}

const DiodeModel_t *Learn_GetModel(ModelSlot_t slot)
{
    if (Learn_IsSlotValid(slot) == 0U)
    {
        return NULL;
    }

    return &g_models[slot];
}

uint8_t Learn_IsModelValid(ModelSlot_t slot)
{
    const DiodeModel_t *model = Learn_GetModel(slot);

    return ((model != NULL) && (model->valid != 0U)) ? 1U : 0U;
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
    if (scan->valid == 0U)
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
