/*
 * optcfg.c - OPTCFG/1 光学配置窗口状态机 (FR-101..109, FD-001 6.2)
 *
 * 解码器/帧校验纯逻辑在 fw_core.c(宿主机 L0 可测);
 * 本文件仅保留窗口状态机: LPTIM 10ms 采样 PB05, PB06 控制 F2 供电(低=使能)。
 */
#include "optcfg.h"
#include "fw_core.h"
#include "params.h"
#include <string.h>
#include "cw32l010_gpio.h"
#include "cw32l010_lptim.h"
#include "cw32l010_sysctrl.h"

static optcfg_decoder_t s_dec;
static uint8_t  s_window_active;
static uint8_t  s_commit_pending;
static uint8_t  s_stop_request;
static uint8_t  s_fail_count;
static uint32_t s_window_ms;
static float    s_pending_v[6];
static uint32_t s_pending_txn;

/* LPTIM 10 ms 采样 */
static void optcfg_lptim_start_sampling(void)
{
    LPTIM_InitTypeDef LPTIM_InitStruct = {0};
    uint32_t pclk = SYSCTRL_GetPClkFreq();
    uint32_t div = 1;
    uint32_t ticks = pclk / 100u;                 /* 10 ms @pclk */
    while (ticks > 0xFFFEu && div < 128u) {
        div <<= 1;
        ticks = pclk / div / 100u;
    }
    if (ticks > 0xFFFEu) ticks = 0xFFFEu;
    uint32_t prs = 0;                             /* log2(div) */
    { uint32_t t = div; while ((t & 1u) == 0u) { t >>= 1; prs++; } }

    LPTIM_InitStruct.LPTIM_ClockSource = LPTIM_CLOCK_SOURCE_MCLK;
    LPTIM_InitStruct.LPTIM_CounterMode = LPTIM_COUNTER_MODE_TIME;
    LPTIM_InitStruct.LPTIM_Period      = ticks - 1;
    LPTIM_InitStruct.LPTIM_Prescaler   = (uint32_t)(prs << 9);
    LPTIM_Init(&LPTIM_InitStruct);
    LPTIM_InternalClockConfig(LPTIM_ICLK_PCLK);
    LPTIM_ITConfig(LPTIM_IT_ARRM, ENABLE);
    CW_LPTIM->ICR = 0x00;
    LPTIM_Cmd(ENABLE);
    LPTIM_SelectOnePulseMode(LPTIM_OPERATION_REPETITIVE);
    NVIC_EnableIRQ(LPTIM_IRQn);
}

static void optcfg_lptim_stop_sampling(void)
{
    LPTIM_Cmd(DISABLE);
    NVIC_DisableIRQ(LPTIM_IRQn);
    CW_LPTIM->ICR = 0x00;
}

void optcfg_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PB06: 推挽输出, 默认高 → F2 断电 (SCH-001 3.2, FR-507) */
    GPIO_InitStruct.Pins = F2_PWR_EN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(OPT_GPIO_PORT, &GPIO_InitStruct);
    GPIO_WritePin(OPT_GPIO_PORT, F2_PWR_EN_PIN, GPIO_Pin_SET);

    /* PB05: 光敏数字输入(施密特) */
    GPIO_InitStruct.Pins = OPT_IN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.IT   = GPIO_IT_NONE;
    GPIO_Init(OPT_GPIO_PORT, &GPIO_InitStruct);

    s_window_active  = 0;
    s_commit_pending = 0;
    s_stop_request   = 0;
    s_fail_count     = 0;
    s_window_ms      = 0;
    optcfg_decoder_init(&s_dec);
}

bool optcfg_window_active(void) { return s_window_active != 0; }
bool optcfg_commit_pending(void) { return s_commit_pending != 0; }

void optcfg_window_start(void)
{
    if (s_window_active) return;               /* 窗口内忽略再次触发 (FR-102) */
    s_window_active  = 1;
    s_commit_pending = 0;
    s_stop_request   = 0;
    s_fail_count     = 0;
    s_window_ms      = 0;
    optcfg_decoder_init(&s_dec);
    GPIO_WritePin(OPT_GPIO_PORT, F2_PWR_EN_PIN, GPIO_Pin_RESET);  /* F2 上电 */
    optcfg_lptim_start_sampling();
}

void optcfg_lptim_isr(void)
{
    uint8_t level;
    bool fr = false;

    if (!s_window_active) return;
    s_window_ms += OPTCFG_SAMPLE_MS;
    if (s_window_ms >= OPTCFG_WINDOW_MS) {      /* 窗口超时 (TD-OPT-004) */
        s_stop_request = 1;
        return;
    }
    level = (GPIO_ReadPin(OPT_GPIO_PORT, OPT_IN_PIN) == GPIO_Pin_SET) ? 1u : 0u;
    optcfg_decoder_tick(&s_dec, level, &fr);
    if (fr) {
        float v[6];
        uint32_t txn = 0;
        if (optcfg_parse_and_validate(s_dec.frame, v, &txn)) {
            memcpy(s_pending_v, v, sizeof(v));
            s_pending_txn = txn;
            s_commit_pending = 1;               /* 主循环提交(Flash 写) */
        } else {
            s_fail_count++;                     /* 整帧失败 (TD-OPT-010..013) */
            if (s_fail_count >= OPTCFG_MAX_FAIL_FRAMES) s_stop_request = 1;
        }
    }
}

void optcfg_process(void)
{
    if (!s_window_active) return;
    if (s_commit_pending) {
        /* 原子提交(FR-106/107/108); 无光学回传 → 提交即结束会话 */
        (void)params_commit(s_pending_v, s_pending_txn);
        optcfg_lptim_stop_sampling();
        GPIO_WritePin(OPT_GPIO_PORT, F2_PWR_EN_PIN, GPIO_Pin_SET);  /* F2 断电 */
        s_window_active  = 0;
        s_commit_pending = 0;
        s_stop_request   = 0;
        s_fail_count     = 0;
        s_window_ms      = 0;
        return;
    }
    if (s_stop_request) {
        optcfg_lptim_stop_sampling();
        GPIO_WritePin(OPT_GPIO_PORT, F2_PWR_EN_PIN, GPIO_Pin_SET);  /* F2 断电 */
        s_window_active  = 0;
        s_stop_request   = 0;
        s_commit_pending = 0;
        s_fail_count     = 0;
        s_window_ms      = 0;
    }
}
