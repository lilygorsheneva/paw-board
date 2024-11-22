#ifndef FILTER_H__
#define FILTER_H__
#include <stdbool.h>

typedef struct filter{

    struct filter* self;
    // Runs every sample, including during calibration.
    void (*process)(struct filter* filter_handle, int *adc_raw, bool *pins_pressed);
    // Runs at pushbutton press.
    void (*calibrate_start)(struct filter* filter_handle,  int *adc_raw);
    // Runs at pushbutton release.
    void (*calibrate_end)(struct filter* filter_handle, int *adc_raw);
    void* filter_data;
} filter;

typedef filter* filter_handle_t;

void default_filter_init(filter_handle_t filter);
void default_filter_process(int *adc_raw, bool *pins_pressed);
void default_filter_calibrate_start(int *adc_raw);
void default_filter_calibrate_end(int *adc_raw);


#endif