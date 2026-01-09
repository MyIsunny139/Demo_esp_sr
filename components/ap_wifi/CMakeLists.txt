idf_component_register(
    SRCS "ap_wifi.c" "ws_server.c" "wifi_manager.c"
    INCLUDE_DIRS    "include"
    REQUIRES esp_wifi esp_http_server lwip json spiffs nvs_flash driver
)