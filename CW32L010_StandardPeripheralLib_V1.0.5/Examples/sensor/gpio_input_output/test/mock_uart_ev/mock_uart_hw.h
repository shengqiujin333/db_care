/*
 * mock_uart_hw.h - 宿主机影子 MCU 层（embedded_tester.embedded_verification 自建）
 *
 * 用途: 在宿主机编译真实产品 USER/src/debug_trace.c（SENSOR_DEBUG_UART=1），
 *       以可控的 TXE/TXBUSY 行为与调用计数验证 ITEM-004 的有界等待/中止/关闭语义。
 *       本层只替换寄存器与外设原语，不替换产品流程逻辑；位于 include 路径最前，
 *       不进入固件构建。实现能力自有 test/mock_trace 不被本 harness 使用。
 *
 * 故障注入:
 *   - TXE 模式 READY / STUCK / SLOW（每 N 次轮询就绪一次）
 *   - 可设置 STUCK 在绝对轮询次数后恢复（用于验证 S/G 各自预算）
 *   - TXBUSY 可永久忙（永不排空）
 *   - 硬上限: 单次函数调用内轮询超过 MOCK_POLL_HARD_CAP 即 longjmp 出 harness，
 *     使"无界等待"的旧实现被判定为 UNBOUNDED 而不是挂死。
 */
#ifndef __MOCK_UART_HW_H
#define __MOCK_UART_HW_H

#include <stdint.h>
#include <stddef.h>

typedef enum { RESET = 0, SET = 1 } FlagStatus, ITStatus;
typedef enum { DISABLE = 0, ENABLE = 1 } FunctionalState;
typedef enum { ERROR = 0, SUCCESS = 1 } ErrorStatus;
typedef enum { GPIO_Pin_RESET = 0, GPIO_Pin_SET = 1 } GPIO_PinState;

#define GPIO_PIN_0 ((uint16_t)0x0001)
#define GPIO_PIN_3 ((uint16_t)0x0008)
#define GPIO_PIN_4 ((uint16_t)0x0010)
#define GPIO_PIN_5 ((uint16_t)0x0020)
#define GPIO_PIN_6 ((uint16_t)0x0040)

#define GPIO_MODE_INPUT        0x10u
#define GPIO_MODE_INPUT_PULLUP 0x11u
#define GPIO_MODE_OUTPUT_PP    0x20u
#define GPIO_MODE_OUTPUT_OD    0x30u
#define GPIO_MODE_ANALOG       0x40u
#define GPIO_IT_NONE           0x00u
#define GPIO_IT_FALLING        0x01u

typedef struct { uint16_t Pins; uint32_t Mode; uint32_t IT; } GPIO_InitTypeDef;

typedef struct { volatile uint32_t AFR5, AFR6; } mock_afrl_t;
typedef struct { mock_afrl_t AFRL_f; } GPIO_TypeDef;

extern GPIO_TypeDef mock_gpioa, mock_gpiob;
#define CW_GPIOA (&mock_gpioa)
#define CW_GPIOB (&mock_gpiob)

#define __SYSCTRL_GPIOA_CLK_ENABLE() do { } while (0)
#define __SYSCTRL_GPIOB_CLK_ENABLE() do { } while (0)
#define PA05_AFx_UART1RXD() do { } while (0)
#define PA06_AFx_UART1TXD() do { } while (0)

void           GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init);
void           GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
GPIO_PinState  GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);

#define CW_UART1 0u
#define UART_Over_16                  0u
#define UART_Source_PCLK              0u
#define UART_StartBit_FE              0u
#define UART_StopBits_1               0u
#define UART_Parity_No                0u
#define UART_HardwareFlowControl_None 0u
#define UART_Mode_Rx                  1u
#define UART_Mode_Tx                  2u
#define UART_FLAG_TXE                 ((uint16_t)0x0001)
#define UART_FLAG_TC                  ((uint16_t)0x0002)
#define UART_FLAG_TXBUSY              ((uint16_t)0x4000)

typedef struct {
    uint32_t UART_BaudRate;
    uint32_t UART_Over;
    uint32_t UART_Source;
    uint32_t UART_UclkFreq;
    uint32_t UART_StartBit;
    uint32_t UART_StopBits;
    uint32_t UART_Parity;
    uint32_t UART_HardwareFlowControl;
    uint32_t UART_Mode;
} UART_InitTypeDef;

void       UART_Init(uint32_t uart, UART_InitTypeDef *cfg);
void       UART_SendData_8bit(uint32_t uart, uint8_t data);
FlagStatus UART_GetFlagStatus(uint32_t uart, uint16_t flag);

#define SYSCTRL_APB1_PERIPH_UART1   0u
#define SYSCTRL_AHB_PERIPH_GPIOA    0u
#define __SYSCTRL_ADC_CLK_ENABLE()  do { } while (0)

void SYSCTRL_APBPeriphClk_Enable1(uint32_t periph, FunctionalState st);
void SYSCTRL_APBPeriphReset1(uint32_t periph, FunctionalState st);
void SYSCTRL_AHBPeriphReset(uint32_t periph, FunctionalState st);
void SYSCTRL_AHBPeriphClk_Enable(uint32_t periph, FunctionalState st);

#endif /* __MOCK_UART_HW_H */
