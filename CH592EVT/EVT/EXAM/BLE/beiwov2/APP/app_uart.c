/***********************************************************************************************************
 * Copyright (c)  2021 - 2022, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : app_uart.c
 * Description : app uart source file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2021-09-24
 ***********************************************************************************************************/
#include "app_uart.h"
#include "um2006A.h"

#include <string.h>

#define APP_UART_TX_PIN								PC1
#define APP_UART_RX_PIN								PD5
uint8_t g_recv_data[1024];
uint16_t g_recv_index = 0;

/***********************************************************************************************************
 * Function		: app_uart1_recv_irq
 * Description	: app_uart1_recv_irq
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
//void app_uart1_recv_irq(void)
//{
//	g_recv_data[g_recv_index++] = uart1_recv_byte();
//}



__INTERRUPT
__HIGH_CODE
void UART0_IRQHandler(void)
{
    volatile uint8_t i;

    switch(UART0_GetITFlag())
    {
        case UART_II_LINE_STAT: // 线路状态错误
        {
//            UART1_GetLinSTA();
            break;
        }

        case UART_II_RECV_RDY: // 数据达到设置触发点
            for(i = 0; i != 1; i++)
            {
                g_recv_data[g_recv_index++] = UART1_RecvByte();
            }
            break;

        case UART_II_RECV_TOUT: // 接收超时，暂时一帧数据接收完成
//            i = UART1_RecvString(RxBuff);
//            UART1_SendString(RxBuff, i);
            break;

        case UART_II_THR_EMPTY: // 发送缓存区空，可继续发送
            break;

        case UART_II_MODEM_CHG: // 只支持串口0
            break;

        default:
            break;
    }
}




/***********************************************************************************************************
 * Function		: app_uart_init
 * Description	: 初始化串口0
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void app_uart_init(void)
{
//	gpio_set_sel(APP_UART_TX_PIN,0x02);
//	gpio_set_sel(APP_UART_RX_PIN,0x03);
//
//	uart1_init(115200);
//	uart1_irq_init(ENABLE,app_uart1_recv_irq);
}

/***********************************************************************************************************
 * Function		: app_uart_send_byte
 * Description	: app_uart_send_byte
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
//void app_uart_send_byte(uint8_t byte)
//{
//	uart1_send_byte(byte);
//}

/***********************************************************************************************************
 * Function		: app_uart_clear_data
 * Description	: app_uart_clear_data
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void app_uart_clear_data(void)
{
	memset(g_recv_data,0,sizeof(g_recv_data));
	g_recv_index = 0;
}

/***********************************************************************************************************
 * Function		: app_uart_recv_bytes
 * Description	: app_uart_recv_bytes
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
int app_uart_recv_bytes(uint8_t *data,uint8_t *len)
{
    PRINT("index rec:%d\r\n",g_recv_index);
	if(g_recv_index != 0)
	{
		uint8_t eom_en = um2006A_read_reg(UM2006A_REG28)&UM2006A_REG28_EOM_EN;
		if(eom_en)
		{
			*len = um2006A_read_reg(UM2006A_REG26_RXBYTE_LEN);
		}
		else
		{
			*len = g_recv_data[0];
		}
		
		if(*len <= g_recv_index)
		{
			for(uint8_t i=0;i<*len;i++)
			{
				data[i] = g_recv_data[i];
			}
			return 0;
		}
	}
	
	return 1;
}
