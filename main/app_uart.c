/* ESP32 UART
   @author： Azolla
   @time  :  2021.06.05
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h" 
#include "nvs.h"
#include "sdkconfig.h"
#include "esp_spiffs.h"

#include "app_uart.h"
#include "app_user.h"
#include "app_mqtt.h"
#include "app_sntp.h"


static const char *TAG = "APP_UART";


/**
 * This is an example which echos any data it receives on configured UART back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define UART_PIN_TXD            (22)
#define UART_PIN_RXD            (23)
#define UART_PIN_RTS            (UART_PIN_NO_CHANGE)
#define UART_PIN_CTS            (UART_PIN_NO_CHANGE)

#define UART_PORT_NUM           (1)
#define UART_BAUD_RATE          (115200)
#define UART_TASK_STACK_SIZE    (2048)

#define UART_BUF_SIZE           (128)

void uart_write_data(const uint8_t *data, int len)
{
    // Write data back to the UART
    uart_write_bytes(UART_PORT_NUM, (const char *) data, len);
}


static void uart_rev_task(void *arg)
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(UART_BUF_SIZE);

    mqtt_data_t mqtt_data = { 0, { 0 }, 0 };

    while (true) {
        // Read data from the UART
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE, 20 / portTICK_RATE_MS);
        
        if (len) {
            // Write data back to the UART
            uart_write_bytes(UART_PORT_NUM, (const char *) data, len);

            //esp_log_buffer_char(TAG, (char *)(data), len);

            // 需要上报MQTT的数据
            mqtt_data.timestamp = sntp_get_timestamp();
            mqtt_data.len = len;
            memcpy(mqtt_data.value, data, mqtt_data.len);

            if (mqtt_publish_queue != NULL) {
                xQueueSend(mqtt_publish_queue, &mqtt_data, 20 / portTICK_RATE_MS);  
            }  
        }
    } 
}


void app_uart_init(void)
{
     /* Configure parameters of an UART driver,
    * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_PIN_TXD, UART_PIN_RXD, UART_PIN_RTS, UART_PIN_CTS));

    ESP_LOGI(TAG, "uart init ok...");

    xTaskCreate(uart_rev_task, "uart_rev_task", UART_TASK_STACK_SIZE, NULL, 10, NULL);
}

