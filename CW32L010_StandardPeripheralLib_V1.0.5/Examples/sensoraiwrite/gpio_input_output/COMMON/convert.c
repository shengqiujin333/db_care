/***********************************************************************************************************
 * Copyright (c)  2017 - 2021, Unicmicro Co.,Ltd .
 * All rights reserved.
 * Filename    : convert.c
 * Description : convert source file
 * Author(s)   : yanhaihua 
 * version     : V1.0
 * Modify date : 2021-02-02
 ***********************************************************************************************************/
#include "convert.h"
#include <stdio.h>

/***********************************************************************************************************
 * Function		: convert_chars_to_hexstr
 * Description	: convert_chars_to_hexstr
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
void convert_chars_to_hexstr(void *des,void *src,uint16_t len)
{
	char *temp_des = des;
	char *temp_src = src;
	for(uint16_t i=0;i<len;i++)
	{
		sprintf(temp_des,"%02X",*temp_src);
		temp_des += 2;
		temp_src++;
	}
}


/***********************************************************************************************************
 * Function		: convert_hexstr_to_int
 * Description	: hex×Ö·û´®×ªint
 * Input		: none
 * Output		: none
 * Return		: none
 ***********************************************************************************************************/
uint8_t convert_hexstr_to_int(uint8_t *str,int *data)
{
	int ret = 0;
	while(*str != NULL)
	{
		if(*str >= '0' && *str <= '9')
		{
			ret = (ret << 4) + *str - '0';
		}
		else if(*str >= 'a' && *str <= 'f')
		{
			ret = (ret << 4) + *str - 'a' + 10;
		}
		else if(*str >= 'A' && *str <= 'F')
		{
			ret = (ret << 4) + *str - 'A' + 10;
		}
		else
		{
			return NULL;
		}
		str++;
	}
	*data = ret;
	return 1;
}
