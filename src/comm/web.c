#include "wi/comm/web.h"
#include <malloc.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"

#include "wi/config.h"
#include "wi/parser/city_code.h"
#include "wi/comm/fetch_weather.h"
#include "wi/nvs.h"

#define LOG_TAG "wi_web"

extern const uint8_t binary_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t binary_index_html_end[] asm("_binary_index_html_end");
extern const uint8_t binary_config_html_start[] asm("_binary_config_html_start");
extern const uint8_t binary_config_html_end[] asm("_binary_config_html_end");

const char *get_index_html(size_t *length) {
	const char *result = (const char *) binary_index_html_start;
	*length = binary_index_html_end - binary_index_html_start;
	ESP_LOGI(LOG_TAG, "index.html原始数据的长度为%u", *length);
	return result;
}

/**
 * @brief HTTP服务器根路径("/")的GET请求处理函数
 * 
 * @param req HTTP请求对象指针，包含客户端请求的相关信息
 * @return esp_err_t 操作结果，ESP_OK表示成功
 * 
 * 该函数负责处理Web服务器根路径的GET请求，主要流程是：
 * 1. 获取基础HTML模板
 * 2. 合并天气数据相关HTML内容
 * 3. 处理并发送完整的HTML响应给客户端
 */
esp_err_t root_get_handler(httpd_req_t *req) {
    // 响应缓冲区指针和长度变量
    char *response = NULL;
    size_t response_length = 0;

    // 获取基础HTML模板并获取其长度
    const char *index_html = get_index_html(&response_length);
    // 计算总响应长度 = 基础HTML长度 + 天气数据HTML长度
    response_length += wi_get_fetch_html_size();
    
    // 为响应分配内存空间，失败时返回内存错误
    ESP_ERROR_CHECK((response = malloc(response_length)) == NULL ? ESP_ERR_NO_MEM : ESP_OK);
    // 将基础HTML和天气数据HTML合并到响应缓冲区
    wi_fetch_html_join(response, index_html, &response_length);

    // 用于存储HTML结束标签的位置
    size_t html_end_pos = 0;

    // 查找HTML响应中的</html>结束标签位置，确保响应完整性
    ESP_ERROR_CHECK(wi_paser_get_str_find(response, "</html>", &html_end_pos));
    
    // 如果找到了结束标签，更新响应长度为从开始到结束标签的实际长度
    if (html_end_pos != 0) {
        response_length -= response_length - html_end_pos;
    }

    // 记录最终处理后的HTML数据长度到日志
    ESP_LOGI(LOG_TAG, "解析后的index.html数据的长度为%u", response_length);

    // 设置HTTP响应类型为HTML，并指定UTF-8字符编码
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    // 发送完整的HTML响应给客户端
    httpd_resp_send(req, response, response_length);

    // 释放之前分配的响应缓冲区内存
    free(response);

    // 返回成功状态码
    return ESP_OK;
}

/**
 * @brief 处理天气查询请求的HTTP处理函数
 * @param req HTTP请求对象指针，包含客户端发送的POST请求数据
 * @return esp_err_t 操作结果，ESP_OK表示成功，其他值表示错误
 * 
 * 该函数负责处理客户端提交的天气查询请求，主要流程是：
 * 1. 接收客户端提交的城市名称数据
 * 2. 解析城市名称并获取对应的城市代码
 * 3. 根据城市代码获取天气数据
 * 4. 调用root_get_handler刷新并显示更新后的天气信息页面
 * 5. 清理相关资源
 */
esp_err_t weather_post_handler(httpd_req_t *req) {
    // 静态变量用于存储城市名称、城市代码和请求内容
    static char city_name[10];    // 存储客户端请求的城市名称
    static char city_code[10];    // 存储根据城市名称获取的城市代码
    static char content[1024];    // 存储接收到的HTTP POST请求内容

    // 接收HTTP POST请求中的数据
    int data_length = httpd_req_recv(req, content, sizeof(content) - 1);

    // 检查数据接收是否成功
    if (data_length <= 0) {
        ESP_LOGW(LOG_TAG, "未接收到数据");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 确保字符串正确终止，防止后续操作出现缓冲区溢出问题
    content[data_length] = '\0';
    ESP_LOGI(LOG_TAG, "接收到POST数据: %s", content);

    // 从接收到的数据中解析出城市名称
    ESP_ERROR_CHECK(wi_paser_post(content, data_length, city_name));
    
    // 根据城市名称获取对应的城市代码，并记录日志
    if (wi_get_city_code(city_code, city_name) == ESP_OK) {
        ESP_LOGI(LOG_TAG, "城市名称: \"%s\", 城市代码: %s", city_name, city_code);
    } else {
        ESP_LOGE(LOG_TAG, "城市名称: \"%s\", 城市代码获取失败", city_name);
    }

    // 使用获取到的城市代码请求天气数据
    wi_fetch_weather(city_code);
    
    // 重新调用根路径处理函数，刷新并显示包含最新天气数据的页面
    root_get_handler(req);
    
    // 清理天气数据资源
    wi_fetch_clear();
    
    // 返回成功状态码
    return ESP_OK;
}

void split_ssid_password(const char* input, char* ssid, char* password) {
    // 提取 ssid
    const char* ssid_prefix = "ssid=";
    char* ssid_start = strstr(input, ssid_prefix);
    if (ssid_start != NULL) {
        ssid_start += strlen(ssid_prefix);
        char* ssid_end = strstr(ssid_start, "&");
        if (ssid_end != NULL) {
            strncpy(ssid, ssid_start, ssid_end - ssid_start);
        } else {
            strcpy(ssid, ssid_start);
        }
    }

    // 提取 password
    const char* pwd_prefix = "password=";
    char* pwd_start = strstr(input, pwd_prefix);
    if (pwd_start != NULL) {
        pwd_start += strlen(pwd_prefix);
        strcpy(password, pwd_start);
    }
}

esp_err_t config_get_handler(httpd_req_t *req){
    const char *response = (const char *) binary_config_html_start;
    // 正确计算HTML文件长度（结束地址减开始地址）
    size_t response_length = binary_config_html_end - binary_config_html_start;
    size_t html_end_pos = 0;

    // 查找HTML响应中的</html>结束标签位置，确保响应完整性
    ESP_ERROR_CHECK(wi_paser_get_str_find(response, "</html>", &html_end_pos));
    
    // 如果找到了结束标签，更新响应长度为从开始到结束标签的实际长度
    if (html_end_pos != 0) 
        response_length -= response_length - html_end_pos;
    

    // 日志消息中使用正确的文件名
    ESP_LOGI(LOG_TAG, "解析后的config.html数据的长度为%u", response_length);

    // 设置HTTP响应类型为HTML，并指定UTF-8字符编码
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    // 发送完整的HTML响应给客户端
    httpd_resp_send(req, response, response_length);
    return ESP_OK;
}

esp_err_t config_set_handler(httpd_req_t *req){
    static char ssid[32] = {0};
    static char password[32] = {0};
    static char content[128] = {0};
    static char encode[128] = {0};

    // 接收HTTP POST请求中的数据
    int data_length = httpd_req_recv(req, content, sizeof(content) - 1);

    // 检查数据接收是否成功
    if (data_length <= 0) {
        ESP_LOGW(LOG_TAG, "未接收到数据");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 确保字符串正确终止，防止后续操作出现缓冲区溢出问题
    content[data_length] = '\0';
    ESP_LOGI(LOG_TAG, "接收到POST数据: %s", content);
    // 解码
    wi_url_decode(encode, content);
    ESP_LOGI(LOG_TAG, "解码后数据: %s", encode);

    split_ssid_password(encode, ssid, password);
    strcpy(WI_Config.ap.ssid, ssid);
    strcpy(WI_Config.ap.password, password);
    ESP_LOGI(LOG_TAG, "SSID: %s, Password: %s", WI_Config.ap.ssid, WI_Config.ap.password);

    
    wi_nvs_save_config();

    ESP_LOGE(LOG_TAG, "软件重启");
    esp_restart();
    
    // 返回成功状态码
    return config_get_handler(req);
}

const httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get_handler};
const httpd_uri_t weather = {.uri = "/", .method = HTTP_POST, .handler = weather_post_handler};
const httpd_uri_t config_get = {.uri = "/config", .method = HTTP_GET, .handler = config_get_handler};
const httpd_uri_t config_set = {.uri = "/config", .method = HTTP_POST, .handler = config_set_handler};



/**
 * @brief 启动Web服务器并注册URI处理函数
 * @param server 输出参数，用于存储启动后的HTTP服务器句柄
 * @return esp_err_t 操作结果，ESP_OK表示成功，其他值表示错误
 * 
 * 该函数负责初始化和启动ESP32的HTTP服务器，主要流程是：
 * 1. 设置服务器配置（端口号等）
 * 2. 启动HTTP服务器
 * 3. 注册URI处理函数，使服务器能够响应不同路径的请求
 */
esp_err_t wi_start_webserver(httpd_handle_t *server, httpd_config_t* web_httpd_config) {
    // 记录函数执行日志
    ESP_LOGI(LOG_TAG, "执行start_webserver");
    
    // 启动HTTP服务器，并将服务器句柄保存到传入的指针中
    const esp_err_t err_code = httpd_start(server, web_httpd_config);
    
    // 检查服务器启动是否成功
    if (err_code != ESP_OK) {
        ESP_LOGE(LOG_TAG, "启动web服务器出错!");
        return err_code; // 如果启动失败，返回具体错误码
    }

    // 注册根路径("/")的URI处理函数
    ESP_ERROR_CHECK(httpd_register_uri_handler(*server, &root));
    // 注册天气查询路径的URI处理函数
    ESP_ERROR_CHECK(httpd_register_uri_handler(*server, &weather));
    // 注册配置页面的GET处理函数
    ESP_ERROR_CHECK(httpd_register_uri_handler(*server, &config_get));
    // 注册配置页面的POST处理函数
    ESP_ERROR_CHECK(httpd_register_uri_handler(*server, &config_set));

    // 服务器启动和URI注册成功，返回成功状态码
    return ESP_OK;
}