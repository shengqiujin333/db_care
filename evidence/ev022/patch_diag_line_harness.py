#!/usr/bin/env python3
"""Test-asset repair for EV-022 (embedded_tester, environment_repair scope=test).

The tester-owned harness test/host_diag_line_verify_ev.c predates the T2 `P` line.
Its [S2] worst-case block sets sample.report = 1, so the REAL debug_trace.c now
emits `S` + `P` in one call. The old assertions judged the whole capture length
against the 96-byte per-line budget and required exactly one CRLF, so they failed
for a harness-staleness reason, not a product defect.

This patch checks the 96-byte budget and CRLF termination per emitted LINE.
No assertion is relaxed: every line still must be <= 96 B and CRLF-terminated.
"""
import sys

PATH = ("D:/mypro/beiwo2/AIP/CW32L010_StandardPeripheralLib_V1.0.5/Examples/"
        "sensor/gpio_input_output/test/host_diag_line_verify_ev.c")

OLD = (
    "        capture_mark(); debug_trace_sample(&s);\r\n"
    "        printf(\"      worst-case S: %d bytes\\n\", captured_len());\r\n"
    "        CHECK(captured_len() <= (int)SENSOR_DEBUG_UART_MAXLINE, \"worst-case S line fits the 96-byte budget\");\r\n"
    "        {\r\n"
    "            int crlf, barelf; count_eol(&crlf, &barelf);\r\n"
    "            CHECK(crlf == 1 && barelf == 0, \"worst-case S line terminated by exactly one CRLF\");\r\n"
    "        }\r\n"
)

NEW = (
    "        capture_mark(); debug_trace_sample(&s);\r\n"
    "        /* T2: report!=0 makes the same call emit S followed by P; the 96-byte\r\n"
    "         * budget must therefore be judged per LINE, not per call. */\r\n"
    "        {\r\n"
    "            const char *p = captured();\r\n"
    "            int ok_len = 1, ok_eol = 1, lines = 0;\r\n"
    "            printf(\"      worst-case capture: %d bytes\\n\", captured_len());\r\n"
    "            while (*p) {\r\n"
    "                const char *e = strstr(p, \"\\r\\n\");\r\n"
    "                if (e == NULL) { ok_eol = 0; break; }\r\n"
    "                if ((int)(e - p) + 2 > (int)SENSOR_DEBUG_UART_MAXLINE) ok_len = 0;\r\n"
    "                printf(\"      line %d: %d bytes\\n\", lines + 1, (int)(e - p) + 2);\r\n"
    "                lines++;\r\n"
    "                p = e + 2;\r\n"
    "            }\r\n"
    "            CHECK(lines >= 1, \"worst case produced at least one line\");\r\n"
    "            CHECK(ok_len, \"every emitted line fits the 96-byte budget\");\r\n"
    "            CHECK(ok_eol, \"every emitted line is CRLF-terminated\");\r\n"
    "        }\r\n"
    "        {\r\n"
    "            int crlf, barelf;\r\n"
    "            count_eol(&crlf, &barelf);\r\n"
    "            CHECK(barelf == 0, \"no bare LF in worst-case capture\");\r\n"
    "        }\r\n"
)


def main():
    with open(PATH, "rb") as f:
        data = f.read()
    if NEW.encode() in data:
        print("already patched")
        return 0
    n = data.count(OLD.encode())
    if n != 1:
        print("ERROR: expected exactly 1 match, found %d" % n)
        return 1
    with open(PATH, "wb") as f:
        f.write(data.replace(OLD.encode(), NEW.encode()))
    print("patched %s" % PATH)
    return 0


if __name__ == "__main__":
    sys.exit(main())
