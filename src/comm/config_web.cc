#include <malloc.h>
#include <string.h>
#include "esp_log.h"

#include "wi/config.h"
#include "wi/parser/city_code.h"
#include "wi/comm/fetch_weather.h"
#include "wi/comm/config_web.h"

#define LOG_TAG "wi_config_web"

// 修复：正确声明HTML文件的结束符号
extern const uint8_t binary_config_html_start[] asm("_binary_config_html_start");
extern const uint8_t binary_config_html_end[] asm("_binary_config_html_end");


esp_err_t config_get_handler(httpd_req_t *req){
    const char *response = (const char *) binary_config_html_start;
    // 修复：正确计算HTML文件长度（结束地址减开始地址）
    size_t response_length = binary_config_html_end - binary_config_html_start;
    size_t html_end_pos = 0;

    // 查找HTML响应中的</html>结束标签位置，确保响应完整性
    ESP_ERROR_CHECK(wi_paser_get_str_find(response, "</html>", &html_end_pos));
    
    // 如果找到了结束标签，更新响应长度为从开始到结束标签的实际长度
    if (html_end_pos != 0) {
        response_length = html_end_pos + strlen("</html>");
    }

    // 修复：日志消息中使用正确的文件名
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

    ESP_LOGI(LOG_TAG, "POST数据: %s", content);
    // 解码
    wi_url_decode(encode, content);
    ESP_LOGI(LOG_TAG, "解码后数据: %s", encode);
    
    // 返回成功状态码
    return ESP_OK;
}

const httpd_uri_t config_get = {.uri = "/config", .method = HTTP_GET, .handler = config_get_handler};
// 修复：POST请求使用正确的处理函数
const httpd_uri_t config_set = {.uri = "/config", .method = HTTP_POST, .handler = config_set_handler};

esp_err_t wi_start_config_webserver(httpd_handle_t *server){
     // 使用静态变量存储HTTP服务器配置，确保配置在函数调用之间保持
    static httpd_config_t web_httpd_config = HTTPD_DEFAULT_CONFIG();
    web_httpd_config.ctrl_port = 8080;

    // 记录函数执行日志
    ESP_LOGI(LOG_TAG, "执行start_webserver");

    // 配置服务器端口号，从全局配置中获取
    web_httpd_config.server_port = WI_Config.web.port;
    
    // 启动HTTP服务器，并将服务器句柄保存到传入的指针中
    const esp_err_t err_code = httpd_start(server, &web_httpd_config);
    
    // 检查服务器启动是否成功
    if (err_code != ESP_OK) {
        ESP_LOGE(LOG_TAG, "启动web服务器出错!");
        return err_code; // 如果启动失败，返回具体错误码
    }

    // 注册配置页面的GET处理函数
    ESP_ERROR_CHECK(httpd_register_uri_handler(*server, &config_get));
    // 注册配置页面的POST处理函数
    ESP_ERROR_CHECK(httpd_register_uri_handler(*server, &config_set));
    
    // 服务器启动和URI注册成功，返回成功状态码
    return ESP_OK;
}