#include "ds3231.h"
#include <string.h>
#include "esp_log.h"

#define TAG "DS3231"

#define DS3231_I2C_ADDR     0x68
#define DS3231_DEFAULT_FREQ 400000
#define DS3231_TIMEOUT_MS   1000

#define REG_SECONDS  0x00
#define REG_MINUTES  0x01
#define REG_HOURS    0x02
#define REG_DOW      0x03
#define REG_DATE     0x04
#define REG_MONTH    0x05
#define REG_YEAR     0x06
#define REG_ALM1_SEC 0x07
#define REG_ALM1_MIN 0x08
#define REG_ALM1_HR  0x09
#define REG_ALM1_DAY 0x0A
#define REG_ALM2_MIN 0x0B
#define REG_ALM2_HR  0x0C
#define REG_ALM2_DAY 0x0D
#define REG_CONTROL  0x0E
#define REG_STATUS   0x0F
#define REG_TEMP_MSB 0x11
#define REG_TEMP_LSB 0x12

#define CTRL_A1IE  0x01
#define CTRL_A2IE  0x02
#define CTRL_INTCN 0x04

static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;

static uint8_t dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10); }
static uint8_t bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }

static esp_err_t reg_read(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, len, DS3231_TIMEOUT_MS);
}

static esp_err_t reg_write(uint8_t reg, const uint8_t *data, size_t len)
{
    uint8_t buf[9];
    buf[0] = reg;
    memcpy(buf + 1, data, len);
    return i2c_master_transmit(s_dev, buf, len + 1, DS3231_TIMEOUT_MS);
}

esp_err_t ds3231_init(const ds3231_config_t *cfg)
{
    esp_err_t ret;
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = cfg->i2c_port,
        .sda_io_num        = cfg->sda_io_num,
        .scl_io_num        = cfg->scl_io_num,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    ret = i2c_new_master_bus(&bus_cfg, &s_bus);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Bus create failed: %s", esp_err_to_name(ret)); return ret; }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = DS3231_I2C_ADDR,
        .scl_speed_hz    = cfg->i2c_freq_hz ? cfg->i2c_freq_hz : DS3231_DEFAULT_FREQ,
    };
    ret = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Device add failed: %s", esp_err_to_name(ret)); return ret; }

    uint8_t stat;
    ret = reg_read(REG_STATUS, &stat, 1);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Not found (SDA=%d SCL=%d)", cfg->sda_io_num, cfg->scl_io_num); return ret; }
    stat &= ~0x80;
    reg_write(REG_STATUS, &stat, 1);

    // Force 24-hour mode
    uint8_t hr_reg;
    ret = reg_read(REG_HOURS, &hr_reg, 1);
    if (ret != ESP_OK) return ret;
    if (hr_reg & 0x40) {
        uint8_t hr = bcd2dec(hr_reg & 0x1F);
        if (hr_reg & 0x20) hr += 12;
        if (hr == 24) hr = 0;
        hr_reg = dec2bcd(hr);
        reg_write(REG_HOURS, &hr_reg, 1);
    }

    ESP_LOGI(TAG, "Init OK — SDA=%d SCL=%d", cfg->sda_io_num, cfg->scl_io_num);
    return ESP_OK;
}

bool ds3231_is_running(void)
{
    uint8_t stat;
    if (reg_read(REG_STATUS, &stat, 1) != ESP_OK) return false;
    return !(stat & 0x80);
}

esp_err_t ds3231_get_time(ds3231_datetime_t *dt)
{
    uint8_t buf[7];
    esp_err_t ret = reg_read(REG_SECONDS, buf, 7);
    if (ret != ESP_OK) return ret;
    dt->second      = bcd2dec(buf[0] & 0x7F);
    dt->minute      = bcd2dec(buf[1] & 0x7F);
    dt->hour        = bcd2dec(buf[2] & 0x3F);
    dt->day_of_week = buf[3] & 0x07;
    dt->day         = bcd2dec(buf[4] & 0x3F);
    dt->month       = bcd2dec(buf[5] & 0x1F);
    dt->year        = 2000 + bcd2dec(buf[6]) + ((buf[5] >> 7) ? 100 : 0);
    return ESP_OK;
}

esp_err_t ds3231_set_time(const ds3231_datetime_t *dt)
{
    bool century = dt->year >= 2100;
    uint8_t buf[7] = {
        dec2bcd(dt->second), dec2bcd(dt->minute), dec2bcd(dt->hour),
        dt->day_of_week & 0x07, dec2bcd(dt->day),
        dec2bcd(dt->month) | (century ? 0x80 : 0x00),
        dec2bcd((uint8_t)(dt->year % 100)),
    };
    return reg_write(REG_SECONDS, buf, 7);
}

esp_err_t ds3231_get_temperature(float *temp_c)
{
    uint8_t buf[2];
    esp_err_t ret = reg_read(REG_TEMP_MSB, buf, 2);
    if (ret != ESP_OK) return ret;
    *temp_c = (int8_t)buf[0] + (buf[1] >> 6) * 0.25f;
    return ESP_OK;
}

esp_err_t ds3231_set_alarm1(uint8_t hour, uint8_t min, uint8_t sec)
{
    uint8_t buf[4] = { dec2bcd(sec), dec2bcd(min), dec2bcd(hour), 0x80 };
    return reg_write(REG_ALM1_SEC, buf, 4);
}

esp_err_t ds3231_set_alarm2(uint8_t hour, uint8_t min)
{
    uint8_t buf[3] = { dec2bcd(min), dec2bcd(hour), 0x80 };
    return reg_write(REG_ALM2_MIN, buf, 3);
}

esp_err_t ds3231_enable_alarm_interrupt(ds3231_alarm_t alarm)
{
    uint8_t ctrl;
    esp_err_t ret = reg_read(REG_CONTROL, &ctrl, 1);
    if (ret != ESP_OK) return ret;
    ctrl |= CTRL_INTCN;
    if (alarm == DS3231_ALARM_1) ctrl |= CTRL_A1IE;
    if (alarm == DS3231_ALARM_2) ctrl |= CTRL_A2IE;
    return reg_write(REG_CONTROL, &ctrl, 1);
}

esp_err_t ds3231_clear_alarm_flags(void)
{
    uint8_t stat;
    esp_err_t ret = reg_read(REG_STATUS, &stat, 1);
    if (ret != ESP_OK) return ret;
    stat &= ~0x03;
    return reg_write(REG_STATUS, &stat, 1);
}
