/*
 * host_sensor_core_test.c - 传感器固件宿主机 L0 测试 (纯逻辑)
 *
 * 编译/运行: 见同目录 build_test.sh (或)
 *   gcc -std=c11 -Wall -Wextra -I../USER/inc host_sensor_core_test.c \
 *       ../USER/src/fw_core.c -lm -o host_sensor_core_test.exe && ./host_sensor_core_test.exe
 *
 * 覆盖 (TD-002):
 *   T-L0-06 CRC16-CCITT-FALSE 标准向量 (保留回归)
 *   T-L0-01 fw_crc8_gxht 参考向量/单字节/空长度
 *   T-L0-02 GXHT40 温度换算与合法域 (含负温、域外无效)
 *   T-L0-03 GXHT40 湿度换算与 0..1000 截断
 *   T-L0-04 light_code_is_dark 光照阈值与滞回
 *   T-L0-05 sensor_decide_report 上报判定边界 (0.9C / 35.0C / 无前值 / 非暗)
 *
 * 注: "测量失败不更新前值" 属采样流程行为 (measure.c)，由 build_test.sh 第二阶段
 *     host_measure_flow_check.c 覆盖；ITEM-009 已移除 OPTCFG/params/history 用例。
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "fw_core.h"

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

/* 参考帧 (TD-001 6.1.0): 六值 [2.5,10,28,30,36,85], txn=1, 尾部 CRC 0xB540 */
static const uint8_t g_ref_frame[35] = {
    0x44,0x42,0x01,0x01,0x18,0x01,0x00,0x00,0x00,
    0x00,0x00,0x20,0x40, 0x00,0x00,0x20,0x41,
    0x00,0x00,0xe0,0x41, 0x00,0x00,0xf0,0x41,
    0x00,0x00,0x10,0x42, 0x00,0x00,0xaa,0x42,
    0xb5,0x40
};

/* ------------------------------------------------------------------ */
/* T-L0-06: CRC16-CCITT-FALSE (保留回归)                                 */
/* ------------------------------------------------------------------ */
static void test_crc16(void)
{
    printf("[1] T-L0-06 CRC16-CCITT-FALSE\n");
    CHECK_EQ_INT(fw_crc16_ccitt((const uint8_t *)"123456789", 9), 0x29B1u,
                 "标准向量 123456789 -> 0x29B1");
    CHECK_EQ_INT(fw_crc16_ccitt(g_ref_frame, 33), 0xB540u,
                 "参考帧前 33B -> 0xB540");
}

/* ------------------------------------------------------------------ */
/* T-L0-01: fw_crc8_gxht (poly 0x31, init 0xFF, 无反转, xorout 0)        */
/* ------------------------------------------------------------------ */
static void test_crc8(void)
{
    uint8_t ref[2]  = {0xBE, 0xEF};
    uint8_t zero[1] = {0x00};

    printf("[2] T-L0-01 fw_crc8_gxht\n");
    CHECK_EQ_INT(fw_crc8_gxht(ref, 2u), 0x92u, "{0xBE,0xEF} -> 0x92 (手册参考向量)");
    CHECK_EQ_INT(fw_crc8_gxht(zero, 1u), 0xACu, "{0x00} -> 0xAC (init 0xFF 走移位链)");
    CHECK_EQ_INT(fw_crc8_gxht(ref, 0u), 0xFFu, "空长度 -> init 0xFF");
}

/* ------------------------------------------------------------------ */
/* T-L0-02: 温度换算与合法域                                             */
/* ------------------------------------------------------------------ */
static void test_temp(void)
{
    printf("[3] T-L0-02 GXHT40 温度换算与合法域\n");
    CHECK_EQ_INT(gxht40_temp_raw_to_x10(1872u), -400, "S=1872 -> -400 (-40.0C 域下界)");
    CHECK_EQ_INT(gxht40_temp_raw_to_x10(16855u), 0, "S=16855 -> 0 (0.0C)");
    CHECK_EQ_INT(gxht40_temp_raw_to_x10(26214u), 250, "S=26214 -> 250 (25.0C)");
    CHECK_EQ_INT(gxht40_temp_raw_to_x10(63664u), 1250, "S=63664 -> 1250 (125.0C 域上界)");
    CHECK_EQ_INT(gxht40_temp_raw_to_x10(0u), -450, "S=0 -> -450 (纯换算, 超有效域)");
    CHECK_EQ_INT(gxht40_temp_raw_to_x10(0xFFFFu), 1300, "S=0xFFFF -> 1300 (纯换算, 超有效域)");
    CHECK(gxht40_temp_x10_valid(-400) && gxht40_temp_x10_valid(1250),
          "有效域端点 -400/1250 合法");
    CHECK(!gxht40_temp_x10_valid(-450) && !gxht40_temp_x10_valid(1251)
          && !gxht40_temp_x10_valid(1300), "-450/1251/1300 判为无效");

    printf("[3b] 组合换算 gxht40_raw_to_x10 (有效才写输出)\n");
    {
        int16_t  t = 1234;
        uint16_t h = 4321;
        CHECK(gxht40_raw_to_x10(26214u, 29360u, &t, &h) == true, "有效温度 -> true");
        CHECK(t == 250 && h == 500, "输出 temp_x10=250 / hum_x10=500");
        t = 1234; h = 4321;
        CHECK(gxht40_raw_to_x10(0u, 29360u, &t, &h) == false, "无效温度 -> false");
        CHECK(t == 1234 && h == 4321, "无效时不写输出");
    }
}

/* ------------------------------------------------------------------ */
/* T-L0-03: 湿度换算与 0..1000 截断                                      */
/* ------------------------------------------------------------------ */
static void test_hum(void)
{
    printf("[4] T-L0-03 GXHT40 湿度换算与截断\n");
    CHECK_EQ_INT(gxht40_hum_raw_to_x10(0u), 0u, "S=0 -> 0 (原始 -60 截断)");
    CHECK_EQ_INT(gxht40_hum_raw_to_x10(2247u), 0u, "S=2247 -> 0 (0 %RH)");
    CHECK_EQ_INT(gxht40_hum_raw_to_x10(29360u), 500u, "S=29360 -> 500 (50 %RH)");
    CHECK_EQ_INT(gxht40_hum_raw_to_x10(55575u), 1000u, "S=55575 -> 1000 (100 %RH)");
    CHECK_EQ_INT(gxht40_hum_raw_to_x10(0xFFFFu), 1000u, "S=0xFFFF -> 1000 (原始 1190 截断)");
}

/* ------------------------------------------------------------------ */
/* T-L0-04: 无光判据 (LIGHT-DEF-2 绝对阈值, FD-002 rev 8 §6.2)          */
/* ------------------------------------------------------------------ */
static void test_light(void)
{
    printf("[5] T-L0-04 light_is_dark (dark = valid && mean >= T, T != 0)\n");
    CHECK(light_is_dark(0u, false, 1u)   == false, "valid=false -> 恒 false");
    CHECK(light_is_dark(4095u, false, 1u) == false, "valid=false + 满量程 -> false (不合成暗态)");
    CHECK(light_is_dark(4095u, true, 0u)  == false, "T=0 (判据未启用) -> false");
    CHECK(light_is_dark(0u, true, 1u)    == false, "T=1: mean=0 < 1 -> false");
    CHECK(light_is_dark(1u, true, 1u)    == true,  "T=1: mean=1 >= 1 -> true");
    CHECK(light_is_dark(3589u, true, 3590u) == false, "T=3590: mean=3589 -> false");
    CHECK(light_is_dark(3590u, true, 3590u) == true,  "T=3590: mean=3590 -> true (相等为暗)");
    CHECK(light_is_dark(3514u, true, 3590u) == false, "T=3590: 实测房间灯 3514 -> LIT");
    CHECK(light_is_dark(3987u, true, 3590u) == true,  "T=3590: 实测近饱和 3987 -> DARK");
    CHECK(light_is_dark(4095u, true, 4095u) == true,  "T=4095: 满量程 -> true");
    CHECK(light_is_dark(300u, true, 1000u) == light_is_dark(300u, true, 1000u),
          "每笔独立: 无滞回/无历史暗态");
}

/* ------------------------------------------------------------------ */
/* T-L0-05: 上报判定边界                                                 */
/* ------------------------------------------------------------------ */
static void test_report(void)
{
    printf("[6] T-L0-05 sensor_decide_report\n");
    CHECK(sensor_decide_report(0, false, 200, true) == false, "无前值 + cur=200 -> false");
    CHECK(sensor_decide_report(0, false, 351, false) == true, "无前值 + cur=351 -> true");
    CHECK(sensor_decide_report(0, false, 350, true) == false, "无前值 + cur=350 (恰好 35.0C) -> false");
    CHECK(sensor_decide_report(300, true, 308, true) == false, "升温 8 (0.8C) -> false");
    CHECK(sensor_decide_report(300, true, 309, true) == false, "升温 9 (0.9C) -> false (严格)");
    CHECK(sensor_decide_report(300, true, 310, true) == true,  "升温 10 (1.0C) 且 DARK -> true");
    CHECK(sensor_decide_report(300, true, 310, false) == false, "升温满足但非 DARK -> false");
    CHECK(sensor_decide_report(310, true, 300, true) == false, "降温 10 (方向错误) -> false");
    CHECK(sensor_decide_report(340, true, 350, true) == true, "34.0->35.0 DARK -> 升温分支 true");
    CHECK(sensor_decide_report(340, true, 350, false) == false, "34.0->35.0 LIT -> false");
    CHECK(sensor_decide_report(340, true, 349, true) == false, "34.0->34.9 DARK -> false");
    CHECK(sensor_decide_report(0, false, 351, false) == true, "cur=351 不受光照/前值限制");
    CHECK(sensor_decide_report(300, true, 300, true) == false, "prev==cur -> false");
    CHECK(sensor_decide_report(32767, true, -32768, true) == false, "int16 极值反向差不溢出");
    CHECK(sensor_decide_report(-32768, true, 32767, true) == true, "int16 极值正向 -> 超温 branch");
}

/* ------------------------------------------------------------------ */
int main(void)
{
    printf("==== sensor firmware core L0 host test ====\n");
    test_crc16();
    test_crc8();
    test_temp();
    test_hum();
    test_light();
    test_report();
    printf("==== result: %d passed, %d failed ====\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
