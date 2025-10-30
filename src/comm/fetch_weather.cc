#include "wi/comm/fetch_weather.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include <cstring>
#include <string>

#define LOG_TAG "wi_fetch_weather"
// 天气API网址
#define WEATHER_API_BASE_URL "http://t.weather.itboy.net/api/weather/city/"

esp_err_t http_event_handler(esp_http_client_event_t* event){
    static char *response_buffer = NULL;
    static size_t response_len = 0;
    
    switch(event->event_id) {
        // 处理HTTP事件：接收到数据
        case HTTP_EVENT_ON_DATA:
            break;
    
        // 处理HTTP事件：请求完成
        case HTTP_EVENT_ON_FINISH:
            break;
        
        // 处理HTTP事件：请求错误
        case HTTP_EVENT_ERROR:
            ESP_LOGE(LOG_TAG, "HTTP请求错误");
            break;
        
        // 默认情况，不处理
        default:
        break;
    }
    return ESP_OK;
}

esp_err_t fetch_weather(char* city_code, char* html_content){
    ESP_ERROR_CHECK(city_code == NULL ? ESP_ERR_INVALID_ARG : ESP_OK);
    ESP_LOGI(LOG_TAG, "尝试获取天气数据, 城市编码为: %s", city_code);
    
    std::string url{WEATHER_API_BASE_URL};
    url += city_code;

    // 配置HTTP客户端
    esp_http_client_config_t weather_api_config = {
        .url = url.c_str(),
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
        .event_handler = http_event_handler
    };
    // 初始化HTTP客户端
    const esp_http_client_handle_t client = esp_http_client_init(&weather_api_config);

    esp_err_t err_code = esp_http_client_perform(client);
    if(err_code != ESP_OK){
        ESP_LOGE(LOG_TAG, "获取天气数据失败, 错误(%d):%s", err_code, esp_err_to_name(err_code));
        strcpy(html_content, "<p>网络异常, 获取天气数据失败</p>");
        return err_code;
    }
    
    return ESP_OK;
}
