/***********************************************************************************************************
 * Copyright (c)  2021 - 2022, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : app_gtimer.h
 * Description : app gtimer header file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2021-09-26
 ***********************************************************************************************************/
#ifndef __APP_GTIMER_H__
#define __APP_GTIMER_H__

#include "stdint.h"

void app_gtimer_init(uint16_t bps);

void app_gtimer_clear_data(void);

void app_gtimer_add_data(uint8_t *data,uint16_t len);

uint8_t app_gtimer_get_flag(void);

void app_gtimer_count_irq(void);

#endif

