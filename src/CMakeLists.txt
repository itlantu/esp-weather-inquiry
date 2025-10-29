set(sources
    "${CMAKE_CURRENT_SOURCE_DIR}/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/wifi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/nvs.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/web.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config.c"
)

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

# 将指定的环境变量的值作为宏
if(DEFINED ENV{WI_CONFIG_WIFI_SSID})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE WI_CONFIG_WIFI_SSID=\"$ENV{WI_CONFIG_WIFI_SSID}\")
    message(STATUS " [*]WI_CONFIG_WIFI_SSID=$ENV{WI_CONFIG_WIFI_SSID}")
else()
    message(WARNING " WI_CONFIG_WIFI_SSID未配置")
endif()

if(DEFINED ENV{WI_CONFIG_WIFI_PASSWORD})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE WI_CONFIG_WIFI_PASSWORD=\""$ENV{WI_CONFIG_WIFI_PASSWORD}"\")
    message(STATUS " [*]WI_CONFIG_WIFI_PASSWORD=$ENV{WI_CONFIG_WIFI_PASSWORD}")
else()
    message(WARNING " WI_CONFIG_WIFI_PASSWORD未配置")
endif()
