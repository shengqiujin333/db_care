/*
 * mock_measure_mcu/cw32l010_sysctrl.h - host shadow of the vendor SYSCTRL header.
 */
#ifndef __MOCK_MEASURE_CW32L010_SYSCTRL_H
#define __MOCK_MEASURE_CW32L010_SYSCTRL_H

#include <stdint.h>

typedef enum { DISABLE = 0, ENABLE = 1 } FunctionalState;

#define SYSCTRL_AHB_PERIPH_GPIOA  0u
#define SYSCTRL_APB1_PERIPH_UART1 0u

void SYSCTRL_AHBPeriphReset(uint32_t periph, FunctionalState state);
void SYSCTRL_APBPeriphReset1(uint32_t periph, FunctionalState state);
void SYSCTRL_AHBPeriphClk_Enable(uint32_t periph, FunctionalState state);
void SYSCTRL_APBPeriphClk_Enable1(uint32_t periph, FunctionalState state);
void SYSCTRL_GotoDeepSleep(void);

#endif /* __MOCK_MEASURE_CW32L010_SYSCTRL_H */
