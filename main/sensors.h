#ifndef SENSORS_H__
#define SENSORS_H__

#include "constants.h"

// Still used by feedback controller. Clean up later.
extern bool pins_pressed[SENSOR_COUNT];

void sensor_init(void);

// Returns a string of 5 bits as char.
char pressure_sensor_read(void);

int pins_pressed_count(void);
bool all_pins_stable(void);

typedef struct
{
    bool calibration;
    bool enhanced_logging;
} jumper_states_t;

jumper_states_t read_jumpers(void);

#endif