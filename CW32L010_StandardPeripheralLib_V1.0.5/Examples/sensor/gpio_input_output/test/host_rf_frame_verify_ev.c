/*
 * Independent verification harness for ITEM-008: 433 frame layout / byte order / Feistel
 * encryption unchanged, verified against the REAL gateway decoder.
 * Author: embedded_tester.embedded_verification.
 *
 * Links the sensor encoder (USER/src/encrytogate.c) with the gateway decoder
 * (CH592EVT/EVT/EXAM/BLE/beiwov2/APP/feistel_al.c) - two independently written
 * implementations - and checks:
 *   - uid(4) | temp_x10 int16 LE(2) | hum_x10 uint16 LE(2) | crc16 LE(2) round-trips
 *   - the encoder writes all 10 output bytes (the manual pre-assembly removed in ITEM-008
 *     was dead code)
 *   - a tampered ciphertext is rejected by the gateway CRC
 *   - encoding is deterministic
 *   - the decoded values map onto the gateway BLE record [id | hum_be | temp_be] and the
 *     Android parsePlainFrame convention (u16be@4 / s16be@6)
 *
 * Build (from gpio_input_output/):
 *   GW=../../../CH592EVT/EVT/EXAM/BLE/beiwov2/APP
 *   gcc -std=c11 -Wall -Wextra -Wno-unused-function -Wno-unused-const-variable \
 *       -Wno-misleading-indentation -I USER/inc -I $GW test/host_rf_frame_verify_ev.c \
 *       USER/src/encrytogate.c $GW/feistel_al.c -o rf_frame_verify
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "encrytogate.h"
#include "feistel_al.h"

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

/* big-endian readers used by Android parsePlainFrame (MqtttService.kt) */
static unsigned u16be(const uint8_t *b) { return ((unsigned)b[0] << 8) | b[1]; }
static int      s16be(const uint8_t *b) { return (int16_t)(((unsigned)b[0] << 8) | b[1]); }

int main(void)
{
    printf("==== independent 433 frame interop verification (embedded_tester) ====\n");

    uint8_t uid[10];
    for (int i = 0; i < 10; i++) uid[i] = (uint8_t)(0xA0 + i);

    /* ---- T1: round-trip through the real gateway decoder ---- */
    printf("[T1] encoder -> gateway decoder round-trip\n");
    { static const int16_t T[] = { 250, -100, -400, 1250, 0, 351, -450, 1300, 32767, -32768, 280, 220 };
      static const uint16_t H[] = { 500, 0, 1000, 550, 0, 1000, 0, 1000, 65535, 0, 510, 550 };
      int n = (int)(sizeof(T) / sizeof(T[0])), ok = 0;
      for (int i = 0; i < n; i++) {
          uint8_t out[10]; uint8_t ruid[4]; int16_t rt; uint16_t rh;
          encode_frame10(uid, T[i], H[i], out);
          if (decode_frame10(out, ruid, &rt, &rh) && rt == T[i] && rh == H[i] &&
              memcmp(ruid, uid, 4) == 0) ok++;
      }
      CHECK(ok == n, "all vectors round-trip (uid[0..3], temp, hum) exactly"); }

    /* ---- T2: field positions and little-endian byte order ---- */
    printf("[T2] field positions / byte order\n");
    { uint8_t out[10]; uint8_t ruid[4]; int16_t rt = 0; uint16_t rh = 0;
      encode_frame10(uid, -100, 0x1234, out);           /* temp = 0xFF9C, hum = 0x1234 */
      CHECK(decode_frame10(out, ruid, &rt, &rh), "decode accepted");
      CHECK(rt == -100 && rh == 0x1234, "negative temperature sign and humidity value preserved");
      CHECK(memcmp(ruid, uid, 4) == 0, "sensor id is uid[0..3] (encoder UID_IDX = {0,1,2,3})"); }

    /* ---- T3: all 10 output bytes are written by the encoder ---- */
    printf("[T3] the encoder writes the whole frame (dead pre-assembly in ITEM-008)\n");
    { uint8_t a[10], b[10];
      memset(a, 0xAA, sizeof a); memset(b, 0x00, sizeof b);
      encode_frame10(uid, 250, 500, a);
      encode_frame10(uid, 250, 500, b);
      CHECK(memcmp(a, b, 10) == 0, "output is independent of the caller's prior buffer contents");
      CHECK(a[0] != 0xAA || a[1] != 0xAA || a[2] != 0xAA || a[3] != 0xAA, "uid bytes overwritten");
      CHECK(memcmp(a, b, 10) == 0 && a[4] != 0xAA, "frame fully populated by encode_frame10"); }

    /* ---- T4: determinism ---- */
    printf("[T4] determinism\n");
    { uint8_t x[10], y[10];
      encode_frame10(uid, 300, 600, x); encode_frame10(uid, 300, 600, y);
      CHECK(memcmp(x, y, 10) == 0, "same input -> byte-identical ciphertext"); }

    /* ---- T5: tampering is rejected by the gateway CRC ---- */
    printf("[T5] integrity\n");
    { uint8_t out[10]; uint8_t ruid[4]; int16_t rt; uint16_t rh; int rejected = 0;
      encode_frame10(uid, 250, 500, out);
      for (int i = 0; i < 10; i++) {
          uint8_t t[10]; memcpy(t, out, 10); t[i] ^= 0x01;
          if (!decode_frame10(t, ruid, &rt, &rh)) rejected++;
      }
      CHECK(rejected == 10, "every single-bit ciphertext flip is rejected"); }

    /* ---- T6: gateway BLE record order and Android parse convention ---- */
    printf("[T6] decoded values -> BLE record [id | hum_be | temp_be] -> Android u16be@4 / s16be@6\n");
    { uint8_t out[10]; uint8_t ruid[4]; int16_t rt; uint16_t rh;
      int16_t temps[3] = { -100, 250, 351 }; uint16_t hums[3] = { 0, 500, 1000 };
      int ok = 0;
      for (int i = 0; i < 3; i++) {
          uint8_t rec[8];
          encode_frame10(uid, temps[i], hums[i], out);
          if (!decode_frame10(out, ruid, &rt, &rh)) continue;
          /* gateway app_um2006A.c: resbf[4..5]=temp BE, [6..7]=hum BE, record=[id|hum|temp] */
          rec[0] = ruid[0]; rec[1] = ruid[1]; rec[2] = ruid[2]; rec[3] = ruid[3];
          rec[4] = (uint8_t)(rh >> 8); rec[5] = (uint8_t)(rh & 0xFF);
          rec[6] = (uint8_t)((uint16_t)rt >> 8); rec[7] = (uint8_t)((uint16_t)rt & 0xFF);
          /* Android: humidity = u16be@4, temperature = s16be@6 */
          if ((int)u16be(rec + 4) == (int)hums[i] && s16be(rec + 6) == (int)temps[i] &&
              memcmp(rec, uid, 4) == 0) ok++;
      }
      CHECK(ok == 3, "id/humidity/temperature attribution survives the gateway->Android convention"); }

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
