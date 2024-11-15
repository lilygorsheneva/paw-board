#include <math.h>
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
#define JUMPERS_BIT_MASK ((1ULL << GPIO_CALIBRATION_PIN) | (1ULL << GPIO_LOGGING_PIN))

const static char *TAG = "SENSOR";

#define SENSOR_COUNT 5

static adc_channel_t channels[] = {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4};

static int thresholds[] = {900, 2000, 2300, 1800, 1000};
static int debounce[] = {100, 100, 100, 100, 100};

static int adc_raw[5];
static int analog_values[SENSOR_COUNT];
static bool pins_unstable[SENSOR_COUNT];
bool pins_pressed[SENSOR_COUNT];

static int calibration_low[SENSOR_COUNT] = {0};
static int calibration_high[SENSOR_COUNT] = {0};

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

  for (int i = 0; i < SENSOR_COUNT; ++i)
  {
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, channels[i], &config));
  }

  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = JUMPERS_BIT_MASK;
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

void processInputPinsAutoCalibrate(){
  /* Autocalibration based on a difference of two moving averages.
    If the average of the past SHORT_SAMPLE_WINDOW_LEN is greater than 
    the average of the past LONG_SAMPLE_WINDOW_LEN by more than a standard
    deviaton, consider the pin pressed.

    This is effectively a very memory hungry (1kb per finger) bandpass filter. 
    However, until I come up with a way to identify or completely zero out 
    the low-amplitude noise that remains after a bandpass filter,
    this will have to suffice. Reading DSP textbooks takes time. 
  */

  #define SHORT_SAMPLE_WINDOW_LEN 5
  #define LONG_SAMPLE_WINDOW_LEN 500
  
  
  static uint16_t _short_window[SENSOR_COUNT][SHORT_SAMPLE_WINDOW_LEN] = {0};
  static uint16_t _long_window[SENSOR_COUNT][LONG_SAMPLE_WINDOW_LEN] = {0};
  static uint16_t _short_window_idx = 0;
  static uint16_t _long_window_idx = 0;


  // All of these are NOT divided until final use to avoid accumulation of precision errors.
  static int32_t _short_average[SENSOR_COUNT] = {0};  // Divide by SHORT_SAMPLE_WINDOW_LEN before use
  static int32_t _long_average[SENSOR_COUNT] = {0};   // Divide by LONG_SAMPLE_WINDOW_LEN
  static double _long_variance[SENSOR_COUNT] = {0};  // It's complicated

  int32_t true_short_average[SENSOR_COUNT];
  int32_t true_long_average[SENSOR_COUNT];
  double true_long_stddev[SENSOR_COUNT];

  for (int i = 0; i < SENSOR_COUNT; ++i){

    _short_average[i] = _short_average[i] + adc_raw[i] - _short_window[i][_short_window_idx] ;
    _short_window[i][_short_window_idx] = adc_raw[i];

    true_short_average[i] = _short_average[i]/SHORT_SAMPLE_WINDOW_LEN;

    int newval = adc_raw[i];
    int oldval = _long_window[i][_long_window_idx];
    _long_window[i][_long_window_idx] = adc_raw[i];

    int32_t old_long_average = _long_average[i];
    int32_t new_long_average =  old_long_average + newval - oldval;
    _long_average[i] = new_long_average;
     
    true_long_average[i] = _long_average[i]/LONG_SAMPLE_WINDOW_LEN;


    // Formula from https://jonisalonen.com/2014/efficient-and-accurate-rolling-standard-deviation/

    // Since long_average is LONG_SAMPLE_WINDOW_LEN bigger than it should be, multiply the other values in the second term by that.
    _long_variance[i] =  _long_variance[i] + (newval-oldval)*(newval - new_long_average/LONG_SAMPLE_WINDOW_LEN + oldval - old_long_average/LONG_SAMPLE_WINDOW_LEN)/(LONG_SAMPLE_WINDOW_LEN-1);
    double true_long_variance = _long_variance[i];

    true_long_stddev[i] = sqrt(true_long_variance);

    pins_pressed[i] = true_short_average[i] > true_long_average[i] + true_long_stddev[i]*3;
  }
  _short_window_idx = (_short_window_idx + 1) % (SHORT_SAMPLE_WINDOW_LEN);
  _long_window_idx = (_long_window_idx + 1) % (LONG_SAMPLE_WINDOW_LEN);

  if (test_state(KEYBOARD_STATE_SENSOR_LOGGING))
  {
    ESP_LOGI(TAG, "SMOOTHED        | %4ld | %4ld | %4ld | %4ld | %4ld |", true_short_average[0],  true_short_average[1],  true_short_average[2], true_short_average[3], true_short_average[4]);
    ESP_LOGI(TAG, "AUTO THRESHOLDS | %4ld | %4ld | %4ld | %4ld | %4ld |", true_long_average[0],  true_long_average[1],  true_long_average[2], true_long_average[3], true_long_average[4]);
    ESP_LOGI(TAG, "DEVIATION       | %4f | %4lf | %4lf | %4lf | %4lf |", true_long_stddev[0],  true_long_stddev[1],  true_long_stddev[2], true_long_stddev[3], true_long_stddev[4]);
  }  
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

jumper_states_t read_jumpers(void)
{
  jumper_states_t ret = {
      .calibration = gpio_get_level(GPIO_CALIBRATION_PIN),
      .enhanced_logging = gpio_get_level(GPIO_LOGGING_PIN)};
  return ret;
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
  ESP_LOGI(TAG, "Enter calibration | %4d | %4d | %4d | %4d | %4d |", adc_raw[0], adc_raw[1], adc_raw[2], adc_raw[3], adc_raw[4]);

  for (int i = 0; i < SENSOR_COUNT; ++i)
  {
    pins_pressed[i] = false;
    calibration_low[i] = adc_raw[i];
  }
};

void calibration_end(void)
{

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

void pressure_sensor_calibration_manage()
{
  static bool last_calibration_state = false;

  bool new_calibration_state = test_state(KEYBOARD_STATE_SENSOR_CALIBRATION);

  if (new_calibration_state != last_calibration_state)
  {
    if (new_calibration_state)
    {
      calibration_start();
    }
    else
    {
      calibration_end();
    }
    last_calibration_state = new_calibration_state;
  }
}

char pressure_sensor_read(void)
{
  pressure_sensor_read_raw();
  pressure_sensor_calibration_manage();

  if (test_state(KEYBOARD_STATE_SENSOR_LOGGING))
  {
    ESP_LOGI(TAG, "SENSORLOG | %4d | %4d | %4d | %4d | %4d |", adc_raw[0], adc_raw[1], adc_raw[2], adc_raw[3], adc_raw[4]);
  }

  switch (device_state & MASK_KEYBOARD_STATE_SENSOR)
  {
  case KEYBOARD_STATE_SENSOR_NORMAL:
    //processInputPinsAutoCalibrate();
    processInputPins();
    return pressure_bits_to_num();
    break;
  case KEYBOARD_STATE_SENSOR_CALIBRATION:
    return 0;
    break;
  default:
    // This should be an error?
    return 0;
    break;
  }
  return 0;
}
