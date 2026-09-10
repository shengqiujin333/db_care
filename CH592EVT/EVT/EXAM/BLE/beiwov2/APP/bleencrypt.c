
#include "bleencrypt.h"

static inline uint8_t rol1(uint8_t v) { return (uint8_t)(((v << 1) | (v >> 7)) & 0xFF); }

static inline size_t frame_len_from_devcount(uint8_t dev_count) {
    // 1 个头块 + N 个设备块
    return FRAME_BLOCK_SIZE * (1u + (size_t)dev_count);
}

/* 生成头块：gwid6(6) | dev_count(1) | pad9(派生)  → out16[16] */
void build_header_block(const uint8_t gwid6[GWID_LEN], uint8_t dev_count, uint8_t out16[FRAME_BLOCK_SIZE]) {
    // 前7字节
    out16[0] = gwid6[0];
    out16[1] = gwid6[1];
    out16[2] = gwid6[2];
    out16[3] = gwid6[3];
    out16[4] = gwid6[4];
    out16[5] = gwid6[5];
    out16[6] = dev_count;

    uint8_t g0 = gwid6[0], g1 = gwid6[1], g2 = gwid6[2];
    uint8_t g3 = gwid6[3], g4 = gwid6[4], g5 = gwid6[5];
    uint8_t dc = dev_count;

    uint8_t s7 = (uint8_t)(g0 + g1 + g2 + g3 + g4 + g5 + dc);
    uint8_t x7 = (uint8_t)(g0 ^ g1 ^ g2 ^ g3 ^ g4 ^ g5 ^ dc);

    // 派生9字节 Pad → out16[7..15]
    out16[7]  = s7;
    out16[8]  = x7;
    out16[9]  = (uint8_t)(rol1(g0) ^ g5);
    out16[10] = (uint8_t)(g1 + g4 + dc);
    out16[11] = (uint8_t)(g2 ^ (uint8_t)(g0 + g5));
    out16[12] = (uint8_t)(g3 + (uint8_t)(g2 << 1));
    out16[13] = (uint8_t)((uint8_t)(g0 * g2) + (g3 ^ dc));
    out16[14] = (uint8_t)((x7 + s7) ^ g4);
    out16[15] = (uint8_t)((s7 - dc) ^ g1);
}

/* 生成设备块：payload8(8) | pad8(派生) → out16[16] */
void build_device_block(const uint8_t payload8[DEV_PAYLOAD_LEN], uint8_t out16[FRAME_BLOCK_SIZE]) {
    // 前8字节 = 有效载荷
    memcpy(out16, payload8, DEV_PAYLOAD_LEN);

    uint8_t p0 = payload8[0], p1 = payload8[1], p2 = payload8[2], p3 = payload8[3];
    uint8_t p4 = payload8[4], p5 = payload8[5], p6 = payload8[6], p7 = payload8[7];

    uint8_t s8 = (uint8_t)(p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7);
    uint8_t x8 = (uint8_t)(p0 ^ p1 ^ p2 ^ p3 ^ p4 ^ p5 ^ p6 ^ p7);

    // 派生8字节 Pad → out16[8..15]
    out16[8]  = s8;
    out16[9]  = x8;
    out16[10] = (uint8_t)((p0 + p3) ^ p6);
    out16[11] = (uint8_t)(p1 + p4 + s8);
    out16[12] = (uint8_t)(p2 ^ p5 ^ x8);
    out16[13] = (uint8_t)(((uint8_t)(p7 << 1)) + p0);
    out16[14] = (uint8_t)((uint8_t)(p1 * p2) + (p3 | p4));
    out16[15] = (uint8_t)((x8 + s8) ^ p7);
}

/* 组整帧（明文，尚未加密）：Header(16) + DeviceBlock(16)*N
   payloads: 连续 N*8 字节（按设备顺序排）
   返回写入长度；失败返回0 */
size_t build_padded_frame_blocks(uint8_t gwid6[GWID_LEN],
                                 uint8_t dev_count,
                                 uint8_t *payloads, /* len = dev_count*8 */
                                 uint8_t *out_buf,
                                 size_t out_cap)
{
    if (!gwid6 || (!payloads && dev_count>0) || !out_buf) return 0;
    if (dev_count > MAX_DEV_COUNT) return 0;

    size_t need = frame_len_from_devcount(dev_count);
    if (out_cap < need) return 0;

    uint8_t *p = out_buf;

    // 头块
    build_header_block(gwid6, dev_count, p);
    p += FRAME_BLOCK_SIZE;

    // 设备块
    for (uint8_t i = 0; i < dev_count; ++i) {
        build_device_block(payloads + (size_t)i * DEV_PAYLOAD_LEN, p);
        p += FRAME_BLOCK_SIZE;
    }
    return need;
}
