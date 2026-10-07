
/******************************************************************************/
/** \file main.c
 **
 ** A detailed description is available at
 ** @link Sample Group Some description @endlink
 **
 **   - 2021-03-12  1.0  xiebin First version for Device Driver Library of Module.
 **
 ******************************************************************************/
/*******************************************************************************
*
* 代码许可和免责信息
* 武汉芯源半导体有限公司授予您使用所有编程代码示例的非专属的版权许可，您可以由此
* 生成根据您的特定需要而定制的相似功能。根据不能被排除的任何法定保证，武汉芯源半
* 导体有限公司及其程序开发商和供应商对程序或技术支持（如果有）不提供任何明示或暗
* 含的保证或条件，包括但不限于暗含的有关适销性、适用于某种特定用途和非侵权的保证
* 或条件。
* 无论何种情形，武汉芯源半导体有限公司及其程序开发商或供应商均不对下列各项负责，
* 即使被告知其发生的可能性时，也是如此：数据的丢失或损坏；直接的、特别的、附带的
* 或间接的损害，或任何后果性经济损害；或利润、业务、收入、商誉或预期可节省金额的
* 损失。
* 某些司法辖区不允许对直接的、附带的或后果性的损害有任何的排除或限制，因此某些或
* 全部上述排除或限制可能并不适用于您。
*
*******************************************************************************/
/******************************************************************************
 * Include files
 ******************************************************************************/
#include "../inc/main.h"
#include "light.h"           /* T1: 光照原始样本统计 (UART1 调试轨迹) */

/******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/


/******************************************************************************
 * Local variable definitions ('static')                                      *
 ******************************************************************************/

/******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*****************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/




/**
  * @brief  Configures the different system clocks.
  * @param  None
  * @retval None
  */
void SYSCTRL_Configuration(void)
{
    SYSCTRL_HSI_Enable(SYSCTRL_HSIOSC_DIV6);
    SYSCTRL_LSI_Enable();
	
	    SYSCTRL_HCLKPRS_Config(SYSCTRL_HCLK_DIV1);
    SYSCTRL_PCLKPRS_Config(SYSCTRL_PCLK_DIV1);
	
    SYSCTRL_APBPeriphClk_Enable2(SYSCTRL_APB2_PERIPH_RTC, ENABLE);
    /* UART1 时钟与 PA05/PA06 由 debug_trace.c 按需开关 (FD-002 rev 4.0 §11.15) */
	
		SYSCTRL_APBPeriphClk_Enable2(SYSCTRL_APB2_PERIPH_LPTIM,ENABLE);
}






void delay(uint16_t ms)
{
    uint32_t i;
    for( ; ms >0; ms--)
    {
        i = 796*2;
        while(i--);
    }
}

extern uint8_t sample_flag;
uint8_t rtc_set_cnt = 0;
uint32_t rtc_tick_total = 0u;   /* T1: RTC 累计 1 分钟节拍 (调试轨迹) */

/*
 * RTC 1 分钟/拍: 累计 SENSOR_SAMPLE_TICKS(=3) 拍才置一次采样标志 -> 每 3 分钟采样一次
 * (readme 修改点 3; FD-002 §5.2)。采样函数消费后清 sample_flag, 故一个周期只测量一次。
 * 上报完全由 sensor_decide_report 条件门控, 无小时上报/首样本强制上报。
 * rtc_tick_total 为只读调试计数 (UART1 轨迹), 不参与采样/判定。
 */
void RTC_IRQHandlerCallBack(void)
{
    if (RTC_GetITState(RTC_IT_INTERVAL))
    {
        rtc_tick_total++;
        rtc_set_cnt++;
        if (rtc_set_cnt >= SENSOR_SAMPLE_TICKS) {
            sample_flag = 1;
            rtc_set_cnt = 0;
        }
        RTC_ClearITPendingBit(RTC_IT_INTERVAL);
    }
}








 void RTC_Configuration(void)
 {
     RTC_InitTypeDef RTC_InitStruct ={0};
     
     RTC_InitStruct.DateStruct.Day = 1;
     RTC_InitStruct.DateStruct.Month = RTC_Month_January;
     RTC_InitStruct.DateStruct.Year = 24;
     RTC_InitStruct.DateStruct.Week = RTC_Weekday_Sunday;
     RTC_InitStruct.RTC_ClockSource = RTC_RTCCLK_FROM_LSI;
     RTC_InitStruct.TimeStruct.AMPM = RTC_H12_AM;
     RTC_InitStruct.TimeStruct.H24 = RTC_HOUR12;
     RTC_InitStruct.TimeStruct.Hour = 1;
     RTC_InitStruct.TimeStruct.Minute = 0;
     RTC_InitStruct.TimeStruct.Second = 0;
     
     SYSCTRL_LSI_Enable();    // 使用LSI作为RTC的时钟源，必须在配置RTC前准备好时钟     
     RTC_Init(&RTC_InitStruct);
     RTC_SetInterval(RTC_INTERVAL_EVERY_1M);    // 1 分钟间隔一次产生中断 (采样节拍 = 3 拍)
     RTC_ITConfig(RTC_IT_INTERVAL, ENABLE);
     RTC_ClearITPendingBit(RTC_IT_ALL);
     
     NVIC_EnableIRQ(RTC_IRQn);
     RTC_Cmd(ENABLE);     
 }




/*
 * printf 重定向已移除 (T1): UART1 的初始化与输出全部由 debug_trace.c 拥有
 * (FD-002 rev 4.0 §2.4/§11.15)。调试输出必须经 debug_trace_* 接口。
 */

uint32_t hclk = 0,pclk = 0;
extern uint8_t mcu_uid[10];
/**
 ******************************************************************************
 ** \brief  Main function of project
 **
 ** \return uint32_t return value, if needed
 **
 ** This sample toggle GPIOA
 **
 ******************************************************************************/
int32_t main(void)
{

	uint8_t i = 0;
	uint32_t k = 0;
#if SENSOR_DEBUG_UART
	debug_trace_sample_t trace;   /* T1: 本采样周期的调试轨迹快照 (仅目标固件) */
#endif
    /* System Clocks Configuration */
    SYSCTRL_Configuration();
	    
   __SYSCTRL_GPIOA_CLK_ENABLE();    //Open GPIOA Clk
	 __SYSCTRL_GPIOB_CLK_ENABLE();    //Open GPIOA Clk
    /* GPIO Configuration */
		hclk = SYSCTRL_GetHClkFreq();
		pclk = SYSCTRL_GetPClkFreq();
		RTC_Configuration();
		DIGITALSIGN_GetChipUid(mcu_uid);
	
	k = 0;
	for(i = 0;i < 10;i++){
		k += mcu_uid[i];
	}
	if(k == 0){
		while(1){
			go_to_sleep();
		}
	}

	/* T1 (readme 修改点 8): UART1 调试串口启动横幅 (固件标识/UID 前 4 字节/复位来源/串口参数) */
#if SENSOR_DEBUG_UART
	debug_trace_boot(mcu_uid, SYSCTRL_GetAllRstFlag());
#endif

	k = 10000;
	while(k--);
	
//	__SYSCTRL_FLASH_CLK_ENABLE();   /* 用户注释: 本轮调试保持关闭, 不得恢复读保护 */
//	FLASH_SetReadOutLevel(FLASH_RDLEVEL2);
	
	/* 传感器初始化 (软 I2C/GXHT40/光照) 在首次采样时惰性完成 (measure.c) */
	
    while(1)
    {
        //-----------------------------------------------------------------------
			temperature_process(); /* 采样节拍: 3 分钟一拍 (ITEM-007) */
			send_data_to_gateway();/* 条件上报: sensor_decide_report 门控 (ITEM-008) */

#if SENSOR_DEBUG_UART
			/* T1: 每个采样周期在判定与发送之后输出一行 S 轨迹 (光照/温度/判定/发送同一周期) */
			if (sensor_trace_fetch(&trace) != 0u) {
				trace.tick        = rtc_tick_total;   /* RTC 累计分钟节拍 (本行对应的采样时刻) */
				trace.light_ok    = light_last_ok();
				trace.light_valid = (trace.light_ok > 0u) ? 1u : 0u;
				trace.light_min   = light_last_min();
				trace.light_max   = light_last_max();
				trace.light_mean  = light_last_mean();
				debug_trace_sample(&trace);
			}
			/* 幂等: 保证任何深睡路径之前 UART1 已排空(TC)并关闭, 采样间期无输出 */
			debug_trace_flush_close();
#endif

			go_to_sleep();
		}

}


/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
#ifdef  USE_FULL_ASSERT
 /**
   * @brief  Reports the name of the source file and the source line number
   *         where the assert_param error has occurred.
   * @param  file: pointer to the source file name
   * @param  line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
       /* USER CODE END 6 */
}
#endif

