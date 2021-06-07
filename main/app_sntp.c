/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"

#include "app_sntp.h"

static const char *TAG = "APP_SNTP";

//#define CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH  1


#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv)
{
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

time_t sntp_timestamp(void)
{
    time_t timestamp = 0; 
    // update 'timestamp' variable with current time
    time(&timestamp);
    ESP_LOGI(TAG, "timestamp = %ld", timestamp);   // 当前时间戳
    return timestamp;
}


static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}


void app_sntp_init(void)
{

    time_t now;
    struct tm timeinfo;

    initialize_sntp();

    // wait for time to be set
    int retry = 0;
    const int retry_count = 15;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
 
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();

    char strftime_buf[64];
    while (0) {
        time(&now);
        localtime_r(&now, &timeinfo);
        ESP_LOGI(TAG, "timestamp = %ld", now);   // 当前时间戳
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // update 'now' variable with current time
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "timestamp = %ld", now);   // 当前时间戳
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);

    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2021 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        return;
    }

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    if (sntp_get_sync_mode() == SNTP_SYNC_MODE_SMOOTH) {
        struct timeval outdelta;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) {
            adjtime(NULL, &outdelta);
            ESP_LOGI(TAG, "Waiting for adjusting time ... outdelta = %li sec: %li ms: %li us",
                        (long)outdelta.tv_sec,
                        outdelta.tv_usec/1000,
                        outdelta.tv_usec%1000);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
#endif
    
}

