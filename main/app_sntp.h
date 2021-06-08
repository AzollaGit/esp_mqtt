#pragma once

#include "stdint.h"

void app_sntp_init(void);

time_t sntp_get_timestamp(void);