#ifndef WI_CITY_CODE_H
#define WI_CITY_CODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

esp_err_t wi_get_city_code(char* code, const char* name);
esp_err_t wi_paser_post(const char* buffer, int data_length, char* city_name);
esp_err_t wi_paser_get_str_find(const char* str, const char* find, size_t* end_pos);

#ifdef __cplusplus
}
#endif

#endif // WI_CITY_CODE_H