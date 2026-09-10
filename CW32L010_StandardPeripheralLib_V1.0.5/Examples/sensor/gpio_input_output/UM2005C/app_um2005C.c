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
