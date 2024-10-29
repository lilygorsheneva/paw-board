#ifndef BLUETOOTH_H__
#define BLUETOOTH_H__

//void bt_initialize(void);
void bt_send(char key);

void bt_passkey_process(char pins);

void bt_init(void);

#endif