/*
 * Independent verification harness for ITEM-005 (pure logic in fw_core.c).
 * Author: embedded_tester.embedded_verification.
 *
 * Links the REAL USER/src/fw_core.c (no MCU headers; the module defines
 * SENSOR_CONFIG_NO_MCU) and checks:
 *   - fw_crc8_gxht           : exhaustive over all 65536 two-byte inputs + vectors
 *   - gxht40_temp_raw_to_x10 : exhaustive over all 65536 raw values
 *   - gxht40_hum_raw_to_x10  : exhaustive over all 65536 raw values
 *   - light_code_is_dark     : exhaustive over 4096 codes x both previous states
 *   - sensor_decide_report   : exhaustive over a structured (prev,cur,have,dark) grid
 * plus explicit boundary vectors. Expected hashes/values were produced by an
 * independent Python reference using exact rational arithmetic (round-half-up)
 * and a from-scratch CRC-8 - never by the code under test.
 *
 * Build (from gpio_input_output/):
 *   gcc -std=c11 -Wall -Wextra -I USER/inc -I COMMON test/host_fw_core_verify_ev.c \
 *       USER/src/fw_core.c -lm -o fw_core_verify
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "fw_core.h"
#define SENSOR_CONFIG_NO_MCU   /* pure numeric config only, no MCU headers */
#include "sensor_config.h"

#define FNV_INIT 2166136261u
static uint32_t fnv(uint32_t h, uint8_t b) { return (h ^ b) * 16777619u; }
static uint32_t fnv_u16(uint32_t h, uint16_t v) { h = fnv(h, (uint8_t)(v & 0xFFu)); return fnv(h, (uint8_t)(v >> 8)); }

/* hashes computed by the independent Python reference */
#define REF_TEMP_HASH   0xBB0C5ABBu
#define REF_HUM_HASH    0x5F674BC5u
#define REF_CRC8_HASH   0x7CC4B9C5u
#define REF_LIGHT_HASH  0x19DB6FE6u
#define REF_REPORT_HASH 0xF7D66E3Eu

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

int main(void)
{
    printf("==== independent fw_core pure-logic verification (embedded_tester) ====\n");

    /* ---- T1: CRC-8 vectors ---- */
    printf("[T1] fw_crc8_gxht vectors (independent Python reference)\n");
    { const uint8_t v1[2] = {0xBE,0xEF}, v2[1] = {0x00}, v3[2] = {0x66,0x66},
                    v4[2] = {0x72,0xB0}, v5[9] = {'1','2','3','4','5','6','7','8','9'};
      CHECK(fw_crc8_gxht(v1, 2u) == 0x92u, "{0xBE,0xEF} -> 0x92 (datasheet reference vector)");
      CHECK(fw_crc8_gxht(v2, 1u) == 0xACu, "{0x00} -> 0xAC (init 0xFF shift chain)");
      CHECK(fw_crc8_gxht(v5, 9u) == 0xF7u, "\"123456789\" -> 0xF7 (CRC-8/NRSC-5 check value)");
      CHECK(fw_crc8_gxht(v3, 2u) == 0x93u && fw_crc8_gxht(v4, 2u) == 0xDCu,
            "GXHT40 frame CRCs 0x93 / 0xDC reproduced");
      CHECK(fw_crc8_gxht(v1, 0u) == 0xFFu, "empty input -> init 0xFF"); }

    /* ---- T2: exhaustive CRC-8 over all two-byte inputs ---- */
    printf("[T2] exhaustive CRC-8 over 256x256 two-byte inputs\n");
    { uint32_t h = FNV_INIT; uint8_t d[2];
      for (unsigned a = 0; a < 256u; a++) for (unsigned b = 0; b < 256u; b++) {
          d[0] = (uint8_t)a; d[1] = (uint8_t)b; h = fnv(h, fw_crc8_gxht(d, 2u));
      }
      printf("    crc8 hash = 0x%08X (ref 0x%08X)\n", h, REF_CRC8_HASH);
      CHECK(h == REF_CRC8_HASH, "all 65536 two-byte CRCs match the independent reference"); }

    /* ---- T3: exhaustive temperature conversion ---- */
    printf("[T3] exhaustive temperature conversion over all 65536 raw values\n");
    { uint32_t h = FNV_INIT;
      for (uint32_t raw = 0; raw < 65536u; raw++)
          h = fnv_u16(h, (uint16_t)gxht40_temp_raw_to_x10((uint16_t)raw));
      printf("    temp hash = 0x%08X (ref 0x%08X)\n", h, REF_TEMP_HASH);
      CHECK(h == REF_TEMP_HASH, "all 65536 temperature conversions match (round-half-up)"); }

    /* ---- T4: exhaustive humidity conversion (clamped) ---- */
    printf("[T4] exhaustive humidity conversion over all 65536 raw values\n");
    { uint32_t h = FNV_INIT;
      for (uint32_t raw = 0; raw < 65536u; raw++)
          h = fnv_u16(h, gxht40_hum_raw_to_x10((uint16_t)raw));
      printf("    hum hash  = 0x%08X (ref 0x%08X)\n", h, REF_HUM_HASH);
      CHECK(h == REF_HUM_HASH, "all 65536 humidity conversions match (0..1000 clamp)"); }

    /* ---- T5: explicit conversion boundaries / rounding ties ---- */
    printf("[T5] conversion boundaries, negative temperature, rounding ties\n");
    CHECK(gxht40_temp_raw_to_x10(0x0000u) == -450, "raw 0x0000 -> -450 (below valid domain)");
    CHECK(gxht40_temp_raw_to_x10(0x073Eu) == -400, "raw 0x073E -> -400 (=-40.0 C domain min)");
    CHECK(gxht40_temp_raw_to_x10(0x4000u) == -12,  "raw 0x4000 (exact .5 tie) -> -12 (round half up)");
    CHECK(gxht40_temp_raw_to_x10(0xC000u) == 863,  "raw 0xC000 (exact .5 tie) -> 863");
    CHECK(gxht40_temp_raw_to_x10(0x41C2u) == 0,    "raw 0x41C2 -> 0");
    CHECK(gxht40_temp_raw_to_x10(0x6666u) == 250,  "raw 0x6666 -> 250 (25.0 C)");
    CHECK(gxht40_temp_raw_to_x10(0xF89Du) == 1250, "raw 0xF89D -> 1250 (=125.0 C domain max)");
    CHECK(gxht40_temp_raw_to_x10(0xBEEFu) == 855,  "raw 0xBEEF -> 855 (negative-sign path OK)");
    CHECK(gxht40_temp_raw_to_x10(0xF8CAu) == 1251, "raw 0xF8CA -> 1251 (just above domain)");
    CHECK(gxht40_temp_raw_to_x10(0xFFFFu) == 1300, "raw 0xFFFF -> 1300 (pure conversion, above domain)");
    CHECK(gxht40_hum_raw_to_x10(0x0000u) == 0u,    "hum raw 0 -> 0 (raw -60 clamped)");
    CHECK(gxht40_hum_raw_to_x10(0x0C64u) == 1u,    "hum raw 0x0C64 -> 1 (first non-zero)");
    CHECK(gxht40_hum_raw_to_x10(0x1234u) == 29u,   "hum raw 0x1234 -> 29");
    CHECK(gxht40_hum_raw_to_x10(0x72B0u) == 500u,  "hum raw 0x72B0 -> 500 (50 %RH)");
    CHECK(gxht40_hum_raw_to_x10(0xD8FDu) == 1000u, "hum raw 0xD8FD -> 1000 (100 %RH)");
    CHECK(gxht40_hum_raw_to_x10(0xFFFFu) == 1000u, "hum raw 0xFFFF -> 1000 (1190 clamped)");

    /* ---- T6: validity gate + no-write-on-invalid + NULL tolerance ---- */
    printf("[T6] gxht40_raw_to_x10 validity gate\n");
    { int16_t t = 1234; uint16_t hh = 4321;
      CHECK(gxht40_raw_to_x10(0x6666u, 0x72B0u, &t, &hh) == true && t == 250 && hh == 500,
            "valid pair -> true, outputs written");
      t = 1234; hh = 4321;
      CHECK(gxht40_raw_to_x10(0x0000u, 0x72B0u, &t, &hh) == false && t == 1234 && hh == 4321,
            "invalid temperature (-450) -> false, outputs untouched");
      t = 1234; hh = 4321;
      CHECK(gxht40_raw_to_x10(0xF8CAu, 0x72B0u, &t, &hh) == false && t == 1234 && hh == 4321,
            "invalid temperature (1251) -> false, outputs untouched");
      CHECK(gxht40_raw_to_x10(0x073Eu, 0x72B0u, &t, &hh) == true && t == -400,
            "domain minimum -400 accepted");
      CHECK(gxht40_raw_to_x10(0xF89Du, 0x72B0u, &t, &hh) == true && t == 1250,
            "domain maximum 1250 accepted");
      CHECK(gxht40_temp_x10_valid(-450) == false && gxht40_temp_x10_valid(1300) == false,
            "validity helper rejects out-of-domain values");
      CHECK(gxht40_raw_to_x10(0x6666u, 0x72B0u, NULL, NULL) == true,
            "NULL output pointers tolerated (no crash, validity still returned)"); }

    /* ---- T7: exhaustive light hysteresis over 4096 codes x 2 states ---- */
    printf("[T7] exhaustive light hysteresis (4096 codes x prev state)\n");
    { uint32_t h = FNV_INIT;
      for (uint32_t code = 0; code < 4096u; code++)
          for (int prev = 0; prev < 2; prev++)
              h = fnv(h, light_code_is_dark((uint16_t)code, prev ? true : false) ? 1u : 0u);
      printf("    light hash = 0x%08X (ref 0x%08X)\n", h, REF_LIGHT_HASH);
      CHECK(h == REF_LIGHT_HASH, "all 8192 hysteresis decisions match the reference state machine"); }
    printf("[T7b] hysteresis boundaries\n");
    CHECK(light_code_is_dark(0u, false) == false && light_code_is_dark(349u, false) == false,
          "LIT: 0 / 349 -> LIT");
    CHECK(light_code_is_dark(350u, false) == true && light_code_is_dark(4095u, false) == true,
          "LIT: 350 (>= ENTER) / 4095 -> DARK");
    CHECK(light_code_is_dark(251u, true) == true && light_code_is_dark(250u, true) == false,
          "DARK: 251 stays DARK / 250 (<= EXIT) -> LIT");
    CHECK(light_code_is_dark(300u, true) == true && light_code_is_dark(300u, false) == false,
          "inside band keeps the previous state (no chatter)");

    /* ---- T8: exhaustive report decision over a structured grid ---- */
    printf("[T8] exhaustive report decision grid\n");
    { static const int16_t PREVS[] = {-32768,-1000,-400,-1,0,9,10,100,300,350,351,1000,32767};
      static const int16_t CURS[]  = {-32768,-1000,-400,0,100,291,300,350,351,1000,32767};
      uint32_t h = FNV_INIT; int n = 0;
      for (unsigned i = 0; i < sizeof(PREVS)/sizeof(PREVS[0]); i++)
        for (unsigned j = 0; j < sizeof(CURS)/sizeof(CURS[0]); j++)
          for (int have = 0; have < 2; have++)
            for (int dark = 0; dark < 2; dark++) {
                h = fnv(h, sensor_decide_report(PREVS[i], have ? true : false,
                                                CURS[j], dark ? true : false) ? 1u : 0u);
                n++;
            }
      printf("    report hash = 0x%08X (ref 0x%08X) over %d combinations\n", h, REF_REPORT_HASH, n);
      CHECK(h == REF_REPORT_HASH, "all grid decisions match IC-002 report=(cur>350)||(cur<350&&have&&dark&&(prev-cur>9))"); }

    printf("[T8b] report truth table (readme/IC-002 semantics)\n");
    CHECK(sensor_decide_report(0, false, 200, true) == false, "no prev, cur=200 -> false");
    CHECK(sensor_decide_report(0, false, 350, true) == false, "no prev, cur=350 (exactly 35.0C) -> false");
    CHECK(sensor_decide_report(0, false, 351, false) == true, "no prev, cur=351 -> true (light-independent)");
    CHECK(sensor_decide_report(300, true, 291, true) == false, "drop exactly 9 (0.9C) -> false (strict)");
    CHECK(sensor_decide_report(300, true, 290, true) == true, "drop 10 (1.0C) + DARK -> true");
    CHECK(sensor_decide_report(300, true, 290, false) == false, "drop 10 but LIT -> false");
    CHECK(sensor_decide_report(360, true, 350, true) == false, "cur=350 with large drop -> false");
    CHECK(sensor_decide_report(360, true, 351, true) == true, "cur=351 -> true regardless of drop");
    CHECK(sensor_decide_report(300, true, 300, true) == false, "prev==cur -> false");
    CHECK(sensor_decide_report(300, true, 320, true) == false, "temperature rise -> false");
    CHECK(sensor_decide_report(-100, true, -120, true) == true, "negative temps: -10.0 -> -12.0 C drop 2.0C -> true");
    CHECK(sensor_decide_report(32767, true, -32768, true) == true, "int16 extremes do not overflow (drop computed in 32-bit)");
    CHECK(sensor_decide_report(1000, true, 350, true) == false,
          "exactly 35.0C + large drop + DARK -> no report (IC-002 T<35 conjunct)");
    CHECK(sensor_decide_report(1000, true, 349, true) == true,
          "34.9C + large drop + DARK -> report");
    CHECK(sensor_decide_report(-32768, true, -32768, true) == false, "int16 min, no drop -> false");
    CHECK(sensor_decide_report(32767, true, 32767, true) == true, "int16 max -> over-temp branch -> true");

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
