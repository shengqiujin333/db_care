/*
 * measure.c
 *
 *  Created on: Feb 22, 2024
 *      Author: kason
 */

#include "measure.h"
#include "sf_i2c.h"
#include "cw32l010_gpio.h"
#include "cw32l010_sysctrl.h"
#include "cw32l010_uart.h"
#include "app_um2005c.h"
#include "encrytogate.h"
void i2c0_sda_pin_out_low(void)
{
    //设置SDA引脚输出低电平
    GPIO_WritePin( CW_GPIOA, GPIO_PIN_4, GPIO_Pin_RESET );
}

void i2c0_sda_pin_out_high(void)
{
    //设置SDA引脚输出高电平
    GPIO_WritePin( CW_GPIOA, GPIO_PIN_4, GPIO_Pin_SET );
}

void i2c0_scl_pin_out_low(void)
{
    //设置SCL引脚输出低电平
    GPIO_WritePin( CW_GPIOA, GPIO_PIN_3, GPIO_Pin_RESET );
}

void i2c0_scl_pin_out_high(void)
{
    //设置SCL引脚输出高电平
    GPIO_WritePin( CW_GPIOA, GPIO_PIN_3, GPIO_Pin_SET );
}

uint8_t i2c0_sda_pin_read_level(void)
{
    //返回SDA引脚电平状态
    if(GPIO_ReadPin(CW_GPIOA, GPIO_PIN_4 )){
        return 1;
    }else{
        return 0;
    }
}

void i2c0_sda_pin_dir_input(void)
{
		GPIO_InitTypeDef GPIO_InitStruct = {0};
    //设置SDA引脚输入方向
    GPIO_InitStruct.Pins =  GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.IT   = GPIO_IT_NONE;
    GPIO_Init( CW_GPIOA, &GPIO_InitStruct);
}

void i2c0_scl_pin_dir_input(void)
{
		GPIO_InitTypeDef GPIO_InitStruct = {0};
    //设置SDA引脚输入方向
    GPIO_InitStruct.Pins =  GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.IT   = GPIO_IT_NONE;
    GPIO_Init( CW_GPIOA, &GPIO_InitStruct);	
}

void i2c0_sda_pin_dir_output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pins = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;

    GPIO_Init( CW_GPIOA, &GPIO_InitStruct);
}


static i2c_dev i2c0_dev = {
    .name                    = "i2c0",
    .speed                   = 100,                      /*! speed:120Hz */
    .port.sda_pin_out_low    = i2c0_sda_pin_out_low,
    .port.sda_pin_out_high   = i2c0_sda_pin_out_high,
    .port.scl_pin_out_low    = i2c0_scl_pin_out_low,
    .port.scl_pin_out_high   = i2c0_scl_pin_out_high,
    .port.sda_pin_read_level = i2c0_sda_pin_read_level,
    .port.sda_pin_dir_input  = i2c0_sda_pin_dir_input,
    .port.sda_pin_dir_output = i2c0_sda_pin_dir_output,
};

static void i2c0_phy_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
		
		   __SYSCTRL_GPIOA_CLK_ENABLE();    //Open GPIOA Clk
	 __SYSCTRL_GPIOB_CLK_ENABLE();    //Open GPIOA Clk

    GPIO_InitStruct.Pins = GPIO_PIN_4| GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;

    GPIO_Init( CW_GPIOA, &GPIO_InitStruct);
		
}

i2c_dev* temp_ptr = NULL;
void bsp_i2c_init(void)
{
    /*! i2c physical layer initialization */
    i2c0_phy_init();

    /*! i2c software layer initialization */
    i2c_init(&i2c0_dev);
    temp_ptr = i2c_obj_find("i2c0");
}

uint8_t txbf[7];
uint8_t rxbf[7];
uint16_t tempvalue = 0;
uint16_t huminityvalue = 0;
uint8_t wait_data_cnt = 0;

uint8_t setdatabf[50];

typedef struct __attribute__((packed)){
    uint8_t dengbei_warn_flag;
    uint8_t yuzhi_warn_flag;
    float temperature_drop;
    float huminity_drop;
    float temperature_max;
    float temperature_min;
    float huminity_max;
    float huminity_min;
    float guard_time;
    float delay_warn_time;
}run_param_t;

uint8_t start_once = 1;
uint8_t sample_flag = 1;
uint8_t send_flag = 1;
uint8_t start_dealonce = 1;

void DebugUART_Close(void)
{
    SYSCTRL_AHBPeriphReset(SYSCTRL_AHB_PERIPH_GPIOA, ENABLE);
    SYSCTRL_AHBPeriphReset(SYSCTRL_AHB_PERIPH_GPIOA, DISABLE);
    SYSCTRL_APBPeriphReset1(SYSCTRL_APB1_PERIPH_UART1, ENABLE);
    SYSCTRL_APBPeriphReset1(SYSCTRL_APB1_PERIPH_UART1, DISABLE);
    
    SYSCTRL_AHBPeriphClk_Enable(SYSCTRL_AHB_PERIPH_GPIOA, DISABLE);
    SYSCTRL_APBPeriphClk_Enable1(SYSCTRL_APB1_PERIPH_UART1, DISABLE);
}


void UartGPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pins = DEBUG_UART_TX_GPIO_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(DEBUG_UART_TX_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.Pins = DEBUG_UART_RX_GPIO_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_Init(DEBUG_UART_RX_GPIO_PORT, &GPIO_InitStructure);

     //UART TX RX ??
    DEBUG_UART_AFTX;
    DEBUG_UART_AFRX; 
}

void UART1_Configure(void)
{
	//UART1?????
	UART_InitTypeDef UART_InitStructure = {0};

		UartGPIO_Configuration();
	
    UART_InitStructure.UART_BaudRate = DEBUG_UART_BaudRate;
    UART_InitStructure.UART_Over = UART_Over_16;
    UART_InitStructure.UART_Source = UART_Source_PCLK;
    UART_InitStructure.UART_UclkFreq = DEBUG_UART_UclkFreq;
    UART_InitStructure.UART_StartBit = UART_StartBit_FE;
    UART_InitStructure.UART_StopBits = UART_StopBits_1;
    UART_InitStructure.UART_Parity = UART_Parity_No ;
    UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
    UART_InitStructure.UART_Mode = UART_Mode_Rx | UART_Mode_Tx;
    UART_Init(DEBUG_UARTx, &UART_InitStructure);
	
}
extern uint32_t hclk ,pclk ;
uint8_t send_data[10] ;

uint16_t temperature_process( void ) {
    //event 处理
    uint8_t i;
    unsigned long s32x;
    //char resbf[4] = {0,0,0,0};
		static uint8_t _tstep = 0;
		uint16_t j;

		if(sample_flag == 0){
			return 0;
		}
	
		switch(_tstep){
			case 0:
				bsp_i2c_init();
				UART1_Configure();
		
				if(start_once){
						start_once = 0;
						txbf[0] = 0x70;
						txbf[1] = 0xe1;
						txbf[2] = 0x08;
						txbf[3] = 0x00;
						i2c_start(temp_ptr);
						for(i = 0; i < 4;i++){
								i2c_write_byte(temp_ptr,txbf[i]);
						}
						i2c_stop(temp_ptr);
				}
								
				_tstep++;
				break;
			case 1:
        txbf[0] = 0x33;
        txbf[1] = 0x0;
        i2c_write_multi_byte(temp_ptr,0x70,0xac,txbf,2);
				_tstep++;
				break;
			case 2:
        memset(rxbf,0,sizeof(rxbf));
        txbf[0] = 0x71;
        i2c_start(temp_ptr);
        i2c_write_byte(temp_ptr,txbf[0]);
        for(i = 0; i < 5;i++){//AHT21B 这里是读取7个字节，有一个crc，而aht10却只有6个字节，没有crc
            rxbf[i] = i2c_read_byte(temp_ptr,1);
        }
        rxbf[i] = i2c_read_byte(temp_ptr,0);//要尤其注意，有的aht10的说明书这里有问题，说明书显示这里仍然有ack
        i2c_stop(temp_ptr);
				_tstep++;
				break;
			case 3:
				s32x=rxbf[1];
        s32x=s32x<<8;
        s32x+=rxbf[2];
        s32x=s32x<<8;
        s32x+=rxbf[3];
        s32x=s32x>>4;
        huminityvalue = 0;
        huminityvalue=(int)((float)(s32x*100.0/1048576.0) * 10);
        s32x=rxbf[3]&0x0F;
        s32x=s32x<<8;
        s32x+=rxbf[4];
        s32x=s32x<<8;
        s32x+=rxbf[5];
        tempvalue = 0;
        tempvalue=(int)(((float)(s32x*200.0/1048576.0)-50) * 10);				
				i2c0_sda_pin_dir_input();
				i2c0_scl_pin_dir_input();
				
				sample_flag = 0;
				_tstep = 0;
				start_dealonce = 0;
				DebugUART_Close();
				//SYSCTRL_GotoDeepSleep();			
				break;
			case 4:
				//send...433 gfsk
				//sleep

						
				//printf("sensor: %d,%d\r\n",huminityvalue,tempvalue);
				//printf("%d %d\r\n",pclk,hclk);
				
//				sample_flag = 0;
//				_tstep = 0;
//				DebugUART_Close();
//				SYSCTRL_GotoDeepSleep();
				break;
			default:
				_tstep = 0;
				break;
		}

    // Discard unknown events
    return 0;
}


uint8_t mcu_uid[10];

void send_data_to_gateway(void)
{
	
	uint8_t *ptr = &mcu_uid[1];
	
	if((start_dealonce == 1)||(send_flag == 0)){
		return;
	}
	
	send_data[0] = ptr[0];
	send_data[1] = ptr[3];
	send_data[2] = ptr[6];
	send_data[3] = ptr[8];
	
	send_data[4] = (huminityvalue >> 8) & 0xff;
	send_data[5] = huminityvalue & 0xff;
	
	send_data[6] = (tempvalue >> 8) & 0xff;
	send_data[7] = tempvalue & 0xff;
	
	
	encode_frame10(mcu_uid, huminityvalue, tempvalue, send_data);
	app_um2005C_send_data(send_data,10);	
	
	send_flag = 0;
	//SYSCTRL_GotoDeepSleep();
	
}

void go_to_sleep(void)
{
	if((send_flag == 0)&&(sample_flag == 0)){
		SYSCTRL_GotoDeepSleep();
	}

}






