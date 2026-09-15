/***********************************************************************************************************
 * Copyright (c)  2022 - 2023, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : app_uart.c
 * Description : app uart source file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2022-12-20
 ***********************************************************************************************************/
#include "app_um2005C.h"
#include "radio.h"
#include "app_gtimer.h"
#include "um2005C.h"
#include "delay.h"

extern uint16_t g_data_rate;

/***********************************************************************************************************
 * Function		: app_um2005C_init
 * Description	: app_um2005C_init
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void app_um2005C_init(void)
{
	
}

/***********************************************************************************************************
 * Function		: app_um2005C_send_data
 * Description	: app_um2005C_send_data
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void app_um2005C_send_data(uint8_t *data,uint16_t len)
{	
	uint8_t sync[] = {0xA7,0xA7,0x98,0xF3};
	radio_clear_data();
	radio_set_preamble(20);
	radio_set_syncword(sync,4);
	radio_set_data(data,len,CODE_NONE);
	
	radio_init();
	radio_into_tx(UM_TIME_WAITTING_FOREVER);
	radio_sleep();
	
}

/***********************************************************************************************************
 * Function		: app_um2005C_send_data_timeout
 * Description	: 有界发送: 在 timeout_ms 内完成发射返回 1, 否则失败返回 0 (FR-303/304)
 ***********************************************************************************************************/
uint8_t app_um2005C_send_data_timeout(uint8_t *data,uint16_t len,uint32_t timeout_ms)
{
	uint8_t sync[] = {0xA7,0xA7,0x98,0xF3};
	uint32_t t = 0;

	radio_clear_data();
	radio_set_preamble(20);
	radio_set_syncword(sync,4);
	radio_set_data(data,len,CODE_NONE);

	radio_init();
	um2005C_into_tx();
	app_gtimer_init(g_data_rate);

	while(!app_gtimer_get_flag()){
		delay_ms(1);
		if(++t >= timeout_ms){
			radio_sleep();
			return 0;
		}
	}
	radio_sleep();
	return 1;
}
