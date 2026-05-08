#include "app.h"

#include <string.h>

#include "app_config.h"
#include "bsp_key.h"
#include "identify.h"
#include "learn.h"
#include "measure.h"
#include "ui.h"

typedef struct
{
    AppMode_t mode;
    uint8_t menu_index;
    uint8_t result_showing;
    uint32_t result_deadline_ms;
    SingleMeasureResult_t last_scan;
} AppContext_t;

static AppContext_t g_app;
static volatile uint32_t g_app_ms = 0U;

static void App_SortFloatArray(float *values, uint8_t count)
{
    uint8_t i;
    uint8_t j;

    for (i = 0U; i < count; i++)
    {
        for (j = (uint8_t)(i + 1U); j < count; j++)
        {
            if (values[j] < values[i])
            {
                float temp = values[i];
                values[i] = values[j];
                values[j] = temp;
            }
        }
    }
}

static uint8_t App_BuildRobustLearnScan(SingleMeasureResult_t *scan_out)
{
    SingleMeasureResult_t scan;
    float low_values[LEARN_SCAN_AVG_TIMES];
    float high_values[LEARN_SCAN_AVG_TIMES];
    uint8_t valid_count = 0U;
    uint8_t i;
    uint8_t start;
    uint8_t end;
    float low_sum = 0.0f;
    float high_sum = 0.0f;
    float low_avg;
    float high_avg;

    if (scan_out == NULL)
    {
        return 0U;
    }

    memset(scan_out, 0, sizeof(*scan_out));
    for (i = 0U; i < LEARN_SCAN_AVG_TIMES; i++)
    {
        Measure_ScanSingleDetailed(&scan);
        if ((scan.valid != 0U) &&
            (scan.display_state == DISPLAY_STATE_FORWARD_ON) &&
            (scan.vf_high_corrected > 0.0f))
        {
            low_values[valid_count] = scan.vf_low_corrected;
            high_values[valid_count] = scan.vf_high_corrected;
            valid_count++;
        }
        HAL_Delay(5U);
    }

    if (valid_count < (uint8_t)(LEARN_SCAN_TRIM_COUNT * 2U + 1U))
    {
        return 0U;
    }

    App_SortFloatArray(low_values, valid_count);
    App_SortFloatArray(high_values, valid_count);
    start = LEARN_SCAN_TRIM_COUNT;
    end = (uint8_t)(valid_count - LEARN_SCAN_TRIM_COUNT);
    for (i = start; i < end; i++)
    {
        low_sum += low_values[i];
        high_sum += high_values[i];
    }

    low_avg = low_sum / (float)(end - start);
    high_avg = high_sum / (float)(end - start);

    Measure_ScanSingleDetailed(scan_out);
    scan_out->valid = 1U;
    scan_out->quality = MEASURE_QUALITY_VALID;
    scan_out->display_state = DISPLAY_STATE_FORWARD_ON;
    scan_out->vf = low_avg;
    scan_out->vf_low_corrected = low_avg;
    scan_out->vf_high_corrected = high_avg;
    return 1U;
}

static void App_ShowMenu(void)
{
    const DiodeModel_t *model = Learn_GetSingleModel();

    UI_ShowMenu(g_app.menu_index, model->valid);
}

static void App_ShowTimedPage(uint32_t hold_ms)
{
    g_app.result_showing = 1U;
    g_app.result_deadline_ms = g_app_ms + hold_ms;
}

static void App_BackToMenu(void)
{
    g_app.result_showing = 0U;
    App_ShowMenu();
}

static void App_RunPolarity(void)
{
    uint8_t ui_dir;

    UI_ShowHint("MEAS...", "POLARITY");
    Measure_ScanSingle(&g_app.last_scan);
    if (g_app.last_scan.valid == 0U)
    {
        ui_dir = 0xFFU;
    }
    else
    {
        ui_dir = g_app.last_scan.dir0_forward;
    }
    UI_ShowPolarityResult(ui_dir);
    App_ShowTimedPage(RESULT_HOLD_MS);
}

static void App_RunSingleVf(void)
{
    UI_ShowHint("MEAS...", "SINGLE VF");
    Measure_ScanSingle(&g_app.last_scan);
    UI_ShowAutoSingle(&g_app.last_scan);
    App_ShowTimedPage(RESULT_HOLD_MS);
}

static void App_RunLearn(void)
{
    LearnResult_t result;

    memset(&result, 0, sizeof(result));
    UI_ShowHint("LEARNING", "WAIT...");
    if (App_BuildRobustLearnScan(&g_app.last_scan) == 0U)
    {
        result.status = LEARN_STATUS_UNSTABLE;
    }
    else
    {
        result.status = Learn_BuildModel(&g_app.last_scan, &result.model);
    }
    if (result.status == LEARN_STATUS_OK)
    {
        if (Learn_SaveSingleModel(&result.model) == 0U)
        {
            result.status = LEARN_STATUS_FAIL;
        }
    }
    UI_ShowLearnResult(&result);
    App_ShowTimedPage(DETAILED_RESULT_HOLD_MS);
}

static void App_RunCount(void)
{
    CountResult_t result;

    UI_ShowHint("COUNTING", "WAIT...");
    result.status = Identify_CountSameType(Learn_GetSingleModel(), &result);
    UI_ShowCountResult(&result);
    App_ShowTimedPage(DETAILED_RESULT_HOLD_MS);
}

static void App_RunDiag(void)
{
    Measure_ScanSingleDetailed(&g_app.last_scan);
    UI_ShowDiagInfo(&g_app.last_scan, Learn_GetSingleModel());
    App_ShowTimedPage(DETAILED_RESULT_HOLD_MS);
}

static void App_ExecuteCurrentMode(void)
{
    switch (g_app.mode)
    {
    case APP_MODE_BASIC_POL:
        App_RunPolarity();
        break;
    case APP_MODE_BASIC_VF:
        App_RunSingleVf();
        break;
    case APP_MODE_LEARN_SINGLE:
        App_RunLearn();
        break;
    case APP_MODE_COUNT_SINGLE:
        App_RunCount();
        break;
    case APP_MODE_DIAG_INFO:
        App_RunDiag();
        break;
    case APP_MODE_MENU:
    default:
        App_ShowMenu();
        break;
    }
}

static void App_StepMode(void)
{
    g_app.menu_index = (uint8_t)((g_app.menu_index + 1U) % 5U);
    g_app.mode = (AppMode_t)(g_app.menu_index + 1U);
    App_ShowMenu();
}

void App_Init(void)
{
    memset(&g_app, 0, sizeof(g_app));
    Learn_Init();
    g_app.mode = APP_MODE_BASIC_POL;
    g_app.menu_index = 0U;

    UI_ShowBoot();
    HAL_Delay(600U);
    App_ShowMenu();
}

void App_Task(void)
{
    KeyEvent_t event = Key_GetEvent();

    if ((g_app.result_showing != 0U) && ((int32_t)(g_app_ms - g_app.result_deadline_ms) >= 0))
    {
        App_BackToMenu();
    }

    if (event == KEY_EVENT_NONE)
    {
        return;
    }

    if (g_app.result_showing != 0U)
    {
        if (event == KEY_EVENT_MODE)
        {
            App_BackToMenu();
        }
        return;
    }

    switch (event)
    {
    case KEY_EVENT_MODE:
        App_StepMode();
        break;
    case KEY_EVENT_LEARN:
        if (g_app.mode == APP_MODE_LEARN_SINGLE)
        {
            App_RunLearn();
        }
        break;
    case KEY_EVENT_MEAS:
        App_ExecuteCurrentMode();
        break;
    default:
        break;
    }
}

void App_Tick1ms(void)
{
    g_app_ms++;
}
