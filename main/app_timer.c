// Copyright (C) 2018-2020 Alibaba Group Holding Limited
// Adaptations to ESP-IDF Copyright (c) 2020 Espressif Systems (Shanghai) Co. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "app_timer.h"


//static const char *TAG = "APP_TIMER";

esp_timer_handle_t esp_timer_periodic_create(esp_timer_cb_t timer_cb, const char* timer_name, long time)
{
    esp_timer_handle_t out_handle;

    esp_timer_create_args_t create_args = {
        .callback = timer_cb,
        .name     = timer_name
    };

    ESP_ERROR_CHECK(esp_timer_create(&create_args, &out_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(out_handle, time));
    return out_handle;
}

esp_timer_handle_t esp_timer_once_create(esp_timer_cb_t timer_cb, const char* timer_name, long time)
{
    esp_timer_handle_t out_handle;

    esp_timer_create_args_t create_args = {
        .callback = timer_cb,
        .name     = timer_name
    };

    ESP_ERROR_CHECK(esp_timer_create(&create_args, &out_handle));
    ESP_ERROR_CHECK(esp_timer_start_once(out_handle, time));
    return out_handle;
}


// ESP_ERROR_CHECK(esp_timer_stop(out_handle));

