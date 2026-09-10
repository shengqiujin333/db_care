/***********************************************************************************************************
 * Copyright (c)  2022 - 2023, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : um2005C.c
 * Description : um2005C source file
 * Author(s)   : limingzu
 * version     : V1.0
 * Modify date : 2022-12-20
 ***********************************************************************************************************/
#include "um2005C.h"
#include "rfconfig.h"

#include <math.h>
#include <stdlib.h>

s_um2005C_ops_t *g_ops;
/***********************************************************************************************************
 * Function		: um2005C_init
 * Description	: um2005C_init
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
ErrorStatus um2005C_init(s_um2005C_ops_t *ops)
{
	g_ops = ops;
	g_ops->twi_init();																						/* 初始化TWI */
	g_ops->twi_on();
	
	uint8_t len = sizeof(rf_config)/sizeof(rf_config[0]);
	for(uint8_t i=0;i<len;i++)
	{
		if(rf_config[i][0] != 0xFF)
		{
			um2005C_write_reg(rf_config[i][0],rf_config[i][1]);		
		}
	}
	return SUCCESS;
}

/***********************************************************************************************************
 * Function		: um2005C_write_reg
 * Description	: 写寄存器
 * Input		: uint8_t addr,uint8_t data
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2005C_write_reg(uint8_t addr,uint8_t data)
{
	if(addr == UM2005C_REG3F)
	{
		g_ops->twi_reset();
		
		addr |= UM2005C_REG_WIRTE;
		g_ops->twi_write_byte(addr);
		g_ops->twi_write_byte(data);
	}
	else if(addr > UM2005C_REG3F)
	{
		g_ops->twi_write_byte(UM2005C_REG3F|UM2005C_REG_WIRTE);
		g_ops->twi_write_byte(UM2005C_REG3F_PAGE1);							
		
		g_ops->delay_us(10);
		
		addr |= UM2005C_REG_WIRTE;
		g_ops->twi_write_byte(addr);							
		g_ops->twi_write_byte(data);							
		
		g_ops->delay_us(10);
		
		g_ops->twi_write_byte(UM2005C_REG3F|UM2005C_REG_WIRTE);
		g_ops->twi_write_byte(UM2005C_REG3F_PAGE0);	
	}
	else
	{
		addr |= UM2005C_REG_WIRTE;
		g_ops->twi_write_byte(addr);
		g_ops->twi_write_byte(data);
	}
}

/***********************************************************************************************************
 * Function		: um2005C_read_reg
 * Description	: 读寄存器
 * Input		: uint8_t addr
 * Output		: none
 * Return		: reg_value
 ***********************************************************************************************************/
uint8_t um2005C_read_reg(uint8_t addr)
{
	uint8_t value;
	if(addr == UM2005C_REG3F)
	{
		g_ops->twi_reset();
		
		addr &= UM2005C_REG_MASK;
		addr |= UM2005C_REG_READ;
		g_ops->twi_write_byte(addr);
		value = g_ops->twi_read_byte();
	}
	else if(addr > UM2005C_REG3F)
	{
		g_ops->twi_write_byte(UM2005C_REG3F|UM2005C_REG_WIRTE);
		g_ops->twi_write_byte(UM2005C_REG3F_PAGE1);
		
		g_ops->delay_us(10);
		
		addr &= UM2005C_REG_MASK;
		addr |= UM2005C_REG_READ;
		g_ops->twi_write_byte(addr);
		value = g_ops->twi_read_byte();
		
		g_ops->delay_us(10);
		
		g_ops->twi_write_byte(UM2005C_REG3F|UM2005C_REG_WIRTE);
		g_ops->twi_write_byte(UM2005C_REG3F_PAGE0);
	}
	else
	{
		addr &= UM2005C_REG_MASK;
		addr |= UM2005C_REG_READ;
		g_ops->twi_write_byte(addr);
		value = g_ops->twi_read_byte();
	}
	return value;
}

/***********************************************************************************************************
 * Function		: um2005C_set_freq
 * Description	: 写载波频率
 * Input		: double rf_freq
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2005C_set_freq(float freq,float ref_freq)
{
	uint32_t div = (float)(freq/ref_freq) * (1<<22);
	uint8_t lo_div = 0x00;
	if(freq >= 200 && freq <= 250)
	{
		lo_div |= (3<<6);
	}
	else if(freq >= 267 && freq <= 350)
	{
		lo_div |= (2<<6);
	}
	else if(freq >= 800 && freq <= 1000)
	{
		lo_div |= (0<<6);
	}
	else if(freq >= 400 && freq <= 500)
	{
		lo_div |= (1<<6);
	}
	
	um2005C_write_reg(UM2005C_REG12,(div>>0)&0xFF);
	um2005C_write_reg(UM2005C_REG13,(div>>8)&0xFF);
	um2005C_write_reg(UM2005C_REG14,(div>>16)&0xFF);
	um2005C_write_reg(UM2005C_REG15,lo_div | ((div>>24)&0x3F));
}

/***********************************************************************************************************
 * Function		: um2005C_set_modulation
 * Description	: 设置调制方式
 * Input		: em_modution_t mod
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2005C_set_modulation(em_modution_t mod)
{
	uint8_t value = um2005C_read_reg(UM2005C_REG10);
	switch(mod)
	{
		case OOK:
		{
			value |= UM2005C_REG10_OOK;
			break;
		}
		case FSK:
		{
			value &= ~UM2005C_REG10_OOK;
			value &= ~UM2005C_REG10_GAU;
			break;
		}
		case GFSK:
		{
			value &= ~UM2005C_REG10_OOK;
			value |= UM2005C_REG10_GAU;
			break;
		}
	}
	um2005C_write_reg(UM2005C_REG10,value);
}

/***********************************************************************************************************
 * Function		: gcd
 * Description	: 计算最大公约数
 * Input		: int a, int b
 * Output		: none
 * Return		: int
 ***********************************************************************************************************/
int gcd(int a, int b) 
{
    if(b == 0)
	{
		return abs(a);
	}
	return gcd(b, a%b);
}

/***********************************************************************************************************
 * Function		: um2005C_set_rate
 * Description	: 设置数据率
 * Input		: rate:数据率  单位：KHz, xtal：晶振频率 单位:MHz
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2005C_set_rate(float rate,float xtal)
{
	rate *= 16;
	uint32_t tx_freq = (rate*16*1000)/gcd(xtal*1e6,rate*16*1000);
	uint32_t tx_limit = (xtal*1e6)/gcd(xtal*1e6,rate*16*1000)-tx_freq;
	
	um2005C_write_reg(UM2005C_REG1A,tx_freq);
	um2005C_write_reg(UM2005C_REG1B,tx_freq>>8);
	um2005C_write_reg(UM2005C_REG1C,tx_limit);
	um2005C_write_reg(UM2005C_REG1D,tx_limit>>8);
}

/***********************************************************************************************************
 * Function		: um2005C_set_deviation
 * Description	: 写(G)FSK调制频偏分频比
 * Input		: dev:频偏  单位：KHz, xtal：晶振频率 单位:MHz
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2005C_set_deviation(float dev,float xtal)
{
	uint16_t dev_freq = (float)(dev/xtal/1000) * (1<<20);
	um2005C_write_reg(UM2005C_REG18,dev_freq&0xFF);
	um2005C_write_reg(UM2005C_REG19,(dev_freq>>8)&0xFF);
}

/***********************************************************************************************************
 * Function		: um2005C_set_compensation
 * Description	: um2005C_set_compensation
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2005C_set_compensation(ErrorStatus enable)
{
	uint8_t value = um2005C_read_reg(UM2005C_REG0B);
	if(enable)
	{
		value |= UM2005C_REG0B_PACOMP;
	}
	else
	{
		value &= ~UM2005C_REG0B_PACOMP;
	}
	um2005C_write_reg(UM2005C_REG0B,value);
}


/***********************************************************************************************************
 * Function		: um2005C_into_tx
 * Description	: um2005C_into_tx
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2005C_into_tx(void)
{
	um2005C_write_reg(UM2005C_REG3F,UM2005C_REG3F_TX);
}

/***********************************************************************************************************
 * Function		: um2005C_rst_dig
 * Description	: um2005C_rst_dig
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2005C_rst_dig(void)
{
	um2005C_write_reg(UM2005C_REG3F,UM2005C_REG3F_RST_DIG);
}
