#include "wi/parser/city_code.h"
#include <string_view>
#include <cstring>
#include <map>
#include "esp_log.h"
#include "esp_http_server.h"

#define LOG_TAG "wi_city_code"

struct CStrCompare{
    /**
     * @brief 字符串比较运算符重载
     * 
     * 此函数为重载的函数调用运算符，用于比较两个C风格字符串的字典序
     * 主要用于城市代码的排序或在有序容器中作为比较器使用
     * 
     * @param a 第一个要比较的字符串指针
     * @param b 第二个要比较的字符串指针
     * @return bool 如果a的字典序小于b，返回true；否则返回false
     */
    bool operator()(const char* a, const char* b) const{
        // 使用标准库的strcmp函数比较两个字符串，返回a是否小于b
        return std::strcmp(a, b) < 0;
    }
};

const std::map<const char*, const char*, CStrCompare> city_codes{
    #include "wi/parser/city_code.inc"
};


/**
 * @brief 根据城市名称获取对应的城市代码
 *
 * 该函数通过城市名称在预定义的城市代码映射表中查找对应的城市代码，并将结果复制到指定的缓冲区中
 *
 * @param[out] code 用于存储查询到的城市代码的缓冲区指针
 * @param[in] name 要查询的城市名称
 * @return esp_err_t 成功返回ESP_OK，失败返回ESP_FAIL（如城市名称不存在）
 */
esp_err_t wi_get_city_code(char* code, const char* name){
    // 检查城市名称是否存在于城市代码映射表中
    if(!city_codes.count(name)){
        // 找不到键时输出警告日志并返回失败
        ESP_LOGW(LOG_TAG, "找不到键: %s", name);
        return ESP_FAIL;
    }
    // 找到城市代码后，将其复制到结果缓冲区中
    strcpy(code, city_codes.at(name));
    return ESP_OK;
}

/**
 * @brief 将十六进制字符转换为对应的整数值
 *
 * 该函数用于将单个十六进制字符（0-9、A-F、a-f）转换为对应的整数值（0-15）
 * 对于无效的十六进制字符，返回-1
 *
 * @param c 需要转换的十六进制字符
 * @return int 转换后的整数值，范围0-15；如果是无效字符，返回-1
 */
int hex_to_int(char c) {
    // 处理数字字符 0-9
    if (c >= '0' && c <= '9') 
        return c - '0';  // 减去字符'0'的ASCII值，得到对应的数字
    
    // 处理大写字母字符 A-F
    if (c >= 'A' && c <= 'F') 
        return 10 + c - 'A';  // 转换为对应的十进制值10-15
    
    // 处理小写字母字符 a-f
    if (c >= 'a' && c <= 'f') 
        return 10 + c - 'a';  // 转换为对应的十进制值10-15
    
    // 无效的十六进制字符
    return -1; // 返回-1表示无效字符
}


/**
 * @brief 对URL编码的字符串进行解码
 *
 * 该函数将URL编码格式的字符串（如包含 %XX 或 + 格式）解码为原始字符串，并将结果存储在指定的缓冲区中
 * URL编码规则：
 * - %XX 表示一个字节的十六进制值，其中XX是两位十六进制数字
 * - '+' 通常表示空格
 * - 其他字符保持不变
 *
 * @param[out] result 用于存储解码后结果的缓冲区指针
 * @param[in] encode_data 需要解码的URL编码字符串视图
 * @return esp_err_t 解码操作的结果，目前固定返回ESP_OK
 * @note 调用者需要确保result缓冲区有足够的空间存储解码后的结果
 */
esp_err_t url_decode(char* result, const std::string_view& encode_data) {
    int n = encode_data.size();   // 获取编码数据的长度
    int j = 0;                   // 结果缓冲区的索引
    
    // 遍历编码数据的每个字符
    for (int i = 0; i < n; ) {
        // 处理 %XX 格式的编码字符
        if (encode_data[i] == '%' && i + 2 < n) {
            // 解析 %XX 格式
            int hex1 = hex_to_int(encode_data[i + 1]);  // 获取第一个十六进制字符的整数值
            int hex2 = hex_to_int(encode_data[i + 2]);  // 获取第二个十六进制字符的整数值
            
            if (hex1 != -1 && hex2 != -1) {  // 确保两个字符都是有效的十六进制数字
                // 合并两个十六进制字符为一个字节
                char c = (hex1 << 4) | hex2;
                result[j++] = c;
                i += 3; // 跳过 %XX 三个字符
                continue;
            }
        } else if (encode_data[i] == '+') {
            // URL 中 '+' 通常表示空格
            result[j++] = ' ';
        } else {
            // 普通字符直接添加到结果中
            result[j++] = encode_data[i];
        }
        i++;
    }
    return ESP_OK;  // 返回成功状态
}

/**
 * @brief 解析HTTP POST请求中的城市名称参数
 *
 * 该函数从HTTP POST请求数据中提取城市名称参数，并对其进行URL解码
 * 支持从格式为"city=编码后的城市名称"的数据中提取城市信息
 *
 * @param[in] buffer POST请求数据的缓冲区指针
 * @param[in] data_length POST请求数据的长度
 * @param[out] city_name 用于存储解析并解码后的城市名称的缓冲区指针
 * @return esp_err_t 解析操作的结果，成功返回ESP_OK，失败返回ESP_FAIL
 * @note 调用者需要确保city_name缓冲区有足够的空间存储解码后的城市名称
 */
esp_err_t wi_paser_post(const char* buffer, const int data_length, char* city_name){
    // 创建字符串视图以方便操作原始数据
    std::string_view encode{buffer};

    // 初始化城市名称缓冲区为空字符串
    city_name[0] = '\0';
    
    // 查找POST数据中的"city="参数起始位置
    size_t start_pos = encode.find("city=");
    if (start_pos == std::string_view::npos) {
        // 未找到city参数，记录警告日志并返回失败
        ESP_LOGW(LOG_TAG, "POST数据格式错误, 未检测city参数");
        return ESP_FAIL;
    }
    
    // 从city=之后的位置开始提取并解码城市名称
    // 解码操作通过调用url_decode函数完成
    if(url_decode(city_name, encode.substr(start_pos + 5)) != ESP_OK){
        // 解码失败，记录错误日志并返回失败
        ESP_LOGE(LOG_TAG, "POST数据city参数解码错误");
        return ESP_FAIL;
    }

    // 解析和解码成功，返回成功状态
    return ESP_OK;
}

/**
 * @brief 在字符串中查找指定子字符串，并返回子字符串结束位置
 *
 * 该函数在给定的字符串中查找指定的子字符串，并通过输出参数返回子字符串结束位置
 * 即使未找到指定子字符串，函数也会返回ESP_OK，仅通过日志警告和end_pos为0来表示查找失败
 *
 * @param[in] str 要进行搜索的原始字符串
 * @param[in] find 要在原始字符串中查找的子字符串
 * @param[out] end_pos 用于存储找到的子字符串结束位置的指针
 * @return esp_err_t 操作结果，**始终返回ESP_OK**，即使未找到字符串
 * @note 即使未找到子字符串，函数也返回ESP_OK，请通过检查end_pos的值来判断是否找到
 */
esp_err_t wi_paser_get_str_find(const char* str, const char* find, size_t* end_pos){
    // 创建字符串视图以方便字符串操作
    std::string_view str_view{str};
    
    // 在字符串视图中查找指定的子字符串
    size_t pos = str_view.find(find);

    if(pos == std::string_view::npos){
        // 未找到子字符串，设置end_pos为0并记录警告日志
        *end_pos = 0;
        ESP_LOGW(LOG_TAG, "未检测到字符串: %s", find);
        // return ESP_FAIL;
    }else{
        // 找到子字符串，计算并存储子字符串结束位置
        *end_pos = pos + strlen(find);
    }
    
    // 无论是否找到子字符串，都返回ESP_OK
    return ESP_OK;
}

esp_err_t wi_url_decode(char* result, const char* encode_data){
    std::string_view encode_view{encode_data};
    return url_decode(result, encode_view);
}