/***********************************************************************************************************
 * Copyright (c)  2023 - 2024, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : um2006A_defs.h
 * Description : um2006A_defs header file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2024-10-26
 ***********************************************************************************************************/
#ifndef __UM2006A_DEFS_H__
#define __UM2006A_DEFS_H__

#define UM2006A_RCCLOCK									32.0
#define UM2006A_XOSC									26

#define UM2006A_REG00   								0x00
#define UM2006A_REG00_MASK_SYNON_DLY      				0xC0
#define UM2006A_REG00_MASK_RCBPF_CAL_TIME 				0x30
#define UM2006A_REG00_MASK_CAL_WAIT_TIME  				0x0C
#define UM2006A_REG00_MASK_VCO_CAL_TIME   				0x03

#define UM2006A_REG01   								0x01
#define UM2006A_REG01_MASK_WKUP_DLY           			0xC0
#define UM2006A_REG01_MASK_SDM_EN             			0x20
#define UM2006A_REG01_MASK_FSMCLK_SEL         			0x10
#define UM2006A_REG01_MASK_RC32K_CAL_TIME_T3  			0x0C
#define UM2006A_REG01_MASK_RC32K_WAIT_TIME_T2 			0x03

#define UM2006A_REG02_RC32K_CAL_STD   					0x02  												/* RC32K时钟校准目标计数器值，晶振时钟/16/32K，是否×N，取决于0e[3:0]worclk_sel的选择 */

#define UM2006A_REG03   								0x03
#define UM2006A_REG03_MASK_DUC_SEL   					0xC0
#define UM2006A_REG03_MASK_RIB_SEL   					0x30
#define UM2006A_REG03_MASK_FB_SEL    					0x0C
#define UM2006A_REG03_MASK_FSKDEM_EN 					0x02
#define UM2006A_REG03_MASK_UART_EN   					0x01

#define UM2006A_REG04_FREQ_FRAC_BIT7_0   				0x04

#define UM2006A_REG05_FREQ_FRAC_BIT15_8  				0x05

#define UM2006A_REG06   								0x06
#define UM2006A_REG06_MASK_FREQ_NDIV_BIT5_0   			0xFC
#define UM2006A_REG06_MASK_FREQ_FRAC_BIT17_16 			0x03

#define UM2006A_REG07   								0x07
#define UM2006A_REG07_MASK_RC32K_EN  					0x80
#define UM2006A_REG07_MASK_RC32K_FTRIM_SEL  			0x40
#define UM2006A_REG07_MASK_AUTO_CAL_BPF_EN  			0x20
#define UM2006A_REG07_MASK_AUTO_CAL_VCO_EN  			0x10
#define UM2006A_REG07_MASK_VCO_FB           			0x0F

#define UM2006A_REG08_RC32K_FTRIM   					0x08

#define UM2006A_REG09   								0x09
#define UM2006A_REG09_MASK_FSKCLK_SEL  					0xC0
#define UM2006A_REG09_MASK_SPI4W_EN    					0x20
#define UM2006A_REG09_MASK_MASKDEM_SEL 					0x18
#define UM2006A_REG09_MASK_CLKOUT_SEL  					0x07

#define UM2006A_REG0A_WOR_TIMER_SET_BIT7_0    			0x0A  												/*  WOR，Sleep计数器 */
#define UM2006A_REG0B_WOR_TIMER_SET_BIT15_8   			0x0B  												/*  WOR，Sleep计数器 */
												
#define UM2006A_REG0C_WOR_RTXTIMER_SET_BIT7_0   		0x0C  												/*  WOR，Rx_T1计数器 */
#define UM2006A_REG0D_WOR_RTXTIMER_SET_BIT15_8  		0x0D  												/*  WOR，Rx_T1计数器 */

#define UM2006A_REG0E   								0x0E
#define UM2006A_REG0E_MASK_RX_DATA_INV  				0x80
#define UM2006A_REG0E_MASK_ADC_CLK_SEL  				0x70
#define UM2006A_REG0E_MASK_WOR_CLK_SEL  				0x0F  												/* WOR计数器分频选择 */

#define UM2006A_REG0F   								0x0F
#define UM2006A_REG0F_MASK_HDB_CIC_SEL  				0xE0
#define UM2006A_REG0F_MASK_BPF_CAL_TUNE 				0x1F

#define UM2006A_REG10_SHAPE_IF_OFFSET_BIT7_0   			0x10
#define UM2006A_REG11_SHAPE_IF_OFFSET_BIT15_8   		0x11
#define UM2006A_REG12_SHAPE_IF_OFFSET_BIT23_16  		0x12

#define UM2006A_REG13   								0x13
#define UM2006A_REG13_MASK_PJD_LEN 						0xE0
#define UM2006A_REG13_MASK_SHAPE_SCALE 					0x1C
#define UM2006A_REG13_MASK_SHAPE_IF_OFFSET_BIT25_24 	0x03

#define UM2006A_REG14   								0x14
#define UM2006A_REG14_MASK_DC_ALP 						0xC0
#define UM2006A_REG14_MASK_DC_PP_TH 					0x3F

#define UM2006A_REG15_DC_MAX_TH   						0x15

#define UM2006A_REG16_DC_MIN_TH   						0x16

#define UM2006A_REG17   								0x17
#define UM2006A_REG17_MASK_SYNC_EN     					0x80
#define UM2006A_REG17_MASK_SYNC_LEN    					0x40
#define UM2006A_REG17_MASK_PKT_CLK_SEL 					0x20
#define UM2006A_REG17_MASK_UPCIC_EN    					0x10
#define UM2006A_REG17_MASK_DC_LEN      					0x0F

#define UM2006A_REG18_SYNC_DATA_BIT7_0   				0x18
#define UM2006A_REG19_SYNC_DATA_BIT15_8  				0x19
#define UM2006A_REG1A_SYNC_DATA_BIT23_16 				0x1A
#define UM2006A_REG1B_SYNC_DATA_BIT31_24 				0x1B

#define UM2006A_REG1C_CDR_STD   						0x1C

#define UM2006A_REG1D   								0x1D
#define UM2006A_REG1D_MASK_AUTO_CDR_STD  				0x80
#define UM2006A_REG1D_MASK_AGC_LOCK_EN   				0x40
#define UM2006A_REG1D_MASK_DC_FORCE_EN   				0x20
#define UM2006A_REG1D_MASK_DELTA_DCAVE   				0x1F

#define UM2006A_REG1E   								0x1E
#define UM2006A_REG1E_MASK_AGC_CHANGE_CNT 				0xF0
#define UM2006A_REG1E_MASK_RM_G_NUM       				0x0F

#define UM2006A_REG1F_AGC_HI_THR   						0x1F

#define UM2006A_REG20_AGC_LO_THR   						0x20

#define UM2006A_REG21_DELTA_RSSI   						0x21

#define UM2006A_REG22   								0x22
#define UM2006A_REG22_MASK_AGC_LOCK_SEL     			0x80
#define UM2006A_REG22_MASK_AGC_DAT_SRC      			0x60
#define UM2006A_REG22_MASK_AGC_RSSI_LOW_THR 			0x1F

#define UM2006A_REG23   								0x23
#define UM2006A_REG23_MASK_AGC_STEP_SEL   				0x80
#define UM2006A_REG23_MASK_AGC_EN_SRC     				0x40
#define UM2006A_REG23_MASK_AGC_FORCE_EN   				0x20
#define UM2006A_REG23_MASK_AGC_FORCE_CTRL 				0x1F

#define UM2006A_REG24   								0x24
#define UM2006A_REG24_MASK_SDO_OSEL           			0xF0
#define UM2006A_REG24_MASK_AGC_END_GAIN_SEL   			0x0C
#define UM2006A_REG24_MASK_AGC_START_GAIN_SEL 			0x03

#define UM2006A_REG25   								0x25
#define UM2006A_REG25_MASK_ADC_CLK_PHI_SEL 				0x80
#define UM2006A_REG25_MASK_AGC_DLY_CNT     				0x60
#define UM2006A_REG25_MASK_DCMEAN_MODE     				0x10
#define UM2006A_REG25_MASK_INVALID_AV_CNT  				0x0F

#define UM2006A_REG26_RXBYTE_LEN   						0x26

#define UM2006A_REG27   								0x27
#define UM2006A_REG27_MASK_INVALID_AV_CLR_PKT  			0x80
#define UM2006A_REG27_MASK_FIR_BYPASS          			0x40
#define UM2006A_REG27_MASK_RSSI_SEL            			0x38
#define UM2006A_REG27_MASK_PJD_ET_SEL          			0x06
#define UM2006A_REG27_MASK_DUC_CMD_SEL         			0x01

#define UM2006A_REG28   								0x28
#define UM2006A_REG28_MANCHST_TYPE  					0x60
#define UM2006A_REG28_MANCHST_TYPE_NONE 				0x00
#define UM2006A_REG28_MANCHST_TYPE_HIGH_01_LOW_10 		0x04
#define UM2006A_REG28_MANCHST_TYPE_HIGH_10_LOW_01 		0x08
#define UM2006A_REG28_MANCHST_TYPE_DIFFERENTIAL 		0x0C
#define UM2006A_REG28_EOM_EN							0x01

#define UM2006A_REG29   								0x29
#define UM2006A_REG29_BPFIF_SEL							0x08
#define UM2006A_REG29_BPFBW_SEL							0x03

#define UM2006A_REG2A   								0x2A
#define UM2006A_REG2B   								0x2B
#define UM2006A_REG2C   								0x2C
#define UM2006A_REG2D   								0x2D
#define UM2006A_REG2E   								0x2E
#define UM2006A_REG2F   								0x2F

#define UM2006A_REG30  									0x30
#define UM2006A_REG30_MASK_GPIO1_OSEL  					0xF0
#define UM2006A_REG30_MASK_GPIO0_OSEL  					0x0F

#define UM2006A_REG40   								0x40
#define UM2006A_REG40_MASK_SYNC_VALID 					0x80
#define UM2006A_REG40_MASK_PJD_VALID  					0x40
#define UM2006A_REG40_MASK_RSSI_VALID 					0x20
#define UM2006A_REG40_MASK_PMU_EN     					0x10

#define UM2006A_REG44									0x44
#define	UM2006A_REG44_RXBYTE_DONE						0x80

#define UM2006A_REG46_RSSI								0x46

#define UM2006A_REG48   								0x48
#define UM2006A_REG48_RC32K_CAL_DONE  					0x40

#define UM2006A_REG4A   								0x4A

#define UM2006A_REG50_SLEEP_CMD 						0x50

#define UM2006A_REG51_WORK_CMD  						0x51

#define UM2006A_REG52  									0x52
#define UM2006A_REG52_MASK_DIG_RESET     				0x04  												/* 写1数字复位信号 */
#define UM2006A_REG52_MASK_CLR_RSSI_LOCK 				0x02  												/* 写1清除rssi锁定的信号 */
#define UM2006A_REG52_MASK_CLR_SYNC      				0x01  												/* 写1清除rssi_valid,pjd_valid、sync_valid、rxbyte_done信号 */

#define UM2006A_REG53_UART_BPS  						0x53

#define UM2006A_REG55_RSSI_THR  						0x55

#define UM2006A_REG56  									0x56
#define UM2006A_REG56_MASK_RSSI_VALID_SIG_SEL  			0x80
#define UM2006A_REG56_MASK_WOR_T3_EXT_MODE     			0x08
#define UM2006A_REG56_MASK_WOR_T2_EXT_EN       			0x04
#define UM2006A_REG56_MASK_WOR_EXT_SEL         			0x03
			
#define UM2006A_REG57_WOR_T2_RX_SET_BIT7_0     			0x57
#define UM2006A_REG58_WOR_T2_RX_SET_BIT15_8    			0x58
			
#define UM2006A_REG59_WOR_T3_RX_SET_BIT7_0     			0x59
#define UM2006A_REG5A_WOR_T3_RX_SET_BIT15_8    			0x5A
			
#define UM2006A_REG60_RX_PAYLOAD_ADDR_START    			0x60
#define UM2006A_REG7F_RX_PAYLOAD_ADDR_END      			0x7F

typedef enum
{
	OOK 												= 0,
	FSK 												= 1,
}em_mod_t;

typedef enum
{
	DUC_DISABLE 										= 0,
	DUC_2S_1MS 											= 1,
	DUC_4S_1MS 											= 2,
	DUC_8S_1MS 											= 3,
}em_duc_sel_t;

typedef enum
{
	MASKDEM_NONE 										= 0,
	RSSI_VALID 											= 1,
	PJD_VALID 											= 2,
	SYNC_VALID 											= 3,
}em_maskdem_sel_t;

typedef enum
{
	RXBIT_CLK 											= 0,
	DEMOD_OUT_CE 										= 1,
	CLK_ADC 											= 2,
	CLK_32K 											= 3,	
}em_clkout_t;

typedef enum
{
	XOSC_8DIV 											= 0,
	XOSC_16DIV 											= 1,
	XOSC_32DIV 											= 2,
	XOSC_64DIV 											= 3,
}em_fskclk_sel_t;

typedef enum
{
	ADC_1600KHz 										= 1,
	ADC_800KHz 											= 2,
	ADC_400KHz 											= 3,
	ADC_200KHz 											= 4,
}em_adcclk_sel_t;

typedef enum
{
	SYNCWORD_LEN_16BIT 									= 0,
	SYNCWORD_LEN_32BIT 									= 1,
}em_syncword_len_t;

typedef enum
{
	GPIO_DEMOD_BITDATA 									= 0x00,
	GPIO_CLK_OUT										= 0x01,
	GPIO_RXBIT_DATA										= 0x02,
	GPIO_RXBYTE_DONE									= 0x03,
	GPIO_RSSI_VALID										= 0x04,
	GPIO_PJD_VALID										= 0x05,
	GPIO_SYNC_VALID										= 0x06,
	GPIO_RXBYTE_EN										= 0x07,
	GPIO_UART_TXD										= 0x08,
	GPIO_WOR_EVENT										= 0x09,
	GPIO_SPI_MISO										= 0x0A,
	GPIO_RX_EN											= 0x0B,
	GPIO_LIM_I											= 0x0C,
	GPIO_LIM_Q											= 0x0D,
	GPIO_HIGH_LEVEL										= 0x0E,
	GPIO_LOW_LEVEL										= 0x0F,
}em_gpio_sel;

typedef enum
{
	SDO 												= 0,
	GPIO0 												= 1,
	GPIO1 												= 2,
}em_gpio_pin_sel;

typedef enum
{
	MANCHEST_NONE 										= 0,
	HIGH_01_LOW_10 										= 1,
	HIGH_10_LOW_01 										= 2,
	DIFFERENTIAL 										= 3,
}em_manchst_mode_t;

typedef enum
{
	CMD_WORK  											= 0x00,
	CMD_SLEEP 											= 0x01,
}em_work_cmd;

typedef enum
{
	CMD_IDLE             								= 0x00,
	CMD_RX               								= 0x01,
	CMD_VCO_RX           								= 0x02,
	CMD_RCBPF_VCO_RX     								= 0x04,
	CMD_32K_RCBPF_VCO_RX 								= 0x08,
}em_work_state_cmd;

typedef enum
{
	WOR_EXTEND_NONE 									= 0,
	WOR_EXTEND_RSSI 									= 1,
	WOR_EXTEND_PJD 										= 2,
	WOR_EXTEND_RSSI_PJD 								= 3,
}em_wor_extend_sel;

typedef enum
{
	ALWAYS_RX 											= 0,
	EXTEND_INTO_SLEEP 									= 1,
}em_t3_extmode_t;

#endif
