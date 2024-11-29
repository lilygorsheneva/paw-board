#include <stdbool.h>
#include "filter.h"

filter_handle_t default_filter;

inline void default_filter_init(filter_handle_t filter) {
    default_filter = filter;
}

void default_filter_process(uint32_t *adc_raw, bool *pins_pressed){
    default_filter->process(default_filter, adc_raw, pins_pressed);
}

void default_filter_calibrate_start(uint32_t *adc_raw){
    default_filter->calibrate_start(default_filter, adc_raw);
}

void default_filter_calibrate_end(uint32_t *adc_raw){
    default_filter->calibrate_end(default_filter, adc_raw);
}