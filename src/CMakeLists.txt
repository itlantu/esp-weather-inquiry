file(GLOB_RECURSE sources "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
get_filename_component(include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/../include" ABSOLUTE)

message(STATUS " sources: ${sources}")
message(STATUS " include: ${include_dirs}")

idf_component_register(
    SRCS ${sources}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES
        esp_wifi
        esp_event
        freertos
        nvs_flash
        esp_http_client
        esp_http_server
    EMBED_FILES
        "web/index.html"
)

# 从.env文件中加载环境变量
get_filename_component(env_file_path "${CMAKE_CURRENT_SOURCE_DIR}/../.env" ABSOLUTE)
function_env_read(${env_file_path})

fucntion_target_env2marco(${COMPONENT_LIB} WI_CONFIG_WIFI_SSID)
fucntion_target_env2marco(${COMPONENT_LIB} WI_CONFIG_WIFI_PASSWORD)
fucntion_target_env2marco(${COMPONENT_LIB} WI_CONFIG_UART_BAUD_RATE)
