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

/*
 * 上电总线身份诊断 (FWR-116; 只观测)。
 *   读取 PA03(SCL)/PA04(SDA) 在**任何 I2C 事务之前**的空闲电平, 然后对
 *   SENSOR_BUS_DIAG_FIRST_ADDR7..LAST_ADDR7 逐地址发地址字节探测 (不写命令/不读数据),
 *   把结果填入 *out。内部调用幂等的 bsp_i2c_init(); 不改变采样/判定/上报/冻结状态。
 * 返回 1 = 已填充; 0 = 无法进行 (out 为空或未绑定总线, *out 未填充)。
 */
uint8_t sensor_bus_diag_scan(debug_trace_bus_t *out);

/* 上报调度状态 (FD-002 §6.4) */
extern uint8_t report_req;
#endif /* MEASURE_H_ */
