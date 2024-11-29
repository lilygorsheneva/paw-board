#ifndef BLUETOOTH_H__
#define BLUETOOTH_H__

//void bt_initialize(void);
void bt_send(key_mask_t mask, keyboard_cmd_t key);

void bt_passkey_process(char number);

void bt_init(void);

#endif