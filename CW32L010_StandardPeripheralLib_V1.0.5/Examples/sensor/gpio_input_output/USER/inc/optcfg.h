/*
 * optcfg.h - OPTCFG/1 光学配置窗口状态机 (FR-101..109, FD-001 6.2)
 *
 * 解码器/帧解析等纯逻辑在 fw_core.h/fw_core.c(宿主机 L0 可测);
 * 本头文件仅声明窗口常量、引脚与窗口状态机(依赖目标硬件)。
 */
#ifndef __OPTCFG_H
#define __OPTCFG_H

#include <stdint.h>
#include <stdbool.h>
#include "fw_core.h"

/* ---- 窗口/采样常量 (FD-001 4.3 / TD-001 7) ---- */
#define OPTCFG_WINDOW_MS        35000   /* 窗口 35 s */
#define OPTCFG_SAMPLE_MS        5       /* 采样周期 5 ms (200 Hz): 满足 IC-001 3.1 ±25% 半位宽容差的可靠解码 */
#define OPTCFG_MAX_FAIL_FRAMES  3       /* 连续 3 帧失败断电 (IC-001 3.1) */

/* ---- 引脚 (SCH-001 核定) ---- */
#define OPT_GPIO_PORT           CW_GPIOB
#define OPT_IN_PIN              GPIO_PIN_5   /* 光敏数字输入(施密特) */
#define F2_PWR_EN_PIN           GPIO_PIN_6   /* F2 供电使能, 低=使能 */
#define HALL_IN_PIN             GPIO_PIN_4   /* Hall 数字输入(中断) */

/* ---- 窗口状态机与硬件访问 ---- */
void  optcfg_init(void);                 /* PB06 高(断电), PB05 输入 */
bool  optcfg_window_active(void);
void  optcfg_window_start(void);         /* Hall 有效沿触发 (FR-101/102) */
void  optcfg_lptim_isr(void);            /* 由 LPTIM_IRQHandler 转发 */
void  optcfg_process(void);              /* 主循环: 提交/关窗 */
bool  optcfg_commit_pending(void);

#endif /* __OPTCFG_H */
