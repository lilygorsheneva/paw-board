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

int64_t gettime()
{
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  uint64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
  return time_us;
}

encoder_output_t envelope_encode(envelope_encoder_state *envelope_state, char pin_bitstring, keyboard_state_t mode)
{
  uint64_t _current_time = gettime();

  encoder_output_t out = {};

  if (!pin_bitstring && !envelope_state->_in_envelope)
  {
    ESP_LOGV(TAG, "No envelope | %c |", pin_bitstring + 'a' - 1);
  }
  if (pin_bitstring && !envelope_state->_in_envelope)
  {
    envelope_state->_in_envelope = true;
    out.encoder_flags = ENCODER_FLAG_ENVELOPE;
    envelope_state->_reject_envelope_at = _current_time + MAX_ENVELOPE_LENGTH_USEC;
    envelope_state->_rejected = false;
    envelope_state->_accept_input_at = _current_time + ENVELOPE_GRACE_PERIOD_USEC;
    envelope_state->_accumulated = pin_bitstring;

    ESP_LOGI(TAG, "Enter envelope %c ", envelope_state->_accumulated + 'a' - 1);
  }
  if (pin_bitstring && envelope_state->_in_envelope)
  {
    envelope_state->_accept_input_at = _current_time + ENVELOPE_GRACE_PERIOD_USEC;
    ESP_LOGV(TAG, "| %c + %c = %c|", envelope_state->_accumulated + 'a' - 1, pin_bitstring + 'a' - 1, (envelope_state->_accumulated | pin_bitstring) + 'a' - 1);
    envelope_state->_accumulated = envelope_state->_accumulated | pin_bitstring;
    out.encoder_flags = ENCODER_FLAG_ENVELOPE;
    if (_current_time >= envelope_state->_reject_envelope_at && !envelope_state->_rejected)
    {
      ESP_LOGI(TAG, "Envelope over time, rejecting.");
      envelope_state->_rejected = true;
      out.encoder_flags = ENCODER_FLAG_REJECTED;
    }
  }
  if (!pin_bitstring && envelope_state->_in_envelope)
  {
    if (_current_time >= envelope_state->_reject_envelope_at)
    {
      ESP_LOGI(TAG, "Exit envelope %c (rejected)", envelope_state->_accumulated + 'a' - 1);
      if (envelope_state->_accumulated == 31)
      {
        out.encoder_flags = ENCODER_FLAG_GRIP;
      }
      else
      {
        out.encoder_flags = ENCODER_FLAG_REJECTED;
      }
      envelope_state->_accumulated = 0;
      envelope_state->_in_envelope = false;
      envelope_state->_rejected = true;
    }
    if (_current_time >= envelope_state->_accept_input_at && !envelope_state->_rejected)
    {
      ESP_LOGI(TAG, "| %c |", envelope_state->_accumulated + 'a' - 1);
      out.accumulated_bitstring = envelope_state->_accumulated;
      ESP_LOGI(TAG, "Exit envelope %c (accepted)", envelope_state->_accumulated + 'a' - 1);
      envelope_state->_accumulated = 0;
      envelope_state->_in_envelope = false;
      envelope_state->_rejected = false;
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

void convert_to_hid_code(encoder_output_t *out, keyboard_state_t mode)
{
  char bitstring = out->accumulated_bitstring;
  char hid;
  keyboard_layout_t layout = keyboard_mode_to_layout(mode);
  switch (layout)
  {
  case KEYBOARD_LAYOUT_ALPHA:
    hid = convert_to_hid_code_alpha(bitstring);
    break;
  case KEYBOARD_LAYOUT_NUMERIC:
    hid = convert_to_hid_code_numeric(bitstring);
    break;
  default:
    ESP_LOGE(TAG, "UNKNOWN MODE %d, defaulting to alpha.", mode);
    hid = convert_to_hid_code_alpha(bitstring);
    break;
  }
  out->hid = hid;
  if (!hid)
  {
    out->encoder_flags = ENCODER_FLAG_REJECTED;
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

keyboard_system_command_t decode_command(command_decoder_state *command_state, encoder_output_t out)
{
  switch (out.encoder_flags)
  {
  case ENCODER_FLAG_GRIP:
    ESP_LOGI(TAG, "Grip detected.");
    return KEYBOARD_COMMAND_OFF;
    break;
  case ENCODER_FLAG_REJECTED:
    command_state->sequence_idx = 0;
    break;
  case ENCODER_FLAG_ACCEPTED:
    if (test_state(KEYBOARD_STATE_PAUSED))
    {
      switch (command_state->sequence_idx)
      {
      case 0:
        command_state->sequence_idx = (out.hid == HID_KEY_A) ? 1 : 0;
        break;
      case 1:
        command_state->sequence_idx = (out.hid == HID_KEY_B) ? 2 : 0;
        break;
      case 2:
        command_state->sequence_idx = (out.hid == HID_KEY_D) ? 3 : 0;
        break;
      case 3:
        command_state->sequence_idx = (out.hid == HID_KEY_H) ? 4 : 0;
        break;
      case 4:
        if (out.hid == HID_KEY_P)
        {
          command_state->sequence_idx = 0;
          return KEYBOARD_COMMAND_ON;
        }
        break;
      default:
        command_state->sequence_idx = 0;
        break;
      }
      ESP_LOGI(TAG, "Unpause sequence idx %d.", command_state->sequence_idx);
    }
  default:
    break;
  }

  return KEYBOARD_COMMAND_NONE;
}