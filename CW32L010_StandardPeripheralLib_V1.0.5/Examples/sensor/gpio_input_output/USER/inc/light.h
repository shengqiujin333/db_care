/*
 * light.h - 光照通路 (readme 修改点 2; FD-002 §2.3/§3.1/§6.3/§10)
 *
 * 拓扑 (sensor_hardware 网表核定):
 *   PB05(LIGTHT_POWER, 输出) -- R3 5M -- LIGHT_ADC(PB04/AIN11) -- 光敏电阻 -- GND
 * ADC 满量程参考 = VDD (比率式, 与电池电压无关):
 *   code = 4095 * R_photo / (R_photo + 5M)
 * 极性: 无光(暗) => 光敏阻值大 => 节点电压高 => 读数大。
 *
 * 不含: 采样节拍、温湿度、上报判定 (属其它模块/任务项)。
 */
#ifndef __LIGHT_H
#define __LIGHT_H

#include <stdint.h>
#include <stdbool.h>

/* 配置 PB05 为推挽输出(低)、PB04 为模拟输入(AIN11) 并配置 ADC (ADC 保持关闭, 采样时打开) */
void light_init(void);

/* 复位滞回状态与最近读数 (上电/标定后) */
void light_reset_state(void);

/*
 * 一次光照采样:
 *   PB05 输出高 -> 稳定延时 -> PB04/AIN11 取样 LIGHT_ADC_SAMPLES 次求均值 -> PB05 置低;
 *   用均值做滞回判定并更新内部状态。
 * 返回 true = 无光(DARK)。
 * 异常: 单次 ADC 转换超时被丢弃; 全部超时按满量程(器件开路/无光)处理, 不阻塞、不无限等待。
 */
bool light_sample(void);

/* 最近一次采样的均值 (0..4095); 供调试跟踪 */
uint16_t light_last_code(void);

#endif /* __LIGHT_H */
