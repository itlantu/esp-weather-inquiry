#ifndef WI_COMM_WEB_H
#define WI_COMM_WEB_H

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t wi_start_webserver(httpd_handle_t* server, httpd_config_t* web_httpd_config);

#endif // WI_COMM_WEB_H