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
#include "hid_dev.h"
#include "driver/ledc.h"


#include "haptics.h"
#include "sensors.h"
#include "encoding.h"
#include "constants.h"


const static char *TAG = "ENCODING";

static uint64_t _current_time = 0;

int64_t gettime()
{
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  uint64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
  return time_us;
}


char pressure_bits_to_num(void)
{
  char buf = 0;
  for (int i = 0; i < 5; ++i)
  {
    buf = buf + (pins_pressed[i] * (1 << i));
  }
  return buf;
}

char decode_and_feedback(void)
{
  static char _last_char_value = 0;
  static unsigned long _accept_input_at;
  static unsigned long _post_send_vibrate = 0;

  // Attempting to prevent key repeat by requriing all to be unpressed.
  static bool _sent = false;

  uint64_t last_time = _current_time;

  _current_time = gettime();

  char out = pressure_bits_to_num();

  // Nothing Pressed
  if (!out)
  {
    _last_char_value = 0;
    _sent = false;

    do_feedback(true, false);
    return 0;
  }
  if (_sent)
  {
    do_feedback(true, _post_send_vibrate > _current_time);
    return 0;
  }

  if (!all_pins_stable())
  {
    do_feedback(false, false);
    return 0;
  }

  // Avoiding key repeat by requring full release.

  // Something Pressed, but different from last input
  if (_last_char_value != out)
  {
    _accept_input_at = _current_time + WAIT_TO_CONFIRM_INPUT_MS;
    _last_char_value = out;
    do_feedback(false, false);
    return 0;
  }

  // Pressed, but not long enough to be an input.
  if (_current_time < _accept_input_at)
  {
    do_feedback(false, false);
    return 0;
  }

  // Send key.

  _sent = true;
  _post_send_vibrate = _current_time + SEND_FEEDBACK_TIME_MS;
  do_feedback(false, true);
  return out;
}

char convert_to_hid_code(char bitstring)
{

  // TODO: switch statement

  if (bitstring == 0)
  {
    return 0;
  }

  if (bitstring == 31)
  {
    return HID_KEY_ENTER;
  }
  else if (bitstring == 30)
  {
    return HID_KEY_DELETE;
  }
  else if (bitstring > 'z' - 'a' + 1)
  {
    return HID_KEY_SPACEBAR;
  }
  else
  {
    return bitstring + HID_KEY_A - 1;
  }
}
