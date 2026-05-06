/* Telic SCP_Welcome Application

   This example code is CC0 licensed

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_flash.h"
#include "esp_chip_info.h"
#include "driver/i2c_master.h"
#include "EepromInfo.h"


/* I2C bus constants */
#define I2C_MASTER_SDA_IO               21
#define I2C_MASTER_SCL_IO               22
#define I2C_MASTER_FREQ_HZ              100000
#define I2C_MASTER_TIMEOUT_MS           10000

static i2c_master_bus_handle_t i2c_master_bus_handle;
static i2c_master_dev_handle_t ioexpander_handle;
static i2c_master_dev_handle_t eeprom_handle;

/* IOEXPANDER PCAL6416 constants */
#define PCAL6416_IOEXPANDER_I2C_ADDR    0x20
#define PCAL6416_REG_OUTPUT_PORT_0      0x02
#define PCAL6416_REG_OUTPUT_PORT_1      0x03
#define PCAL6416_REG_CONFIG_PORT_0      0x06
#define PCAL6416_REG_CONFIG_PORT_1      0x07

/* LED connectors */
#define LED_1_MASK                      0x01
#define LED_2_MASK                      0x02

/*
 * Initialising of the I2C bus the IOExpander is connected to
 */
static void initI2C(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_master_bus_handle));

    i2c_device_config_t ioexpander_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PCAL6416_IOEXPANDER_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_master_bus_handle, &ioexpander_config, &ioexpander_handle));

    i2c_device_config_t eeprom_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = EEPROM_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_master_bus_handle, &eeprom_config, &eeprom_handle));
}


/*
 * Initialising of the GPIOs of the IOExpander the LEDs are connected to
 */
static void initLedGpios(void)
{
    uint8_t buffer[2];

    // set LED GPIOs to output
    // after power up, all ports of the GPIO expander are set to input
    // so its safe to set non LED GPIOs as input
    buffer[0] = PCAL6416_REG_CONFIG_PORT_0;
    buffer[1] = 0xff ^ (LED_1_MASK | LED_2_MASK);
    ESP_ERROR_CHECK(i2c_master_transmit(ioexpander_handle, buffer, sizeof(buffer), I2C_MASTER_TIMEOUT_MS));
}


/*
 * setting/clearing the LED GPIOs
 */
static void setLED(const uint8_t ledPort, const bool shine)
{
    uint8_t buffer[2];
    uint8_t currentValue;
    const uint8_t currentRegister = PCAL6416_REG_OUTPUT_PORT_0;

    // read current output setting
    ESP_ERROR_CHECK(i2c_master_transmit_receive(ioexpander_handle,
                                                &currentRegister,
                                                sizeof(currentRegister),
                                                &currentValue,
                                                sizeof(currentValue),
                                                I2C_MASTER_TIMEOUT_MS));

    // set LED GPIOs
    buffer[0] = PCAL6416_REG_OUTPUT_PORT_0;
    if (shine)
    {
        buffer[1] = currentValue | ledPort;
    }
    else
    {
        buffer[1] = currentValue & ~ledPort;
    }
    ESP_ERROR_CHECK(i2c_master_transmit(ioexpander_handle, buffer, sizeof(buffer), I2C_MASTER_TIMEOUT_MS));
}


void app_main(void)
{
    printf("\nWelcome to Telic Sensor2Cloud Platform V1.0.8\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    uint32_t flash_size;
    ESP_ERROR_CHECK(esp_flash_get_size(NULL, &flash_size));
    printf("%" PRIu32 "MB %s flash\n", flash_size / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n\n", esp_get_minimum_free_heap_size());

    printf("Init I2C\n");
    initI2C();
    printf("Init IOExpander\n");
    initLedGpios();

    // read and decode EEProm
    uint8_t buffer[EEPROM_MAX_READ_INDEX];
    const uint8_t currentRegister = 0;
    memset(buffer, 0x55, sizeof(buffer));

    // read values
    ESP_ERROR_CHECK(i2c_master_transmit_receive(eeprom_handle,
                                                &currentRegister,
                                                sizeof(currentRegister),
                                                buffer,
                                                sizeof(buffer),
                                                I2C_MASTER_TIMEOUT_MS));
    EEPROMInfo info;
    getEepromInfo(buffer, sizeof(buffer), &info);
    printEepromInfo(info);


    for (int i = 15; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        setLED(LED_1_MASK, i % 2);
        setLED(LED_2_MASK, !(i % 2));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    setLED(LED_1_MASK | LED_2_MASK, false);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    printf("Restarting now.\n");

    fflush(stdout);
    esp_restart();
}
