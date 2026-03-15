#ifndef WI_CONFIG_H
#define WI_CONFIG_H

#include "esp_log.h"
#include "esp_netif_ip_addr.h"

/* 默认配置 */
#define WI_CONFIG_DEFAULT_STA_SSID "ssid"
#define WI_CONFIG_DEFAULT_STA_PASSWORD  "password"
#define WI_CONFIG_DEFAULT_AP_SSID "esp-weather-inquiry-ap"
#define WI_CONFIG_DEFAULT_AP_PASSWORD  "12345678"
#define WI_CONFIG_DEFAULT_WEB_PORT 80
#define WI_CONFIG_DEFAULT_UART_BAUD_RATE 115200
#define WI_CONFIG_DEFAULT_WEB_TITLE "ESP-Weather-Inquiry"

/* 配置项 */
#ifndef WI_CONFIG_STA_SSID
#define WI_CONFIG_STA_SSID WI_CONFIG_DEFAULT_STA_SSID
#endif

#ifndef WI_CONFIG_STA_PASSWORD
#define WI_CONFIG_STA_PASSWORD WI_CONFIG_DEFAULT_STA_PASSWORD
#endif

#ifndef WI_CONFIG_AP_SSID
#define WI_CONFIG_AP_SSID WI_CONFIG_DEFAULT_AP_SSID
#endif

#ifndef WI_CONFIG_AP_PASSWORD
#define WI_CONFIG_AP_PASSWORD WI_CONFIG_DEFAULT_AP_PASSWORD
#endif

#ifndef WI_CONFIG_WEB_PORT
#define WI_CONFIG_WEB_PORT WI_CONFIG_DEFAULT_WEB_PORT
#endif

#ifndef WI_CONFIG_WEB_TITLE
#define WI_CONFIG_WEB_TITLE WI_CONFIG_DEFAULT_WEB_TITLE
#endif

#ifndef WI_CONFIG_UART_BAUD_RATE
#define WI_CONFIG_UART_BAUD_RATE WI_CONFIG_DEFAULT_UART_BAUD_RATE
#endif

/* 结构体与函数声明 */
struct WI_Config_t{
	struct {
		char ssid[32];       // WiFi SSID，最大32字符
		char password[64];    // WiFi 密码，最大64字符
	}sta;
	struct {
		char ssid[32];       // AP模式SSID，最大32字符
		char password[64];   // AP模式密码，最大64字符
	}ap;
	struct {
		esp_ip4_addr_t host_ip;
		uint32_t port;
		char title[50];      // 网页标题，最大50字符
	}web;
};

extern struct WI_Config_t WI_Config;
extern char wi_config_init_flag;

void wi_config_init();
void wi_config_set_wifi(const char* ssid);
void wi_config_set_password(const char* password);

#endif // WI_CONFIG_H