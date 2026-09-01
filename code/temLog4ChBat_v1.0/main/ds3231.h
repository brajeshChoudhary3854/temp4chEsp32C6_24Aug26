#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

typedef struct {
    int        i2c_port;
    gpio_num_t sda_io_num;
    gpio_num_t scl_io_num;
    uint32_t   i2c_freq_hz;
    gpio_num_t sqw_io_num;
    gpio_num_t clk32k_io_num;
} ds3231_config_t;

typedef struct {
    uint8_t  second;
    uint8_t  minute;
    uint8_t  hour;
    uint8_t  day_of_week;
    uint8_t  day;
    uint8_t  month;
    uint16_t year;
} ds3231_datetime_t;

typedef enum { DS3231_ALARM_1 = 1, DS3231_ALARM_2 = 2 } ds3231_alarm_t;

esp_err_t ds3231_init(const ds3231_config_t *cfg);
bool      ds3231_is_running(void);
esp_err_t ds3231_get_time(ds3231_datetime_t *dt);
esp_err_t ds3231_set_time(const ds3231_datetime_t *dt);
esp_err_t ds3231_get_temperature(float *temp_c);
esp_err_t ds3231_set_alarm1(uint8_t hour, uint8_t min, uint8_t sec);
esp_err_t ds3231_set_alarm2(uint8_t hour, uint8_t min);
esp_err_t ds3231_enable_alarm_interrupt(ds3231_alarm_t alarm);
esp_err_t ds3231_clear_alarm_flags(void);
