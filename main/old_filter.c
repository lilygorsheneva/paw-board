#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "filter.h"
#include "constants.h"
#include "old_filter.h"

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
    int thresholds[SENSOR_COUNT];
    int debounce[SENSOR_COUNT];
    int calibration_high[SENSOR_COUNT];
    int calibration_low[SENSOR_COUNT];
} old_filter_data;

// Naive approach to handling a read. Hardcoded threshold with
// a small debouce window later.
void processInputPins(struct filter* filter_handle, int *adc_raw, bool *pins_pressed)
{
    old_filter_data *data = (old_filter_data *)filter_handle->filter_data;

    for (int i = 0; i < SENSOR_COUNT; ++i)
    {
        int value = adc_raw[i];
        if (!pins_pressed[i])
        {
            if (value ADC_GE_OPERATOR data->thresholds[i])
            {
                pins_pressed[i] = true;
            }
        }
        else
        {
            if (value ADC_LT_OPERATOR data->thresholds[i] - data->debounce[i])
            {
                pins_pressed[i] = false;
            }
        }
    }
}

void calibration_start(filter_handle_t filter_handle, int *adc_raw)
{
    old_filter_data *data = (old_filter_data *)filter_handle->filter_data;

    ESP_LOGI(TAG, "Enter calibration | %4d | %4d | %4d | %4d | %4d |", adc_raw[0], adc_raw[1], adc_raw[2], adc_raw[3], adc_raw[4]);

    for (int i = 0; i < SENSOR_COUNT; ++i)
    {
        data->calibration_low[i] = adc_raw[i];
    }
};

void calibration_end(filter_handle_t filter_handle, int *adc_raw)
{
    old_filter_data *data = (old_filter_data *)filter_handle->filter_data;


    ESP_LOGI(TAG, "Exit calibration | %4d | %4d | %4d | %4d | %4d |", adc_raw[0], adc_raw[1], adc_raw[2], adc_raw[3], adc_raw[4]);

    for (int i = 0; i < SENSOR_COUNT; ++i)
    {
        data->calibration_high[i] = adc_raw[i];

        data->thresholds[i] = (data->calibration_high[i] + data->calibration_low[i]) / 2;
        data->debounce[i] = abs(data->calibration_high[i] - data->calibration_low[i]) / 8;
    }
    ESP_LOGI(TAG, "Thresholds | %4d | %4d | %4d | %4d | %4d |", data->thresholds[0], data->thresholds[1], data->thresholds[2], data->thresholds[3], data->thresholds[4]);
    ESP_LOGI(TAG, "Debounce | %4d | %4d | %4d | %4d | %4d |", data->debounce[0], data->debounce[1], data->debounce[2], data->debounce[3], data->debounce[4]);
};


void* init_old_filter(void* params)
{
    filter_handle_t filter_handle = malloc(sizeof(filter));
    filter_handle->self = filter_handle;
    filter_handle->process = processInputPins;
    filter_handle->calibrate_start = calibration_start;
    filter_handle->calibrate_end = calibration_end;

    old_filter_data *data = calloc(1, sizeof(old_filter_data));
    
    const int default_thresholds[] = {900, 2000, 2300, 1800, 1000};
    const int default_debounce[] = {100, 100, 100, 100, 100};
    memcpy(data->thresholds, default_thresholds, sizeof(default_thresholds));
    memcpy(data->debounce, default_debounce, sizeof(default_debounce));

    filter_handle->filter_data = (void *)data;
    return filter_handle;
}
