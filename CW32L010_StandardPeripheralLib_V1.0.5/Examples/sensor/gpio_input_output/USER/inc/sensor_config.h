/*
 * sensor_config.h - 传感器固件唯一配置点
 *
 * 依据: FD-002 firmware_design.md §2/§6/§11.1, FWR-002, IC-002, readme.txt
 * 作用: 集中本次改动涉及的全部引脚、器件参数、阈值、节拍与上报常量。
 *       其它 .c 文件只引用本头文件的宏, 不得再散落同值魔法数 (FD-002 §11.1)。
 *
 * 宿主机纯逻辑 (fw_core.c 与 test/ 下的宿主机测试):
 *   定义 SENSOR_CONFIG_NO_MCU 后再包含本头文件, 只取纯数值常量, 不引入任何
 *   MCU 寄存器/外设头 (FD-002 §3.2: fw_core.c 不依赖 MCU 头)。
 *
 * 维护约定:
 *   - 任何数值调整只允许改本文件;
 *   - 数值型默认值若标注"待标定", 必须由实板标定后回填 (FWR-OPEN-1), 不得当作已验收。
 */
#ifndef __SENSOR_CONFIG_H
#define __SENSOR_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* ==================================================================== */
/* 1. 采样节拍 (readme 修改点 3; FD-002 §5.2)                            */
/* ==================================================================== */
/* RTC 中断间隔由 RTC_Configuration() 设为 RTC_INTERVAL_EVERY_1M */
#define SENSOR_RTC_TICK_PERIOD_MIN      1u      /* RTC 节拍: 1 分钟 */
#define SENSOR_SAMPLE_TICKS             3u      /* 1 min x 3 = 每 3 分钟采样一次 */

/* ==================================================================== */
/* 2. 温湿度传感器 GXHT40 (readme 修改点 1; gxht40.pdf; FD-002 §2.2)     */
/* ==================================================================== */
/* 7bit I2C 地址: GXHT40-AD = 0x44, GXHT40-BD/GXHT40C-AD = 0x45; 运行期探测 */
#define GXHT40_ADDR_7BIT_A              0x44u
#define GXHT40_ADDR_7BIT_B              0x45u
/* sf_i2c 使用 8bit 地址字节: 写 = 7bit<<1, 读 = 写|0x01 */
#define GXHT40_ADDR_WRITE_A             ((uint8_t)(GXHT40_ADDR_7BIT_A << 1))  /* 0x88 */
#define GXHT40_ADDR_READ_A              ((uint8_t)(GXHT40_ADDR_WRITE_A | 0x01u)) /* 0x89 */
#define GXHT40_ADDR_WRITE_B             ((uint8_t)(GXHT40_ADDR_7BIT_B << 1))  /* 0x8A */
#define GXHT40_ADDR_READ_B              ((uint8_t)(GXHT40_ADDR_WRITE_B | 0x01u)) /* 0x8B */

/* 功能命令 (手册表 9) */
#define GXHT40_CMD_MEASURE_HIGH_REP     0xFD    /* 高重复率测量温湿度, 周期路径只用此命令 */
#define GXHT40_CMD_MEASURE_MED_REP      0xF6    /* 中重复率 (备用, 不周期使用) */
#define GXHT40_CMD_MEASURE_LOW_REP      0xE0    /* 低重复率 (备用, 不周期使用) */
#define GXHT40_CMD_SOFT_RESET           0x94    /* 软复位, 周期路径不得使用 (FD-002 §11.10) */

/* 测量返回: 2B 温度 + 1B CRC + 2B 湿度 + 1B CRC (手册 §7.2) */
#define GXHT40_RESULT_LEN               6u

/* 时序: 高重复率 tMEAS.H 最大 8.3 ms (VDD=1.6V), 取 10 ms 余量 (FD-002 §11.3) */
#define GXHT40_MEASURE_WAIT_MS          10u
/* 读请求 NACK = 转换未完成; 等待后重读 */
#define GXHT40_READ_RETRY               5u
#define GXHT40_READ_RETRY_DELAY_MS      1u
/* 整帧重测上限: I2C 错误或 CRC 校验失败时 */
#define GXHT40_MEAS_RETRY               3u
/* 首次访问前上电余量: 手册 tPU 最大 1 ms */
#define GXHT40_POWER_ON_WAIT_MS         2u

/* CRC-8: poly 0x31, init 0xFF, 不反转, xorout 0x00; 参考向量 CRC(0xBEEF)=0x92 */
#define GXHT40_CRC8_POLY                0x31u
#define GXHT40_CRC8_INIT                0xFFu

/* 原始值 -> x10 整数换算 (FD-002 §6.2, 禁止浮点)
 * T[0.1C]  = -450 + round(1750 * S_T  / 65536)
 * RH[0.1%] =  -60 + round(1250 * S_RH / 65536), 再截断 0..1000
 */
#define GXHT40_TEMP_OFFSET_X10          (-450)
#define GXHT40_TEMP_SCALE_NUM           1750uL
#define GXHT40_HUM_OFFSET_X10           (-60)
#define GXHT40_HUM_SCALE_NUM            1250uL
#define GXHT40_RAW_DEN                  65536uL
#define GXHT40_RAW_ROUND                32768uL
/* 有效量程 (手册: -40..125 C, 0..100 %RH) */
#define GXHT40_TEMP_X10_MIN             (-400)
#define GXHT40_TEMP_X10_MAX             (1250)
#define GXHT40_HUM_X10_MIN              (0u)
#define GXHT40_HUM_X10_MAX              (1000u)

/* ==================================================================== */
/* 3. 光照通路数值配置 (readme 修改点 2; FD-002 §2.3/§6.3)              */
/*    拓扑: PB05(VDD) -> R3 5M -> LIGHT_ADC(PB04) -> 光敏电阻 -> GND     */
/*    ADC 满量程参考 = VDD (比率式, 与电池电压无关)                      */
/*    极性: 无光 = 光敏阻值大 = 读数高 = DARK                            */
/* ==================================================================== */
#define LIGHT_ADC_FULL_SCALE            4095u           /* 12 bit */
#define LIGHT_ADC_SAMPLES               8u              /* 多次取样求算术平均 */
/* ADC EOC 轮询上限(防转换挂死): 超时样本丢弃, 全部超时按满量程(无光)处理, 不阻塞 */
#define LIGHT_ADC_EOC_GUARD             100000uL
/* PB05 上电到首次转换的稳定等待, 覆盖分压 RC 与光敏器件响应 */
#define LIGHT_SETTLE_MS                 100u
#define LIGHT_IDLE_POWER_OFF            1               /* 1 = 非采样期 PB05 输出低 */

/* 明暗阈值 + 滞回 (FD-002 §6.3)
 * !! 待标定: 以下为工程默认值, 不是需求给定值; 必须按 T-L3-03 实板标定后回填,
 *    回填前置 LIGHT_DARK_CALIBRATED = 1, 否则不得宣称"无光判定已验收"。
 *    标定方法: PB05 恒高, 在"预期最亮/预期最暗"各读 30 次均值, 取二者之间且
 *    偏离两端 >=2 倍噪声带的整数作为 ENTER, EXIT = ENTER - 100。
 */
#define LIGHT_DARK_ENTER                350u
#define LIGHT_DARK_EXIT                 250u
#define LIGHT_DARK_HYSTERESIS_STEP      100u            /* EXIT = ENTER - STEP 的标定关系 */
#define LIGHT_DARK_CALIBRATED           0               /* 0 = 默认未标定 (FWR-OPEN-1) */

/* ==================================================================== */
/* 4. 上报判定 (readme 修改点 4; IC-002 §2; FD-002 §6.4)                 */
/*    report = ((prev - cur) > 0.9C 且 DARK) 或 (cur > 35.0C)            */
/* ==================================================================== */
#define SENSOR_REPORT_DROP_X10          9       /* 0.9 C; 严格大于(IC-002): >9 即下降>=1.0 C 才触发 */
#define SENSOR_REPORT_HIGH_X10          350     /* 35.0 C; 严格大于: 恰好 350 不触发 */

/* ==================================================================== */
/* 5. 433MHz 上报链路 (FD-002 §8.1, 布局不许变)                          */
/* ==================================================================== */
#define SENSOR_RF_FRAME_LEN             10u     /* uid4 | temp_le2 | hum_le2 | crc16_le2 */
#define SENSOR_RF_TX_TIMEOUT_MS         200u    /* 单次有界发送超时 (复用既有实现) */
#define SENSOR_RF_TX_RETRY              3u      /* 发送失败重试上限 */

/* ==================================================================== */
/* 6. 调试跟踪钩子 (test_design.md §2 交接项; 编译期默认关闭)            */
/*    打开后可在采样周期打印: 周期计数/光照 code/dark/temp_x10/hum_x10/   */
/*    report 判定/prev_temp_x10/I2C 重试计数。量产必须保持关闭。          */
/* ==================================================================== */
#ifndef SENSOR_TEST_TRACE
#define SENSOR_TEST_TRACE               0
#endif

/* ==================================================================== */
/* 7. MCU 相关配置 (引脚/外设宏; 宿主机纯逻辑定义 SENSOR_CONFIG_NO_MCU 跳过) */
/* ==================================================================== */
#ifndef SENSOR_CONFIG_NO_MCU
#include "cw32l010_gpio.h"
#include "cw32l010_adc.h"

#define LIGHT_POWER_PORT                CW_GPIOB
#define LIGHT_POWER_PIN                 GPIO_PIN_5      /* PB05 = LIGTHT_POWER */
#define LIGHT_ADC_PORT                  CW_GPIOB
#define LIGHT_ADC_PIN                   GPIO_PIN_4      /* PB04 = LIGHT_ADC  */
#define LIGHT_ADC_INPUT_CHANNEL         ADC_InputCH11   /* CW32L010: PB04 = AIN11 */
/* ADC 配置: ADCCLK = PCLK/8 = 1 MHz; 390 clk = 390 us 采样保持, 适配 5M 源阻抗 */
#define LIGHT_ADC_CLK_DIV               ADC_Clk_Div8
#define LIGHT_ADC_SAMPLE_TIME           ADC_SampTime390Clk
#endif /* SENSOR_CONFIG_NO_MCU */

#endif /* __SENSOR_CONFIG_H */
