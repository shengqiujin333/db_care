/*
 * host_fw_core_gate_verify_item001.c
 *
 * 嵌入式测试能力独立验证 harness（ITEM-001 条件上报门控纯逻辑）。
 * 被测对象：当前工作树 USER/src/fw_core.c（真实产品实现）+ USER/inc/fw_core.h。
 *
 * 独立性约定：
 *  - 期望值不取自实现侧自检；14 向量期望来自 FDS-003 §5.2 / IC-003 IF02。
 *  - 全网格只输出原始结果，由独立 Python 参考模型（按契约重新表述）逐点比对。
 *  - 本文件不修改产品源码，不把实现侧 harness 当证据。
 *
 * 输出为机器可解析的行：
 *   V  <idx> <sv> <hp> <prev> <cur> <lv> <dark> <got> <exp>
 *   G  <sv> <hp> <prev> <cur> <lv> <dark> <gate> <rep4>
 *   E  <sv> <hp> <prev> <cur> <lv> <dark> <gate> <rep4>   (仅不一致时)
 *   B  <name> <gate> <exp>
 *   C  <rise_x10> <high_x10>
 *   SELF <vector_fail> <equiv_fail> <boundary_fail>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "fw_core.h"

/* 只取纯数值配置，不引入 MCU 外设头 */
#ifndef SENSOR_CONFIG_NO_MCU
#define SENSOR_CONFIG_NO_MCU
#endif
#include "sensor_config.h"

typedef struct {
    int sv;
    int hp;
    int prev;
    int cur;
    int lv;
    int dark;
    int exp;
} vec_t;

static const vec_t kVec[14] = {
    {1, 1,  300, 308, 1, 1, 0},   /* v1  +0.8C 不报 */
    {1, 1,  300, 309, 1, 1, 0},   /* v2  +0.9C 不报（严格 >9） */
    {1, 1,  300, 310, 1, 1, 1},   /* v3  +1.0C 暗 报（旧实现 0） */
    {1, 1,  300, 310, 1, 0, 0},   /* v4  有光不报 */
    {1, 1,  300, 310, 0, 0, 0},   /* v5  光照无效不报 */
    {1, 1,  310, 300, 1, 1, 0},   /* v6  降温不报（旧实现 1） */
    {1, 1,  340, 350, 1, 1, 1},   /* v7  恰好 35.0 继续升温分支（旧实现 0） */
    {1, 1,  340, 350, 1, 0, 0},   /* v8  35.0 但有光 */
    {1, 0,    0, 351, 0, 0, 1},   /* v9  超温独立于前值/光照 */
    {1, 0,    0, 350, 1, 1, 0},   /* v10 无前值且非超温 */
    {1, 0,    0, 360, 0, 0, 1},   /* v11 超温 */
    {1, 1,  352, 351, 0, 0, 1},   /* v12 35.2->35.1 仍超温 */
    {1, 1,  300, 310, 0, 0, 0},   /* v13 ADC 全失败（light invalid） */
    {0, 0,    0,   0, 0, 0, 0}    /* v14 采集无效：不判定 */
};

/* 结构化网格取值：含 int16 边界、35.0 邻界、0.9C 邻界 */
static const int kPrev[] = {-32768, -400, -1, 0, 9, 340, 349, 350, 351, 1250, 32767};
static const int kCur[]  = {-32768, -400, -1, 0, 9, 340, 349, 350, 351, 1250, 32767};

static int call_gate(int sv, int hp, int prev, int cur, int lv, int dark)
{
    return sensor_gate_decide(sv ? true : false,
                              (int16_t)prev, hp ? true : false,
                              (int16_t)cur,
                              lv ? true : false, dark ? true : false)
               ? 1 : 0;
}

static int call_rep4(int hp, int prev, int cur, int dark)
{
    return sensor_decide_report((int16_t)prev, hp ? true : false,
                                (int16_t)cur, dark ? true : false)
               ? 1 : 0;
}

int main(void)
{
    int vector_fail = 0, equiv_fail = 0, boundary_fail = 0;
    int i, a, b, c, d, p, q;

    printf("C %d %d\n", (int)SENSOR_REPORT_RISE_X10, (int)SENSOR_REPORT_HIGH_X10);

    /* 1. 14 规范向量 */
    for (i = 0; i < 14; i++) {
        int got = call_gate(kVec[i].sv, kVec[i].hp, kVec[i].prev,
                            kVec[i].cur, kVec[i].lv, kVec[i].dark);
        printf("V %d %d %d %d %d %d %d %d %d\n",
               i + 1, kVec[i].sv, kVec[i].hp, kVec[i].prev, kVec[i].cur,
               kVec[i].lv, kVec[i].dark, got, kVec[i].exp);
        if (got != kVec[i].exp) {
            vector_fail++;
        }
    }

    /* 2. 全网格原始结果 + 4 参兼容等价性 */
    for (a = 0; a < 2; a++) {            /* sample_valid */
        for (b = 0; b < 2; b++) {        /* have_prev */
            for (c = 0; c < 2; c++) {    /* light_valid */
                for (d = 0; d < 2; d++) { /* dark */
                    for (p = 0; p < 11; p++) {
                        for (q = 0; q < 11; q++) {
                            int got = call_gate(a, b, kPrev[p], kCur[q], c, d);
                            int rep4 = call_rep4(b, kPrev[p], kCur[q], c && d);
                            printf("G %d %d %d %d %d %d %d %d\n",
                                   a, b, kPrev[p], kCur[q], c, d, got, rep4);
                            /* 4 参函数应等价于 sample_valid=1 的包装 */
                            if (rep4 != got) {
                                printf("E %d %d %d %d %d %d %d %d\n",
                                       a, b, kPrev[p], kCur[q], c, d, got, rep4);
                                if (a == 1) {
                                    equiv_fail++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /* 3. 显式边界用例（在网格之外的语义点） */
    struct {
        const char *name;
        int sv, hp, prev, cur, lv, dark, exp;
    } bnd[] = {
        {"int32_pos_overflow_safe", 1, 1, -32768, 32767, 1, 1, 1},
        {"int32_neg_overflow_safe", 1, 1,  32767, -32768, 1, 1, 0},
        {"rise_exact_9",            1, 1,    300,   309, 1, 1, 0},
        {"rise_exact_10",           1, 1,    300,   310, 1, 1, 1},
        {"high_exact_350_no_prev",  1, 0,      0,   350, 1, 1, 0},
        {"high_exact_350_rise",     1, 1,    340,   350, 1, 1, 1},
        {"high_exact_350_lv_only",  1, 1,    340,   350, 1, 0, 0},
        {"high_exact_350_dark_only",1, 1,    340,   350, 0, 1, 0},
        {"high_351_no_light",       1, 0,      0,   351, 0, 0, 1},
        {"valid0_high_360",         0, 0,      0,   360, 0, 0, 0},
        {"valid0_rise",             0, 1,    300,   310, 1, 1, 0},
        {"cool_exact_minus_10",     1, 1,    310,   300, 1, 1, 0},
        {"rise_min_temp_range",     1, 1,   -400,  -390, 1, 1, 1}
    };
    for (i = 0; i < (int)(sizeof(bnd) / sizeof(bnd[0])); i++) {
        int got = call_gate(bnd[i].sv, bnd[i].hp, bnd[i].prev, bnd[i].cur,
                            bnd[i].lv, bnd[i].dark);
        printf("B %s %d %d\n", bnd[i].name, got, bnd[i].exp);
        if (got != bnd[i].exp) {
            boundary_fail++;
        }
    }

    printf("SELF %d %d %d\n", vector_fail, equiv_fail, boundary_fail);

    if ((vector_fail != 0) || (equiv_fail != 0) || (boundary_fail != 0)) {
        return 1;
    }
    return 0;
}
