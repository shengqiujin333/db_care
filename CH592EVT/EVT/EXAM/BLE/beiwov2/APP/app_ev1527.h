/***********************************************************************************************************
 * Copyright (c)  2021 - 2022, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : app_ev1527.h
 * Description : app_ev1527 header file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2021-09-23
 ***********************************************************************************************************/
#ifndef __APP_EV1527_H__
#define __APP_EV1527_H__

#include "CH59x_common.h"
#include "type.h"


#define waitdata   (0x0001<<0)
#define read_value   (0x0001<<1)
#define calculate_value   (0x0001<<2)
#define wait_data       (0x0001<<3)

void app_ev1527_init(void);

em_ret_t app_ev1527_recv_data(uint8_t *data,uint8_t *len);

#endif
