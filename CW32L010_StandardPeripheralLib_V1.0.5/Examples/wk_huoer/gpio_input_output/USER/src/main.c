
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
		SYSCTRL_AHBPeriphClk_Enable(DEBUG_UART_GPIO_CLK, ENABLE);
    DEBUG_UART_APBClkENx(DEBUG_UART_CLK, ENABLE);
	
		SYSCTRL_APBPeriphClk_Enable2(SYSCTRL_APB2_PERIPH_LPTIM,ENABLE);
	
	
		 REGBITS_SET(CW_SYSCTRL->AHBEN, (0x5A5A0000 |bv1));
  REGBITS_SET(CW_SYSCTRL->AHBEN, (0x5A5A0000 |bv5));
}


void LPTIM_Configuration(void);


void delay(uint16_t ms)
{
    uint32_t i;
    for( ; ms >0; ms--)
    {
        i = 796*2;
        while(i--);
    }
}

uint8_t temp_cnt = 0;
//extern uint8_t sample_flag;
uint8_t rtc_set_cnt = 0;
//extern uint8_t send_flag;

uint8_t work_period_flag = 0;
uint8_t dis_menu = 0;

uint32_t no_key = 0;

void RTC_IRQHandlerCallBack(void)
{
    if (RTC_GetITState(RTC_IT_INTERVAL))
    {
//				temp_cnt++;
//				if(temp_cnt > 10){
//					rtc_set_flag = 1;
//					temp_cnt = 0;
//				}
			
			
			rtc_set_cnt++;
			if(rtc_set_cnt == 2){
				//sample_flag = 1;
				//rtc_set_cnt = 0;
			}
			
			if(rtc_set_cnt >= 5){
				//send_flag = 1;
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
     RTC_SetInterval(RTC_INTERVAL_EVERY_1M);    // 1s间隔一次产生中断
     RTC_ITConfig(RTC_IT_INTERVAL, ENABLE);
     RTC_ClearITPendingBit(RTC_IT_ALL);
     
     NVIC_EnableIRQ(RTC_IRQn);
     RTC_Cmd(ENABLE);     
 }




#ifdef __GNUC__
    /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
    set to 'Yes') calls __io_putchar() */
    #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
    #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE
{
    UART_SendData_8bit(DEBUG_UARTx, (uint8_t)ch);

    while (UART_GetFlagStatus(DEBUG_UARTx, UART_FLAG_TXE) == RESET);

    return ch;
}

size_t __write(int handle, const unsigned char * buffer, size_t size)
{
    size_t nChars = 0;

    if (buffer == 0)
    {
        /*
         * This means that we should flush internal buffers.  Since we
         * don't we just return.  (Remember, "handle" == -1 means that all
         * handles should be flushed.)
         */
        return 0;
    }


    for (/* Empty */; size != 0; --size)
    {
        UART_SendData_8bit(DEBUG_UARTx, *buffer++);
        while (UART_GetFlagStatus(DEBUG_UARTx, UART_FLAG_TXE) == RESET);
        ++nChars;
    }

    return nChars;
}

uint32_t hclk = 0,pclk = 0;
extern uint8_t mcu_uid[10];


#define A_GPIO_PORT CW_GPIOA
#define B_GPIO_PORT CW_GPIOB

#define HE_GPIO_PINS_0  GPIO_PIN_4
#define HE_GPIO_PINS_1  GPIO_PIN_5
#define HE_GPIO_PINS_2  GPIO_PIN_6
#define HE_GPIO_PINS_3  GPIO_PIN_0
#define KEY_GPIO_PINS_0  GPIO_PIN_1
#define KEY_GPIO_PINS_1  GPIO_PIN_2


#define OUT12_GPIO_PINS  GPIO_PIN_3


void Delay(__IO uint16_t nCount)
{
    /* Decrement nCount value */
    while (nCount != 0)
    {
        nCount--;
    }
}
void NVIC_Configuration(void)
{
    __disable_irq();
    NVIC_EnableIRQ(LPTIM_IRQn);
    __enable_irq();
}
void display_io_init(void);
void gpio_init(void);

uint8_t count_value;//当前累加值
uint8_t threashold_value = 20;//阈值
uint8_t lowpower = 0;


uint8_t he0_start = 0;
uint8_t he1_start = 0;
uint8_t he2_start = 0;
uint8_t he3_start = 0;

uint16_t he0_cnt = 0;
uint16_t he1_cnt = 0;
uint16_t he2_cnt = 0;
uint16_t he3_cnt = 0;

uint8_t he_cnt_enable = 0;

uint8_t AddrBuf[256];
void storage_threasholdx(void)
{
	uint8_t  Flag, tmp8;
	uint16_t i;
    FLASH_UnlockPages(512*80, 512*80);    //解锁127页
    Flag = FLASH_ErasePages(512*80, 512*80);    //擦除最后一页面
	   if( Flag )
    {
        while(1);  //擦除失败
    }
		
    for( i=0; i<512; i++ )
    {
        tmp8 =  *((volatile uint8_t *)( 512*80 + i ));
        if( tmp8 != 0xFF )
        {
            while(1);  //对擦除后的结果进行验证失败
        }
    }		
		
		for( i=0; i<=255; i++ )
    {
        AddrBuf[i] = 0;
    }
		AddrBuf[0] = threashold_value;
		FLASH_UnlockPages(512*80, 512*80);
    Flag = FLASH_WriteBytes(512*80, AddrBuf, 256 );   // 127页按字节写入
    if( Flag )
    {
        while(1);  //写入失败
    }
			tmp8 =  *((volatile uint8_t *)( 512*80 + 0 ));
        if( tmp8 != threashold_value )
        {
            while(1);  //127页写入的数据进行验证失败
        }		
}

void storage_threashold(void)
{
    uint8_t  Flag, tmp8;
    uint16_t i;

    // 1. 擦除阶段
    FLASH_UnlockPages(512*127, 512*127);
    Flag = FLASH_ErasePages(512*127, 512*127);
    FLASH_LockAllPages();  // ✅ 例程：擦除后立即锁定
    if(Flag) { while(1); }
    
    // 验证擦除
    for(i = 0; i < 512; i++) {
        if(*((volatile uint8_t *)(512*127 + i)) != 0xFF) { while(1); }
    }
    
    // 2. 写入阶段
    for(i = 0; i < 256; i++) { AddrBuf[i] = 0; }
    AddrBuf[0] = threashold_value;
    
    FLASH_UnlockPages(512*127, 512*127);  // ✅ 写入前再解锁
    Flag = FLASH_WriteBytes(512*127, AddrBuf, 256);
    if(Flag) { while(1); }
    
    // 3. 验证阶段（✅ 用 0 而不是 i）
    tmp8 = *((volatile uint8_t *)(512*127 + 0));
    if(tmp8 != threashold_value) { while(1); }
    
    // 可选：最终锁定
    FLASH_LockAllPages();
}

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
//    RTC_InitTypeDef RTC_InitStruct = {0};
//    RTC_AlarmTypeDef RTC_AlarmStruct = {0};

    /* System Clocks Configuration */
    SYSCTRL_Configuration();
	    
   __SYSCTRL_GPIOA_CLK_ENABLE();    //Open GPIOA Clk
	 __SYSCTRL_GPIOB_CLK_ENABLE();    //Open GPIOA Clk
    /* GPIO Configuration */
//		hclk = SYSCTRL_GetHClkFreq();
//		pclk = SYSCTRL_GetPClkFreq();
		//RTC_Configuration();
		LPTIM_Configuration();
		gpio_init();
		
	
		display_io_init();
	GPIO_WritePin(B_GPIO_PORT, OUT12_GPIO_PINS,GPIO_Pin_RESET);
		NVIC_Configuration();
	
//	k = 10000;
//	while(k--);
		Display_Init();
		
//		Display_SetNumber(42);
//	__SYSCTRL_FLASH_CLK_ENABLE();
//	FLASH_SetReadOutLevel(FLASH_RDLEVEL2);
//FirmwareDelay(2000000); 

		threashold_value = *((volatile uint8_t *)(512*127 + 0));
		
		if(threashold_value > 99){
			threashold_value = 98;
		}
    while(1)
    {
        //-----------------------------------------------------------------------
      //GPIO_TogglePin(CW_GPIOB, GPIO_PIN_3);
      
			//GPIO_WritePin(B_GPIO_PORT, OUT12_GPIO_PINS,GPIO_Pin_RESET);
			//Delay(10000);
//			if(lowpower == 1){
				Display_Scan();
				
//			}else{
//				SYSCTRL_GotoDeepSleep();
//			}
			
		}

}







void display_io_init(void)
{
		GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.IT = GPIO_IT_NONE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pins = GPIO_PIN_2;

    GPIO_Init(CW_GPIOB, &GPIO_InitStruct);		
		
    GPIO_InitStruct.IT = GPIO_IT_NONE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pins = GPIO_PIN_6;

    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);		
		

    GPIO_InitStruct.IT = GPIO_IT_NONE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pins = GPIO_PIN_5;

    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);		
				
    GPIO_InitStruct.IT = GPIO_IT_NONE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pins = GPIO_PIN_4;

    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);		
		
    GPIO_InitStruct.IT = GPIO_IT_NONE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pins = GPIO_PIN_3;

    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);		
		

}


void gpio_init(void)
{
		GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.IT = GPIO_IT_FALLING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pins = HE_GPIO_PINS_0;

    GPIO_Init(B_GPIO_PORT, &GPIO_InitStruct);	
		

    GPIO_InitStruct.IT = GPIO_IT_FALLING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pins = HE_GPIO_PINS_1;

    GPIO_Init(B_GPIO_PORT, &GPIO_InitStruct);	
		
		
    GPIO_InitStruct.IT = GPIO_IT_FALLING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pins = HE_GPIO_PINS_2;

    GPIO_Init(B_GPIO_PORT, &GPIO_InitStruct);	

    GPIO_InitStruct.IT = GPIO_IT_FALLING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pins = HE_GPIO_PINS_3;

    GPIO_Init(A_GPIO_PORT, &GPIO_InitStruct);	


    GPIO_InitStruct.IT = GPIO_IT_FALLING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pins = KEY_GPIO_PINS_0;

    GPIO_Init(A_GPIO_PORT, &GPIO_InitStruct);	

    GPIO_InitStruct.IT = GPIO_IT_FALLING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pins = KEY_GPIO_PINS_1;

    GPIO_Init(A_GPIO_PORT, &GPIO_InitStruct);	



    GPIO_InitStruct.IT = GPIO_IT_NONE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pins = OUT12_GPIO_PINS;

    GPIO_Init(B_GPIO_PORT, &GPIO_InitStruct);


    GPIOB_INTFLAG_CLR(bv4);
		GPIOB_INTFLAG_CLR(bv5);
		GPIOB_INTFLAG_CLR(bv6);
		GPIOA_INTFLAG_CLR(bv0);
		
		GPIOA_INTFLAG_CLR(bv1);
		GPIOA_INTFLAG_CLR(bv2);
		
		
		
    NVIC_EnableIRQ(GPIOA_IRQn);
		NVIC_EnableIRQ(GPIOB_IRQn);


}


void GPIOA_IRQHandlerCallback(void)
{
		no_key = 0;
		lowpower = 1;
		uint8_t left_count = 0;
    if (CW_GPIOA->ISR_f.PIN0)
    {
			//he3
        GPIOA_INTFLAG_CLR(bv0);
//				count_value++;
//				left_count = threashold_value - count_value;
//			if(left_count > threashold_value){
//					left_count = 0;
//				}			
//        Display_SetNumber(left_count);
        if (he3_start == 0) {
            he3_start = 1;      // 启动处理
            he3_cnt = 0;         // 重置计数器
            he_cnt_enable = 0;   // 允许首次触发
        }
    }else if (CW_GPIOA->ISR_f.PIN1)
    {
			//key1
        GPIOA_INTFLAG_CLR(bv1);
			  if(dis_menu == 1){
					Display_SetNumber(threashold_value);
				}else{
					left_count = threashold_value - count_value;
			if(left_count > threashold_value){
					left_count = 0;
				}					
					Display_SetNumber(left_count);
				}
				
    }else if (CW_GPIOA->ISR_f.PIN2)
    {
			//key2
        GPIOA_INTFLAG_CLR(bv2);
        if(dis_menu == 1){
					Display_SetNumber(threashold_value);
				}else{
					left_count = threashold_value - count_value;
			if(left_count > threashold_value){
					left_count = 0;
				}					
					Display_SetNumber(left_count);
				}
				
		}
}

void GPIOB_IRQHandlerCallback(void)
{
		no_key = 0;
		lowpower = 1;
		uint8_t left_count = 0;
    if (CW_GPIOB->ISR_f.PIN4)
    {
			//he0
        GPIOB_INTFLAG_CLR(bv4);
//				count_value++;
//				left_count = threashold_value - count_value;
//			if(left_count > threashold_value){
//					left_count = 0;
//				}			
//        Display_SetNumber(left_count);
        if (he0_start == 0) {
            he0_start = 1;      // 启动处理
            he0_cnt = 0;         // 重置计数器
            he_cnt_enable = 0;   // 允许首次触发
        }
    }else if (CW_GPIOB->ISR_f.PIN5)
    {
			//he1
        GPIOB_INTFLAG_CLR(bv5);
//				count_value++;
//				left_count = threashold_value - count_value;
//			if(left_count > threashold_value){
//					left_count = 0;
//				}			
//        Display_SetNumber(left_count);
        if (he1_start == 0) {
            he1_start = 1;      // 启动处理
            he1_cnt = 0;         // 重置计数器
            he_cnt_enable = 0;   // 允许首次触发
        }
    }else if (CW_GPIOB->ISR_f.PIN6)
    {
			//he2
        GPIOB_INTFLAG_CLR(bv6);
//				count_value++;
//				left_count = threashold_value - count_value;
//				if(left_count > threashold_value){
//					left_count = 0;
//				}
//			Display_SetNumber(left_count);
        if (he2_start == 0) {
            he2_start = 1;      // 启动处理
            he2_cnt = 0;         // 重置计数器
            he_cnt_enable = 0;   // 允许首次触发
        }
		}
}


void he_hanldex(void)
{
	uint8_t iostate = 0;
	if(he0_start){
		he0_cnt++;
		if((he_cnt_enable == 0)&&(he0_cnt >= 4)){//修改3~5ms，改这里这个4
	//		iostate = GPIO_ReadPin(CW_GPIOB, GPIO_PIN_2);
			if(iostate == 0){
				count_value++;
				if(count_value >= threashold_value){
					count_value = 0;
					GPIO_WritePin(B_GPIO_PORT, OUT12_GPIO_PINS,GPIO_Pin_SET);
				}
				he_cnt_enable = 1;
			}else{
				he0_cnt = 0;
				he0_start = 0;
			}
			
		}else if((he0_cnt >= 40)&&(he_cnt_enable == 1)){
			he_cnt_enable = 0;
			he0_cnt = 0;
			he0_start = 0;
		}
	}
}


// 通用霍尔处理函数 (1ms调用一次)
// 调用示例: he_handle(&he0_start, &he0_cnt, CW_GPIOB, GPIO_PIN_2);
void he_handle(volatile uint8_t *start, volatile uint16_t *cnt, void *port, uint32_t pin)
{
    uint8_t iostate = 0;
    
		uint8_t left_count = 0;
    if (*start) {
        (*cnt)++;
        
        // 阶段1: 4ms 确认窗口
        if ((he_cnt_enable == 0) && (*cnt >= 3)) {
            // 🔧【留出读取IO口部分】
            iostate = GPIO_ReadPin(port, pin);
            
            if (iostate == 0) {  // 确认仍是低电平
                // 🔧【业务逻辑】计数+1
                count_value++;
							
								left_count = threashold_value - count_value;
								if(left_count > threashold_value){
									left_count = 0;
								}
								Display_SetNumber(left_count);
                if (count_value >= threashold_value) {
                    count_value = 0;
                    GPIO_WritePin(B_GPIO_PORT, OUT12_GPIO_PINS, GPIO_Pin_SET);
                }
                he_cnt_enable = 1;  // 🔒 全局加锁：40ms内其他霍尔无法触发
            } else {
                // 毛刺/无效信号，重置
                *cnt = 0;
                *start = 0;
            }
        }
        // 阶段2: 40ms 防重窗口结束
        else if ((*cnt >= 500) && (he_cnt_enable == 1)) {
            he_cnt_enable = 0;  // 🔓 全局解锁
            *cnt = 0;
            *start = 0;
        }
    }
}

void he_lptm_handle(void)
{
	// 1ms定时器中依次调用
	he_handle(&he0_start, &he0_cnt, CW_GPIOB, GPIO_PIN_4);
	he_handle(&he1_start, &he1_cnt, CW_GPIOB, GPIO_PIN_5);
	he_handle(&he2_start, &he2_cnt, CW_GPIOB, GPIO_PIN_6);
	he_handle(&he3_start, &he3_cnt, CW_GPIOA, GPIO_PIN_0);
}


//void Button_Process_Modified(void) {
//    static uint8_t pa1_stable = 0;
//    static uint8_t pa2_stable = 0;
//    static uint8_t pa1_last = 0;
//    static uint8_t pa2_last = 0;
//    
//    static uint16_t db_timer_1 = 0;
//    static uint16_t db_timer_2 = 0;
//    
//    static uint16_t long_timer_1 = 0;
//    static uint16_t long_timer_2 = 0;
//    
//    static uint8_t func_a_triggered = 0;  // 函数A触发标志
//    static uint8_t func_b_triggered = 0;  // 函数B触发标志（新增）

//    // 1. 读取原始电平
//    uint8_t raw_1 = (GPIO_ReadPin(CW_GPIOA, GPIO_PIN_1) == GPIO_Pin_RESET);
//    uint8_t raw_2 = (GPIO_ReadPin(CW_GPIOA, GPIO_PIN_2) == GPIO_Pin_RESET);

//    // 2. 消抖处理
//    if (raw_1 != pa1_stable) {
//        if (++db_timer_1 >= 5) {
//            pa1_last = pa1_stable;
//            pa1_stable = raw_1;
//            db_timer_1 = 0;
//            if (!pa1_stable) {
//                long_timer_1 = 0;
//                func_b_triggered = 0;  // 释放按键时重置函数B标志
//            }
//        }
//    } else {
//        db_timer_1 = 0;
//    }

//    if (raw_2 != pa2_stable) {
//        if (++db_timer_2 >= 5) {
//            pa2_last = pa2_stable;
//            pa2_stable = raw_2;
//            db_timer_2 = 0;
//            if (!pa2_stable) {
//                long_timer_2 = 0;
//                func_b_triggered = 0;  // 释放按键时重置函数B标志
//            }
//        }
//    } else {
//        db_timer_2 = 0;
//    }

//    // 3. 逻辑判断
//    
//    // 场景1: 两个按键同时按下 -> 触发函数A
//    if (pa1_stable && pa2_stable) {
//        //if (!func_a_triggered) {
//				if(dis_menu == 0){
//            Display_SetNumber(threashold_value);
//					  dis_menu = 1;
//				}else{
//						Display_SetNumber(count_value);
//					  dis_menu = 0;
//				}
//				no_key = 0;
//        //    func_a_triggered = 1;
//        //}
//        func_b_triggered = 0;  // 有按键按下，允许下次触发函数B
//    }
//    // 场景2: 两个按键都没有按下 -> 触发函数B（新增）
//    else if (!pa1_stable && !pa2_stable) {
//        //if (!func_b_triggered) {
//        //    Function_B();  // 调用伪函数B
//        //    func_b_triggered = 1;
//        //}

//				if(no_key >= 1000){
//					set_all_input();
//					dis_menu = 0;
//				}else{
//					no_key++;
//				}
//				
//        func_a_triggered = 0;  // 允许下次触发函数A
//        
//        // 重置长按计时器
//        long_timer_1 = 0;
//        long_timer_2 = 0;
//    }
//    // 场景3: 只有PA2按下 -> threashold_value 加1
//		if(dis_menu == 1){
//			if (pa2_stable && !pa1_stable) {
//					func_a_triggered = 0;
//					func_b_triggered = 0;
//					no_key = 0;
//					// 上升沿检测（首次按下）
//					if (pa2_stable && !pa2_last) {
//							threashold_value++;
//						Display_SetNumber(threashold_value);
//							long_timer_2 = 0;
//					}
//					
//					// 长按连发（每300ms）
//					if (++long_timer_2 >= 200) {
//							threashold_value++;
//						Display_SetNumber(threashold_value);
//							long_timer_2 = 0;
//					}
//			}
//			// 场景4: 只有PA1按下 -> threashold_value 减1
//			else if (pa1_stable && !pa2_stable) {
//					func_a_triggered = 0;
//					func_b_triggered = 0;
//					no_key = 0;
//					// 上升沿检测（首次按下）
//					if (pa1_stable && !pa1_last) {
//							threashold_value--;
//							Display_SetNumber(threashold_value);
//							long_timer_1 = 0;
//					}
//					
//					// 长按连发（每300ms）
//					if (++long_timer_1 >= 200) {
//							threashold_value--;
//						Display_SetNumber(threashold_value);
//							long_timer_1 = 0;
//					}
//			}
//	}
//}

void Button_Process_Modified(void) {
    static uint8_t pa1_stable = 0;
    static uint8_t pa2_stable = 0;
    static uint8_t pa1_last = 0;  // 用于边沿检测
    static uint8_t pa2_last = 0;
    
    static uint16_t db_timer_1 = 0;
    static uint16_t db_timer_2 = 0;
    
    static uint16_t long_timer_1 = 0;
    static uint16_t long_timer_2 = 0;
    
    static uint8_t func_a_triggered = 0;

    uint8_t raw_1 = (GPIO_ReadPin(CW_GPIOA, GPIO_PIN_1) == GPIO_Pin_RESET);
    uint8_t raw_2 = (GPIO_ReadPin(CW_GPIOA, GPIO_PIN_2) == GPIO_Pin_RESET);

    // 消抖（20ms = 2 * 10ms）
    if (raw_1 != pa1_stable) {
        if (++db_timer_1 >= 2) {
            pa1_last = pa1_stable;  // 保存变化前的状态
            pa1_stable = raw_1;
            db_timer_1 = 0;
            if (!pa1_stable) {  // 释放时重置
                long_timer_1 = 0;
                func_a_triggered = 0;
            }
        }
    } else {
        db_timer_1 = 0;
    }

    if (raw_2 != pa2_stable) {
        if (++db_timer_2 >= 2) {
            pa2_last = pa2_stable;
            pa2_stable = raw_2;
            db_timer_2 = 0;
            if (!pa2_stable) {
                long_timer_2 = 0;
                func_a_triggered = 0;
            }
        }
    } else {
        db_timer_2 = 0;
    }

    // 逻辑判断
    if (pa1_stable && pa2_stable) {
        // 双键：模式切换
        if (!func_a_triggered) {
            if(dis_menu == 0){
                Display_SetNumber(threashold_value);
                dis_menu = 1;
            } else {
                Display_SetNumber(count_value);
                dis_menu = 0;
								storage_threashold();
            }
            func_a_triggered = 1;
        }
        no_key = 0;
    }
    else if (!pa1_stable && !pa2_stable) {
        // 无按键：超时处理
        if(no_key >= 10000){  // 100 * 10ms = 1秒
            //set_all_input();
            //dis_menu = 0;
						lowpower = 0;
            //no_key = 0;
						if(dis_menu == 1){
							//storage_threashold();
							dis_menu = 0;
						}
        } else {
            no_key++;
        }
        func_a_triggered = 0;
        long_timer_1 = 0;
        long_timer_2 = 0;
    }
    else if (dis_menu == 1) {
        // PA2: 加
        if (pa2_stable && !pa1_stable) {
            no_key = 0;
            
            // ✅ 上升沿检测 + 立即更新 pa2_last（关键修复！）
            if (pa2_stable && !pa2_last) {
                threashold_value++;
								if(threashold_value > 99){
									threashold_value = 99;
								}							
                Display_SetNumber(threashold_value);
                long_timer_2 = 0;
                pa2_last = pa2_stable;  // 🔑 标记边沿已处理，防止重复触发
            }
            
            // 长按连发（300ms = 30 * 10ms）
            if (++long_timer_2 >= 400) {
                threashold_value++;
								if(threashold_value > 99){
									threashold_value = 99;
								}							
                Display_SetNumber(threashold_value);
                long_timer_2 = 0;
            }
        }
        // PA1: 减
        else if (pa1_stable && !pa2_stable) {
            no_key = 0;
            
            if (pa1_stable && !pa1_last) {
                threashold_value--;
								if(threashold_value > 99){
									threashold_value = 99;
								}							
                Display_SetNumber(threashold_value);
                long_timer_1 = 0;
                pa1_last = pa1_stable;  // 🔑 关键修复！
            }
            
            if (++long_timer_1 >= 400) {
                threashold_value--;
								if(threashold_value > 99){
									threashold_value = 99;
								}
                Display_SetNumber(threashold_value);
                long_timer_1 = 0;
            }
        }
    }
}

void LPTIM_Configuration(void)
{
    LPTIM_InitTypeDef LPTIM_InitStruct = {0};
    __SYSCTRL_LPTIM_CLK_ENABLE();

    LPTIM_InitStruct.LPTIM_ClockSource = LPTIM_CLOCK_SOURCE_MCLK;
    LPTIM_InitStruct.LPTIM_CounterMode = LPTIM_COUNTER_MODE_TIME;
    LPTIM_InitStruct.LPTIM_Period = 16;
    LPTIM_InitStruct.LPTIM_Prescaler = LPTIM_PRS_DIV1;

    LPTIM_Init(&LPTIM_InitStruct);

    //SYSCTRL_LSE_Enable(SYSCTRL_LSE_MODE_OSC, SYSCTRL_LSE_DRIVER_LEVEL2);
    LPTIM_InternalClockConfig(LPTIM_ICLK_LSI);

    LPTIM_ITConfig(LPTIM_IT_ARRM, ENABLE);
    CW_LPTIM->ICR = 0x00;

    LPTIM_Cmd(ENABLE);
    LPTIM_SelectOnePulseMode(LPTIM_OPERATION_REPETITIVE);
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

