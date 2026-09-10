/***********************************************************************************************************
 * Copyright (c)  2021 - 2022, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : delay.h
 * Description : delay header file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2021-09-23
 ***********************************************************************************************************/
#ifndef __DELAY_H__
#define __DELAY_H__

#include <stdint.h>

void delay_init(void);

void delay_us(uint16_t us);

void delay_ms(uint16_t ms);

void delay(uint32_t count);

#endif
