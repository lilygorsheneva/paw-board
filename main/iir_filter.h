#ifndef IIR_FILTER_H__
#define IIR_FILTER_H__

typedef struct {
    float sample_rate;
    float target_frequency;
    float qfactor;
    float calibration_peak_multiplier;
} iir_filter_params;

void* init_iir_filter(iir_filter_params* params);
#endif
