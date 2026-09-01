#pragma once

#include "onewire.h"
#include "esp_err.h"

typedef enum {
    DS18B20_RES_9BIT  = 0x1F,
    DS18B20_RES_10BIT = 0x3F,
    DS18B20_RES_11BIT = 0x5F,
    DS18B20_RES_12BIT = 0x7F,
} ds18b20_resolution_t;

typedef struct {
    onewire_bus_t        bus;
    ds18b20_resolution_t resolution;
} ds18b20_device_t;

esp_err_t ds18b20_init(ds18b20_device_t *dev, gpio_num_t pin, ds18b20_resolution_t res);
esp_err_t ds18b20_read_rom(ds18b20_device_t *dev, uint8_t rom[8]);
esp_err_t ds18b20_trigger_conversion(ds18b20_device_t *dev);
esp_err_t ds18b20_read_temperature(ds18b20_device_t *dev, float *temp_c);
