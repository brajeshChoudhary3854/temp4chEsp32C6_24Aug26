idf_component_register(
    SRCS
        "main.c"
        "onewire.c"
        "ds18b20.c"
        "sensor_manager.c"
        "ds3231.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_rom freertos esp_log
)
