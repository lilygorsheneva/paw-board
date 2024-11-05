#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

// INIT CODE
#include "demo_main.h"

// SEND FUNCTION
#include "esp_hidd_prf_api.h"

#include "haptics.h"
#include "sensors.h"
#include "encoding.h"
#include "constants.h"
#include "state.h"
#include "bluetooth.h"

const static char *TAG = "MAIN";

keyboard_state_t device_state = KEYBOARD_STATE_UNCONNECTED;

void hid_task(void *pvParameters)
{
    char out;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while (1)
    {
        vTaskDelay(POLLING_PERIOD_MS / portTICK_PERIOD_MS);
        char pins = pressure_sensor_read();
        out = decode_and_feedback(pins, device_state);

        switch (device_state)
        {
        case KEYBOARD_STATE_CONNECTED:
            if (out)
            {
                bt_send(out);
            }
            break;
        case KEYBOARD_STATE_PASSKEY_ENTRY:
            if (out)
            {
                // This should really be using out instead of pins for consistency. However, that would require mapping HID codes back to numbers.
                bt_passkey_process(pins);
            }
        default:
            break;
        }
    }
}

void app_main(void)
{
    bt_init();

    pressure_sensor_init();
    initialize_feedback();

    ESP_LOGI(TAG, "Start main loop");

    xTaskCreate(&hid_task, "hid_task", 8000, NULL, 5, NULL);
}