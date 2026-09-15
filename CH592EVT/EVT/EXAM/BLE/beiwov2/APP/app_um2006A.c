/***********************************************************************************************************
 * Copyright (c)  2023 - 2024, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : app_um2006A.c
 * Description : app_um2006A driver source file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2024-10-26
 ***********************************************************************************************************/
#include "app_um2006A.h"
#include "um2006A.h"
#include "um2006A_hal.h"
#include "radio.h"
#include "gattprofile.h"
#include "app_uart.h"
#include "CH59xBLE_LIB.h"
#include <stdbool.h>
#include "feistel_al.h"
#include "bleencrypt.h"
//#include "CH59xBLE_ROM.h"
#include "ch59xBLE_LIB.h"
tmosTaskID  temperature_task_idx = 2;



typedef struct{
    uint16_t _huminityvalue;
    int16_t _tempvalue;
}temphumnitytype;
temphumnitytype resbf[5] = {};//save five minutes data
uint8_t resbfptr = 0;//data pointer


#define waitdata   (0x0001<<0)
/***********************************************************************************************************
 * Function		: app_um2006A_init
 * Description	: app_um2006A_init
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void app_um2006A_init(void)
{
//	radio_init();
//	um2006A_into_rx_bpfcal();
//
//	app_ev1527_init();


//    radio_init();
//    um2006A_clear_validall();
//    um2006A_hal_clear_flag();
//    app_uart_clear_data();


    radio_init();

    um2006A_clear_validall();
    um2006A_hal_clear_flag();
}

uint16_t is_all_zero_or(const uint8_t *buf, uint8_t len)
{
    uint8_t acc = 0;
    for (uint8_t i = 0; i < len; i++) acc |= buf[i];
    return acc;
}

//extern int LL_Encrypt(uint8_t *key16, uint8_t *in16, uint8_t *out16);

bool encrypt_frame_ecb_inplace(uint8_t *buf, size_t len, const uint8_t key16[16]) {
    if (!buf || !key16 || (len==0) || (len % FRAME_BLOCK_SIZE)) return false;
    uint8_t blk[FRAME_BLOCK_SIZE];
    for (size_t off=0; off<len; off+=FRAME_BLOCK_SIZE) {
        memcpy(blk, buf+off, FRAME_BLOCK_SIZE);
        if (LL_Encrypt((uint8_t*)key16, blk, buf+off) != 0) return false;
    }
    return true;
}

static const uint8_t APP_AES_KEY16[16] = {
    0x91,0x4E,0x03,0xB7,0xC2,0x5A,0x88,0x1D,
    0xF4,0x60,0x7B,0x2E,0xA9,0x17,0x6C,0x55
};
uint8_t ble_send_bf[500];

/***********************************************************************************************************
 * Function		: app_um2006A_run
 * Description	: app_um2006A_run
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/

uint8_t sensorres[150];
uint8_t timeoutcnt = 10;

void app_um2006A_run(void)
{
//	uint8_t recv_data[64] = {0};
//	uint8_t len = 0;
//	if(SUCCESS == app_ev1527_recv_data(recv_data,&len))														/* ����ĵ�һ���ֽ�Ϊ���ݳ��� */
//	{
//	    PRINT("Recv Data [len:%d] : ",len);
//	     for(uint8_t i=0;i<len;i++)
//	     {
//	         PRINT("0x%02X ",recv_data[i]);
//	     }
//	     PRINT("\r\n");
//
//
//	}else{
//
//	    PRINT("ERROR RECV!\r\n");
//	}

//    uint8_t rcv_data[64] = {0};
//    uint8_t len = 0;
//    if(0 == app_uart_recv_bytes(rcv_data,&len))
//    {
//        um2006A_into_rx_bpfcal();
//        um2006A_clear_validall();
//        app_uart_clear_data();
//
//        PRINT("Recv Data [len:%d] : ",len);
//        for(uint8_t i=0;i<len;i++)
//        {
//            PRINT("0x%02X ",rcv_data[i]);
//        }
//        PRINT("\r\n");
//    }

    uint8_t recv_buff[30] = {0};
    uint8_t resbf[30] = {0};
    uint8_t len = 0;
    uint8_t get_data_state = um2006A_read_reg(UM2006A_REG30);
    uint16_t uid_sum = 0;
    uint16_t tempervalue = 0;
    uint16_t humivalue = 0;
    uint8_t tempsensoruid[4];
    //PRINT("get_data_state :[%d] \r\n: ",get_data_state);
    extern uint8_t acaddr[6];
    size_t needbytes = 0;
    extern uint8_t newValue[SIMPLEPROFILE_CHAR3_LEN];
    uint8_t i;
   extern uint8_t bind_device_id[120][4];
   extern uint8_t bind_device_bf[500];
   extern uint8_t bind_device_cnt;

    if(SUCCESS == radio_recv_data(recv_buff,&len))
    {

//        PRINT("Recv Data [len:%d] : ",len);
//        for(uint8_t i=0;i<len;i++)
//        {
//            PRINT("0x%02X ",recv_buff[i]);
//        }
//        PRINT("\r\n");


        if(true ==  decode_frame10(recv_buff,tempsensoruid,&tempervalue,&humivalue)){
            GPIOA_SetBits(GPIO_Pin_13);
            timeoutcnt = 100;
            resbf[0] = tempsensoruid[0];
            resbf[1] = tempsensoruid[1];
            resbf[2] = tempsensoruid[2];
            resbf[3] = tempsensoruid[3];

            resbf[4] = 0xff & (tempervalue >> 8);
            resbf[5] = tempervalue & 0xff;
            resbf[6] = 0xff & (humivalue >> 8);
            resbf[7] = humivalue & 0xff;

            PRINT("sensorid : ");
            for(uint8_t i=0;i<4;i++)
            {
                PRINT("0x%02X ",resbf[i]);
            }
            PRINT("\r\n");

            for(uint8_t i = 0; i < bind_device_cnt;i++){
                if((bind_device_id[i][0] == resbf[0])&&(bind_device_id[i][1] == resbf[1])&&(bind_device_id[i][2] == resbf[2])&&(bind_device_id[i][3] == resbf[3])){
                    sensorres[i*8+0] = resbf[0];
                    sensorres[i*8+1] = resbf[1];
                    sensorres[i*8+2] = resbf[2];
                    sensorres[i*8+3] = resbf[3];

                    /* FR-401: 每设备记录 = [id(4) | humidity_be(2) | temperature_be(2)]
                     * (IC-001 6 节; 传感器实参修正后 tempervalue=温度, humivalue=湿度)
                     * 原 resbf[4..5]=tempervalue(温度), resbf[6..7]=humivalue(湿度)
                     * → 交换为 [湿度 | 温度], 与 Android parsePlainFrame(u16be@4/s16be@6) 一致 */
                    sensorres[i*8+4] = resbf[6];   /* 湿度高字节 */
                    sensorres[i*8+5] = resbf[7];   /* 湿度低字节 */
                    sensorres[i*8+6] = resbf[4];   /* 温度高字节 */
                    sensorres[i*8+7] = resbf[5];   /* 温度低字节 */
                }
            }


//�ж�id���Ƿ��б������������������������ô�ͽ����ϴ������û�б�����������ô������ݷ�������ͨ�������ϴ�
            //�ҵ�����Э�������λ�ã�Ȼ���ٽ����ϴ�
            needbytes = build_padded_frame_blocks(acaddr,bind_device_cnt,sensorres,ble_send_bf,sizeof(ble_send_bf));
//            for(uint8_t i=0;i<needbytes;i++){
//                PRINT("0x%02X ",ble_send_bf[i]);
//
//            }
//            PRINT("will encrypt>>>\r\n");

            if(true == encrypt_frame_ecb_inplace(ble_send_bf,needbytes,APP_AES_KEY16)){
                SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR2, SIMPLEPROFILE_CHAR2_LEN, ble_send_bf);
//                for(uint8_t i=0;i<needbytes;i++){
//                    PRINT("0x%02X ",ble_send_bf[i]);
//
//                }
//                PRINT("encrypt over!!!\r\n");
            }
        }else{
            if(timeoutcnt > 0){
                timeoutcnt--;
            }else{
                memset(sensorres,0,sizeof(sensorres));
                needbytes = build_padded_frame_blocks(acaddr,bind_device_cnt,sensorres,ble_send_bf,sizeof(ble_send_bf));
                if(true == encrypt_frame_ecb_inplace(ble_send_bf,needbytes,APP_AES_KEY16)){
                    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR2, SIMPLEPROFILE_CHAR2_LEN, ble_send_bf);
                    PRINT("no sensors!!!\r\n");
                }
            }
        }

        GPIOA_ResetBits(GPIO_Pin_13);
    }


   len = newValue[1]*4+2;
   if(newValue[0] == 0xA1){
       for(i = 0; i < SIMPLEPROFILE_CHAR3_LEN;i++){
           if(bind_device_bf[10+i] != newValue[i+2]){
               memset(bind_device_bf,0,sizeof(bind_device_bf));
               bind_device_bf[0] = 0x55;
               bind_device_cnt = bind_device_bf[1] = newValue[1];
               memcpy(&bind_device_bf[10],&newValue[2],(len-2));
               for(i = 0;i < newValue[1];i++){
                   bind_device_id[i][0] = bind_device_bf[10 + i*4];
                   bind_device_id[i][1] = bind_device_bf[11 + i*4];
                   bind_device_id[i][2] = bind_device_bf[12 + i*4];
                   bind_device_id[i][3] = bind_device_bf[13 + i*4];
              }
              EEPROM_ERASE(0,150);
              EEPROM_WRITE(0, bind_device_bf, 150);
              break;
           }
       }
       newValue[0] = 0;
   }



//   PRINT("newvalue = %d %d\r\n",newValue[0],newValue[1] );
//   for(i = 0; i < len;i++){
//       PRINT("%x ",newValue[i]);
//   }
//   PRINT("\r\n");
}





static uint16_t get_temperature_task_process_event( uint8_t task_id, uint16_t events ) {
    if(events & waitdata) {
        //PRINT("%x %x %x %d %d\r\n",um2006A_read_reg(0x40),um2006A_read_reg(0x43),um2006A_read_reg(0x44),um2006A_read_reg(0x45),um2006A_read_reg(0x46));
        //PRINT("%x\r\n",UART1_RecvByte());

        app_um2006A_run();
        um2006A_write_reg(0x52,3);
        tmos_start_task(temperature_task_idx,waitdata,1600*10);

        return (events ^ waitdata);
    }

    return 0;
}



void get_temperature_task_init( void )
{
    app_um2006A_init();

    //ע��task id,ͬ�°Ѹ�task��event������������ȥ
    temperature_task_idx  = TMOS_ProcessEventRegister( get_temperature_task_process_event );
    //������ʼһ��event
    //tmos_set_event(temperature_task_id,DEMO_TASK_TMOS_EVT_TEST_1);
    //��ʼһ����ʱevent,1s�����,��ǰ���ֻ�����һ��event
    //������event������ȥ����event,�����Ǳ��task��,Ҳ�����ǵ�ǰtask��event`
    tmos_start_task(temperature_task_idx,waitdata,160);
//    PRINT("lalalalala....\r\n");
//    tmos_start_reload_task(temperature_task_id,start_measure,1);
}



