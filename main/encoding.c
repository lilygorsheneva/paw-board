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

// Key mapping
#include "hid_dev.h"

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

char decode_and_feedback(char pins)
{
  static char _last_char_value = 0;
  static unsigned long _accept_input_at;
  static unsigned long _post_send_vibrate = 0;

  // Attempting to prevent key repeat by requriing all to be unpressed.
  static bool _sent = false;

  _current_time = gettime();

  // Nothing Pressed
  if (!pins)
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
  if (_last_char_value != pins)
  {
    _accept_input_at = _current_time + WAIT_TO_CONFIRM_INPUT_MS * 1000;
    _last_char_value = pins;
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
  _post_send_vibrate = _current_time + SEND_FEEDBACK_TIME_MS * 1000;
  do_feedback(false, true);

  ESP_LOGI(TAG, "| %c |", pins);
  return convert_to_hid_code(pins, ALPHA);
}

char convert_to_hid_code_alpha(char bitstring);
char convert_to_hid_code_numeric(char bitstring);

char convert_to_hid_code(char bitstring, keyboard_mode_t mode)
{
  switch (mode)
  {
  default:
    return convert_to_hid_code_alpha(bitstring);
  }
}

char convert_to_hid_code_numeric(char bitstring)
{
  switch (bitstring)
  {
  case 1:
    return HID_KEY_1;
  case 2:
    return HID_KEY_2;
  case 3:
    return HID_KEY_3;
  case 4:
    return HID_KEY_4;
  case 5:
    return HID_KEY_5;
  case 6:
    return HID_KEY_6;
  case 7:
    return HID_KEY_7;
  case 8:
    return HID_KEY_8;
  case 9:
    return HID_KEY_9;
  case 10:
    return HID_KEY_0;
  default:
    return 0;
  };
}

char convert_to_hid_code_alpha(char bitstring)
{
  switch (bitstring)
  {
  case 1:
    return HID_KEY_A;
  case 2:
    return HID_KEY_B;
  case 3:
    return HID_KEY_C;
  case 4:
    return HID_KEY_D;
  case 5:
    return HID_KEY_E;
  case 6:
    return HID_KEY_F;
  case 7:
    return HID_KEY_G;
  case 8:
    return HID_KEY_H;
  case 9:
    return HID_KEY_I;
  case 10:
    return HID_KEY_J;
  case 11:
    return HID_KEY_K;
  case 12:
    return HID_KEY_L;
  case 13:
    return HID_KEY_M;
  case 14:
    return HID_KEY_N;
  case 15:
    return HID_KEY_O;
  case 16:
    return HID_KEY_P;
  case 17:
    return HID_KEY_Q;
  case 18:
    return HID_KEY_R;
  case 19:
    return HID_KEY_S;
  case 20:
    return HID_KEY_T;
  case 21:
    return HID_KEY_U;
  case 22:
    return HID_KEY_V;
  case 23:
    return HID_KEY_W;
  case 24:
    return HID_KEY_X;
  case 25:
    return HID_KEY_Y;
  case 26:
    return HID_KEY_Z;

  case 27:
    return HID_KEY_SPACEBAR;
  case 28:
    return HID_KEY_SPACEBAR;
  case 29:
    return HID_KEY_SPACEBAR;

  case 30:
    return HID_KEY_DELETE;
  case 31:
    return HID_KEY_ENTER;

  default:
    return 0;
  };
}
