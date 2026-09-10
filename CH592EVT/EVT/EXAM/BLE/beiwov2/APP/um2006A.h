/***********************************************************************************************************
 * Copyright (c)  2023 - 2024, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : um2006A.h
 * Description : um2006A drvier header file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2024-10-26
 ***********************************************************************************************************/
#ifndef __UM2006A_H__
#define __UM2006A_H__

#include <stdint.h>
#include "um2006A_defs.h"
#include "config.h"
#include "type.h"

typedef struct
{
	void (*spi_init)(void);																					/* SPI初始化 */
	void (*spi_cs_enable)(void);																			/* CS使能 */
	void (*spi_cs_disable)(void);																			/* CS不使能 */
	void (*spi_write_byte)(uint8_t byte);																	/* spi写一个字节 */
	uint8_t (*spi_read_byte)(void);																			/* SPI读一个字节 */
}s_um2006A_ops_t;

void um2006A_init(s_um2006A_ops_t *ops);																	/* 初始化UM2006A */

void um2006A_write_reg(uint8_t addr,uint8_t value);															/* 写寄存器 */
void um2006A_write_regs(uint8_t addr,uint8_t *data,uint8_t len);											/* 写连续地址寄存器 */
uint8_t um2006A_read_reg(uint8_t addr);																		/* 读寄存器 */
uint8_t um2006A_read_regs(uint8_t addr,uint8_t *data,uint8_t len);											/* 读连续地址寄存器 */
void um2006A_read_fifo(uint8_t *data,uint8_t len);															/* 读FIFO数据 */

void um2006A_into_idle(void);																				/* 进入IDLE模式 */
void um2006A_into_sleep(void);																				/* 进入Sleep模式 */
void um2006A_into_rx(void);																					/* 直接进入RX模式 */
void um2006A_into_rx_vcocal(void);																			/* VCO校准后进入RX模式 */
void um2006A_into_rx_bpfcal(void);																			/* BPF、VCO校准后进入RX模式 */
void um2006A_into_rx_rc32kcal(void);		
																											/* RC32K、BPF、VCO校准后进入RX模式 */													
void um2006A_set_sdo_sel(em_gpio_sel sdo_sel);
void um2006A_set_gpio0_sel(em_gpio_sel gpio0_sel);
void um2006A_set_gpio1_sel(em_gpio_sel gpio1_sel);
void um2006A_set_clkout(em_clkout_t clk);


void um2006A_set_freq(double rf_freq,double ref_freq);														/* 设置频点 */

void um2006A_cal_rc32K(void);

void um2006A_set_payload_enable(em_enable_t enable);
void um2006A_set_payload_length(uint8_t length);

void um2006A_set_rssithr(float rssithr);
float um2006A_read_rssi(void);

void um2006A_clear_validall(void);

void um2006A_set_modulation(em_mod_t mod);

void um2006A_set_uart_enable(em_enable_t enable,em_gpio_pin_sel pin);

void um2006A_set_wor_extend(em_wor_extend_sel mode);
void um2006A_set_wor_t2_ext_en(em_enable_t enable);
void um2006A_set_wor_t3_ext_mode(em_t3_extmode_t mode);
void um2006A_set_wor_sleep(double time);
void um2006A_set_wor_t1(double time);
void um2006A_set_wor_t2(double time);
void um2006A_set_wor_t3(double time);

void um2006A_set_fsk_speed(float speed);
void um2006A_set_ook_speed(float speed);

void um2006A_set_manchester_mode(em_enable_t enable,em_manchst_mode_t mode);

void um2006A_reset_digit(void);

uint8_t um2006A_get_pjdvalid(void);
uint8_t um2006A_get_rssivalid(void);
uint8_t um2006A_get_syncvalid(void);
uint8_t um2006A_get_rxbyte_done(void);

void um2006A_set_maskdem_sel(em_maskdem_sel_t maskdem_sel);

void um2006A_set_dutycycle_mode(em_duc_sel_t duc_sel);

void um2006A_set_pjd_len(uint8_t len);
void um2006A_set_syncword(uint32_t data,em_syncword_len_t len);

#endif
