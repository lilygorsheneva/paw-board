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

#include "constants.h"
#include "sensors.h"
#include "state.h"
#include "filter.h"

#define FORCE_ANALOG_LOG false

#define JUMPERS_BIT_MASK ((1ULL << GPIO_CALIBRATION_PIN) | (1ULL << GPIO_LOGGING_PIN))

const static char *TAG = "SENSOR";

static adc_channel_t adc_channels[] = SENSOR_ADC_CHANNELS;
static uint8_t digital_sensors[] = DIGITAL_SENSORS;

static uint32_t adc_raw[SENSOR_COUNT];
bool pins_pressed[SENSOR_COUNT] = {0};

#ifdef JUMPERS_COMMON_POSITIVE
#define JUMPERS_SIGN_OPERATOR
#else
#define JUMPERS_SIGN_OPERATOR !
#endif

#ifdef DIGITAL_COMMON_POSITIVE
#define DIGITAL_BUTTON_SIGN_OPERATOR
#else
#define DIGITAL_BUTTON_SIGN_OPERATOR !
#endif

static adc_oneshot_unit_handle_t adc1_handle;

void jumpers_init(void)
{
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = JUMPERS_BIT_MASK;
  io_conf.pull_down_en = JUMPERS_SIGN_OPERATOR 1;
  io_conf.pull_up_en = JUMPERS_SIGN_OPERATOR 0;
  gpio_config(&io_conf);
}

void digital_button_init(void)
{
  uint64_t bit_mask = 0;
  for (int i = 0; i < DIGITAL_SENSOR_COUNT; ++i)
  {
    bit_mask |= (1ULL << digital_sensors[i]);
  }
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = bit_mask;
  io_conf.pull_down_en = DIGITAL_BUTTON_SIGN_OPERATOR 1;
  io_conf.pull_up_en = DIGITAL_BUTTON_SIGN_OPERATOR 0;
  gpio_config(&io_conf);
}

void pressure_sensor_init(void)
{
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTENUATION,
  };

  for (int i = 0; i < ADC_SENSOR_COUNT; ++i)
  {
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc_channels[i], &config));
  }
}

void sensor_init(void)
{
  jumpers_init();
  pressure_sensor_init();
  digital_button_init();
  ESP_LOGI(TAG, "Init sensors");
}

#ifdef ADC_COMMON_POSITIVE
#define ADC_LT_OPERATOR <
#define ADC_GE_OPERATOR >
#else
#define ADC_LT_OPERATOR >
#define ADC_GE_OPERATOR <
#endif

int pins_pressed_count(void)
{
  if (test_state(KEYBOARD_STATE_SENSOR_CALIBRATION))
  {
    return 0;
  }
  char buf = 0;

  for (int i = 0; i < SENSOR_COUNT; ++i)
  {
    buf = buf + pins_pressed[i];
  }
  return buf;
}

char pressure_bits_to_num(void)
{
  char buf = 0;
  for (int i = 0; i < ENCODING_SENSOR_COUNT; ++i)
  {
    buf = buf + (pins_pressed[i] * (1 << i));
  }
  return buf;
}

jumper_states_t read_jumpers(void)
{

  bool calibration = JUMPERS_SIGN_OPERATOR gpio_get_level(GPIO_CALIBRATION_PIN);
  bool logging = JUMPERS_SIGN_OPERATOR gpio_get_level(GPIO_LOGGING_PIN);
  jumper_states_t ret = {
      .calibration = calibration,
      .enhanced_logging = logging};
  return ret;
}

void pressure_sensor_read_raw(void)
{
  for (int i = 0; i < ADC_SENSOR_COUNT; ++i)
  {
    int tmp;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc_channels[i], &tmp));
    adc_raw[i] = tmp;
  }
}

void digital_sensor_read_raw(void)
{
  for (int i = 0; i < DIGITAL_SENSOR_COUNT; ++i)
  {
    // Multiplier here is a hack to get around filters having a minimum threshold value.
    // That should be configurable per-channel or bypassed entirely, but i have a demo to do soon.
    adc_raw[i + ADC_SENSOR_COUNT] = 100*(gpio_get_level(digital_sensors[i]));
  }
}

void pressure_sensor_calibration_manage()
{
  static bool last_calibration_state = false;

  bool new_calibration_state = test_state(KEYBOARD_STATE_SENSOR_CALIBRATION);

  if (new_calibration_state != last_calibration_state)
  {
    if (new_calibration_state)
    {
      default_filter_calibrate_start(adc_raw);
    }
    else
    {
      default_filter_calibrate_end(adc_raw);
    }
    last_calibration_state = new_calibration_state;
  }
}

char pressure_sensor_read(void)
{
  pressure_sensor_read_raw();
  digital_sensor_read_raw();
  pressure_sensor_calibration_manage();
  default_filter_process(adc_raw, pins_pressed);

  if (test_state(KEYBOARD_STATE_SENSOR_LOGGING))
  {
    ESP_LOGI(TAG, "SENSORLOG | %4ld | %4ld | %4ld | %4ld | %4ld | %4ld | %4ld | %4ld | %4ld | %4ld |", adc_raw[0], adc_raw[1], adc_raw[2], adc_raw[3], adc_raw[4], adc_raw[5], adc_raw[6], adc_raw[7], adc_raw[8], adc_raw[9]);
  }

  switch (device_state & MASK_KEYBOARD_STATE_SENSOR)
  {
  case KEYBOARD_STATE_SENSOR_NORMAL:
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
