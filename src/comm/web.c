#include "wi/comm/web.h"
#include <malloc.h>
#include <string.h>
#include "esp_log.h"

#include "wi/config.h"

#define LOG_TAG "wi_web"

extern const uint8_t binary_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t binary_index_html_end[] asm("_binary_index_html_end");

const char* get_index_html(size_t* length) {
	size_t index_html_length = binary_index_html_end - binary_index_html_start;
	const char* result = (const char*)binary_index_html_start;

	// 打印原始内容
	// ESP_LOGI(LOG_TAG, "打印前 %d 个字节：\n%.*s\n", *length, *index_html_length, binary_index_html_start);

	ESP_LOGI(LOG_TAG, "index.html计算后的长度为%u", index_html_length);
	*length = index_html_length - 2;
	return result;
}

esp_err_t root_get_handler(httpd_req_t *req) {
	static char* response = NULL;
	static size_t response_length = 0;

	// 初始化response
	if(response == NULL) {
		ESP_LOGI(LOG_TAG, "root_get_handler初始化html页面内容");

		size_t index_html_length;
		const char* index_html = get_index_html(&index_html_length);
		response_length = index_html_length - 2;

		if((response = malloc(index_html_length)) == NULL){
			ESP_LOGE(LOG_TAG, "response内存分配失败");
			return ESP_FAIL;
		}
		memset(response, 0, index_html_length);
		snprintf(response, index_html_length, index_html, "");
	}

	// 发送网页
	httpd_resp_set_type(req, "text/html");
+	httpd_resp_send(req, response, response_length);

	return ESP_OK;
}
esp_err_t weather_handler(httpd_req_t *req) {
	static char buffer[1024];
	int data_length = httpd_req_recv(req, buffer, sizeof(buffer) - 1);

	if (data_length <= 0) {
		ESP_LOGW(LOG_TAG, "未接收到数据");
		return ESP_ERR_INVALID_STATE;
	}
	// 确保字符串终止
	buffer[data_length] = '\0'; 
	ESP_LOGI(LOG_TAG, "接收到POST数据: %s", buffer);

	// todo 发送网页

	return ESP_OK;
};


const httpd_uri_t root = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = root_get_handler
};

const httpd_uri_t weather = {
	.uri = "/weather",
	.method = HTTP_POST,
	.handler = weather_handler
};

esp_err_t wi_start_webserver(httpd_handle_t* server) {
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