#pragma once

#include "stdint.h"

extern xQueueHandle mqtt_publish_queue;

void app_mqtt_init(void);

int mqtt_send_data(const char *data, int len);


