#include <string.h>

#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_log.h"

#include "remote_config.h"
#include "iir_filter.h"

#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))

const static char *TAG = "REMOTE_CONFIG";

/* Attribute table indices */
enum
{
    IDX_RCFG_SVC,

    // Normal filter (for non-held keys)

    // target_frequency in iir_filter_params
    IDX_RCFG_CHAR_NORMAL_FREQ,
    IDX_RCFG_CHAR_NORMAL_FREQ_VAL,
    // qfactor
    IDX_RCFG_CHAR_NORMAL_Q,
    IDX_RCFG_CHAR_NORMAL_Q_VAL,

    // Filter for held. Same as _NORMAL_ but applied to different ones.
    IDX_RCFG_CHAR_HELD_FREQ,
    IDX_RCFG_CHAR_HELD_FREQ_VAL,
    IDX_RCFG_CHAR_HELD_Q,
    IDX_RCFG_CHAR_HELD_Q_VAL,

    // Sending floats seems to be a mess. Divide all values above by the denominator.
    IDX_RCFG_CHAR_DENOMINATOR,
    IDX_RCFG_CHAR_DENOMINATOR_VAL,

    IDX_RCFG_NB,
};

// Not every field needs to be the same length, but for a first attempt a single byte should be enough.
#define RCFG_CHAR_LEN_MAX 1
// This is gonna waste a significant amount of space due to being a sparse array. However, it's easy to implement in the super-short-term.
uint8_t remote_config_data[IDX_RCFG_NB][RCFG_CHAR_LEN_MAX];
uint16_t rcfg_handle_table[IDX_RCFG_NB];

static const uint8_t RCFG_SVC_UUID[16] = {"lilypawsconf.svc"};
static const uint8_t RCFG_CHAR_NF_UUID[16] = {"lilypawsconf. nf"};
static const uint8_t RCFG_CHAR_NQ_UUID[16] = {"lilypawsconf. nq"};
static const uint8_t RCFG_CHAR_HF_UUID[16] = {"lilypawsconf. hf"};
static const uint8_t RCFG_CHAR_HQ_UUID[16] = {"lilypawsconf. hq"};
static const uint8_t RCFG_CHAR_DEN_UUID[16] = {"lilypawsconf.den"};

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
        [IDX_RCFG_CHAR_NORMAL_FREQ] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        /* Characteristic Value */
        [IDX_RCFG_CHAR_NORMAL_FREQ_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_NF_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, RCFG_CHAR_LEN_MAX, remote_config_data[IDX_RCFG_CHAR_NORMAL_FREQ_VAL]}},

        [IDX_RCFG_CHAR_NORMAL_Q] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_NORMAL_Q_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_NQ_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, RCFG_CHAR_LEN_MAX, remote_config_data[IDX_RCFG_CHAR_NORMAL_Q_VAL]}},

        /* Characteristic Declaration */
        [IDX_RCFG_CHAR_HELD_FREQ] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        /* Characteristic Value */
        [IDX_RCFG_CHAR_HELD_FREQ_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_HF_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, RCFG_CHAR_LEN_MAX, remote_config_data[IDX_RCFG_CHAR_HELD_FREQ_VAL]}},

        [IDX_RCFG_CHAR_HELD_Q] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_HELD_Q_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_HQ_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, RCFG_CHAR_LEN_MAX, remote_config_data[IDX_RCFG_CHAR_HELD_Q_VAL]}},

        [IDX_RCFG_CHAR_DENOMINATOR] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_DENOMINATOR_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_DEN_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, RCFG_CHAR_LEN_MAX, remote_config_data[IDX_RCFG_CHAR_DENOMINATOR_VAL]}},
};

void remote_config_gatt_callback_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                         esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        esp_ble_gatts_create_attr_tab(rcfg_gatt_db, gatts_if, IDX_RCFG_NB, 0);
    break;

    case ESP_GATTS_WRITE_EVT:
        for (int i = 0; i < IDX_RCFG_NB; ++i)
        {
            if (param->write.handle == rcfg_handle_table[i])
            {
                memcpy(remote_config_data[i], param->write.value, param->write.len);
                break;
            }
        }
        break;

    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        if (param->add_attr_tab.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
        }
        else if (param->add_attr_tab.num_handle != IDX_RCFG_NB)
        {
            ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) does not match (%d)",
                     param->add_attr_tab.num_handle, IDX_RCFG_NB);
        }
        else
        {
            memcpy(rcfg_handle_table, param->add_attr_tab.handles, sizeof(rcfg_handle_table));
            esp_ble_gatts_start_service(rcfg_handle_table[IDX_RCFG_SVC]);
        }
        break;
    default:
        break;
    }
}

iir_filter_params filter_params;

iir_filter_params get_remote_config(void)
{
    uint8_t denominator = remote_config_data[IDX_RCFG_CHAR_DENOMINATOR_VAL][0];
    if (denominator == 0)
    {
        denominator = 1;
    }

    float freq_normal = remote_config_data[IDX_RCFG_CHAR_NORMAL_FREQ_VAL][0] / denominator;
    float freq_held = remote_config_data[IDX_RCFG_CHAR_HELD_FREQ_VAL][0] / denominator;
    float q_normal = remote_config_data[IDX_RCFG_CHAR_NORMAL_Q_VAL][0] / denominator;
    float q_held = remote_config_data[IDX_RCFG_CHAR_HELD_Q_VAL][0] / denominator;

    iir_filter_params filter_params = {
        .sample_rate = 100,
        .target_frequency = {freq_normal, freq_normal, freq_normal, freq_normal, freq_normal, freq_held, freq_held, freq_held, freq_held, freq_held},
        .qfactor = {q_normal, q_normal, q_normal, q_normal, q_normal, q_held, q_held, q_held, q_held, q_held},
        .holdable = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1},
        .calibration_peak_multiplier = 2.5,
        .calibration_time_seconds = 2,
        .debounce_count = 4,
        .min_threshold = 1};
    return filter_params;
}
