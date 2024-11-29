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
    float coeffs[SENSOR_COUNT][5];
    float delay_line[SENSOR_COUNT][2];
    float thresholds[SENSOR_COUNT];

    int consecutive_movement[SENSOR_COUNT];

    int calibration_countdown;

    iir_filter_params params;
} iir_filter_data;

void iir_filter_process_filter(iir_filter_data *data, uint32_t *adc_raw, float *out)
{

    // Actual filtering.
    for (int i = 0; i < SENSOR_COUNT; ++i)
    {
        float in = (float)adc_raw[i];
        dsps_biquad_f32_ansi(&in, &(out[i]), 1, data->coeffs[i], data->delay_line[i]);
    }
    if (test_state(KEYBOARD_STATE_SENSOR_LOGGING))
    {
        ESP_LOGI(TAG, "FILTER_LOG | %4f | %4f | %4f | %4f | %4f | %4f | %4f | %4f | %4f | %4f |", out[0], out[1], out[2], out[3], out[4],  out[5], out[6], out[7], out[8], out[9]);
    }
}

void iir_filter_process_normal(iir_filter_data *data, float *filtered_data, bool *pins_pressed)
{
    for (int i = 0; i < SENSOR_COUNT; ++i)
    {
        if (pins_pressed[i])
        {
            // If pressed, wait for a "release".
            // I fear this may lead to the sensor getting "sticky", but it's needed for held heys.
            bool releasing = filtered_data[i] ADC_LT_OPERATOR(-data->thresholds[i]);
            data->consecutive_movement[i] = releasing ? data->consecutive_movement[i] + 1 : 0;

            if (data->consecutive_movement[i] > data->params.debounce_count)
            {
                pins_pressed[i] = false;
                data->consecutive_movement[i] = 0;
            }
        }
        else
        {
            // If not pressed, wait until it goes over the threshold.
            bool pressing = filtered_data[i] ADC_GE_OPERATOR data->thresholds[i];
            data->consecutive_movement[i] = pressing ? data->consecutive_movement[i] + 1 : 0;
            if (data->consecutive_movement[i] > data->params.debounce_count)
            {
                pins_pressed[i] = true;
                data->consecutive_movement[i] = 0;
            }
        }
    }
}

void iir_filter_process_calibration(iir_filter_data *data, float *filtered_data)
{
    // Overtime represents a startup initialization delay, allowing filters to stabilize.
    if (data->calibration_countdown > (data->params.calibration_time_seconds) * (data->params.sample_rate))
    {
        return;
    }
    for (int i = 0; i < SENSOR_COUNT; ++i)
    {
        float abs = filtered_data[i] > 0 ? filtered_data[i] : -filtered_data[i];
        data->thresholds[i] = data->thresholds[i] ADC_GE_OPERATOR abs ? data->thresholds[i] : abs;
    }
    if (data->calibration_countdown == 0)
    {
        for (int i = 0; i < SENSOR_COUNT; ++i)
        {
// Not ideal but I don't know how to auto-calculate a good one at the moment.
#ifdef ADC_COMMON_POSITIVE
            data->thresholds[i] = data->thresholds[i] * data->params.calibration_peak_multiplier + data->params.min_threshold;
#else
            data->thresholds[i] = -data->thresholds[i] * data->params.calibration_peak_multiplier + data->params.min_threshold;
#endif
            data->consecutive_movement[i] = 0;
        }
        ESP_LOGI(TAG, "Exit IIR calibration | %4f | %4f | %4f | %4f | %4f |", data->thresholds[0], data->thresholds[1], data->thresholds[2], data->thresholds[3], data->thresholds[4]);
    }
}
void iir_sensor_process(filter_handle_t filter_handle, uint32_t *adc_raw, bool *pins_pressed)
{
    iir_filter_data *data = (iir_filter_data *)filter_handle->filter_data;
    float filtered_data[SENSOR_COUNT];

    iir_filter_process_filter(data, adc_raw, filtered_data);

    if (!data->calibration_countdown)
    {
        iir_filter_process_normal(data, filtered_data, pins_pressed);
    }
    else
    {
        data->calibration_countdown--;
        for (int i = 0; i < SENSOR_COUNT; ++i)
        {
            pins_pressed[i] = false;
        }
        iir_filter_process_calibration(data, filtered_data);
    }
    return;
}

void iir_filter_calibration_start(filter_handle_t filter_handle, uint32_t *adc_raw)
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

void iir_filter_calibration_end(filter_handle_t filter_handle, uint32_t *adc_raw) {}

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

    esp_err_t err = ESP_OK;

    // Calculate iir filter coefficients
    for (int i = 0; i < SENSOR_COUNT; ++i)
    {
        float freq = params->target_frequency[i] / params->sample_rate;
        float qFactor = params->qfactor[i];
        err = dsps_biquad_gen_bpf_f32(data->coeffs[i], freq, qFactor);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Operation error = %i", err);
            return NULL;
        }
    }

    data->calibration_countdown = (data->params.calibration_time_seconds + 2) * (data->params.sample_rate);

    return filter_handle;
}
