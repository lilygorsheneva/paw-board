#ifndef REMOTE_CONF
#define REMOTE_CONFIG_H_

#include "esp_gatts_api.h"

void init_remote_config(void);
void* get_remote_config(void);

void remote_config_gatt_callback_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
									esp_ble_gatts_cb_param_t *param);

#endif