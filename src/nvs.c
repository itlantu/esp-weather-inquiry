#include "wi/nvs.h"
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