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

static uint8_t App_IsLearnVfValid(float vf)
{
    return ((vf >= VF_VALID_MIN) && (vf <= VF_VALID_MAX)) ? 1U : 0U;
}

static uint8_t App_BuildRobustLearnScan(SingleMeasureResult_t *scan_out)
{
    SingleMeasureResult_t scan;
    float low_values[LEARN_SCAN_AVG_TIMES];
    float high_values[LEARN_SCAN_AVG_TIMES];
    uint8_t low_count = 0U;
    uint8_t high_count = 0U;
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
            (App_IsLearnVfValid(scan.vf_low_corrected) != 0U))
        {
            low_values[low_count] = scan.vf_low_corrected;
            low_count++;

            if (App_IsLearnVfValid(scan.vf_high_corrected) != 0U)
            {
                high_values[high_count] = scan.vf_high_corrected;
                high_count++;
            }
        }
        HAL_Delay(5U);
    }

    if ((low_count < LEARN_LOW_MIN_SAMPLES) ||
        (low_count < (uint8_t)(LEARN_SCAN_TRIM_COUNT * 2U + 1U)))
    {
        return 0U;
    }

    App_SortFloatArray(low_values, low_count);
    start = LEARN_SCAN_TRIM_COUNT;
    end = (uint8_t)(low_count - LEARN_SCAN_TRIM_COUNT);
    for (i = start; i < end; i++)
    {
        low_sum += low_values[i];
    }
    low_avg = low_sum / (float)(end - start);

    if ((high_count >= LEARN_HIGH_MIN_SAMPLES) &&
        (high_count >= (uint8_t)(LEARN_SCAN_TRIM_COUNT * 2U + 1U)))
    {
        App_SortFloatArray(high_values, high_count);
        start = LEARN_SCAN_TRIM_COUNT;
        end = (uint8_t)(high_count - LEARN_SCAN_TRIM_COUNT);
        for (i = start; i < end; i++)
        {
            high_sum += high_values[i];
        }
        high_avg = high_sum / (float)(end - start);
    }
    else
    {
        high_avg = low_avg;
    }

    Measure_ScanSingleDetailed(scan_out);
    scan_out->valid = 1U;
    scan_out->quality = MEASURE_QUALITY_VALID;
    scan_out->vf = low_avg;
    scan_out->vf_low_corrected = low_avg;
    scan_out->vf_high_corrected = high_avg;
    return 1U;
}

static void App_ShowMenu(void)
{
    UI_ShowMenu(g_app.menu_index,
                Learn_IsModelValid(MODEL_SLOT_SINGLE),
                Learn_IsModelValid(MODEL_SLOT_A),
                Learn_IsModelValid(MODEL_SLOT_B),
                Learn_IsModelValid(MODEL_SLOT_C));
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

static void App_RunLearnSlot(ModelSlot_t slot, const char *title)
{
    LearnResult_t result;

    memset(&result, 0, sizeof(result));
    UI_ShowHint("LEARNING", title);
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
        if (Learn_SaveModel(slot, &result.model) == 0U)
        {
            result.status = LEARN_STATUS_FAIL;
        }
    }
    UI_ShowLearnResult(slot, &result);
    App_ShowTimedPage(DETAILED_RESULT_HOLD_MS);
}

static void App_RunLearnSingle(void)
{
    App_RunLearnSlot(MODEL_SLOT_SINGLE, "REF");
}

static void App_RunLearnA(void)
{
    App_RunLearnSlot(MODEL_SLOT_A, "MODEL A");
}

static void App_RunLearnB(void)
{
    App_RunLearnSlot(MODEL_SLOT_B, "MODEL B");
}

static void App_RunLearnC(void)
{
    App_RunLearnSlot(MODEL_SLOT_C, "MODEL C");
}

static void App_RunCount(void)
{
    CountResult_t result;

    UI_ShowHint("COUNTING", "REF");
    result.status = Identify_CountSameType(Learn_GetSingleModel(), &result);
    UI_ShowCountResult(&result);
    App_ShowTimedPage(DETAILED_RESULT_HOLD_MS);
}

static void App_RunCountAbc(void)
{
    CountAbcResult_t result;

    UI_ShowHint("COUNTING", "A/B/C");
    result.status = Identify_CountABC(Learn_GetModel(MODEL_SLOT_A),
                                      Learn_GetModel(MODEL_SLOT_B),
                                      Learn_GetModel(MODEL_SLOT_C),
                                      &result);
    UI_ShowCountAbcResult(&result);
    App_ShowTimedPage(DETAILED_RESULT_HOLD_MS);
}

static void App_RunDiag(void)
{
    FaultMeasureResult_t fault_sample;

    Measure_ScanSingleDetailed(&g_app.last_scan);
    (void)Measure_GetFaultSample(&fault_sample);
    UI_ShowDiagInfo(&g_app.last_scan, &fault_sample, Learn_GetSingleModel());
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
    case APP_MODE_COUNT_SINGLE:
        App_RunCount();
        break;
    case APP_MODE_COUNT_ABC:
        App_RunCountAbc();
        break;
    case APP_MODE_DIAG_INFO:
        App_RunDiag();
        break;
    case APP_MODE_LEARN_SINGLE:
    case APP_MODE_LEARN_A:
    case APP_MODE_LEARN_B:
    case APP_MODE_LEARN_C:
    case APP_MODE_MENU:
    default:
        App_ShowMenu();
        break;
    }
}

static void App_RunLearnForCurrentMode(void)
{
    switch (g_app.mode)
    {
    case APP_MODE_LEARN_SINGLE:
        App_RunLearnSingle();
        break;
    case APP_MODE_LEARN_A:
        App_RunLearnA();
        break;
    case APP_MODE_LEARN_B:
        App_RunLearnB();
        break;
    case APP_MODE_LEARN_C:
        App_RunLearnC();
        break;
    default:
        break;
    }
}

static void App_StepMode(void)
{
    g_app.menu_index = (uint8_t)((g_app.menu_index + 1U) % 9U);
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
        App_RunLearnForCurrentMode();
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
