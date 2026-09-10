/***********************************************************************************************************
 * Copyright (c)  2021 - 2022, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : delay.c
 * Description : delay source file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2021-09-24
 ***********************************************************************************************************/
#include "delay.h"

//static uint32_t fac_us;                  																	/* 每个 us 的计数因子 */

/***********************************************************************************************************
 * Function		: delay_init
 * Description	: 初始化
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
//void delay_init(void)
//{
//	s_system_clock_t sys_clk;
//	system_get_clock(&sys_clk);
//	if(sys_clk.hclk > 1000000)
//	{
//		fac_us = (((uint32_t)sys_clk.hclk)+(1000000/2))/(1000000);    										/* 每us所需systick时钟个数(四舍五入公式) */
//	}
//	else																									/* 如果hclk小于1M，则最小计数已经大于1us，无法计算us和ms，只能用delay */
//	{
////		print2Debug("hclk小于1M，则单位计数的时间已经大于1us，无法计算us和ms，只能用delay或者另行计算\r\n");
//	}
//	
//	systick_init();
//	
//}

/***********************************************************************************************************
 * Function		: delay_us
 * Description	: 延时us
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void delay_us(uint16_t us)
{
//	uint32_t ticks;			
//	uint32_t told;
//	uint32_t tnow;				
//	uint32_t tcnt;					
//	uint32_t reload = 0;			
//	
//	tcnt = 0;
//	reload = systick_get_load();	//获取重载值
//	ticks = us*fac_us;
//	told = systick_get_count();		//获取刚进入时的计数器值
//	
//	while(1)
//	{
//		tnow = systick_get_count();	
//		if(tnow != told)
//		{
//			if(tnow < told)
//			{
//				tcnt += told - tnow;
//			}
//			else
//			{
//				tcnt += reload-tnow+told;
//	
//			}
//			told = tnow;
//			if(tcnt > ticks)
//			{
//				break;
//			}
//		
//		}
//			
//	}
//	
    volatile uint32_t thisCnt = us;
    while( thisCnt-- )
    {
        ;
    }	
	
}

/***********************************************************************************************************
 * Function		: delay_ms
 * Description	: 延时ms
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void delay_ms(uint16_t ms)
{
	delay(ms);
}

/***********************************************************************************************************
 * Function		: delay
 * Description	: 延时
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void delay(uint32_t count)
{
	while(count--);
}


