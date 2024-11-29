#ifndef IIR_FILTER_H__
#define IIR_FILTER_H__

typedef struct {
    float sample_rate;
    float target_frequency[SENSOR_COUNT];
    float qfactor[SENSOR_COUNT];
    float calibration_peak_multiplier;
    float calibration_time_seconds;

    // extra smoothing
    int debounce_count;

    // for sensors that somehow read zero when idle
    float min_threshold;
} iir_filter_params;

void* init_iir_filter(iir_filter_params* params);
#endif
