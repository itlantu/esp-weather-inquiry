#include "wi/nvs.h"
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"

#define LOG_TAG "wi_nvs"

// 是否初始化nvs标志位
char nvs_init_flag = 0;

/**
 * @brief 初始化ESP32的NVS
 * @details 此函数负责初始化ESP32的NVS flash存储系统，用于保存持久化数据
 *          如果NVS已经初始化，则直接返回成功，避免重复初始化
 *          如果遇到NVS版本不兼容或没有可用页的情况，会先擦除NVS再重新初始化
 * @return esp_err_t 返回ESP_OK表示初始化成功，其他错误码表示初始化失败
 */
esp_err_t wi_nvs_init() {
    // 记录日志，表示开始执行NVS初始化
    ESP_LOGI(LOG_TAG, "执行wi_nvs_init");
    
    // 检查NVS是否已经初始化，避免重复初始化
    if (nvs_init_flag) {
        // 如果已初始化，直接返回成功
        return ESP_OK;
    }

    // 执行NVS flash初始化操作
    const esp_err_t err_code = nvs_flash_init();
    
    // 检查初始化结果，如果是NVS无可用页或发现新版本，需要先擦除再重新初始化
    if (err_code == ESP_ERR_NVS_NO_FREE_PAGES || err_code == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 擦除NVS flash内容，ESP_ERROR_CHECK会在执行失败时自动中止程序
        ESP_ERROR_CHECK(nvs_flash_erase());
        // 重新初始化NVS flash
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    // 设置初始化标志为1，表示NVS已经成功初始化
    nvs_init_flag = 1;

    // 返回成功状态
    return ESP_OK;
}

/**
 * @brief 从NVS加载设备配置
 * 
 * 此函数负责从ESP32的NVS分区中读取之前保存的WiFi SSID和密码配置，
 * 并将其加载到全局配置结构体WI_Config中，供系统其他组件使用
 * 
 * @return esp_err_t ESP_OK表示加载成功，其他值表示加载失败
 */
esp_err_t wi_nvs_load_config() {
    // 初始化NVS系统，为后续的配置读写操作做准备
    ESP_LOGI(LOG_TAG, "执行wi_nvs_load_config");
    wi_nvs_init();

    nvs_handle_t nvs_handle;
    // 打开NVS存储区域，使用WI_NVS_KEY_CONFIG_SSID作为命名空间，以读写模式访问
    esp_err_t err_code = nvs_open(WI_NVS_KEY_CONFIG_SSID, NVS_READWRITE, &nvs_handle);
    if (err_code != ESP_OK) {
        // 记录打开NVS存储失败的错误信息
        ESP_LOGW(LOG_TAG, "从NVS中打开Config SSID失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
        return err_code;
    }
    
    // 从NVS中读取WiFi SSID配置到全局配置结构体中
    size_t length = 32;
    err_code = nvs_get_str(nvs_handle, WI_NVS_KEY_CONFIG_SSID, WI_Config.sta.ssid, &length);
    if (err_code != ESP_OK) {
        // 记录读取SSID失败的错误信息
        ESP_LOGW(LOG_TAG, "从NVS中读取Config SSID失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
        return err_code;
    }
    
    // 从NVS中读取WiFi密码配置到全局配置结构体中
    length = 32;
    err_code = nvs_get_str(nvs_handle, WI_NVS_KEY_CONFIG_PASSWORD, WI_Config.sta.password, &length);
    if (err_code != ESP_OK) {
        // 记录读取密码失败的错误信息
        ESP_LOGW(LOG_TAG, "从NVS中读取Config Password失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
        return err_code;
    }

    return ESP_OK;
}

/**
 * @brief 将设备配置保存到NVS
 * 
 * 此函数负责将全局配置结构体WI_Config中的WiFi SSID和密码保存到ESP32的NVS分区中，
 * 确保设备重启后配置信息不会丢失
 * 
 * @return esp_err_t ESP_OK表示保存成功，其他值表示保存失败
 */
esp_err_t wi_nvs_save_config() {
    // 初始化NVS系统，为后续的配置读写操作做准备
    wi_nvs_init();

    nvs_handle_t nvs_handle;
    // 打开NVS存储区域，使用WI_NVS_KEY_CONFIG_SSID作为命名空间，以读写模式访问
    esp_err_t err_code = nvs_open(WI_NVS_KEY_CONFIG_SSID, NVS_READWRITE, &nvs_handle);
    if (err_code != ESP_OK) {
        // 记录打开NVS存储失败的错误信息
        ESP_LOGW(LOG_TAG, "从NVS中打开Config SSID失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
        return err_code;
    }
    
    // 保存WiFi SSID到NVS存储中
    size_t length = sizeof(WI_Config.sta.ssid);
    err_code = nvs_set_str(nvs_handle, WI_NVS_KEY_CONFIG_SSID, WI_Config.sta.ssid);
    if (err_code != ESP_OK) {
        // 记录保存SSID失败的错误信息
        ESP_LOGW(LOG_TAG, "从NVS中保存Config SSID失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
        return err_code;
    }
    
    // 保存WiFi密码到NVS存储中
    length = sizeof(WI_Config.sta.password);
    err_code = nvs_set_str(nvs_handle, WI_NVS_KEY_CONFIG_PASSWORD, WI_Config.sta.password);
    if (err_code != ESP_OK) {
        // 记录保存密码失败的错误信息
        ESP_LOGW(LOG_TAG, "从NVS中保存config Password失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
        return err_code;
    }
    
    // 提交所有更改到NVS存储，确保数据被实际写入闪存
    err_code = nvs_commit(nvs_handle);
    if (err_code != ESP_OK) {
        // 记录提交更改失败的错误信息
        ESP_LOGW(LOG_TAG, "从NVS中提交Config失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
        return err_code;
    }

    return ESP_OK;
}

/**
 * @brief 从ESP32的NVS中加载历史天气查询数据
 * @details 此函数负责从ESP32的NVS中读取历史天气数据
 *          如果NVS尚未初始化，会先调用wi_nvs_init()进行初始化
 *          读取成功后，数据将被复制到传入的缓冲区中
 * @param[out] history_data 用于存储读取历史数据的字符数组缓冲区
 * @return esp_err_t 返回ESP_OK表示读取成功，其他错误码表示读取失败
 */
esp_err_t wi_nvs_load_history(char* history_data){
    // 确保NVS已初始化，如果未初始化则执行初始化操作
    wi_nvs_init();

    // 定义NVS句柄，用于后续NVS操作
    nvs_handle_t nvs_handle;
    
    // 打开NVS命名空间，使用读写模式
    esp_err_t err_code = nvs_open(WI_NVS_KEY_HISTORY, NVS_READWRITE, &nvs_handle);
    
    // 检查NVS打开是否成功
    if(err_code != ESP_OK){
        // 如果打开失败，记录警告日志，包含具体错误原因和错误码
        ESP_LOGW(LOG_TAG, "从NVS中打开History失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
        return err_code;
    }
        
    // 从NVS中读取字符串数据，覆盖到传入的history_data缓冲区
    // 设置缓冲区大小为50字节
    size_t length = 50;
    err_code = nvs_get_str(nvs_handle, WI_NVS_KEY_HISTORY, history_data, &length);
    
    // 检查读取是否成功
    if(err_code != ESP_OK) {
        // 读取失败时记录警告日志，包含尝试读取的长度和错误信息
        ESP_LOGW(LOG_TAG, "从NVS中读取History失败, 数据长度%d 错误原因: %s (%d)", length, esp_err_to_name(err_code), err_code);
    } else {
        // 读取成功时记录信息日志，包含实际读取的数据长度
        ESP_LOGI(LOG_TAG, "读取成功, 数据长度: %d", length);
    }
    
    // 关闭NVS句柄，释放资源
    nvs_close(nvs_handle);

    // 返回操作结果，成功返回ESP_OK，失败返回对应的错误码
    return err_code;
}

/**
 * @brief 将天气查询历史数据保存到ESP32的NVS中
 * @details 此函数负责将天气查询历史数据持久化存储到ESP32的NVS中
 *          如果NVS尚未初始化，会先调用wi_nvs_init()进行初始化
 *          写入数据后需要调用nvs_commit()确保数据被实际写入闪存
 * @param[in] history_data 要保存的历史天气数据字符串
 * @return esp_err_t 返回ESP_OK表示保存成功，其他错误码表示保存失败
 */
esp_err_t wi_nvs_save_history(const char* history_data){
    // 确保NVS已初始化，如果未初始化则执行初始化操作
    wi_nvs_init();

    // 定义NVS句柄，用于后续NVS操作
    nvs_handle_t nvs_handle;
    
    // 打开NVS命名空间，使用读写模式
    esp_err_t err_code = nvs_open(WI_NVS_KEY_HISTORY, NVS_READWRITE, &nvs_handle);
    
    // 检查NVS打开是否成功
    if(err_code != ESP_OK){
        // 如果打开失败，记录警告日志，包含具体错误原因和错误码
        ESP_LOGW(LOG_TAG, "从NVS中打开History失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
        return err_code;
    }

    // 将历史数据字符串写入NVS存储
    err_code = nvs_set_str(nvs_handle, WI_NVS_KEY_HISTORY, history_data);
    
    // 检查写入是否成功
    if(err_code != ESP_OK) {
        // 写入失败时记录警告日志，包含具体错误原因和错误码
        ESP_LOGW(LOG_TAG, "从NVS中写入History失败, 错误原因: %s (%d)", esp_err_to_name(err_code), err_code);
    }
    
    // 提交更改到NVS，确保数据被实际写入闪存
    // ESP_ERROR_CHECK会在执行失败时自动中止程序
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    
    // 记录写入成功的信息日志，包含写入的数据长度
    ESP_LOGI(LOG_TAG, "写入成功, 数据长度: %d", strlen(history_data));
        
    // 关闭NVS句柄，释放资源
    nvs_close(nvs_handle);

    // 返回操作结果，成功返回ESP_OK，失败返回对应的错误码
    return err_code;
}