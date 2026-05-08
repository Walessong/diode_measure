#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define ADC_AVG_TIMES           16U
#define ADC_VREF                3.3f
#define ADC_FULL_SCALE          4095.0f
/*
 * PA4/PA5 按推荐硬件改为 33k / 10k 分压后采样：
 * Vadc = Vnode * 10k / (33k + 10k)
 * 因此软件还原倍率约为 4.3
 */
#define ADC_DIVIDER_RATIO       (10.0f / 43.0f)
#define ADC_INPUT_SCALE         (1.0f / ADC_DIVIDER_RATIO)

#define RELAY_SETTLE_MS         20U
#define ANALOG_SETTLE_MS        8U
#define MEASURE_REPEAT_TIMES    3U
#define MEASURE_SCAN_AVG_TIMES  3U
#define COUNT_SCAN_AVG_TIMES    5U
#define LEARN_SCAN_AVG_TIMES    7U
#define LEARN_SCAN_TRIM_COUNT   1U
#define RESULT_HOLD_MS          1800U
#define DETAILED_RESULT_HOLD_MS 2600U

#define SENSE_RES_LOW_OHM       390.0f
#define SENSE_RES_HIGH_OHM      100.0f
#define SENSE_VALID_MIN_V       0.05f
#define FIXED_DROP_LOW_V        0.20f
#define FIXED_DROP_HIGH_V       0.30f

#define VF_VALID_MIN            0.10f
#define VF_VALID_MAX            10.50f
#define POLARITY_SCORE_MARGIN   0.02f
#define WEAK_SENSE_MIN_V        0.02f
#define COUNT_MAX_SERIES        10U
#define DIODE_PRESENT_MIN_V     0.08f
#define MODEL_VF_DELTA_MAX      1.20f
#define COUNT_TOL_ONE_TWO       0.10f
#define COUNT_TOL_NORMAL        0.18f
#define COUNT_TOL_HIGH_SERIES   0.24f
#define COUNT_TOL_HIGH_RELAX    0.45f
#define SERIES_FIT_BIAS_V       0.145f
#define SERIES_STEP_MIN_V       0.18f
#define SERIES_FIT_TOL_V        0.20f

#define RELAY_ACTIVE_LEVEL      GPIO_PIN_SET
#define RELAY_INACTIVE_LEVEL    GPIO_PIN_RESET

#endif
