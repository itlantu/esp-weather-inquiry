#include "wi/config.h"
#include <string.h>

struct WI_Config_t WI_Config = {
	.wifi = {
		.ssid = WI_CONFIG_WIFI_SSID,
		.password = WI_CONFIG_WIFI_PASSWORD,
	},
	.web = {
		.port = WI_CONFIG_WEB_PORT
	}
};


void wi_config_set_wifi(const char* ssid) {
	strncpy((char*)WI_Config.wifi.ssid, ssid, 32);
}

void wi_config_set_password(const char* password) {
	strncpy((char*)WI_Config.wifi.password, password, 64);
}