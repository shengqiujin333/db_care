/*
 * host_rf_frame_check.c - 433MHz 帧编码/解码互操作自检 (ITEM-008/ITEM-012, 宿主机)
 *
 * 直接编译**真实**的传感器编码器 USER/src/encrytogate.c、网关解码器与帧构造器
 * CH592EVT/.../beiwov2/APP/{feistel_al.c,bleencrypt.c}, 验证:
 *   - encode_frame10(uid,temp,hum) -> decode_frame10 往返一致 (uid_pick/temp/hum)
 *   - 线上布局 uid_pick(4) | temp_x10_LE(2) | hum_x10_LE(2) | crc16_LE(2) + Feistel 加密
 *     (若字节序或字段位置改变, 往返值会不一致 -> 失败)
 *   - 密文任一字节被篡改 -> decode 返回 false (CRC 校验)
 *   - 编码确定性
 *   - ITEM-012: 网关设备块 = [id | hum_be | temp_be] (app_um2006A.c 交换 + 真实
 *     build_device_block) -> Android parsePlainFrame(u16be@4/s16be@6) 映射一致
 *
 * 编译(在本目录; 网关 APP 目录路径按仓库相对位置):
 *   gcc -std=c11 -Wall -Wextra \
 *       -Wno-unused-function -Wno-unused-const-variable -Wno-misleading-indentation \
 *       -Wno-type-limits \
 *       -I../USER/inc -I../../../../../CH592EVT/EVT/EXAM/BLE/beiwov2/APP \
 *       host_rf_frame_check.c ../USER/src/encrytogate.c \
 *       ../../../../../CH592EVT/EVT/EXAM/BLE/beiwov2/APP/feistel_al.c \
 *       ../../../../../CH592EVT/EVT/EXAM/BLE/beiwov2/APP/bleencrypt.c \
 *       -o host_rf_frame_check.exe && ./host_rf_frame_check.exe
 *   (最后三个 -Wno-* 仅屏蔽既有 encrytogate.c/feistel_al.c 中的未用静态量/打印缩进告警)
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "encrytogate.h"   /* 传感器: encode_frame10 */
#include "feistel_al.h"    /* 网关:   decode_frame10 */
#include "bleencrypt.h"    /* 网关:   build_padded_frame_blocks */

/* 网关 bleencrypt.c 内部函数(未在其头文件声明); 仅在本测试中前向声明, 不修改网关代码 */
extern void build_device_block(const uint8_t payload8[DEV_PAYLOAD_LEN],
                               uint8_t out16[FRAME_BLOCK_SIZE]);

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("  PASS  %s\n", m); } \
                         else   { fail++; printf("  FAIL  %s\n", m); } } while (0)

static const uint8_t UID10[10] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0x11,0x22};

/* 传感器端 UID_IDX = {0,1,2,3}: 线上 uid_pick = UID10[0..3] */
static bool roundtrip(int16_t temp, uint16_t hum)
{
    uint8_t frame[10];
    uint8_t uid[4];
    int16_t  t = 0;
    uint16_t h = 0;

    encode_frame10((uint8_t *)UID10, temp, hum, frame);
    if (!decode_frame10(frame, uid, &t, &h)) return false;
    if (memcmp(uid, UID10, 4) != 0) return false;
    return (t == temp) && (h == hum);
}

int main(void)
{
    uint8_t frame[10], frame2[10];
    uint8_t uid[4];
    int16_t t;
    uint16_t h;
    int i;

    printf("[1] T-L0i-01 向量往返 (传感器 encode -> 网关 decode)\n");
    CHECK(roundtrip(-400, 0),    "temp_x10=-400, hum=0");
    CHECK(roundtrip(-100, 0),    "temp_x10=-100, hum=0 (负温符号)");
    CHECK(roundtrip(0, 0),       "temp_x10=0, hum=0");
    CHECK(roundtrip(250, 550),   "temp_x10=250, hum=550");
    CHECK(roundtrip(350, 1000),  "temp_x10=350, hum=1000");
    CHECK(roundtrip(1250, 1000), "temp_x10=1250, hum=1000");
    CHECK(roundtrip(-450, 0),    "int16 域外低 -450");
    CHECK(roundtrip(1300, 1000), "int16 域外高 1300");
    CHECK(roundtrip(32767, 65535), "int16/uint16 上极值");
    CHECK(roundtrip(-32768, 0),  "int16 下极值");

    printf("[2] 伪随机向量往返 (128 组)\n");
    {
        uint32_t s = 0x12345678u;
        int ok = 1;
        for (i = 0; i < 128; i++) {
            s = s * 1103515245u + 12345u;
            int16_t  rt = (int16_t)(s >> 16);
            uint16_t rh = (uint16_t)(s & 0xFFFFu);
            if (!roundtrip(rt, rh)) ok = 0;
        }
        CHECK(ok == 1, "128 组随机 (temp,hum) 全部往返一致");
    }

    printf("[3] 帧布局与字节序 (LE, 字段位置)\n");
    encode_frame10((uint8_t *)UID10, -100, 0x1234, frame);
    CHECK(decode_frame10(frame, uid, &t, &h) && t == -100 && h == 0x1234,
          "temp=-100/hum=0x1234 往返 -> 温度在 p[4..5] 小端、湿度在 p[6..7] 小端");

    printf("[4] 密文篡改 -> CRC 拒绝\n");
    {
        int all_rejected = 1;
        encode_frame10((uint8_t *)UID10, 250, 550, frame);
        for (i = 0; i < 10; i++) {
            uint8_t bad[10];
            memcpy(bad, frame, 10);
            bad[i] ^= 0x01u;
            if (decode_frame10(bad, uid, &t, &h)) all_rejected = 0;
        }
        CHECK(all_rejected == 1, "10 个字节逐一翻转后 decode 均返回 false");
    }

    printf("[5] 编码确定性\n");
    encode_frame10((uint8_t *)UID10, 250, 550, frame);
    encode_frame10((uint8_t *)UID10, 250, 550, frame2);
    CHECK(memcmp(frame, frame2, 10) == 0, "同一输入两次编码逐字节相同");

    printf("[6] ITEM-012 网关记录组装 + Android 解析映射\n");
    {
        const uint8_t GWID[6] = {0x3C,0x1A,0x40,0x7A,0x4E,0xE0};
        uint8_t payload8[8];
        uint8_t block16[16];
        uint8_t buf[32];
        int16_t  rt;
        uint16_t rh;
        size_t   need;
        int      humi, temp;

        /* 传感器 encode -> 网关 decode (均为真实实现) */
        encode_frame10((uint8_t *)UID10, -100, 0x1234, frame);
        CHECK(decode_frame10(frame, uid, &rt, &rh) && rt == -100 && rh == 0x1234,
              "传感器编码 -> 网关 decode: temp=-100 / hum=0x1234");

        /* app_um2006A.c: resbf[4..5]=temp_be, resbf[6..7]=hum_be;
         * sensorres 记录 = [id(4) | humidity_be(2) | temperature_be(2)] */
        payload8[0] = uid[0]; payload8[1] = uid[1]; payload8[2] = uid[2]; payload8[3] = uid[3];
        payload8[4] = (uint8_t)(rh >> 8); payload8[5] = (uint8_t)(rh & 0xFF);
        payload8[6] = (uint8_t)(rt >> 8); payload8[7] = (uint8_t)(rt & 0xFF);

        /* 真实网关帧构造器: 设备块前 8B = payload8 直拷 */
        build_device_block(payload8, block16);
        CHECK(memcmp(block16, payload8, 8) == 0, "build_device_block: 前 8B = payload8 直拷");

        /* Android parsePlainFrame: p8=block[0..8), humi=u16be@4, temp=s16be@6 */
        humi = ((int)block16[4] << 8) | (int)block16[5];
        temp = ((int)block16[6] << 8) | (int)block16[7];
        if (temp & 0x8000) temp -= 0x10000;
        CHECK(memcmp(block16, uid, 4) == 0, "Android devId(0..3) 一致");
        CHECK(humi == (int)rh, "Android humi = u16be@4 一致");
        CHECK(temp == (int)rt, "Android temp = s16be@6 一致 (负温符号保持)");

        /* 整帧: Header(16) + DeviceBlock(16)*N, Android 从 offset 16 起解析 */
        need = build_padded_frame_blocks((uint8_t *)GWID, 1u, payload8, buf, sizeof(buf));
        CHECK(need == 32u, "build_padded_frame_blocks: 1 设备 -> 32 B");
        CHECK(memcmp(buf, GWID, 6) == 0 && buf[6] == 1u, "Header: gwid6 + devCount@6");
        {
            int h2 = ((int)buf[16 + 4] << 8) | (int)buf[16 + 5];
            int t2 = ((int)buf[16 + 6] << 8) | (int)buf[16 + 7];
            if (t2 & 0x8000) t2 -= 0x10000;
            CHECK(memcmp(buf + 16, uid, 4) == 0 && h2 == (int)rh && t2 == (int)rt,
                  "Android 从 offset 16 解析: id/hum/temp 全部一致");
        }
    }

    printf("\n==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
