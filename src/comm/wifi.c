#include "wi/comm/wifi.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_http_server.h"

#include "wi/comm/web.h"
#include "wi/nvs.h"
#include "wi/config.h"

#define LOG_TAG "wi_wifi"
httpd_handle_t server_handle = NULL;

static void wifi_event_handler(void* args, esp_event_base_t event_base, int32_t event_id, void* event_data){
    ESP_LOGD(LOG_TAG, "执行wifi_event_handler");
    if (event_base != WIFI_EVENT)
        return;
    if (event_id == WIFI_EVENT_STA_START)
        esp_wifi_connect();
    if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(LOG_TAG, "WIFI连接断开, 正在尝试重连");
        esp_wifi_connect();
        // 停止web服务
        if (server_handle == NULL)
            return;
        httpd_stop(server_handle);
        server_handle = NULL;
    }
}

static void ip_event_handler(void* args, esp_event_base_t event_base, int32_t event_id, void* event_data){
    ESP_LOGD(LOG_TAG, "执行ip_event_handler");
    if (event_base != IP_EVENT)
        return;
    if (event_id != IP_EVENT_STA_GOT_IP) {
        return ;
    }

    const ip_event_got_ip_t* ipv4_event = (ip_event_got_ip_t*)event_data;
    const esp_ip4_addr_t* ipv4_addr = &(ipv4_event->ip_info.ip);
	// 记录IP
	WI_Config.web.host_ip = ipv4_event->ip_info.ip;
    ESP_LOGI(LOG_TAG, "本机获取到IP: " IPSTR, IP2STR(ipv4_addr));
    if (server_handle == NULL) {
        wi_start_webserver(&server_handle);
    }
}

esp_err_t wi_wifi_init(){
    ESP_LOGI(LOG_TAG, "执行wi_wifi_init");
    // 初始化
    wi_nvs_init();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 创建默认wifi站点接口
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_sta() != NULL ? ESP_OK : ESP_FAIL);

    // 初始化wifi
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    // 注册事件
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

    // 配置wifi
    wifi_config_t wifi_config = {0};
	strncpy((char*)wifi_config.sta.ssid, WI_Config.wifi.ssid, strlen(WI_Config.wifi.ssid));
	strncpy((char*)wifi_config.sta.password, WI_Config.wifi.password, strlen(WI_Config.wifi.password));
	ESP_LOGI(LOG_TAG, "尝试连接wifi: %s | 密码: %s", (char*)wifi_config.sta.ssid, (char*)wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    return ESP_OK;
}