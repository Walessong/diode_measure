#include "ui.h"

#include <stdio.h>

#include "bsp_oled.h"

#define CN_TITLE        "\xE4\xBA\x8C\xE6\x9E\x81\xE7\xAE\xA1\xE6\xB5\x8B\xE8\xAF\x95"
#define CN_FORWARD_ON   "\xE6\xAD\xA3\xE5\x90\x91\xE5\xAF\xBC\xE9\x80\x9A"
#define CN_REVERSE_OFF  "\xE5\x8F\x8D\xE5\x90\x91\xE6\x88\xAA\xE6\xAD\xA2"
#define CN_OPEN         "\xE5\xBC\x80\xE8\xB7\xAF"
#define CN_SERIES_CAP   "\xE4\xB8\xB2\xE6\x8E\xA5\xE7\x94\xB5\xE5\xAE\xB9"
#define CN_SERIES_RES   "\xE4\xB8\xB2\xE6\x8E\xA5\xE7\x94\xB5\xE9\x98\xBB"
#define CN_VOLTAGE      "\xE7\x94\xB5\xE5\x8E\x8B"

static float UI_CalibrateDisplayVoltage(float measured_v)
{
    static const float x_points[] = {0.00f, 0.21f, 0.61f, 0.64f, 0.75f};
    static const float y_points[] = {0.00f, 0.153f, 0.55f, 0.577f, 0.758f};
    uint8_t i;

    if (measured_v <= 0.0f)
    {
        return measured_v;
    }

    if (measured_v >= x_points[4])
    {
        return measured_v;
    }

    for (i = 0U; i < 4U; i++)
    {
        if (measured_v <= x_points[i + 1U])
        {
            float x0 = x_points[i];
            float x1 = x_points[i + 1U];
            float y0 = y_points[i];
            float y1 = y_points[i + 1U];
            float t = (measured_v - x0) / (x1 - x0);

            return y0 + (t * (y1 - y0));
        }
    }

    return measured_v;
}

static const char *UI_DisplayStateText(DisplayState_t state)
{
    switch (state)
    {
    case DISPLAY_STATE_FORWARD_ON:
        return CN_FORWARD_ON;
    case DISPLAY_STATE_REVERSE_OFF:
        return CN_REVERSE_OFF;
    case DISPLAY_STATE_OPEN:
    default:
        return CN_OPEN;
    }
}

static const char *UI_ModelSlotName(ModelSlot_t slot)
{
    switch (slot)
    {
    case MODEL_SLOT_A:
        return "A";
    case MODEL_SLOT_B:
        return "B";
    case MODEL_SLOT_C:
        return "C";
    case MODEL_SLOT_SINGLE:
    default:
        return "REF";
    }
}

void UI_ShowBoot(void)
{
    OLED_ClearAll();
    OLED_ShowStringLine(0, "DIODE TESTER");
    OLED_ShowStringLine(2, "MATCH MODE");
    OLED_ShowStringLine(4, "BOOT...");
}

void UI_ShowMenu(uint8_t menu_index,
                 uint8_t single_valid,
                 uint8_t model_a_valid,
                 uint8_t model_b_valid,
                 uint8_t model_c_valid)
{
    char line[21];

    OLED_ClearAll();
    OLED_ShowStringLine(0, "MODE SELECT");
    switch (menu_index)
    {
    case 0U:
        OLED_ShowStringLine(2, "> POLARITY");
        break;
    case 1U:
        OLED_ShowStringLine(2, "> SINGLE VF");
        break;
    case 2U:
        OLED_ShowStringLine(2, "> LEARN REF");
        break;
    case 3U:
        OLED_ShowStringLine(2, "> COUNT REF");
        break;
    case 4U:
        OLED_ShowStringLine(2, "> LEARN A");
        break;
    case 5U:
        OLED_ShowStringLine(2, "> LEARN B");
        break;
    case 6U:
        OLED_ShowStringLine(2, "> LEARN C");
        break;
    case 7U:
        OLED_ShowStringLine(2, "> COUNT ABC");
        break;
    case 8U:
        OLED_ShowStringLine(2, "> DIAG INFO");
        break;
    default:
        OLED_ShowStringLine(2, "> UNKNOWN");
        break;
    }

    snprintf(line, sizeof(line), "R%d A%d B%d C%d",
             single_valid ? 1 : 0,
             model_a_valid ? 1 : 0,
             model_b_valid ? 1 : 0,
             model_c_valid ? 1 : 0);
    OLED_ShowStringLine(5, line);
    OLED_ShowStringLine(7, "MODE/LEARN/MEAS");
}

void UI_ShowPolarityResult(uint8_t dir0_forward)
{
    OLED_ClearAll();
    OLED_ShowStringRow16(0, CN_TITLE);
    if (dir0_forward == 0xFFU)
    {
        OLED_ShowStringRow16(1, CN_OPEN);
    }
    else if (dir0_forward != 0U)
    {
        OLED_ShowStringRow16(1, CN_FORWARD_ON);
    }
    else
    {
        OLED_ShowStringRow16(1, CN_REVERSE_OFF);
    }
}

void UI_ShowVf(float vf)
{
    char line[32];

    OLED_ClearAll();
    OLED_ShowStringRow16(0, CN_TITLE);
    if (vf < 0.0f)
    {
        OLED_ShowStringRow16(1, CN_OPEN);
        return;
    }

    snprintf(line, sizeof(line), "%s:%.2fV", CN_VOLTAGE, UI_CalibrateDisplayVoltage(vf));
    OLED_ShowStringRow16(1, line);
}

void UI_ShowAutoSingle(const SingleMeasureResult_t *result)
{
    char line[32];

    OLED_ClearAll();
    OLED_ShowStringRow16(0, CN_TITLE);
    if ((result == NULL) || (result->valid == 0U))
    {
        OLED_ShowStringRow16(1, CN_OPEN);
        OLED_ShowStringRow16(2, "\xE7\x94\xB5\xE5\x8E\x8B:--.--V");
        return;
    }

    OLED_ShowStringRow16(1, UI_DisplayStateText(result->display_state));
    snprintf(line, sizeof(line), "%s:%.2fV", CN_VOLTAGE, UI_CalibrateDisplayVoltage(result->vf));
    OLED_ShowStringRow16(2, line);
}

void UI_ShowLearnResult(ModelSlot_t slot, const LearnResult_t *result)
{
    char line[21];

    OLED_ClearAll();
    if ((result == NULL) || (result->status != LEARN_STATUS_OK))
    {
        snprintf(line, sizeof(line), "LEARN %s FAIL", UI_ModelSlotName(slot));
        OLED_ShowStringLine(1, line);
        switch ((result == NULL) ? LEARN_STATUS_FAIL : result->status)
        {
        case LEARN_STATUS_OPEN:
            OLED_ShowStringLine(3, "OPEN / NO DUT");
            break;
        case LEARN_STATUS_REVERSE:
            OLED_ShowStringLine(3, "REVERSE DUT");
            break;
        case LEARN_STATUS_UNSTABLE:
            OLED_ShowStringLine(3, "UNSTABLE DUT");
            break;
        default:
            OLED_ShowStringLine(3, "CHECK DUT");
            break;
        }
        return;
    }

    snprintf(line, sizeof(line), "LEARN %s OK", UI_ModelSlotName(slot));
    OLED_ShowStringLine(0, line);
    snprintf(line, sizeof(line), "LOW=%.2fV", UI_CalibrateDisplayVoltage(result->model.vf_low));
    OLED_ShowStringLine(2, line);
    snprintf(line, sizeof(line), "HIGH=%.2fV", UI_CalibrateDisplayVoltage(result->model.vf_high));
    OLED_ShowStringLine(4, line);
    snprintf(line, sizeof(line), "Q=%.2f", result->model.quality_score);
    OLED_ShowStringLine(6, line);
}

void UI_ShowCountResult(const CountResult_t *result)
{
    char line[21];

    OLED_ClearAll();
    if (result == NULL)
    {
        OLED_ShowStringLine(2, "COUNT FAIL");
        return;
    }

    switch (result->status)
    {
    case COUNT_OK:
        OLED_ShowStringLine(0, "COUNT OK");
        snprintf(line, sizeof(line), "COUNT=%u", result->final_count);
        OLED_ShowStringLine(2, line);
        snprintf(line, sizeof(line), "L%u E%.2f", result->count_low, result->err_low);
        OLED_ShowStringLine(4, line);
        snprintf(line, sizeof(line), "H%u E%.2f", result->count_high, result->err_high);
        OLED_ShowStringLine(6, line);
        break;
    case COUNT_NO_MODEL:
        OLED_ShowStringLine(2, "NO MODEL");
        OLED_ShowStringLine(4, "LEARN FIRST");
        break;
    case COUNT_OPEN:
        OLED_ShowStringRow16(1, CN_OPEN);
        break;
    case COUNT_UNSTABLE:
        OLED_ShowStringLine(2, "COUNT FAIL");
        OLED_ShowStringLine(4, "UNSTABLE DUT");
        break;
    case COUNT_OUT_OF_RANGE:
        OLED_ShowStringLine(2, "COUNT FAIL");
        OLED_ShowStringLine(4, "OUT OF RNG");
        break;
    case COUNT_FAIL:
    default:
        if ((result->has_fault != 0U) && (result->fault_type == FAULT_SERIES_CAP))
        {
            OLED_ShowStringRow16(1, CN_SERIES_CAP);
        }
        else if ((result->has_fault != 0U) && (result->fault_type == FAULT_SERIES_RES))
        {
            OLED_ShowStringRow16(1, CN_SERIES_RES);
        }
        else
        {
            OLED_ShowStringLine(2, "COUNT FAIL");
            OLED_ShowStringLine(4, "CHECK DUT");
        }
        break;
    }
}

void UI_ShowCountAbcResult(const CountAbcResult_t *result)
{
    char line[21];

    OLED_ClearAll();
    if (result == NULL)
    {
        OLED_ShowStringLine(2, "ABC FAIL");
        return;
    }

    switch (result->status)
    {
    case COUNT_ABC_OK:
        OLED_ShowStringLine(0, "ABC COUNT OK");
        snprintf(line, sizeof(line), "A=%u B=%u", result->count_a, result->count_b);
        OLED_ShowStringLine(2, line);
        snprintf(line, sizeof(line), "C=%u T=%u", result->count_c, result->total_count);
        OLED_ShowStringLine(4, line);
        snprintf(line, sizeof(line), "S%.2f V%u", result->fit_error, result->votes);
        OLED_ShowStringLine(5, line);
        snprintf(line, sizeof(line), "L%.2f H%.2f", result->fit_error_low, result->fit_error_high);
        OLED_ShowStringLine(6, line);
        break;
    case COUNT_ABC_NO_MODEL:
        OLED_ShowStringLine(1, "ABC NO MODEL");
        OLED_ShowStringLine(3, "LEARN A/B/C");
        break;
    case COUNT_ABC_OPEN:
        OLED_ShowStringLine(2, "OPEN");
        break;
    case COUNT_ABC_UNSTABLE:
        OLED_ShowStringLine(2, "ABC FAIL");
        OLED_ShowStringLine(4, "UNSTABLE DUT");
        break;
    case COUNT_ABC_OUT_OF_RANGE:
        OLED_ShowStringLine(2, "ABC FAIL");
        OLED_ShowStringLine(4, "OUT OF RNG");
        break;
    case COUNT_ABC_FAIL:
    default:
        OLED_ShowStringLine(2, "ABC FAIL");
        OLED_ShowStringLine(4, "CHECK DUT");
        break;
    }
}

void UI_ShowDiagInfo(const SingleMeasureResult_t *result,
                     const FaultMeasureResult_t *fault,
                     const DiodeModel_t *model)
{
    char line[22];
    float model_low = (model != NULL) ? model->vf_low : 0.0f;
    float model_high = (model != NULL) ? model->vf_high : 0.0f;
    float extra_low = 0.0f;
    float extra_high = 0.0f;

    OLED_ClearAll();
    OLED_ShowStringLine(0, "DIAG INFO");
    if (result == NULL)
    {
        OLED_ShowStringLine(2, "NO SAMPLE");
        return;
    }

    snprintf(line, sizeof(line), "D0L %.2f %.2f", result->dir0_low.vdut_corr, result->dir0_low.vsense);
    OLED_ShowStringLine(1, line);
    snprintf(line, sizeof(line), "D1L %.2f %.2f", result->dir1_low.vdut_corr, result->dir1_low.vsense);
    OLED_ShowStringLine(2, line);
    snprintf(line, sizeof(line), "F %.2f H%.2f",
             UI_CalibrateDisplayVoltage(result->vf_low_corrected),
             UI_CalibrateDisplayVoltage(result->vf_high_corrected));
    OLED_ShowStringLine(3, line);
    if (fault != NULL)
    {
        snprintf(line, sizeof(line), "E%.2f M%.2f", fault->step_early_vsense, fault->step_mid_vsense);
        OLED_ShowStringLine(4, line);
        snprintf(line, sizeof(line), "L%.2f S%.2f", fault->step_late_vsense, fault->low_pt.vsense);
        OLED_ShowStringLine(5, line);
        extra_low = fault->low_pt.vdut_corr - model_low;
        extra_high = fault->high_pt.vdut_corr - model_high;
        snprintf(line, sizeof(line), "XL%.2f XH%.2f", extra_low, extra_high);
        OLED_ShowStringLine(6, line);
    }
    else
    {
        OLED_ShowStringLine(4, "NO FAULT SAMP");
        OLED_ShowStringLine(5, " ");
        OLED_ShowStringLine(6, " ");
    }
    snprintf(line, sizeof(line), "M %.2f %.2f", model_low, model_high);
    OLED_ShowStringLine(7, line);
}

void UI_ShowHint(const char *line0, const char *line1)
{
    OLED_ClearAll();
    OLED_ShowStringLine(2, line0);
    OLED_ShowStringLine(4, line1);
}
