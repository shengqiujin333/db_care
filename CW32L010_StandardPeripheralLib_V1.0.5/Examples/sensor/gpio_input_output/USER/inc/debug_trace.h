/*
 * debug_trace.h - UART1 调试通道接口 (readme 修改点 8; FD-002 rev 4.0 §2.4/§6.5)
 *
 * 用途: 传感器固件使用 UART1 作为调试串口; 上电打印一条启动横幅, 每个采样周期在
 *       判定与发送之后打印一行不超过 96 字节的整数轨迹, 供真实目标观测与验证。
 *
 * 引脚/参数 (网表 sensor_hardware/pstxnet.dat 核定):
 *   PA06 = UART1_TXD -> J3.1 ; PA05 = UART1_RXD -> J3.2 ; J3.3 = GND
 *   9600 8N1, PCLK 8 MHz (HSI DIV6), 轮询发送, 不使用 UART 中断。
 *
 * 所有权与副作用 (FD-002 §2.4/§11.15-§11.18):
 *   - 本模块是 UART1 与 PA05/PA06 的唯一所有者: 其它模块不得配置 UART1 或改写这两个引脚。
 *   - 关闭串口只复位/关闭 UART1 与改写 PA05/PA06; 不得复位 GPIOA、不得关闭 GPIOA 时钟、
 *     不得改动 PA03/PA04(软 I2C)与 PA07/PA08(SWD)。
 *   - 打印只读取调用方传入的状态, 不修改任何业务状态 (report_req/前值/快照/发送结果)。
 *   - SENSOR_DEBUG_UART=0 时全部实现为空, 采样/判定/上报行为不变。
 */
#ifndef __DEBUG_TRACE_H
#define __DEBUG_TRACE_H

#include <stdint.h>
#include <stddef.h>

#include "sensor_config.h"   /* SENSOR_DEBUG_UART / SENSOR_DEBUG_UART_MAXLINE (单一配置点) */

/* 一次采样周期的可观测状态 (由 measure.c / light.c 收集, 本模块只格式化与发送) */
typedef struct {
    uint32_t tick;          /* RTC 累计 1 分钟节拍计数 (置位时每 3 拍一次采样; 用于核对节拍) */
    int16_t  prev_temp_x10; /* 判定用的前一有效温度 x10 (0.1 C, 有符号) */
    uint8_t  have_prev;     /* 是否存在前一有效样本 (0/1) */
    uint8_t  light_valid;   /* 光照采样是否有效 (成功样本数 > 0) */
    uint8_t  light_ok;      /* 成功转换样本数 (0..LIGHT_ADC_SAMPLES) */
    uint16_t light_min;     /* 成功样本最小 ADC 码 (0..4095) */
    uint16_t light_max;     /* 成功样本最大 ADC 码 (0..4095) */
    uint16_t light_mean;    /* 成功样本算术均值 (ok==0 时为 0, 不得冒充暗态) */
    uint8_t  light_dark;    /* 判定路径实际使用的无光结果 (0/1) */
    int16_t  temp_x10;      /* 本周期温度 x10 (失败周期为最近有效值, 由 sample_ok=0 标记) */
    uint16_t hum_x10;       /* 本周期湿度 x10 (同上) */
    uint8_t  sample_ok;     /* 本周期温湿度采样是否有效 (1=有效, 0=失败) */
    uint8_t  report;        /* 本周期条件上报判定结果 (0/1) */
    uint8_t  send;          /* 433 发送结果: 0=未发送 1=成功 2=失败 */
    uint8_t  retry;         /* 待上报重试计数 */
} debug_trace_sample_t;

#if SENSOR_DEBUG_UART

/* 上电横幅: 固件标识 + 芯片 UID 前 4 字节 + 复位来源 + 串口参数, 并打开 UART1 */
void debug_trace_boot(const uint8_t *uid, uint32_t reset_flags);

/* 打印一个采样周期的 S 轨迹行 (要求 UART1 已由 debug_trace_boot 打开) */
void debug_trace_sample(const debug_trace_sample_t *s);

/* 等待发送完成(TC)后关闭 UART1; 幂等, 可在任意深睡路径前重复调用 */
void debug_trace_flush_close(void);

#else

/*
 * 宿主机 harness / 关闭态: 编译为空操作。
 * 宿主机没有 MCU UART 外设, 且既有宿主机验证资产不提供这些符号;
 * 空操作使调用点编译为空, 不引入未定义引用, 业务行为不变。
 */
static inline void debug_trace_boot(const uint8_t *uid, uint32_t reset_flags)
{
    (void)uid;
    (void)reset_flags;
}

static inline void debug_trace_sample(const debug_trace_sample_t *s)
{
    (void)s;
}

static inline void debug_trace_flush_close(void)
{
}

#endif /* SENSOR_DEBUG_UART */

#endif /* __DEBUG_TRACE_H */
