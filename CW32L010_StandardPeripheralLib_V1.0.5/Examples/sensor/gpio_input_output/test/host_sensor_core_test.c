/*
 * host_sensor_core_test.c - 传感器固件纯逻辑宿主机 L0 单元测试 (TD-001 6.1.3)
 *
 * 编译(在本目录):
 *   gcc -I../USER/inc -I../COMMON host_sensor_core_test.c ../USER/src/fw_core.c \
 *       ../USER/src/history.c -lm -o host_sensor_core_test
 *
 * 覆盖: CRC16-CCITT-FALSE、参数数值校验/换算、NVM 记录编解码、
 *       OPTCFG/1 帧解析校验、Manchester 解码器(含抖动/相位)、历史环/下降告警。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "fw_core.h"
#include "params.h"
#include "history.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond, msg) do { \
    if (cond) { g_pass++; printf("  PASS  %s\n", msg); } \
    else      { g_fail++; printf("  FAIL  %s (line %d)\n", msg, __LINE__); } \
} while (0)

#define CHECK_EQ_INT(a, b, msg) do { \
    long _a = (long)(a), _b = (long)(b); \
    if (_a == _b) { g_pass++; printf("  PASS  %s\n", msg); } \
    else { g_fail++; printf("  FAIL  %s: got %ld expect %ld (line %d)\n", msg, _a, _b, __LINE__); } \
} while (0)

#define CHECK_NEAR(a, b, msg) do { \
    double _a = (double)(a), _b = (double)(b); \
    if (fabs(_a - _b) < 1e-6) { g_pass++; printf("  PASS  %s\n", msg); } \
    else { g_fail++; printf("  FAIL  %s: got %f expect %f (line %d)\n", msg, _a, _b, __LINE__); } \
} while (0)

/* ------------------------------------------------------------------ */
/* 参考帧 (TD-001 6.1.0): 六值 [2.5,10,28,30,36,85], txn=1              */
/* ------------------------------------------------------------------ */
static const uint8_t g_ref_frame[OPTCFG_FRAME_LEN] = {
    0x44,0x42,0x01,0x01,0x18,0x01,0x00,0x00,0x00,
    0x00,0x00,0x20,0x40, 0x00,0x00,0x20,0x41,
    0x00,0x00,0xe0,0x41, 0x00,0x00,0xf0,0x41,
    0x00,0x00,0x10,0x42, 0x00,0x00,0xaa,0x42,
    0xb5,0x40
};

/* ------------------------------------------------------------------ */
/* 测试 1: CRC16-CCITT-FALSE                                            */
/* ------------------------------------------------------------------ */
static void test_crc(void)
{
    printf("[1] CRC16-CCITT-FALSE\n");
    CHECK_EQ_INT(fw_crc16_ccitt((const uint8_t *)"123456789", 9), 0x29B1u, "标准向量 123456789 -> 0x29B1");
    CHECK_EQ_INT(fw_crc16_ccitt(g_ref_frame, 33), 0xB540u, "参考帧前 33B -> 0xB540 (TD-001)");
}

/* ------------------------------------------------------------------ */
/* 测试 2: 参数数值校验 (FR-105, IC-001 2)                               */
/* ------------------------------------------------------------------ */
static void test_params_validate(void)
{
    printf("[2] params_validate_floats\n");
    float ok[6] = {2.5f, 10.0f, 28.0f, 30.0f, 36.0f, 85.0f};
    float v[6];
    int i;

    CHECK(params_validate_floats(ok), "合法向量通过");

    memcpy(v, ok, sizeof(v)); v[0] = NAN;        CHECK(!params_validate_floats(v), "NaN 拒绝");
    memcpy(v, ok, sizeof(v)); v[1] = INFINITY;   CHECK(!params_validate_floats(v), "+Inf 拒绝");
    memcpy(v, ok, sizeof(v)); v[2] = -INFINITY;  CHECK(!params_validate_floats(v), "-Inf 拒绝");
    memcpy(v, ok, sizeof(v)); v[3] = -0.0f;      CHECK(!params_validate_floats(v), "-0.0 拒绝");
    memcpy(v, ok, sizeof(v)); v[0] = 21.0f;      CHECK(!params_validate_floats(v), "temp_drop>20 拒绝");
    memcpy(v, ok, sizeof(v)); v[1] = 101.0f;     CHECK(!params_validate_floats(v), "hum_drop>100 拒绝");
    memcpy(v, ok, sizeof(v)); v[2] = -41.0f;     CHECK(!params_validate_floats(v), "temp_low<-40 拒绝");
    memcpy(v, ok, sizeof(v)); v[4] = 86.0f;      CHECK(!params_validate_floats(v), "temp_high>85 拒绝");
    memcpy(v, ok, sizeof(v)); v[2] = 40.0f;      CHECK(!params_validate_floats(v), "temp_low>=temp_high 拒绝");
    memcpy(v, ok, sizeof(v)); v[3] = 90.0f;      CHECK(!params_validate_floats(v), "hum_low>=hum_high 拒绝");
    memcpy(v, ok, sizeof(v)); v[0] = 0.0f;       CHECK(params_validate_floats(v), "temp_drop=0 边界通过");
    memcpy(v, ok, sizeof(v)); v[2] = -40.0f;     CHECK(params_validate_floats(v), "temp_low=-40 边界通过");

    /* x10 换算 */
    {
        int16_t x10[6];
        params_floats_to_x10(ok, x10);
        CHECK_EQ_INT(x10[0], 25,  "2.5 -> 25");
        CHECK_EQ_INT(x10[1], 100, "10.0 -> 100");
        CHECK_EQ_INT(x10[2], 280, "28.0 -> 280");
        CHECK_EQ_INT(x10[4], 360, "36.0 -> 360");
        float neg[6] = {0,0,-25.0f,0,0,0};
        params_floats_to_x10(neg, x10);
        CHECK_EQ_INT(x10[2], -250, "-25.0 -> -250 (int16 负温)");
    }
}

/* ------------------------------------------------------------------ */
/* 测试 3: NVM 记录编解码 (FR-108)                                      */
/* ------------------------------------------------------------------ */
static void test_record(void)
{
    printf("[3] params record build/parse\n");
    float v[6] = {2.5f, 10.0f, 28.0f, 30.0f, 36.0f, 85.0f};
    uint8_t rec[PARAMS_REC_LEN];
    float out[6];
    uint32_t txn = 0;

    params_build_record(v, 1u, rec);
    CHECK(params_parse_record(rec, out, &txn), "roundtrip 解析成功");
    CHECK_EQ_INT(txn, 1, "txn=1");
    for (int i = 0; i < 6; i++) CHECK_NEAR(out[i], v[i], "值一致");

    rec[10] ^= 0xFF;                            /* 破坏 float 区 */
    CHECK(!params_parse_record(rec, out, &txn), "CRC 破坏拒绝");
    rec[10] ^= 0xFF;
    rec[2] = 0x02;                              /* 版本不符 */
    CHECK(!params_parse_record(rec, out, &txn), "版本不符拒绝");
}

/* ------------------------------------------------------------------ */
/* 测试 4: OPTCFG/1 帧解析校验 (FR-104/105, IC-001 3.2/7)               */
/* ------------------------------------------------------------------ */
static void test_optcfg_parse(void)
{
    printf("[4] optcfg_parse_and_validate\n");
    float v[6];
    uint32_t txn = 0;
    uint8_t f[OPTCFG_FRAME_LEN];

    CHECK(optcfg_parse_and_validate(g_ref_frame, v, &txn), "参考帧通过");
    CHECK_EQ_INT(txn, 1, "txn=1");
    CHECK_NEAR(v[0], 2.5f, "temp_drop=2.5");
    CHECK_NEAR(v[1], 10.0f, "hum_drop=10.0");
    CHECK_NEAR(v[2], 28.0f, "temp_low=28.0");
    CHECK_NEAR(v[3], 30.0f, "hum_low=30.0");
    CHECK_NEAR(v[4], 36.0f, "temp_high=36.0");
    CHECK_NEAR(v[5], 85.0f, "hum_high=85.0");

    memcpy(f, g_ref_frame, OPTCFG_FRAME_LEN);
    f[33] ^= 0xFF;                              /* CRC 错 */
    CHECK(!optcfg_parse_and_validate(f, v, &txn), "CRC 错误拒绝");
    f[33] ^= 0xFF;

    memcpy(f, g_ref_frame, OPTCFG_FRAME_LEN); f[0] = 0x00;
    CHECK(!optcfg_parse_and_validate(f, v, &txn), "magic 错误拒绝");
    memcpy(f, g_ref_frame, OPTCFG_FRAME_LEN); f[2] = 0x02;
    CHECK(!optcfg_parse_and_validate(f, v, &txn), "version 0x02 拒绝");
    memcpy(f, g_ref_frame, OPTCFG_FRAME_LEN); f[3] = 0x02;
    CHECK(!optcfg_parse_and_validate(f, v, &txn), "message_type 0x02 拒绝");
    memcpy(f, g_ref_frame, OPTCFG_FRAME_LEN); f[4] = 0x17;
    CHECK(!optcfg_parse_and_validate(f, v, &txn), "payload_len 错误拒绝");

    /* 截断帧由解码器层处理(字节数不足不产生 frame_ready), 解析函数仅接受 35B */
    (void)f;

    /* 数值非法 payload: temp_drop=21.0 (0x0000A841 LE) */
    memcpy(f, g_ref_frame, OPTCFG_FRAME_LEN);
    f[9]=0x00; f[10]=0x00; f[11]=0xa8; f[12]=0x41;
    uint16_t c = fw_crc16_ccitt(f, 33);
    f[33] = (uint8_t)(c >> 8); f[34] = (uint8_t)(c & 0xFF);
    CHECK(!optcfg_parse_and_validate(f, v, &txn), "越界值(21.0)拒绝");
}

/* ------------------------------------------------------------------ */
/* Manchester 流生成(1ms 分辨率采样 10ms)                                */
/*   half_ms: 每半位时长(标称 20ms); 支持逐半位抖动                        */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t *samples;   /* 10ms 采样电平 */
    size_t   n;         /* 样本数 */
    uint32_t t_ms;      /* 当前时刻 */
} stream_t;

static void stream_emit(stream_t *s, int level, int dur_ms, int *cur)
{
    int end = s->t_ms + dur_ms;
    while (s->t_ms < end) {
        if ((int)(s->t_ms % 5) == 0) {           /* 5ms 采样点 */
            if (*cur < (int)s->n) s->samples[*cur] = (uint8_t)level;
            (*cur)++;
        }
        s->t_ms++;
    }
}

/* 生成一帧: 暗区 + 前导 16×0x55 + 35B 帧, Manchester 编码 */
static void build_frame_stream(const uint8_t frame[OPTCFG_FRAME_LEN],
                               const int half_jitter_ms[], int n_jitter,
                               int dark_before_ms, stream_t *s)
{
    int level = 0;
    s->t_ms = 0;
    s->n = (size_t)(dark_before_ms + OPTCFG_PREAMBLE_LEN*8*2*20 + OPTCFG_FRAME_LEN*8*2*20 + 600) / 5 + 16;
    s->samples = calloc(s->n, 1);
    int cur = 0;

    stream_emit(s, 0, dark_before_ms, &cur);     /* 暗区 */

    for (int i = 0; i < OPTCFG_PREAMBLE_LEN; i++) {
        for (int b = 7; b >= 0; b--) {
            int bit = (0x55 >> b) & 1;
            int h0 = (bit == 0) ? 1 : 0;         /* 首半: 0→亮(1), 1→灭(0) */
            int h1 = (bit == 0) ? 0 : 1;
            int j0 = half_jitter_ms ? half_jitter_ms[(cur) % n_jitter] : 20;
            int j1 = half_jitter_ms ? half_jitter_ms[(cur+1) % n_jitter] : 20;
            stream_emit(s, level = h0, j0, &cur);
            stream_emit(s, level = h1, j1, &cur);
        }
    }
    for (int i = 0; i < OPTCFG_FRAME_LEN; i++) {
        for (int b = 7; b >= 0; b--) {
            int bit = (frame[i] >> b) & 1;
            int h0 = (bit == 0) ? 1 : 0;
            int h1 = (bit == 0) ? 0 : 1;
            int j0 = half_jitter_ms ? half_jitter_ms[(cur) % n_jitter] : 20;
            int j1 = half_jitter_ms ? half_jitter_ms[(cur+1) % n_jitter] : 20;
            stream_emit(s, level = h0, j0, &cur);
            stream_emit(s, level = h1, j1, &cur);
        }
    }
    stream_emit(s, 0, 600, &cur);                /* 帧后暗区 */
    s->n = (size_t)cur;
}

static bool run_decoder(const uint8_t *samples, size_t n, uint8_t out[OPTCFG_FRAME_LEN])
{
    optcfg_decoder_t dec;
    optcfg_decoder_init(&dec);
    bool fr = false;
    for (size_t i = 0; i < n; i++) {
        optcfg_decoder_tick(&dec, samples[i], &fr);
        if (fr) {
            memcpy(out, dec.frame, OPTCFG_FRAME_LEN);
            return true;
        }
    }
    return false;
}

/* ------------------------------------------------------------------ */
/* 测试 5: Manchester 解码器 (FR-103, TD-OPT-007/008/009)               */
/* ------------------------------------------------------------------ */
static void test_decoder(void)
{
    printf("[5] Manchester 解码器\n");
    stream_t s = {0};
    uint8_t out[OPTCFG_FRAME_LEN];

    /* 标称 20ms 半位 */
    build_frame_stream(g_ref_frame, NULL, 0, 600, &s);
    CHECK(run_decoder(s.samples, s.n, out), "标称 20ms 半位: 收满一帧");
    if (memcmp(out, g_ref_frame, OPTCFG_FRAME_LEN) == 0) { g_pass++; printf("  PASS  标称帧字节一致\n"); }
    else { g_fail++; printf("  FAIL  标称帧字节不一致\n"); }
    free(s.samples);

    /* 抖动: 半位 15..25ms (±25%), 3 帧重发 → ≥1 帧通过 (TD-OPT-008) */
    {
        int ok_frames = 0;
        int jit[64];
        unsigned seed = 42;
        for (int i = 0; i < 64; i++) jit[i] = 15 + (int)((seed = seed*1103515245u+12345u) % 11u);
        for (int f = 0; f < 3; f++) {
            uint8_t of[OPTCFG_FRAME_LEN] = {0};
            float v[6]; uint32_t txn = 0;
            stream_t s2 = {0};
            build_frame_stream(g_ref_frame, jit, 64, 600, &s2);
            if (run_decoder(s2.samples, s2.n, of) &&
                optcfg_parse_and_validate(of, v, &txn) &&
                memcmp(of, g_ref_frame, OPTCFG_FRAME_LEN) == 0) ok_frames++;
            free(s2.samples);
        }
        printf("  抖动 3 帧通过 %d/3\n", ok_frames);
        CHECK(ok_frames >= 1, "±25% 抖动 3 帧内至少 1 帧通过");
    }

    /* 反相(互补电平): 应解码失败(magic 校验拒绝) */
    {
        build_frame_stream(g_ref_frame, NULL, 0, 600, &s);
        for (size_t i = 0; i < s.n; i++) s.samples[i] ^= 1u;
        bool got = run_decoder(s.samples, s.n, out);
        if (got) {
            float v[6]; uint32_t txn = 0;
            bool ok = optcfg_parse_and_validate(out, v, &txn);
            CHECK(!ok, "反相流不被接受(magic 消歧)");
        } else {
            CHECK(true, "反相流未收满帧");
        }
        free(s.samples);
    }
}

/* ------------------------------------------------------------------ */
/* 测试 6: 历史环与下降告警 (FR-202/204)                                 */
/* ------------------------------------------------------------------ */
static void test_history(void)
{
    printf("[6] history ring / drop alarm\n");
    history_init();
    CHECK_EQ_INT(history_count(), 0, "初始为空");

    for (int i = 0; i < 25; i++) history_push((int16_t)(250 + i), (uint16_t)(500 + i));
    CHECK_EQ_INT(history_count(), 20, "满 20 项");
    {
        history_entry_t e;
        CHECK(history_get(0, &e), "idx0 可读");
        CHECK_EQ_INT(e.temp_x10, 250 + 5, "最旧=第 6 项(回绕丢弃前 5 项)");
    }

    /* 下降告警: 历史有 30.0°C(300), 当前 27.0°C(270), temp_drop=3.0(30) → 触发 */
    history_init();
    history_push(300, 600);   /* 30.0C */
    CHECK(history_any_drop(270, 600, 30, 100), "下降 3.0C 触发 (300-270>=30)");
    CHECK(!history_any_drop(280, 600, 30, 100), "下降 2.0C 不触发 (300-280<30)");
    CHECK(history_any_drop(290, 600, 10, 100), "恰好等于阈值触发 (>=)");
    /* 湿度下降 */
    CHECK(history_any_drop(300, 500, 30, 100), "湿度下降触发 (600-500>=100)");
}

/* ------------------------------------------------------------------ */
int main(void)
{
    printf("==== sensor firmware core L0 host test ====\n");
    test_crc();
    test_params_validate();
    test_record();
    test_optcfg_parse();
    test_decoder();
    test_history();
    printf("==== result: %d passed, %d failed ====\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
