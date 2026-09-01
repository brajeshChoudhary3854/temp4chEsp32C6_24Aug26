#include "ds18b20.h"
#include <math.h>

#define CMD_READ_ROM      0x33
#define CMD_SKIP_ROM      0xCC
#define CMD_CONVERT_T     0x44
#define CMD_READ_SCRATCH  0xBE
#define CMD_WRITE_SCRATCH 0x4E

static uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (int j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ byte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            byte >>= 1;
        }
    }
    return crc;
}

esp_err_t ds18b20_init(ds18b20_device_t *dev, gpio_num_t pin, ds18b20_resolution_t res)
{
    dev->resolution = res;
    esp_err_t ret = onewire_init(&dev->bus, pin);
    if (ret != ESP_OK) return ret;
    if (!onewire_reset(&dev->bus)) return ESP_ERR_NOT_FOUND;

    onewire_write_byte(&dev->bus, CMD_SKIP_ROM);
    onewire_write_byte(&dev->bus, CMD_WRITE_SCRATCH);
    onewire_write_byte(&dev->bus, 0x00);
    onewire_write_byte(&dev->bus, 0x00);
    onewire_write_byte(&dev->bus, res);
    return ESP_OK;
}

esp_err_t ds18b20_read_rom(ds18b20_device_t *dev, uint8_t rom[8])
{
    if (!onewire_reset(&dev->bus)) return ESP_ERR_NOT_FOUND;
    onewire_write_byte(&dev->bus, CMD_READ_ROM);
    for (int i = 0; i < 8; i++) rom[i] = onewire_read_byte(&dev->bus);
    if (crc8(rom, 7) != rom[7]) return ESP_ERR_INVALID_CRC;
    if (rom[0] != 0x28) return ESP_ERR_NOT_FOUND;
    return ESP_OK;
}

esp_err_t ds18b20_trigger_conversion(ds18b20_device_t *dev)
{
    if (!onewire_reset(&dev->bus)) return ESP_ERR_NOT_FOUND;
    onewire_write_byte(&dev->bus, CMD_SKIP_ROM);
    onewire_write_byte(&dev->bus, CMD_CONVERT_T);
    return ESP_OK;
}

esp_err_t ds18b20_read_temperature(ds18b20_device_t *dev, float *temp_c)
{
    *temp_c = NAN;
    if (!onewire_reset(&dev->bus)) return ESP_ERR_NOT_FOUND;
    onewire_write_byte(&dev->bus, CMD_SKIP_ROM);
    onewire_write_byte(&dev->bus, CMD_READ_SCRATCH);

    uint8_t sp[9];
    for (int i = 0; i < 9; i++) sp[i] = onewire_read_byte(&dev->bus);
    if (crc8(sp, 8) != sp[8]) return ESP_ERR_INVALID_CRC;

    int16_t raw = (int16_t)((sp[1] << 8) | sp[0]);
    *temp_c = raw * 0.0625f;
    return ESP_OK;
}
