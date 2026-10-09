/*
 * host_light_check.c - 光照通路 (真实 USER/src/light.c) 实现侧自检 (T2)
 * 作者: firmware_engineer.firmware_implementation
 *
 * 覆盖 (FD-002 rev 4.0 §6.3/§10; TD-002 T-L0-04 / T-L3-04 的宿主机主证据):
 *   - light_init(): PB05 推挽输出(低) + PB04 模拟输入(AIN11) + ADC 配置后关闭;
 *   - light_sample(): PB05 供电 -> 8 次 AIN11 转换 -> PB05 置低; ADC 采样后关闭;
 *   - 结果 {valid, adc_ok, samples_ok, mean, min, max, dark}: 均值/极值只取成功样本;
 *   - 全部转换超时 -> adc_ok=false/valid=false/dark=false, mean=min=max=0,
 *     **不得**用满量程 4095 合成暗态;
 *   - 有效性门禁: 未标定 (LIGHT_DARK_CALIBRATED=0) 时 valid 恒 false -> dark=false;
 *     标定后 (编译期 -DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=...) dark = 3*mean>=C_dark;
 *   - 每笔独立: 同脚本两次采样结果相同 (无滞回/无历史暗态)。
 *
 * 构建 (从 gpio_input_output/):
 *   gcc -std=c11 -Wall -Wextra -Itest/mock_light -IUSER/inc -ICOMMON \
 *       test/host_light_check.c USER/src/light.c USER/src/fw_core.c -o host_light_check
 *   ./host_light_check                       # 未标定变体 (交付件默认)
 *   gcc ... -DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=3000 ...   # 标定后变体
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "light.h"
#include "fw_core.h"
#include "sensor_config.h"
#include "mock_light_hw.h"

/* ==================================================================== */
/* 模拟 MCU 外设层                                                       */
/* ==================================================================== */
GPIO_TypeDef mock_gpiob;
ADC_TypeDef  mock_adc;

static int      script[64];
static int      script_n, script_i;
static uint16_t conv_value;
static int      adc_enabled;
static int      conv_starts;
static int      conv_without_power;      /* 转换时 PB05 未供电的次数 (必须为 0) */
static int      pb05_high_seen;
static GPIO_PinState pb05_state;
static uint16_t last_init_pins;
static uint32_t last_init_mode;
static int      init_calls;

void delay_ms(uint16_t ms) { (void)ms; }

void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *cfg)
{
    (void)port;
    init_calls++;
    last_init_pins = cfg->Pins;
    last_init_mode = cfg->Mode;
}

void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    (void)port;
    if (pin == GPIO_PIN_5) {
        pb05_state = state;
        if (state == GPIO_Pin_SET) pb05_high_seen = 1;
    }
}

void ADC_Init(ADC_InitTypeDef *cfg) { (void)cfg; }
void ADC_ClearITPendingAll(void) { mock_adc.ISR = 0u; }
void ADC_Disable(void) { adc_enabled = 0; }
ErrorStatus ADC_Enable(void) { adc_enabled = 1; return SUCCESS; }

void ADC_SoftwareStartConvCmd(FunctionalState state)
{
    (void)state;
    conv_starts++;
    if (pb05_state != GPIO_Pin_SET) conv_without_power++;
    if (script_i < script_n) {
        int v = script[script_i++];
        if (v >= 0) {                       /* >=0: 转换完成 */
            conv_value = (uint16_t)v;
            mock_adc.ISR |= ADC_ISR_EOC_Msk;
        } else {                            /* -1: 转换超时 (不置 EOC) */
            mock_adc.ISR &= ~ADC_ISR_EOC_Msk;
        }
    } else {
        mock_adc.ISR &= ~ADC_ISR_EOC_Msk;
    }
}

void ADC_ClearITPendingBit(uint16_t it) { (void)it; mock_adc.ISR &= ~ADC_ISR_EOC_Msk; }
uint16_t ADC_GetConversionValue(uint32_t idx) { (void)idx; return conv_value; }

/* ==================================================================== */
/* 断言与脚本                                                            */
/* ==================================================================== */
static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

static void script_begin(void)
{
    script_n = 0; script_i = 0; conv_starts = 0; conv_without_power = 0;
    pb05_high_seen = 0; pb05_state = GPIO_Pin_RESET;
    mock_adc.ISR = 0u;
}

static void script_add(int v) { if (script_n < 64) script[script_n++] = v; }

int main(void)
{
    light_result_t r;

    printf("==== light.c self-check (T2, LIGHT_DARK_CALIBRATED=%d, C_dark=%u) ====\n",
           (int)LIGHT_DARK_CALIBRATED, (unsigned)LIGHT_DARK_REF_CODE);

    printf("[1] light_init: 引脚配置与 ADC 关闭\n");
    init_calls = 0; adc_enabled = 1;
    light_init();
    CHECK(init_calls == 2, "light_init configures exactly two pins (PB05, PB04)");
    CHECK((last_init_pins == GPIO_PIN_4) && (last_init_mode == GPIO_MODE_ANALOG),
          "PB04 configured as analog input (AIN11) after PB05");
    CHECK(adc_enabled == 0, "ADC is left disabled outside sampling");

    printf("[2] 8 次全部成功: 均值/极值取自成功样本, PB05 供电后置低, ADC 关闭\n");
    script_begin();
    script_add(100); script_add(110); script_add(120); script_add(130);
    script_add(140); script_add(150); script_add(160); script_add(170);
    r = light_sample();
    CHECK(conv_starts == LIGHT_ADC_SAMPLES, "exactly LIGHT_ADC_SAMPLES conversions");
    CHECK(conv_without_power == 0, "PB05 was powered (high) for every conversion");
    CHECK(pb05_state == GPIO_Pin_RESET, "PB05 returned low after sampling");
    CHECK(adc_enabled == 0, "ADC disabled after sampling");
    CHECK(r.samples_ok == 8u && r.adc_ok == true, "samples_ok=8, adc_ok=true");
    CHECK(r.mean_adc_code == 135u, "mean = 135 over successful samples");
    CHECK(r.code_min == 100u && r.code_max == 170u, "min=100, max=170");

    printf("[3] 部分超时: 均值只取成功样本, 成功数如实记录\n");
    script_begin();
    script_add(-1); script_add(1000); script_add(-1); script_add(1000);
    script_add(-1); script_add(1000); script_add(-1); script_add(1000);
    r = light_sample();
    CHECK(r.samples_ok == 4u && r.adc_ok == true, "4 successes recorded (timeouts dropped)");
    CHECK(r.mean_adc_code == 1000u && r.code_min == 1000u && r.code_max == 1000u,
          "mean/min/max computed over the 4 successful samples only");

    printf("[4] 全部超时: valid=false, 不得用满量程合成暗态\n");
    script_begin();
    for (int i = 0; i < (int)LIGHT_ADC_SAMPLES; i++) script_add(-1);
    r = light_sample();
    CHECK(r.samples_ok == 0u && r.adc_ok == false, "samples_ok=0, adc_ok=false");
    CHECK(r.mean_adc_code == 0u && r.code_min == 0u && r.code_max == 0u,
          "mean/min/max stay 0 (no 4095 full-scale synthesis)");
    CHECK(r.valid == false && r.dark == false, "valid=false, dark=false on total timeout");
    CHECK(pb05_state == GPIO_Pin_RESET && adc_enabled == 0, "PB05 low and ADC off after timeout too");

    printf("[5] 有效性/无光判定 (未标定门禁 vs 标定后 1/3 判据)\n");
#if LIGHT_DARK_CALIBRATED == 0
    script_begin();
    script_add(4095); script_add(4095); script_add(4095); script_add(4095);
    script_add(4095); script_add(4095); script_add(4095); script_add(4095);
    r = light_sample();
    CHECK(r.adc_ok == true && r.valid == false,
          "uncalibrated: adc_ok=true but valid=false (cannot prove darkness)");
    CHECK(r.dark == false, "uncalibrated: dark=false even at full-scale reading");
#else
    script_begin();
    script_add(1000); script_add(1000); script_add(1000); script_add(1000);
    script_add(1000); script_add(1000); script_add(1000); script_add(1000);
    r = light_sample();
    CHECK(r.valid == true, "calibrated: valid=true when ADC succeeds");
    CHECK(r.dark == (((uint32_t)3u * 1000u) >= (uint32_t)LIGHT_DARK_REF_CODE),
          "dark = (3*mean >= C_dark) at mean=1000");
    script_begin();
    script_add(999); script_add(999); script_add(999); script_add(999);
    script_add(999); script_add(999); script_add(999); script_add(999);
    r = light_sample();
    CHECK(r.dark == (((uint32_t)3u * 999u) >= (uint32_t)LIGHT_DARK_REF_CODE),
          "dark = (3*mean >= C_dark) at mean=999 (one below)");
#endif

    printf("[6] 每笔独立 (无滞回/无历史暗态)\n");
    {
        light_result_t a, b;
        script_begin();
        script_add(2000); script_add(2000); script_add(2000); script_add(2000);
        script_add(2000); script_add(2000); script_add(2000); script_add(2000);
        a = light_sample();
        script_begin();
        script_add(2000); script_add(2000); script_add(2000); script_add(2000);
        script_add(2000); script_add(2000); script_add(2000); script_add(2000);
        b = light_sample();
        CHECK(a.dark == b.dark && a.valid == b.valid && a.mean_adc_code == b.mean_adc_code,
              "same input -> same result regardless of the previous sample");
    }

    printf("[7] 上电供电轨测量 light_read_vdd_mv (ADC 内部 BGR1.2V 反推 VDD)\n");
    {
        uint16_t mv = 0xFFFFu, code = 0xFFFFu, bgrmv = 0xFFFFu;
        uint8_t  ok;

        /* 正常: code=1632, 出厂修调 1200mV -> mv = 4095*1200/1632 = 3011 mV */
        script_begin();
        script_add(1632);
        mock_adc.CR_f.BGREN = 0u;
        adc_enabled = 1;
        ok = light_read_vdd_mv(&mv, &code, &bgrmv);
        CHECK(ok == 1u, "有效转换: 返回 1");
        CHECK(code == 1632u && bgrmv == 1200u, "上报原始码与出厂修调值 (code=1632, bgrmv=1200)");
        CHECK(mv == (uint16_t)((4095uL * 1200uL) / 1632uL), "mv = 4095*bgrmv/code (可独立复核)");
        CHECK(mock_adc.CR_f.BGREN == 0u, "测完后 BGREN 已清 (不持续耗电)");
        CHECK(adc_enabled == 0, "测完后 ADC 已关闭");

        /* 转换超时: 返回 0 且不伪造读数, BGREN 仍被清 */
        script_begin();
        script_add(-1);
        mock_adc.CR_f.BGREN = 0u;
        adc_enabled = 1;
        mv = 0xFFFFu; code = 0xFFFFu; bgrmv = 0xFFFFu;
        ok = light_read_vdd_mv(&mv, &code, &bgrmv);
        CHECK(ok == 0u, "转换超时: 返回 0 (无效)");
        CHECK(mv == 0u && code == 0u && bgrmv == 0u, "无效时不写任何伪造读数");
        CHECK(mock_adc.CR_f.BGREN == 0u && adc_enabled == 0, "超时路径也关闭 BGR 与 ADC");

        /* 测量后光照通路仍可用 (VDD 测量不得破坏后续 AIN11 采样) */
        light_init();
        script_begin();
        script_add(10); script_add(20); script_add(30); script_add(40);
        script_add(50); script_add(60); script_add(70); script_add(80);
        r = light_sample();
        CHECK(r.samples_ok == LIGHT_ADC_SAMPLES && r.adc_ok, "VDD 测量后 light_sample 仍 8/8 成功");
    }

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
