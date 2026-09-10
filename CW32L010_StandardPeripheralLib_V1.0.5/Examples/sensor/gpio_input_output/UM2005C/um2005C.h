/***********************************************************************************************************
 * Copyright (c)  2022 - 2023, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : um2005C.h
 * Description : um2005C header file
 * Author(s)   : limingzu
 * version     : V1.0
 * Modify date : 2022-12-20
 ***********************************************************************************************************/
#ifndef __UM2005C_H__
#define __UM2005C_H__

#include "type.h"
#include "um2005C_defs.h"
#include "stdint.h"

typedef struct
{
	void (*twi_init)(void);																					/* TWI初始化 */
	void (*twi_reset)(void);																				/* TWI复位 */
	ErrorStatus (*twi_on)(void);																				/* TWI打开 */
	void (*twi_off)(void);																					/* TWI关闭 */
	void (*twi_write_byte)(uint8_t byte);																	/* TWI写一个字节 */
	uint8_t (*twi_read_byte)(void);																			/* TWI读一个字节 */
	void (*delay_us)(uint16_t us);
}s_um2005C_ops_t;

ErrorStatus um2005C_init(s_um2005C_ops_t *ops);

void um2005C_write_reg(uint8_t addr,uint8_t data);
uint8_t um2005C_read_reg(uint8_t addr);

void um2005C_set_freq(float freq,float xtal);
void um2005C_set_modulation(em_modution_t mod);
void um2005C_set_rate(float rate,float xtal);
void um2005C_set_deviation(float dev,float xtal);
void um2005C_set_compensation(ErrorStatus enable);
void um2005C_into_tx(void);
void um2005C_rst_dig(void);

#endif
