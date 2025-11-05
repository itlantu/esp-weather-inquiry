#ifndef WI_COMM_CONFIG_WEB_H
#define WI_COMM_CONFIG_WEB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t wi_start_config_webserver(httpd_handle_t *server);

#ifdef __cplusplus
}
#endif

#endif // WI_COMM_CONFIG_WEB_H