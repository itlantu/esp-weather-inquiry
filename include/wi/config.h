#ifndef WI_CONFIG_H
#define WI_CONFIG_H

#include "esp_log.h"
#include "esp_netif_ip_addr.h"

#define WI_CONFIG_DEFAULT_WIFI_SSID "ssid"
#define WI_CONFIG_DEFAULT_WIFI_PASSWORD  "password"
#define WI_CONFIG_DEFAULT_WEB_PORT 80

#ifndef WI_CONFIG_WIFI_SSID
#define WI_CONFIG_WIFI_SSID WI_CONFIG_DEFAULT_WIFI_SSID
#endif

#ifndef WI_CONFIG_WIFI_PASSWORD
#define WI_CONFIG_WIFI_PASSWORD WI_CONFIG_DEFAULT_WIFI_PASSWORD
#endif

#ifndef WI_CONFIG_WEB_PORT
#define WI_CONFIG_WEB_PORT WI_CONFIG_DEFAULT_WEB_PORT
#endif

struct WI_Config_t{
	struct {
		char ssid[32];
		char password[64];
	}wifi;
	struct {
		esp_ip4_addr_t host_ip;
		uint32_t port;
	}web;
};

extern struct WI_Config_t WI_Config;

extern char wi_config_init_flag;

void wi_config_init();
void wi_config_set_wifi(const char* ssid);
void wi_config_set_password(const char* password);

#endif // WI_CONFIG_H