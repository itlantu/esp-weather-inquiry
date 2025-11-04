#ifndef WI_COMM_FETCH_WEATHER_H
#define WI_COMM_FETCH_WEATHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

esp_err_t wi_fetch_weather(const char *city_code);
esp_err_t wi_fetch_html_join(char *result, const char* index_content, size_t* html_content_length);
size_t wi_get_fetch_html_size();
void wi_fetch_clear();

#ifdef __cplusplus
}
#endif

#endif // WI_COMM_FETCH_WEATHER_H