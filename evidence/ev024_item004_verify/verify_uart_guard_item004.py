#!/usr/bin/env python3
"""
verify_uart_guard_item004.py - 独立判据/参考模型（embedded_tester.embedded_verification）

读取 test/host_uart_bounded_verify_item004.c 的原始观测输出，独立重算:
  - BOOT/VDD/IOTEST/IOSIG/BUS/S/G 的期望字节（由文档格式文本模型生成，不调用产品代码）
  - 每行 CRLF 与 <= SENSOR_DEBUG_UART_MAXLINE(96)
  - TXE/TXBUSY 轮询次数与 DEBUG_UART_LINE_GUARD/DEBUG_UART_CLOSE_GUARD 的关系
  - 中止后 teardown 动作、引脚回收、无 GPIOA 越权操作、幂等
  - 业务模型序列与睡眠次数在卡死/正常两情形一致

用法:
  python verify_uart_guard_item004.py harness_current.out
  python verify_uart_guard_item004.py harness_old.out --expect-unbounded
"""
import sys

MAXLINE = 96
LINE_GUARD = 600000
CLOSE_GUARD = 100000
INPUT_MODE = 0x10
PULLUP_MODE = 0x11
PP_MODE = 0x20

passed = 0
failed = 0
fail_msgs = []


def check(cond, msg):
    global passed, failed
    if cond:
        passed += 1
    else:
        failed += 1
        fail_msgs.append(msg)


def parse(path):
    cases = {}
    order = []
    header = {}
    tail = {}
    cur = None
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.rstrip("\n").rstrip("\r")
            if line.startswith("CASE "):
                cur = line[5:].strip()
                cases[cur] = {}
                order.append(cur)
                continue
            if line == "ENDCASE":
                cur = None
                continue
            if " " not in line:
                tail[line] = True
                continue
            k, v = line.split(" ", 1)
            if cur is None:
                header[k] = v
            else:
                cases[cur][k] = v
    return header, cases, order, tail


def bytes_of(c):
    return bytes.fromhex(c.get("CAP_HEX", ""))


def split_lines(data):
    """按 CRLF 切分, 返回 (完整行列表, 是否有未以 CRLF 结束的残留)。"""
    if data == b"":
        return [], False
    parts = data.split(b"\r\n")
    trailing = parts[-1]
    return [p + b"\r\n" for p in parts[:-1]], trailing != b""


def biz_expect(i):
    lcg = ((i + 1) * 2654435761) & 0xFFFFFFFF
    for _ in range(2):
        lcg = (lcg * 1664525 + 1013904223) & 0xFFFFFFFF
    temp = ((lcg >> 16) % 600) - 100
    hum = (lcg >> 8) % 1001
    report = (lcg >> 5) & 1
    send = (lcg >> 7) % 3
    return temp, hum, report, send


def s_line(tick):
    # 单次 biz_step 从 lcg=1 出发（与 harness 的单周期用例一致）
    lcg = (1 * 1664525 + 1013904223) & 0xFFFFFFFF
    t = ((lcg >> 16) % 600) - 100
    h = (lcg >> 8) % 1001
    r = (lcg >> 5) & 1
    snd = (lcg >> 7) % 3
    n = 10 + tick % 7
    x = 20 + tick % 9
    a = 15 + tick % 5
    d = 1 if (tick & 1) else 0
    return ("S k=%d p=%d H=1 V=1 o=8 n=%d x=%d a=%d D=%d t=%d h=%d q=1 r=%d s=%d y=0\r\n"
            % (tick, t - 5, n, x, a, d, t, h, r, snd)).encode()


def line_ok(data, name, require_crlf=True):
    lines, dangling = split_lines(data)
    if require_crlf:
        check(not dangling, "%s: 数据以 CRLF 结束（无半个字符/截断）" % name)
    check(all(len(x) <= MAXLINE for x in lines),
          "%s: 每行 <= %d B（实际 %s）" % (name, MAXLINE, [len(x) for x in lines]))
    return lines


def check_teardown(c, name):
    check(int(c.get("UART_RESET", "-1")) == 2, "%s: UART1 复位断言+释放 = 2" % name)
    check(int(c.get("UART_CLK_DIS", "-1")) == 1, "%s: UART1 时钟关闭 = 1" % name)
    check(int(c.get("GPIOA_RESET", "-1")) == 0, "%s: 不复位 GPIOA（保护 PA03/PA04 I2C / PA07/PA08 SWD）" % name)
    check(int(c.get("GPIOA_CLK_DIS", "-1")) == 0, "%s: 不关闭 GPIOA 时钟" % name)
    check(int(c.get("PIN5_MODE", "0"), 16) == INPUT_MODE, "%s: PA05(UART1_RXD) 置输入" % name)
    check(int(c.get("PIN6_MODE", "0"), 16) == INPUT_MODE, "%s: PA06(UART1_TXD) 置输入" % name)
    oplog = c.get("OPLOG", "")
    check("RRC65" in oplog, "%s: 动作序列含 复位×2→关时钟→PA06→PA05（%s）" % (name, oplog))
    check("A" not in oplog and "D" not in oplog, "%s: 无 GPIOA 复位/时钟操作（%s）" % (name, oplog))


def verify_current(path):
    header, cases, order, tail = parse(path)
    check("UNBOUNDED case=" not in open(path, encoding="utf-8", errors="replace").read(),
          "当前实现未触发无界轮询硬上限")
    check(header.get("LINE_GUARD") == str(LINE_GUARD), "配置 LINE_GUARD = %d" % LINE_GUARD)
    check(header.get("CLOSE_GUARD") == str(CLOSE_GUARD), "配置 CLOSE_GUARD = %d" % CLOSE_GUARD)
    check(header.get("MAXLINE") == str(MAXLINE), "配置 MAXLINE = %d" % MAXLINE)

    # ---- N_BOOT_ORDER: 上电顺序与逐行格式 ----
    c = cases.get("N_BOOT_ORDER", {})
    data = bytes_of(c)
    expect = (
        b"BOOT fw=FD-002r4 uid=11223344 rst=0001 uart=9600\r\n"
        b"VDD ok=1 code=1632 bgrmv=1200 mv=3011\r\n"
        b"IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=0\r\n"
        b"IOSIG scl=95555 sda=30303\r\n"
        b"BUS idle=3 scl=1 sda=1 ack=none\r\n"
    )
    check(data == expect, "N_BOOT_ORDER: BOOT→VDD→IOTEST→IOSIG→BUS 逐字节与文档格式一致")
    lines = line_ok(data, "N_BOOT_ORDER")
    check(len(lines) == 5, "N_BOOT_ORDER: 恰好 5 行")
    check(all(x.endswith(b"\r\n") for x in lines), "N_BOOT_ORDER: 每行含 CRLF")
    check(int(c.get("UART_BAUD", "0")) == 9600 and int(c.get("UART_UCLK", "0")) == 8000000,
          "N_BOOT_ORDER: UART 9600 @ PCLK 8 MHz")
    check(int(c.get("PIN5_MODE", "0"), 16) == PULLUP_MODE, "N_BOOT_ORDER: PA05 上拉输入")
    check(int(c.get("PIN6_MODE", "0"), 16) == PP_MODE, "N_BOOT_ORDER: PA06 推挽输出")
    check(int(c.get("TXE_POLLS", "-1")) == int(c.get("BYTES", "-2")),
          "N_BOOT_ORDER: TXE 就绪时每字节一次轮询（无额外等待）")
    vdd = lines[1] if len(lines) > 1 else b""
    check(vdd == b"VDD ok=1 code=1632 bgrmv=1200 mv=3011\r\n",
          "N_BOOT_ORDER: VDD mv == 4095*bgr/code = 3011（独立复算）")

    # ---- 最坏行 ----
    c = cases.get("N_WORST_S", {})
    data = bytes_of(c)
    exp_s = (b"S k=4294967295 p=-1250 H=1 V=1 o=8 n=4095 x=4095 a=4095 D=1 "
             b"t=-1250 h=1000 q=0 r=1 s=2 y=2\r\n")
    check(data == exp_s, "N_WORST_S: 最坏 S 行逐字节与文档格式一致")
    line_ok(data, "N_WORST_S")
    check(len(data) == 92, "N_WORST_S: 最坏 S 行 92 B <= 96")
    keys = [b" k=", b" p=", b" H=", b" V=", b" o=", b" n=", b" x=", b" a=",
            b" D=", b" t=", b" h=", b" q=", b" r=", b" s=", b" y="]
    pos = -1
    ok = True
    for k in keys:
        nxt = data.find(k)
        if nxt <= pos:
            ok = False
        pos = nxt
    check(ok, "N_WORST_S: 字段顺序冻结")

    c = cases.get("N_WORST_G", {})
    data = bytes_of(c)
    exp = (b"S k=3 p=0 H=0 V=0 o=0 n=0 x=0 a=0 D=0 t=456 h=678 q=0 r=0 s=0 y=0\r\n"
           b"G s=5 a44=1 a45=1 rd=5 at=3 raw=ABACADAEAFB0\r\n")
    check(data == exp, "N_WORST_G: S 后紧跟最宽 G 行，逐字节一致")
    line_ok(data, "N_WORST_G")
    lines = split_lines(data)[0]
    check(len(lines) == 2 and len(lines[1]) == 46, "N_WORST_G: G 行 46 B <= 96")

    c = cases.get("N_WORST_BUS", {})
    exp = b"BUS idle=0 scl=0 sda=0 ack=08,09,0A,0B,0C,0D,0E,0F+\r\n"
    check(bytes_of(c) == exp, "N_WORST_BUS: 最坏 BUS 行逐字节一致（8 地址+截断标记）")
    line_ok(bytes_of(c), "N_WORST_BUS")

    c = cases.get("N_WORST_IOTEST", {})
    exp = b"IOTEST sda_lo=1 scl_lo=1 idle=3 swap=44,45 gc=3\r\n"
    check(bytes_of(c) == exp, "N_WORST_IOTEST: 最坏 IOTEST 行逐字节一致")
    line_ok(bytes_of(c), "N_WORST_IOTEST")

    c = cases.get("N_WORST_IOSIG", {})
    exp = b"IOSIG scl=FFFFF sda=FFFFF\r\n"
    check(bytes_of(c) == exp, "N_WORST_IOSIG: 最坏 IOSIG 行逐字节一致")
    line_ok(bytes_of(c), "N_WORST_IOSIG")

    c = cases.get("N_WORST_VDD", {})
    exp = b"VDD ok=1 code=4095 bgrmv=4095 mv=6000\r\n"
    check(bytes_of(c) == exp, "N_WORST_VDD: 最宽 VDD 行逐字节一致")
    line_ok(bytes_of(c), "N_WORST_VDD")

    # ---- 慢但可用 TXE: 不得截断 ----
    c = cases.get("N_SLOW_TXE", {})
    data = bytes_of(c)
    check(data == s_line(3), "N_SLOW_TXE: TXE 每 7 次轮询就绪时整行完整（无截断）")
    line_ok(data, "N_SLOW_TXE")
    check(int(c.get("TXE_POLLS", "0")) == 7 * int(c.get("BYTES", "0")),
          "N_SLOW_TXE: 轮询数 = 7×字节数（模型一致）")
    check(int(c.get("TXE_POLLS", "999999")) <= LINE_GUARD,
          "N_SLOW_TXE: 正常慢速仍在整行预算内")

    # ---- F1: TXE 永不就绪 ----
    c = cases.get("F1_TXE_STUCK_LINE", {})
    check(int(c.get("BYTES", "-1")) == 0, "F1: TXE 卡死时本行不写任何字节")
    check(int(c.get("TXE_POLLS", "-1")) == LINE_GUARD + 1,
          "F1: TXE 轮询次数恰为 LINE_GUARD+1 = %d（有界中止）" % (LINE_GUARD + 1))
    check(c.get("SNAP_CHANGED") == "0", "F1: 传入的采样快照未被修改")
    check(int(c.get("CLOSE_TXE_POLLS", "-1")) == CLOSE_GUARD + 1,
          "F1: 行中止后的关闭 TXE 等待恰为 CLOSE_GUARD+1 = %d" % (CLOSE_GUARD + 1))
    check(int(c.get("CLOSE_TXBUSY_POLLS", "99999999")) <= CLOSE_GUARD + 1,
          "F1: 关闭 TXBUSY 等待 <= CLOSE_GUARD+1")
    check_teardown(c, "F1")

    # ---- F2: 下一行恢复 ----
    c = cases.get("F2_NEXT_LINE_RECOVERS", {})
    data = bytes_of(c)
    check(data == s_line(3), "F2: 卡死后的下一行重新获得预算并完整输出")
    line_ok(data, "F2")

    # ---- F3: S/G 独立预算 ----
    c = cases.get("F3_S_G_INDEPENDENT_BUDGET", {})
    data = bytes_of(c)
    exp_g = b"G s=2 a44=0 a45=0 rd=0 at=0 raw=------------\r\n"
    check(data == exp_g, "F3: S 行中止后 G 行仍完整（各自独立预算），且无 S 残留字节")
    line_ok(data, "F3")
    check(int(c.get("TXE_POLLS", "-1")) == LINE_GUARD + 1 + len(exp_g),
          "F3: 轮询 = S 的 LINE_GUARD+1 + G 每字节 1 次")

    # ---- F4: 关闭 TXE 卡死 ----
    c = cases.get("F4_CLOSE_TXE_STUCK", {})
    check(int(c.get("CLOSE_TXE_POLLS", "-1")) == CLOSE_GUARD + 1,
          "F4: 关闭 TXE 卡死时等待恰为 CLOSE_GUARD+1")
    check(int(c.get("CLOSE_TXBUSY_POLLS", "99999999")) <= CLOSE_GUARD + 1,
          "F4: 关闭 TXBUSY 等待有界")
    check_teardown(c, "F4")
    check(int(c.get("IDEMPOTENT_RESET", "-1")) == 0 and int(c.get("IDEMPOTENT_CLK_DIS", "-1")) == 0,
          "F4: 第二次 close 幂等（无重复复位/关时钟）")
    check(c.get("IDEMPOTENT_GPIO_INIT") == "none", "F4: 第二次 close 无引脚改写")
    check(bytes_of(c) == s_line(3), "F4: 关闭超时后仍可重新初始化并完整打印")

    # ---- F5: 关闭 TXBUSY 卡死 ----
    c = cases.get("F5_CLOSE_TXBUSY_STUCK", {})
    check(int(c.get("CLOSE_TXE_POLLS", "-1")) == 1, "F5: TXE 就绪时关闭等待 1 次轮询")
    check(int(c.get("CLOSE_TXBUSY_POLLS", "-1")) == CLOSE_GUARD + 1,
          "F5: 关闭 TXBUSY 卡死时等待恰为 CLOSE_GUARD+1")
    check_teardown(c, "F5")

    # ---- F6: 业务结果不变 ----
    c = cases.get("F6_BUSINESS_INVARIANCE", {})
    check(c.get("BIZ_EQUAL") == "1", "F6: 卡死与正常两种情形业务模型序列完全一致")
    check(c.get("SLEEP_NORMAL") == "5" and c.get("SLEEP_STUCK") == "5",
          "F6: 卡死情形仍完成 5 次睡眠/业务推进")
    check(c.get("BIZ_NORMAL") == c.get("BIZ_STUCK") and c.get("BIZ_NORMAL") not in (None, ""),
          "F6: 业务序列非空且逐项相同")

    check("DONE" in tail, "harness 正常结束")
    check(order == ["N_BOOT_ORDER", "N_WORST_S", "N_WORST_G", "N_WORST_BUS",
                    "N_WORST_IOTEST", "N_WORST_IOSIG", "N_WORST_VDD", "N_SLOW_TXE",
                    "F1_TXE_STUCK_LINE", "F2_NEXT_LINE_RECOVERS",
                    "F3_S_G_INDEPENDENT_BUDGET", "F4_CLOSE_TXE_STUCK",
                    "F5_CLOSE_TXBUSY_STUCK", "F6_BUSINESS_INVARIANCE"],
          "全部 14 个用例按序执行")


def verify_old(path):
    """负向对照: 旧实现必须仍在同一故障注入下被判定为无界。"""
    global passed, failed
    header, cases, order, tail = parse(path)
    text = open(path, encoding="utf-8", errors="replace").read()
    check("UNBOUNDED case=F1_TXE_STUCK_LINE" in text,
          "旧实现（父提交 fdd1ff1）在 TXE 卡死用例上被硬上限捕获为 UNBOUNDED")
    check("DONE_UNBOUNDED" in tail, "旧实现以 UNBOUNDED 结束（未挂死）")
    check("N_BOOT_ORDER" in cases and "F1_TXE_STUCK_LINE" in cases,
          "旧实现在卡死用例前完成了同样的正常路径用例")
    c = cases.get("N_BOOT_ORDER", {})
    check(bytes_of(c).startswith(b"BOOT fw=FD-002r4 uid=11223344"),
          "旧实现正常路径行为与当前一致（差异只在有界性）")
    check(not failed, "负向对照中不应有其它判据失败")


def main():
    if len(sys.argv) < 2:
        print("usage: verify_uart_guard_item004.py <harness.out> [--expect-unbounded]")
        return 2
    path = sys.argv[1]
    expect_unbounded = "--expect-unbounded" in sys.argv[2:]
    if expect_unbounded:
        verify_old(path)
        status = "NEGATIVE_CONTROL_OK" if failed == 0 else "NEGATIVE_CONTROL_FAIL"
    else:
        verify_current(path)
        status = "PASS" if failed == 0 else "FAIL"
    print("checks: %d passed / %d failed" % (passed, failed))
    for m in fail_msgs:
        print("  FAILED: " + m)
    print("RESULT " + status)
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
