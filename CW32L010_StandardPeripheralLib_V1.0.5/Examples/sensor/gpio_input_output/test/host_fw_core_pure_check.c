/*
 * host_fw_core_pure_check.c - fw_core.c 新增纯逻辑的宿主机自检 (ITEM-005)
 *
 * 覆盖 TD-002 T-L0-01..T-L0-05:
 *   T-L0-01 fw_crc8_gxht        参考向量/单字节/空长度
 *   T-L0-02 温度换算与合法域       -40.0/0/25/125 C 与 S=0/0xFFFF 无效域
 *   T-L0-03 湿度换算与截断         0%RH/50%RH/100%RH/S=0xFFFF
 *   T-L0-04 光照滞回状态机         阈值端点/带内不抖动
 *   T-L0-05 上报判定真值表         无前值/下降边界/需 DARK/35.0C 边界
 *
 * 编译(在本目录):
 *   gcc -std=c11 -Wall -Wextra -I../USER/inc host_fw_core_pure_check.c \
 *       ../USER/src/fw_core.c -lm -o host_fw_core_pure_check.exe \
 *       && ./host_fw_core_pure_check.exe
 *
 * 帧/换算向量由独立工具按 gxht40.pdf §7.3/§7.5 计算 (非本实现)。
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "fw_core.h"

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("  PASS  %s\n", m); } \
                         else   { fail++; printf("  FAIL  %s\n", m); } } while (0)

int main(void)
{
    uint8_t  ref[2] = {0xBE, 0xEF};
    uint8_t  zero[1] = {0x00};
    int16_t  t;
    uint16_t h;

    printf("[1] T-L0-01 fw_crc8_gxht (poly 0x31, init 0xFF)\n");
    CHECK(fw_crc8_gxht(ref, 2u) == 0x92u, "{0xBE,0xEF} -> 0x92 (手册参考向量)");
    CHECK(fw_crc8_gxht(zero, 1u) == 0xACu, "{0x00} -> 0xAC (init 0xFF 走移位链)");
    CHECK(fw_crc8_gxht(ref, 0u) == 0xFFu, "空长度 -> init 0xFF");

    printf("[2] T-L0-02 温度换算与合法域\n");
    CHECK(gxht40_temp_raw_to_x10(1872u) == -400, "S=1872 -> -400 (-40.0 C 边界)");
    CHECK(gxht40_temp_raw_to_x10(16855u) == 0, "S=16855 -> 0 (0.0 C)");
    CHECK(gxht40_temp_raw_to_x10(26214u) == 250, "S=26214 -> 250 (25.0 C)");
    CHECK(gxht40_temp_raw_to_x10(63664u) == 1250, "S=63664 -> 1250 (125.0 C 边界)");
    CHECK(gxht40_temp_raw_to_x10(0u) == -450, "S=0 -> -450 (纯换算, 超有效域)");
    CHECK(gxht40_temp_raw_to_x10(0xFFFFu) == 1300, "S=0xFFFF -> 1300 (纯换算, 超有效域)");
    CHECK(gxht40_temp_x10_valid(-400) && gxht40_temp_x10_valid(1250), "有效域端点 -400/1250 合法");
    CHECK(!gxht40_temp_x10_valid(-450) && !gxht40_temp_x10_valid(1300) && !gxht40_temp_x10_valid(1251),
          "-450/1251/1300 判为无效");

    printf("[3] T-L0-03 湿度换算与截断 (0..1000)\n");
    CHECK(gxht40_hum_raw_to_x10(0u) == 0u, "S=0 -> 0 (原始 -60 截断)");
    CHECK(gxht40_hum_raw_to_x10(2247u) == 0u, "S=2247 -> 0 (0 %RH)");
    CHECK(gxht40_hum_raw_to_x10(29360u) == 500u, "S=29360 -> 500 (50 %RH)");
    CHECK(gxht40_hum_raw_to_x10(55575u) == 1000u, "S=55575 -> 1000 (100 %RH)");
    CHECK(gxht40_hum_raw_to_x10(0xFFFFu) == 1000u, "S=0xFFFF -> 1000 (原始 1190 截断)");

    printf("[4] 组合换算 gxht40_raw_to_x10: 有效才写输出\n");
    t = 1234; h = 4321;
    CHECK(gxht40_raw_to_x10(26214u, 29360u, &t, &h) == true, "有效温度 -> true");
    CHECK(t == 250 && h == 500, "输出 temp_x10=250 / hum_x10=500");
    t = 1234; h = 4321;
    CHECK(gxht40_raw_to_x10(0u, 29360u, &t, &h) == false, "无效温度 -> false");
    CHECK(t == 1234 && h == 4321, "无效时不写输出");

    printf("[5] T-L0-04 光照: 完全无光基准 1/3 判据 (light_is_dark)\n");
    CHECK(light_is_dark(0u, false, 1u) == false, "valid=false -> 恒 false (即使 3*mean>=C_dark)");
    CHECK(light_is_dark(4095u, false, 1u) == false, "valid=false + 满量程 -> false (不合成暗态)");
    CHECK(light_is_dark(0u, true, 1u) == false, "C_dark=1: 3*0=0 < 1 -> false");
    CHECK(light_is_dark(1u, true, 1u) == true, "C_dark=1: 3*1=3 >= 1 -> true");
    CHECK(light_is_dark(1364u, true, 4095u) == false, "C_dark=4095: 3*1364=4092 < 4095 -> false");
    CHECK(light_is_dark(1365u, true, 4095u) == true, "C_dark=4095: 3*1365=4095 = C_dark -> true (相等为暗)");
    CHECK(light_is_dark(4095u, true, 4095u) == true, "C_dark=4095: 满量程 -> true");
    CHECK(light_is_dark(1000u, true, 3000u) == true && light_is_dark(999u, true, 3000u) == false,
          "C_dark=3000: 边界相等为暗, 低 1 不为暗");
    CHECK(light_is_dark(300u, true, 1000u) == light_is_dark(300u, true, 1000u),
          "每笔独立: 同输入同输出 (无滞回/无历史暗态)");
    CHECK(LIGHT_DARK_CALIBRATED == 0 && LIGHT_DARK_REF_CODE == 0u,
          "本交付件为未标定状态 (C_dark 占位 0, 不得当作已验收)");
    {
        uint16_t cd;
        int bad = 0;
        for (cd = 1u; cd <= 4095u; cd++) {
            uint32_t thr = ((uint32_t)cd + 2u) / 3u;          /* ceil(C_dark/3) */
            if (light_is_dark((uint16_t)(thr - 1u), true, cd) != false) bad++;
            if (light_is_dark((uint16_t)thr, true, cd) != true) bad++;
        }
        CHECK(bad == 0, "C_dark=1..4095 全量: mean=ceil(C/3) 为暗, 再低 1 不是暗");
    }

    printf("[6] T-L0-05 上报判定真值表\n");
    CHECK(sensor_decide_report(0, false, 200, true) == false, "无前值 + cur=200 -> false");
    CHECK(sensor_decide_report(0, false, 351, false) == true, "无前值 + cur=351 -> true");
    CHECK(sensor_decide_report(300, true, 291, true) == false, "下降 9 (0.9 C) -> false");
    CHECK(sensor_decide_report(300, true, 290, true) == true, "下降 10 (1.0 C) -> true");
    CHECK(sensor_decide_report(300, true, 289, true) == true, "下降 11 (1.1 C) -> true");
    CHECK(sensor_decide_report(300, true, 290, false) == false, "下降满足但非 DARK -> false");
    CHECK(sensor_decide_report(360, true, 350, true) == false, "cur=350 (恰好 35.0 C) -> false");
    CHECK(sensor_decide_report(0, false, 351, false) == true, "cur=351 不受光照/前值限制");
    CHECK(sensor_decide_report(300, true, 300, true) == false, "prev==cur -> false");

    printf("\n==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
