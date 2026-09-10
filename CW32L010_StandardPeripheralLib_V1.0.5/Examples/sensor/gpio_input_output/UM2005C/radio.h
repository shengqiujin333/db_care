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

#include "type.h"
#include "stdint.h"

ErrorStatus radio_init(void);

void radio_clear_data(void);

void radio_set_preamble(uint8_t len);

void radio_set_syncword(uint8_t *sync,uint8_t len);

void radio_set_data(uint8_t *data,uint8_t len,um_code_t code);

void radio_into_tx(um_time_t time);

void radio_sleep(void);


#endif
