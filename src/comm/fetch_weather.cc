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

/**
 * @brief 从cJSON对象中获取指定键的值并转换为字符串
 * @details 此函数负责从cJSON对象中查找指定键的值，并将其转换为C++字符串
 *          支持在直接对象中查找，若找不到则尝试在forecast数组的第一个元素中查找
 *          对高温和低温等特殊字段进行额外处理，去除前缀文字
 * @param[out] result 用于存储获取到的值的字符串引用
 * @param[in] root 要查找的cJSON根对象指针
 * @param[in] key 要查找的键名
 * @return esp_err_t 返回ESP_OK表示获取成功，其他错误码表示获取失败
 */
esp_err_t json_get_from_data(std::string& result, cJSON* root, const std::string& key) {
    // 检查传入的root指针是否为空，为空则返回参数无效错误
    ESP_ERROR_CHECK(root == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK);

    // 尝试直接在root对象中查找指定的键
    cJSON* value = cJSON_GetObjectItem(root, key.c_str());
    
    // 如果直接查找失败，则尝试在forecast数组中查找
    if (value == nullptr) {
        // 尝试在forecast下查找键
        cJSON* forecast = cJSON_GetObjectItem(root, "forecast");
        if(forecast != nullptr && cJSON_IsArray(forecast) && cJSON_GetArraySize(forecast) > 0){
            // 取forecast数组的第一个元素(通常是今天的天气数据)
            cJSON* today = cJSON_GetArrayItem(forecast, 0);
            // 在今天的天气数据中查找指定的键
            value = cJSON_GetObjectItem(today, key.c_str());
        }else{
            // 找不到指定的键，记录错误日志并返回参数无效错误
            ESP_LOGE(LOG_TAG, "未找到键: %s", key.c_str());
            return ESP_ERR_INVALID_ARG;
        }
    }

    // 根据值的类型进行相应处理
    if (cJSON_IsString(value)) {
        // 如果是字符串类型，直接转换
        result = value->valuestring; 
    } else if (cJSON_IsNumber(value)) {
        // 如果是数字类型，先转换为整数再转为字符串
        result = std::to_string(value->valueint);
    } else {
        // 不支持的值类型，记录错误日志并返回参数无效错误
        ESP_LOGE(LOG_TAG, "键 %s 的值类型不支持（非字符串/数字）", key.c_str());
        return ESP_ERR_INVALID_ARG;
    }

    // 对高温和低温字段进行特殊处理，去除前缀文字
    size_t pos = 0;
    if(key == "high" && (pos = result.find("高温")) != std::string::npos){
        // 去除"高温"前缀
        result.erase(pos, strlen("高温"));
    }else if (key == "low" && (pos = result.find("低温")) != std::string::npos){
        // 去除"低温"前缀
        result.erase(pos, strlen("低温"));
    }
    
    // 返回成功状态
    return ESP_OK;
}

/**
 * @brief 将JSON对象中的指定键值转换为HTML表格行
 * @details 此函数负责从cJSON对象中获取指定键的值，
 *          并将键和值格式化为HTML表格行追加到全局HTML内容中
 *          对高温和低温等特殊字段进行额外处理，去除前缀文字
 * @param[in] root 包含数据的cJSON对象指针
 * @param[in] key 要获取的JSON键名
 * @param[in] value 要在HTML中显示的字段名称
 */
void json_to_html(cJSON* root, const std::string& key, const std::string& value){
    // 静态字符串变量，用于临时存储从JSON中获取的值
    static std::string json_value;
    
    // 清空之前可能存在的值，准备存储新值
    json_value.clear();
    
    // 调用json_get_from_data函数从JSON对象中获取指定键的值
    // ESP_ERROR_CHECK会在函数执行失败时自动中止程序
    ESP_ERROR_CHECK(json_get_from_data(json_value, root, key));
    
    // 记录信息日志，显示当前处理的字段名称和对应的值
    ESP_LOGI(LOG_TAG, "%s = %s", value.c_str(), json_value.c_str());
    
    // 格式化HTML表格行，并将其追加到全局HTML内容中
    fetch_html_content += std::format("<tr><td>{0}</td><td>{1}</td></tr>", value, json_value);
}

/**
 * @brief 处理天气数据JSON字符串并生成HTML内容
 * 
 * 该函数接收天气API返回的JSON数据，解析其中的天气信息，并构建HTML表格格式的天气数据展示页面。
 * 同时会提取城市名称和查询时间信息，保存到非易失性存储(NVS)中作为查询历史记录。
 * 
 * @param json_data 包含天气信息的JSON字符串
 * @return esp_err_t 成功返回ESP_OK，失败返回相应错误码
 *         - ESP_ERR_INVALID_ARG: JSON数据为空或解析失败或缺少必要字段
 */
esp_err_t process_weather_data(const std::string& json_data){
    // 定义天气数据标签映射表，用于将英文键名映射为中文显示名称
    static std::vector<std::tuple<const std::string, const std::string>> data_tabs{
        {"week", "星期"},
        {"type", "天气"},
        {"high", "最高温度"},
        {"low", "最低温度"},
        {"quality", "空气质量"},
        {"shidu", "湿度"},
        {"notice", "提示"},
    };
    // 静态变量，用于临时存储从JSON中提取的字符串值
    static std::string json_value;

    // 清空之前可能存在的HTML内容
    fetch_html_content.clear();

    // 检查输入的JSON数据是否为空
    if(json_data.empty()){
        ESP_LOGE(LOG_TAG, "json数据为空");
        return ESP_ERR_INVALID_ARG;
    }

    // 解析JSON字符串为cJSON对象
    cJSON* root = cJSON_Parse(json_data.c_str());
    if(root == nullptr){
        ESP_LOGE(LOG_TAG, "JSON解析失败: %s", cJSON_GetErrorPtr());
        return ESP_ERR_INVALID_ARG;
    }

    // 从根对象中获取data和cityInfo字段
    cJSON* data = cJSON_GetObjectItem(root, "data");
    cJSON* city_info = cJSON_GetObjectItem(root, "cityInfo");
    if(data == nullptr || city_info == nullptr){
        ESP_LOGE(LOG_TAG, "未找到data对象或cityInfo对象");
        cJSON_Delete(root);  // 释放已分配的cJSON资源
        return ESP_ERR_INVALID_ARG;
    }

    // 开始构建HTML内容，创建卡片和表格
    fetch_html_content += "<div id=\"weather-result\" class=\"card\"><table class=\"weather-table\">";

    // 解析并添加响应时间和城市名称信息到HTML
    json_to_html(root, "time", "响应时间");
    json_to_html(city_info, "city", "城市名称");
    
    // 遍历数据标签映射表，解析并添加各天气字段到HTML
    for(const auto& [key, value]: data_tabs){
        json_to_html(data, key, value);
    }

    // 完成HTML表格构建
    fetch_html_content += "</table></div>";
    
    // 构建历史记录数据（城市名称+响应时间）并保存到NVS
    std::string history_data = std::format("{0}({1})", 
        cJSON_GetObjectItem(city_info, "city")->valuestring,
        cJSON_GetObjectItem(root, "time")->valuestring);
    wi_nvs_save_history(history_data.c_str());
    
    // 释放cJSON资源，防止内存泄漏
    cJSON_Delete(root);

    // 处理成功，返回ESP_OK
    return ESP_OK;
}

/**
 * @brief HTTP客户端数据接收事件处理函数
 * 
 * 该函数用于处理ESP HTTP客户端接收到数据时的事件，将接收到的数据追加到指定的字符串缓冲区中。
 * 目前该函数不支持HTTP分块传输模式，检测到分块传输时会返回错误。
 * 
 * @param event HTTP客户端事件结构体指针，包含接收到的数据及相关信息
 * @param buffer 字符串引用，用于存储接收到的数据
 * @return esp_err_t 成功返回ESP_OK，失败返回相应错误码
 *         - ESP_ERR_NOT_SUPPORTED: 当检测到HTTP分块传输时返回此错误码
 */
esp_err_t http_event_on_data_handler(esp_http_client_event_t* event, std::string& buffer){
    // 检查是否为HTTP分块传输响应（当前实现不支持分块传输）
    if(esp_http_client_is_chunked_response(event->client)){
        ESP_LOGE(LOG_TAG, "当前代码不支持分块传输");
        return ESP_ERR_NOT_SUPPORTED;
    }

    // 将接收到的数据追加到字符串缓冲区中
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

/**
 * @brief 从天气API获取指定城市的天气数据
 * 
 * 该函数接收城市编码作为参数，构建天气API请求URL，通过HTTP客户端发送GET请求获取天气数据。
 * 获取成功后会记录状态码和内容长度；获取失败时会记录错误信息，并设置错误提示HTML内容。
 * 
 * @param city_code 城市编码字符串，用于指定要查询天气的城市
 * @return esp_err_t 成功返回ESP_OK，失败返回相应的ESP错误码
 *         - ESP_ERR_INVALID_ARG: 当city_code为NULL时返回此错误码
 *         - 其他esp_err_t错误码: 取决于HTTP请求过程中发生的具体错误
 */
esp_err_t wi_fetch_weather(const char *city_code){
    // 检查城市编码参数是否有效（非NULL）
    ESP_ERROR_CHECK(city_code == NULL ? ESP_ERR_INVALID_ARG : ESP_OK);
    
    // 记录开始获取天气数据的日志信息，包含目标城市编码
    ESP_LOGI(LOG_TAG, "尝试获取天气数据, 城市编码为: %s", city_code);
    
    // 构建完整的天气API请求URL
    std::string url{WEATHER_API_BASE_URL};
    url += city_code;

    // 配置HTTP客户端参数
    esp_http_client_config_t weather_api_config = {
        .url = url.c_str(),        // 设置请求的URL
        .method = HTTP_METHOD_GET, // 使用GET请求方法
        .timeout_ms = 5000,        // 设置请求超时时间为5000毫秒
        .event_handler = http_event_handler // 设置HTTP事件处理函数
    };
    
    // 初始化HTTP客户端实例
    const esp_http_client_handle_t client = esp_http_client_init(&weather_api_config);

    // 执行HTTP请求，获取天气数据
    esp_err_t err_code = esp_http_client_perform(client);
    
    // 处理HTTP请求结果
    if(err_code != ESP_OK){
        // 请求失败时，记录错误信息并设置错误提示HTML内容
        ESP_LOGE(LOG_TAG, "获取天气数据失败, 错误(%d):%s", err_code, esp_err_to_name(err_code));
        fetch_html_content = "<p>网络异常, 获取天气数据失败</p>";
    }else{
        // 请求成功时，获取并记录HTTP状态码和响应内容长度
        int status = esp_http_client_get_status_code(client);
        int64_t length = esp_http_client_get_content_length(client);
        ESP_LOGI(LOG_TAG, "获取天气数据成功, HTTP GET = %d 内容长度 = %lld", status, length);
    }

    // 清理并释放HTTP客户端资源
    esp_http_client_cleanup(client);
    
    // 返回HTTP请求的结果状态码
    return err_code;
}

/**
 * @brief 合并HTML内容与历史记录数据生成完整HTML页面
 * 
 * 该函数将HTML模板内容、天气查询历史记录和当前天气数据HTML内容合并成一个完整的HTML页面。
 * 首先从NVS加载历史记录数据，然后将历史记录和天气数据插入到HTML模板的相应位置。
 * 
 * @param result 输出缓冲区指针，用于存储合并后的完整HTML内容
 * @param index_content HTML模板内容，包含用于插入历史记录和天气数据的格式占位符
 * @param html_content_length 输入输出参数，指向HTML内容长度的指针，函数会更新此值以包含历史记录的长度
 * @return esp_err_t 成功返回ESP_OK
 */
esp_err_t wi_fetch_html_join(char *result, const char* index_content, size_t* html_content_length){
    // 静态数组，用于临时存储从NVS加载的历史记录数据
    static char history_data[100];
    
    // 从NVS加载天气查询历史记录
    wi_nvs_load_history(history_data);
    
    // 更新HTML内容长度，加上历史记录数据的长度
    *html_content_length += strlen(history_data);

    // 使用snprintf函数将HTML模板、历史记录和天气数据合并到结果缓冲区
    // index_content中应包含格式占位符，分别对应history_data和fetch_html_content.c_str()
    snprintf(result, *html_content_length, index_content, history_data, fetch_html_content.c_str());

    // 函数执行成功，返回ESP_OK
    return ESP_OK;
}

/**
 * @brief 获取当前天气数据HTML内容的大小
 * 
 * 该函数返回存储天气数据的HTML内容字符串的长度，用于在生成完整HTML页面时
 * 确定所需的缓冲区大小，避免缓冲区溢出。
 * 
 * @return size_t 返回天气数据HTML内容的字节长度
 */
size_t wi_get_fetch_html_size(){
    // 返回全局变量fetch_html_content中存储的HTML内容的大小
    return fetch_html_content.size();
}

/**
 * @brief 清除已获取的天气数据HTML内容
 * 
 * 此函数用于清空存储天气数据的HTML内容字符串，通常在重新获取天气数据前调用
 * 或需要重置显示内容时使用
 */
void wi_fetch_clear(){
    // 记录清除天气数据的日志信息
    ESP_LOGI(LOG_TAG, "清除天气数据");
    // 清空存储天气数据的HTML内容字符串
    fetch_html_content.clear();
}