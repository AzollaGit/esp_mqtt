/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"


#include "app_smartconfig.h"

static const char *TAG = "APP_SMARTCOFIG";

#define ESP_WIFI_SSID           "xiaomi4c"
#define ESP_WIFI_PASSWORD       "1234567800"

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;


/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

static wifi_config_t wifi_config = {
    .sta = {
        // .ssid = ESP_WIFI_SSID,
        // .password = ESP_WIFI_PASSWORD,
        .ssid = { 0 },
        .password = { 0 },
        /* Setting a password implies station will connect to all security modes including WEP/WPA.
            * However these modes are deprecated and not advisable to be used. Incase your Access point
            * doesn't support WPA2, these mode can be enabled by commenting below line */
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,

        .pmf_cfg = {
            .capable = true,
            .required = false
        },
    },
};

//============================================================================================================
//============================================================================================================

typedef struct {
    uint8_t ssid[32];
    uint8_t pass[32];
    uint8_t status;
    uint8_t reset_cnt;
} wifi_config_nvs_t;

static wifi_config_nvs_t wifi_nvs = {
    .ssid = { 0 },
    .pass = { 0 },
    .status = 0,
    .reset_cnt = 0,
};

static void smartconfig_example_task(void * parm);

#define WIFI_NVS_STATUS         0XA5        // NVS WIFI 数据状态正确标识

#define STORAGE_NAMESPACE       "storage"
#define STORAGE_BLOB_KEY        "wifi"

#define NVS_WIFI_CMD_READ       0
#define NVS_WIFI_CMD_WRITE      1

esp_err_t nvs_wifi_config(wifi_config_nvs_t *wifi_nvs, uint8_t wr_cmd)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    //printf("nvs_open\r\n");

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Read the size of memory space required for blob
    size_t required_size = sizeof(wifi_config_nvs_t);  
    //printf("required_size = %d\r\n", required_size);

    // Read previously saved blob if available
    if (wr_cmd == NVS_WIFI_CMD_READ) {  
        if (required_size > 0) {
            //printf("nvs_get_blob\r\n");
            err = nvs_get_blob(my_handle, STORAGE_BLOB_KEY, wifi_nvs, &required_size);
            if (err != ESP_OK) {
                return err;
            }
        } else {
            err = ESP_FAIL;
        }
         // Close
        nvs_close(my_handle);
        return err;
    }

    // Write value including previously saved blob if available
    err = nvs_set_blob(my_handle, STORAGE_BLOB_KEY, wifi_nvs, required_size);

    if (err != ESP_OK) return err;

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

//============================================================================================================
//============================================================================================================

static void wifi_sta_connect(wifi_config_t wifi_config)
{
    ESP_ERROR_CHECK( esp_wifi_disconnect() );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    esp_wifi_connect();
}

static uint8_t wifi_event_sta = 0;

uint8_t wifi_connect_status(void)
{
    return wifi_event_sta;
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if (wifi_nvs.status != WIFI_NVS_STATUS) {  // 没有配网过
            xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        wifi_event_sta = WIFI_EVENT_STA_DISCONNECTED;
        //ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta = WIFI_EVENT_STA_CONNECTED;
        ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        
        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            uint8_t rvd_data[33] = { 0 };
            ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
            ESP_LOGI(TAG, "RVD_DATA:");
            for (int i=0; i<33; i++) {
                printf("%02x ", rvd_data[i]);
            }
            printf("\n");
        }
    
        if (wifi_nvs.status != WIFI_NVS_STATUS) {  // 没有配网过
            memcpy(wifi_nvs.ssid, evt->ssid, strlen((const char *)evt->ssid));
            memcpy(wifi_nvs.pass, evt->password, strlen((const char *)evt->password));
            wifi_nvs.status = WIFI_NVS_STATUS;   // 标识已经配网了
            nvs_wifi_config(&wifi_nvs, NVS_WIFI_CMD_WRITE);   
        } 

        ESP_LOGI(TAG, "GOT SSID: %s", wifi_nvs.ssid);
        ESP_LOGI(TAG, "GOT PASSWORD: %s", wifi_nvs.pass);

        wifi_sta_connect(wifi_config);

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}


static void initialise_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    s_wifi_event_group = xEventGroupCreate();
}

static void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}


static void wifi_reset_timer(void *args)
{
    wifi_nvs.reset_cnt = 0;
    nvs_wifi_config(&wifi_nvs, NVS_WIFI_CMD_WRITE);   
    ESP_LOGI(TAG, "wifi_reset_timer->reset_cnt = 0");
}

// 支持快速上电5次，3S内断电复位WIFI
void app_smartconfig_init(void)
{

    nvs_wifi_config(&wifi_nvs, NVS_WIFI_CMD_READ);    // read wifi ssid password.

    // ESP_LOGI(TAG, "wifi_nvs.status = %d", wifi_nvs.status);
    // ESP_LOGI(TAG, "SSID: %s", wifi_nvs.ssid);
    // ESP_LOGI(TAG, "PASSWORD: %s", wifi_nvs.pass);

    initialise_wifi();

    if (wifi_nvs.status == WIFI_NVS_STATUS) {  // 已经配网了 
        
        ESP_LOGI(TAG, "WIFI reset_cnt: %d", wifi_nvs.reset_cnt);

        if (++wifi_nvs.reset_cnt > 5) {  // 复位
            wifi_nvs.reset_cnt = 0;
            wifi_nvs.status = 0;
            memset(wifi_nvs.ssid, 0, sizeof(wifi_nvs.ssid));
            memset(wifi_nvs.pass, 0, sizeof(wifi_nvs.pass));
        }  

        nvs_wifi_config(&wifi_nvs, NVS_WIFI_CMD_WRITE);   

        if (wifi_nvs.status != WIFI_NVS_STATUS) {  // 未配
            return;
        }

        memcpy(wifi_config.sta.ssid, wifi_nvs.ssid, strlen((const char *)wifi_nvs.ssid));
        memcpy(wifi_config.sta.password, wifi_nvs.pass, strlen((const char *)wifi_nvs.pass));
        ESP_LOGI(TAG, "WIFI SSID: %s", wifi_config.sta.ssid);
        ESP_LOGI(TAG, "WIFI PASS: %s", wifi_config.sta.password);
        wifi_sta_connect(wifi_config);

        // 单词定时器3s
        esp_timer_handle_t out_handle;
        esp_timer_create_args_t create_args = {
            .callback = wifi_reset_timer,
            .name     = "wifi_reset"
        };

        ESP_ERROR_CHECK(esp_timer_create(&create_args, &out_handle));
        ESP_ERROR_CHECK(esp_timer_start_once(out_handle, 3000*1000));
    } else {
        ESP_LOGI(TAG, "WIFI smartconfig start...");
    }

}
