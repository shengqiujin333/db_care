/*
 * hall.h - F1 霍尔磁铁检测 (FR-101/102/506, HWI-001 2 节)
 */
#ifndef __HALL_H
#define __HALL_H

#include <stdint.h>
#include <stdbool.h>

/* 单极霍尔开关(AH180 类, 开漏, 磁铁接近=输出低) */
#define HALL_ACTIVE_EDGE    GPIO_IT_FALLING   /* 有效沿 = 磁铁接近 */
#define HALL_ACTIVE_LOW     1u                /* 1=有效时 GPIO_ReadPin==RESET */

void  hall_init(void);          /* PB04 输入+EXTI, 使能 GPIOB 中断 */
void  hall_isr(void);           /* 由 GPIOB_IRQHandler 调用 */
bool  hall_event_pending(void); /* 有待处理的 Hall 有效沿 */
void  hall_event_clear(void);
bool  hall_debounced_active(void);  /* 主循环去抖确认(~10ms) */

#endif /* __HALL_H */
