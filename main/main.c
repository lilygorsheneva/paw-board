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

#define MAIN_TAG "MAIN"

void hid_task(void *pvParameters)
{
    char out;
    uint8_t key_value[] = {0};
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while(1) {
        vTaskDelay(POLLING_PERIOD_MS / portTICK_PERIOD_MS);
        out = decode_and_feedback();


        if (sec_conn && out) {
            ESP_LOGI(MAIN_TAG, "Send key");
            key_value[0] = out;
            esp_hidd_send_keyboard_value(hid_conn_id, 0, key_value, 1);
            esp_hidd_send_keyboard_value(hid_conn_id, 0, NULL, 0);
        } else {
        }
    }
}


void app_main(void){
    demo_main();

    pressure_sensor_init();
    initialize_feedback();  

    xTaskCreate(&hid_task, "hid_task", 8000, NULL, 5, NULL);
}