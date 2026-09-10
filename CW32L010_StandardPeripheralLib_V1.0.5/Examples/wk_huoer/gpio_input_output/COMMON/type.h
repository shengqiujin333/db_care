/***********************************************************************************************************
 * Copyright (c)  2021 - 2022, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : type.h
 * Description : type header file
 * Author(s)   : yanhaihua
 * version     : V1.0
 * Modify date : 2021-09-23
 ***********************************************************************************************************/
#ifndef __TYPE_H__
#define __TYPE_H__
#include "base_types.h"
//typedef enum
//{
//	ENABLE = 1,
//	DISABLE = 0,
//}em_enable_t;

//typedef enum
//{
//	SUCCESS = 0,
//	FAILED = 1,
//}em_ret_t;

typedef enum
{
	UM_TIME_WAITTING_FOREVER = 0,																			/* ×èÈû */
	UM_TIME_WAITTING_NO = 1																					/* ·Ç×èÈû */
}um_time_t;

typedef enum
{
	CODE_NONE = 0,																							/* ÎÞ±àÂë */
	CODE_EV1527 = 1																							/* 1527±àÂë */
}um_code_t;


#endif
