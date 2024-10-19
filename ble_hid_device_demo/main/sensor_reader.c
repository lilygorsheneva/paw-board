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

const static char *TAG = "SENSOR";

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_6

static adc_channel_t channels[] = {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4};
static int adc_raw[10];
static int analog_values[5];
static bool pins_pressed[5];
static int thresholds[5];


static char out = 0;
static int debounce = 100;
static int  wait_to_confirm_input = 500;

unsigned int feedback_time_for_press = 100;
unsigned int feedback_time_for_input = 200;
unsigned long _feedback_times[5];


#define GPIO_OUTPUT_IO_0    11
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0))

void processInputPin(uint8_t i) {
  int value = adc_raw[i];
  analog_values[i] = value;
  if (!pins_pressed[i]) {
    if (value >= thresholds[i]) {
      pins_pressed[i] = true;
    //   _feedback_times[i] = _current_time + feedback_time_for_press;
    }
  } else {
    if (value < thresholds[i] - debounce) {
      pins_pressed[i] = false;
    //   _feedback_times[i] = 0;
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
  char out = buf - 1 + 'a';
  if (out > 'z') {
    // 11111 on the keys default maps to del. Setting everything past Z to a space.
    // TODO If you ever want a more complex mapping, just write otu a whole map.
    out = ' ';
  }
  return out;
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

void processFeedback() {
  bool vibrate[5];
  for (int i = 0; i < 5; ++i) {
    vibrate[i] = !(_feedback_times[i] >= _current_time);
  }
  writeOutputPins(vibrate);
}

char decodeAndUpdate(void) {
  static char _last_char_value = 0;
  static unsigned long _accept_input_at;

  uint64_t last_time = _current_time;

  _current_time = gettime();
  
  char out = decode();

  if (!out) {
    _last_char_value = 0;
    return 0;
  }

  if (_last_char_value != out) {
    _accept_input_at = _current_time + wait_to_confirm_input;
    _last_char_value = out;
    return 0;
  }

  if (_current_time < _accept_input_at) {
    return 0;
  }

  unsigned long feedback_until = _current_time + feedback_time_for_input;

  for (int i = 0; i < 5; ++i) {
    // TODO give this feedback on actuation.
    _feedback_times[i] = feedback_until;
  }

  _last_char_value = 0;  // TODO handle key repeat somehow.
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
      

    thresholds[0] = 400;
    thresholds[1] = 400;
    thresholds[2] = 400;
    thresholds[3] = 400;
    thresholds[4] = 400;

    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}


char pressure_sensor_read(void) {
    for (int i = 0; i < 5; ++i) {

    
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channels[i], &adc_raw[i]));
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, channels[i], adc_raw[i]);
    }    
     
        processInputPins();
        char out = decodeAndUpdate();
        processFeedback();
        ESP_LOGI(TAG, "| %4d | %4d | %4d | %4d | %4d | %c |", analog_values[0], analog_values[1], analog_values[2], analog_values[3], analog_values[4], out);

        return out;
    
}

void stop_vibration(void){
  for (int i = 0; i < 5; ++i) {
    _feedback_times[i] = 0;
  }
    gpio_set_level(GPIO_OUTPUT_IO_0, 0);
}