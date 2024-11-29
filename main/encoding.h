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
    // Temporary data used during a single iteration.
    keyboard_cmd_t hid;
    char accumulated_bitstring;
    encoder_flags_t encoder_flags;
} encoder_output_t;


typedef struct {
  bool _in_envelope;
  char _accumulated;
  bool _rejected;

unsigned long _accept_input_at;
 unsigned long _reject_envelope_at;

} envelope_encoder_state;

typedef struct {
  int sequence_idx;
} command_decoder_state;

keyboard_layout_t keyboard_mode_to_layout(keyboard_state_t mode);

encoder_output_t envelope_encode(envelope_encoder_state* envelope_state, char pin_bitstring, keyboard_state_t mode);
void convert_to_hid_code(encoder_output_t* out, keyboard_state_t mode);

keyboard_system_command_t decode_command(command_decoder_state* command_state, encoder_output_t out);

#endif