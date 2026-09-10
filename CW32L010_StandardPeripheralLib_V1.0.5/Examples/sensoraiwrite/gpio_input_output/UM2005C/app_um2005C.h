/***********************************************************************************************************
 * Copyright (c)  2022 - 2023, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : app_um2005C.h
 * Description : app um2005C source file
 * Author(s)   : limingzu
 * version     : V1.0
 * Modify date : 2022-12-20
 ***********************************************************************************************************/
#ifndef __APP_UM2005C_H__
#define __APP_UM2005C_H__ 

#include "stdint.h"

void app_um2005C_init(void);

void app_um2005C_send_data(uint8_t *data,uint16_t len);

#endif
