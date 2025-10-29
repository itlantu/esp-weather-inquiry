#ifndef ESP_WI_WEB_H
#define ESP_WI_WEB_H

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t wi_start_webserver(httpd_handle_t* server);

#endif // ESP_WI_WEB_H