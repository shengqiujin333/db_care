/*
 * hall.c - F1 霍尔磁铁检测 (FR-101/102/506, HWI-001 2 节)
 *
 * PB04 = Hall 数字输入 + 外部中断; 有效沿触发一次有界配置会话。
 * ISR 只置标志, 去抖与开窗在主循环完成。
 */
#include "hall.h"
#include "optcfg.h"
#include "cw32l010_gpio.h"
#include "cw32l010_sysctrl.h"
#include "delay.h"

static volatile uint8_t s_hall_event;

void hall_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pins = HALL_IN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;   /* 开漏 Hall 上拉 */
    GPIO_InitStruct.IT   = HALL_ACTIVE_EDGE;
    GPIO_Init(OPT_GPIO_PORT, &GPIO_InitStruct);

    s_hall_event = 0;
    NVIC_EnableIRQ(GPIOB_IRQn);
}

void hall_isr(void)
{
    if (optcfg_window_active()) return;   /* 窗口活跃忽略再次触发 (FR-102) */
    s_hall_event = 1;
}

bool hall_event_pending(void) { return s_hall_event != 0; }
void hall_event_clear(void)   { s_hall_event = 0; }

bool hall_debounced_active(void)
{
#if HALL_ACTIVE_LOW
    if (GPIO_ReadPin(OPT_GPIO_PORT, HALL_IN_PIN) == GPIO_Pin_RESET) {
        delay_ms(10);                          /* 去抖 ~10ms (FR-506) */
        if (GPIO_ReadPin(OPT_GPIO_PORT, HALL_IN_PIN) == GPIO_Pin_RESET) return true;
    }
#else
    if (GPIO_ReadPin(OPT_GPIO_PORT, HALL_IN_PIN) == GPIO_Pin_SET) {
        delay_ms(10);
        if (GPIO_ReadPin(OPT_GPIO_PORT, HALL_IN_PIN) == GPIO_Pin_SET) return true;
    }
#endif
    return false;
}
