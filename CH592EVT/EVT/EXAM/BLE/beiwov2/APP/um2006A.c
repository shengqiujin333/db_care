/***********************************************************************************************************
 * Copyright (c)  2023 - 2024, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : um2006A.c
 * Description : um2006A driver source file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2024-10-26
 ***********************************************************************************************************/
#include "um2006A.h"
#include "rfconfig.h"
#include "math.h"
#include "string.h"

static s_um2006A_ops_t *g_ops;
/***********************************************************************************************************
 * Function		: um2006A_init
 * Description	: UM2006A初始化
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_init(s_um2006A_ops_t *ops)
{
	g_ops = ops;
	g_ops->spi_init();																						/* 初始化SPI */
	
	um2006A_reset_digit();                                                                                  /* 数字电路复位 */	
	DelayMs(1);																							/* 等待数字电路复位完成 */
	
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_IDLE);														/* 进入IDLE */
	uint8_t size = sizeof(rf_config)/sizeof(rf_config[0]);
	for(uint8_t i=0;i<size;i++)																				/* 初始化寄存器 */
	{
		if(rf_config[i][0] == 0xFF)
		{
			break;
		}
		um2006A_write_reg(rf_config[i][0],rf_config[i][1]);
	}
	
	um2006A_cal_rc32K();
}

/***********************************************************************************************************
 * Function		: um2006A_write_reg
 * Description	: 写寄存器
 * Input		: uint8_t addr：寄存器地址,uint8_t value：寄存器值
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_write_reg(uint8_t addr,uint8_t value)
{
	g_ops->spi_cs_enable();
	addr |= 0x80;
	g_ops->spi_write_byte(addr);
	g_ops->spi_write_byte(value);
	g_ops->spi_cs_disable();
}

/***********************************************************************************************************
 * Function		: um2006A_write_regs
 * Description	: 连续写寄存器
 * Input		: uint8_t addr：起始地址,uint8_t *data：寄存器值,uint8_t len：寄存器长度
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_write_regs(uint8_t addr,uint8_t *data,uint8_t len)
{
	g_ops->spi_cs_enable();
	addr |= 0x80;
	g_ops->spi_write_byte(addr);
	for(uint8_t i=0;i<len;i++)
	{
		g_ops->spi_write_byte(data[i]);
	}
	g_ops->spi_cs_disable();
}

/***********************************************************************************************************
 * Function		: um2006A_read_reg
 * Description	: 读寄存器
 * Input		: uint8_t addr：寄存器地址
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
uint8_t um2006A_read_reg(uint8_t addr)
{
	uint8_t value = 0;
	g_ops->spi_cs_enable();
	addr &= 0x7F;
	g_ops->spi_write_byte(addr);
	value = g_ops->spi_read_byte();
	g_ops->spi_cs_disable();
	return value;
}

/***********************************************************************************************************
 * Function		: um2006A_read_regs
 * Description	: 连续读寄存器
 * Input		: uint8_t addr：寄存器起始地址,uint8_t *data：读取寄存器的值,uint8_t len：读取长度
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
uint8_t um2006A_read_regs(uint8_t addr,uint8_t *data,uint8_t len)
{
	uint8_t temp_len = 0;
	g_ops->spi_cs_enable();
	addr &= 0x7F;
	g_ops->spi_write_byte(addr);
	for(uint8_t i=0;i<len;i++)
	{
		data[i] = g_ops->spi_read_byte();
		temp_len++;
	}
	g_ops->spi_cs_disable();
	return temp_len;
}

/***********************************************************************************************************
 * Function		: um2006A_read_fifo
 * Description	: 读FIFO
 * Input		: uint8_t *data：读取FIFO的数据,uint8_t len：读取FIFO长度
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_read_fifo(uint8_t *data,uint8_t len)
{
	um2006A_read_regs(UM2006A_REG60_RX_PAYLOAD_ADDR_START,data,len);
}

/***********************************************************************************************************
 * Function		: um2006A_into_idle
 * Description	: 进入IDLE模式
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_into_idle(void)
{
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_IDLE);	
}

/***********************************************************************************************************
 * Function		: um2006A_into_sleep
 * Description	: 进入睡眠模式
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_into_sleep(void)
{
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_IDLE);	
	um2006A_write_reg(UM2006A_REG50_SLEEP_CMD,CMD_SLEEP);	
}

/***********************************************************************************************************
 * Function		: um2006A_into_rx
 * Description	: 进入接收模式
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_into_rx(void)
{
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_IDLE);	
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_RX);	
}

/***********************************************************************************************************
 * Function		: um2006A_into_rx_vcocal
 * Description	: 先校准VCO后进入RX模式
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_into_rx_vcocal(void)
{
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_IDLE);	
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_VCO_RX);	
}

/***********************************************************************************************************
 * Function		: um2006A_into_rx_bpfcal
 * Description	: 先校准BPF，再校准VCO后进入RX模式
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_into_rx_bpfcal(void)
{
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_IDLE);	
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_RCBPF_VCO_RX);	
}

/***********************************************************************************************************
 * Function		: um2006A_into_rx_rc32kcal
 * Description	: 先校准RC32K,再校准BPF，最后校准VCO后进入RX模式
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_into_rx_rc32kcal(void)
{
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_IDLE);	
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_32K_RCBPF_VCO_RX);	
}

/***********************************************************************************************************
 * Function		: um2006A_set_sdo_sel
 * Description	: 设置SDO引脚的功能
 * Input		: em_gpio_sel sdo_sel
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_sdo_sel(em_gpio_sel sdo_sel)
{
	uint8_t tmp;
	tmp = um2006A_read_reg(UM2006A_REG24);
	tmp &= ~UM2006A_REG24_MASK_SDO_OSEL;
	tmp |= (sdo_sel<<4);
	um2006A_write_reg(UM2006A_REG24, tmp);
}

/***********************************************************************************************************
 * Function		: um2006A_set_gpio0_osel
 * Description	: 设置GPIO0引脚的功能
 * Input		: em_gpio_sel gpio0_sel
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_gpio0_sel(em_gpio_sel gpio0_sel)
{
	uint8_t tmp;
	tmp = um2006A_read_reg(UM2006A_REG30);
	tmp &= ~UM2006A_REG30_MASK_GPIO0_OSEL;
	tmp |= gpio0_sel;
	um2006A_write_reg(UM2006A_REG30, tmp);
}

/***********************************************************************************************************
 * Function		: um2006A_set_gpio1_osel
 * Description	: 设置GPIO1引脚的功能
 * Input		: em_gpio_sel gpio1_sel
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_gpio1_sel(em_gpio_sel gpio1_sel)
{
	uint8_t tmp;
	tmp = um2006A_read_reg(UM2006A_REG30);
	tmp &= ~UM2006A_REG30_MASK_GPIO1_OSEL;
	tmp |= (gpio1_sel<<4);
	um2006A_write_reg(UM2006A_REG30, tmp);
}

/***********************************************************************************************************
 * Function		: um2006A_set_clkout
 * Description	: GPIO设置时钟输出
 * Input		: em_clkout_t clk：选择时钟输出类型
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_clkout(em_clkout_t clk)
{
	uint8_t reg09 = um2006A_read_reg(UM2006A_REG09);
	reg09 &= ~UM2006A_REG09_MASK_CLKOUT_SEL;
	um2006A_write_reg(UM2006A_REG09,reg09|clk);
}

/***********************************************************************************************************
 * Function		: um2006A_set_freq
 * Description	: 设置频点
 * Input		: double rf_freq：频点，单位MHz， double ref_freq：晶振，单位MHz
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_freq(double freq,double ref_freq)
{
	float lo_freq = 0.34;
	if((um2006A_read_reg(UM2006A_REG29) & UM2006A_REG29_BPFIF_SEL) == 0)                            		/* 获取中频频点 */
	{
		lo_freq = 0.2;																					
	}
	
	if(freq < 600)
	{
		ref_freq = ref_freq/2;
	}
	
	uint32_t div = (freq + lo_freq)/ref_freq;
	uint32_t nfrac = ((float)(freq + lo_freq)/ref_freq - div)*131072+0.5;									/* 2^17 = 131072 */
	um2006A_write_reg(UM2006A_REG04_FREQ_FRAC_BIT7_0,(nfrac>>0)&0xFF);
	um2006A_write_reg(UM2006A_REG05_FREQ_FRAC_BIT15_8,(nfrac>>8)&0xFF);
	um2006A_write_reg(UM2006A_REG06,((div<<2) | (nfrac>>16))&0xFF);
}

/***********************************************************************************************************
 * Function		: um2006A_cal_rc32K
 * Description	: RC32K校准
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_cal_rc32K(void)
{
	uint8_t reg03 = um2006A_read_reg(UM2006A_REG03);
	uint8_t reg07 = um2006A_read_reg(UM2006A_REG07);
	uint8_t reg51 = um2006A_read_reg(UM2006A_REG51_WORK_CMD);
	
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_IDLE);
	um2006A_write_reg(UM2006A_REG03,reg03 & ~UM2006A_REG03_MASK_DUC_SEL);									/* 关闭Duty Cycle */
    um2006A_write_reg(UM2006A_REG07,reg07 | UM2006A_REG07_MASK_RC32K_EN);									/* 使能RC32K */
    um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_32K_RCBPF_VCO_RX);											/* 进行RC32K校准 */
	uint32_t count = 5000;
	while(count--)																
	{
		if((um2006A_read_reg(UM2006A_REG48)&UM2006A_REG48_RC32K_CAL_DONE) == UM2006A_REG48_RC32K_CAL_DONE)	/* 等待校准完成 */
		{
			break;
		}
		DelayUs(1);
	}
	
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_IDLE);
	um2006A_write_reg(UM2006A_REG03,reg03);																	/* 恢复功能 */
	um2006A_write_reg(UM2006A_REG07,reg07);
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,reg51);
}

/***********************************************************************************************************
 * Function		: um2006A_set_payload_enable
 * Description	: 数据包使能
 * Input		: em_enable_t enable
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_payload_enable(em_enable_t enable)
{
	uint8_t value = um2006A_read_reg(UM2006A_REG17);
	
	if(enable)
	{
		value |= 0x80;   		
	}
	else
	{
		value &= ~0x80;                                            
	}
	um2006A_write_reg(UM2006A_REG17,value);
}

/***********************************************************************************************************
 * Function		: um2006A_set_payload_length
 * Description	: 数据包接收长度设置
 * Input		: uint8_t length：长度 单位byte
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_payload_length(uint8_t length)
{
	uint8_t value = um2006A_read_reg(UM2006A_REG28);
	value |= 0x01;
	um2006A_write_reg(UM2006A_REG28,value);                 												/* payload长度由寄存器rxbyte_len控制使能 */
	um2006A_write_reg(UM2006A_REG26_RXBYTE_LEN,length);   													/* 设置payload长度 */
}

/***********************************************************************************************************
 * Function		: um2006A_set_rssithr
 * Description	: 设置RSSI门限值
 * Input		: float rssithr
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_rssithr(float rssithr)
{
	uint8_t rssi = fabs(rssithr);
	if(rssi>0x7F)
	{
		rssi = 0x7F;
	}
	um2006A_write_reg(UM2006A_REG55_RSSI_THR,rssi*2);
}

/***********************************************************************************************************
 * Function		: um2006A_read_rssi
 * Description	: 读RSSI值
 * Input		: none
 * Output		: none
 * Return		: uint8_t RSSI值 单位dBm
 ***********************************************************************************************************/
float um2006A_read_rssi(void)
{
	return (float)(0-um2006A_read_reg(UM2006A_REG46_RSSI)/2);
}

/***********************************************************************************************************
 * Function		: um2006A_clear_validall
 * Description	: 清除rssi_valid、pjd_valid、sync_valid、rxbyte_done
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_clear_validall(void)
{
	um2006A_write_reg(UM2006A_REG52, UM2006A_REG52_MASK_CLR_SYNC);
}

/***********************************************************************************************************
 * Function		: um2006A_set_modulation
 * Description	: 设置调制方式
 * Input		: em_mod_t mod  1：FSK 0:OOK
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_modulation(em_mod_t mod)
{
	uint8_t reg03 = um2006A_read_reg(UM2006A_REG03);
	reg03 &= ~UM2006A_REG03_MASK_FSKDEM_EN;
	um2006A_write_reg(UM2006A_REG03,reg03 |= mod<<0x01);
}

/***********************************************************************************************************
 * Function		: um2006A_set_uart_enable
 * Description	: UM2006A串口输出使能，仅包模式下可用
 * Input		: em_enable_t enable ,em_gpio_pin_sel pin
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_uart_enable(em_enable_t enable,em_gpio_pin_sel pin)
{
	uint8_t reg03 = um2006A_read_reg(UM2006A_REG03);
	reg03 &= ~UM2006A_REG03_MASK_UART_EN;
	
	if(enable)
	{
		um2006A_write_reg(UM2006A_REG03,reg03|0x01);														/* UART功能使能，波特率默认115200 */
		switch(pin)
		{
			case SDO:                                                               						/* 选择SDO脚输出UART_TX信号 */
			{
				um2006A_set_sdo_sel(GPIO_UART_TXD);
				break;
			}
			case GPIO0:
			{
				um2006A_set_gpio0_sel(GPIO_UART_TXD);                             							/* 选择GPIO0脚输出UART_TX信号 */
				break;
			}
			case GPIO1:                                                          							/* 选择GPIO1脚输出UART_TX信号 */
			{
				um2006A_set_gpio1_sel(GPIO_UART_TXD);
				break;
			}
			default:
			{
				break;
			}
		}	
	}
	else
	{
		um2006A_write_reg(UM2006A_REG03,reg03);                                 							/* 关闭UART功能 */
	}
}

/***********************************************************************************************************
 * Function		: um2006A_set_wor_extend
 * Description	: UM2006A设置T1时间内的拓展模式
 * Input		: em_wor_extend_sel mode
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_wor_extend(em_wor_extend_sel mode)
{
	uint8_t reg56 = um2006A_read_reg(UM2006A_REG56);
	reg56 &= ~UM2006A_REG56_MASK_WOR_EXT_SEL;
	
	switch(mode)
	{
		case WOR_EXTEND_NONE:                                                   							/* 无扩展 */
		{
			um2006A_write_reg(UM2006A_REG56,reg56|WOR_EXTEND_NONE);
			break;
		}
		case WOR_EXTEND_RSSI:                                                  								/* T1时间RSSI有效,进入T2 */
		{
			um2006A_write_reg(UM2006A_REG56,reg56|WOR_EXTEND_RSSI);
			break;
		}
		case WOR_EXTEND_PJD:                                                  								/* T1时间PJD有效,进入T2 */
		{
			um2006A_write_reg(UM2006A_REG56,reg56|WOR_EXTEND_PJD);
			break;
		}
		case WOR_EXTEND_RSSI_PJD:                                            								/* T1时间RSSI与PJD有效,进入T2 */
		{
			um2006A_write_reg(UM2006A_REG56,reg56|WOR_EXTEND_RSSI_PJD);
			break;
		}
		default:
		{
			break;
		}		
	}
}

static uint8_t wor_sleep[16][3] = {0};                 														/* 保存wor sleep配置，wor_sleep[][0]为wor时钟分频系数，wor_sleep[][1]为reg0A值，wor_sleep[][2]为reg0B值 */
static uint8_t wor_t1[16][3] = {0};					   														/* 保存wor_t1配置，wor_t1[][0]为wor时钟分频系数，wor_t1[][1]为reg0C值，wor_t1[][2]为reg0D值 */
static uint8_t wor_t2[16][3] = {0};                    														/* 保存wor_t2配置，wor_t2[][0]为wor时钟分频系数，wor_t2[][1]为reg57值，wor_t2[][2]为reg58值 */
static uint8_t wor_t3[16][3] = {0};                    														/* 保存wor_t1配置，wor_t3[][0]为wor时钟分频系数，wor_t3[][1]为reg59值，wor_t3[][2]为reg5A值 */
/***********************************************************************************************************
 * Function		: um2006A_set_wor_calculate
 * Description	: 根据设置的时间窗口，倒推寄存器配置
 * Input		: uint8_t array[][3],double time
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_wor_calculate(uint8_t array[][3],double time)
{
    const double epsilon = 1e-6; 																			/* 允许的浮点误差范围 */
	uint8_t index = 0;

    /* 遍历所有可能的n值 (0-15) */
    for(uint8_t n = 0; n <= 15; ++n) 
	{
        uint16_t pow2_n = 1 << n;       				   													/* 计算2^n */
        double denominator = UM2006A_RCCLOCK / pow2_n; 														/* 计算分母32/(2^n) */
        double target_sum = time * denominator; 															/* 计算目标总和值 */

        /* 检查数值有效性 */
        if(target_sum < -epsilon || target_sum > 65535 + epsilon)
		{
			continue;
		}

        /* 四舍五入并校验整数精度 */
        long sum_val = round(target_sum);
		
        if(fabs(target_sum - sum_val) > epsilon)
		{
			continue;
		}
		
        if(sum_val < 0 || sum_val > 65535)
		{
			continue;
		}

        /* 分解为x和y */
        uint8_t x = sum_val % 256;
        uint8_t y = sum_val / 256;
        if(y > 255)
		{
			continue;
		}

        /* 逆向验证公式准确性 */
        double verify = (x + y * 256) * pow2_n / UM2006A_RCCLOCK;
		
        if(fabs(verify - time) > epsilon) 
		{
			continue;
		}

		array[index][0] = n;
		array[index][1] = x;
		array[index][2] = y;
		index++;
	}
}

/***********************************************************************************************************
 * Function		: find_match_rows
 * Description	: 当开启多个时间窗口时，选择WOR时钟分频一致的配置
 * Input		: uint8_t array1[][3],array2[][3],array3[][3],array4[][3] - 包含配置参数的二维数组（每行首元素为分频值）
 *              ：uint8_t count - 需要匹配的数组数量（2/3/4）
 * Output		: none
 * Return		: uint16_t - 合成值，包含所有匹配行的索引（按array1-array4顺序存放于16位中）
 ***********************************************************************************************************/
uint16_t find_match_rows(uint8_t array1[][3],uint8_t array2[][3],uint8_t array3[][3],uint8_t array4[][3],uint8_t count)
{
	uint16_t value = 0;                                                  									/* 存储最终合成的16位值（每个数组索引占4位）*/
	uint8_t i,y,x,z;                                                    									/* 循环索引（i对应array1，y对应array2，x对应array3，z对应array4）*/
	
	if(count > 1 && count <= 4)																				/* count大于1，小于等于4时 */
	{
		if(count == 2)                                                    									/* 匹配两个数组的配置 */
		{
			for(i=0;i<16;i++)
			{
				for(y=0;y<16;y++)
				{
					if(array1[i][0] == array2[y][0])                    									/* 检查两数组当前行的首元素（分频值）是否相等 */
					{
						value = y<<4;
						value |= i;
						return value;                                   									/* 合成值：高4位存array2索引，低4位存array1索引 */
					}
				}
			}
		}
		else if(count == 3)                                              									/* 匹配三个数组的配置 */
		{
			for(i=0;i<16;i++)
			{
				for(y=0;y<16;y++)
				{
					for(x=0;x<16;x++)
					{
						if(array1[i][0] == array2[y][0] && array2[y][0] == array3[x][0])               		/* 检查三数组当前行的首元素（分频值）是否相等 */
						{
							value = x<<8;
							value |= y<<4;
							value |= i;
							return value;                                                        			/* 合成值：array3索引(8-11), array2索引(4-7), array1索引(0-3) */
						}
					}
				}
			}
		}
		else if(count == 4)                                                                       			/* 匹配四个数组的配置 */ 
		{
			for(i=0;i<16;i++)
			{
				for(y=0;y<16;y++)
				{
					for(x=0;x<16;x++)
					{
						for(z=0;z<16;z++)
						{
							if(array1[i][0] == array2[y][0] && array2[y][0] == array3[x][0] && array3[x][0] == array4[z][0]) /* 检查四数组当前行的首元素（分频值）是否相等 */
							{
								value = z<<12;
								value |= x<<8;
								value |= y<<4;
								value |= i;
								return value;                                                    			/* 合成值：array4索引(15-12),array3索引(8-11), array2索引(4-7), array1索引(0-3) */
							}
						}
					}
				}
			}
		}
	}
	return 0xFF;                                                                               				/* 无效count或未找到匹配时返回FF */
}

/***********************************************************************************************************
 * Function		: um2006A_clear_wor_config
 * Description	: 清除wor相关配置
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_clear_wor_config(uint8_t array[][3], uint8_t rows)
{
	if(rows > 0 && rows <= 16)
	{
		 memset(array, 0, rows * 3 * sizeof(uint8_t));
	}
}

/***********************************************************************************************************
 * Function		: um2006A_set_wor_sleep
 * Description	: 设置SLEEP的时间
 * Input		: double time 单位ms
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_wor_sleep(double time)
{
	um2006A_clear_wor_config(wor_sleep,16);
    um2006A_set_wor_calculate(wor_sleep,time);
	
	uint8_t reg0E = um2006A_read_reg(UM2006A_REG0E);
	reg0E &= ~UM2006A_REG0E_MASK_WOR_CLK_SEL;
	
	um2006A_write_reg(UM2006A_REG0E,reg0E | wor_sleep[0][0]);
	um2006A_write_reg(UM2006A_REG0A_WOR_TIMER_SET_BIT7_0,wor_sleep[0][1]);
	um2006A_write_reg(UM2006A_REG0B_WOR_TIMER_SET_BIT15_8,wor_sleep[0][2]);
}

/***********************************************************************************************************
 * Function		: um2006A_set_wor_t1
 * Description	: 设置T1的时间
 * Input		: double time  单位ms
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_wor_t1(double time)
{
	um2006A_clear_wor_config(wor_t1,16);
	
	um2006A_set_wor_calculate(wor_t1,time);
    uint16_t value = find_match_rows(wor_sleep,wor_t1,wor_t2,wor_t3,2);
	uint8_t reg0E = um2006A_read_reg(UM2006A_REG0E);
	reg0E &= ~UM2006A_REG0E_MASK_WOR_CLK_SEL;
    
	if(0xFF != value)
    {
        uint8_t sleep_index = value&0x0F;
        uint8_t t1_index = value>>4;
        
        um2006A_write_reg(UM2006A_REG0E,reg0E | wor_sleep[sleep_index][0]);
        um2006A_write_reg(UM2006A_REG0A_WOR_TIMER_SET_BIT7_0,wor_sleep[sleep_index][1]);
        um2006A_write_reg(UM2006A_REG0B_WOR_TIMER_SET_BIT15_8,wor_sleep[sleep_index][2]);
        
        um2006A_write_reg(UM2006A_REG0E,reg0E | wor_t1[t1_index][0]);
        um2006A_write_reg(UM2006A_REG0C_WOR_RTXTIMER_SET_BIT7_0,wor_t1[t1_index][1]);
    }
}

/***********************************************************************************************************
 * Function		: um2006A_wor_t2_ext_en
 * Description	: T2扩展使能
 * Input		: em_enable_t enable
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_wor_t2_ext_en(em_enable_t enable)
{
	uint8_t reg56 = um2006A_read_reg(UM2006A_REG56);
	if(enable)
	{
		um2006A_write_reg(UM2006A_REG56,reg56|UM2006A_REG56_MASK_WOR_T2_EXT_EN);
	}
	else
	{
		um2006A_write_reg(UM2006A_REG56,reg56 & ~UM2006A_REG56_MASK_WOR_T2_EXT_EN);
	}
}

/***********************************************************************************************************
 * Function		: um2006A_set_wor_t2
 * Description	: 设置T2的时间
 * Input		: double time  单位ms
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_wor_t2(double time)
{
	um2006A_clear_wor_config(wor_t2,16);
	
	um2006A_set_wor_calculate(wor_t2,time);
    uint16_t value = find_match_rows(wor_sleep,wor_t1,wor_t2,wor_t3,3);
	uint8_t reg0E = um2006A_read_reg(UM2006A_REG0E);
	reg0E &= ~UM2006A_REG0E_MASK_WOR_CLK_SEL;
    
    if(0xFF != value)
    {
        uint8_t sleep_index = value&0x0F;
        uint8_t t1_index = (value>>4)&0x0F;
        uint8_t t2_index = value>>8;
          
        um2006A_write_reg(UM2006A_REG0E,reg0E | wor_sleep[sleep_index][0]);
        um2006A_write_reg(UM2006A_REG0A_WOR_TIMER_SET_BIT7_0,wor_sleep[sleep_index][1]);
        um2006A_write_reg(UM2006A_REG0B_WOR_TIMER_SET_BIT15_8,wor_sleep[sleep_index][2]);
        
        um2006A_write_reg(UM2006A_REG0C_WOR_RTXTIMER_SET_BIT7_0,wor_t1[t1_index][1]);
        um2006A_write_reg(UM2006A_REG0D_WOR_RTXTIMER_SET_BIT15_8,wor_t1[t1_index][2]);
        
        um2006A_write_reg(UM2006A_REG57_WOR_T2_RX_SET_BIT7_0,wor_t2[t2_index][1]);
        um2006A_write_reg(UM2006A_REG58_WOR_T2_RX_SET_BIT15_8,wor_t2[t2_index][2]);
    
	}
}

/***********************************************************************************************************
 * Function		: um2006A_wor_t3_ext_mode
 * Description	: 设置T3扩展模式
 * Input		: em_t3_extmode_t mode
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_wor_t3_ext_mode(em_t3_extmode_t mode)
{
	uint8_t reg56 = um2006A_read_reg(UM2006A_REG56);
	
	if(mode == ALWAYS_RX)                                    //进入T3后保持RX状态
	{
		um2006A_write_reg(UM2006A_REG56,reg56 | 1<<3);
	}
	else if(mode == EXTEND_INTO_SLEEP)                       //T3扩展结束后进入SLEEP
	{
		um2006A_write_reg(UM2006A_REG56,reg56 & ~(1<<3));
	}
}

/***********************************************************************************************************
 * Function		: um2006A_set_wor_t3
 * Description	: 设置T3的时间
 * Input		: double time  单位ms
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_wor_t3(double time)
{
	um2006A_clear_wor_config(wor_t3,16);
	
	um2006A_set_wor_calculate(wor_t3,time);
	uint16_t value = find_match_rows(wor_sleep,wor_t1,wor_t2,wor_t3,4);
	uint8_t reg0E = um2006A_read_reg(UM2006A_REG0E);
	reg0E &= ~UM2006A_REG0E_MASK_WOR_CLK_SEL;
	
    if(0xFF != value)
    {
        uint8_t sleep_index = value&0x0F;
        uint8_t t1_index = (value>>4)&0x0F;
        uint8_t t2_index = (value>>8)&0x0F;
        uint8_t t3_index = value>>12;
           
        um2006A_write_reg(UM2006A_REG0E,reg0E | wor_sleep[sleep_index][0]);
        um2006A_write_reg(UM2006A_REG0A_WOR_TIMER_SET_BIT7_0,wor_sleep[sleep_index][1]);
        um2006A_write_reg(UM2006A_REG0B_WOR_TIMER_SET_BIT15_8,wor_sleep[sleep_index][2]);
        
        um2006A_write_reg(UM2006A_REG0C_WOR_RTXTIMER_SET_BIT7_0,wor_t1[t1_index][1]);
        um2006A_write_reg(UM2006A_REG0D_WOR_RTXTIMER_SET_BIT15_8,wor_t1[t1_index][2]);
        
        um2006A_write_reg(UM2006A_REG57_WOR_T2_RX_SET_BIT7_0,wor_t2[t2_index][1]);
        um2006A_write_reg(UM2006A_REG58_WOR_T2_RX_SET_BIT15_8,wor_t2[t2_index][2]);
        
        um2006A_write_reg(UM2006A_REG59_WOR_T3_RX_SET_BIT7_0,wor_t3[t3_index][1]);
        um2006A_write_reg(UM2006A_REG5A_WOR_T3_RX_SET_BIT15_8,wor_t3[t3_index][2]);
    }
}

static float shape_if_offset_200K[4] = {409,3387,27126,217800};
static float shape_if_offset_340K[4] = {715,5779,46244,367575};
/***********************************************************************************************************
 * Function		: um2006A_set_fsk_speed
 * Description	: 设置FSK数据率
 * Input		: float speed  单位KHz
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_fsk_speed(float speed)
{
	uint8_t reg1c = 0;
	float fskclk = 0;
	uint32_t value = 0;
	
	uint8_t reg09 = um2006A_read_reg(UM2006A_REG09);
	uint8_t reg17 = um2006A_read_reg(UM2006A_REG17);
	uint8_t reg13 = um2006A_read_reg(UM2006A_REG13);
	
	reg09 &= ~UM2006A_REG09_MASK_FSKCLK_SEL;
	reg17 &= ~(UM2006A_REG17_MASK_UPCIC_EN|UM2006A_REG17_MASK_PKT_CLK_SEL);									
	reg13 &= ~ UM2006A_REG13_MASK_SHAPE_SCALE;
	
	um2006A_write_reg(UM2006A_REG17,reg17|UM2006A_REG17_MASK_UPCIC_EN);									    //CIC不参与计算
	
	if(speed > 250)
	{
		return;
	}
	else if(speed > 20)
	{
		fskclk = (float)UM2006A_XOSC/8;
		um2006A_write_reg(UM2006A_REG09,reg09|(XOSC_8DIV<<6));
															
		reg1c = (float)(fskclk/speed*1000)+0.5;
		um2006A_write_reg(UM2006A_REG1C_CDR_STD,reg1c);
	}
	else if(speed > 10)
	{
		fskclk = (float)UM2006A_XOSC/16;
		um2006A_write_reg(UM2006A_REG09,reg09|(XOSC_16DIV<<6));
		
		reg1c = (float)(fskclk/speed*1000)+0.5;
		um2006A_write_reg(UM2006A_REG1C_CDR_STD,reg1c);
	}
	else if(speed > 5)
	{
		fskclk = (float)UM2006A_XOSC/32;
		um2006A_write_reg(UM2006A_REG09,reg09|(XOSC_32DIV<<6));
		
		reg1c = (float)(fskclk/speed*1000)+0.5;
		um2006A_write_reg(UM2006A_REG1C_CDR_STD,reg1c);
	}
	else if(speed > 2)
	{	
		fskclk = (float)UM2006A_XOSC/64;
		um2006A_write_reg(UM2006A_REG09,reg09|(XOSC_64DIV<<6));
		
		reg1c = (float)(fskclk/speed*1000)+0.5;
		um2006A_write_reg(UM2006A_REG1C_CDR_STD,reg1c);
	}
	else if(speed > 0)
	{
		reg17 |= UM2006A_REG17_MASK_PKT_CLK_SEL;
		reg17 &= ~UM2006A_REG17_MASK_UPCIC_EN;
		um2006A_write_reg(UM2006A_REG17,reg17);
		
		fskclk = (float)UM2006A_XOSC/64;
		um2006A_write_reg(UM2006A_REG09,reg09|(XOSC_64DIV<<6));
		
		reg1c = (float)(fskclk/speed/4*1000)+0.5;
		um2006A_write_reg(UM2006A_REG1C_CDR_STD,reg1c);
	}
	
	reg09 = um2006A_read_reg(UM2006A_REG09);
	reg09 &= UM2006A_REG09_MASK_FSKCLK_SEL;
	switch(reg09)
	{
		case 0x00:
		{
			um2006A_write_reg(UM2006A_REG13,reg13);	
			if(speed>120)
			{
				value = (uint32_t)shape_if_offset_340K[0];
			}
			else
			{
				value = (uint32_t)shape_if_offset_200K[0];
			}
			break;
		}
		case 0x40:
		{
			um2006A_write_reg(UM2006A_REG13,reg13|(1<<2));
			if(speed>120)
			{
				value = (uint32_t)shape_if_offset_340K[1];
			}
			else
			{
				value = (uint32_t)shape_if_offset_200K[1];
			}
			break;
		}
		case 0x80:
		{
			um2006A_write_reg(UM2006A_REG13,reg13|(3<<2));
			if(speed>120)
			{
				value = (uint32_t)shape_if_offset_340K[2];
			}
			else
			{
				value = (uint32_t)shape_if_offset_200K[2];
			}
			break;
		}
		case 0xC0:
		{
			um2006A_write_reg(UM2006A_REG13,reg13|(6<<2));
			if(speed>120)
			{
				value = (uint32_t)shape_if_offset_340K[3];
			}
			else
			{
				value = (uint32_t)shape_if_offset_200K[3];
			}
			break;
		}
	}
	
	reg13 = um2006A_read_reg(UM2006A_REG13);
	reg13 &= ~ UM2006A_REG13_MASK_SHAPE_IF_OFFSET_BIT25_24;
	
	um2006A_write_reg(UM2006A_REG10_SHAPE_IF_OFFSET_BIT7_0,value);
	um2006A_write_reg(UM2006A_REG11_SHAPE_IF_OFFSET_BIT15_8,value>>8);
	um2006A_write_reg(UM2006A_REG12_SHAPE_IF_OFFSET_BIT23_16,value>>16);
	um2006A_write_reg(UM2006A_REG13,reg13|(value>>24));
}

/***********************************************************************************************************
 * Function		: um2006A_set_ook_speed
 * Description	: 设置OOK数据率
 * Input		: float speed  单位KHz
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_ook_speed(float speed)
{
	uint8_t reg1c = 0;
	uint8_t reg0E = um2006A_read_reg(UM2006A_REG0E);
	reg0E &= ~UM2006A_REG0E_MASK_ADC_CLK_SEL;
	
	if(speed > 40)
	{
		return;
	}
	else if(speed > 20)
	{
		um2006A_write_reg(UM2006A_REG0E,reg0E | ADC_1600KHz<<4);
		reg1c = 1600/(speed);
		um2006A_write_reg(UM2006A_REG1C_CDR_STD,reg1c);
	}
	else if(speed >10)
	{
		um2006A_write_reg(UM2006A_REG0E,reg0E | ADC_800KHz<<4);
		reg1c = 800/(speed);
		um2006A_write_reg(UM2006A_REG1C_CDR_STD,reg1c);
	}
	else if(speed >5)
	{
		um2006A_write_reg(UM2006A_REG0E,reg0E | ADC_400KHz<<4);
		reg1c = 400/(speed);
		um2006A_write_reg(UM2006A_REG1C_CDR_STD,reg1c);
	}
	else if(speed > 0)
	{
		um2006A_write_reg(UM2006A_REG0E,reg0E | ADC_200KHz<<4);
		reg1c = 200/(speed);
		um2006A_write_reg(UM2006A_REG1C_CDR_STD,reg1c);
	}
}

/***********************************************************************************************************
 * Function		: um2006A_set_manchester_mode
 * Description	: 设置曼彻斯特模式
 * Input		: em_enable_t enable,e_manchst_mode mode
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_manchester_mode(em_enable_t enable,em_manchst_mode_t mode)
{
	uint8_t value = um2006A_read_reg(UM2006A_REG28);
	value &= ~UM2006A_REG28_MANCHST_TYPE;

	if(enable)
	{	
		switch(mode)
		{
			case MANCHEST_NONE:                                                                   			/* 不进行编码 */
			{
				um2006A_write_reg(UM2006A_REG28,value|UM2006A_REG28_MANCHST_TYPE_NONE);
				break;
			}	
			case HIGH_01_LOW_10:                                                                 			/* 上升沿为1，下降沿为0 */
			{
				um2006A_write_reg(UM2006A_REG28,value|UM2006A_REG28_MANCHST_TYPE_HIGH_01_LOW_10);
				break;
			}
			case HIGH_10_LOW_01:                                                                 			/* 上升沿为0，下降沿为1 */
			{
				um2006A_write_reg(UM2006A_REG28,value|UM2006A_REG28_MANCHST_TYPE_HIGH_10_LOW_01);
				break;
			}
			case DIFFERENTIAL:                                                                   			/* 差分曼彻斯特编码 */
			{
				um2006A_write_reg(UM2006A_REG28,value|UM2006A_REG28_MANCHST_TYPE_DIFFERENTIAL);
				break;
			}
			default:
			{
				break;
			}
		}
	}
	else
	{
		um2006A_write_reg(UM2006A_REG28,value|UM2006A_REG28_MANCHST_TYPE_NONE);
	}
}

/***********************************************************************************************************
 * Function		: um2006A_dig_reset
 * Description	: 数字电路复位
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_reset_digit(void)
{
	um2006A_write_reg(UM2006A_REG51_WORK_CMD,CMD_IDLE);		
	um2006A_write_reg(UM2006A_REG52, UM2006A_REG52_MASK_DIG_RESET);   
}

/***********************************************************************************************************
 * Function		: um2006A_get_pjdvalid
 * Description	: 获取PJD匹配标志位
 * Input		: none
 * Output		: none
 * Return		: uint8_t  1：成功  0：失败
 ***********************************************************************************************************/
uint8_t um2006A_get_pjdvalid(void)
{
	return um2006A_read_reg(UM2006A_REG40)&UM2006A_REG40_MASK_PJD_VALID;
}

/***********************************************************************************************************
 * Function		: um2006A_get_rssivalid
 * Description	: 获取RSSI匹配标志位
 * Input		: none
 * Output		: none
 * Return		: uint8_t  1：成功  0：失败
 ***********************************************************************************************************/
uint8_t um2006A_get_rssivalid(void)
{
	return um2006A_read_reg(UM2006A_REG40)&UM2006A_REG40_MASK_RSSI_VALID;
}

/***********************************************************************************************************
 * Function		: um2006A_get_syncvalid
 * Description	: 获取SYNC匹配标志位，数据包模式下有效
 * Input		: none
 * Output		: none
 * Return		: uint8_t  1：成功  0：失败
 ***********************************************************************************************************/
uint8_t um2006A_get_syncvalid(void)
{ 
	return um2006A_read_reg(UM2006A_REG40)&UM2006A_REG40_MASK_SYNC_VALID;
}

/***********************************************************************************************************
 * Function		: um2006A_get_rxbyte_done
 * Description	: 获取接收数据完成标志，数据包模式下有效
 * Input		: none
 * Output		: none
 * Return		: uint8_t  1：成功  0：失败
 ***********************************************************************************************************/
uint8_t um2006A_get_rxbyte_done(void)
{
	return um2006A_read_reg(UM2006A_REG44)&UM2006A_REG44_RXBYTE_DONE;
}

/***********************************************************************************************************
 * Function		: um2006A_maskdem_sel
 * Description	: GPIO输出demod_bitdata信号时，maskdem_sel选择PJD_valide有效或sync_valid有效或rssi_valid后再输出demod_bit 
 * Input		: em_maskdem_sel_t maskdem_sel
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_maskdem_sel(em_maskdem_sel_t maskdem_sel)
{
	uint8_t reg09 = um2006A_read_reg(UM2006A_REG09);
	reg09 &= ~UM2006A_REG09_MASK_MASKDEM_SEL;
	
	switch(maskdem_sel)
	{
		case MASKDEM_NONE:                                           										/* 不屏蔽 */
		{
			um2006A_write_reg(UM2006A_REG09,reg09|MASKDEM_NONE<<3);
			break;
		}
		case RSSI_VALID:                                            										/* RSSI匹配后输出demod_bit */ 
		{
			um2006A_write_reg(UM2006A_REG09,reg09|RSSI_VALID<<3);
			break;
		}
		case PJD_VALID:                                             										/* PJD匹配后输出demod_bit */ 
		{
			um2006A_write_reg(UM2006A_REG09,reg09|PJD_VALID<<3);          
			break;
		}
		case SYNC_VALID:                                            										/* SYNC匹配后输出demod_bit */ 
		{
			um2006A_write_reg(UM2006A_REG09,reg09|SYNC_VALID<<3);
			break;
		}
		default:
		{
			break;
		}
	}
}

/***********************************************************************************************************
 * Function		: um2006A_set_dutycycle_mode
 * Description	: duty cycle模式选择
 * Input		: em_duc_sel_t duc_sel
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_dutycycle_mode(em_duc_sel_t duc_sel)
{
	uint8_t reg03 = um2006A_read_reg(UM2006A_REG03);
	reg03 &= ~UM2006A_REG03_MASK_DUC_SEL;
	
	switch(duc_sel)
	{
		case DUC_DISABLE:                                                        							/* 关闭duty cycle */
		{
			um2006A_write_reg(UM2006A_REG03,reg03|DUC_DISABLE<<6);
			break;
		}
		case DUC_2S_1MS:                                                         							/* 2S_1MS */
		{
			um2006A_write_reg(UM2006A_REG03,reg03|DUC_2S_1MS<<6);
			break;
		}
		case DUC_4S_1MS:                                                         							/* 4S_1MS */
		{
			um2006A_write_reg(UM2006A_REG03,reg03|DUC_4S_1MS<<6);
			break;
		}
		case DUC_8S_1MS:                                                         							/* 8S_1MS */
		{
			um2006A_write_reg(UM2006A_REG03,reg03|DUC_8S_1MS<<6);
			break;
		}
		default:
		{
			break;
		}
	}
}

/***********************************************************************************************************
 * Function		: um2006A_set_pjd_len
 * Description	: 设置接收PJD信号翻转检测次数，次数为len*4
 * Input		: uint8_t len
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_pjd_len(uint8_t len)
{
	uint8_t reg13 = um2006A_read_reg(UM2006A_REG13);
	reg13 &= ~UM2006A_REG13_MASK_PJD_LEN;
	
	if(len > 7)
	{
		len = 7;
	}
	um2006A_write_reg(UM2006A_REG13,reg13|len<<6);
}

/***********************************************************************************************************
 * Function		: um2006A_set_syncdata
 * Description	: 设置同步字及长度
 * Input		: uint32_t data , em_synclen_t len 0：16bit  1:32bit
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void um2006A_set_syncword(uint32_t data,em_syncword_len_t len)
{
	uint8_t reg17 = um2006A_read_reg(UM2006A_REG17);
	
	um2006A_write_reg(UM2006A_REG18_SYNC_DATA_BIT7_0,data);
	um2006A_write_reg(UM2006A_REG19_SYNC_DATA_BIT15_8,data>>8);
	um2006A_write_reg(UM2006A_REG1A_SYNC_DATA_BIT23_16,data>>16);
	um2006A_write_reg(UM2006A_REG1B_SYNC_DATA_BIT31_24,data>>24);
	
	if(len == SYNCWORD_LEN_16BIT)
	{
		um2006A_write_reg(UM2006A_REG17,reg17 & ~UM2006A_REG17_MASK_SYNC_LEN);
	}
	else if(len == SYNCWORD_LEN_32BIT)
	{
		um2006A_write_reg(UM2006A_REG17,reg17 | UM2006A_REG17_MASK_SYNC_LEN);
	}
}
