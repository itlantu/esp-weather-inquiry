#ifndef WI_NVS_H
#define WI_NVS_H

#include "esp_err.h"
#include "wi/config.h"

#define WI_NVS_KEY_CONFIG "wi_config"

esp_err_t wi_nvs_init();
esp_err_t wi_nvs_load_config();
esp_err_t wi_nvs_save_config();

#endif // WI_NVS_H