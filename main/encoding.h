#ifndef ENCODING_H__
#define ENCODING_H__

typedef enum
{
  ALPHA,
  NUMERIC,
} keyboard_mode_t;

char decode_and_feedback(char pins);
char convert_to_hid_code(char bitstring, keyboard_mode_t mode);

#endif