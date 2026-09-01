#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    gpio_num_t pin;
} onewire_bus_t;

esp_err_t onewire_init(onewire_bus_t *bus, gpio_num_t pin);
bool      onewire_reset(onewire_bus_t *bus);
void      onewire_write_byte(onewire_bus_t *bus, uint8_t byte);
uint8_t   onewire_read_byte(onewire_bus_t *bus);
