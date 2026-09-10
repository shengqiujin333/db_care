/*
 * measure.h
 *
 *  Created on: Feb 22, 2024
 *      Author: kason
 */

#ifndef MEASURE_H_
#define MEASURE_H_

#include "CH59x_common.h"
#include "stdint.h"


#define start_measure   (0x0001<<0)
#define read_value   (0x0001<<1)
#define calculate_value   (0x0001<<2)
#define wait_data       (0x0001<<3)
#define prepare (0x0001<<4)

void temperature_task_init( void );
void bsp_i2c_init();

#endif /* MEASURE_H_ */
