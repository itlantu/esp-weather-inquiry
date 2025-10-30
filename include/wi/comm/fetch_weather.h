#ifndef WI_COMM_FETCH_WEATHER_H
#define WI_COMM_FETCH_WEATHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

esp_err_t wi_fetch_weather(const char *city_code, char* content);

#ifdef __cplusplus
}
#endif

#endif // WI_COMM_FETCH_WEATHER_H