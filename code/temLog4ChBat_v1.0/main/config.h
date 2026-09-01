#pragma once

// ── DS18B20 sensor config ────────────────────────────────────────────────────
#define DS18B20_MAX_SENSORS     4
#define DS18B20_CONVERSION_MS   750     // 12-bit conversion time
#define DS18B20_RETRY_MS        15000   // retry missing sensors every 15 s

// ── GPIO pin map ─────────────────────────────────────────────────────────────
#define PIN_DS18B20_CH1    GPIO_NUM_4
#define PIN_DS18B20_CH2    GPIO_NUM_5
#define PIN_DS18B20_CH3    GPIO_NUM_10
#define PIN_DS18B20_CH4    GPIO_NUM_11

#define PIN_DS3231_SDA     GPIO_NUM_15
#define PIN_DS3231_SCL     GPIO_NUM_3

// ── Logging interval ─────────────────────────────────────────────────────────
#define LOG_INTERVAL_S     60           // read + print every 60 seconds
