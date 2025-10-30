#include "wi/comm/web.h"
#include <malloc.h>
#include <string.h>
#include "esp_log.h"

#include "wi/config.h"
#include "wi/parser/city_code.h"

#define LOG_TAG "wi_web"

extern const uint8_t binary_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t binary_index_html_end[] asm("_binary_index_html_end");

const char *get_index_html(size_t *length) {
	const size_t index_html_length = binary_index_html_end - binary_index_html_start;
	const char *result = (const char *) binary_index_html_start;

	ESP_LOGI(LOG_TAG, "index.html计算后的长度为%u", index_html_length);
	*length = index_html_length - 2;
	return result;
}

esp_err_t url_decode(char* result, const char *str) {
	size_t len = strlen(str);
	if(result == NULL)
		return ESP_FAIL;

	size_t i, j = 0;
	for (i = 0; i < len; ++i) {
		if (str[i] == '%' && i + 2 < len) {
			char hex[3] = {str[i + 1], str[i + 2], '\0'};
			char *endptr;
			long hex_value = strtol(hex, &endptr, 16);

			if (*endptr == '\0') {
				result[j++] = (char) hex_value;
				i += 2;
			} else 
				result[j++] = str[i];
			
		} else if (str[i] == '+')
			result[j++] = ' ';
		else 
			result[j++] = str[i];
	}
	result[j] = '\0';
	return ESP_OK;
}

esp_err_t root_get_handler(httpd_req_t *req) {
	static char *response = NULL;
	static size_t response_length = 0;

	// 初始化response
	if (response == NULL) {
		ESP_LOGI(LOG_TAG, "root_get_handler初始化html页面内容");

		size_t index_html_length;
		const char *index_html = get_index_html(&index_html_length);
		response_length = index_html_length - 2;

		if ((response = malloc(index_html_length)) == NULL) {
			ESP_LOGE(LOG_TAG, "response内存分配失败");
			return ESP_FAIL;
		}
		memset(response, 0, index_html_length);
		snprintf(response, index_html_length, index_html, "");
	}

	// 发送网页
	httpd_resp_set_type(req, "text/html; charset=utf-8");
	httpd_resp_send(req, response, response_length);

	return ESP_OK;
}
esp_err_t weather_handler(httpd_req_t *req) {
	static char buffer[1024];
	static char city_name[10];
	static char city_code[10];

	int data_length = httpd_req_recv(req, buffer, sizeof(buffer) - 1);

	if (data_length <= 0) {
		ESP_LOGW(LOG_TAG, "未接收到数据");
		return ESP_ERR_INVALID_STATE;
	}
	// 确保字符串终止
	buffer[data_length] = '\0';
	ESP_LOGI(LOG_TAG, "接收到POST数据: %s", buffer);

	// 解码城市数据
	ESP_ERROR_CHECK(wi_paser_post(buffer, data_length, city_name));
	if(wi_get_city_code(city_code, city_name) == ESP_OK)
		ESP_LOGI(LOG_TAG, "城市名称: \"%s\", 城市代码: %s", city_name, city_code);
	else
		ESP_LOGE(LOG_TAG, "城市名称: \"%s\", 城市代码获取失败", city_name);

	// todo 发送网页

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
