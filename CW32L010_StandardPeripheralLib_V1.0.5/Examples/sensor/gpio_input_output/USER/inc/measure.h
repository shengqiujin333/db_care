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

/*
 * 上电 I/O 自检 (E1; 交回实现的区分实验)。
 *   ① 主机把 SDA/SCL 拉低并回读 (sda_lo/scl_lo, 0 = 确实拉低);
 *   ② 释放后回读空闲电平 (idle);
 *   ③ SDA/SCL 角色对调后两个候选地址是否有 ACK (swap, 区分装配接反)。
 * 只做电平驱动与只发地址字节的探测; 不改变采样/判定/上报/冻结状态。
 * 返回 1 = 已填充; 0 = 无法进行。
 */
uint8_t sensor_io_diag_scan(debug_trace_iotest_t *out);

/*
 * 上电事务内逐位回读签名 (E1b; H12 判别)。
 *   用与软 I2C 相同的引脚原语与半位延时, 手工发出一个地址字节 0x88 的完整事务
 *   (START + 8 数据位 + ACK 时隙), 在 20 个半位采样点回读 SCL/SDA 并 MSB 先入拼成
 *   两个 20 bit 值。无器件应答时正确主机应得 scl=0x95555 / sda=0x30303。
 *   不写 0xFD、不发第二个字节; 结束后补一个合法 STOP 并释放两线。
 * 返回 1 = 已填充; 0 = 无法进行。
 */
uint8_t sensor_io_sig_scan(debug_trace_iosig_t *out);

/* 上报调度状态 (FD-002 §6.4) */
extern uint8_t report_req;
#endif /* MEASURE_H_ */
