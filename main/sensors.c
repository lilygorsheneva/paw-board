#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

#include "sensors.h"

#define FORCE_ANALOG_LOG false

const static char *TAG = "SENSOR";

static adc_channel_t channels[] = {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4};
static int thresholds[] = {20, 20, 20, 20, 20}; 
static int debounce[] = {10,10,10,10,10};


static int adc_raw[10];
static int analog_values[5];
static bool pins_unstable[5];
bool pins_pressed[5];


static adc_oneshot_unit_handle_t adc1_handle;
void pressure_sensor_init(void)
{
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_6,
  };

  for (int i = 0; i < 5; ++i)
  {
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, channels[i], &config));
  }

  ESP_LOGI(TAG, "Init adc");
}

// Naive approach to handling a read. Hardcoded threshold with
// a small debouce window later.
void processInputPin(uint8_t i)
{
  int value = adc_raw[i];
  analog_values[i] = value;
  if (!pins_pressed[i])
  {
    if (value >= thresholds[i])
    {
      pins_pressed[i] = true;
    }
  }
  else
  {
    if (value < thresholds[i] - debounce[i])
    {
      pins_pressed[i] = false;
    }
  }
}

// A more sophisticated pin read mode. Track pin fluctuations.
void processInputPinStabilityMode(uint8_t i)
{
  // Maybe this can be dynamic
  const int noise_floor = 10;

  int value = adc_raw[i];
  if (value <= noise_floor)
  {
    pins_pressed[i] = false;
    pins_unstable[i] = false;
    analog_values[i] = value;
    return;
  }

  if (value >= thresholds[i])
  {
    if (fabsf((float)(value - analog_values[i]) / (float)(analog_values[i])) <= 0.1)
    {
      pins_pressed[i] = true;
      pins_unstable[i] = false;
    }
    if (fabsf((float)(value - analog_values[i]) / (float)(analog_values[i])) >= 0.1)
    {
      pins_unstable[i] = true;
      pins_pressed[i] = true;
    }
  }

  analog_values[i] = value;
}

void processInputPins(void)
{
  for (int i = 0; i < 5; ++i)
  {
    processInputPinStabilityMode(i);
  }
}

int pins_pressed_count(void)
{
  char buf = 0;

  for (int i = 0; i < 5; ++i)
  {
    buf = buf + pins_pressed[i];
  }
  return buf;
}


bool all_pins_stable(void)
{
  bool unstable = false;
  for (int i = 0; i < 5; ++i)
  {
    unstable = unstable & pins_unstable[i];
  }
  return !unstable;
}

char pressure_bits_to_num(void)
{
  char buf = 0;
  for (int i = 0; i < 5; ++i)
  {
    buf = buf + (pins_pressed[i] * (1 << i));
  }
  return buf;
}

char pressure_sensor_read(void)
{
  for (int i = 0; i < 5; ++i)
  {
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channels[i], &adc_raw[i]));
  }

  processInputPins();

  char out = pressure_bits_to_num();

  if (analog_values[0] || analog_values[1] || analog_values[2] || analog_values[3] || analog_values[4] || FORCE_ANALOG_LOG)
  {
    ESP_LOGI(TAG, "| %4d | %4d | %4d | %4d | %4d | %c |", analog_values[0], analog_values[1], analog_values[2], analog_values[3], analog_values[4], out + 'a' - 1);
  }

  return out;
}
