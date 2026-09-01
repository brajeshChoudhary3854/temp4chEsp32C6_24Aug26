#include "sensor_manager.h"
#include "ds18b20.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>

#define TAG "sensor_mgr"

typedef struct {
    ds18b20_device_t device;
    gpio_num_t       gpio;
    bool             registered;
    bool             initialized;
    TickType_t       last_attempt_ticks;
} channel_t;

static channel_t channels[DS18B20_MAX_SENSORS];
static uint8_t   registered_count = 0;

static void try_init_channel(int i)
{
    channels[i].last_attempt_ticks = xTaskGetTickCount();
    esp_err_t ret = ds18b20_init(&channels[i].device, channels[i].gpio, DS18B20_RES_12BIT);
    if (ret != ESP_OK) return;

    uint8_t rom[8];
    ret = ds18b20_read_rom(&channels[i].device, rom);
    if (ret == ESP_OK) {
        channels[i].initialized = true;
        ESP_LOGI(TAG, "CH%d GPIO%02d | ROM: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                 i, channels[i].gpio,
                 rom[0], rom[1], rom[2], rom[3], rom[4], rom[5], rom[6], rom[7]);
    }
}

esp_err_t sensor_manager_add(uint8_t channel, gpio_num_t gpio)
{
    if (channel >= DS18B20_MAX_SENSORS) return ESP_ERR_INVALID_ARG;
    channels[channel].gpio               = gpio;
    channels[channel].registered         = true;
    channels[channel].initialized        = false;
    channels[channel].last_attempt_ticks = 0;
    registered_count++;
    ESP_LOGI(TAG, "CH%d registered on GPIO%d", channel, gpio);
    return ESP_OK;
}

esp_err_t sensor_manager_init_all(void)
{
    for (int i = 0; i < DS18B20_MAX_SENSORS; i++) {
        if (!channels[i].registered) continue;
        try_init_channel(i);
        if (!channels[i].initialized)
            ESP_LOGW(TAG, "CH%d GPIO%d: no sensor at boot — retry every %ds",
                     i, channels[i].gpio, DS18B20_RETRY_MS / 1000);
    }
    return ESP_OK;
}

esp_err_t sensor_manager_read_all(sensor_reading_t *readings)
{
    TickType_t now = xTaskGetTickCount();
    for (int i = 0; i < DS18B20_MAX_SENSORS; i++) {
        if (!channels[i].registered || channels[i].initialized) continue;
        if ((now - channels[i].last_attempt_ticks) >= pdMS_TO_TICKS(DS18B20_RETRY_MS))
            try_init_channel(i);
    }

    for (int i = 0; i < DS18B20_MAX_SENSORS; i++) {
        readings[i].valid  = false;
        readings[i].temp_c = NAN;
        if (channels[i].initialized) ds18b20_trigger_conversion(&channels[i].device);
    }

    vTaskDelay(pdMS_TO_TICKS(DS18B20_CONVERSION_MS));

    for (int i = 0; i < DS18B20_MAX_SENSORS; i++) {
        if (!channels[i].initialized) continue;
        esp_err_t ret = ds18b20_read_temperature(&channels[i].device, &readings[i].temp_c);
        readings[i].valid = (ret == ESP_OK);
        if (!readings[i].valid)
            ESP_LOGW(TAG, "CH%d read failed: %s", i, esp_err_to_name(ret));
    }
    return ESP_OK;
}

uint8_t sensor_manager_get_count(void) { return registered_count; }
