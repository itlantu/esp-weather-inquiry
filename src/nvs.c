#include "wi/nvs.h"
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"

#define LOG_TAG "wi_nvs"

// 是否初始化nvs标志位
char nvs_init_flag = 0;

esp_err_t wi_nvs_init() {
	ESP_LOGI(LOG_TAG, "执行vi_nvs_init");
	if (nvs_init_flag)
		return ESP_OK;

	const esp_err_t err_code = nvs_flash_init();
	if (err_code == ESP_ERR_NVS_NO_FREE_PAGES || err_code == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ESP_ERROR_CHECK(nvs_flash_init());
	}
	nvs_init_flag = 1;

	return ESP_OK;
}

esp_err_t wi_nvs_load_history(char* history_data){
	wi_nvs_init();

	nvs_handle_t nvs_handle;
	esp_err_t err_code = nvs_open(WI_NVS_KEY_HISTORY, NVS_READWRITE, &nvs_handle);
	if(err_code != ESP_OK){
		ESP_LOGW(LOG_TAG, "从NVS中打开History失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
		return err_code;
	}
		
	// NVS读取, 将NVS的内容覆盖到history
	size_t length = 50;
	err_code = nvs_get_str(nvs_handle, WI_NVS_KEY_HISTORY, history_data, &length);	
	if(err_code != ESP_OK)
		ESP_LOGW(LOG_TAG, "从NVS中读取History失败, 数据长度%d 错误原因: %s (%d)", length, esp_err_to_name(err_code), err_code);
	else
		ESP_LOGI(LOG_TAG, "读取成功, 数据长度: %d", length);
	nvs_close(nvs_handle);

	return err_code;
}

esp_err_t wi_nvs_save_history(const char* history_data){
	wi_nvs_init();

	nvs_handle_t nvs_handle;
	esp_err_t err_code = nvs_open(WI_NVS_KEY_HISTORY, NVS_READWRITE, &nvs_handle);
	if(err_code != ESP_OK){
		ESP_LOGW(LOG_TAG, "从NVS中打开History失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
		return err_code;
	}

	// NVS写入, 将history的内容写入NVS
	err_code = nvs_set_str(nvs_handle, WI_NVS_KEY_HISTORY, history_data);
	if(err_code != ESP_OK)
		ESP_LOGW(LOG_TAG, "从NVS中写入History失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
	ESP_ERROR_CHECK(nvs_commit(nvs_handle));
	ESP_LOGI(LOG_TAG, "写入成功, 数据长度: %d", strlen(history_data));
		
	nvs_close(nvs_handle);

	return err_code;
}