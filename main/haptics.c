#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "constants.h"
#include "haptics.h"
#include "sensors.h"

const static char *TAG = "HAPTICS";

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO (11) // Define the output GPIO
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY (4096)                // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY (4000)           // Frequency in Hertz. Set frequency at 4 kHz

//  Single-motor pwm

static void single_pwm_init(void)
{
  // Prepare and then apply the LEDC PWM timer configuration
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_MODE,
      .timer_num = LEDC_TIMER,
      .duty_resolution = LEDC_DUTY_RES,
      .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
      .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // Prepare and then apply the LEDC PWM channel configuration
  ledc_channel_config_t ledc_channel = {
      .speed_mode = LEDC_MODE,
      .channel = LEDC_CHANNEL,
      .timer_sel = LEDC_TIMER,
      .intr_type = LEDC_INTR_DISABLE,
      .gpio_num = SINGLE_MOTOR_GPIO_OUTPUT,
      .duty = (1 << 13) - 1, // Set duty to 0%
      .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  ESP_LOGI(TAG, "Init single-mode feedback");
}

void scale_vibration_to_pincount(bool force_off, bool force_full)
{
  uint32_t duty = (1 << 13) - 1;
  if (force_off)
  {
    #ifdef FEEDBACK_COMMON_POSITIVE
    duty = (1 << 13) - 1;
    #else 
    duty = 0;
    #endif
  }
  else if (force_full)
  {
    #ifdef FEEDBACK_COMMON_POSITIVE
    duty = 0;
    #else 
    duty = (1 << 13) - 1;
    #endif
  }
  else
  {
    int c = pins_pressed_count();
    const uint32_t duty_increment = (1 << 13) / 12 - 1;
    #ifdef FEEDBACK_COMMON_POSITIVE
    duty = (6 - c) * duty_increment;
    #else
    duty = (6 + c) * duty_increment;
    #endif
  }

  ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

// Multi-motor per-finger feedback. Not currently PWM.


#define GPIO_OUTPUT_PIN_SEL ((1ULL << MULTI_MOTOR_GPIO_OUTPUT_0)) | ((1ULL << MULTI_MOTOR_GPIO_OUTPUT_1)) | ((1ULL << MULTI_MOTOR_GPIO_OUTPUT_2)) | ((1ULL << MULTI_MOTOR_GPIO_OUTPUT_3)) | ((1ULL << MULTI_MOTOR_GPIO_OUTPUT_4))

static gpio_num_t output_gpio_ids[] = {MULTI_MOTOR_GPIO_OUTPUT_0, MULTI_MOTOR_GPIO_OUTPUT_1, MULTI_MOTOR_GPIO_OUTPUT_2, MULTI_MOTOR_GPIO_OUTPUT_3, MULTI_MOTOR_GPIO_OUTPUT_4};

static void multi_gpio_init(void)
{
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  // disable pull-down mode
  io_conf.pull_down_en = 0;
  // disable pull-up mode
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  ESP_LOGI(TAG, "Init multi-mode feedback");
}

void multi_motor_feedback(bool force_off, bool force_full)
{
#ifdef FEEDBACK_COMMON_POSITIVE
#define FEEDBACK_SIGN_OPERATOR !
#else
#define FEEDBACK_SIGN_OPERATOR 
#endif

  if (force_off)
  {
    for (int i = 0; i < 5; ++i)
    {
      gpio_set_level(output_gpio_ids[i], FEEDBACK_SIGN_OPERATOR 0);
    }
  }
  else if (force_full)
  {
    for (int i = 0; i < 5; ++i)
    {
      gpio_set_level(output_gpio_ids[i], FEEDBACK_SIGN_OPERATOR 1);
    }
  }
  else
  {
    for (int i = 0; i < 5; ++i)
    {
      gpio_set_level(output_gpio_ids[i], FEEDBACK_SIGN_OPERATOR pins_pressed[i]);
    }
  }
}

void do_feedback(bool force_off, bool force_full)
{
  switch (HAPTICS_MODE)
  {
  case HAPTICS_MODE_FIVE:
    multi_motor_feedback(force_off, force_full);
    break;
  case HAPTICS_MODE_SINGLE:
    scale_vibration_to_pincount(force_off, force_full);
    break;
  default:
    scale_vibration_to_pincount(force_off, force_full);
    break;
  }
}

void initialize_feedback(void)
{
  switch (HAPTICS_MODE)
  {
  case HAPTICS_MODE_FIVE:
    multi_gpio_init();
    break;
  case HAPTICS_MODE_SINGLE:
    single_pwm_init();
    break;
  default:
    single_pwm_init();
    break;
  }
}
