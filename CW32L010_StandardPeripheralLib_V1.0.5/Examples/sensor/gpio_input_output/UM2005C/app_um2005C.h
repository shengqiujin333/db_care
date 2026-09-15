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

/* 有界发送: 在 timeout_ms 内未完成发射返回 0(失败), 完成返回 1 (FR-303/304) */
uint8_t app_um2005C_send_data_timeout(uint8_t *data,uint16_t len,uint32_t timeout_ms);

#endif
