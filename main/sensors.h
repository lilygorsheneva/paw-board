#ifndef SENSORS_H__
#define SENSORS_H__

void pressure_sensor_init(void);

// Returns a string of 5 bits as char.
char pressure_sensor_read(void);

int pins_pressed_count(void);
bool all_pins_stable(void);

#endif