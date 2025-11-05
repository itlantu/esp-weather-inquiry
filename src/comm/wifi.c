#include "wi/comm/wifi.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"

#include "wi/comm/web.h"
#include "wi/comm/config_web.h"
#include "wi/config.h"
#include "wi/nvs.h"

#define LOG_TAG "wi_wifi"
httpd_handle_t index_server_handle = NULL;
httpd_handle_t index_config_handle = NULL;

/**
 * @brief WiFi事件处理函数
 *
 * 此函数用于处理ESP32的WiFi相关事件，包括STA启动、断开连接等情况的处理
 *
 * @param args 用户数据参数，未使用
 * @param event_base 事件基础类型
 * @param event_id 事件ID
 * @param event_data 事件相关数据，未使用
 */
static void wifi_event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	// 记录事件处理函数执行的调试日志
	ESP_LOGD(LOG_TAG, "执行wifi_event_handler");

	// 检查事件基础类型是否为WiFi事件，不是则直接返回
	if (event_base != WIFI_EVENT)
		return;

	// 当WiFi STA模式启动时，尝试连接WiFi
	if (event_id == WIFI_EVENT_STA_START)
		esp_wifi_connect();

	// 当WiFi连接断开时的处理逻辑
	if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
		// 记录连接断开信息并尝试重新连接
		ESP_LOGI(LOG_TAG, "WIFI连接断开, 正在尝试重连");
		esp_wifi_connect();

		// 停止web服务器以避免在未连接网络时提供服务
		if (index_server_handle == NULL)
			return;
		httpd_stop(index_server_handle);
		index_server_handle = NULL;
	}

	// 手机连接AP
	if (event_id == WIFI_EVENT_AP_STACONNECTED) {
		wifi_event_ap_staconnected_t *conn = (wifi_event_ap_staconnected_t *) event_data;
		ESP_LOGI(LOG_TAG, "其他设备已连接AP MAC: " MACSTR ", AID: %d", MAC2STR(conn->mac), conn->aid);
		if(index_config_handle == NULL)
            wi_start_config_webserver(&index_config_handle);
	}

	// 手机断开AP
	else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t *disconn = (wifi_event_ap_stadisconnected_t *) event_data;
		ESP_LOGI(LOG_TAG, "其他设备连接AP已断开 MAC: " MACSTR ", 原因: %d", MAC2STR(disconn->mac), disconn->reason);

        if(index_config_handle != NULL){
            return;
        }
        httpd_stop(index_config_handle);
		index_config_handle = NULL;
	}
}

/**
 * @brief IP事件处理函数
 *
 * 此函数用于处理ESP32的IP相关事件，主要处理STA模式下获取到IP地址的情况
 *
 * @param args 用户数据参数，未使用
 * @param event_base 事件基础类型
 * @param event_id 事件ID
 * @param event_data 事件相关数据，包含IP地址信息
 */
static void ip_event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	// 记录事件处理函数执行的调试日志
	ESP_LOGD(LOG_TAG, "执行ip_event_handler");

	// 检查事件基础类型是否为IP事件，不是则直接返回
	if (event_base != IP_EVENT)
		return;

	// 检查事件ID是否为STA模式获取到IP地址的事件，不是则直接返回
	if (event_id != IP_EVENT_STA_GOT_IP) {
		return;
	}

	// 从事件数据中获取IP地址信息
	const ip_event_got_ip_t *ipv4_event = (ip_event_got_ip_t *) event_data;
	const esp_ip4_addr_t *ipv4_addr = &(ipv4_event->ip_info.ip);

	// 保存获取到的IP地址到配置结构体中
	WI_Config.web.host_ip = ipv4_event->ip_info.ip;

	// 记录获取到的IP地址信息
	ESP_LOGI(LOG_TAG, "本机获取到IP: " IPSTR, IP2STR(ipv4_addr));

	// 如果Web服务器尚未启动，则启动Web服务器
	if (index_server_handle == NULL) {
		wi_start_webserver(&index_server_handle);
	}
}

/**
 * @brief 初始化WiFi功能
 *
 * 此函数负责初始化ESP32的WiFi功能，包括网络接口初始化、WiFi配置、事件注册等
 * 是整个应用连接WiFi网络的核心初始化函数
 *
 * @return esp_err_t 初始化结果，成功返回ESP_OK，失败返回对应的错误码
 */
esp_err_t wi_wifi_init() {
	// 记录WiFi初始化函数执行的信息日志
	ESP_LOGI(LOG_TAG, "执行wi_wifi_init");

	// 初始化NVS，用于存储WiFi配置等信息
	wi_nvs_init();

	// 初始化网络接口
	ESP_ERROR_CHECK(esp_netif_init());
	// 创建默认的事件循环
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	// 创建默认的WiFi站点模式接口
	ESP_ERROR_CHECK(esp_netif_create_default_wifi_sta() != NULL ? ESP_OK : ESP_FAIL);

	// 初始化WiFi，使用默认配置
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

	// 注册WiFi事件处理函数，处理所有WiFi事件
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

	// 注册IP事件处理函数，只处理获取IP地址的事件
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

	// 配置AP连接参数
	wifi_config_t wifi_ap_config = {.ap = {
											.ssid_len = 0, // 自动计算长度（以\0结尾）
											.max_connection = 4,
											.ssid_hidden = 0,
											.authmode = WIFI_AUTH_WPA3_PSK,
											.sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
											.pmf_cfg =
													{
															.required = true,
													},
									}};
	// 从配置结构体中复制SSID和密码
	strncpy((char *) wifi_ap_config.ap.ssid, WI_Config.ap.ssid, strlen(WI_Config.ap.ssid));
	strncpy((char *) wifi_ap_config.ap.password, WI_Config.ap.password, strlen(WI_Config.ap.password));
	wifi_ap_config.ap.max_connection = 4;
	// 记录要连接的WiFi信息
	ESP_LOGI(LOG_TAG, "尝试启动AP: %s | 密码: %s", (char *) wifi_ap_config.ap.ssid,
			 (char *) wifi_ap_config.ap.password);
	// 设置WiFi为AP模式并配置
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

	// 配置STA连接参数
	wifi_config_t wifi_sta_config = {0};
	// 从配置结构体中复制SSID和密码
	strncpy((char *) wifi_sta_config.sta.ssid, WI_Config.sta.ssid, strlen(WI_Config.sta.ssid));
	strncpy((char *) wifi_sta_config.sta.password, WI_Config.sta.password, strlen(WI_Config.sta.password));
	// 记录要连接的WiFi信息
	ESP_LOGI(LOG_TAG, "尝试连接wifi: %s | 密码: %s", (char *) wifi_sta_config.sta.ssid,
			 (char *) wifi_sta_config.sta.password);
	// 设置WiFi为STA模式并配置
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
	// 启动WiFi
	ESP_ERROR_CHECK(esp_wifi_start());

	// 返回初始化成功状态
	return ESP_OK;
}
