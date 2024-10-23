/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "sensor_reader.h"
#include "hid_dev.h"
#include "driver/ledc.h"

const static char *TAG = "SENSOR";

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_6

static adc_channel_t channels[] = {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4};
static int adc_raw[10];
static int analog_values[5];
static bool pins_pressed[5];
static int thresholds[5];
static int debounce[5];


static char out = 0;

//microseconds
static int wait_to_confirm_input = 300* 1000;
static int key_repeat_delay = 5000* 1000;

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (11) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4096) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY          (4000) // Frequency in Hertz. Set frequency at 4 kHz

static void example_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = (1 << 13) - 1, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

#define GPIO_OUTPUT_IO_0    11
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0))

void processInputPin(uint8_t i) {
  int value = adc_raw[i];
  analog_values[i] = value;
  if (!pins_pressed[i]) {
    if (value >= thresholds[i]) {
      pins_pressed[i] = true;
    }
  } else {
    if (value < thresholds[i] - debounce[i]) {
      pins_pressed[i] = false;
    }
  }
}

void processInputPins(void) {
  for (int i = 0; i < 5; ++i) {
    processInputPin(i);
  }
}

char decode(void) {
  char buf = 0;
  for (int i = 0; i < 5; ++i) {
    buf = buf + (pins_pressed[i] * (1 << i));
  }
  if (!buf) {
    return 0;
  }
  return buf;
}


void writeOutputPins(bool vibrate[5]) {
  // TODO multipin feedback
gpio_set_level(GPIO_OUTPUT_IO_0, vibrate[0]);
}


static uint64_t _current_time = 0;

int64_t gettime(){
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  uint64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
  return time_us;
}

char decodeAndUpdate(void) {
  static char _last_char_value = 0;
  static unsigned long _accept_input_at;
  static unsigned long _repeat_delay_threshold = 0;

  uint64_t last_time = _current_time;

  _current_time = gettime();
  
  char out = decode();

  // Nothing Pressed
  if (!out) {
    _last_char_value = 0;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, (1 << 13) -1));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

    return 0;
  }

  // Something Pressed, but different from last input
  if (_last_char_value != out) {
    _accept_input_at = _current_time + wait_to_confirm_input;
    _last_char_value = out;
    
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, (1 << 12)));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

    return 0;
  }

  // Pressed, but not long enough to be an input.
  if (_current_time < _accept_input_at) {


    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0 ));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));


    return 0;
  }

  // Send key.
    _accept_input_at = _current_time + key_repeat_delay;
  
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, (1 << 12)  ));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));



  return out;
}


static     adc_oneshot_unit_handle_t adc1_handle;

void pressure_sensor_init(void)
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = EXAMPLE_ADC_ATTEN,
    };
    
    for (int i = 0; i < 5; ++i) {
      ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, channels[i], &config));
    }
      

    thresholds[0] = 120;
    thresholds[1] = 120;
    thresholds[2] = 250;
    thresholds[3] = 80;
    thresholds[4] = 40;

    debounce[0] = 30;
    debounce[1] = 30;
    debounce[2] = 50;
    debounce[3] = 20;
    debounce[4] = 10;


    // //zero-initialize the config structure.
    // gpio_config_t io_conf = {};
    // //disable interrupt
    // io_conf.intr_type = GPIO_INTR_DISABLE;
    // //set as output mode
    // io_conf.mode = GPIO_MODE_OUTPUT;
    // //bit mask of the pins that you want to set,e.g.GPIO18/19
    // io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // //disable pull-down mode
    // io_conf.pull_down_en = 0;
    // //disable pull-up mode
    // io_conf.pull_up_en = 0;
    // //configure GPIO with the given settings
    // gpio_config(&io_conf);

    example_ledc_init();
}

char convert_to_hid_code(char in){
if (in == 31) {
  return  HID_KEY_DELETE;
} else if (in > 'z' - 'a' + 1) {
  return HID_KEY_SPACEBAR;
}
else {
  return in + HID_KEY_A - 1;
}

}

char pressure_sensor_read(void) {
    for (int i = 0; i < 5; ++i) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channels[i], &adc_raw[i]));
        //ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, channels[i], adc_raw[i]);
    }    
     
        processInputPins();
        char out = decodeAndUpdate();
        ESP_LOGI(TAG, "| %4d | %4d | %4d | %4d | %4d | %c |", analog_values[0], analog_values[1], analog_values[2], analog_values[3], analog_values[4], out + 'a' - 1);

        return convert_to_hid_code(out);
    
}


