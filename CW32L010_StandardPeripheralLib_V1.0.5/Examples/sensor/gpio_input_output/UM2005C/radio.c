/***********************************************************************************************************
 * Copyright (c)  2023 - 2024, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : radio.c
 * Description : radio source file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2024-10-26
 ***********************************************************************************************************/
#include "radio.h"
#include "um2005C.h"
#include "um2005C_hal.h"
#include "delay.h"
#include "app_gtimer.h"

uint16_t g_data_rate = 0;
/***********************************************************************************************************
 * Function		: radio_init
 * Description	: ≥ı ºªØ
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
ErrorStatus radio_init(void)
{
	static s_um2005C_ops_t ops = 
	{
		twi_init,
		twi_reset,
		twi_on,
		twi_off,
		twi_write_byte,
		twi_read_byte,
		delay_us,
	};
	
	um2005C_hal_init();
	um2005C_init(&ops);
	g_data_rate = 10000;
	return SUCCESS;
}

/***********************************************************************************************************
 * Function		: radio_clear_data
 * Description	: radio_clear_data
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void radio_clear_data(void)
{
	app_gtimer_clear_data();
}

/***********************************************************************************************************
 * Function		: rdio_set_preamble
 * Description	: rdio_set_preamble
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void radio_set_preamble(uint8_t len)
{
	uint8_t preamble[256];
	for(uint8_t i=0;i<len;i++)
	{
		preamble[i] = 0x55;
	}
	app_gtimer_add_data(preamble,len);
}

/***********************************************************************************************************
 * Function		: rdio_set_syncword
 * Description	: rdio_set_syncword
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void radio_set_syncword(uint8_t *sync,uint8_t len)
{
	app_gtimer_add_data(sync,len);
}

/***********************************************************************************************************
 * Function		: rdio_set_data
 * Description	: rdio_set_data
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void radio_set_data(uint8_t *data,uint8_t len,um_code_t code)
{
	if(code == CODE_EV1527)
	{
		for(uint8_t i=0;i<len;i++)
		{
			uint8_t b = data[i];
			uint8_t temp = 0;
			for(uint8_t j=0;j<8;j++)
			{
				temp <<= 4;
				if((b&0x80) == 0x80)
				{
					temp |= 0x07;
				}
				else
				{
					temp |= 0x01;
				}
				b<<=1;
				if(j%2 == 1)
				{
					app_gtimer_add_data(&temp,1);
				}
			}
		}
	}
	else
	{
		app_gtimer_add_data(data,len);
	}
}

/***********************************************************************************************************
 * Function		: rdio_into_tx
 * Description	: rdio_into_tx
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void radio_into_tx(um_time_t time)
{
	um2005C_into_tx();
	app_gtimer_init(g_data_rate);
	if(time == UM_TIME_WAITTING_FOREVER)
	{
		while(!app_gtimer_get_flag())
		{
			delay_ms(1);
		}
//		uint32_t i = 1000;
//		while(i--);
	}
}

/***********************************************************************************************************
 * Function		: radio_sleep
 * Description	: radio_sleep
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void radio_sleep()
{
	twi_off();
}
