#include "esp_dsp.h"
#include "esp_system.h"
#include "esp_log.h"

#include "filter.h"
#include "constants.h"
#include "iir_filter.h"
#include "state.h"

const static char *TAG = "FILTER";

#ifdef ADC_COMMON_POSITIVE
#define ADC_LT_OPERATOR <
#define ADC_GE_OPERATOR >
#else
#define ADC_LT_OPERATOR >
#define ADC_GE_OPERATOR <
#endif

typedef struct
{
    float coeffs[5];
    float delay_line[SENSOR_COUNT][2];
    float thresholds[SENSOR_COUNT];
    int calibration_countdown;

    iir_filter_params params;
} iir_filter_data;

void iir_sensor_process(filter_handle_t filter_handle, int *adc_raw, bool *pins_pressed)
{
    iir_filter_data *data = (iir_filter_data *)filter_handle->filter_data;
    float out[SENSOR_COUNT];

    for (int i = 0; i < SENSOR_COUNT; ++i)
    {
        float in = (float)adc_raw[i];
        dsps_biquad_f32_ansi(&in, &(out[i]), 1, data->coeffs, data->delay_line[i]);
    }
    if (test_state(KEYBOARD_STATE_SENSOR_LOGGING))
    {
        ESP_LOGI(TAG, "FILTER_LOG | %4f | %4f | %4f | %4f | %4f |", out[0], out[1], out[2], out[3], out[4]);
    }

    if (!data->calibration_countdown)
    {
        for (int i = 0; i < SENSOR_COUNT; ++i)
        {
            if (pins_pressed[i])
            {
                // If pressed, wait for a "release".
                // I fear this may lead to the sensor getting "sticky", but it's needed for held heys.
                pins_pressed[i] = !(out[i] ADC_LT_OPERATOR - data->thresholds[i]);
            }
            else
            {
                // If not pressed, wait until it goes over the threshold.
                pins_pressed[i] = out[i] ADC_GE_OPERATOR data->thresholds[i];
            }
        }
        if (test_state(KEYBOARD_STATE_SENSOR_LOGGING))
        {
            ESP_LOGI(TAG, "PINSPRESSED | %4d | %4d | %4d | %4d | %4d |", pins_pressed[0], pins_pressed[1], pins_pressed[2], pins_pressed[3], pins_pressed[4]);
        }
    }
    else
    {
        data->calibration_countdown--;
        // Overtime represents a startup initialization delay, allowing filters to stabilize.
        if (data->calibration_countdown > (data->params.calibration_time_seconds) * (data->params.sample_rate))
        {
            return;
        }
        for (int i = 0; i < SENSOR_COUNT; ++i)
        {
            float abs = out[i] > 0 ? out[i] : -out[i];
            data->thresholds[i] = data->thresholds[i] ADC_GE_OPERATOR abs ? data->thresholds[i] : abs;
        }
        if (data->calibration_countdown == 0)
        {
            for (int i = 0; i < SENSOR_COUNT; ++i)
            {
// Not ideal but I don't know how to auto-calculate a good one at the moment.
#ifdef ADC_COMMON_POSITIVE
                data->thresholds[i] = data->thresholds[i] * data->params.calibration_peak_multiplier;
#else
                data->thresholds[i] = -data->thresholds[i] * data->params.calibration_peak_multiplier;
#endif
            }
            ESP_LOGI(TAG, "Exit IIR calibration | %4f | %4f | %4f | %4f | %4f |", data->thresholds[0], data->thresholds[1], data->thresholds[2], data->thresholds[3], data->thresholds[4]);
        }
    }
    return;
}

void iir_filter_calibration_start(filter_handle_t filter_handle, int *adc_raw)
{
    ESP_LOGI(TAG, "Enter IIR calibration");

    iir_filter_data *data = (iir_filter_data *)filter_handle->filter_data;

    // Sample N seconds of noise.
    data->calibration_countdown = (data->params.calibration_time_seconds) * (data->params.sample_rate);

    for (int i = 0; i < SENSOR_COUNT; ++i)
    {
        data->thresholds[i] = 0;
    }
}

void iir_filter_calibration_end(filter_handle_t filter_handle, int *adc_raw) {}

void *init_iir_filter(iir_filter_params *params)
{
    filter_handle_t filter_handle = malloc(sizeof(filter));
    filter_handle->self = filter_handle;
    filter_handle->process = iir_sensor_process;
    filter_handle->calibrate_start = iir_filter_calibration_start;
    filter_handle->calibrate_end = iir_filter_calibration_end;

    iir_filter_data *data = calloc(1, sizeof(iir_filter_data));
    filter_handle->filter_data = (void *)data;
    data->params = *params;

    float freq = params->target_frequency / params->sample_rate;
    float qFactor = params->qfactor;

    esp_err_t err = ESP_OK;

    // Calculate iir filter coefficients
    err = dsps_biquad_gen_bpf_f32(data->coeffs, freq, qFactor);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Operation error = %i", err);
        return NULL;
    }

    data->calibration_countdown = (data->params.calibration_time_seconds + 2) * (data->params.sample_rate);

    return filter_handle;
}
