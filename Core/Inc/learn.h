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
void Learn_ClearSingleModel(void);
uint8_t Learn_SaveSingleModel(const DiodeModel_t *model);
uint8_t Learn_LoadSingleModel(void);
const DiodeModel_t *Learn_GetSingleModel(void);
LearnStatus_t Learn_BuildModel(const SingleMeasureResult_t *scan, DiodeModel_t *model);

#endif
