#ifndef FILTER_H__
#define FILTER_H__
#include <stdbool.h>


typedef struct filter{
    struct filter* self;
    void (*initialize)(struct filter* filter_handle);
    void (*process)(struct filter* filter_handle, int *adc_raw, bool *pins_pressed);
    void (*calibrate_start)(struct filter* filter_handle);
    void (*calibrate_end)(struct filter* filter_handle);
    void* filter_data;
} filter;

typedef filter* filter_handle_t;

filter_handle_t init_filter(void);

#endif