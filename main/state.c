#include <stdlib.h>
#include <stdbool.h>

#include "state.h"

keyboard_state_t device_state = 0;


inline void _enter_state(keyboard_state_t new_state);
inline void _exit_state(keyboard_state_t new_state);

void enter_state(keyboard_state_t new_state)
{

    switch (new_state)
    {
    case KEYBOARD_STATE_BT_UNCONNECTED:
    case KEYBOARD_STATE_BT_CONNECTED:
    case KEYBOARD_STATE_BT_PASSKEY_ENTRY:
        _exit_state(KEYBOARD_STATE_BT_UNCONNECTED);
        _exit_state(KEYBOARD_STATE_BT_CONNECTED);
        _exit_state(KEYBOARD_STATE_BT_PASSKEY_ENTRY);
        _enter_state(new_state);
        break;

    case KEYBOARD_STATE_SENSOR_NORMAL:
    case KEYBOARD_STATE_SENSOR_CALIBRATION:
    case KEYBOARD_STATE_SENSOR_LOGGING:
        _exit_state(KEYBOARD_STATE_SENSOR_NORMAL);
        _exit_state(KEYBOARD_STATE_SENSOR_CALIBRATION);
        _exit_state(KEYBOARD_STATE_SENSOR_LOGGING);
        _enter_state(new_state);
        break;

    default:
        _enter_state(new_state);
        break;
    }
}

inline void _enter_state(keyboard_state_t new_state)
{
    device_state |= new_state;
};

inline void _exit_state(keyboard_state_t new_state)
{
    device_state &= ~new_state;
};

