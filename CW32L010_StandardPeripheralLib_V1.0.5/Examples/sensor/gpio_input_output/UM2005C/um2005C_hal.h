/***********************************************************************************************************
 * Copyright (c)  2023 - 2024, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : um2005c_hal.h
 * Description : um2005c_hal drvier header file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2024-10-26
 ***********************************************************************************************************/
#ifndef __UM2005C_HAL_H__
#define __UM2005C_HAL_H__

#include <type.h>
#include <stdint.h>

ErrorStatus um2005C_hal_init(void);

void um2005C_hal_direct_data(uint8_t data);

void twi_init(void);

void twi_write_byte(uint8_t byte);

uint8_t twi_read_byte(void);

void twi_reset(void);

ErrorStatus twi_on(void);

void twi_off(void);

#endif
