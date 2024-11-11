#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

#include "sensors.h"
#include "state.h"

#define FORCE_ANALOG_LOG false

// 17 chosen to not conflict with multi-motor feedback. In the future, this may be better as an rtc gpio
#define GPIO_CALIBRATION_PIN 17

#define GPIO_LOGGING_PIN 18

const static char *TAG = "SENSOR";

#define SENSOR_COUNT 5

static adc_channel_t channels[] = {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4};

static int thresholds[] = {900, 2000, 2300, 1800, 1000};
static int debounce[] = {100, 100, 100, 100, 100};

static int adc_raw[10];
static int analog_values[SENSOR_COUNT];
static bool pins_unstable[SENSOR_COUNT];
bool pins_pressed[SENSOR_COUNT];

static int calibration_low[SENSOR_COUNT] = {0};
static int calibration_high[SENSOR_COUNT] = {0};

static adc_oneshot_unit_handle_t adc1_handle;
void pressure_sensor_init(void)
{
  enter_state(KEYBOARD_STATE_SENSOR_NORMAL);
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_6,
  };

  for (int i = 0; i < SENSOR_COUNT; ++i)
  {
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, channels[i], &config));
  }

  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << GPIO_CALIBRATION_PIN) | (1ULL << GPIO_LOGGING_PIN);
  io_conf.pull_down_en = 1;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

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

  pins_unstable[i] = (fabsf((float)(value - analog_values[i]) / (float)(analog_values[i])) > 0.1);

  if (value >= thresholds[i])
  {
    pins_pressed[i] = true;
  }
  else if (value < thresholds[i] - debounce[i])
  {
    pins_pressed[i] = false;
  }

  analog_values[i] = value;
}

void processInputPins(void)
{
  for (int i = 0; i < SENSOR_COUNT; ++i)
  {
    processInputPinStabilityMode(i);
  }
}

int pins_pressed_count(void)
{
  char buf = 0;

  for (int i = 0; i < SENSOR_COUNT; ++i)
  {
    buf = buf + pins_pressed[i];
  }
  return buf;
}

bool all_pins_stable(void)
{
  bool unstable = false;
  for (int i = 0; i < SENSOR_COUNT; ++i)
  {
    unstable = unstable & pins_unstable[i];
  }
  return !unstable;
}

char pressure_bits_to_num(void)
{
  char buf = 0;
  for (int i = 0; i < SENSOR_COUNT; ++i)
  {
    buf = buf + (pins_pressed[i] * (1 << i));
  }
  return buf;
}

inline bool read_calibration_button(void)
{
  return gpio_get_level(GPIO_CALIBRATION_PIN);
}

inline bool read_logging_jumper(void)
{
  return gpio_get_level(GPIO_LOGGING_PIN);
}

void pressure_sensor_read_raw(void)
{
  for (int i = 0; i < SENSOR_COUNT; ++i)
  {
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channels[i], &adc_raw[i]));
  }
}

void calibration_start(void)
{
  enter_state(KEYBOARD_STATE_SENSOR_CALIBRATION);

  pressure_sensor_read_raw();

  ESP_LOGI(TAG, "Enter calibration | %4d | %4d | %4d | %4d | %4d |", adc_raw[0], adc_raw[1], adc_raw[2], adc_raw[3], adc_raw[4]);

  for (int i = 0; i < SENSOR_COUNT; ++i)
  {

    pins_pressed[i] = false;
    calibration_low[i] = adc_raw[i];
  }
};

void calibration_end(void)
{
  enter_state(KEYBOARD_STATE_SENSOR_NORMAL);

  pressure_sensor_read_raw();

  ESP_LOGI(TAG, "Exit calibration | %4d | %4d | %4d | %4d | %4d |", adc_raw[0], adc_raw[1], adc_raw[2], adc_raw[3], adc_raw[4]);

  for (int i = 0; i < SENSOR_COUNT; ++i)
  {
    calibration_high[i] = adc_raw[i];

    thresholds[i] = (calibration_high[i] + calibration_low[i]) / 2;
    debounce[i] = abs(calibration_high[i] - calibration_low[i]) / 8;
  }
  ESP_LOGI(TAG, "Thresholds | %4d | %4d | %4d | %4d | %4d |", thresholds[0], thresholds[1], thresholds[2], thresholds[3], thresholds[4]);
  ESP_LOGI(TAG, "Debounce | %4d | %4d | %4d | %4d | %4d |", debounce[0], debounce[1], debounce[2], debounce[3], debounce[4]);
};

void pressure_sensor_state_manage()
{
  bool calibration_button = read_calibration_button();
  bool logging_jumper = read_logging_jumper();

  switch (device_state & MASK_KEYBOARD_STATE_SENSOR)
  {
  case KEYBOARD_STATE_SENSOR_NORMAL:
    if (logging_jumper)
    {
      enter_state(KEYBOARD_STATE_SENSOR_LOGGING);
    }
    if (calibration_button)
    {
      calibration_start();
    }
    break;
  case KEYBOARD_STATE_SENSOR_CALIBRATION:
    if (!calibration_button)
      calibration_end();
    break;
  case KEYBOARD_STATE_SENSOR_LOGGING:
    if (!logging_jumper)
    {
      enter_state(KEYBOARD_STATE_SENSOR_NORMAL);
    }
    break;
  default:
    break;
  }
}

char pressure_sensor_read(void)
{
  pressure_sensor_state_manage();
  switch (device_state & MASK_KEYBOARD_STATE_SENSOR)
  {
  case KEYBOARD_STATE_SENSOR_NORMAL:
    pressure_sensor_read_raw();
    processInputPins();
    return pressure_bits_to_num();
    break;
  case KEYBOARD_STATE_SENSOR_CALIBRATION:
    return 0;
    break;
  case KEYBOARD_STATE_SENSOR_LOGGING:
    pressure_sensor_read_raw();
    ESP_LOGI(TAG, "SENSORLOG | %4d | %4d | %4d | %4d | %4d |", adc_raw[0], adc_raw[1], adc_raw[2], adc_raw[3], adc_raw[4]);
    return 0;
    break;
  default:
    return 0;
    break;
  }
  return 0;
}
