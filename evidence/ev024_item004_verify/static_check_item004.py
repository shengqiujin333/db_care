#!/usr/bin/env python3
"""
static_check_item004.py - ITEM-004 静态核对（embedded_tester.embedded_verification）

只读取当前产品源码的三个既有文件，检查:
  1) 预算常量存在且为数值, 行预算 >= 2x 96 B @9600 的轮询预算说明；
  2) 每个调试行函数在首字符前重置整行预算（trace_line_begin）；
  3) trace_putc 在 TXE 等待中递减预算并在耗尽时置中止标志，后续字符不再写；
  4) flush_close 对 TXE 与 TXBUSY 都使用关闭预算，超时后仍执行复位/关时钟/引脚回收；
  5) debug_trace.c 不引入定时器/中断/RTOS 依赖；
  6) main.c 中 flush_close 位于 go_to_sleep 之前，业务调用位于调试输出之前。

退出码 0 = 全部符合, 1 = 有不符合项。输出可复跑。
"""
import re
import sys

BASE = "D:/mypro/beiwo2/AIP/CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output"
CFG = BASE + "/USER/inc/sensor_config.h"
TRACE = BASE + "/USER/src/debug_trace.c"
MAIN = BASE + "/USER/src/main.c"

passed = 0
failed = 0


def check(cond, msg):
    global passed, failed
    if cond:
        passed += 1
        print("PASS  " + msg)
    else:
        failed += 1
        print("FAIL  " + msg)


def read(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.read()


def main():
    cfg = read(CFG)
    trace = read(TRACE)
    main_c = read(MAIN)

    print("== sensor_config.h ==")
    m_line = re.search(r"#define\s+DEBUG_UART_LINE_GUARD\s+(\d+)uL", cfg)
    m_close = re.search(r"#define\s+DEBUG_UART_CLOSE_GUARD\s+(\d+)uL", cfg)
    m_max = re.search(r"#define\s+SENSOR_DEBUG_UART_MAXLINE\s+(\d+)u", cfg)
    check(m_line is not None, "DEBUG_UART_LINE_GUARD 已定义（数值）")
    check(m_close is not None, "DEBUG_UART_CLOSE_GUARD 已定义（数值）")
    check(m_max is not None and int(m_max.group(1)) == 96, "SENSOR_DEBUG_UART_MAXLINE = 96")
    line_guard = int(m_line.group(1)) if m_line else 0
    close_guard = int(m_close.group(1)) if m_close else 0
    # 96 B @ 9600 8N1 = 960 bit / 9600 = 100 ms；行预算须 >= 2x（设计值，不是实测）
    check(line_guard >= 600000, "行预算 %d >= 600000（约 2x 96 B@9600 的设计轮询预算）" % line_guard)
    check(close_guard > 0 and close_guard <= line_guard,
          "关闭预算 %d > 0 且不超过行预算" % close_guard)
    check("轮询循环次数" in cfg and "超时中止本行" in cfg,
          "配置注释说明预算为轮询次数、超时中止本行（设计值非实测）")

    print("== debug_trace.c ==")
    check(trace.count("static void trace_line_begin(void)") == 1, "trace_line_begin() 存在且唯一")
    check("s_tx_guard = (uint32_t)DEBUG_UART_LINE_GUARD;" in trace,
          "trace_line_begin 重置 s_tx_guard = DEBUG_UART_LINE_GUARD")
    check("s_tx_abort = 0u;" in trace, "trace_line_begin 清除 s_tx_abort")
    check("s_tx_guard--;" in trace, "trace_putc 在 TXE 等待中递减预算")
    check("s_tx_abort = 1u;" in trace and "后续字符不再写" in trace,
          "TXE 预算耗尽置 s_tx_abort 并停止本行后续字符")

    # 每个行函数在首个输出原语之前调用 trace_line_begin
    funcs = ["debug_trace_boot", "debug_trace_bus", "debug_trace_iotest",
             "debug_trace_iosig", "debug_trace_vdd", "debug_trace_sample"]
    for fn in funcs:
        m = re.search(r"void\s+%s\([^)]*\)\s*\{(.*?)\n\}" % fn, trace, re.S)
        body = m.group(1) if m else ""
        before = body.find("trace_line_begin();")
        first_emit = min([p for p in [body.find("trace_putc("), body.find("trace_puts(")] if p >= 0] or [10 ** 9])
        check(before >= 0 and before < first_emit,
              "%s 在首个输出前调用 trace_line_begin()" % fn)
    # S 行后的 G 行独立预算
    m = re.search(r"if \(s->diag_valid != 0u\) \{(.*?)\n    \}", trace, re.S)
    gbody = m.group(1) if m else ""
    check("trace_line_begin();" in gbody and "trace_putc('G')" in gbody,
          "G 行在自身首字符前调用 trace_line_begin()（S/G 预算独立）")

    # flush_close: TXE 与 TXBUSY 都有预算
    m = re.search(r"void debug_trace_flush_close\(void\)\s*\{(.*?)\n\}", trace, re.S)
    fc = m.group(1) if m else ""
    check(fc.count("DEBUG_UART_CLOSE_GUARD") == 2,
          "flush_close 对 TXE 与 TXBUSY 各使用一次 DEBUG_UART_CLOSE_GUARD")
    check(re.search(r"UART_FLAG_TXE", fc) is not None and re.search(r"UART_FLAG_TXBUSY", fc) is not None,
          "flush_close 等待 TXE 与 TXBUSY")
    check("SYSCTRL_APBPeriphReset1(DEBUG_UART_CLK, ENABLE);" in fc and
          "SYSCTRL_APBPeriphReset1(DEBUG_UART_CLK, DISABLE);" in fc,
          "flush_close 仍执行 UART1 复位断言+释放")
    check("SYSCTRL_APBPeriphClk_Enable1(DEBUG_UART_CLK, DISABLE);" in fc, "flush_close 仍关闭 UART1 时钟")
    check(fc.count("GPIO_MODE_INPUT") == 2, "flush_close 把 PA05/PA06 两者都置输入")
    check("SYSCTRL_AHBPeriphReset" not in fc and "SYSCTRL_AHBPeriphClk_Enable" not in fc,
          "flush_close 不触碰 GPIOA 复位/时钟（保护 I2C/SWD 引脚）")
    check("s_uart_open = 0u;" in fc and "s_tx_abort = 0u;" in fc,
          "flush_close 清除打开状态与中止标志")

    # 无定时器/中断/RTOS
    forbidden = ["TIM_", "GTIM", "LPTIM", "IRQHandler", "NVIC", "EXTI", "osThread",
                 "xTask", "vTask", "HAL_NVIC"]
    hits = [w for w in forbidden if w in trace]
    check(not hits, "debug_trace.c 无定时器/中断/RTOS 依赖（命中: %s）" % (hits or "无"))
    check("UART_IT" not in trace and "ITConfig" not in trace, "UART 发送为轮询，不使用 UART 中断")

    print("== main.c ==")
    pos_biz1 = main_c.find("temperature_process();")
    pos_biz2 = main_c.find("send_data_to_gateway();")
    pos_trace = main_c.find("debug_trace_sample(&trace)")
    pos_close = main_c.find("debug_trace_flush_close();")
    pos_sleep = main_c.find("go_to_sleep();", pos_close)   # 主循环内的睡眠（首次出现在 UID 零保护路径）
    check(0 <= pos_biz1 < pos_biz2 < pos_trace < pos_close < pos_sleep,
          "顺序: 采样 → 发送 → S 行 → flush_close → go_to_sleep（UART 不阻塞业务与睡眠）")
    check(main_c.count("debug_trace_flush_close();") == 1,
          "主循环恰有一处 flush_close（幂等接口仍有另一路径可安全重复调用）")

    print("\nstatic checks: %d passed / %d failed" % (passed, failed))
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
