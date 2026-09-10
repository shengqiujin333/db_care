/***********************************************************************************************************
 * Copyright (c)  2023 - 2024, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : um2006A_hal.c
 * Description : um2006A_hal driver source file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2024-10-26
 ***********************************************************************************************************/
#include "um2006A_hal.h"
#include "CONFIG.h"
#include "CH59x_gpio.h"


#define UM2006A_HAL_SPI_CS_ENABLE								GPIOB_ResetBits(GPIO_Pin_10);
#define UM2006A_HAL_SPI_CS_DISABLE								GPIOB_SetBits(GPIO_Pin_10);
#define UM2006A_HAL_SPI_CLK_LOW									GPIOB_ResetBits(GPIO_Pin_7);
#define UM2006A_HAL_SPI_CLK_HIGH								GPIOB_SetBits(GPIO_Pin_7);
#define UM2006A_HAL_SPI_DATA_LOW								GPIOB_ResetBits(GPIO_Pin_4);
#define UM2006A_HAL_SPI_DATA_HIGH								GPIOB_SetBits(GPIO_Pin_4);
#define UM2006A_HAL_SPI_DATA_GET								GPIOB_ReadPortPin(GPIO_Pin_4)
#define UM2006A_HAL_SPI_DATA_INPUT								do{GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);}while(0);
#define UM2006A_HAL_SPI_DATA_OUTPUT								do{GPIOB_ModeCfg(GPIO_Pin_4,GPIO_ModeOut_PP_20mA);}while(0);

static uint8_t g_rxbyte_flag = 1;
/***********************************************************************************************************
 * Function		: um2006A_hal_irq
 * Description	: 中断处理
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_hal_irq(void)
{
	g_rxbyte_flag = 1;
}

/***********************************************************************************************************
 * Function		: um2006A_hal_get_flag
 * Description	: 获取中断标志
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
uint8_t um2006A_hal_get_flag(void)
{
	return g_rxbyte_flag;
}

/***********************************************************************************************************
 * Function		: um2006A_hal_clear_flag
 * Description	: 清除标志位
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_hal_clear_flag(void)
{
	g_rxbyte_flag = 0;
}

/***********************************************************************************************************
 * Function		: app_um2006A_spi_init
 * Description	: 初始化SPI
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
uint8_t um2006A_hal_init(void)
{
//	gpio_init(UM2006A_HAL_GPIO1_PIN);
//	gpio_set_ie(UM2006A_HAL_GPIO1_PIN,ENABLE);
//	gpio_set_dir(UM2006A_HAL_GPIO1_PIN, GPIO_DIR_INPUT);
//	gpio_set_is(UM2006A_HAL_GPIO1_PIN,GPIO_IS_EDGE);
//	gpio_set_ibe(UM2006A_HAL_GPIO1_PIN,GPIO_IBE_SINGLE);
//	gpio_set_iev(UM2006A_HAL_GPIO1_PIN,GPIO_IEV_RISING_EDGE_HIGH_LEVEL);
//	gpio_irq_init(UM2006A_HAL_GPIO1_PIN,ENABLE,um2006A_hal_irq);
	
	return 0;
}

/***********************************************************************************************************
 * Function		: app_um2006A_spi_init
 * Description	: 初始化SPI
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void spi_init(void)
{
//    gpio_init(UM2006A_HAL_CS_PIN);
//    gpio_set_dir(UM2006A_HAL_CS_PIN,GPIO_DIR_OUTPUT);
//    gpio_set_pu(UM2006A_HAL_CS_PIN,ENABLE);
//    gpio_set_bit(UM2006A_HAL_CS_PIN);

    GPIOB_ModeCfg(GPIO_Pin_10,GPIO_ModeOut_PP_20mA);
    UM2006A_HAL_SPI_CS_DISABLE

//
//    gpio_init(UM2006A_HAL_CLK_PIN);
//    gpio_set_dir(UM2006A_HAL_CLK_PIN,GPIO_DIR_OUTPUT);
//    gpio_set_pu(UM2006A_HAL_CLK_PIN,ENABLE);
//    gpio_set_bit(UM2006A_HAL_CLK_PIN);

    GPIOB_ModeCfg(GPIO_Pin_7,GPIO_ModeOut_PP_20mA);
    UM2006A_HAL_SPI_CLK_HIGH
//
//    gpio_init(UM2006A_HAL_DATA_PIN);
//    gpio_set_dir(UM2006A_HAL_DATA_PIN,GPIO_DIR_OUTPUT);
//    gpio_set_pu(UM2006A_HAL_DATA_PIN,ENABLE);
//	gpio_set_ie(UM2006A_HAL_DATA_PIN,ENABLE);
//    gpio_set_bit(UM2006A_HAL_DATA_PIN);

    GPIOB_ModeCfg(GPIO_Pin_4,GPIO_ModeOut_PP_20mA);
    UM2006A_HAL_SPI_DATA_HIGH

}

/***********************************************************************************************************
 * Function		: spi_cs_enable
 * Description	: CS使能
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void spi_cs_enable(void)
{
	UM2006A_HAL_SPI_CS_ENABLE;
}

/***********************************************************************************************************
 * Function		: spi_cs_disable
 * Description	: CS不使能
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void spi_cs_disable(void)
{
	UM2006A_HAL_SPI_CS_DISABLE;
	UM2006A_HAL_SPI_DATA_INPUT;																				/* 防止UM2006A的SDA电平与MCU IO输出电平冲突 */
}

/***********************************************************************************************************
 * Function		: spi_cs_disable
 * Description	: spi写一个字节
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void spi_write_byte(uint8_t byte)
{
	UM2006A_HAL_SPI_DATA_OUTPUT;
	for(uint8_t i=0;i<8;i++)
    {
        UM2006A_HAL_SPI_CLK_LOW;
        if(byte & 0x80)
        {
			UM2006A_HAL_SPI_DATA_HIGH;
        }
        else
        {
            UM2006A_HAL_SPI_DATA_LOW;
        }
        UM2006A_HAL_SPI_CLK_HIGH;
		byte <<= 1;
    }
	UM2006A_HAL_SPI_CLK_HIGH;
	UM2006A_HAL_SPI_DATA_HIGH;
}

/***********************************************************************************************************
 * Function		: spi_cs_disable
 * Description	: SPI读一个字节
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
uint8_t spi_read_byte(void)
{
	uint8_t value = 0;
	UM2006A_HAL_SPI_DATA_INPUT;
	for(uint8_t i=0;i<8;i++)
    {
		value <<= 1;
		UM2006A_HAL_SPI_CLK_LOW;
        if(UM2006A_HAL_SPI_DATA_GET != 0)
        {
			value++;
        }
        UM2006A_HAL_SPI_CLK_HIGH;
    }
	UM2006A_HAL_SPI_DATA_OUTPUT;
	UM2006A_HAL_SPI_CLK_HIGH;
	UM2006A_HAL_SPI_DATA_HIGH;
	
	return value;
}
