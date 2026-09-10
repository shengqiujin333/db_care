/*
 * measure.c
 *
 *  Created on: Feb 22, 2024
 *      Author: kason
 */

#include "measure.h"
#include "CH59xBLE_LIB.h"
#include "sf_i2c.h"
#include "gattprofile.h"
#include "RTC.h"
#include "sleep.h"
tmosTaskID  temperature_task_id = 2;
uint16_t read_temp_period = 1600;//0.625ms*1600=1000ms


void i2c0_sda_pin_out_low(void)
{
    //设置SDA引脚输出低电平
    GPIOB_ResetBits(GPIO_Pin_14);
}

void i2c0_sda_pin_out_high(void)
{
    //设置SDA引脚输出高电平
    GPIOB_SetBits(GPIO_Pin_14);
}

void i2c0_scl_pin_out_low(void)
{
    //设置SCL引脚输出低电平
    GPIOB_ResetBits(GPIO_Pin_15);
}

void i2c0_scl_pin_out_high(void)
{
    //设置SCL引脚输出高电平
    GPIOB_SetBits(GPIO_Pin_15);
}

uint8_t i2c0_sda_pin_read_level(void)
{
    //返回SDA引脚电平状态
    if(GPIOB_ReadPortPin(GPIO_Pin_14)){
        return 1;
    }else{
        return 0;
    }
}

void i2c0_sda_pin_dir_input(void)
{
    //设置SDA引脚输入方向
    GPIOB_ModeCfg(GPIO_Pin_14 , GPIO_ModeIN_Floating);
}

void i2c0_sda_pin_dir_output(void)
{
    //设置SDA引脚输出方向
    GPIOB_ModeCfg(GPIO_Pin_14 , GPIO_ModeOut_PP_5mA);
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
    // Config pin output direction
    GPIOB_ModeCfg(GPIO_Pin_14 | GPIO_Pin_15, GPIO_ModeOut_PP_5mA);
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
int16_t tempvalue = 0;
uint16_t huminityvalue = 0;
uint8_t wait_data_cnt = 0;

uint8_t setdatabf[50];
#define testflag 1

typedef struct{
    uint16_t _huminityvalue;
    int16_t _tempvalue;
}temphumnitytype;
temphumnitytype resbf[5] = {};//save five minutes data
uint8_t resbfptr = 0;//data pointer

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

static uint16_t temperature_task_process_event( uint8_t task_id, uint16_t events ) {
    //event 处理
    uint8_t i,h;
    unsigned long s32x;


    if(events & start_measure) {
        SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR3, setdatabf);
        PRINT("getdata:%d %d\r\n",(int)(((run_param_t*)setdatabf)->temperature_drop * 1000),(int)(((run_param_t*)setdatabf)->huminity_drop));

        for(i = 0;i < 50;i++){
            PRINT("%x ",setdatabf[i]);
        }
        PRINT("\r\n");
        PRINT("-->%d %d\r\n",(int)(((run_param_t*)setdatabf)->temperature_drop),testflag);
        if((((run_param_t*)setdatabf)->temperature_drop > 0.1f * 10) || testflag){
//            bsp_i2c_init();

    //        txbf[0] = 0x70;
    //        txbf[1] = 0xac;
            if(start_once){
                bsp_i2c_init();
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
                GPIOB_ModeCfg(GPIO_Pin_14 , GPIO_ModeIN_PU);
                GPIOB_ModeCfg(GPIO_Pin_15 , GPIO_ModeIN_PU);
            }
//            i2c_write_multi_byte(temp_ptr,0x70,0xac,txbf,2);
            tmos_start_task(temperature_task_id,prepare,90);
            PRINT("start_measure evt test \r\n");

        }else{
            tmos_start_task(temperature_task_id,start_measure,1600*60);
        }
        return (events ^ start_measure);
    }

    if(events & prepare){
        bsp_i2c_init();
        txbf[0] = 0x33;
        txbf[1] = 0x0;
        i2c_write_multi_byte(temp_ptr,0x70,0xac,txbf,2);
        GPIOB_ModeCfg(GPIO_Pin_14 , GPIO_ModeIN_PU);
        GPIOB_ModeCfg(GPIO_Pin_15 , GPIO_ModeIN_PU);
//        txbf[0] = 0x71;
//        i2c_start(temp_ptr);
//        i2c_write_byte(temp_ptr,txbf[0]);
//        for(i = 0; i < 1;i++){//AHT21B 这里是读取7个字节，有一个crc，而aht10却只有6个字节，没有crc
//            rxbf[i] = i2c_read_byte(temp_ptr,0);
//        }
//        i2c_stop(temp_ptr);
//        h = 255;
//        while(h--);
//        PRINT("read_value evt test---- %d\r\n",rxbf[0]);
        tmos_start_task(temperature_task_id,read_value,160*5);
        return (events ^ prepare);
    }

    //event 处理
    if(events & read_value) {
        bsp_i2c_init();
        memset(rxbf,0,sizeof(rxbf));
        txbf[0] = 0x71;
        i2c_start(temp_ptr);
        i2c_write_byte(temp_ptr,txbf[0]);
        for(i = 0; i < 5;i++){//AHT21B 这里是读取7个字节，有一个crc，而aht10却只有6个字节，没有crc
            rxbf[i] = i2c_read_byte(temp_ptr,1);
        }
        rxbf[i] = i2c_read_byte(temp_ptr,0);//要尤其注意，有的aht10的说明书这里有问题，说明书显示这里仍然有ack
        i2c_stop(temp_ptr);
        GPIOB_ModeCfg(GPIO_Pin_14 , GPIO_ModeIN_PU);
        GPIOB_ModeCfg(GPIO_Pin_15 , GPIO_ModeIN_PU);
        //i2c_read_multi_byte(temp_ptr,0x70,0,);
        tmos_set_event(temperature_task_id,calculate_value);
        PRINT("read_value evt test %d\r\n",rxbf[0]);
        return (events ^ read_value);
    }
    //event 处理
    if(events & calculate_value) {
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
        PRINT("res0: %d %d\r\n", (int)tempvalue,(int)huminityvalue);
        resbf[resbfptr]._tempvalue = tempvalue;
        resbf[resbfptr]._huminityvalue = huminityvalue;
        resbfptr++;
        if(resbfptr >= sizeof(resbf)/sizeof(resbf[0])){
            resbfptr = 0;
        }

//        resbf[3] = (tempvalue >> 8) & 0xff;
//        resbf[2] = tempvalue & 0xff;
//        resbf[1] = (huminityvalue >> 8) & 0xff;
//        resbf[0] = huminityvalue & 0xff;
        SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR2, SIMPLEPROFILE_CHAR2_LEN, resbf);
        tmos_set_event(temperature_task_id,wait_data);
        //tmos_start_task(temperature_task_id,start_measure,1600);
        //PRINT("calculate_value evt test \r\n");
        wait_data_cnt = 0;
//        GPIOB_ModeCfg(GPIO_Pin_14 , GPIO_ModeIN_PU);
//        GPIOB_ModeCfg(GPIO_Pin_15 , GPIO_ModeIN_PU);

        return (events ^ calculate_value);
    }

    if(events & wait_data) {
//        wait_data_cnt++;
        //等待数据的到来
//        SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR3, setdatabf);
        //PRINT("setd CHAR3..%d\n",setdatabf[0]);
        uint8_t i;

//        PRINT("getdata:%d %d\r\n",(int)(((run_param_t*)setdatabf)->temperature_drop * 1000),(int)(((run_param_t*)setdatabf)->huminity_drop));
//
//        for(i = 0;i < 50;i++){
//            PRINT("%x ",setdatabf[i]);
//        }
//        PRINT("\r\n");
//        if(wait_data_cnt > 3){
//            wait_data_cnt = 0;


//
//        txbf[0] = 0x70;
//        txbf[1] = 0xe1;
//        i2c_start(temp_ptr);
//        for(i = 0; i < 2;i++){
//            i2c_write_byte(temp_ptr,txbf[i]);
//        }
//        i2c_stop(temp_ptr);

        tmos_start_task(temperature_task_id,start_measure,(1600*60 -160));
//        }else{
//            tmos_start_task(temperature_task_id,wait_data,160);
//        }
        return (events ^ wait_data);
    }



    // Discard unknown events
    return 0;
}



uint8_t measure_step = 0;

static uint16_t temperature_task_process_event2( uint8_t task_id, uint16_t events ) {
    //event 处理
    uint8_t i,h;
    unsigned long s32x;
    char resbf[4] = {};

    while(TRUE){
        switch(measure_step){
        case 0:
            if(start_once){
                bsp_i2c_init();
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
                GPIOB_ModeCfg(GPIO_Pin_14 , GPIO_ModeIN_PU);
                GPIOB_ModeCfg(GPIO_Pin_15 , GPIO_ModeIN_PU);
                CH59x_LowPower((uint32_t)(MS_TO_RTC(60)+ RTC_GetCycle32k()));
            }
            measure_step++;
            break;
        case 1:
            bsp_i2c_init();
            txbf[0] = 0x33;
            txbf[1] = 0x0;
            i2c_write_multi_byte(temp_ptr,0x70,0xac,txbf,2);
            GPIOB_ModeCfg(GPIO_Pin_14 , GPIO_ModeIN_PU);
            GPIOB_ModeCfg(GPIO_Pin_15 , GPIO_ModeIN_PU);
            CH59x_LowPower(MS_TO_RTC(500)+ RTC_GetCycle32k());
            measure_step++;
            break;
        case 2:
            bsp_i2c_init();
            memset(rxbf,0,sizeof(rxbf));
            txbf[0] = 0x71;
            i2c_start(temp_ptr);
            i2c_write_byte(temp_ptr,txbf[0]);
            for(i = 0; i < 5;i++){//AHT21B 这里是读取7个字节，有一个crc，而aht10却只有6个字节，没有crc
                rxbf[i] = i2c_read_byte(temp_ptr,1);
            }
            rxbf[i] = i2c_read_byte(temp_ptr,0);//要尤其注意，有的aht10的说明书这里有问题，说明书显示这里仍然有ack
            i2c_stop(temp_ptr);

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
            PRINT("res0: %d %d\r\n", (int)tempvalue,(int)huminityvalue);
            resbf[3] = (tempvalue >> 8) & 0xff;
            resbf[2] = tempvalue & 0xff;
            resbf[1] = (huminityvalue >> 8) & 0xff;
            resbf[0] = huminityvalue & 0xff;
            SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR2, SIMPLEPROFILE_CHAR2_LEN, resbf);
            GPIOB_ModeCfg(GPIO_Pin_14 , GPIO_ModeIN_PU);
            GPIOB_ModeCfg(GPIO_Pin_15 , GPIO_ModeIN_PU);
            CH59x_LowPower(MS_TO_RTC(500+1000*29)+ RTC_GetCycle32k());
            measure_step = 0;
            return (events ^ start_measure);
        default:
            break;
        }
    }

    // Discard unknown events
    return 0;
}










void temperature_task_init( void ) {
    //注册task id,同事把该task的event处理函数传进去
    temperature_task_id  = TMOS_ProcessEventRegister( temperature_task_process_event );
    //立即开始一个event
    //tmos_set_event(temperature_task_id,DEMO_TASK_TMOS_EVT_TEST_1);
    //开始一个定时event,1s后产生,当前语句只会产生一次event
    //可以在event产生后去开启event,可以是别的task的,也可以是当前task的event`
    tmos_start_task(temperature_task_id,start_measure,1600);
//    tmos_start_reload_task(temperature_task_id,start_measure,1);
}



