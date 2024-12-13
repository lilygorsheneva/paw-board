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
#include "filter.h"
#include "iir_filter.h"

const static char *TAG = "MAIN";

envelope_encoder_state encoder_state;
command_decoder_state command_state;

keyboard_system_command_t last_command;
keyboard_cmd_t last_tx_key;
key_mask_t last_tx_mask;

void hid_task(void *pvParameters)
{

    encoder_output_t out;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while (1)
    {
        update_state(last_command);

        // Polling period of 10ms to work with the fixed window sizes used by autocalibration.
        // Any future filtering attempts should use polling frequency when setting thresholds.
        vTaskDelay(1);

        // if (test_state(KEYBOARD_STATE_SENSOR_LOGGING)) {
        //     vTaskDelay(1);
        // } else {
        //     vTaskDelay(POLLING_PERIOD_MS / portTICK_PERIOD_MS);
        // }
        char pins = pressure_sensor_read();
        out = envelope_encode(&encoder_state, pins, device_state);
        convert_to_hid_code(&out, device_state);
        do_feedback(out.encoder_flags);
        last_command = decode_command(&command_state, out);

        switch (device_state)
        // Not KEYBOARD_STATE_PAUSED.
        // Move the logic into BT itself, maybe?
        {
        case KEYBOARD_STATE_BT_CONNECTED | KEYBOARD_STATE_SENSOR_NORMAL:
            // This check definitely needs to be in a BT wrapper.
            if (!((out.mask == last_tx_mask) && (out.hid == last_tx_key)))
            {
                last_tx_key = out.hid;
                last_tx_mask= out.mask;
                bt_send(out.mask, out.hid);
            }
            break;
        case KEYBOARD_STATE_BT_PASSKEY_ENTRY | KEYBOARD_STATE_SENSOR_NORMAL:
            if (out.hid)
            {
                bt_passkey_process(out.hid);
            }
        default:
            break;
        }
    }
}

void app_main(void)
{
    bt_init();
    sensor_init();


    default_filter_init(init_iir_filter_default());

    initialize_feedback();

    ESP_LOGI(TAG, "Start main loop");

    xTaskCreate(&hid_task, "hid_task", 8000, NULL, 5, NULL);
}