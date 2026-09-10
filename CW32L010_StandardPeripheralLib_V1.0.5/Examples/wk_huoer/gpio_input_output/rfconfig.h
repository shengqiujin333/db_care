/***********************************************************************************************************
 * @File     	: rfconfig.h
 * @Author  	: RFOCT
 * @Version 	: V2.61 
 * @Date    	: 2025-06-11 14:33:05.922
 * @Brief   	: Radio Frequency Online Configuration Tools
 * Copyright (c) 2017-2025 Unicmicro Co.,Ltd.
 * All rights reserved.  
 ***********************************************************************************************************/
#ifndef __RFCONFIG__
#define __RFCONFIG__ 

/***********************************************************************************************************
 * Chip                                                    = UM2005C
 * Frequency                                               = 433.92MHz
 * XTAL Frequncy                                           = 26MHz
 * Modulation                                              = GFSK
 * Data Rate                                               = 10Kbps
 * Deviation                                               = 20KHz
 * PA RAMP                                                 = Enable
 * PA Compensation                                         = Disable
 * Tx Matching Network                                     = +13dbm
 * Tx Power                                                = +13dbm
 * Time to Sleep                                           = 32bit
 * CRC                                                     = 0xF363
 ***********************************************************************************************************/

const unsigned char rf_config[][2] = 
{
    {0x00, 0x8E},
    {0x01, 0x04},
    {0x02, 0x7F},
    {0x03, 0x24},
    {0x04, 0x0F},
    {0x05, 0x80},
    {0x06, 0x3F},
    {0x07, 0x18},
    {0x08, 0x39},
    {0x09, 0xC0},
    {0x0A, 0x70},
    {0x0B, 0x00},
    {0x0C, 0x71},
    {0x0D, 0x11},
    {0x0E, 0x00},
    {0x0F, 0x00},
    {0x10, 0x04},
    {0x11, 0xF6},
    {0x12, 0x58},
    {0x13, 0x1C},
    {0x14, 0x2C},
    {0x15, 0x44},
    {0x16, 0x22},
    {0x17, 0x22},
    {0x18, 0x27},
    {0x19, 0x03},
    {0x1A, 0x20},
    {0x1B, 0x50},
    {0x1C, 0x25},
    {0x1D, 0x01},
    {0x1E, 0x00},
    {0x1F, 0x70},
}; 

#endif 
