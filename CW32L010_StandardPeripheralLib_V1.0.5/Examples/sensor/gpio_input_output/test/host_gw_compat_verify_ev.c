/*
 * Independent verification harness for ITEM-012: gateway (CH592 beiwov2) compatibility.
 * Author: embedded_tester.embedded_verification.
 *
 * Links the REAL sensor encoder (USER/src/encrytogate.c) and the REAL gateway decoder
 * (CH592EVT/.../beiwov2/APP/feistel_al.c), then replays - byte for byte - the gateway's
 * BLE record assembly (APP/app_um2006A.c) and block packing (APP/bleencrypt.c), and the
 * Android parser (MqtttService.kt parsePlainFrame / u16be / s16be), asserting that
 * temperature / humidity / sensor-id attribution survives the whole chain.
 *
 * Build (from gpio_input_output/):
 *   GW=../../../CH592EVT/EVT/EXAM/BLE/beiwov2/APP
 *   gcc -std=c11 -Wall -Wextra -Wno-unused-function -Wno-unused-const-variable \
 *       -Wno-misleading-indentation -I USER/inc -I $GW test/host_gw_compat_verify_ev.c \
 *       USER/src/encrytogate.c $GW/feistel_al.c -o gw_compat_verify
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "encrytogate.h"
#include "feistel_al.h"

/* ---------------- gateway: APP/bleencrypt.c (exact formulas) ---------------- */
#define FRAME_BLOCK_SIZE 16
#define GWID_LEN         6
#define DEV_PAYLOAD_LEN  8

static uint8_t rol1(uint8_t v) { return (uint8_t)(((v << 1) | (v >> 7)) & 0xFF); }
static size_t frame_len_from_devcount(uint8_t n) { return FRAME_BLOCK_SIZE * (1u + (size_t)n); }

static void build_header_block(const uint8_t gwid6[GWID_LEN], uint8_t dc, uint8_t out16[FRAME_BLOCK_SIZE])
{
    out16[0] = gwid6[0]; out16[1] = gwid6[1]; out16[2] = gwid6[2];
    out16[3] = gwid6[3]; out16[4] = gwid6[4]; out16[5] = gwid6[5];
    out16[6] = dc;
    uint8_t g0 = gwid6[0], g1 = gwid6[1], g2 = gwid6[2], g3 = gwid6[3], g4 = gwid6[4], g5 = gwid6[5];
    uint8_t s7 = (uint8_t)(g0 + g1 + g2 + g3 + g4 + g5 + dc);
    uint8_t x7 = (uint8_t)(g0 ^ g1 ^ g2 ^ g3 ^ g4 ^ g5 ^ dc);
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

static void build_device_block(const uint8_t payload8[DEV_PAYLOAD_LEN], uint8_t out16[FRAME_BLOCK_SIZE])
{
    memcpy(out16, payload8, DEV_PAYLOAD_LEN);
    uint8_t p0 = payload8[0], p1 = payload8[1], p2 = payload8[2], p3 = payload8[3];
    uint8_t p4 = payload8[4], p5 = payload8[5], p6 = payload8[6], p7 = payload8[7];
    uint8_t s8 = (uint8_t)(p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7);
    uint8_t x8 = (uint8_t)(p0 ^ p1 ^ p2 ^ p3 ^ p4 ^ p5 ^ p6 ^ p7);
    out16[8]  = s8;
    out16[9]  = x8;
    out16[10] = (uint8_t)((p0 + p3) ^ p6);
    out16[11] = (uint8_t)(p1 + p4 + s8);
    out16[12] = (uint8_t)(p2 ^ p5 ^ x8);
    out16[13] = (uint8_t)(((uint8_t)(p7 << 1)) + p0);
    out16[14] = (uint8_t)((uint8_t)(p1 * p2) + (p3 | p4));
    out16[15] = (uint8_t)((x8 + s8) ^ p7);
}

static size_t build_padded_frame_blocks(const uint8_t gwid6[GWID_LEN], uint8_t dc,
                                        const uint8_t *payloads, uint8_t *out, size_t cap)
{
    size_t need = frame_len_from_devcount(dc);
    if (cap < need) return 0;
    build_header_block(gwid6, dc, out);
    for (uint8_t i = 0; i < dc; i++) build_device_block(payloads + (size_t)i * DEV_PAYLOAD_LEN, out + FRAME_BLOCK_SIZE * (1u + i));
    return need;
}

/* ---------------- gateway: APP/app_um2006A.c record assembly (exact ops) ----------------
 * decode_frame10 -> uid[4], tempervalue (int16), humivalue (uint16)
 * resbf[0..3]=uid; resbf[4..5]=temp BE; resbf[6..7]=hum BE
 * record = id(4) | resbf[6..7] (hum BE) | resbf[4..5] (temp BE)
 */
static void gw_build_record(const uint8_t uid4[4], int16_t temp, uint16_t hum, uint8_t rec8[8])
{
    uint8_t resbf[8];
    resbf[0] = uid4[0]; resbf[1] = uid4[1]; resbf[2] = uid4[2]; resbf[3] = uid4[3];
    resbf[4] = (uint8_t)(0xff & (temp >> 8));
    resbf[5] = (uint8_t)(temp & 0xff);
    resbf[6] = (uint8_t)(0xff & (hum >> 8));
    resbf[7] = (uint8_t)(hum & 0xff);
    rec8[0] = resbf[0]; rec8[1] = resbf[1]; rec8[2] = resbf[2]; rec8[3] = resbf[3];
    rec8[4] = resbf[6]; rec8[5] = resbf[7];   /* humidity, big endian */
    rec8[6] = resbf[4]; rec8[7] = resbf[5];   /* temperature, big endian */
}

/* ---------------- Android: MqtttService.kt (exact ops) ---------------- */
static unsigned u16be(const uint8_t *b, int i) { return ((unsigned)b[i] << 8) | b[i + 1]; }
static int      s16be(const uint8_t *b, int i) { int v = (int)u16be(b, i); return (v & 0x8000) ? v - 0x10000 : v; }

typedef struct { uint8_t id[4]; int hum_x10; int temp_x10; } android_reading_t;

static int android_parse(const uint8_t *plain, size_t n, android_reading_t *out, int maxout)
{
    if (n < 16) return -1;
    int devCount = plain[6];
    size_t expected = 16u * (1u + (size_t)devCount);
    if (n < expected) return -2;
    int off = 16;
    for (int i = 0; i < devCount && i < maxout; i++) {
        const uint8_t *p8 = plain + off;
        memcpy(out[i].id, p8, 4);
        out[i].hum_x10  = (int)u16be(p8, 4);     /* humidity */
        out[i].temp_x10 = s16be(p8, 6);          /* temperature (signed) */
        off += 16;
    }
    return devCount;
}

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

int main(void)
{
    printf("==== independent gateway/Android compatibility verification (embedded_tester) ====\n");
    const uint8_t gwid[GWID_LEN] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    /* value sets chosen so a temperature/humidity swap is always detectable */
    static const int16_t  T[] = { 250, -100, -400, 1250, 351, 0, 289, 350 };
    static const uint16_t H[] = { 500, 1000,    0,  100,   0, 7, 999, 500 };
    const int NV = (int)(sizeof(T) / sizeof(T[0]));

    /* ---- T1: sensor encoder -> gateway decoder -> gateway record -> Android parse ---- */
    printf("[T1] full chain: sensor 433 frame -> gateway decode -> BLE record -> Android parse\n");
    { int ok = 0;
      for (int i = 0; i < NV; i++) {
          uint8_t uid[10];
          for (int k = 0; k < 10; k++) uid[k] = (uint8_t)(0xA0 + k);
          uint8_t frame10[10]; uint8_t uid4[4]; int16_t t = 0; uint16_t h = 0;
          encode_frame10(uid, T[i], H[i], frame10);
          if (!decode_frame10(frame10, uid4, &t, &h)) continue;
          if (t != T[i] || h != H[i]) continue;
          uint8_t rec8[8]; gw_build_record(uid4, t, h, rec8);
          uint8_t payloads[8]; memcpy(payloads, rec8, 8);
          uint8_t frame[16 * 2]; size_t n = build_padded_frame_blocks(gwid, 1, payloads, frame, sizeof frame);
          if (n != 32) continue;
          android_reading_t r[1];
          if (android_parse(frame, n, r, 1) != 1) continue;
          if (memcmp(r[0].id, uid, 4) == 0 && r[0].temp_x10 == T[i] && r[0].hum_x10 == (int)H[i]) ok++;
      }
      CHECK(ok == NV, "all vectors survive the whole chain (id / temperature / humidity)"); }

    /* ---- T2: multi-device aggregation and attribution ---- */
    printf("[T2] multi-device frame: per-device attribution and block offsets\n");
    { const int N = 4;
      uint8_t payloads[N * 8]; android_reading_t expect[N];
      for (int i = 0; i < N; i++) {
          uint8_t uid[10]; for (int k = 0; k < 10; k++) uid[k] = (uint8_t)(0x10 * (i + 1) + k);
          uint8_t frame10[10], uid4[4]; int16_t t = 0; uint16_t h = 0;
          encode_frame10(uid, T[i], H[i], frame10);
          if (!decode_frame10(frame10, uid4, &t, &h)) { CHECK(0, "decode failed"); return 1; }
          gw_build_record(uid4, t, h, payloads + i * 8);
          memcpy(expect[i].id, uid, 4); expect[i].temp_x10 = T[i]; expect[i].hum_x10 = H[i];
      }
      uint8_t frame[16 * 6]; size_t n = build_padded_frame_blocks(gwid, (uint8_t)N, payloads, frame, sizeof frame);
      CHECK(n == 16u * (1u + N), "frame length = 16*(1+devCount), matching Android 'expected'");
      CHECK(memcmp(frame, gwid, 6) == 0 && frame[6] == N, "header block: gwid6 at [0..5], devCount at [6]");
      android_reading_t got[8];
      int cnt = android_parse(frame, n, got, 8);
      CHECK(cnt == N, "Android parsed exactly devCount devices");
      { int allok = 1;
        for (int i = 0; i < N; i++)
            if (memcmp(got[i].id, expect[i].id, 4) != 0 || got[i].temp_x10 != expect[i].temp_x10 ||
                got[i].hum_x10 != expect[i].hum_x10) allok = 0;
        CHECK(allok, "every device's id / temperature / humidity attributed correctly (no ordering mix-up)"); }
      /* the 16-byte block pad must not disturb the 8-byte payload */
      CHECK(memcmp(frame + 16, payloads, 8) == 0, "device block starts with the 8-byte record verbatim"); }

    /* ---- T3: negative control - the check can detect a swapped layout ---- */
    printf("[T3] negative control: the old [id|temp_be|hum_be] order must be detected as wrong\n");
    { uint8_t uid[10]; for (int k = 0; k < 10; k++) uid[k] = (uint8_t)(0xA0 + k);
      uint8_t frame10[10], uid4[4]; int16_t t = 0; uint16_t h = 0;
      encode_frame10(uid, -100, 1000, frame10);      /* temp=-10.0C, hum=100.0% */
      decode_frame10(frame10, uid4, &t, &h);
      uint8_t oldrec[8];
      oldrec[0] = uid4[0]; oldrec[1] = uid4[1]; oldrec[2] = uid4[2]; oldrec[3] = uid4[3];
      oldrec[4] = (uint8_t)(0xff & (t >> 8)); oldrec[5] = (uint8_t)(t & 0xff);   /* OLD: temp first */
      oldrec[6] = (uint8_t)(0xff & (h >> 8)); oldrec[7] = (uint8_t)(h & 0xff);   /* OLD: hum second */
      uint8_t frame[32]; build_padded_frame_blocks(gwid, 1, oldrec, frame, sizeof frame);
      android_reading_t r[1]; android_parse(frame, 32, r, 1);
      CHECK(!(r[0].temp_x10 == t && r[0].hum_x10 == (int)h),
            "a swapped record is NOT read correctly -> the positive result above is meaningful"); }

    /* ---- T4: frame length rule vs Android for several device counts ---- */
    printf("[T4] frame length rule for devCount = 0..5\n");
    { int ok = 1;
      for (int dc = 0; dc <= 5; dc++)
          if (frame_len_from_devcount((uint8_t)dc) != (size_t)(16 * (1 + dc))) ok = 0;
      CHECK(ok, "gateway 16*(1+devCount) == Android expected 16*(1+devCount)"); }

    /* ---- T5: sensor id bytes are passed through unchanged ---- */
    printf("[T5] sensor id passthrough (encoder uid[0..3] -> gateway -> BLE record -> Android)\n");
    { uint8_t uid[10] = {0xDE,0xAD,0xBE,0xEF,0x01,0x02,0x03,0x04,0x05,0x06};
      uint8_t frame10[10], uid4[4]; int16_t t = 0; uint16_t h = 0;
      encode_frame10(uid, 250, 500, frame10);
      CHECK(decode_frame10(frame10, uid4, &t, &h), "gateway decode accepted");
      CHECK(memcmp(uid4, uid, 4) == 0, "air id = encoder uid[0..3] (gateway copies p[0..3] verbatim)");
      uint8_t rec8[8]; gw_build_record(uid4, t, h, rec8);
      CHECK(memcmp(rec8, uid, 4) == 0, "record id bytes unchanged"); }

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
