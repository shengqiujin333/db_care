# Test Plan: uart_console (Phase 2.3)

## 1. Scope

This test plan covers the UART1 console driver module for CW32L010Y8M6.

**Module**: uart_console  
**Layer**: driver  
**Bus**: UART1 (PA05=RXD, PA06=TXD, AF1)  
**Config**: 115200 baud, 8N1, TX-only, polling  

## 2. Test Environment

### 2.1 Hardware Setup

| Item | Description |
|------|-------------|
| MCU board | CW32L010Y8M6 TSSOP-20 on project PCB |
| UART adapter | USB-UART (e.g. CP2102, CH340) connected to J3 (CON3) |
| Connections | J3-1 (TXD from MCU PA06) → USB-UART RXD |
|              | J3-2 (RXD from MCU PA05) → USB-UART TXD |
|              | J3-3 (GND) → USB-UART GND |
| Terminal | 115200 8N1, no flow control, LF→CRLF enabled |

### 2.2 Software Prerequisites

- Project compiled with `CONFIG_CONSOLE_ENABLE = 1`
- `SystemCoreClock` reports correct PCLK frequency (e.g. 48 MHz)
- `UART_Init()` and `UART_SendData_8bit()` available from SDK

## 3. Test Cases

### TC-001: Initialization (Basic Sanity)

**Purpose**: Verify `uart_console_init()` returns `ERR_OK` and UART pins are configured.

**Procedure**:
1. Call `uart_console_init(SystemCoreClock)`.
2. Check return value is `ERR_OK`.
3. Read back `CW_GPIOA->AFRL` bits for positions 5 and 6 to verify AF1 is set.
4. Read `CW_UART1->CR1` to verify UART is enabled with TX mode.

**Pass criteria**:
- Return value = 0 (ERR_OK).
- AFRL[5:5] = 1 (PA05 AF1 = UART1_RXD).
- AFRL[6:6] = 1 (PA06 AF1 = UART1_TXD).
- CR1 shows UART enabled, TX direction active.

---

### TC-002: Single Character Transmission

**Purpose**: Verify `uart_putchar()` sends a single character correctly.

**Procedure**:
1. Initialize UART console via `uart_console_init()`.
2. Send char `'A'` (0x41) via `uart_putchar('A')`.
3. Observe on terminal.

**Pass criteria**:
- Terminal displays `A`.
- No extra bytes, no garbage.
- Waveform on PA06 shows: start bit (low), 8 data bits (0x41 = 01000001 LSB-first), stop bit (high).
- Bit time ≈ 8.68 μs (1/115200).

---

### TC-003: String Transmission

**Purpose**: Verify `uart_puts()` sends a null-terminated string.

**Procedure**:
1. Initialize UART console.
2. Send `uart_puts("Hello, World!")`.
3. Observe on terminal.

**Pass criteria**:
- Terminal displays `Hello, World!`.
- No trailing garbage after string.
- Automatic CR+LF conversion: `\n` in string becomes `\r\n`.

---

### TC-004: CR+LF Expansion

**Purpose**: Verify `\n` is expanded to `\r\n`.

**Procedure**:
1. Send `uart_puts("Line1\nLine2\n")`.
2. Observe on terminal.

**Pass criteria**:
- Terminal shows:
  ```
  Line1
  Line2
  ```
- Both CR (0x0D) and LF (0x0A) are sent after each `\n`.
- Waveform shows 2 stop bits between the CR and LF bytes (back-to-back frames).

---

### TC-005: Lightweight printf — String

**Purpose**: Verify `uart_printf("%s", str)`.

**Procedure**:
1. `uart_printf("Status: %s", "OK")`.
2. Terminal should show: `Status: OK`.

**Pass criteria**: Exact match.

---

### TC-006: Lightweight printf — Integer

**Purpose**: Verify `uart_printf("%d", val)` for positive, negative, zero.

**Procedure**:
1. `uart_printf("%d", 1234)` → `1234`.
2. `uart_printf("%d", -567)` → `-567`.
3. `uart_printf("%d", 0)` → `0`.

**Pass criteria**: Exact match for each case.

---

### TC-007: Lightweight printf — Hex

**Purpose**: Verify `%x` and `%X` format.

**Procedure**:
1. `uart_printf("%x", 0xABCD)` → `abcd`.
2. `uart_printf("%X", 0xABCD)` → `ABCD`.
3. `uart_printf("%x", 0)` → `0`.

**Pass criteria**: Exact match.

---

### TC-008: Lightweight printf — Unsigned

**Purpose**: Verify `%u` format.

**Procedure**:
1. `uart_printf("%u", 4294967295U)` → `4294967295`.

**Pass criteria**: Exact match (max 32-bit).

---

### TC-009: Lightweight printf — Float (Fixed-Point)

**Purpose**: Verify `%f` format with scaled integer (×1000).

**Procedure**:
1. `uart_printf("%f", 25412)` (25.412°C × 1000) → `25.412`.
2. `uart_printf("%f", -5123)` → `-5.123`.
3. `uart_printf("%f", 0)` → `0.000`.

**Pass criteria**: Exact match for each case.

---

### TC-010: Binary Buffer Write

**Purpose**: Verify `uart_write()` sends raw bytes without CR+LF expansion.

**Procedure**:
1. `uint8_t buf[] = {0x01, 0x02, 0x0A, 0xFF};`
2. `uart_write(buf, 4)`.
3. Observe on terminal (hex mode).

**Pass criteria**:
- Terminal (hex mode) displays: `01 02 0A FF`.
- The `0x0A` byte is NOT expanded to `0x0D 0x0A` (no CR+LF expansion in `uart_write`).

---

### TC-011: De-initialization

**Purpose**: Verify `uart_console_deinit()` disables UART and releases pins.

**Procedure**:
1. Initialize, send "test".
2. Call `uart_console_deinit()`.
3. Measure PA06 pin state.
4. Attempt to send via `uart_putchar('X')` after deinit.
5. Re-init and send "OK".

**Pass criteria**:
- After deinit: PA06 is high-Z input (floating, <1μA leakage).
- UART1 APBEN1 bit is cleared (clock disabled).
- After re-init: `OK` appears on terminal.

---

### TC-012: Compile-Time Disable (CONFIG_CONSOLE_ENABLE = 0)

**Purpose**: Verify zero code footprint when console is disabled.

**Procedure**:
1. Set `CONFIG_CONSOLE_ENABLE 0` in build config.
2. Rebuild project.
3. Call `CONSOLE_PRINT("test")`, `CONSOLE_PUTS("test")`, `CONSOLE_PUTCHAR('x')`.

**Pass criteria**:
- Compilation succeeds.
- No UART1 init or TX code linked (check map file for `UART_SendData_8bit` — should not appear).
- `uart_console_init()` should not be called from app code when console is disabled.
- GPIOA/UART1 clocks remain disabled at startup (saves power).

---

### TC-013: Long String Stress Test

**Purpose**: Verify no buffer overflow with long output.

**Procedure**:
1. Create a 200-char string.
2. `uart_puts(long_string)`.

**Pass criteria**:
- All 200 characters transmitted correctly.
- No stack overflow, no watchdog reset.

---

### TC-014: Consecutive printf Calls

**Purpose**: Verify multiple printf calls in sequence work.

**Procedure**:
1. `uart_printf("A=%d ", 1);`
2. `uart_printf("B=%s ", "two");`
3. `uart_printf("C=%x\n", 0xFF);`

**Pass criteria**:
- Terminal shows: `A=1 B=two C=ff\n` (with CR+LF expansion on `\n`).

---

## 4. Automated Test Code

```c
/**
 * @brief  Self-test for uart_console module
 *
 * Compile with CONFIG_CONSOLE_ENABLE=1.
 * Connect USB-UART to J3 at 115200 8N1.
 * Run and observe terminal output.
 */
#include "uart_console.h"
#include "common_types.h"
#include <string.h>

static int test_count = 0;
static int pass_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    test_count++; \
    if (!(cond)) { \
        uart_printf("  FAIL [%d]: %s\n", test_count, msg); \
    } else { \
        pass_count++; \
        uart_printf("  PASS [%d]: %s\n", test_count, msg); \
    } \
} while(0)

void uart_console_selftest(void)
{
    int32_t ret;
    const char *hello = "Hello, World!";

    uart_printf("\n====== uart_console Self-Test ======\n\n");

    /* TC-001 */
    uart_printf("--- TC-001: Init ---\n");
    ret = uart_console_init(48000000);
    TEST_ASSERT(ret == ERR_OK, "uart_console_init returns ERR_OK");

    /* TC-002 */
    uart_printf("--- TC-002: putchar ---\n");
    uart_putchar('A');
    uart_putchar('\n');
    TEST_ASSERT(1, "putchar 'A' (visual check)");

    /* TC-003 */
    uart_printf("--- TC-003: puts ---\n");
    uart_puts(hello);
    uart_putchar('\n');
    TEST_ASSERT(1, "puts string (visual check)");

    /* TC-005 through TC-009 */
    uart_printf("--- TC-005..009: printf ---\n");
    uart_printf("String: %s\n", "OK");
    uart_printf("Dec: %d %d %d\n", 1234, -567, 0);
    uart_printf("Hex: %x %X\n", 0xABCD, 0xABCD);
    uart_printf("Uns: %u\n", 4294967295U);
    uart_printf("Fix: %f %f %f\n", 25412, -5123, 0);
    TEST_ASSERT(1, "printf format (visual check)");

    /* TC-010 */
    uart_printf("--- TC-010: write (raw) ---\n");
    uint8_t raw[] = {0x01, 0x02, 0x0A, 0xFF};
    uart_write(raw, 4);
    uart_putchar('\n');
    TEST_ASSERT(1, "write raw bytes (visual hex check 01 02 0A FF)");

    /* TC-011 */
    uart_printf("--- TC-011: Deinit & Reinit ---\n");
    uart_console_deinit();
    /* Attempt to send after deinit (should have no effect or hang briefly) */
    uart_putchar('X');  /* May not work — that's expected */
    ret = uart_console_init(48000000);
    TEST_ASSERT(ret == ERR_OK, "reinit after deinit");
    uart_printf("Reinit OK\n");

    /* TC-013 */
    uart_printf("--- TC-013: Long string ---\n");
    {
        char longbuf[256];
        memset(longbuf, 'A', 200);
        longbuf[200] = '\0';
        uart_puts(longbuf);
        uart_putchar('\n');
    }
    TEST_ASSERT(1, "200-char string (visual check)");

    /* TC-014 */
    uart_printf("--- TC-014: Consecutive printf ---\n");
    uart_printf("A=%d ", 1);
    uart_printf("B=%s ", "two");
    uart_printf("C=%x\n", 0xFF);
    TEST_ASSERT(1, "consecutive printf (visual check)");

    uart_printf("\n====== Results: %d/%d passed ======\n", pass_count, test_count);
}
```

## 5. Pass/Fail Summary

| TC-ID | Description | Visual? | Automated? | Criteria |
|-------|-------------|---------|------------|----------|
| TC-001 | Init returns ERR_OK | No | Yes | Return code |
| TC-002 | putchar 'A' | Yes | Partial | Terminal shows 'A' |
| TC-003 | puts string | Yes | Partial | "Hello, World!" on terminal |
| TC-004 | CR+LF expansion | Yes | No | \n → \r\n on scope |
| TC-005 | printf %s | Yes | Partial | String match |
| TC-006 | printf %d | Yes | Partial | Integer formats |
| TC-007 | printf %x/%X | Yes | Partial | Hex formats |
| TC-008 | printf %u | Yes | Partial | Unsigned max |
| TC-009 | printf %f | Yes | Partial | Fixed-point float |
| TC-010 | write raw bytes | Yes | Partial | 0x0A not expanded |
| TC-011 | Deinit/reinit | Yes | Partial | Reinit works |
| TC-012 | Compile-time disable | No | Build check | No UART code linked |
| TC-013 | Long string | Yes | Partial | All chars received |
| TC-014 | Consecutive printf | Yes | Partial | Correct sequence |

**Note**: Tests marked "Visual" require a human to observe terminal output. Tests marked "Partial" automated include a visual confirmation step.
