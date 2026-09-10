/***********************************************************************************************************
 * Copyright (c)  2021 - 2022, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : app_uart.h
 * Description : app uart header file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2021-09-23
 ***********************************************************************************************************/
#ifndef __APP_UART_H__
#define __APP_UART_H__

#include "config.h"

void app_uart_init(void);

void app_uart_send_byte(uint8_t byte);

void app_uart_clear_data(void);

int app_uart_recv_bytes(uint8_t *data,uint8_t *len);

#endif

