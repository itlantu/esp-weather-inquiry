#include "wi/config.h"
#include <string.h>
#include "wi/nvs.h"

struct WI_Config_t WI_Config = {
	.sta = {
		.ssid = WI_CONFIG_STA_SSID,
		.password = WI_CONFIG_STA_PASSWORD,
	},
	.ap = {
		.ssid = WI_CONFIG_AP_SSID,
		.password = WI_CONFIG_AP_PASSWORD,
	},
	.web = {
		.port = WI_CONFIG_WEB_PORT,
		.title = WI_CONFIG_WEB_TITLE,
	}
};


void wi_config_init(){
	wi_nvs_load_config();
}

/**
 * @brief 设置WiFi的SSID配置
 * 
 * 此函数用于将WiFi的SSID（网络名称）保存到全局配置结构体中
 * 
 * @param ssid 要设置的WiFi网络名称
 */
void wi_config_set_wifi(const char* ssid) {
    // 安全地将SSID复制到配置结构体中，确保null终止
    strncpy((char*)WI_Config.sta.ssid, ssid, 31);
    WI_Config.sta.ssid[31] = '\0';
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
    // 安全地将密码复制到配置结构体中，确保null终止
    strncpy((char*)WI_Config.sta.password, password, 63);
    WI_Config.sta.password[63] = '\0';
}