#include "wi/comm/web.h"
#include <malloc.h>
#include <string.h>
#include "esp_log.h"

#include "wi/config.h"
#include "wi/parser/city_code.h"
#include "wi/comm/fetch_weather.h"

#define LOG_TAG "wi_web"

extern const uint8_t binary_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t binary_index_html_end[] asm("_binary_index_html_end");

const char *get_index_html(size_t *length) {
	const char *result = (const char *) binary_index_html_start;
	*length = binary_index_html_end - binary_index_html_start;
	ESP_LOGI(LOG_TAG, "index.html原始数据的长度为%u", *length);
	return result;
}

esp_err_t root_get_handler(httpd_req_t *req) {
	char *response = NULL;
	size_t response_length = 0;

	// 初始化response
	const char *index_html = get_index_html(&response_length);
	response_length += wi_get_fetch_html_size();
	// 格式化html内容
	ESP_ERROR_CHECK((response = malloc(response_length)) == NULL ? ESP_ERR_NO_MEM : ESP_OK);
	// snprintf(response, response_length, index_html, fetch_html_content);
	wi_fetch_html_join(response, index_html, response_length);
	
	size_t html_end_pos = 0;
	// ESP_LOGI(LOG_TAG, "[debug]解析内容数据(%d): %s", strlen(fetch_html_content), fetch_html_content);
	
	ESP_ERROR_CHECK(wi_paser_get_str_find(response, "</html>", &html_end_pos));
	// 更新response_length
	if(html_end_pos != 0)
		response_length -= response_length - html_end_pos;

	ESP_LOGI(LOG_TAG, "index.html解析后的数据(%d): %s", strlen(index_html), index_html);
	ESP_LOGI(LOG_TAG, "解析后的index.html数据的长度为%u", response_length);
	
	// 发送网页
	httpd_resp_set_type(req, "text/html; charset=utf-8");
	httpd_resp_send(req, response, response_length);

	free(response);

	return ESP_OK;
}

esp_err_t weather_handler(httpd_req_t *req) {
	static char city_name[10];
	static char city_code[10];
	static char content[1024];

	int data_length = httpd_req_recv(req, content, sizeof(content) - 1);

	if (data_length <= 0) {
		ESP_LOGW(LOG_TAG, "未接收到数据");
		return ESP_ERR_INVALID_STATE;
	}
	// 确保字符串终止
	content[data_length] = '\0';
	ESP_LOGI(LOG_TAG, "接收到POST数据: %s", content);

	// 解码城市数据
	ESP_ERROR_CHECK(wi_paser_post(content, data_length, city_name));
	if(wi_get_city_code(city_code, city_name) == ESP_OK)
		ESP_LOGI(LOG_TAG, "城市名称: \"%s\", 城市代码: %s", city_name, city_code);
	else
		ESP_LOGE(LOG_TAG, "城市名称: \"%s\", 城市代码获取失败", city_name);

	// 获取天气
	wi_fetch_weather(city_code);
	// 发送网页
	root_get_handler(req);
	wi_fetch_clear();
	return ESP_OK;
};

const httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get_handler};
const httpd_uri_t weather = {.uri = "/weather", .method = HTTP_POST, .handler = weather_handler};

esp_err_t wi_start_webserver(httpd_handle_t *server) {
	static httpd_config_t web_httpd_config = HTTPD_DEFAULT_CONFIG();

	ESP_LOGI(LOG_TAG, "执行start_webserver");

	// 启动httpd服务器
	web_httpd_config.server_port = WI_Config.web.port;
	const esp_err_t err_code = httpd_start(server, &web_httpd_config);
	if (err_code != ESP_OK) {
		ESP_LOGE(LOG_TAG, "启动web服务器出错!");
		return err_code;
	}

	// 注册URI处理函数
	ESP_ERROR_CHECK(httpd_register_uri_handler(*server, &root));
	ESP_ERROR_CHECK(httpd_register_uri_handler(*server, &weather));

	return ESP_OK;
}
