/*
 * Independent verification harness for ITEM-005 (pure logic in fw_core.c).
 * Author: embedded_tester.embedded_verification.
 *
 * Links the REAL USER/src/fw_core.c (no MCU headers; the module defines
 * SENSOR_CONFIG_NO_MCU) and checks:
 *   - fw_crc8_gxht           : exhaustive over all 65536 two-byte inputs + vectors
 *   - gxht40_temp_raw_to_x10 : exhaustive over all 65536 raw values
 *   - gxht40_hum_raw_to_x10  : exhaustive over all 65536 raw values
 *   - light_is_dark          : exhaustive 1/3 no-light criterion (mean x valid x sampled
 *                              c_dark), compared per case against an independently written
 *                              ceil-division reference
 *   - sensor_decide_report   : exhaustive over a structured (prev,cur,have,dark) grid
 * plus explicit boundary vectors. Expected hashes/values were produced by an
 * independent Python reference using exact rational arithmetic (round-half-up)
 * and a from-scratch CRC-8 - never by the code under test.
 *
 * T2/T4 sync note: the light criterion changed in T2 from the two-state hysteresis
 * (light_code_is_dark) to the readme-point-7 form light_is_dark(mean,valid,c_dark) =
 * valid && 3*mean >= c_dark; T7/T7b below were re-derived for it. T8/T8b characterise
 * the AS-BUILT report decision, which still uses the cooling direction
 * ((prev-cur) > 9) - that is the registered deviation D-01 in TD-002 rev 5.0; the
 * readme's rising direction is pending firmware task T4 and this harness will need
 * re-derivation when it lands.
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

    /* ---- T7: exhaustive 1/3 no-light criterion (readme point 7) ----
     * Reference written independently of the implementation:
     *     dark = valid && mean >= ceil(c_dark / 3)
     * The implementation states the same rule as (uint32)3*mean >= c_dark; the harness
     * recomputes every case with the ceil-division form instead, so a rounding or
     * overflow mistake on the multiplication side can not hide behind a matching
     * expression. Equality counts as DARK in both forms. Each case is compared
     * individually (no aggregate hash, which could in principle collide). */
    printf("[T7] exhaustive 1/3 criterion (mean 0..4095 x valid x sampled c_dark)\n");
    { static uint16_t cd[160];
      unsigned ncd = 0, ci, k;
      static const uint16_t extra[] = {100,101,102,341,342,343,1023,1024,1025,
                                       2047,2048,2049,3071,3072,3073,4093,4094,4095};
      for (k = 0; k <= 66u; k++) cd[ncd++] = (uint16_t)k;              /* all residues mod 3 */
      for (k = 0; k < sizeof(extra)/sizeof(extra[0]); k++) cd[ncd++] = extra[k];
      for (k = 0; k < 4096u; k += 61u) cd[ncd++] = (uint16_t)k;        /* stride sample */
      uint32_t n = 0, bad = 0;
      for (ci = 0; ci < ncd; ci++)
        for (int valid = 0; valid < 2; valid++)
          for (uint32_t mean = 0; mean < 4096u; mean++) {
              bool exp = (valid != 0) && (mean >= (((uint32_t)cd[ci] + 2u) / 3u));
              if (light_is_dark((uint16_t)mean, valid != 0, cd[ci]) != exp) bad++;
              n++;
          }
      printf("    %u cases over %u c_dark values, %u mismatches\n", n, ncd, bad);
      CHECK(bad == 0, "every case matches dark = valid && mean >= ceil(c_dark/3)"); }
    printf("[T7b] 1/3 criterion boundaries + uncalibrated gate\n");
    CHECK(light_is_dark(0u, false, 4095u) == false && light_is_dark(4095u, false, 4095u) == false,
          "valid=false is never dark, even with a full-scale code (uncalibrated gate)");
    CHECK(light_is_dark(1364u, true, 4095u) == false,
          "mean one below the boundary (ceil(4095/3)=1365) -> LIT (no hysteresis, no slack)");
    CHECK(light_is_dark(1365u, true, 4095u) == true, "mean at the boundary -> DARK (equality counts)");
    CHECK(light_is_dark(1366u, true, 4095u) == true, "mean one above the boundary -> DARK");
    CHECK(light_is_dark(0u, true, 0u) == true, "c_dark=0 -> every valid sample is DARK");
    CHECK(light_is_dark(1u, true, 3u) == true && light_is_dark(0u, true, 3u) == false,
          "c_dark=3 -> mean 1 dark / mean 0 lit (3*mean >= c_dark)");
    CHECK(light_is_dark(100u, true, 300u) == true && light_is_dark(100u, true, 301u) == false,
          "c_dark straddling 3*mean flips the decision (stateless, per-sample)");

    /* ---- T8: exhaustive report decision over a structured grid ----
     * NOTE: the hash below characterises the AS-BUILT cooling direction (registered
     * deviation D-01); the readme's rising direction is firmware task T4 and will
     * require a new reference hash here once it lands. */
    printf("[T8] exhaustive report decision grid (as-built, D-01)\n");
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
      CHECK(h == REF_REPORT_HASH, "all grid decisions match the as-built rule report=(cur>350)||(cur<350&&have&&dark&&(prev-cur>9)) [D-01: cooling direction]"); }

    printf("[T8b] report truth table (as-built, D-01; readme's rising direction pending T4)\n");
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
