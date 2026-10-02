/*
 * gxht40.h - GXHT40 温湿度传感器驱动 (readme 修改点 1; gxht40.pdf; FD-002 §3.1/§6/§10)
 *
 * 职责: 在软 I²C (sf_i2c) 之上完成一次完整测量并输出 x10 整数结果。
 * 不含: 采样节拍、光照、上报判定、低功耗调度 (属其它模块/任务项)。
 *
 * 协议要点 (gxht40.pdf):
 *   - 7bit 地址 0x44 (GXHT40-AD) 或 0x45 (GXHT40-BD/GXHT40C-AD), 运行期探测;
 *     8bit 写 0x88/0x8A, 读 0x89/0x8B。
 *   - 高重复率测量命令 0xFD; 返回 6 字节: T_MSB,T_LSB,T_CRC, RH_MSB,RH_LSB,RH_CRC。
 *   - 不支持 clock stretching; 数据未就绪时对读请求返回 NACK, 需等待后重读。
 *   - CRC-8: poly 0x31, init 0xFF, 无反转, xorout 0x00; 温度字与湿度字各一个 CRC。
 *   - 换算: T[0.1C] = -450 + round(1750*S_T/65536); RH[0.1%] = -60 + round(1250*S_RH/65536)。
 */
#ifndef __GXHT40_H
#define __GXHT40_H

#include <stdint.h>
#include <stdbool.h>
#include "sf_i2c.h"

/* 测量结果码 */
typedef enum
{
    GXHT40_OK = 0,          /* 成功; 输出已写入 */
    GXHT40_ERR_PARAM,       /* 入参非法(未绑定总线/空指针) */
    GXHT40_ERR_NO_DEVICE,   /* 0x44 与 0x45 均无 ACK */
    GXHT40_ERR_IO,          /* 器件在但 I²C 读失败(读 NACK 重试用尽) */
    GXHT40_ERR_CRC,         /* 温度/湿度字 CRC 校验失败(重测已用尽) */
    GXHT40_ERR_RANGE        /* 换算结果超出有效量程 (-40.0..125.0 C) */
} gxht40_status_t;

/* 绑定软 I²C 设备并清除地址探测缓存 (不访问总线) */
void gxht40_init(i2c_dev *dev);

/*
 * 执行一次完整测量: 地址探测(带缓存) -> 0xFD -> 等待 tMEAS -> 读 6B -> CRC -> 换算。
 * 成功: 写入 *temp_x10 (int16, 0.1C) 与 *hum_x10 (uint16, 0.1%RH), 返回 GXHT40_OK。
 * 失败: **不修改** *temp_x10 / *hum_x10, 返回对应失败码。
 * 重试: 命令/读取失败按 sensor_config.h 的 GXHT40_MEAS_RETRY 重测; 读 NACK 按
 *       GXHT40_READ_RETRY 等待后重读 (FD-002 §10)。
 */
gxht40_status_t gxht40_measure(int16_t *temp_x10, uint16_t *hum_x10);

/* 已探测到的 7bit 地址 (0 = 尚未探测到); 供调试跟踪使用 */
uint8_t gxht40_detected_addr7(void);

#endif /* __GXHT40_H */
