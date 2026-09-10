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
//#include "app_i2c.h"
//#include "measure.h"
#include "app_um2006A.h"
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

//        if(tmr_cnt >= 200){
//            tmr_cnt = 0;
//            temp_sensor_flag = 1;
//        }
//        if(tmr_cnt_2_enable == 1){
//            tmr_cnt_2++;
//            if(tmr_cnt_2 >= 1){
//                tmr_cnt_2 = 0;
//                tmr_cnt_2_flag = 1;
//            }
//        }
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

void aes128_test(void)
{
    int i = 0;
    // 加密秘钥 16个字节也就是128 bit
    uint8_t key[16] = {0X00,0X11,0X22,0X33,0X44,0X55,0X66,0X77,0X88,0X99,0XAA,0XBB,0XCC,0XDD,0XEE,0XFF};

    // 需要加密的数据(保证16个字节，不够的自己填充)
    uint8_t source_buf[16] = {0X00,0X01,0X02,0X03,0X04,0X05,0X06,0X07,0X08,0X09,0X0A,0X0B,0X0C,0X0D,0X0E,0X0F};

    // 加密后数据存放区
    uint8_t encrypted_buf[16];

    // 解密后数据存放区
    uint8_t deccrypted_buf[16];

    // 开始加密，加密后的数据存放到encrypted_buf,
    LL_Encrypt( key, source_buf, encrypted_buf );

    // 开始解密，将解密后的数据存到deccrypted_buf,ch579 11us
    LL_Decrypt( key, encrypted_buf, deccrypted_buf );


    //打印原始数据
    PRINT("source:");
    for(i = 0;i < 16;i++) {
        PRINT("0x%02x ",source_buf[i]);
    }
    PRINT("\r\n");
    //打印加密后的数据
    PRINT("encrypte:");
    for(i = 0;i < 16;i++) {
        PRINT("0x%02x ",encrypted_buf[i]);
    }
    PRINT("\r\n");
    //打印解密后的数据
    PRINT("deccrypte:");
    for(i = 0;i < 16;i++) {
        PRINT("0x%02x ",deccrypted_buf[i]);
    }
    PRINT("\r\n");
}

uint8_t magic = 0x55;
uint8_t bind_device_cnt = 0;
uint8_t bind_device_id[120][4];
uint8_t bind_device_bf[480];
uint8_t acaddr[6];
/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main(void)
{
    uint16_t i;
    uint8_t  s;
    uint8_t tempbf[500];
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

    GetMACAddress(acaddr);
    PRINT("serialnu:%x%x%x%x%x%x\n",acaddr[0],acaddr[1],acaddr[2],acaddr[3],acaddr[4],acaddr[5]);

    GPIOA_ModeCfg(GPIO_Pin_13,GPIO_ModeOut_PP_5mA);
    GPIOA_ModeCfg(GPIO_Pin_14,GPIO_ModeOut_PP_5mA);

//    aes128_test();
//    GPIOA_SetBits(GPIO_Pin_14);
//    GPIOA_ModeCfg(GPIO_Pin_15, GPIO_ModeIN_PU);      // RXD-配置上拉输入
//    GPIOA_ModeCfg(GPIO_Pin_14,GPIO_ModeOut_PP_5mA);
//
//    UART0_DefInit();
//
//    GPIOPinRemap(1,RB_PIN_UART0);
//    UART0_ByteTrigCfg(UART_1BYTE_TRIG);
//    UART0_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
//    PFIC_EnableIRQ(UART0_IRQn);


    DelayMs(100);
    //PRINT("init   1\n");


        //PRINT("EEPROM_READ...\n");
        EEPROM_READ(0, tempbf, 150);


//        for(i = 0; i < 150;i++){
//            PRINT("%x ",tempbf[i]);
//        }
        if(tempbf[0] != 0x55){
            s = EEPROM_ERASE(0, EEPROM_BLOCK_SIZE);
            memset(tempbf,0,sizeof(tempbf));
            tempbf[0] = 0x55;
            s = EEPROM_WRITE(0, tempbf, 500);
        }else{
            bind_device_cnt = tempbf[1];
            memcpy(bind_device_bf,&tempbf[10],sizeof(bind_device_bf));
            //得到所有绑定的设备id,从第10个字节开始存储绑定的设备id，解绑的都归为0
            for(i = 0;i < 35;i++){
                bind_device_id[i][0] = tempbf[10 + i*4];
                bind_device_id[i][1] = tempbf[11 + i*4];
                bind_device_id[i][2] = tempbf[12 + i*4];
                bind_device_id[i][3] = tempbf[13 + i*4];
                //PRINT("%x %x %x %x \r\n",tempbf[10 + i*4],tempbf[11 + i*4],tempbf[12 + i*4],tempbf[13 + i*4]);
            }
        }

//        tempbf[10] = 111;
//        tempbf[11] = 112;
//        tempbf[13] = 113;
//        tempbf[14] = 114;
//        s = EEPROM_WRITE(0, tempbf, 240);

//       PRINT("EEPROM_ERASE=%02x\n", s);
//        PRINT("EEPROM_READ...\n");
//        EEPROM_READ(0, TestBuf, 500);
//        for(i = 0; i < 500; i++)
//        {
//            PRINT("%02x ", TestBuf[i]);
//        }
//        PRINT("\n");
//
//        for(i = 0; i < 500; i++)
//            TestBuf[i] = 0x0 + i;
//        s = EEPROM_WRITE(0, TestBuf, 500);
//        PRINT("EEPROM_WRITE=%02x\n", s);
//        PRINT("EEPROM_READ...\n");
//        EEPROM_READ(0, TestBuf, 500);
//        for(i = 0; i < 500; i++)
//        {
//            PRINT("%02x ", TestBuf[i]);
//        }
//        PRINT("\n");

    get_temperature_task_init();

    //PRINT("finish init\n");

    Main_Circulation();
}




/******************************** endfile @ main ******************************/
