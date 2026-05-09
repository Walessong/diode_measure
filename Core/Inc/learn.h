#ifndef LEARN_H
#define LEARN_H

#include "main.h"
#include "measure.h"

typedef struct
{
    float vf_low;
    float vf_high;
    float offset_low;
    float offset_high;
    float quality_score;
    uint8_t valid;
} DiodeModel_t;

typedef enum
{
    MODEL_SLOT_SINGLE = 0,
    MODEL_SLOT_A,
    MODEL_SLOT_B,
    MODEL_SLOT_C,
    MODEL_SLOT_COUNT
} ModelSlot_t;

typedef enum
{
    LEARN_STATUS_OK = 0,
    LEARN_STATUS_OPEN,
    LEARN_STATUS_REVERSE,
    LEARN_STATUS_UNSTABLE,
    LEARN_STATUS_FAIL
} LearnStatus_t;

typedef struct
{
    LearnStatus_t status;
    DiodeModel_t model;
} LearnResult_t;

void Learn_Init(void);
void Learn_ClearAllModels(void);
void Learn_ClearModel(ModelSlot_t slot);
uint8_t Learn_SaveModel(ModelSlot_t slot, const DiodeModel_t *model);
uint8_t Learn_LoadAllModels(void);
const DiodeModel_t *Learn_GetModel(ModelSlot_t slot);
uint8_t Learn_IsModelValid(ModelSlot_t slot);
LearnStatus_t Learn_BuildModel(const SingleMeasureResult_t *scan, DiodeModel_t *model);

/* Compatibility wrappers for the existing single-reference workflow. */
static inline void Learn_ClearSingleModel(void)
{
    Learn_ClearModel(MODEL_SLOT_SINGLE);
}

static inline uint8_t Learn_SaveSingleModel(const DiodeModel_t *model)
{
    return Learn_SaveModel(MODEL_SLOT_SINGLE, model);
}

static inline uint8_t Learn_LoadSingleModel(void)
{
    return Learn_LoadAllModels();
}

static inline const DiodeModel_t *Learn_GetSingleModel(void)
{
    return Learn_GetModel(MODEL_SLOT_SINGLE);
}

#endif
