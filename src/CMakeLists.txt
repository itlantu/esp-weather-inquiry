set(sources
        "main.c"
        "wifi.c"
)

idf_component_register(
    SRCS ${sources}
    INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    REQUIRES
        esp_wifi
        esp_event
        freertos
        nvs_flash
        esp_http_client
        esp_http_server
)