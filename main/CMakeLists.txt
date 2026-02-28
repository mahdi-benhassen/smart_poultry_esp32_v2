idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS ""
    REQUIRES nvs_flash esp_event esp_wifi log esp_system freertos sensors actuators control monitoring communication utils mesh
)
