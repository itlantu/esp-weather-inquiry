#include "wi/comm/fetch_weather.h"
#include <cstring>
#include <string>
#include <tuple>
#include <vector>
#include <format>

#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "wi/nvs.h"

#define LOG_TAG "wi_fetch_weather"
// 天气API网址
#define WEATHER_API_BASE_URL "http://t.weather.itboy.net/api/weather/city/" 

std::string fetch_html_content;

esp_err_t json_get_from_data(std::string& result, cJSON* root, const std::string& key) {
    ESP_ERROR_CHECK(root == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK);

    cJSON* value = cJSON_GetObjectItem(root, key.c_str());
    if (value == nullptr) {
        // 尝试在forecast下查找键
        cJSON* forecast = cJSON_GetObjectItem(root, "forecast");
        if(forecast != nullptr && cJSON_IsArray(forecast) && cJSON_GetArraySize(forecast) > 0){
            cJSON* today = cJSON_GetArrayItem(forecast, 0);
            value = cJSON_GetObjectItem(today, key.c_str());
        }else{
            ESP_LOGE(LOG_TAG, "未找到键: %s", key.c_str());
            return ESP_ERR_INVALID_ARG;
        }
    }

    if (cJSON_IsString(value)) {
        result = value->valuestring; 
    } else if (cJSON_IsNumber(value)) {
        result = std::to_string(value->valueint);
    } else {
        ESP_LOGE(LOG_TAG, "键 %s 的值类型不支持（非字符串/数字）", key.c_str());
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

void json_to_html(cJSON* root, const std::string& key, const std::string& value){
    static std::string json_value;
    
    json_value.clear();
    ESP_ERROR_CHECK(json_get_from_data(json_value, root, key));
    ESP_LOGI(LOG_TAG, "%s = %s", value.c_str(), json_value.c_str());
    fetch_html_content += std::format("<tr><td>{0}</td><td>{1}</td></tr>", value, json_value);
};

esp_err_t process_weather_data(const std::string& json_data){
    static std::vector<std::tuple<const std::string, const std::string>> data_tabs{
        {"week", "星期"},
        {"type", "天气"},
        {"high", "最高温度"},
        {"low", "最低温度"},
        {"quality", "空气质量"},
        {"shidu", "湿度"},
        {"notice", "提示"},
    };
    static std::string json_value;

    fetch_html_content.clear();

    if(json_data.empty()){
        ESP_LOGE(LOG_TAG, "json数据为空");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON* root = cJSON_Parse(json_data.c_str());
    if(root == nullptr){
        ESP_LOGE(LOG_TAG, "JSON解析失败: %s", cJSON_GetErrorPtr());
        return ESP_ERR_INVALID_ARG;
    }

    cJSON* data = cJSON_GetObjectItem(root, "data");
    cJSON* city_info = cJSON_GetObjectItem(root, "cityInfo");
    if(data == nullptr || city_info == nullptr){
        ESP_LOGE(LOG_TAG, "未找到data对象或cityInfo对象");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    fetch_html_content += "<div id=\"weather-result\" class=\"card\"><table class=\"weather-table\">";

    // 解析root和cityinfo部分
    json_to_html(root, "time", "响应时间");
    json_to_html(city_info, "city", "城市名称");
    
    // 解析data部分
    for(const auto& [key, value]: data_tabs){
        json_to_html(data, key, value);
    }

    fetch_html_content += "</table></div>";
    // 写入history
    std::string history_data = std::format("{0}({1})", 
        cJSON_GetObjectItem(city_info, "city")->valuestring,
        cJSON_GetObjectItem(root, "time")->valuestring);
    wi_nvs_save_history(history_data.c_str());
    cJSON_Delete(root);

    return ESP_OK;
}

esp_err_t http_event_on_data_handler(esp_http_client_event_t* event, std::string& buffer){
    if(esp_http_client_is_chunked_response(event->client)){
        ESP_LOGE(LOG_TAG, "当前代码不支持分块传输");
        return ESP_ERR_NOT_SUPPORTED;
    }

    buffer.append((char*)event->data, event->data_len);
    return ESP_OK;
}

esp_err_t http_event_handler(esp_http_client_event_t* event){
    static std::string buffer;

    switch(event->event_id) {
        // 处理HTTP事件：接收到数据
        case HTTP_EVENT_ON_DATA:
            ESP_ERROR_CHECK(http_event_on_data_handler(event, buffer));
        break;
    
        // 处理HTTP事件：请求完成
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(LOG_TAG, "HTTP请求完成, 响应数据长度: %d", buffer.size());
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

esp_err_t wi_fetch_weather(const char *city_code){
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
        fetch_html_content = "<p>网络异常, 获取天气数据失败</p>";
    }else{
        int status = esp_http_client_get_status_code(client);
        int64_t length = esp_http_client_get_content_length(client);
        ESP_LOGI(LOG_TAG, "获取天气数据成功, HTTP GET = %d 内容长度 = %lld", status, length);
    }

    esp_http_client_cleanup(client);
    
    return err_code;
}

esp_err_t wi_fetch_html_join(char *result, const char* index_content, const size_t html_content_length){
    static char history_data[100];
    wi_nvs_load_history(history_data);
    snprintf(result, html_content_length, index_content, history_data, fetch_html_content.c_str());

    return ESP_OK;
}

size_t wi_get_fetch_html_size(){
    return fetch_html_content.size();
}

void wi_fetch_clear(){
    ESP_LOGI(LOG_TAG, "清除天气数据");
    fetch_html_content.clear();
}