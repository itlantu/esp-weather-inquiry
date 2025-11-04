#ifndef WI_NVS_H
#define WI_NVS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "wi/config.h"

#define WI_NVS_KEY_HISTORY "wi_history"

esp_err_t wi_nvs_load_history(char* history_data);
esp_err_t wi_nvs_save_history(const char* history_data);

#ifdef __cplusplus
}
#endif

#endif // WI_NVS_H