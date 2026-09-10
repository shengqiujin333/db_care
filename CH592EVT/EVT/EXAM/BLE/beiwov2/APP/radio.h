/***********************************************************************************************************
 * Copyright (c)  2023 - 2024, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : radio.h
 * Description : radio drvier header file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2024-10-26
 ***********************************************************************************************************/
#ifndef __RADIO_H__
#define __RADIO_H__

#include "config.h"
#include "type.h"
em_ret_t radio_init(void);

em_ret_t radio_recv_data(uint8_t *data,uint8_t *len);

#endif
