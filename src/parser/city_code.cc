#include "wi/parser/city_code.h"
#include <string_view>
#include <cstring>
#include <map>
#include "esp_log.h"
#include "esp_http_server.h"

#define LOG_TAG "wi_city_code"

struct CStrCompare{
    bool operator()(const char* a, const char* b) const{
        return std::strcmp(a, b) < 0;
    }
};

const std::map<const char*, const char*, CStrCompare> city_codes{
    #include "wi/parser/city_code.inc"
};


esp_err_t wi_get_city_code(char* code, const char* name){
    if(!city_codes.count(name)){
        // 找不到键
        ESP_LOGW(LOG_TAG, "找不到键: %s", name);
        return ESP_FAIL;
    }
    strcpy(code, city_codes.at(name));
    return ESP_OK;
}

int hex_to_int(char c) {
    if (c >= '0' && c <= '9') 
        return c - '0';
    if (c >= 'A' && c <= 'F') 
        return 10 + c - 'A';
    if (c >= 'a' && c <= 'f') 
        return 10 + c - 'a';
    return -1; // 无效字符
}

// URL 解码函数：将 %XX 格式的编码转换为原始字符（支持中文等UTF-8编码）
esp_err_t url_decode(char* result, const std::string_view& encode_data) {
    int n = encode_data.size();
    int j = 0;
    for (int i = 0; i < n; ) {
        if (encode_data[i] == '%' && i + 2 < n) {
            // 解析 %XX 格式
            int hex1 = hex_to_int(encode_data[i + 1]);
            int hex2 = hex_to_int(encode_data[i + 2]);
            if (hex1 != -1 && hex2 != -1) {
                // 合并两个十六进制字符为一个字节
                char c = (hex1 << 4) | hex2;
                result[j++] = c;
                i += 3; // 跳过 %XX
                continue;
            }
        } else if (encode_data[i] == '+') {
            // URL 中 '+' 通常表示空格
            result[j++] = ' ';
        } else {
            // 普通字符直接添加
            result[j++] = encode_data[i];
        }
        i++;
    }
    return ESP_OK;
}

esp_err_t wi_paser_post(const char* buffer, const int data_length, char* city_name){
    std::string_view encode{buffer};

    city_name[0] = '\0';
    size_t start_pos = encode.find("city=");
    if (start_pos == std::string_view::npos) {
        ESP_LOGW(LOG_TAG, "POST数据格式错误, 未检测city参数");
        return ESP_FAIL;
    }
    
    // 解码
    if(url_decode(city_name, encode.substr(start_pos + 5)) != ESP_OK){
        ESP_LOGE(LOG_TAG, "POST数据city参数解码错误");
        return ESP_FAIL;
    }
    // todo如果结尾为"市", 则删除"市"
    // encode = city_name;
    // auto city_name_length = encode.size();
    // ESP_LOGI(LOG_TAG, "测试: %s", encode.substr(encode.size() - 1).data());
    // if(encode.substr(encode.size() - 1) == "市"){
    //     city_name[city_name_length - 1] = '\0';
    // }

    return ESP_OK;
}

esp_err_t wi_paser_get_str_find(const char* str, const char* find, size_t* start_pos){
    std::string_view str_view{str};
    size_t pos = str_view.find(find);

    if(pos == std::string_view::npos)
        return ESP_FAIL;
    *start_pos = pos;
    
    return ESP_OK;
}