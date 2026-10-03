/*
 * mock_measure_mcu/cw32l010_gpio.h - host shadow of the vendor GPIO header.
 * Used only by test/host_measure_flow_check.c to compile the REAL USER/src/measure.c
 * on the host (placed before any vendor include path).
 */
#ifndef __MOCK_MEASURE_CW32L010_GPIO_H
#define __MOCK_MEASURE_CW32L010_GPIO_H

#include <stdint.h>

typedef enum { GPIO_Pin_RESET = 0, GPIO_Pin_SET = 1 } GPIO_PinState;

typedef struct { uint32_t Pins; uint32_t Mode; uint32_t IT; } GPIO_InitTypeDef;
typedef struct { int unused; } GPIO_TypeDef;

#define GPIO_PIN_3          ((uint16_t)0x0008)
#define GPIO_PIN_4          ((uint16_t)0x0010)
#define GPIO_PIN_5          ((uint16_t)0x0020)
#define GPIO_PIN_6          ((uint16_t)0x0040)

#define GPIO_MODE_INPUT     0x10u
#define GPIO_MODE_INPUT_PULLUP 0x11u
#define GPIO_MODE_OUTPUT_PP 0x20u
#define GPIO_MODE_OUTPUT_OD 0x30u
#define GPIO_IT_NONE        0x00u

#define CW_GPIOA ((GPIO_TypeDef *)0)
#define CW_GPIOB ((GPIO_TypeDef *)0)

#define __SYSCTRL_GPIOA_CLK_ENABLE() do { } while (0)
#define __SYSCTRL_GPIOB_CLK_ENABLE() do { } while (0)

void          GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *init);
void          GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t pin, GPIO_PinState state);
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t pin);

#endif /* __MOCK_MEASURE_CW32L010_GPIO_H */
