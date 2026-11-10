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

    printf("[5] T-L0-04 光照: LIGHT-DEF-2 绝对阈值判据 (light_is_dark)\n");
    CHECK(light_is_dark(4095u, false, 1u) == false, "valid=false -> 恒 false");
    CHECK(light_is_dark(4095u, true, 0u) == false, "threshold=0 (判据未启用) -> false");
    CHECK(light_is_dark(0u, true, 1u) == false, "T=1: mean=0 < 1 -> false");
    CHECK(light_is_dark(1u, true, 1u) == true, "T=1: mean=1 >= 1 -> true (边界相等为暗)");
    CHECK(light_is_dark(3589u, true, 3590u) == false, "T=3590: mean=3589 -> false");
    CHECK(light_is_dark(3590u, true, 3590u) == true, "T=3590: mean=3590 -> true");
    CHECK(light_is_dark(3514u, true, 3590u) == false, "T=3590: 实测房间灯 3514 -> LIT");
    CHECK(light_is_dark(3987u, true, 3590u) == true, "T=3590: 实测近饱和 3987 -> DARK");
    CHECK(light_is_dark(4094u, true, 4095u) == false, "T=4095: mean=4094 -> false");
    CHECK(light_is_dark(4095u, true, 4095u) == true, "T=4095: 仅满量程为暗");
    CHECK(light_is_dark(300u, true, 1000u) == light_is_dark(300u, true, 1000u),
          "每笔独立: 同输入同输出 (无滞回/无历史暗态)");
    CHECK(LIGHT_DARK_THRESHOLD_CODE == 0u,
          "本交付件判据未启用 (T=0, 不得当作已启用/已验收)");
    {
        uint16_t th;
        int bad = 0;
        for (th = 1u; th <= 4095u; th++) {
            if (light_is_dark((uint16_t)(th - 1u), true, th) != false) bad++;
            if (light_is_dark(th, true, th) != true) bad++;
        }
        CHECK(bad == 0, "T=1..4095 全量: mean=T 为暗, 再低 1 不是暗");
    }

    printf("[6] T-L0-05 上报判定真值表\n");
    CHECK(sensor_decide_report(0, false, 200, true) == false, "无前值 + cur=200 -> false");
    CHECK(sensor_decide_report(0, false, 351, false) == true, "无前值 + cur=351 -> true");
    CHECK(sensor_decide_report(0, false, 350, true) == false, "无前值 + cur=350 (恰好 35.0 C) -> false");
    CHECK(sensor_decide_report(300, true, 308, true) == false, "升温 8 (0.8 C) -> false");
    CHECK(sensor_decide_report(300, true, 309, true) == false, "升温 9 (0.9 C) -> false (严格)");
    CHECK(sensor_decide_report(300, true, 310, true) == true, "升温 10 (1.0 C) 且 DARK -> true");
    CHECK(sensor_decide_report(300, true, 310, false) == false, "升温满足但非 DARK -> false");
    CHECK(sensor_decide_report(310, true, 300, true) == false, "降温 10 (方向错误) -> false");
    CHECK(sensor_decide_report(340, true, 350, true) == true, "34.0->35.0 DARK -> 升温分支 true");
    CHECK(sensor_decide_report(340, true, 350, false) == false, "34.0->35.0 LIT -> false");
    CHECK(sensor_decide_report(340, true, 349, true) == false, "34.0->34.9 DARK -> false");
    CHECK(sensor_decide_report(0, false, 351, false) == true, "cur=351 不受光照/前值限制");
    CHECK(sensor_decide_report(300, true, 300, true) == false, "prev==cur -> false");
    CHECK(sensor_decide_report(32767, true, -32768, true) == false, "int16 极值反向差不溢出");
    CHECK(sensor_decide_report(-32768, true, 32767, true) == true, "int16 极值正向 -> 超温 branch");

    printf("\n==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
