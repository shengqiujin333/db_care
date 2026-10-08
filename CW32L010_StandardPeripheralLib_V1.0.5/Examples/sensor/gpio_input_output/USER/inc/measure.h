/*
 * measure.h
 *
 *  Created on: Feb 22, 2024
 *      Author: kason
 */

#ifndef MEASURE_H_
#define MEASURE_H_

#include "stdint.h"
#include "debug_trace.h"   /* debug_trace_sample_t (T1: UART1 调试轨迹状态) */

#define start_measure   (0x0001<<0)
#define read_value   (0x0001<<1)
#define calculate_value   (0x0001<<2)
#define wait_data       (0x0001<<3)
#define prepare (0x0001<<4)

void temperature_task_init( void );
void bsp_i2c_init(void);
uint16_t temperature_process( void );

void send_data_to_gateway(void);
void go_to_sleep(void);

/*
 * 取出本采样周期的调试轨迹状态 (T1/T2; 只读, 不改变任何业务状态)。
 * 返回 1 = 本周期存在待发布轨迹并已填充/清除 pending; 返回 0 = 无 (不修改 *out)。
 * 光照字段来自 measure.c 内本周期 light_result_t (与判定同一份数据; T2)。
 */
uint8_t sensor_trace_fetch(debug_trace_sample_t *out);

/* 上报调度状态 (FD-002 §6.4) */
extern uint8_t report_req;
#endif /* MEASURE_H_ */
