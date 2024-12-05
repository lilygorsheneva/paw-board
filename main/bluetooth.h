#ifndef BLUETOOTH_H__
#define BLUETOOTH_H__

void bt_send(key_mask_t mask, keyboard_cmd_t key);

void bt_passkey_process(char number);

void bt_init(void);

esp_err_t ble_register_profile(uint16_t app_id, esp_gatts_cb_t callback);

#endif