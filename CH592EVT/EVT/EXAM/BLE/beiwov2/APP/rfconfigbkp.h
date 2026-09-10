/***********************************************************************************************************
 * @File     	: rfconfig.h
 * @Author  	: RFOCT
 * @Version 	: V2.61 
 * @Date    	: 2025-09-23 00:01:05.566
 * @Brief   	: Radio Frequency Online Configuration Tools
 * Copyright (c) 2017-2025 Unicmicro Co.,Ltd.
 * All rights reserved.  
 ***********************************************************************************************************/
#ifndef __RFCONFIG__
#define __RFCONFIG__ 

/***********************************************************************************************************
 * Chip                                                    = UM2006A
 * Frequency                                               = 433.92MHz
 * XTAL Frequency                                          = 26MHz
 * Modulation                                              = FSK/GFSK
 * Data Rate                                               = 10Kbps
 * Deviation                                               = 20KHz
 * AGC                                                     = Enable
 * Mode                                                    = Packet
 * Bandwidth                                               = 240K
 * Data Representation                                     = 1:F-low 0:F-high
 * WOR                                                     = Enable
 * Mode                                                    = PJD
 * PJD Len                                                 = 1 byte
 * T2 Enable                                               = Enable
 * Rx Exit State                                           = Sleep
 * RC32K Calibration                                       = None
 * Sleep Time                                              = 1ms
 * Rx Time T1                                              = 3ms
 * Rx Time T2                                              = 1.5ms
 * Rx Time T3                                              = 3.5ms
 * Preamble Rx Size                                        = 1 byte
 * Sync Len                                                = 32bit
 * Sync Order                                              = MSB
 * SyncWord(Hex)                                           = A7A798F3
 * Manchester                                              = None
 * Data Mode                                               = FIFO
 * Length Enable                                           = Enable
 * Length                                                  = 8byte
 * SDA                                                     = Demod_data
 * GPIO0                                                   = None
 * GPIO1                                                   = RX Byte Done
 * CRC                                                     = 0xE8EF
 ***********************************************************************************************************/

const unsigned char rf_config[][2] = 
{
    {0x00, 0x55},
    {0x01, 0xA7},
    {0x02, 0xC8},
    {0x03, 0xF6},
    {0x04, 0xA6},
    {0x05, 0xC9},
    {0x06, 0x84},
    {0x07, 0xB8},
    {0x08, 0x88},
    {0x09, 0xC0},
    {0x0A, 0x1F},
    {0x0B, 0x00},
    {0x0C, 0x5F},
    {0x0D, 0x00},
    {0x0E, 0xD0},
    {0x0F, 0x60},
    {0x10, 0xC8},
    {0x11, 0x52},
    {0x12, 0x03},
    {0x13, 0x58},
    {0x14, 0x58},
    {0x15, 0xF0},
    {0x16, 0x32},
    {0x17, 0xD7},
    {0x18, 0xF3},
    {0x19, 0x98},
    {0x1A, 0xA7},
    {0x1B, 0xA7},
    {0x1C, 0x28},
    {0x1D, 0x00},
    {0x1E, 0x77},
    {0x1F, 0xC8},
    {0x20, 0x80},
    {0x21, 0xBC},
    {0x22, 0x00},
    {0x23, 0x1F},
    {0x24, 0x00},
    {0x25, 0x27},
    {0x26, 0x08},
    {0x27, 0x03},
    {0x28, 0xF1},
    {0x29, 0x64},
    {0x2A, 0x80},
    {0x2B, 0xB5},
    {0x2C, 0x60},
    {0x2D, 0x35},
    {0x2E, 0xA5},
    {0x2F, 0x00},
    {0x30, 0x30},
    {0x50, 0x00},
    {0x51, 0x04},
    {0x52, 0x00},
    {0x53, 0xE5},
    {0x54, 0x02},
    {0x55, 0xC0},
    {0x56, 0x06},
    {0x57, 0x2F},
    {0x58, 0x00},
    {0x59, 0x6F},
    {0x5A, 0x00},
}; 

#endif 
