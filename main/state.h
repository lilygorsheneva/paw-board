#ifndef STATE_H__
#define STATE_H__

typedef enum
{
  KEYBOARD_MODE_ALPHA,
  KEYBOARD_MODE_NUMERIC,
} keyboard_mode_t;

typedef enum 
{
    KEYBOARD_STATE_UNCONNECTED,
    KEYBOARD_STATE_PASSKEY_ENTRY,
    KEYBOARD_STATE_CONNECTED
} keyboard_state_t;

extern keyboard_state_t device_state;

#endif