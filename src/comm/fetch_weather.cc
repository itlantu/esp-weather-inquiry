#include "wi/comm/fetch_weather.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include <cstring>
#include <string>

#define LOG_TAG "wi_fetch_weather"
// 天气API网址
#define WEATHER_API_BASE_URL "http://t.weather.itboy.net/api/weather/city/" 

esp_err_t http_event_on_data_handler(esp_http_client_event_t* event, std::string& buffer){
    if(esp_http_client_is_chunked_response(event->client)){
        ESP_LOGE(LOG_TAG, "当前代码不支持分块传输");
        return ESP_ERR_NOT_SUPPORTED;
    }

    buffer.append((char*)event->data, event->data_len);
}

void process_weather_data(const std::string& json_data, char* html_content){
    
}

esp_err_t http_event_handler(esp_http_client_event_t* event){
    static std::string buffer;
    static std::string html_content;

    switch(event->event_id) {
        // 处理HTTP事件：接收到数据
        case HTTP_EVENT_ON_DATA:
            ESP_ERROR_CHECK(http_event_on_data_handler(event, buffer));
        break;
    
        // 处理HTTP事件：请求完成
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(LOG_TAG, "HTTP请求完成, 响应数据长度: %d", buffer.size());
            buffer.clear();
            process_weather_data(buffer);
        break;
        
        // 处理HTTP事件：请求错误
        case HTTP_EVENT_ERROR:
            ESP_LOGE(LOG_TAG, "HTTP请求错误");
            buffer.clear();
            break;
        
        // 默认情况，不处理
        default:
            buffer.clear();
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
