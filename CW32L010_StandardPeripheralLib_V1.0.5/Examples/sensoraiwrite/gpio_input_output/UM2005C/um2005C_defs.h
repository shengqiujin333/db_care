/***********************************************************************************************************
 * Copyright (c)  2022 - 2023, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : um2005C_defs.h
 * Description : um2005C_defs header file
 * Author(s)   : limingzu
 * version     : V1.0
 * Modify date : 2022-12-20
 ***********************************************************************************************************/
#ifndef _UM2005C_DEFS_H_
#define _UM2005C_DEFS_H_

#define UM2005C_REG_MASK							0x3F
#define UM2005C_REG_WIRTE							0xC0
#define UM2005C_REG_READ							0x80

#define UM2005C_REG00								0x00
#define UM2005C_REG01								0x01
#define UM2005C_REG02								0x02
#define UM2005C_REG03								0x03
#define UM2005C_REG04								0x04
#define UM2005C_REG05								0x05
#define UM2005C_REG06								0x06
#define UM2005C_REG07								0x07
#define UM2005C_REG08								0x08
#define UM2005C_REG09								0x09
#define UM2005C_REG0A								0x0A
#define UM2005C_REG0B								0x0B
#define UM2005C_REG0B_PACOMP						0x20

#define UM2005C_REG0C								0x0C
#define UM2005C_REG0D								0x0D
#define UM2005C_REG0E								0x0E
#define UM2005C_REG0F								0x0F
#define UM2005C_REG10								0x10
#define UM2005C_REG10_GAU							0x04
#define UM2005C_REG10_OOK							0x01

#define UM2005C_REG11								0x11
#define UM2005C_REG12								0x12
#define UM2005C_REG13								0x13
#define UM2005C_REG14								0x14
#define UM2005C_REG15								0x15
#define UM2005C_REG16								0x16
#define UM2005C_REG17								0x17
#define UM2005C_REG18								0x18
#define UM2005C_REG19								0x19
#define UM2005C_REG1A								0x1A
#define UM2005C_REG1B								0x1B
#define UM2005C_REG1C								0x1C
#define UM2005C_REG1D								0x1D
#define UM2005C_REG1E								0x1E
#define UM2005C_REG1F								0x1F

#define UM2005C_REG3F								0x3F
#define UM2005C_REG3F_PAGE0							0x00
#define UM2005C_REG3F_PAGE1							0x80
#define UM2005C_REG3F_TX							0x10
#define UM2005C_REG3F_RST_DIG						0x04

typedef enum
{
	FSK  = 0,
	OOK  = 1,
	GFSK = 2,
}em_modution_t;

#endif
