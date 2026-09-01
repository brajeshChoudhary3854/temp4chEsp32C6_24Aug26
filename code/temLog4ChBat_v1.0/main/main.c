/*
 * temLog4ChBat v1.0 — Comet Instrument
 * 4-channel DS18B20 temperature logger with DS3231 RTC timestamp.
 *
 * SW Blocks used:
 *   - ESP32_C6/DS18B20_multisensor/v1.0  (onewire + ds18b20 + sensor_manager)
 *   - ESP32_C6/DS3231_RTC/v1.0           (ds3231)
 *
 * Pin map (see config.h):
 *   CH1 DS18B20  → GPIO 4
 *   CH2 DS18B20  → GPIO 5
 *   CH3 DS18B20  → GPIO 10
 *   CH4 DS18B20  → GPIO 11
 *   DS3231 SDA   → GPIO 15
 *   DS3231 SCL   → GPIO 3
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "config.h"
#include "sensor_manager.h"
#include "ds3231.h"

#define TAG "temLog4ChBat"

static void print_header(void)
{
    printf("\r\n");
    printf("================================================\r\n");
    printf("  temLog4ChBat v1.0 — Comet Instrument\r\n");
    printf("  4-Channel Temperature Logger\r\n");
    printf("================================================\r\n");
    printf("  CH1 DS18B20 : GPIO %d\r\n", PIN_DS18B20_CH1);
    printf("  CH2 DS18B20 : GPIO %d\r\n", PIN_DS18B20_CH2);
    printf("  CH3 DS18B20 : GPIO %d\r\n", PIN_DS18B20_CH3);
    printf("  CH4 DS18B20 : GPIO %d\r\n", PIN_DS18B20_CH4);
    printf("  DS3231 SDA  : GPIO %d\r\n", PIN_DS3231_SDA);
    printf("  DS3231 SCL  : GPIO %d\r\n", PIN_DS3231_SCL);
    printf("  Log interval: %d s\r\n", LOG_INTERVAL_S);
    printf("================================================\r\n\r\n");
}

void app_main(void)
{
    print_header();

    // ── Init DS3231 RTC ──────────────────────────────────────────────────────
    ds3231_config_t rtc_cfg = {
        .i2c_port      = -1,
        .sda_io_num    = PIN_DS3231_SDA,
        .scl_io_num    = PIN_DS3231_SCL,
        .i2c_freq_hz   = 100000,
        .sqw_io_num    = GPIO_NUM_NC,
        .clk32k_io_num = GPIO_NUM_NC,
    };

    esp_err_t ret = ds3231_init(&rtc_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DS3231 init failed: %s — continuing without RTC", esp_err_to_name(ret));
    } else {
        if (!ds3231_is_running()) {
            ESP_LOGW(TAG, "RTC oscillator stopped — setting default time 2026-09-01 00:00:00");
            ds3231_datetime_t t = {
                .second = 0, .minute = 0, .hour = 0,
                .day_of_week = 2,   // Monday
                .day = 1, .month = 9, .year = 2026,
            };
            ds3231_set_time(&t);
        }
        ESP_LOGI(TAG, "DS3231 ready");
    }

    // ── Init DS18B20 channels ────────────────────────────────────────────────
    sensor_manager_add(0, PIN_DS18B20_CH1);
    sensor_manager_add(1, PIN_DS18B20_CH2);
    sensor_manager_add(2, PIN_DS18B20_CH3);
    sensor_manager_add(3, PIN_DS18B20_CH4);
    sensor_manager_init_all();

    // ── Main logging loop ────────────────────────────────────────────────────
    uint32_t log_index = 0;

    while (1) {
        sensor_reading_t readings[DS18B20_MAX_SENSORS];
        sensor_manager_read_all(readings);

        ds3231_datetime_t dt = {0};
        bool rtc_ok = (ds3231_get_time(&dt) == ESP_OK);

        if (rtc_ok) {
            printf("[%05lu] %04d-%02d-%02d %02d:%02d:%02d | ",
                   (unsigned long)log_index,
                   dt.year, dt.month, dt.day,
                   dt.hour, dt.minute, dt.second);
        } else {
            printf("[%05lu] --no RTC--              | ", (unsigned long)log_index);
        }

        for (int ch = 0; ch < DS18B20_MAX_SENSORS; ch++) {
            if (readings[ch].valid) {
                printf("CH%d: %6.2f°C  ", ch + 1, readings[ch].temp_c);
            } else {
                printf("CH%d: --N/A--   ", ch + 1);
            }
        }
        printf("\r\n");
        log_index++;

        // Wait for next log interval (minus the ~750ms conversion time already spent)
        int wait_ms = (LOG_INTERVAL_S * 1000) - DS18B20_CONVERSION_MS;
        if (wait_ms > 0) vTaskDelay(pdMS_TO_TICKS(wait_ms));
    }
}
