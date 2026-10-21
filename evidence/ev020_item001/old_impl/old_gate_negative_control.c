/*
 * old_gate_negative_control.c
 *
 * T-L0-06 旧实现负向对照：从 Git 8ae4a33 只读导出的 fw_core.c/fw_core.h/
 * sensor_config.h（旧降温语义、cur==350 提前返回），以改名符号编入独立进程。
 * 目的：证明 14 向量能区分新旧语义；若旧实现也“全过”，则向量或调用方式无效。
 *
 * 编译：-Dsensor_decide_report=old_sensor_decide_report
 * 输出：O <idx> <old_got> <spec_exp>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "fw_core.h"

static const int8_t kSv[14]   = {1,1,1,1,1,1,1,1,1,1,1,1,1,0};
static const int8_t kHp[14]   = {1,1,1,1,1,1,1,1,0,0,0,1,1,0};
static const int16_t kPrev[14] = {300,300,300,300,300,310,340,340,0,0,0,352,300,0};
static const int16_t kCur[14]  = {308,309,310,310,310,300,350,350,351,350,360,351,310,0};
static const int8_t kLv[14]   = {1,1,1,1,0,1,1,1,0,1,0,0,0,0};
static const int8_t kDark[14] = {1,1,1,0,0,1,1,0,0,1,0,0,0,0};
static const int8_t kExp[14]  = {0,0,1,0,0,0,1,0,1,0,1,1,0,0};

int main(void)
{
    int i;
    for (i = 0; i < 14; i++) {
        int got = 0;
        if (kSv[i]) {
            got = old_sensor_decide_report(kPrev[i], kHp[i] ? true : false,
                                           kCur[i], (kLv[i] && kDark[i]) ? true : false)
                      ? 1 : 0;
        }
        printf("O %d %d %d\n", i + 1, got, kExp[i]);
    }
    return 0;
}
