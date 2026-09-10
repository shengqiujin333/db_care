/***********************************************************************************************************
 * Copyright (c)  2024 - 2025, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : app_uart.c
 * Description : app uart source file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2024-09-24
 ***********************************************************************************************************/

#include "CH59x_common.h"
#include "app_ev1527.h"

#include "um2006A_hal.h"



#define APP_EV1527_TIER_CNT_MAX							65535
#define APP_EV1527_UNIT_TIME							(100)
#define APP_EV1527_CYCLE_TIME 							(APP_EV1527_UNIT_TIME * 4)
#define APP_EV1527_TOLERANCE							(APP_EV1527_CYCLE_TIME * 0.3)
#define APP_EV1527_CYCLE_MAX 							(APP_EV1527_CYCLE_TIME + APP_EV1527_TOLERANCE) 		/* 周期最大时间 单位us */
#define APP_EV1527_CYCLE_MIN 							(APP_EV1527_CYCLE_TIME - APP_EV1527_TOLERANCE) 		/* 周期最小时间 单位us */
#define APP_EV1527_UNIT_MIN  							(APP_EV1527_UNIT_TIME * 0.3)  						/* 高电平最小时间 */

#define APP_EV1527_DATA_MAX								256
uint32_t ev1527_cnt = 0;

uint8_t g_recv_data[APP_EV1527_DATA_MAX];
uint8_t g_recv_index = 0;
__attribute__((aligned(4))) uint32_t CapBuf[100];
/***********************************************************************************************************
 * Function		: app_rate_bit_check
 * Description	: app rate bit check
 * Input		: uint16_t tHCnt, uint16_t tLCnt
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void app_ev1527_check_bit(uint32_t high, uint32_t low)
{
	uint16_t cycle = high + low;
	if((cycle > APP_EV1527_CYCLE_MAX) || (cycle < APP_EV1527_CYCLE_MIN))  									/* 周期时间错误 */
	{
		g_recv_index = 0;  																					/* 编码错误，剔除已接收数据 */
		return; 
	}
	
	if(high < APP_EV1527_UNIT_MIN || low < APP_EV1527_UNIT_MIN)												/* 电平小于最小允许电平 */
	{
		g_recv_index = 0;  																					/* 编码错误，剔除已接收数据 */
		return; 
	}
	
	if(high > low)
	{
		g_recv_data[g_recv_index/8] |= (0x80>>(g_recv_index%8));
		g_recv_index++;
	}
	else
	{
		g_recv_data[g_recv_index/8] &= ~(0x80>>(g_recv_index%8));
		g_recv_index++;
	}
}

/***********************************************************************************************************
 * Function		: app_ev1527_get_time
 * Description	: 获取时间
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void app_ev1527_cler_time(void)
{
    //TMR2_ITCfg(0, TMR0_3_IT_CYC_END | TMR0_3_IT_DATA_ACT);
    TMR2_ITCfg(0, TMR0_3_IT_DATA_ACT);
	ev1527_cnt = 0;
	TMR2_ITCfg(1, TMR0_3_IT_DATA_ACT);
	//TMR2_ITCfg(1, TMR0_3_IT_CYC_END | TMR0_3_IT_DATA_ACT);
}

/***********************************************************************************************************
 * Function		: app_ev1527_get_time
 * Description	: 获取时间
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
uint32_t app_ev1527_get_time(void)
{
	return  ev1527_cnt * 65536+TMR2_GetCurrentCount();
}

/***********************************************************************************************************
 * Function		: lptimer_callback
 * Description	: 中断回调函数
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void lptimer_callback(void)
{
	ev1527_cnt++;
}

/***********************************************************************************************************
 * Function		: gpio_callback
 * Description	: 中断
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void gpio_callback(void)
{
	uint32_t time = app_ev1527_get_time();
	app_ev1527_cler_time();
	static uint32_t high_time = 0;
	

	PRINT("%d\r\n",time);
	if(GPIOB_ReadPortPin(GPIO_Pin_11) == 1)																		/* 下升沿 */
	{
		/* 低电平时间 */
		app_ev1527_check_bit(high_time,time);
	}
	else																									/* 上降沿 */
	{
		/* 高电平时间 */
		high_time = time;
	}
}

/***********************************************************************************************************
 * Function		: app_ev1527_init
 * Description	: ev1527初始化
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void app_ev1527_init(void)
{



	/* LPTimer1启动计数 */
//    GPIOA_SetBits(GPIO_Pin_14);
//    GPIOA_ModeCfg(GPIO_Pin_14, GPIO_ModeOut_PP_20mA);
    GPIOB_ModeCfg(GPIO_Pin_11, GPIO_ModeIN_Floating);
//    GPIOB_ITModeCfg(GPIO_Pin_11,GPIO_ITMode_FallEdge);
//    PFIC_EnableIRQ(GPIO_B_IRQn);
    GPIOPinRemap(1, RB_PIN_TMR2);

    TMR2_CapInit(Edge_To_Edge);
    TMR2_CAPTimeoutCfg(65536); // 设置捕捉超时时间
    //TMR2_DMACfg(1, (uint16_t)(uint32_t)&CapBuf[0], (uint16_t)(uint32_t)&CapBuf[100], Mode_Single);
    TMR2_ITCfg(1, TMR0_3_IT_DATA_ACT | TMR0_3_IT_CYC_END); // 开启DMA完成中断
    PFIC_EnableIRQ(TMR2_IRQn);

//    TMR1_TimerInit(FREQ_SYS / 100000);         // 设置定时时间 100ms
//    TMR1_ITCfg(1, TMR0_3_IT_CYC_END); // 开启中断
//    PFIC_EnableIRQ(TMR1_IRQn);
//    PRINT("BDBDBDBDBDBD\n");

}



//__INTERRUPT
//__HIGH_CODE
//void GPIOB_IRQHandler(void)
//{
//    if(GPIOB_ReadITFlagBit(GPIO_Pin_11))
//    {
//        GPIOB_ClearITFlagBit(GPIO_Pin_11);
//        gpio_callback();
//    }
//}



__INTERRUPT
__HIGH_CODE
void TMR2_IRQHandler(void)
{
    if(TMR2_GetITFlag(TMR0_3_IT_DATA_ACT))
    {
        TMR2_ClearITFlag(TMR0_3_IT_DATA_ACT);    // 清除中断标志
        gpio_callback();

    }

    if(TMR2_GetITFlag(TMR0_3_IT_CYC_END))
    {
        TMR2_ClearITFlag(TMR0_3_IT_CYC_END);    // 清除中断标志
        lptimer_callback();
    }
}


__INTERRUPT
__HIGH_CODE
void TMR1_IRQHandler(void) // TMR0 定时中断
{
    if(TMR1_GetITFlag(TMR0_3_IT_CYC_END))
    {
        TMR1_ClearITFlag(TMR0_3_IT_CYC_END); // 清除中断标志
        lptimer_callback();

    }
}
/***********************************************************************************************************
 * Function		: app_ev1527_recv_data
 * Description	: 接收解码数据
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
em_ret_t app_ev1527_recv_data(uint8_t *data,uint8_t *len)
{
    PRINT("RECV INDEX :%d\r\n",g_recv_index);
	if(g_recv_index > 8)
	{
		*len = g_recv_data[0];
		if((*len * 8 <= g_recv_index))
		{
			for(uint8_t i=0;i<*len;i++)
			{
				data[i] = g_recv_data[i];
			}
			g_recv_index = 0;
			return SUCCESS;
		}
	}
	return FAILED;
}


