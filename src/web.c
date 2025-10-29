#include "wi/web.h"
#include <malloc.h>
#include <string.h>
#include "esp_log.h"

#include "wi/config.h"

#define LOG_TAG "wi_web"

extern const uint8_t binary_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t binary_index_html_end[] asm("_binary_index_html_end");

const char* get_index_html(size_t* length) {
	*length = binary_index_html_end - binary_index_html_start;
	ESP_LOGI(LOG_TAG, "index.html计算后的长度为%u", *length);

	// 打印原始内容
	// ESP_LOGI(LOG_TAG, "打印前 %d 个字节：\n%.*s\n", *length, *length, binary_index_html_start);

	const char* result = (const char*)binary_index_html_start;
	return result;
}

esp_err_t root_get_handler(httpd_req_t *req) {
	size_t index_html_length;
	const char* index_html = get_index_html(&index_html_length);

	// 移除了最后两个导致乱码的字符
	index_html_length -= 2;
	char* response = malloc(sizeof(char) * (index_html_length + 1));
	for (size_t i = 0; i < index_html_length; ++i)
		response[i] = index_html[i];
	response[index_html_length] = '\0';

	// 发送网页
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);

	// 回收内存
	free(response);

	return ESP_OK;
}
esp_err_t weather_get_handler(httpd_req_t *req) {
	return ESP_OK;
};

esp_err_t wi_start_webserver(httpd_handle_t* server) {
	static httpd_config_t web_httpd_config = HTTPD_DEFAULT_CONFIG();
	static const httpd_uri_t root = {
		.uri = "/",
		.method = HTTP_GET,
		.handler = root_get_handler
	};
	static const httpd_uri_t weather = {
		.uri = "/weather",
		.method = HTTP_GET,
		.handler = weather_get_handler
	};

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