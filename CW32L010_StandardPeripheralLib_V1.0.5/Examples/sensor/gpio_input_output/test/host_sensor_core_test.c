/*
 * host_sensor_core_test.c - 传感器固件纯逻辑宿主机 L0 单元测试
 *
 * 编译(在本目录):
 *   gcc -I../USER/inc host_sensor_core_test.c ../USER/src/fw_core.c -lm \
 *       -o host_sensor_core_test && ./host_sensor_core_test
 *
 * ITEM-009: 随 hall/OPTCFG/params/history 模块退役, 已移除参数校验/换算/NVM 记录、
 *           OPTCFG/1 帧解析与 Manchester 解码器、历史环/下降告警用例；
 *           保留 CRC16-CCITT-FALSE 标准向量回归。
 * ITEM-010: 将补充本次需求新增纯逻辑用例 (CRC-8/换算/光照滞回/上报判定)。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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

#define CHECK_NEAR(a, b, msg) do { \
    double _a = (double)(a), _b = (double)(b); \
    if (fabs(_a - _b) < 1e-6) { g_pass++; printf("  PASS  %s\n", msg); } \
    else { g_fail++; printf("  FAIL  %s: got %f expect %f (line %d)\n", msg, _a, _b, __LINE__); } \
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
/* 测试 1: CRC16-CCITT-FALSE (保留的既有纯逻辑回归)                       */
/* ------------------------------------------------------------------ */
static void test_crc(void)
{
    printf("[1] CRC16-CCITT-FALSE\n");
    CHECK_EQ_INT(fw_crc16_ccitt((const uint8_t *)"123456789", 9), 0x29B1u, "标准向量 123456789 -> 0x29B1");
    CHECK_EQ_INT(fw_crc16_ccitt(g_ref_frame, 33), 0xB540u, "参考帧前 33B -> 0xB540 (TD-001)");
}

/* ------------------------------------------------------------------ */
int main(void)
{
    printf("==== sensor firmware core L0 host test ====\n");
    test_crc();
    printf("==== result: %d passed, %d failed ====\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
