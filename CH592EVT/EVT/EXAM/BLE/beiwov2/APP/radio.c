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
#include "um2006A.h"
#include "um2006A_hal.h"
#include "um2006A_defs.h"

/***********************************************************************************************************
 * Function		: radio_init
 * Description	: 初始化
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
em_ret_t radio_init(void)
{
	static s_um2006A_ops_t ops = 
	{
		spi_init,
		spi_cs_enable,
		spi_cs_disable,
		spi_write_byte,
		spi_read_byte,
	};
	
	um2006A_hal_init();
	um2006A_init(&ops);
	um2006A_into_rx_bpfcal();
	
	return SUCCESS;
}

/***********************************************************************************************************
 * Function		: radio_recv_data
 * Description	: 接收数据
 * Input		: uint16_t len：接收长度
 * Output		: uint8_t *data：接收到的数据
 * Return		: 接收到长度
 ***********************************************************************************************************/
em_ret_t radio_recv_data(uint8_t *data,uint8_t *len)
{
	if(1||(um2006A_hal_get_flag() == 1))
	{
		um2006A_hal_clear_flag();
		uint8_t eom_en = um2006A_read_reg(UM2006A_REG28)&UM2006A_REG28_EOM_EN;
		//PRINT("en:%d\r\n",eom_en);
		if(eom_en)
		{
			*len = um2006A_read_reg(UM2006A_REG26_RXBYTE_LEN);
		}
		else
		{
			*len = um2006A_read_reg(UM2006A_REG60_RX_PAYLOAD_ADDR_START);
		}
		for(uint8_t i=0;i<*len;i++)
		{
			data[i] = um2006A_read_reg(UM2006A_REG60_RX_PAYLOAD_ADDR_START+i);
		}
		
		um2006A_into_rx_bpfcal();
		um2006A_clear_validall();
		return SUCCESSS;
	}
	return FAILEDD;
}
