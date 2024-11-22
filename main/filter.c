#include "filter.h"
#include "constants.h"

#include "esp_dsp.h"
#include "esp_system.h"
#include "esp_log.h"

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
} iir_filter_data;

void init_iir_filter(filter_handle_t filter_handle)
{
    iir_filter_data* data = calloc(1, sizeof(iir_filter_data));
    filter_handle->filter_data = (void*)data;

    float freq = 15 / 500;
    float qFactor = 1.0;

    esp_err_t err = ESP_OK;

    // Calculate iir filter coefficients
    err = dsps_biquad_gen_lpf_f32(data->coeffs, freq, qFactor);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Operation error = %i", err);
        return;
    }

    data->calibration_countdown = 500;
}

void iir_sensor_process(filter_handle_t filter_handle, int *adc_raw, bool *pins_pressed)
{
    iir_filter_data *data = (iir_filter_data*) filter_handle->filter_data;
    float out[SENSOR_COUNT];

    for (int i = 0; i < SENSOR_COUNT; ++i)
    {
        float in = (float)adc_raw[i];
        dsps_biquad_f32_ansi(&in, &(out[i]), 1, data->coeffs, data->delay_line[i]);
    }

    if (!data->calibration_countdown)
    {
        for (int i = 0; i < SENSOR_COUNT; ++i)
        {
            pins_pressed[i] = out[i] ADC_GE_OPERATOR data->thresholds[i];
            return;
        }
    }
    else
    {
        for (int i = 0; i < SENSOR_COUNT; ++i)
        {
            data->thresholds[i] = data->thresholds[i] ADC_GE_OPERATOR out[i] ? data->thresholds[i] : out[i];
        }
        data->calibration_countdown--;
        if (data->calibration_countdown == 0)
        {
            for (int i = 0; i < SENSOR_COUNT; ++i)
            {
                data->thresholds[i] = data->thresholds[i] * 2;
            }
        }
    }
    return;
}

void iir_filter_calibration_start(filter_handle_t filter_handle)
{
    iir_filter_data *data = (iir_filter_data*) filter_handle->filter_data;
    data->calibration_countdown = 500;
}

void iir_filter_calibration_end(filter_handle_t filter_handle) {}



filter_handle_t init_filter(void){
    filter_handle_t handle = malloc(sizeof(filter));
    handle->self = handle;
    handle->initialize = init_iir_filter;
    handle->process = iir_sensor_process;
    handle->calibrate_start = iir_filter_calibration_start;
    handle->calibrate_end = iir_filter_calibration_end;
    handle->initialize(handle);

    return handle;
}
