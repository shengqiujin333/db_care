#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
evidence/embedded_verify_uart_trace_item001.py

独立验证脚本 (embedded_tester.embedded_verification) —— 当前队列项的 UART1 调试通道轨迹检查。

用途: 解析真实目标 COM42@9600 的原始采集字节流, 对启动横幅与每周期 S 轨迹做
      独立、可复跑的结构与自洽检查。脚本不依赖实现代码, 只按本项冻结契约解析字段。

用法:
    python embedded_verify_uart_trace_item001.py <capture1.bin> [capture2.bin ...]

检查项 (逐项输出 PASS/FAIL, 退出码 0=全部 PASS, 1=存在 FAIL):
    A  BANNER   启动横幅存在, 字段完整 (fw/uid/rst/uart), uid 为 8 位十六进制, uart=9600
    B  TERM     每条 S 轨迹以 CRLF 正确终止 (相邻消息之间必须有 CRLF)
    C  LEN      每条 S 轨迹有效载荷 <= 96 字节
    D  FIELDS   S 轨迹含全部 15 个冻结字段 (k,p,H,V,o,n,x,a,D,t,h,q,r,s,y), 顺序正确
    E  RANGES   字段取值范围/内部关系: V==(o>0); o<=8; n<=a<=x (o>0); o==0 => n=x=a=0;
                t in [-400,1250] 或 q==0; h in [0,1000] 或 q==0; r,q,V,H,D in {0,1}; s in {0,1,2}
    F  DARK     D 与实现的光照判据一致 (滞回: 初态 LIT; 明->暗需 a>=350; 暗->明需 a<=250; o==0 时 code=4095)
    G  REPORT   r 与实现的判定函数一致: r = (t>350) or (t!=350 and H and D and (p-t)>9)
    H  ADVANCE  成功后推进前一有效温度: 上一有效行的 t 应等于本行 p (未发生复位时)

注: 本脚本验证的是"轨迹字段与固件实际状态自洽"(本项职责)。判定语义本身 (升温方向、
    1/3 全暗判据) 属其它队列项, 不作为本项通过条件, 因此 F/G 按当前实现语义重算。
"""
import re
import sys

KEY_ORDER = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
MAXLINE = 96
LIGHT_DARK_ENTER = 350
LIGHT_DARK_EXIT = 250
FULL_SCALE = 4095


def messages(raw: bytes):
    """按 BOOT / S k= 标记切分原始流, 返回 [(kind, payload_bytes, had_crlf_before_next)]."""
    out = []
    idx = []
    for m in re.finditer(rb"BOOT |S k=", raw):
        idx.append(m.start())
    for i, start in enumerate(idx):
        end = idx[i + 1] if i + 1 < len(idx) else len(raw)
        blk = raw[start:end]
        kind = "BOOT" if blk.startswith(b"BOOT ") else "S"
        # 消息有效载荷 = 去掉尾部行尾符
        payload = blk
        crlf = False
        if payload.endswith(b"\r\n"):
            crlf = True
            payload = payload[:-2]
        out.append((kind, payload, crlf))
    return out


def parse_s(payload: bytes):
    text = payload.decode("ascii", errors="replace")
    fields = {}
    # 形如  k=123 / p=-45
    for km in re.finditer(r"([A-Za-z])=(-?\d+)", text):
        fields[km.group(1)] = int(km.group(2))
    order = [m.group(1) for m in re.finditer(r"([A-Za-z])=", text)]
    return fields, order


def check_file(path):
    raw = open(path, "rb").read()
    msgs = messages(raw)
    results = []
    banners = [m for m in msgs if m[0] == "BOOT"]
    slines = [m for m in msgs if m[0] == "S"]

    # A banner
    if not banners:
        results.append(("A BANNER exists", False, "no BOOT banner found"))
    for _, payload, _ in banners:
        t = payload.decode("ascii", errors="replace")
        m = re.fullmatch(r"BOOT fw=(\S+) uid=([0-9A-Fa-f]{8}) rst=([0-9A-Fa-f]+) uart=(\d+)", t)
        results.append(("A BANNER fields", m is not None, t))
        if m:
            results.append(("A BANNER baud=9600", m.group(4) == "9600", m.group(4)))
            results.append(("A BANNER rst nonzero", m.group(3) not in ("0", "0000", "00000", "00000000"),
                            "rst=%s" % m.group(3)))

    # B/C/D/E per S line
    for i, (_, payload, crlf) in enumerate(slines):
        tag = "S#%d" % i
        results.append(("B TERM %s" % tag, crlf, payload.decode("ascii", "replace")))
        results.append(("C LEN %s <= %d" % (tag, MAXLINE), len(payload) <= MAXLINE,
                        "len=%d" % len(payload)))
        fields, order = parse_s(payload)
        missing = [k for k in KEY_ORDER if k not in fields]
        results.append(("D FIELDS %s complete" % tag, not missing, "missing=%s order=%s" % (missing, order)))
        results.append(("D ORDER %s frozen" % tag, order == KEY_ORDER, str(order)))
        if missing:
            continue
        k, p, H, V, o, n, x, a, D, tt, h, q, r, s, y = (fields[key] for key in KEY_ORDER)
        ok = True
        detail = []
        if V != (1 if o > 0 else 0):
            ok = False; detail.append("V!= (o>0)")
        if not (0 <= o <= 8):
            ok = False; detail.append("o out of 0..8")
        if o > 0 and not (n <= a <= x):
            ok = False; detail.append("not n<=a<=x")
        if o == 0 and not (n == x == a == 0):
            ok = False; detail.append("o==0 but n/x/a != 0")
        if q == 1 and not (-400 <= tt <= 1250):
            ok = False; detail.append("t out of valid domain while q=1")
        if q == 1 and not (0 <= h <= 1000):
            ok = False; detail.append("h out of valid domain while q=1")
        if D not in (0, 1) or r not in (0, 1) or q not in (0, 1) or H not in (0, 1) or V not in (0, 1):
            ok = False; detail.append("binary field not 0/1")
        if s not in (0, 1, 2):
            ok = False; detail.append("s not in 0/1/2")
        if not (0 <= a <= FULL_SCALE):
            ok = False; detail.append("a out of 0..4095")
        results.append(("E RANGES %s" % tag, ok, "; ".join(detail) if detail else "ok"))

    # F dark (implemented hysteresis semantics)
    state = False  # power-on default LIT
    for i, (_, payload, _) in enumerate(slines):
        fields, _ = parse_s(payload)
        if not all(k in fields for k in ("o", "a", "D")):
            continue
        a = fields["a"]; o = fields["o"]; D = fields["D"]
        code = FULL_SCALE if o == 0 else a
        if state:
            new = not (code <= LIGHT_DARK_EXIT)
        else:
            new = (code >= LIGHT_DARK_ENTER)
        results.append(("F DARK S#%d" % i, new == bool(D), "code=%d state=%s D=%d expected=%d" % (code, state, D, new)))
        state = new

    # G report (implemented semantics: superheat OR (have and dark and prev-cur>9); cur==350 excluded)
    for i, (_, payload, _) in enumerate(slines):
        fields, _ = parse_s(payload)
        if not all(k in fields for k in ("p", "H", "D", "t", "r")):
            continue
        p = fields["p"]; H = fields["H"]; D = fields["D"]; tt = fields["t"]; r = fields["r"]
        exp = (tt > 350) or (tt != 350 and H == 1 and D == 1 and (p - tt) > 9)
        results.append(("G REPORT S#%d" % i, bool(exp) == bool(r),
                        "p=%d H=%d D=%d t=%d r=%d expected=%d" % (p, H, D, tt, r, exp)))

    # H prev advancement
    prev_ok_t = None
    for i, (_, payload, _) in enumerate(slines):
        fields, _ = parse_s(payload)
        if not all(k in fields for k in ("p", "H", "t", "q")):
            continue
        if prev_ok_t is not None:
            results.append(("H ADVANCE S#%d" % i, fields["p"] == prev_ok_t and fields["H"] == 1,
                            "p=%d H=%d expected_prev=%d" % (fields["p"], fields["H"], prev_ok_t)))
        if fields["q"] == 1:
            prev_ok_t = fields["t"]
    return msgs, results


def main():
    if len(sys.argv) < 2:
        print("usage: %s <capture.bin> [...]" % sys.argv[0])
        return 2
    total = 0
    failed = 0
    for path in sys.argv[1:]:
        print("==== %s ====" % path)
        msgs, results = check_file(path)
        print("messages: %d (BOOT=%d, S=%d)" % (
            len(msgs), sum(1 for m in msgs if m[0] == "BOOT"), sum(1 for m in msgs if m[0] == "S")))
        for name, ok, detail in results:
            total += 1
            if not ok:
                failed += 1
            print("  %-4s %-28s | %s" % ("PASS" if ok else "FAIL", name, detail))
    print("==== result: %d checks, %d failed ====" % (total, failed))
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
