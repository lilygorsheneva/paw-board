#include <string.h>

#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_log.h"

#include "remote_config.h"

#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))

const static char *TAG = "REMOTE_CONFIG";

/* Attribute table indices */
enum
{
    IDX_RCFG_SVC,

    // Normal filter (for non-held keys)

    // These characteristics will all be treated as integers initially.
    // Will figure out float later.
    // target_frequency in iir_filter_params
    IDX_RCFG_CHAR_NORMAL_FREQ_NUM,
    IDX_RCFG_CHAR_NORMAL_FREQ_NUM_VAL,
    // sample_rate.
    IDX_RCFG_CHAR_NORMAL_FREQ_DEN,
    IDX_RCFG_CHAR_NORMAL_FREQ_DEN_VAL,
    // qfactor
    IDX_RCFG_CHAR_NORMAL_Q,
    IDX_RCFG_CHAR_NORMAL_Q_VAL,

    // Filter for held. Same as _NORMAL_ but applied to different ones.
    IDX_RCFG_CHAR_HELD_FREQ_NUM,
    IDX_RCFG_CHAR_HELD_FREQ_NUM_VAL,
    IDX_RCFG_CHAR_HELD_FREQ_DEN,
    IDX_RCFG_CHAR_HELD_FREQ_DEN_VAL,
    IDX_RCFG_CHAR_HELD_Q,
    IDX_RCFG_CHAR_HELD_Q_VAL,

    // Bitmask allowing keys to be disabled.
    IDX_RCFG_CHAR_KEY_MASK,
    IDX_RCFG_CHAR_KEY_MASK_VAL,

    IDX_RCFG_NB,
};

// I don't anticipate needing more than 2 bits for each field.
#define RCFG_CHAR_LEN_MAX 2
static const uint8_t initial_value[2] = {0};

static const uint8_t RCFG_SVC_UUID[16] = {"lilypawsconf.svc"};
static const uint8_t RCFG_CHAR_NFN_UUID[16] = {"lilypawsconf.nfn"};
static const uint8_t RCFG_CHAR_NFD_UUID[16] = {"lilypawsconf.nfd"};
static const uint8_t RCFG_CHAR_NQ_UUID[16] = {"lilypawsconf. nq"};
static const uint8_t RCFG_CHAR_HFN_UUID[16] = {"lilypawsconf.hfn"};
static const uint8_t RCFG_CHAR_HFD_UUID[16] = {"lilypawsconf.hfd"};
static const uint8_t RCFG_CHAR_HQ_UUID[16] = {"lilypawsconf. hq"};
static const uint8_t RCFG_CHAR_KM_UUID[16] = {"lilypawsconf. km"};

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static const esp_gatts_attr_db_t rcfg_gatt_db[IDX_RCFG_NB] =
    {
        // Service Declaration
        [IDX_RCFG_SVC] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(RCFG_SVC_UUID), (uint8_t *)&RCFG_SVC_UUID}},

        /* Characteristic Declaration */
        [IDX_RCFG_CHAR_NORMAL_FREQ_NUM] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        /* Characteristic Value */
        [IDX_RCFG_CHAR_NORMAL_FREQ_NUM_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_NFN_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_value), (uint8_t *)initial_value}},

        [IDX_RCFG_CHAR_NORMAL_FREQ_DEN] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_NORMAL_FREQ_DEN_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_NFD_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_value), (uint8_t *)initial_value}},

        [IDX_RCFG_CHAR_NORMAL_Q] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_NORMAL_Q_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_NQ_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_value), (uint8_t *)initial_value}},

        /* Characteristic Declaration */
        [IDX_RCFG_CHAR_HELD_FREQ_NUM] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        /* Characteristic Value */
        [IDX_RCFG_CHAR_HELD_FREQ_NUM_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_HFN_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_value), (uint8_t *)initial_value}},

        [IDX_RCFG_CHAR_HELD_FREQ_DEN] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_HELD_FREQ_DEN_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_HFD_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_value), (uint8_t *)initial_value}},

        [IDX_RCFG_CHAR_HELD_Q] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_HELD_Q_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_HQ_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_value), (uint8_t *)initial_value}},

        [IDX_RCFG_CHAR_KEY_MASK] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_KEY_MASK_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_KM_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_value), (uint8_t *)initial_value}},
};

uint16_t rcfg_handle_table[IDX_RCFG_NB];

void remote_config_gatt_callback_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                         esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {

    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        if (param->add_attr_tab.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
        }
        else if (param->add_attr_tab.num_handle != IDX_RCFG_NB)
        {
            ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)",
                     param->add_attr_tab.num_handle, IDX_RCFG_NB);
        }
        else
        {
            ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d", param->add_attr_tab.num_handle);
            memcpy(rcfg_handle_table, param->add_attr_tab.handles, sizeof(rcfg_handle_table));
            esp_ble_gatts_start_service(rcfg_handle_table[IDX_RCFG_SVC]);
        }
        break;

    default:
        break;
    }
}