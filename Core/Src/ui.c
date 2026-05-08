#include "ui.h"

#include <stdio.h>

#include "bsp_oled.h"

#define CN_TITLE        "\xE4\xBA\x8C\xE6\x9E\x81\xE7\xAE\xA1\xE6\xB5\x8B\xE8\xAF\x95"
#define CN_FORWARD_ON   "\xE6\xAD\xA3\xE5\x90\x91\xE5\xAF\xBC\xE9\x80\x9A"
#define CN_REVERSE_OFF  "\xE5\x8F\x8D\xE5\x90\x91\xE6\x88\xAA\xE6\xAD\xA2"
#define CN_OPEN         "\xE5\xBC\x80\xE8\xB7\xAF"
#define CN_VOLTAGE      "\xE7\x94\xB5\xE5\x8E\x8B"

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

void UI_ShowBoot(void)
{
    OLED_ClearAll();
    OLED_ShowStringLine(0, "DIODE TESTER");
    OLED_ShowStringLine(2, "MATCH MODE");
    OLED_ShowStringLine(4, "BOOT...");
}

void UI_ShowMenu(uint8_t menu_index, uint8_t model_valid)
{
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
        OLED_ShowStringLine(2, "> DIAG INFO");
        break;
    default:
        OLED_ShowStringLine(2, "> UNKNOWN");
        break;
    }
    OLED_ShowStringLine(5, model_valid ? "MODEL:OK" : "MODEL:NONE");
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

    snprintf(line, sizeof(line), "%s:%.2fV", CN_VOLTAGE, vf);
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
    if (result->display_state == DISPLAY_STATE_FORWARD_ON)
    {
        snprintf(line, sizeof(line), "%s:%.2fV", CN_VOLTAGE, result->vf);
    }
    else
    {
        snprintf(line, sizeof(line), "%s:--.--V", CN_VOLTAGE);
    }
    OLED_ShowStringRow16(2, line);
}

void UI_ShowLearnResult(const LearnResult_t *result)
{
    char line[21];

    OLED_ClearAll();
    if ((result == NULL) || (result->status != LEARN_STATUS_OK))
    {
        OLED_ShowStringLine(1, "LEARN FAIL");
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

    OLED_ShowStringLine(0, "LEARN OK");
    snprintf(line, sizeof(line), "LOW=%.2fV", result->model.vf_low);
    OLED_ShowStringLine(2, line);
    snprintf(line, sizeof(line), "HIGH=%.2fV", result->model.vf_high);
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
        OLED_ShowStringLine(2, "OPEN");
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
        OLED_ShowStringLine(2, "COUNT FAIL");
        OLED_ShowStringLine(4, "CHECK DUT");
        break;
    }
}

void UI_ShowDiagInfo(const SingleMeasureResult_t *result, const DiodeModel_t *model)
{
    char line[22];

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
    snprintf(line, sizeof(line), "FWDL %.2f H%.2f", result->vf_low_corrected, result->vf_high_corrected);
    OLED_ShowStringLine(3, line);
    snprintf(line, sizeof(line), "RAWL %.2f", (result->forward_dir == DIR_0) ? result->dir0_low.vdut : result->dir1_low.vdut);
    OLED_ShowStringLine(4, line);
    snprintf(line, sizeof(line), "M %.2f %.2f", (model != NULL) ? model->vf_low : 0.0f, (model != NULL) ? model->vf_high : 0.0f);
    OLED_ShowStringLine(5, line);
    OLED_ShowStringLine(7, "MODE BACK");
}

void UI_ShowHint(const char *line0, const char *line1)
{
    OLED_ClearAll();
    OLED_ShowStringLine(2, line0);
    OLED_ShowStringLine(4, line1);
}
