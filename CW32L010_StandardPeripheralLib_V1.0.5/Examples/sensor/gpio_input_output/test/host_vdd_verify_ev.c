/*
 * host_vdd_verify_ev.c
 * 独立验证 harness (embedded_tester.embedded_verification) —— rev 5.6 新增的
 * 上电供电轨测量 API `light_read_vdd_mv()` (USER/src/light.c)。
 *
 * 独立性:
 *   - 编译的是**真实** USER/src/light.c (以及它依赖的真实 fw_core.c);
 *   - 影子 MCU 层是本能力自有的 test/mock_vdd_ev/ (非实现能力的 mock_light/);
 *   - 期望值由本文件独立推导: 4095*bgrmv/code 的**双重参考**——
 *     (a) 按厂商示例 Examples/ADC/adc_sgl_sw_vdd 的
 *         MCU_VDD = (4095 * BGR_1_2V_value * 0.001f) / valueAdc   [伏]
 *         即整数式 mv = 4095*BGR_mV/code;
 *     (b) 本文件用 double 逐步复算并按同一 6000 mV 上限截断;
 *     两者与固件输出逐例比对。
 *   - 断言目标: 公式自洽、超时/除零不伪造读数、测量后清理 (ADC 关、BGREN 清)、
 *     参数保护、测量不破坏后续 AIN11 光照采样。
 *
 * 构建 (从 gpio_input_output/ 目录):
 *   gcc -std=c11 -Wall -Wextra -Itest/mock_vdd_ev -IUSER/inc -ICOMMON \
 *       -DLIGHT_BGR_TRIM_MV=mock_bgr_trim_value \
 *       test/host_vdd_verify_ev.c USER/src/light.c USER/src/fw_core.c -o _hostbin/vdd_verify
 *   （-DLIGHT_BGR_TRIM_MV=mock_bgr_trim_value 把器件绝对地址 0x001007D2 的读取
 *     替换为 harness 可控变量, 从而在同一次构建内覆盖多个修调值。）
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "light.h"
#include "sensor_config.h"
#include "delay.h"
#include "mock_vdd_hw.h"

/* ---------------- mock MCU state (harness-owned) ---------------- */
GPIO_TypeDef mock_gpiob;
ADC_TypeDef  mock_adc;
volatile uint16_t mock_bgr_trim_value = 1200u;

static uint32_t init_channel[32]; static int init_samp[32]; static int init_calls;
static int bgr_at_conv_start[32]; static int conv_calls;
static int enable_calls, disable_calls, adc_enabled;
static int pending_timeout;        /* 1 = 本次转换永不置 EOC */
static uint16_t conv_value;
static uint16_t light_script[16]; static int light_script_n, light_script_i;
static int gpio_write_pin[8], gpio_write_val[8], gpio_write_n;

void delay_ms(uint16_t ms) { (void)ms; }

void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *cfg) { (void)port; (void)cfg; }
void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    (void)port;
    if (gpio_write_n < 8) { gpio_write_pin[gpio_write_n] = pin; gpio_write_val[gpio_write_n] = (int)state; gpio_write_n++; }
}

void ADC_Init(ADC_InitTypeDef *cfg)
{
    if (init_calls < 32) {
        init_channel[init_calls] = cfg->ADC_IN0.ADC_InputChannel;
        init_samp[init_calls]    = (int)cfg->ADC_IN0.ADC_SampTime;
    }
    init_calls++;
}
void ADC_ClearITPendingAll(void) { mock_adc.ISR = 0; }
void ADC_ClearITPendingBit(uint16_t it) { (void)it; mock_adc.ISR = 0; }
void ADC_Disable(void) { disable_calls++; adc_enabled = 0; }
ErrorStatus ADC_Enable(void) { enable_calls++; adc_enabled = 1; return SUCCESS; }
uint16_t ADC_GetConversionValue(uint32_t idx) { (void)idx; return conv_value; }

void ADC_SoftwareStartConvCmd(FunctionalState state)
{
    (void)state;
    if (conv_calls < 32) { bgr_at_conv_start[conv_calls] = (int)mock_adc.CR_f.BGREN; }
    conv_calls++;
    mock_adc.ISR = 0;
    if (pending_timeout) {
        return;                                  /* EOC 永不置位 -> 走有界超时分支 */
    }
    if (adc_enabled && light_script_i < light_script_n) {
        conv_value = light_script[light_script_i++];
    } else if (conv_value == 0u) {
        conv_value = 0u;
    }
    mock_adc.ISR = ADC_ISR_EOC_Msk;
}

/* ---------------- helpers ---------------- */
static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

static void reset_run(void)
{
    init_calls = conv_calls = enable_calls = disable_calls = 0;
    adc_enabled = 0; pending_timeout = 0; conv_value = 0;
    light_script_n = light_script_i = 0; gpio_write_n = 0;
    mock_adc.ISR = 0; mock_adc.CR_f.BGREN = 0;
}

/* 独立参考: 厂商示例公式的整数形式 + 6000 mV 上限 (double 逐步复算) */
static uint32_t ref_mv(uint32_t code, uint32_t trim_mv)
{
    double v = (4095.0 * (double)trim_mv) / (double)code;   /* mV */
    if (v > 6000.0) { v = 6000.0; }
    return (uint32_t)v;
}

int main(void)
{
    printf("==== independent verification of light_read_vdd_mv() (embedded_tester) ====\n");

    /* ---- [V1] 配置必须与厂商示例一致 (BGR 通道 / 采样时间 / ADCCLK) ---- */
    printf("[V1] BGR-channel ADC configuration (vendor example parity)\n");
    reset_run();
    { uint16_t mv = 0xFFFF, code = 0xFFFF, bgrmv = 0xFFFF;
      conv_value = 1480u;                        /* 一只正常转换的码 */
      uint8_t ok = light_read_vdd_mv(&mv, &code, &bgrmv);
      CHECK(ok == 1u, "returns 1 for a valid conversion");
      CHECK(init_calls == 1, "exactly one ADC_Init for the single measurement");
      CHECK(init_channel[0] == ADC_InputVref1P2, "channel = internal BGR1.2V (ADC_InputVref1P2)");
      CHECK(init_samp[0] == ADC_SampTime390Clk,
            "sample time = 390 clk (>= 40 us at ADCCLK=1 MHz, vendor BGR/TS requirement)");
      CHECK(conv_calls == 1, "exactly one conversion per call (bounded)");
    }

    /* ---- [V2] BGR 使能必须在转换之前, 且测量后关闭 (清理/低功耗) ---- */
    printf("[V2] BGREN raised before the conversion and cleared afterwards\n");
    reset_run();
    { uint16_t mv = 0, code = 0, bgrmv = 0;
      light_read_vdd_mv(&mv, &code, &bgrmv);
      CHECK(conv_calls >= 1 && bgr_at_conv_start[0] == 1, "BGREN == 1 when the conversion starts");
      CHECK(mock_adc.CR_f.BGREN == 0, "BGREN cleared after the measurement");
      CHECK(disable_calls >= 1 && adc_enabled == 0, "ADC left disabled after the measurement");
    }

    /* ---- [V3] 正常读数: 真实板上读到的同一点 (code=1480, trim=1189) ---- */
    printf("[V3] value path (real-board sample) and the two vendor-example arithmetic forms\n");
    {
        struct { uint16_t code, trim; uint32_t exp; } cases[] = {
            {1480u, 1189u, 3289u},   /* 与实板 VDD 行同一点 */
            {1632u, 1200u, 3011u},
            {4095u, 1200u, 1200u},
            {2048u, 1200u, 2399u},
            {100u,  1200u, 6000u},   /* 49140 mV -> clamped */
            {1200u, 1200u, 4095u},
        };
        for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
            uint16_t mv = 0, code = 0, bgrmv = 0;
            reset_run();
            mock_bgr_trim_value = cases[i].trim;
            conv_value = cases[i].code;
            uint8_t ok = light_read_vdd_mv(&mv, &code, &bgrmv);
            char m[160];
            snprintf(m, sizeof m, "code=%u trim=%u -> ok=1 mv=%u (independent ref=%u)",
                     cases[i].code, cases[i].trim, mv, ref_mv(cases[i].code, cases[i].trim));
            CHECK(ok == 1u && mv == cases[i].exp && mv == ref_mv(cases[i].code, cases[i].trim)
                  && code == cases[i].code && bgrmv == cases[i].trim, m);
        }
        mock_bgr_trim_value = 1200u;
    }

    /* ---- [V4] 公式扫描: 与 double 独立复算逐例一致 ---- */
    printf("[V4] formula sweep against an independent double reference\n");
    {
        const uint16_t codes[] = {1u, 7u, 512u, 1000u, 2048u, 3000u, 4094u, 4095u};
        const uint16_t trims[] = {1100u, 1189u, 1200u, 1250u};
        int bad = 0, n = 0;
        for (unsigned i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
            for (unsigned j = 0; j < sizeof(trims) / sizeof(trims[0]); j++) {
                uint16_t mv = 0, code = 0, bgrmv = 0;
                reset_run();
                mock_bgr_trim_value = trims[j];
                conv_value = codes[i];
                light_read_vdd_mv(&mv, &code, &bgrmv);
                n++;
                if (mv != ref_mv(codes[i], trims[j])) {
                    bad++;
                    printf("      mismatch code=%u trim=%u: fw=%u ref=%u\n",
                           codes[i], trims[j], mv, ref_mv(codes[i], trims[j]));
                }
            }
        }
        mock_bgr_trim_value = 1200u;
        CHECK(bad == 0, "mv == 4095*trim/code (clamped at 6000) for all 32 (code,trim) pairs");
    }

    /* ---- [V5] 除零保护: code=0 不得产生读数 ---- */
    printf("[V5] code==0 (divide-by-zero guard)\n");
    reset_run();
    { uint16_t mv = 0xFFFF, code = 0xFFFF, bgrmv = 0xFFFF;
      conv_value = 0u;
      uint8_t ok = light_read_vdd_mv(&mv, &code, &bgrmv);
      CHECK(ok == 0u, "returns 0 when the conversion code is 0");
      CHECK(mv == 0u && code == 0u && bgrmv == 1200u,
            "mv stays 0 (no fabricated voltage); code/bgrmv report the actual raw facts");
      CHECK(mock_adc.CR_f.BGREN == 0 && adc_enabled == 0, "BGREN/ADC still cleaned up");
    }

    /* ---- [V6] 转换超时: 有界等待 + 不伪造读数 + 清理 ---- */
    printf("[V6] EOC timeout (conversion never completes)\n");
    reset_run();
    { uint16_t mv = 0xFFFF, code = 0xFFFF, bgrmv = 0xFFFF;
      pending_timeout = 1;
      uint8_t ok = light_read_vdd_mv(&mv, &code, &bgrmv);
      CHECK(ok == 0u, "returns 0 on timeout (call returns -> the EOC wait is bounded)");
      CHECK(mv == 0u && code == 0u && bgrmv == 0u, "no fabricated reading on timeout");
      CHECK(mock_adc.CR_f.BGREN == 0, "BGREN cleared on the timeout path");
      CHECK(adc_enabled == 0, "ADC disabled on the timeout path");
      CHECK(LIGHT_ADC_EOC_GUARD <= 1000000uL, "EOC guard constant is bounded (config sanity)");
    }

    /* ---- [V7] 空指针保护 ---- */
    printf("[V7] NULL pointer guard\n");
    reset_run();
    { uint16_t mv = 0, code = 0, bgrmv = 0;
      uint8_t ok1 = light_read_vdd_mv(NULL, &code, &bgrmv);
      uint8_t ok2 = light_read_vdd_mv(&mv, NULL, &bgrmv);
      uint8_t ok3 = light_read_vdd_mv(&mv, &code, NULL);
      uint8_t ok4 = light_read_vdd_mv(NULL, NULL, NULL);
      CHECK(ok1 == 0u && ok2 == 0u && ok3 == 0u && ok4 == 0u,
            "all NULL-argument combinations return 0");
      CHECK(conv_calls == 0, "no conversion is started when an argument is NULL");
    }

    /* ---- [V8] 输出清零: 失败时不得残留调用者的旧值 ---- */
    printf("[V8] outputs are cleared before the conversion\n");
    reset_run();
    { uint16_t mv = 1234, code = 2345, bgrmv = 3456;
      pending_timeout = 1;
      light_read_vdd_mv(&mv, &code, &bgrmv);
      CHECK(mv == 0u && code == 0u && bgrmv == 0u, "stale caller values replaced by 0 on failure");
    }

    /* ---- [V9] 测量不破坏后续光照通路 (同一次运行内先 VDD 后 AIN11) ---- */
    printf("[V9] the light path still works after a VDD measurement\n");
    reset_run();
    { uint16_t mv = 0, code = 0, bgrmv = 0;
      conv_value = 1480u;
      light_read_vdd_mv(&mv, &code, &bgrmv);
      light_init();                              /* 真实初始化: PB05 输出/PB04 模拟 + AIN11 */
      CHECK(init_calls == 2 && init_channel[1] == ADC_InputCH11,
            "the light channel (AIN11) is re-configured after the BGR measurement");
      /* 8 次光照转换 */
      { uint16_t v[8] = {32u, 33u, 34u, 33u, 32u, 33u, 34u, 33u};
        for (int i = 0; i < 8; i++) { light_script[light_script_n++] = v[i]; }
      }
      light_result_t r = light_sample();
      CHECK(r.adc_ok && r.samples_ok == LIGHT_ADC_SAMPLES,
            "light_sample() still converts all 8 samples after the VDD measurement");
      CHECK(r.mean_adc_code == 33u, "mean of the scripted light samples (33)");
      { int raised = 0, lowered_after = 0;
        for (int i = 0; i < gpio_write_n; i++) {
            if (gpio_write_pin[i] == GPIO_PIN_5 && gpio_write_val[i] == 1) { raised = 1; }
            else if (raised && gpio_write_pin[i] == GPIO_PIN_5 && gpio_write_val[i] == 0) {
                lowered_after = 1;
            }
        }
        CHECK(raised && lowered_after,
              "PB05 raised for the light sample and lowered again (divider powered only while sampling)");
      }
      CHECK(LIGHT_DARK_CALIBRATED == 0 ? (r.valid == false) : true,
            "uncalibrated delivery: light validity stays false (no dark decision without calibration)");
    }

    printf("\n==== result: %d passed, %d failed ====\n", pass, fail);
    return fail == 0 ? 0 : 1;
}
