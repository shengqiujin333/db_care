/***********************************************************************************************************
 * Copyright (c)  2023 - 2024, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : um2005c_hal.c
 * Description : um2005c_hal driver source file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2024-10-26
 ***********************************************************************************************************/
#include "um2005C_hal.h"
#include "cw32l010_gpio.h"
#include "delay.h"
#include "cw32l010_sysctrl.h"
#include "base_types.h"

#define UM2005C_HAL_TWI_CLK_PIN 					PB2
#define UM2005C_HAL_TWI_DATA_PIN 					PB3
//#define UM2005C_HAL_TWI_GPIO0_PIN 					PD6
//#define UM2005C_HAL_TWI_GPIO1_PIN 					PA1

#define UM2005C_HAL_TWI_CLK_LOW						GPIO_WritePin(CW_GPIOB, GPIO_PIN_2, GPIO_Pin_RESET)					/* 输出低电平 */
#define UM2005C_HAL_TWI_CLK_HIGH					GPIO_WritePin(CW_GPIOB, GPIO_PIN_2, GPIO_Pin_SET)					/* 输出高电平 */
#define UM2005C_HAL_TWI_DATA_INPUT				\
do{{	\
		GPIO_InitTypeDef GPIO_InitStruct = {0};	\
    GPIO_InitStruct.Pins =  GPIO_PIN_3;\
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;\
    GPIO_InitStruct.IT   = GPIO_IT_NONE;\
    GPIO_Init( CW_GPIOB, &GPIO_InitStruct);\
}}while(0)
#define UM2005C_HAL_TWI_DATA_OUTPUT					\
do{{	\
		GPIO_InitTypeDef GPIO_InitStruct = {0};	\
    GPIO_InitStruct.Pins =  GPIO_PIN_3;\
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;\
    GPIO_InitStruct.IT   = GPIO_IT_NONE;\
    GPIO_Init( CW_GPIOB, &GPIO_InitStruct);\
}}while(0)
#define UM2005C_HAL_TWI_DATA_LOW					GPIO_WritePin(CW_GPIOB, GPIO_PIN_3, GPIO_Pin_RESET)					/* 输出低电平 */
#define UM2005C_HAL_TWI_DATA_HIGH					GPIO_WritePin(CW_GPIOB, GPIO_PIN_3, GPIO_Pin_SET)					/* 输出高电平 */
#define UM2005C_HAL_TWI_DATA_READ					GPIO_ReadPin(CW_GPIOB, GPIO_PIN_3)					/* 读取电平 */

/***********************************************************************************************************
 * Function		: um2005C_hal_init
 * Description	: 初始化
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
ErrorStatus um2005C_hal_init(void)
{
	return SUCCESS;
}

/***********************************************************************************************************
 * Function		: um2005C_hal_direct_data
 * Description	: um2005C_hal_direct_data
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2005C_hal_direct_data(uint8_t data)
{
	if(data)
	{
		UM2005C_HAL_TWI_DATA_HIGH;
	}
	else
	{
		UM2005C_HAL_TWI_DATA_LOW;
	}
}

/***********************************************************************************************************
 * Function		: twi_init
 * Description	: TWI初始化
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void twi_init(void)
{
		__SYSCTRL_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pins = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

    GPIO_Init( CW_GPIOB, &GPIO_InitStruct);	
	
    GPIO_InitStruct.Pins =  GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.IT   = GPIO_IT_NONE;
    GPIO_Init( CW_GPIOB, &GPIO_InitStruct);
	
	
	UM2005C_HAL_TWI_DATA_OUTPUT;																			/* 设置为输出 */
	UM2005C_HAL_TWI_CLK_HIGH;																				/* 设置为高电平 */
	UM2005C_HAL_TWI_DATA_HIGH;																				/* 设置为高电平 */			
}

/***********************************************************************************************************
 * Function		: twi_write_byte
 * Description	: TWI写一个字节
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void twi_write_byte(uint8_t byte)
{
	UM2005C_HAL_TWI_DATA_OUTPUT;																					/* data脚设置为输出 */
	UM2005C_HAL_TWI_CLK_HIGH;																						/* 拉高sclk脚电平 */
	for(uint8_t i=0;i<8;i++)
	{
		if(byte&0x80)																						/* 高位如果为1则拉高电平 */
		{
			UM2005C_HAL_TWI_DATA_HIGH;
		}
		else
		{
			UM2005C_HAL_TWI_DATA_LOW;																				/* 为0则拉低电平 */
		}
		UM2005C_HAL_TWI_CLK_LOW;																					/* 拉低电平 */
		UM2005C_HAL_TWI_CLK_HIGH;																					/* 拉高电平 */
		byte <<= 1;																							/* 左移1位 */
	}
	UM2005C_HAL_TWI_CLK_HIGH;	
	UM2005C_HAL_TWI_DATA_HIGH;	
}

/***********************************************************************************************************
 * Function		: twi_read_byte
 * Description	: twi读一个字节
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
uint8_t twi_read_byte(void)
{
	UM2005C_HAL_TWI_DATA_INPUT;
	UM2005C_HAL_TWI_CLK_HIGH;																						/* 拉低时钟电平 */
	
	uint8_t value = 0;
	for(uint8_t i=0;i<8;i++)
	{
		value <<= 1;
		UM2005C_HAL_TWI_CLK_LOW;
		if(UM2005C_HAL_TWI_DATA_READ == 1)							
		{
			value++;
		}
		UM2005C_HAL_TWI_CLK_HIGH;
	}
	UM2005C_HAL_TWI_DATA_OUTPUT;																					/* data脚设置为输出 */
	UM2005C_HAL_TWI_DATA_HIGH;
	UM2005C_HAL_TWI_CLK_HIGH;																						/* 拉高clk脚电平 */
	
	return value;
}

/***********************************************************************************************************
 * Function		: twi_reset
 * Description	: TWI复位
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void twi_reset(void)
{
	UM2005C_HAL_TWI_DATA_LOW;
	for(uint8_t i=0;i<50;i++)
	{
		UM2005C_HAL_TWI_CLK_LOW;																					/* 拉低电平 */
		UM2005C_HAL_TWI_CLK_HIGH;																					/* 拉高电平 */
	}
}

/***********************************************************************************************************
 * Function		: twi_on
 * Description	: 打开TWI
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
ErrorStatus twi_on(void)
{
	uint8_t count = 10;
	
	/* 唤醒 */
	UM2005C_HAL_TWI_DATA_LOW;
	UM2005C_HAL_TWI_CLK_LOW;
	delay_us(50);
	UM2005C_HAL_TWI_CLK_HIGH;
	
	/* 2ms CLK */
	UM2005C_HAL_TWI_DATA_LOW;
	for(uint16_t i=0;i<680;i++)
	{
		UM2005C_HAL_TWI_CLK_LOW;																					/* 拉低电平 */
		UM2005C_HAL_TWI_CLK_HIGH;																					/* 拉高电平 */
	}
	UM2005C_HAL_TWI_CLK_HIGH;
	UM2005C_HAL_TWI_DATA_HIGH;

	twi_write_byte(0x80);
	twi_read_byte();
	
	do
	{
		twi_reset();
		twi_write_byte(0xBF);
		if((twi_read_byte()&0x0F) == 0x07)
		{
			break;
		}
	}while(count--);
	
	if(count == 0)
	{
		return ERROR;
	}
	return SUCCESS;
}

/***********************************************************************************************************
 * Function		: twi_off
 * Description	: 关闭twi
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void twi_off(void)
{
	twi_reset();
	twi_write_byte(0xFF);
	twi_write_byte(0x02);
}
