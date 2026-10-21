/*
 * host_item005_link_stubs.c
 *
 * Minimal link stubs so the existing RTC-cadence harness can link the current
 * USER/src/main.c (ITEM-005 added the UID-zero branch, which references
 * report_req and light_init()).  These symbols are normally provided by
 * measure.c/light.c, which the cadence harness does not compile.
 *
 * Test-side asset only; contains no product behaviour.
 */
#include <stdint.h>

uint8_t report_req = 0u;

void light_init(void)
{
}
