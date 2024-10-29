#ifndef ENCODING_H__
#define ENCODING_H__

#include "state.h"

char decode_and_feedback(char pins, keyboard_state_t mode);
char convert_to_hid_code(char bitstring, keyboard_state_t mode);

#endif