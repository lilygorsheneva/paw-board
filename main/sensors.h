#ifndef SENSORS_H__
#define SENSORS_H__

void pressure_sensor_init(void);

extern bool pins_pressed[5];

// Returns its data in pins_pressed.
void pressure_sensor_read(void);

int pins_pressed_count(void);
bool all_pins_stable(void);

#endif