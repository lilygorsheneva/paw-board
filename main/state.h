#ifndef STATE_H__
#define STATE_H__

typedef int keyboard_state_t;

extern keyboard_state_t device_state;

enum 
{
    // BT stack states
    KEYBOARD_STATE_BT_UNCONNECTED =      1 << 0,
    KEYBOARD_STATE_BT_PASSKEY_ENTRY =    1 << 1,
    KEYBOARD_STATE_BT_CONNECTED =        1 << 2,

    KEYBOARD_STATE_SENSOR_NORMAL =      1 << 3,
    KEYBOARD_STATE_SENSOR_CALIBRATION =      1 << 4,
    KEYBOARD_STATE_SENSOR_LOGGING = 1 << 5,

    KEYBOARD_STATE_PAUSED = 1 << 6,
};

typedef enum {
    KEYBOARD_COMMAND_NONE=0,
    KEYBOARD_COMMAND_OFF,
    KEYBOARD_COMMAND_ON,
} keyboard_system_command_t;

typedef enum 
{
    HAPTICS_MODE_SINGLE,
    HAPTICS_MODE_SCALING,
    HAPTICS_MODE_FIVE,
} haptics_mode_t;

static const haptics_mode_t HAPTICS_MODE = HAPTICS_MODE_SINGLE;

#define MASK_KEYBOARD_STATE_BT (KEYBOARD_STATE_BT_UNCONNECTED | KEYBOARD_STATE_BT_PASSKEY_ENTRY | KEYBOARD_STATE_BT_CONNECTED)
#define MASK_KEYBOARD_STATE_SENSOR (KEYBOARD_STATE_SENSOR_NORMAL | KEYBOARD_STATE_SENSOR_CALIBRATION)

void update_state(keyboard_system_command_t command);

// Allow BT stack to set its state async, polling loop will sync occasionally.
void update_bt_state(keyboard_state_t new_state);

inline bool test_state(keyboard_state_t state)
{
    return (device_state & state) != 0;
};


#endif