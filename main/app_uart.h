#pragma once

#include "stdint.h"


void app_uart_init(void);

void uart_write_data(const uint8_t *data, int len);

