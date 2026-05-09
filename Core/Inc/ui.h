#ifndef UI_H
#define UI_H

#include "identify.h"
#include "measure.h"

void UI_ShowBoot(void);
void UI_ShowMenu(uint8_t menu_index,
                 uint8_t single_valid,
                 uint8_t model_a_valid,
                 uint8_t model_b_valid,
                 uint8_t model_c_valid);
void UI_ShowPolarityResult(uint8_t dir0_forward);
void UI_ShowVf(float vf);
void UI_ShowAutoSingle(const SingleMeasureResult_t *result);
void UI_ShowLearnResult(ModelSlot_t slot, const LearnResult_t *result);
void UI_ShowCountResult(const CountResult_t *result);
void UI_ShowCountAbcResult(const CountAbcResult_t *result);
void UI_ShowDiagInfo(const SingleMeasureResult_t *result,
                     const FaultMeasureResult_t *fault,
                     const DiodeModel_t *model);
void UI_ShowHint(const char *line0, const char *line1);

#endif
