#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wi/wifi.h"

#define LOG_TAG "wi_main"

void app_main(void){
	ESP_LOGI(LOG_TAG, "app main函数执行");
	wi_wifi_init();

	while (1) {
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}