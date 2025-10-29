set(sources
    "${CMAKE_CURRENT_SOURCE_DIR}/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/wifi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/nvs.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/web.c"
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