/*
 * host_light_check.c - 光照通路纯逻辑的实现侧自检 (宿主机, 不进入固件构建)
 *
 * 目的: 确定性验证 ITEM-004 的滞回判定 (FD-002 §6.3, TD-002 T-L0-04 的边界):
 *   - 已明(LIT): code >= LIGHT_DARK_ENTER 才转 DARK; 无光 = code >= 进入阈值
 *   - 已暗(DARK): code <= LIGHT_DARK_EXIT 才转 LIT
 *   - 端点: 0 -> LIT; 4095 -> DARK; 阈值上下逐点
 *   - 配置关系: LIGHT_DARK_EXIT < LIGHT_DARK_ENTER, 差 = LIGHT_DARK_HYSTERESIS_STEP
 *
 * light.c 的 ADC/GPIO 部分直接操作寄存器, 本测试只调用纯函数 light_code_is_dark,
 * 其余被引用函数以下方宿主机桩满足链接 (不被执行)。
 *
 * 编译(在本目录):
 *   gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast \
 *       -I../USER/inc -I../COMMON -I../../../../Libraries/inc \
 *       -I<CMSIS 5.9.0 Core Include> \
 *       host_light_check.c ../USER/src/light.c -o host_light_check.exe \
 *       && ./host_light_check.exe
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "light.h"
#include "sensor_config.h"
#include "delay.h"

/* ---- 宿主机桩: light.c 引用但不被本测试调用的硬件函数 ---- */
void delay_ms(uint16_t ms) { (void)ms; }
void ADC_Init(ADC_InitTypeDef *p) { (void)p; }
void ADC_ClearITPendingAll(void) { }
void ADC_Disable(void) { }
ErrorStatus ADC_Enable(void) { return SUCCESS; }
void ADC_SoftwareStartConvCmd(FunctionalState s) { (void)s; }
void ADC_ClearITPendingBit(uint16_t it) { (void)it; }
uint16_t ADC_GetConversionValue(uint32_t x) { (void)x; return 0u; }
void GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *q) { (void)p; (void)q; }
void GPIO_WritePin(GPIO_TypeDef *p, uint16_t pin, GPIO_PinState st) { (void)p; (void)pin; (void)st; }

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("  PASS  %s\n", m); } \
                         else   { fail++; printf("  FAIL  %s\n", m); } } while (0)

int main(void)
{
    printf("[1] 配置关系与阈值\n");
    CHECK(LIGHT_DARK_ENTER == 350u && LIGHT_DARK_EXIT == 250u,
          "LIGHT_DARK_ENTER=350 / LIGHT_DARK_EXIT=250");
    CHECK(LIGHT_DARK_EXIT < LIGHT_DARK_ENTER, "退出阈值 < 进入阈值 (滞回方向正确)");
    CHECK((uint16_t)(LIGHT_DARK_ENTER - LIGHT_DARK_EXIT) == LIGHT_DARK_HYSTERESIS_STEP,
          "ENTER - EXIT == LIGHT_DARK_HYSTERESIS_STEP");
    CHECK(LIGHT_ADC_FULL_SCALE == 4095u, "满量程 4095 (12 bit)");

    printf("[2] 已明 -> 暗: 达到进入阈值才转 DARK\n");
    CHECK(light_code_is_dark(0u, false) == false, "code=0 -> LIT");
    CHECK(light_code_is_dark(349u, false) == false, "code=349 (<350) -> LIT");
    CHECK(light_code_is_dark(350u, false) == true, "code=350 (=350) -> DARK");
    CHECK(light_code_is_dark(351u, false) == true, "code=351 -> DARK");
    CHECK(light_code_is_dark(4095u, false) == true, "code=4095 (满量程/开路) -> DARK");

    printf("[3] 已暗 -> 明: 低于退出阈值才转 LIT\n");
    CHECK(light_code_is_dark(4095u, true) == true, "code=4095 保持 DARK");
    CHECK(light_code_is_dark(351u, true) == true, "code=351 (>250) 保持 DARK");
    CHECK(light_code_is_dark(251u, true) == true, "code=251 (>250) 保持 DARK");
    CHECK(light_code_is_dark(250u, true) == false, "code=250 (=250) -> LIT");
    CHECK(light_code_is_dark(249u, true) == false, "code=249 -> LIT");
    CHECK(light_code_is_dark(0u, true) == false, "code=0 -> LIT");

    printf("[4] 滞回带内不抖动 (250..349 保持原状态)\n");
    CHECK(light_code_is_dark(300u, false) == false, "已明 + code=300 (带内) -> 仍 LIT");
    CHECK(light_code_is_dark(300u, true) == true, "已暗 + code=300 (带内) -> 仍 DARK");

    printf("\n==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
