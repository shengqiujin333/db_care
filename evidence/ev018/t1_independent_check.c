/*
 * t1_independent_check.c - embedded_tester independent cross-check of
 * sensor_decide_report() (TD-002 T1-01/T1-02).
 *
 * Prints one line per vector: prev cur dark have_prev actual
 * The expected value is computed independently in Python and diffed.
 * This file is a tester-owned verification asset; it does not modify product code.
 */
#include <stdio.h>
#include <stdint.h>
#include "fw_core.h"

int main(void)
{
    static const int16_t temps[] = {
        -32768, -30000, -450, -401, -400, -399,
        -10, 0, 200, 290, 300, 309, 310, 340,
        349, 350, 351, 1000, 1250, 1251, 32767
    };
    const int nt = (int)(sizeof(temps) / sizeof(temps[0]));
    int i, j, dark, have;

    for (i = 0; i < nt; i++) {
        for (j = 0; j < nt; j++) {
            for (dark = 0; dark < 2; dark++) {
                for (have = 0; have < 2; have++) {
                    int r = sensor_decide_report(temps[i], have ? true : false,
                                                 temps[j], dark ? true : false)
                                ? 1 : 0;
                    printf("%d %d %d %d %d\n", (int)temps[i], (int)temps[j],
                           dark, have, r);
                }
            }
        }
    }
    return 0;
}
