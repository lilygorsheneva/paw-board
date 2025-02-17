#ifndef CONSTANTS_H__
#define CONSTANTS_H__

#include "hal/adc_types.h"
#include "hal/touch_sensor_types.h"


#define POLLING_PERIOD_MS = 50;
#define WAIT_TO_CONFIRM_INPUT_MS = 300;
#define SEND_FEEDBACK_TIME_MS = 300;

#define MAX_ENVELOPE_LENGTH_USEC 2000000;
#define ENVELOPE_GRACE_PERIOD_USEC  100000;


#define ADC_SENSOR_COUNT 10
#define DIGITAL_SENSOR_COUNT 0

#define ENCODING_SENSOR_COUNT 5
#define MODIFIER_SENSOR_COUNT 5
#define SENSOR_COUNT ENCODING_SENSOR_COUNT + MODIFIER_SENSOR_COUNT 


// ADC on pins labeled on Arduino as A0,A1,A2,A3,D7; D2,D3,D4,D5,D6
#define SENSOR_ADC_CHANNELS {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_9, ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6, ADC_CHANNEL_7, ADC_CHANNEL_8}
#define ADC_ATTENUATION ADC_ATTEN_DB_2_5
// Select positive/negative for pins. Defaults to common ground
#define ADC_COMMON_POSITIVE

// Technically, these shouldn't go through filtering. However, held button filters shouldn't interfere with them.
#define DIGITAL_SENSORS {}
// Select positive/negative for pins. Defaults to common ground
#define DIGITAL_COMMON_POSITIVE


// Selected to not conflict with feedback selections.
// Calibration pin may be better on an RTC gpio to be used as a wake button in the distant future.
#define GPIO_CALIBRATION_PIN 11 //A4
#define GPIO_LOGGING_PIN 18     //D9
// Select positive/negative for pins. Defaults to common ground
// #define JUMPERS_COMMON_POSITIVE

// Feedback on A5
#define SINGLE_MOTOR_GPIO_OUTPUT 12

#define MULTI_MOTOR_GPIO_OUTPUT_0 10  //D7
#define MULTI_MOTOR_GPIO_OUTPUT_1 11  //A4
#define MULTI_MOTOR_GPIO_OUTPUT_2 12  //A5
#define MULTI_MOTOR_GPIO_OUTPUT_3 13  //A6
#define MULTI_MOTOR_GPIO_OUTPUT_4 14  //A7

// Select positive/negative for pins. Defaults to common ground
// #define FEEDBACK_COMMON_POSITIVE

//#define DISABLECTRLALTWIN

#endif
