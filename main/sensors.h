#ifndef SENSORS_H__
#define SENSORS_H__

// Still used by feedback controller. Clean up later.
extern bool pins_pressed[5];


void pressure_sensor_init(void);

// Returns a string of 5 bits as char.
char pressure_sensor_read(void);

int pins_pressed_count(void);
bool all_pins_stable(void);

#endif