#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float temp_c;   // NAN if read failed
    bool  valid;
} sensor_reading_t;

esp_err_t sensor_manager_add(uint8_t channel, gpio_num_t gpio);
esp_err_t sensor_manager_init_all(void);
esp_err_t sensor_manager_read_all(sensor_reading_t *readings);
uint8_t   sensor_manager_get_count(void);
