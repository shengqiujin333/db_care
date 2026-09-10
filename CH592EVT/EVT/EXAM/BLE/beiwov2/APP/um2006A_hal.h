/***********************************************************************************************************
 * Copyright (c)  2023 - 2024, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : um2006A_hal.h
 * Description : um2006A_hal drvier header file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2024-10-26
 ***********************************************************************************************************/
#ifndef __UM2006A_HAL_H__
#define __UM2006A_HAL_H__

#include <stdint.h>

uint8_t um2006A_hal_get_flag(void);

void um2006A_hal_clear_flag(void);

uint8_t um2006A_hal_init(void);

void spi_init(void);																						/* SPI初始化 */

void spi_cs_enable(void);																					/* CS使能 */

void spi_cs_disable(void);																					/* CS不使能 */

void spi_write_byte(uint8_t byte);																			/* SPI写一个字节 */

uint8_t spi_read_byte(void);																				/* SPI读一个字节 */

#endif
