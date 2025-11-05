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


/**
 * @brief 设置WiFi的SSID配置
 * 
 * 此函数用于将WiFi的SSID（网络名称）保存到全局配置结构体中
 * 
 * @param ssid 要设置的WiFi网络名称
 */
void wi_config_set_wifi(const char* ssid) {
    // 安全地将SSID复制到配置结构体中，最多复制32个字符
    strncpy((char*)WI_Config.wifi.ssid, ssid, 32);
}

/**
 * @brief 设置WiFi的密码配置
 * 
 * 此函数用于将WiFi的密码保存到全局配置结构体中
 * 
 * @param password 要设置的WiFi密码
 * @note 实际产品中应注意密码的安全性，避免明文存储或不当记录
 */
void wi_config_set_password(const char* password) {
    // 安全地将密码复制到配置结构体中，最多复制64个字符
    strncpy((char*)WI_Config.wifi.password, password, 64);
}