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
#include "state.h"

const static char *TAG = "ENCODING";

static uint64_t _current_time = 0;

int64_t gettime()
{
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  uint64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
  return time_us;
}

encoder_output_t decode_and_feedback(char pins, keyboard_state_t mode)
{
  static bool _in_envelope = false;
  static char _accumulated = 0;
  static bool _rejected = false;

  static unsigned long _accept_input_at;
  static unsigned long _reject_envelope_at;

  const unsigned long MAX_ENVELOPE_LENGTH_USEC = 2000000;
  const unsigned long ENVELOPE_GRACE_PERIOD_USEC = 100000;

  _current_time = gettime();

  encoder_output_t out = {};

  if (!pins && !_in_envelope)
  {
    ESP_LOGV(TAG, "No envelope | %c |", pins + 'a' - 1);
  }
  if (pins && !_in_envelope)
  {
    _in_envelope = true;
    out.encoder_flags = ENCODER_FLAG_ENVELOPE;
    _reject_envelope_at = _current_time + MAX_ENVELOPE_LENGTH_USEC;
    _rejected = false;
    _accept_input_at = _current_time + ENVELOPE_GRACE_PERIOD_USEC;
    _accumulated = pins;

    ESP_LOGI(TAG, "Enter envelope %c ", _accumulated + 'a' - 1);
  }
  if (pins && _in_envelope)
  {
    _accept_input_at = _current_time + ENVELOPE_GRACE_PERIOD_USEC;
    ESP_LOGV(TAG, "| %c + %c = %c|", _accumulated + 'a' - 1, pins + 'a' - 1, (_accumulated | pins) + 'a' - 1);
    _accumulated = _accumulated | pins;
    out.encoder_flags = ENCODER_FLAG_ENVELOPE;
    if (_current_time >= _reject_envelope_at && !_rejected)
    {
      ESP_LOGI(TAG, "Envelope over time, rejecting.");
      _rejected = true;
      out.encoder_flags = ENCODER_FLAG_REJECTED;
    }
  }
  if (!pins && _in_envelope)
  {
    if (_current_time >= _reject_envelope_at)
    {
      ESP_LOGI(TAG, "Exit envelope %c (rejected)", _accumulated + 'a' - 1);
      if (_accumulated == 31)
      {
        out.encoder_flags = ENCODER_FLAG_GRIP;
      }
      else
      {
        out.encoder_flags = ENCODER_FLAG_REJECTED;
      }
      _accumulated = 0;
      _in_envelope = false;
      _rejected = true;
    }
    if (_current_time >= _accept_input_at && !_rejected)
    {
      ESP_LOGI(TAG, "| %c |", _accumulated + 'a' - 1);
      out.hid = convert_to_hid_code(_accumulated, keyboard_mode_to_layout(mode));
      ESP_LOGI(TAG, "Exit envelope %c (accepted)", _accumulated + 'a' - 1);
      _accumulated = 0;
      _in_envelope = false;
      _rejected = false;
      if (out.hid)
      {
        out.encoder_flags = ENCODER_FLAG_ACCEPTED;
      }
      else
      {
        out.encoder_flags = ENCODER_FLAG_REJECTED;
      }
    }
  }
  return out;
}

keyboard_cmd_t convert_to_hid_code_alpha(char bitstring);
keyboard_cmd_t convert_to_hid_code_numeric(char bitstring);

keyboard_layout_t keyboard_mode_to_layout(keyboard_state_t mode)
{
  switch (mode & (MASK_KEYBOARD_STATE_BT))
  {
  case KEYBOARD_STATE_BT_CONNECTED:
    return KEYBOARD_LAYOUT_ALPHA;
  case KEYBOARD_STATE_BT_UNCONNECTED:
    return KEYBOARD_LAYOUT_ALPHA;
  case KEYBOARD_STATE_BT_PASSKEY_ENTRY:
    return KEYBOARD_LAYOUT_NUMERIC;
  default:
    ESP_LOGE(TAG, "UNKNOWN STATE %d, defaulting to alpha.", mode);
    return KEYBOARD_LAYOUT_ALPHA;
  }
}

keyboard_cmd_t convert_to_hid_code(char bitstring, keyboard_layout_t mode)
{
  switch (mode)
  {
  case KEYBOARD_LAYOUT_ALPHA:
    return convert_to_hid_code_alpha(bitstring);
  case KEYBOARD_LAYOUT_NUMERIC:
    return convert_to_hid_code_numeric(bitstring);
  default:
    ESP_LOGE(TAG, "UNKNOWN MODE %d, defaulting to alpha.", mode);
    return convert_to_hid_code_alpha(bitstring);
  }
}

keyboard_cmd_t convert_to_hid_code_numeric(char bitstring)
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
  case 31:
    return HID_KEY_ENTER;
  default:
    return 0;
  };
}

keyboard_cmd_t convert_to_hid_code_alpha(char bitstring)
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
    return HID_KEY_CAPS_LOCK;
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

keyboard_system_command_t decode_command(encoder_output_t out)
{
  static int sequence_idx = 0;

  switch (out.encoder_flags)
  {
  case ENCODER_FLAG_GRIP:
    ESP_LOGI(TAG, "Grip detected.");
    return KEYBOARD_COMMAND_OFF;
    break;
  case ENCODER_FLAG_REJECTED:
    sequence_idx = 0;
    break;
  case ENCODER_FLAG_ACCEPTED:
    if (test_state(KEYBOARD_STATE_PAUSED))
    {
      switch (sequence_idx)
      {
      case 0:
        sequence_idx = (out.hid == HID_KEY_A) ? 1 : 0;
        break;
      case 1:
        sequence_idx = (out.hid == HID_KEY_B) ? 2 : 0;
        break;
      case 2:
        sequence_idx = (out.hid == HID_KEY_D) ? 3 : 0;
        break;
      case 3:
        sequence_idx = (out.hid == HID_KEY_H) ? 4 : 0;
        break;
      case 4:
        if (out.hid == HID_KEY_P)
        {
          sequence_idx = 0;
          return KEYBOARD_COMMAND_ON;
        }
        break;
      default:
        sequence_idx = 0;
        break;
      }
      ESP_LOGI(TAG, "Unpause sequence idx %d.", sequence_idx);
    }
  default:
    break;
  }

  return KEYBOARD_COMMAND_NONE;
}