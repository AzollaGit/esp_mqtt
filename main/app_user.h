#pragma once

#include "stdint.h"


extern uint16_t lightChannelValue;

#define BUFF_MAX_SIZE           (32)    //  数据缓存大小

typedef struct 
{
    long    timestamp;
    uint8_t value[BUFF_MAX_SIZE];
    uint8_t len;
}mqtt_data_t;

// typedef struct 
// {
//     uint16_t total;     // 总的目标数
//     uint16_t used;      // 试用数（完成的数目）
// }mqtt_info_t;


#define NVS_CMD_READ       0
#define NVS_CMD_WRITE      1

esp_err_t nvs_readwrite_blob(const char* name, const char* key, void* out_value, size_t size, uint8_t rw_cmd);

void app_spiffs_init(void);

void mqtt_spiffs_write(mqtt_data_t mqtt_data);
int mqtt_spiffs_read(mqtt_data_t *mqtt_data);
uint32_t spiffs_file_size(const char *filename);

void app_bell_contorl(uint16_t duty);

void app_relay_contorl(uint8_t channel, bool state);

void app_user_init(void);

int32_t mqtt_nvs_used(int32_t used_num, uint8_t rw_cmd);
 

