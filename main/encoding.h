#ifndef ENCODING_H__
#define ENCODING_H__

#include "state.h"

typedef enum 
{
    KEYBOARD_LAYOUT_ALPHA,
    KEYBOARD_LAYOUT_NUMERIC,
} keyboard_layout_t;

keyboard_layout_t keyboard_mode_to_layout(keyboard_state_t mode);

typedef enum {
    INPUT_HANDLER_STABILITY,

    // NOT YET IMPLEMENTED
    INPUT_HANDLER_ENVELOPE,
} input_handler_t;

static const input_handler_t INPUT_HANDLER_MODE = INPUT_HANDLER_STABILITY;

char decode_and_feedback(char pins, keyboard_state_t mode);
char convert_to_hid_code(char bitstring, keyboard_layout_t mode);

#endif