
#ifndef SENSOR_READER_H__
#define SENSOR_READER_H__


void pressure_sensor_init(void);
char pressure_sensor_read(void);
void stop_vibration(void);
static int polling_period = 500;
#endif