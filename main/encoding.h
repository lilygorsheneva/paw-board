#ifndef ENCODING_H__
#define ENCODING_H__

#include "state.h"
#include "hid_dev.h"

typedef enum 
{
    KEYBOARD_LAYOUT_ALPHA,
    KEYBOARD_LAYOUT_NUMERIC,
} keyboard_layout_t;

typedef enum {
  ENCODER_FLAG_NONE,
  ENCODER_FLAG_ENVELOPE,
  ENCODER_FLAG_REJECTED,
  ENCODER_FLAG_ACCEPTED,
  ENCODER_FLAG_GRIP
} encoder_flags_t;

typedef struct {
    keyboard_cmd_t hid;
    encoder_flags_t encoder_flags;
} encoder_output_t;

keyboard_layout_t keyboard_mode_to_layout(keyboard_state_t mode);

encoder_output_t decode_and_feedback(char pins, keyboard_state_t mode);
keyboard_cmd_t convert_to_hid_code(char bitstring, keyboard_layout_t mode);

keyboard_system_command_t decode_command(encoder_output_t out);

#endif