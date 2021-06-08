/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

// #include "lwip/sockets.h"
// #include "lwip/dns.h"
// #include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "app_user.h"
#include "app_mqtt.h"
#include "app_uart.h"

static const char *TAG = "APP_MQTT";

static esp_mqtt_client_handle_t client_handle;

static const char *mqtt_topic_read = "/topic/read";
static const char *mqtt_topic_write = "/topic/write";
// static const char *mqtt_topic_read = "read";
// static const char *mqtt_topic_write = "write";
static const int mqtt_qos = 2;  

static uint8_t mqtt_event_connect = 0;   // 1: connect; 0: disconnect.
//static esp_mqtt_event_id_t  mqtt_event_id = MQTT_EVENT_ANY;

static bool mqtt_pubilsh_uart_data = 0;  // 1: 发布的是串口数据，才能通知队列，防止队列满了

static bool mqtt_reissue_ack = 0;

xQueueHandle mqtt_event_queue = NULL;
xQueueHandle mqtt_publish_queue = NULL;

int mqtt_send_data(const char *data, int len)
{
    if (len < 1 || mqtt_event_connect == 0)  return -1;
    // int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char *topic, const char *data, int len, int qos, int retain);
    int msg_id = esp_mqtt_client_publish(client_handle, mqtt_topic_write, data, len, mqtt_qos, 0);
    ESP_LOGI(TAG, "mqtt_send_data_publish, msg_id = %d", msg_id);
    mqtt_pubilsh_uart_data = 1;
    return msg_id;
}

void mqtt_publish_task(void * arg)
{
#define MQTT_EVENT_TIMEOUT   500        // MQTT响应超时时间
    int msg_id = -1;

    esp_mqtt_event_id_t  mqtt_event_id = 0;

    mqtt_data_t mqtt_data = { 0, { 0 }, 0 };

    uint8_t *mqtt_txbuff = (uint8_t *) malloc(BUFF_MAX_SIZE + 8);
    if (mqtt_txbuff == NULL) {
         ESP_LOGE(TAG, "malloc err %s", __func__);
    }
    
    while (true) {

        // vTaskDelay(50 / portTICK_PERIOD_MS);

        if (xQueueReceive(mqtt_publish_queue, &mqtt_data, portMAX_DELAY) == pdPASS) {  // 接收UART数据
            ESP_LOGI(TAG, "mqtt_data.value: %s", mqtt_data.value);
            // 这里需要把MQTT数据格式重新排列： TIME（8/Byte） + DATA(len/Byte)
            char *timestamp_string = NULL;
            asprintf(&timestamp_string, "%010ld", mqtt_data.timestamp);  // 时间戳有10位数字
            memcpy(mqtt_txbuff, timestamp_string, 10);
            memcpy(mqtt_txbuff + 8, mqtt_data.value, mqtt_data.len);
            free(timestamp_string);
            msg_id = mqtt_send_data((const char *)mqtt_txbuff, mqtt_data.len + 10);  // MQTT推送数据
            if (msg_id) {  // 有联网，判断事件消息，看有没有发布成功
                // wait mqtt event mqtt publish over.
                if (xQueueReceive(mqtt_event_queue, &mqtt_event_id, MQTT_EVENT_TIMEOUT / portTICK_PERIOD_MS) == pdPASS) {
                    if (mqtt_event_id != MQTT_EVENT_PUBLISHED) {  // mqtt publish event err.
                        msg_id = -2;  // err...
                    }  
                } else {  // mqtt publish event timeout/err.
                    msg_id = -3;  // err...
                }
            } 

            if (msg_id >= 0) {  // ok
                ESP_LOGI(TAG, "mqtt publish ok...");
                mqtt_reissue_ack = 0;
            } else {  // err.
                ESP_LOGI(TAG, "mqtt publish err[%d]->%d", msg_id, mqtt_event_id);
                if (mqtt_reissue_ack == 0) {  // 标识是断网补发的数据，不需要存的
                    mqtt_spiffs_write(mqtt_data);
                } 
            }
        }
    }

    vTaskDelete(NULL);
}

void spiffs_read_task(void *arg)
{
    mqtt_data_t mqtt_data = { 0, { 0 }, 0 };

    while (true) {

        vTaskDelay(800 / portTICK_PERIOD_MS);
        if (mqtt_event_connect) {  // MQTT connect.

            if (mqtt_reissue_ack == 0 ) {  // 还没应答，就不准
                if (mqtt_spiffs_read(&mqtt_data) > 0) {  // 还有数据要发
                    mqtt_reissue_ack = 1;  // 标识是断网补发的数据,成功后需要清零
                }
            }  

            if (mqtt_reissue_ack) {  // 没有应答就一直发，除非断线
                if (mqtt_publish_queue != NULL) {
                    ESP_LOGI(TAG, "spiffs_read_task...");
                    xQueueSend(mqtt_publish_queue, &mqtt_data, 10 / portTICK_RATE_MS);    
                }
            }
        }
    }
    vTaskDelete(NULL);
}

void mqtt_publish_task_init(void)
{
    mqtt_publish_queue = xQueueCreate(10, sizeof(mqtt_data_t));
    if (mqtt_publish_queue == NULL) {
        ESP_LOGI(TAG, "mqtt_publish_queue ERR.");
        return;
    }
    mqtt_event_queue = xQueueCreate(10, sizeof(esp_mqtt_event_id_t));
    if (mqtt_event_queue == NULL) {
        ESP_LOGI(TAG, "mqtt_event_queue ERR.");
        return;
    }
    xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 2048, NULL, 8, NULL);
    xTaskCreate(spiffs_read_task, "spiffs_read_task", 2048, NULL, 9, NULL);
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    if (mqtt_pubilsh_uart_data) {  // 发布的是串口数据，才发送队列消息报告MQTT 事件
        ESP_LOGI(TAG, "xQueueSend mqtt_event_queue event_id = %d", event->event_id);
        if (mqtt_event_queue != NULL) {
            esp_mqtt_event_id_t  mqtt_event_id = event->event_id;
            xQueueSend(mqtt_event_queue, &mqtt_event_id, 50 / portTICK_RATE_MS);   
        }
        mqtt_pubilsh_uart_data = 0;
    }
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, mqtt_topic_read, mqtt_qos);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            mqtt_event_connect = 1;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED,  msg_id=%d", event->msg_id);
            mqtt_event_connect = 0;
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, mqtt_topic_read, "data", 0, mqtt_qos, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA,  msg_id=%d", event->msg_id);
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            if (strcmp(event->topic, mqtt_topic_read) >= 0) {   
                if (strncmp(event->data, "data", event->data_len) == 0) { 
                    ESP_LOGI(TAG, "MQTT event->data ok...");
                }
            }  

            break;
        case MQTT_EVENT_ERROR:
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGE(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = "emqx.qinyun575.cn",
        .port = 1883,
        .username = "emqx",
        .password = "public",
        .lwt_qos = mqtt_qos,
        .buffer_size = 512,
        .out_buffer_size = 512,
        //.cert_pem = (const char *)mqtt_mqtt_broker_pem_start,
    };

#if 1
    client_handle = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client_handle, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client_handle);
#else 
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
#endif

}

void app_mqtt_init(void)
{
	ESP_LOGI(TAG, "APP MQTT Startup..");

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    mqtt_app_start();

    mqtt_publish_task_init();

#if 0  // test publish
    while (1) { 
        mqtt_send_data("12345677890", 10);
        vTaskDelay(2000 / portTICK_PERIOD_MS);   // 等待先联网
    }
#endif

}
