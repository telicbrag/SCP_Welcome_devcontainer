/* Telic SCP_Welcome Application

   This example code is CC0 licensed

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_flash.h"
#include "esp_chip_info.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "memory.h"
#include "EepromInfo.h"

/* I2C bus constants */
#define I2C_MASTER_SDA_IO               21
#define I2C_MASTER_SCL_IO               22
#define I2C_MASTER_FREQ_HZ              100000
#define I2C_MASTER_TX_BUF_DISABLE       0
#define I2C_MASTER_RX_BUF_DISABLE       0
#define I2C_MASTER_TIMEOUT_MS           10000

i2c_port_t i2c_master_port              = I2C_NUM_0;

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
    i2c_config_t i2c_conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_MASTER_SDA_IO,         
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_io_num       = I2C_MASTER_SCL_IO,         
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,  
        .clk_flags        = 0,         
    };

    ESP_ERROR_CHECK( i2c_param_config(i2c_master_port, &i2c_conf) );
    ESP_ERROR_CHECK( i2c_driver_install(i2c_master_port, i2c_conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0) );
}


/*
 * Initialising of the GPIOs of the IOExpander the LEDs are connected to
 */
static void initLedGpios(void)
{
    uint8_t buffer[4];

    // set LED GPIOs to output
    // after power up, all ports of the GPIO expander are set to input 
    // so its save to set not LED GPIOs  as input 
    buffer[0] = PCAL6416_REG_CONFIG_PORT_0;
    buffer[1] = 0xff ^ ( LED_1_MASK | LED_2_MASK);
    ESP_ERROR_CHECK( i2c_master_write_to_device(i2c_master_port, PCAL6416_IOEXPANDER_I2C_ADDR, buffer, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
}


/* 
 * setting/clearing the LED GPIOs
 */
static void setLED( const uint8_t ledPort, const bool shine)
{
    uint8_t buffer[4];
    uint8_t currentValue;
    const uint8_t curentRegister = PCAL6416_REG_OUTPUT_PORT_0;

    // read current output setting
    ESP_ERROR_CHECK( i2c_master_write_read_device(i2c_master_port, PCAL6416_IOEXPANDER_I2C_ADDR, &curentRegister, sizeof(curentRegister), &currentValue, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
 
    // set LED GPIOs
    buffer[0] = PCAL6416_REG_OUTPUT_PORT_0;
    if( shine)
    {
        buffer[1] = currentValue | ledPort;
    }
    else
    {
        buffer[1] = currentValue ^ ledPort;
    }
    ESP_ERROR_CHECK( i2c_master_write_to_device(i2c_master_port, PCAL6416_IOEXPANDER_I2C_ADDR, buffer, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
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

    esp_flash_init(NULL);
    uint32_t flashSize;
    esp_flash_get_size(NULL, &flashSize);
    printf("%ldMB %ld %s flash\n", flashSize / (1024 * 1024),flashSize,
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %ld bytes\n\n", esp_get_minimum_free_heap_size());

    printf("Init I2C\n");
    initI2C();
    printf("Init IOExpander\n");
    initLedGpios();

    // read and decode EEProm
    uint8_t buffer[EEPROM_MAX_READ_INDEX];
    const uint8_t currentRegister = 0;
    memset( buffer, 0x55, sizeof(buffer));

    // read values
    ESP_ERROR_CHECK( i2c_master_write_read_device(i2c_master_port, EEPROM_I2C_ADDR, &currentRegister, sizeof(currentRegister), buffer, sizeof(buffer), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    EEPROMInfo info;
    getEepromInfo( buffer, sizeof(buffer), &info);
    printEepromInfo( info);


    for (int i = 15; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        setLED( LED_1_MASK, i%2);
        setLED( LED_2_MASK, !(i%2));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    setLED( LED_1_MASK|LED_2_MASK, false);
    vTaskDelay(500 / portTICK_PERIOD_MS);    
    printf("Restarting now.\n");
    
    fflush(stdout);
    esp_restart();
}
