/***********************************************************************************************************
 * Copyright (c)  2021 - 2022, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : app_gtimer.c
 * Description : app gtimer source file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2021-09-24
 ***********************************************************************************************************/
#include "app_gtimer.h"
#include "um2005C_hal.h"
#include "cw32l010_lptim.h"
#include <string.h>
#include "cw32l010_sysctrl.h"

#define APP_GTIMER_DATA_MAX					1024

uint8_t g_send_data[APP_GTIMER_DATA_MAX];
uint16_t g_send_count = 0;
uint16_t g_send_index = 0;
uint8_t g_send_flag = 0;
/***********************************************************************************************************
 * Function		: app_gtimer_count_init
 * Description	: app_gtimer_count_init
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void app_gtimer_count_irq(void)
{
	if(g_send_index >= g_send_count*8)
	{
		NVIC_DisableIRQ(LPTIM_IRQn);
		LPTIM_Cmd(DISABLE);
		g_send_flag = 1;
		um2005C_hal_direct_data(0);
	}
	else
	{
		uint16_t index = g_send_index>>3;
		uint8_t b = g_send_index & 0x07;
		if(((g_send_data[index]>>(0x07-b))&0x01) == 0x01)													 //MSB
		{
			um2005C_hal_direct_data(1);
		}
		else
		{
			um2005C_hal_direct_data(0);
		}
		g_send_index++;
	}
}

/***********************************************************************************************************
 * Function		: app_gtimer_init
 * Description	: app_gtimer_init
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void LPTIM_Configuration(void);
void app_gtimer_init(uint16_t bps)
{

	//LPTIM_Cmd(DISABLE);
	LPTIM_Configuration();
	g_send_index = 0;
	g_send_flag = 0;
}



void LPTIM_Configuration(void)
{
    LPTIM_InitTypeDef LPTIM_InitStruct = {0};
    __SYSCTRL_LPTIM_CLK_ENABLE();

    LPTIM_InitStruct.LPTIM_ClockSource = LPTIM_CLOCK_SOURCE_MCLK;
    LPTIM_InitStruct.LPTIM_CounterMode = LPTIM_COUNTER_MODE_TIME;
    LPTIM_InitStruct.LPTIM_Period = 799;
    LPTIM_InitStruct.LPTIM_Prescaler = LPTIM_PRS_DIV1;

    LPTIM_Init(&LPTIM_InitStruct);

   
    LPTIM_InternalClockConfig(LPTIM_ICLK_PCLK);

    LPTIM_ITConfig(LPTIM_IT_ARRM, ENABLE);
    CW_LPTIM->ICR = 0x00;

    LPTIM_Cmd(ENABLE);
    LPTIM_SelectOnePulseMode(LPTIM_OPERATION_REPETITIVE);
		
		
    __disable_irq();
    NVIC_EnableIRQ(LPTIM_IRQn);
    __enable_irq();		
}

/***********************************************************************************************************
 * Function		: app_gtimer_clear_data
 * Description	: app_gtimer_clear_data
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void app_gtimer_clear_data(void)
{
	memset(g_send_data,0,APP_GTIMER_DATA_MAX);
	g_send_count = 0;
}

/***********************************************************************************************************
 * Function		: app_gtimer_add_data
 * Description	: app_gtimer_add_data
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void app_gtimer_add_data(uint8_t *data,uint16_t len)
{
	memcpy(g_send_data+g_send_count,data,len);
	g_send_count += len;
}

/***********************************************************************************************************
 * Function		: app_gtimer_get_flag
 * Description	: app_gtimer_get_flag
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
uint8_t app_gtimer_get_flag(void)
{
	return g_send_flag;
}
