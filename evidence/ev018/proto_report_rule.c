/*
 * proto_report_rule.c - 固件可行性探索用最小原型 (非产品代码)
 *
 * 目的: 用 PDR-001 PR-004 的 14 个规范测试向量，对比
 *   (a) 当前产品纯逻辑 sensor_decide_report() 的实际返回；
 *   (b) 按 PR-004 字面判据实现的参考函数（升温 > 0.9C 且无光，或 > 35.0C）。
 * 只链接 USER/src/fw_core.c（无 MCU 依赖），不修改任何产品文件。
 *
 * 编译 (MinGW gcc):
 *   gcc -std=c11 -Wall -Wextra -I ../../CW32L010_.../USER/inc \
 *       proto_report_rule.c ../../CW32L010_.../USER/src/fw_core.c -o /tmp/proto_report_rule
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "fw_core.h"

typedef struct {
    const char *id;
    int16_t prev;
    bool have_prev;
    int16_t cur;
    bool dark;
    bool expected;
    bool applicable;   /* false = 该向量不属于本纯函数（失败路径不调用判定） */
} vec_t;

/* PR-004 规范判据的独立参考实现（只用于对照，不是交付实现） */
static bool spec_decide(int16_t prev, bool have_prev, int16_t cur, bool dark)
{
    if (cur > 350) {
        return true;                    /* 超温分支独立于光照/前值 */
    }
    if (!have_prev) {
        return false;
    }
    if (!dark) {
        return false;
    }
    return ((int32_t)cur - (int32_t)prev) > 9;   /* 升温严格大于 0.9C */
}

int main(void)
{
    static const vec_t v[] = {
        {"v1",  300, true,  308, true,  false, true },
        {"v2",  300, true,  309, true,  false, true },
        {"v3",  300, true,  310, true,  true,  true },
        {"v4",  300, true,  310, false, false, true },
        {"v5",  300, true,  310, false, false, true },   /* 光照无效 => dark 必须为 false */
        {"v6",  310, true,  300, true,  false, true },
        {"v7",  340, true,  350, true,  true,  true },
        {"v8",  340, true,  350, false, false, true },
        {"v9",    0, false, 351, false, true,  true },
        {"v10",   0, false, 350, true,  false, true },
        {"v11",   0, false, 360, false, true,  true },
        {"v12", 352, true,  351, false, true,  true },
        {"v13", 300, true,  310, false, false, true },   /* ADC 全失败 => valid=false,dark=false */
        {"v14", 300, true,  310, true,  false, false},   /* 采样失败时根本不调用判定 */
    };
    unsigned i;
    unsigned cur_fail = 0u;
    unsigned spec_fail = 0u;

    printf("id  prev have cur dark expected current spec\n");
    for (i = 0u; i < sizeof(v) / sizeof(v[0]); i++) {
        bool cur_ret  = sensor_decide_report(v[i].prev, v[i].have_prev, v[i].cur, v[i].dark);
        bool spec_ret = spec_decide(v[i].prev, v[i].have_prev, v[i].cur, v[i].dark);
        const char *spec_s = v[i].applicable ? (spec_ret ? "1" : "0") : "n/a";
        if (v[i].applicable && cur_ret != v[i].expected)  { cur_fail++; }
        if (v[i].applicable && spec_ret != v[i].expected) { spec_fail++; }
        printf("%-4s %4d %d %4d %d   %d        %d       %-3s%s%s\n",
               v[i].id, v[i].prev, v[i].have_prev ? 1 : 0, v[i].cur,
               v[i].dark ? 1 : 0, v[i].expected, cur_ret, spec_s,
               (v[i].applicable && cur_ret != v[i].expected) ? "  <-- CURRENT-MISMATCH" : "",
               (v[i].applicable && spec_ret != v[i].expected) ? "  <-- SPEC-MISMATCH" : "");
    }
    printf("current_mismatches=%u spec_mismatches=%u\n", cur_fail, spec_fail);
    /* v14: 采样失败路径由 measure.c 决定“不调用判定”，两个函数都不参与，applicable=false。 */
    return (cur_fail == 3u && spec_fail == 0u) ? 0 : 1;
}
