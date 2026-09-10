/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2020/08/06
 * Description        : 外设从机应用主函数及任务系统初始化
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "HAL.h"
#include "gattprofile.h"
#include "peripheral.h"
#include "CH59x_common.h"
#include "app_i2c.h"
#include "measure.h"

/* I2C Mode Definition */
#define HOST_MODE     0
#define SLAVE_MODE    1


/* I2C Communication Mode Selection */
#define I2C_MODE      HOST_MODE
//#define I2C_MODE      SLAVE_MODE

/* Global define */
#define SIZE            7
#define MASTER_ADDR     0x42
#define SLAVE_ADDR      0x70
/* Global Variable */
uint8_t TxData[SIZE] = {0xAC, 0x33, 0x00};
uint8_t RxData[SIZE];
/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};//3个不同的mac
#endif


uint8_t tmr_cnt = 0;//计数器
uint8_t temp_sensor_flag = 0;//测量温度标志位
uint8_t tmr_cnt_2 = 0; // 转换时间计数器
uint8_t tmr_cnt_2_enable = 0;//tmr_cnt_2 enable 标志
uint8_t tmr_cnt_2_flag = 0;

__INTERRUPT
__HIGH_CODE
void TMR0_IRQHandler(void) // TMR0 定时中断
{
    if(TMR0_GetITFlag(TMR0_3_IT_CYC_END))
    {
        TMR0_ClearITFlag(TMR0_3_IT_CYC_END); // 清除中断标志
        tmr_cnt++;

        if(tmr_cnt >= 200){
            tmr_cnt = 0;
            temp_sensor_flag = 1;
        }
        if(tmr_cnt_2_enable == 1){
            tmr_cnt_2++;
            if(tmr_cnt_2 >= 1){
                tmr_cnt_2 = 0;
                tmr_cnt_2_flag = 1;
            }
        }
    }
}

unsigned char  CheckCrc8(unsigned char *pDat,unsigned char Lenth)
{
unsigned char crc = 0xff, i, j;

    for (i = 0; i < Lenth ; i++)
    {
        crc = crc ^ *pDat;
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x80) crc = (crc << 1) ^ 0x31;
            else crc <<= 1;
        }
                pDat++;
    }
    return crc;
}


uint8_t wake_flag = 0;
uint8_t sleep_cnt = 0;

/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   主循环
 *
 * @return  none
 */
__HIGH_CODE
__attribute__((noinline))
void Main_Circulation()
{
    uint8_t i = 0;
    static uint8_t temp_sensor_step = 0;
    unsigned long s32x;
    float tempvalue = 0.0f;
    float huminityvalue = 0.0f;
    uint32_t flag1 = 0, flag2 = 0;

    while(1)
    {
        //PRINT(" -----------------\r\n");
        TMOS_SystemProcess();
//        if(wake_flag == 1){
//            wake_flag = 0;
//            PRINT("wake...\n");
//            DelayMs(5000);
//        }else{
//            LowPower_Sleep(RB_PWR_RAM24K | RB_PWR_RAM2K |RB_XT_PRE_EN|RB_PWR_EXTEND ); //只保留24+2K SRAM 供电
//            HSECFG_Current(HSE_RCur_100);                 // 降为额定电流(低功耗函数中提升了HSE偏置电流)
//        }


    }

}


/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main(void)
{

#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif

    SetSysClock(CLK_SOURCE_PLL_24MHz);
    HSECFG_Capacitance(HSECap_12p);
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif
#ifdef DEBUG
    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif

    LClk32K_Select(Clk32K_LSI);
    PRINT("%s\n", VER_LIB);
//    GPIOB_ModeCfg(GPIO_Pin_14 | GPIO_Pin_15, GPIO_ModeIN_Floating);

    CH59x_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    Peripheral_Init();

    DelayMs(100);
//    TMR0_TimerInit(FREQ_SYS / 10);         // 设置定时时间 100ms
//    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END); // 开启中断
//    PFIC_EnableIRQ(TMR0_IRQn);
//    bsp_i2c_init();
    temperature_task_init();
    PRINT("finish init\n");

    Main_Circulation();
}




/******************************** endfile @ main ******************************/
