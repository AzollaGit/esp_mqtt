#pragma once

#include "stdint.h"


extern uint16_t lightChannelValue;

typedef struct 
{
    uint64_t timestamp;
    uint8_t value[32];
    uint8_t len;
}mqtt_data_t;


#define NVS_CMD_READ       0
#define NVS_CMD_WRITE      1

esp_err_t nvs_readwrite_blob(const char* name, const char* key, void* out_value, size_t size, uint8_t rw_cmd);

void app_spiffs_init(void);

void mqtt_spiffs_write(mqtt_data_t mqtt_info);
void mqtt_spiffs_erase(void);
void mqtt_spiffs_read(mqtt_data_t *mqtt_info);

void app_bell_contorl(uint16_t duty);

void app_relay_contorl(uint8_t channel, bool state);

void app_user_init(void);

