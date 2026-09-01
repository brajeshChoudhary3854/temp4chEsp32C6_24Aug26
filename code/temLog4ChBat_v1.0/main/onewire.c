#include "onewire.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"

esp_err_t onewire_init(onewire_bus_t *bus, gpio_num_t pin)
{
    bus->pin = pin;

    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&cfg);
    if (ret == ESP_OK) gpio_set_level(pin, 1);
    return ret;
}

bool onewire_reset(onewire_bus_t *bus)
{
    gpio_set_level(bus->pin, 0);
    esp_rom_delay_us(480);

    portDISABLE_INTERRUPTS();
    gpio_set_level(bus->pin, 1);
    esp_rom_delay_us(70);
    bool present = (gpio_get_level(bus->pin) == 0);
    portENABLE_INTERRUPTS();

    esp_rom_delay_us(410);
    return present;
}

static void write_bit(onewire_bus_t *bus, uint8_t bit)
{
    if (bit) {
        portDISABLE_INTERRUPTS();
        gpio_set_level(bus->pin, 0);
        esp_rom_delay_us(6);
        gpio_set_level(bus->pin, 1);
        portENABLE_INTERRUPTS();
        esp_rom_delay_us(64);
    } else {
        gpio_set_level(bus->pin, 0);
        esp_rom_delay_us(60);
        gpio_set_level(bus->pin, 1);
        esp_rom_delay_us(10);
    }
}

static uint8_t read_bit(onewire_bus_t *bus)
{
    portDISABLE_INTERRUPTS();
    gpio_set_level(bus->pin, 0);
    esp_rom_delay_us(2);
    gpio_set_level(bus->pin, 1);
    esp_rom_delay_us(10);
    uint8_t bit = gpio_get_level(bus->pin);
    portENABLE_INTERRUPTS();
    esp_rom_delay_us(50);
    return bit;
}

void onewire_write_byte(onewire_bus_t *bus, uint8_t byte)
{
    for (int i = 0; i < 8; i++) { write_bit(bus, byte & 0x01); byte >>= 1; }
}

uint8_t onewire_read_byte(onewire_bus_t *bus)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) byte |= (read_bit(bus) << i);
    return byte;
}
