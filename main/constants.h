#ifndef CONSTANTS_H__
#define CONSTANTS_H__

#include "hal/adc_types.h"

static const int POLLING_PERIOD_MS = 50;
static const int WAIT_TO_CONFIRM_INPUT_MS = 300;
static const int SEND_FEEDBACK_TIME_MS = 300;

// ADC on pins labeled on Arduino as A0,A1,A2,A3,D2
#define SENSOR_ADC_CHANNELS {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4}
#define ADC_ATTENUATION ADC_ATTEN_DB_2_5

// Selected to not conflict with feedback selections.
// Calibration pin may be better on an RTC gpio to be used as a wake button in the distant future.
#define GPIO_CALIBRATION_PIN 17 //D8
#define GPIO_LOGGING_PIN 18     //D9


// Feedback on A4
#define SINGLE_MOTOR_GPIO_OUTPUT 11

#define MULTI_MOTOR_GPIO_OUTPUT_0 10  //D7
#define MULTI_MOTOR_GPIO_OUTPUT_1 11  //A4
#define MULTI_MOTOR_GPIO_OUTPUT_2 12  //A5
#define MULTI_MOTOR_GPIO_OUTPUT_3 13  //A6
#define MULTI_MOTOR_GPIO_OUTPUT_4 14  //A7

// Select positive/negative for pins. Defaults to common ground

#define ADC_COMMON_POSITIVE
#define FEEDBACK_COMMON_POSITIVE
#define JUMPERS_COMMON_POSITIVE

#endif
