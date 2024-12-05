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
    IDX_RCFG_CHAR_NORMAL_FREQ_DESC,
    // qfactor
    IDX_RCFG_CHAR_NORMAL_Q,
    IDX_RCFG_CHAR_NORMAL_Q_VAL,
    IDX_RCFG_CHAR_NORMAL_Q_DESC,

    // Filter for held. Same as _NORMAL_ but applied to different ones.
    IDX_RCFG_CHAR_HELD_FREQ,
    IDX_RCFG_CHAR_HELD_FREQ_VAL,
    IDX_RCFG_CHAR_HELD_FREQ_DESC,
    IDX_RCFG_CHAR_HELD_Q,
    IDX_RCFG_CHAR_HELD_Q_VAL,
    IDX_RCFG_CHAR_HELD_Q_DESC,

    // Sending floats seems to be a mess. Divide all values above by the denominator.
    IDX_RCFG_CHAR_DENOMINATOR,
    IDX_RCFG_CHAR_DENOMINATOR_VAL,
    IDX_RCFG_CHAR_DENOMINATOR_DESC,

    IDX_RCFG_NB,
};

#define RCFG_CHAR_LEN_MAX 1
uint8_t initial_data = 0;
uint16_t rcfg_handle_table[IDX_RCFG_NB];

static uint8_t RCFG_SVC_UUID[16] = {'l', 'i', 'l', 'y', 'p', 'a', 'w', 's', 'c', 'o', 'n', 'f', '.', 's', 'v', 'c'};
static uint8_t RCFG_CHAR_NF_UUID[16] = {'l', 'i', 'l', 'y', 'p', 'a', 'w', 's', 'c', 'o', 'n', 'f', '.', ' ', 'n', 'f'};
static uint8_t RCFG_CHAR_NQ_UUID[16] = {'l', 'i', 'l', 'y', 'p', 'a', 'w', 's', 'c', 'o', 'n', 'f', '.', ' ', 'n', 'q'};
static uint8_t RCFG_CHAR_HF_UUID[16] = {'l', 'i', 'l', 'y', 'p', 'a', 'w', 's', 'c', 'o', 'n', 'f', '.', ' ', 'h', 'f'};
static uint8_t RCFG_CHAR_HQ_UUID[16] = {'l', 'i', 'l', 'y', 'p', 'a', 'w', 's', 'c', 'o', 'n', 'f', '.', ' ', 'h', 'q'};
static uint8_t RCFG_CHAR_DEN_UUID[16] = {'l', 'i', 'l', 'y', 'p', 'a', 'w', 's', 'c', 'o', 'n', 'f', '.', 'd', 'e', 'n'};

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t characteristic_description_descriptor = ESP_GATT_UUID_CHAR_DESCRIPTION;
static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static const uint8_t NF_DESC[] = "Target frequency of normal sensors (numerator).";
static const uint8_t NQ_DESC[] = "Q factor of normal sensors (numerator).";
static const uint8_t HF_DESC[] = "Target frequency of held sensors (numerator).";
static const uint8_t HQ_DESC[] = "Q factor of held sensors (numerator).";
static const uint8_t DEN_DESC[] = "Denominator frequency and q factor.";

static const esp_gatts_attr_db_t rcfg_gatt_db[IDX_RCFG_NB] =
    {
        // Service Declaration
        [IDX_RCFG_SVC] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, ESP_UUID_LEN_128, ESP_UUID_LEN_128, RCFG_SVC_UUID}},

        /* Characteristic Declaration */
        [IDX_RCFG_CHAR_NORMAL_FREQ] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        /* Characteristic Value */
        [IDX_RCFG_CHAR_NORMAL_FREQ_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_NF_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_data), &initial_data}},
        /* Characteristic User Descriptione */
        [IDX_RCFG_CHAR_NORMAL_FREQ_DESC] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&characteristic_description_descriptor, ESP_GATT_PERM_READ, sizeof(NF_DESC), sizeof(NF_DESC), NF_DESC}},

        [IDX_RCFG_CHAR_NORMAL_Q] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_NORMAL_Q_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_NQ_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_data), &initial_data}},

        [IDX_RCFG_CHAR_NORMAL_Q_DESC] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&characteristic_description_descriptor, ESP_GATT_PERM_READ, sizeof(NQ_DESC), sizeof(NQ_DESC), NQ_DESC}},

        [IDX_RCFG_CHAR_HELD_FREQ] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_HELD_FREQ_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_HF_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_data), &initial_data}},
        [IDX_RCFG_CHAR_HELD_FREQ_DESC] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&characteristic_description_descriptor, ESP_GATT_PERM_READ, sizeof(HF_DESC), sizeof(HF_DESC), HF_DESC}},

        [IDX_RCFG_CHAR_HELD_Q] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_HELD_Q_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_HQ_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_data), &initial_data}}

        ,
        [IDX_RCFG_CHAR_HELD_Q_DESC] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&characteristic_description_descriptor, ESP_GATT_PERM_READ, sizeof(HQ_DESC), sizeof(HQ_DESC), HQ_DESC}},

        [IDX_RCFG_CHAR_DENOMINATOR] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
        [IDX_RCFG_CHAR_DENOMINATOR_VAL] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&RCFG_CHAR_DEN_UUID, ESP_GATT_PERM_WRITE, RCFG_CHAR_LEN_MAX, sizeof(initial_data), &initial_data}}

        ,
        [IDX_RCFG_CHAR_DENOMINATOR_DESC] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&characteristic_description_descriptor, ESP_GATT_PERM_READ, sizeof(DEN_DESC), sizeof(DEN_DESC), DEN_DESC}},
};

void remote_config_gatt_callback_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                         esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "Register remote config svc.");
        esp_ble_gatts_create_attr_tab(rcfg_gatt_db, gatts_if, IDX_RCFG_NB, 0);
        break;

    // No write event handler needed use esp_ble_gatts_get_attr_value.

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
            ESP_LOGI(TAG, "created attribute table for remote config svc.");
            memcpy(rcfg_handle_table, param->add_attr_tab.handles, sizeof(rcfg_handle_table));
            esp_ble_gatts_start_service(rcfg_handle_table[IDX_RCFG_SVC]);
        }
        break;
    default:
        break;
    }
}

iir_filter_params filter_params;

uint8_t get_int_value(int idx) {
    uint16_t len;
    const uint8_t *dest; 
    esp_err_t err = esp_ble_gatts_get_attr_value(rcfg_handle_table[idx], &len, &dest);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read char %d, error %d", idx, err);
        return 0;
    }
    return dest[0];
}


iir_filter_params get_remote_config(void)
{
    uint8_t denominator = get_int_value(IDX_RCFG_CHAR_DENOMINATOR_VAL);
    if (denominator == 0)
    {
        iir_filter_params blank = {};
        // TODO NICER API HERE
        ESP_LOGE(TAG, "DENOMINATOR UNSET. DO NOT USE THIS DATA.");
        return blank;
    }

    float freq_normal = get_int_value(IDX_RCFG_CHAR_NORMAL_FREQ_VAL) / denominator;
    float freq_held = get_int_value(IDX_RCFG_CHAR_HELD_FREQ_VAL) / denominator;
    float q_normal = get_int_value(IDX_RCFG_CHAR_NORMAL_Q_VAL) / denominator;
    float q_held = get_int_value(IDX_RCFG_CHAR_HELD_Q_VAL) / denominator;

    ESP_LOGI(TAG, "REMOTE PARAMS %f,%f,%f,%f", freq_normal, freq_held, q_normal, q_held);

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
