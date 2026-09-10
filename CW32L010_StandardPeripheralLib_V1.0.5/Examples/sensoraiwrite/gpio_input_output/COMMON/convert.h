/***********************************************************************************************************
 * Copyright (c)  2017 - 2021, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : convert.h
 * Description : convert header file
 * Author(s)   : yanhaihua  
 * version     : V1.0
 * Modify date : 2021-02-02
 ***********************************************************************************************************/
#ifndef __COVERT_H__
#define __COVERT_H__

#include <stdint.h>

void convert_chars_to_hexstr(void * des,void *src,uint16_t len);

uint8_t convert_hexstr_to_int(uint8_t *str,int *data);

#endif
