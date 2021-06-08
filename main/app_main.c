/* ESP32 MQTT project
   @author： Azolla
   @time  :  2021.06.05
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "app_smartconfig.h"
#include "app_https_ota.h"
#include "app_mqtt.h"
#include "app_touch.h"
#include "app_user.h"
#include "app_sntp.h"
#include "app_uart.h"


static const char *TAG = "APP_MAIN";

void app_main()
{
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    ESP_LOGI(TAG, "Startup..");
    ESP_LOGI(TAG, "Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

   // app_touch_init();

   // app_user_init();

   app_uart_init();

   app_spiffs_init();

   // ==============================================================================
   // Note: 以下为需要联网（WIFI）外设初始化，因为联网需要时间，所以放最后！

#if 0   // 使用 idf.py menuconfig 配置进行联网！
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
#else   // 配网！
    app_smartconfig_init();
    #if 0
    while (wifi_connect_status() != WIFI_EVENT_STA_CONNECTED) {
        vTaskDelay(500 / portTICK_PERIOD_MS);   // 等待先联网
    }
    #else 
    vTaskDelay(2000 / portTICK_PERIOD_MS);   // 等待先联网
    #endif
#endif

#if 0   // 使用 OTA ? (外设调试，建议先屏蔽网络代码！)
    app_https_ota_init();       // 连接上wifi之后，进行https ota 固件升级！
#endif

    app_mqtt_init(); 

    app_sntp_init();

}

