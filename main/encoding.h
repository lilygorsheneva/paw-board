#ifndef ENCODING_H__
#define ENCODING_H__

char decode_and_feedback(char pins, keyboard_mode_t mode);
char convert_to_hid_code(char bitstring, keyboard_mode_t mode);

#endif