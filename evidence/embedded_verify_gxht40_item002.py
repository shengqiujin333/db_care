#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
evidence/embedded_verify_gxht40_item002.py

独立验证脚本 (embedded_tester.embedded_verification) —— run8 ITEM-002
「GXHT40 温湿度采集在真实目标上恢复有效」。

用途: 解析真实目标 COM42@9600 的原始采集字节流, 独立判定本项的可观察验收:
      每个 3 分钟采样周期的 S 行 q=1、t/h 落在有效域、连续多个周期稳定;
      失败周期紧随一条自洽的 G 行; 成功周期不得出现 G 行。

脚本不引用实现源码与任何固件常量, 只按冻结契约 (FD-002 §6.7 / TD-002 rev 5.0)
逐字节解析; 判定所需阈值全部为需求文本中的字面值, 不从被测实现推导。

用法:
    python embedded_verify_gxht40_item002.py <capture1.bin> [capture2.bin ...]

检查项 (逐项 PASS/FAIL; 退出码 0=无 FAIL):
    A STRUCT   每行 <=96 B、以 CRLF 终止 (采集窗口边缘的截断只记 FRAG);
               行首仅 BOOT/IOTEST/BUS/S/G
    B FIELDS   S 行 15 个冻结字段齐全且顺序正确; BUS/G/IOTEST 行字段齐全
    C DOMAIN   q==1 => t in [-400,1250] 且 h in [0,1000]; q in {0,1}; H,V,D,r in {0,1}; s in {0,1,2}
    D GLINE    q==0 的 S 行之后紧随且仅有 1 条 G 行; q==1 的 S 行之后不得有 G 行
    E GCONS    G 行字段自洽: s in 1..5; a44/a45 in {0,1}; rd in 0..5; at in 1..3;
               raw 为 12 位大写 hex 或 12 个 '-'; raw 为 hex 时用脚本内独立 CRC-8 复算两个字;
               s=2 <=> 两个候选地址均未 ACK, s in {3,4,5} => 至少一个候选 ACK
               (EV-009 登记的 "s=2 与 a44=1 并存" 张力在 ITEM-002 修复后由 E 项作为硬门禁)
    F CADENCE  S 行的 k 单调不减且同一次采集内相邻样本 Δk==3 (采样周期 = 3 个 1 分钟节拍)
    G ADVANCE  q==0 的周期不得推进前一有效温度 (下一行 p 仍等于失败前的有效 t)
    H ACCEPT   本项核心: 统计连续 q==1 的周期数; 连续 >=3 记为 ACCEPT 达成
    I OBS      登记性观察 (不作通过门槛): 采集内 G 行的结果码分布、BUS 行 ack 集合与
               G 行 a44/a45 的一致性
    J IOTEST   上电 I/O 自检行 (EV-011 §5 E1 交回实现的判别观测量):
               域 sda_lo/scl_lo in {0,1}、idle in 0..3、swap in {none, 合法 7bit 地址列表};
               每次复位段内恰 1 条, 位于横幅之后、BUS 之前; 并与该复位段的 BUS 行
               idle 位掩码一致 (两者都读同一对引脚, 不得互相矛盾)
    L IOSIG    上电事务内逐位回读签名行 (EV-012 §5 E1b 交回实现的判别观测量):
               `IOSIG scl=<5 位大写 hex> sda=<5 位大写 hex>`, 各 20 bit;
               本脚本按 TD/实现文档化的 20 个半位采样点顺序**独立重建**期望序列
               (不引用实现常量), 据此判定: ①SCL 时钟形态是否真实成立
               (无器件时 0x95555); ②SDA 的 8 个地址位回读是否等于被驱动的位
               (0x88 的位模式); ③ACK 时隙两位读数指向「有/无器件应答」,
               并与同一复位段的 BUS ack 地址表**交叉核对**(同一对引脚的两次观测
               不得互相矛盾)。逐位不符 = 位序列未真实送达 (H12); 全符 = H12 排除,
               解释面收缩到器件/接线侧。
"""
import re
import sys

KEY_ORDER = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
GKEY_ORDER = ["s", "a44", "a45", "rd", "at", "raw"]
IOKEY_ORDER = ["sda_lo", "scl_lo", "idle", "swap"]
SIGKEY_ORDER = ["scl", "sda"]
MAXLINE = 96
T_MIN, T_MAX = -400, 1250      # -40.0 .. 125.0 C  (x10), readme 点 1 + FD-002 §6.1
H_MIN, H_MAX = 0, 1000         # 0.0 .. 100.0 %RH (x10)
SAMPLE_TICKS = 3               # readme 点 3: 每 3 分钟采样一次
SIG_ADDR7 = 0x44               # IOSIG 发出的地址字节 = (0x44 << 1) = 0x88 (GXHT40 候选 A)
SIG_SAMPLES = 20               # 20 个半位采样点 (S0..S19)
results = []   # (name, ok, detail)


def add(name, ok, detail=""):
    results.append((name, ok, detail))


def iosig_expected(addr7=SIG_ADDR7, device_ack=False):
    """独立重建 IOSIG 的 20 个半位采样点期望读数 (不引用被测实现的常量/函数)。

    文档化的采样顺序 (TD/实现交付说明, 亦为 E1b 的构造):
      S0  START 建立: 主机先把 SCL/SDA 释放为高, 再把 SDA 拉低 -> 读 scl=1 sda=0
      S1  SCL 拉低                                              -> scl=0 sda=0
      S2..S17  地址字节的 8 个数据位 (MSB 先): 每位先 SCL 低再 SCL 高各采一点
                -> (scl,sda) = (0,bit) 与 (1,bit)
      S18 ACK 时隙: 主机释放 SDA, SCL 仍低                       -> scl=0
      S19 ACK 时隙: SCL 抬高                                    -> scl=1
          无器件时 SDA 被上拉读到高; 器件 ACK 时该时隙两位被拉低。
    返回 (scl20, sda20, sda_data_ok_mask) —— 每项都是 20 bit 整数。
    """
    scl_bits = [1, 0]
    sda_bits = [0, 0]
    byte = (addr7 << 1) & 0xFF
    for k in range(8):
        bit = (byte >> (7 - k)) & 1
        scl_bits += [0, 1]
        sda_bits += [bit, bit]
    scl_bits += [0, 1]
    sda_bits += [0, 0] if device_ack else [1, 1]
    assert len(scl_bits) == SIG_SAMPLES and len(sda_bits) == SIG_SAMPLES

    def pack(bits):
        v = 0
        for b in bits:
            v = (v << 1) | b
        return v

    return pack(scl_bits), pack(sda_bits)


def crc8_gxht(data: bytes) -> int:
    """GXHT40/SHT4x 双字 CRC-8: poly 0x31, init 0xFF, MSB-first, xorout 0x00 (手册)."""
    crc = 0xFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = ((crc << 1) ^ 0x31) & 0xFF if (crc & 0x80) else ((crc << 1) & 0xFF)
    return crc


def split_messages(raw: bytes):
    """按行首标记切分; 返回 [(kind, payload, terminated, offset)]。"""
    starts = [(m.start(), m.group(0))
              for m in re.finditer(rb"BOOT |IOTEST |IOSIG |BUS |S k=|G s=", raw)]
    out = []
    for i, (start, marker) in enumerate(starts):
        end = starts[i + 1][0] if i + 1 < len(starts) else len(raw)
        blk = raw[start:end]
        terminated = blk.endswith(b"\r\n")
        payload = blk[:-2] if terminated else blk
        kind = {b"BOOT ": "BOOT", b"IOTEST ": "IOTEST", b"IOSIG ": "IOSIG", b"BUS ": "BUS",
                b"S k=": "S", b"G s=": "G"}[marker]
        out.append((kind, payload, terminated, start))
    return out


def parse_kv(payload: bytes, first_key: str):
    text = payload.decode("ascii", errors="replace")
    text = text.lstrip()
    # every diagnostic line starts with its fixed label token; drop it before splitting
    if first_key == "S":
        text = text[1:].strip()               # 去掉行首 'S'
    elif first_key == "G":
        text = text[1:].strip()               # 去掉行首 'G'
    elif first_key in ("IOTEST", "IOSIG", "BUS", "BOOT"):
        text = text[len(first_key):].strip()  # 去掉行首标签
    parts = text.split()
    kv, order = {}, []
    for p in parts:
        if "=" not in p:
            return None, None, "token without '=': %r" % p
        k, v = p.split("=", 1)
        kv[k] = v
        order.append(k)
    return kv, order, None


def parse_bus_ack(kv):
    """BUS 行 ack 字段 -> 已应答 7bit 地址集合; 解析失败返回 None。"""
    ack = (kv or {}).get("ack", "")
    if ack == "none":
        return set()
    body = ack[:-1] if ack.endswith("+") else ack
    if not re.fullmatch(r"[0-9A-F]{2}(,[0-9A-F]{2})*", body or ""):
        return None
    return set(body.split(","))


SIG_READINGS = []   # 跨捕获登记: (tag, reset#, line, scl20, sda20, dev_ack)


def _check_iosig(tag, reset_no, sig, bus_row):
    """E1b 事务内逐位回读签名行: 结构域 + 与「主机自己驱动的形态」比对 + 与 BUS 交叉核对。

    期望序列由 iosig_expected() 独立重建 (不引用被测实现的常量/函数):
      scl  = 0x95555      —— START/时钟形态
      sda  = 0x30303      —— 8 个地址位 (0x88) 回读 + ACK 时隙两次读高 (无器件)
      sda  = 0x30300      —— 同上, 但 ACK 时隙两次读低 (器件在该地址应答)
    """
    kv = sig["kv"]
    msg = sig["_msg"]
    scl_s = kv.get("scl", "")
    sda_s = kv.get("sda", "")
    exp_scl, exp_sda_none = iosig_expected(SIG_ADDR7, False)
    _e, exp_sda_ack = iosig_expected(SIG_ADDR7, True)

    ok_dom = re.fullmatch(r"[0-9A-F]{5}", scl_s or "") is not None and \
             re.fullmatch(r"[0-9A-F]{5}", sda_s or "") is not None
    add("L IOSIG %s reset#%d field domain (scl/sda = 5 upper-hex digits)"
        % (tag, reset_no), ok_dom, msg)
    if not ok_dom:
        return
    scl = int(scl_s, 16)
    sda = int(sda_s, 16)
    dev_ack = (sda & 0x3) == 0x0          # ACK 时隙两点都被拉低
    SIG_READINGS.append((tag, reset_no, msg, scl, sda, dev_ack))

    # ① SCL: 回读是否等于主机自己驱动的时钟形态 (START + 8 位 + ACK 时隙)
    add("L IOSIG %s reset#%d scl readback == driven clock shape (exp %05X)"
        % (tag, reset_no, exp_scl), scl == exp_scl,
        "scl=%s exp=%05X -> %s" % (scl_s, exp_scl,
                                   "SCL 未按时序翻转/未被真实送达" if scl != exp_scl else "ok"))
    # ② SDA 数据位: 中间 16 个采样点 (bit17..bit2) 回读是否等于被驱动的地址位
    add("L IOSIG %s reset#%d sda data bits read back == driven address bits 0x%02X"
        % (tag, reset_no, (SIG_ADDR7 << 1)),
        (sda >> 2) == ((exp_sda_none >> 2)),   # 数据位与 ACK 时隙无关
        "sda=%s data=0x%05X exp=0x%05X" % (sda_s, sda >> 2, exp_sda_none >> 2))
    # ③ ACK 时隙两点必须自洽: 00 = 该地址有器件应答, 11 = 无器件应答; 10/01 不成立
    ack_bits = sda & 0x3
    add("L IOSIG %s reset#%d ACK-slot bits self-consistent (00 device-ACK / 11 none, "
        "not mixed)" % (tag, reset_no), ack_bits in (0x0, 0x3),
        "sda=%s ack-slot bits=%s" % (sda_s, format(ack_bits, "02b")))
    add("L IOSIG %s reset#%d sda value is one of the two valid readings (%05X no-device / "
        "%05X device-ACK)" % (tag, reset_no, exp_sda_none, exp_sda_ack),
        sda == exp_sda_none or sda == exp_sda_ack, "sda=%05X" % sda)

    # ④ 交叉核对: IOSIG 与同一次复位段的 BUS 扫描观测同一对引脚/同一器件,
    #    两者对「0x44 是否应答」的结论不得互相矛盾。
    acked = parse_bus_ack(bus_row["kv"] if bus_row else None)
    if acked is None:
        add("L IOSIG %s reset#%d BUS ack field parseable for cross-check"
            % (tag, reset_no), False, repr((bus_row or {}).get("kv")))
        return
    bus_has_44 = "%02X" % SIG_ADDR7 in acked
    add("L IOSIG %s reset#%d cross-check with BUS scan: device-ACK(%s) <=> 0x%02X in "
        "BUS ack list (%s)" % (tag, reset_no, dev_ack, SIG_ADDR7,
                               ",".join(sorted(acked)) or "none"),
        dev_ack == bus_has_44,
        "IOSIG %r / BUS %r" % (msg, (bus_row or {}).get("_msg")))


def check_file(path):
    raw = open(path, "rb").read()
    tag = path.split("/")[-1].split("\\")[-1]
    print("\n===== %s (%d bytes) =====" % (tag, len(raw)))
    if not raw:
        add("A STRUCT %s non-empty" % tag, False, "0 bytes captured in this window")
        return [], []

    msgs = split_messages(raw)
    # ---- A: structure ----
    # 未以 CRLF 终止的行有两类互不相同的原因, 必须分开处理:
    #   (1) 采集窗口边缘把行切开 (与固件无关) -> 只记 FRAG note;
    #   (2) 烧录流程 (mdk_flash 擦写/下载) 在打印中途 halt/复位了目标 -> 行被切断,
    #       其后紧接一条 BOOT 横幅; 该现象由测试手段造成, 不是固件的行终止缺陷,
    #       因此必须另有**同窗口内同类型的完整行**作为对照, 且被切断的字节必须是
    #       那条完整行的前缀 (否则仍判 FAIL)。
    #     对照 EV-006 的固件级缺陷 D-ITEM001-1 (CRLF 不发): 那种情形下未终止行后面
    #     跟的是下一条 S 行而不是 BOOT, 且不是完整行的前缀 -> 本规则不会把它放过。
    cut = set()
    for i, (kind, payload, terminated, off) in enumerate(msgs):
        if terminated:
            continue
        if off + len(payload) >= len(raw):
            add("A FRAG %s @%d window-edge fragment" % (tag, off), True,
                "note only: %r" % payload[:48])
            continue
        next_is_boot = (i + 1 < len(msgs)) and msgs[i + 1][0] == "BOOT"
        intact = [p for k, p, t, _ in msgs if k == kind and t]
        if next_is_boot and any(p.startswith(payload) and payload for p in intact):
            cut.add(i)
            add("A CUT %s @%d line interrupted by a target reset (flash/halt); bytes are a "
                "prefix of the intact %s line in the same window" % (tag, off, kind),
                True, "note only: %r" % payload)
        else:
            add("A TERM %s @%d unterminated in mid-stream" % (tag, off), False,
                "%r" % payload[:64])
    for kind, payload, terminated, off in msgs:
        if len(payload) + 2 > MAXLINE:
            add("A LEN %s @%d <=%d B" % (tag, off, MAXLINE), False,
                "payload=%d B" % len(payload))
    add("A STRUCT %s every line <= %d B" % (tag, MAXLINE),
        all(len(p) + 2 <= MAXLINE for _, p, _, _ in msgs), "")
    add("A CRLF %s all complete lines CRLF-terminated (reset-cut lines excluded)"
        % tag,
        all(t or i in cut for i, (_, _, t, _) in enumerate(msgs[:-1]))
        if len(msgs) > 1 else True,
        "cut=%s" % sorted(cut))

    periods, g_lines, io_lines, bus_lines, sig_lines = [], [], [], [], []
    for i, (kind, payload, terminated, off) in enumerate(msgs):
        if kind == "S":
            kv, order, err = parse_kv(payload, "S")
            if err or order != KEY_ORDER:
                add("B FIELDS %s S@%d" % (tag, off), False,
                    "err=%s order=%s" % (err, order))
                continue
            row = {"_off": off, "_tag": tag, "_msg": payload.decode("ascii", "replace")}
            for k in KEY_ORDER:
                try:
                    row[k] = int(kv[k])
                except ValueError:
                    add("B INT %s S@%d field %s" % (tag, off, k), False, "%r" % kv[k])
                    row[k] = None
            periods.append(row)
        elif kind == "G":
            kv, order, err = parse_kv(payload, "G")
            if err or order != GKEY_ORDER:
                add("B FIELDS %s G@%d" % (tag, off), False,
                    "err=%s order=%s" % (err, order))
                continue
            g_lines.append({"after": i, "_msg": payload.decode("ascii", "replace"),
                            "raw": kv.get("raw", ""),
                            "nums": {k: kv.get(k) for k in ("s", "a44", "a45", "rd", "at")}})
        elif kind == "IOTEST":
            kv, order, err = parse_kv(payload, "IOTEST")
            if err or order != IOKEY_ORDER:
                add("B FIELDS %s IOTEST@%d" % (tag, off), False,
                    "err=%s order=%s" % (err, order))
                continue
            io_lines.append({"after": i, "_msg": payload.decode("ascii", "replace"),
                             "kv": kv})
        elif kind == "IOSIG":
            kv, order, err = parse_kv(payload, "IOSIG")
            if err or order != SIGKEY_ORDER:
                add("B FIELDS %s IOSIG@%d" % (tag, off), False,
                    "err=%s order=%s" % (err, order))
                continue
            sig_lines.append({"after": i, "_msg": payload.decode("ascii", "replace"),
                              "kv": kv})
        elif kind == "BUS":
            kv, _order, _err = parse_kv(payload, "BUS")
            bus_lines.append({"after": i, "_msg": payload.decode("ascii", "replace"),
                              "kv": kv})

    # ---- J: power-up I/O self-check line (EV-011 §5 E1) ----
    io_idx = [i for i, (k, _, _, _) in enumerate(msgs) if k == "IOTEST"]
    sig_idx = [i for i, (k, _, _, _) in enumerate(msgs) if k == "IOSIG"]
    boot_idx = [i for i, (k, _, _, _) in enumerate(msgs) if k == "BOOT"]
    bus_idx = [i for i, (k, _, _, _) in enumerate(msgs) if k == "BUS"]
    for io in io_lines:
        kv = io["kv"]
        msg = io["_msg"]
        ok_dom = True
        for f in ("sda_lo", "scl_lo"):
            if kv.get(f) not in ("0", "1"):
                ok_dom = False
        if kv.get("idle") not in ("0", "1", "2", "3"):
            ok_dom = False
        sw = kv.get("swap", "")
        if sw == "none":
            pass
        elif re.fullmatch(r"[0-9A-F]{2}(,[0-9A-F]{2})*", sw or ""):
            for a in sw.split(","):
                if not (0x08 <= int(a, 16) <= 0x77):
                    ok_dom = False
        else:
            ok_dom = False
        add("J IOTEST %s @%d field domain (sda_lo/scl_lo in {0,1}, idle in 0..3, "
            "swap in {none|7bit addr list})" % (tag, io["after"]), ok_dom, msg)
    # exactly one IOTEST per reset segment (between a BOOT and the next BOOT),
    # placed after its BOOT banner and before that segment's BUS line
    # A reset segment (BOOT .. next BOOT) that *completed* its power-up print run is the
    # one carrying a BUS line: main() prints BOOT -> IOTEST -> BUS with nothing in between.
    # The flash sequence may halt the target mid-boot (erase/program), so an earlier
    # segment may legitimately carry a BOOT banner with neither IOTEST nor BUS; that is
    # recorded as a note, never as a pass for the missing lines.
    if boot_idx:
        bounds = boot_idx + [len(msgs)]
        completed = 0
        for n, (b0, b1) in enumerate(zip(bounds, bounds[1:])):
            seg_io = [i for i in io_idx if b0 < i < b1]
            seg_bus = [i for i in bus_idx if b0 < i < b1]
            if not seg_bus:
                add("J IOTEST %s reset#%d no BUS -> boot print run interrupted "
                    "(allowed: debugger halted target); IOTEST count=%d"
                    % (tag, n + 1, len(seg_io)), len(seg_io) <= 1, "io=%s" % seg_io)
                continue
            completed += 1
            add("J IOTEST %s reset#%d exactly one IOTEST in the completed boot run"
                % (tag, n + 1), len(seg_io) == 1, "found=%d" % len(seg_io))
            if len(seg_io) == 1:
                add("J IOTEST %s reset#%d order BOOT<IOTEST<BUS" % (tag, n + 1),
                    b0 < seg_io[0] < min(seg_bus),
                    "boot=%d io=%d bus=%s" % (b0, seg_io[0], seg_bus))
            # same pin pair read twice: IOTEST idle and BUS idle must agree
            io_row = next((io for io in io_lines if io["after"] in seg_io), None)
            bus_row = next((b for b in bus_lines if b["after"] in seg_bus), None)
            if io_row and bus_row and bus_row["kv"].get("idle") is not None:
                add("J IOTEST %s reset#%d idle == BUS idle (%s vs %s)"
                    % (tag, n + 1, io_row["kv"].get("idle"), bus_row["kv"].get("idle")),
                    io_row["kv"].get("idle") == bus_row["kv"].get("idle"),
                    "IOTEST %r / BUS %r" % (io_row["_msg"], bus_row["_msg"]))

            # ---- segment wiring for the IOSIG line (E1b) ----
            # the power-up print run is BOOT -> IOTEST -> IOSIG -> BUS; each self-check
            # prints exactly once and sits immediately before the next one.
            seg_sig = [i for i in sig_idx if b0 < i < b1]
            add("L IOSIG %s reset#%d exactly one IOSIG in the completed boot run"
                % (tag, n + 1), len(seg_sig) == 1, "found=%d" % len(seg_sig))
            if not seg_sig:
                continue
            first_bus = min(seg_bus)
            add("L IOSIG %s reset#%d order BOOT<IOTEST<IOSIG<BUS and adjacent "
                "(IOTEST,IOSIG) then (IOSIG,BUS)" % (tag, n + 1),
                len(seg_io) == 1 and b0 < seg_io[0] == seg_sig[0] - 1 == first_bus - 2,
                "boot=%d io=%s sig=%s bus=%s" % (b0, seg_io, seg_sig, seg_bus))
            sig_row = next((s for s in sig_lines if s["after"] == seg_sig[0]), None)
            if sig_row is None:
                continue
            _check_iosig(tag, n + 1, sig_row, bus_row)
        if completed == 0 and boot_idx:
            add("J IOTEST %s no completed boot run in this window (nothing to check)" % tag,
                True, "note only: %d BOOT banner(s), 0 BUS" % len(boot_idx))
    elif io_lines:
        add("J IOTEST %s every IOTEST has a preceding BOOT banner" % tag, False,
            "%d IOTEST without any BOOT in window" % len(io_lines))
    if not boot_idx and sig_lines:
        add("L IOSIG %s every IOSIG has a preceding BOOT banner" % tag, False,
            "%d IOSIG without any BOOT in window" % len(sig_lines))

    # ---- C: value domain ----
    for r in periods:
        if r["q"] not in (0, 1):
            add("C q %s k=%s in {0,1}" % (tag, r["k"]), False, "q=%s" % r["q"])
        for f in ("H", "V", "D", "r"):
            if r[f] not in (0, 1):
                add("C %s %s k=%s in {0,1}" % (f, tag, r["k"]), False, "%s=%s" % (f, r[f]))
        if r["s"] not in (0, 1, 2):
            add("C s %s k=%s in {0,1,2}" % (tag, r["k"]), False, "s=%s" % r["s"])
        if r["q"] == 1:
            add("C DOMAIN %s k=%s t in [%d,%d]" % (tag, r["k"], T_MIN, T_MAX),
                T_MIN <= r["t"] <= T_MAX, "t=%s" % r["t"])
            add("C DOMAIN %s k=%s h in [%d,%d]" % (tag, r["k"], H_MIN, H_MAX),
                H_MIN <= r["h"] <= H_MAX, "h=%s" % r["h"])

    # ---- D: G line placement ----
    s_index = [i for i, (k, _, _, _) in enumerate(msgs) if k == "S"]
    for r, si in zip(periods, s_index):
        gs = [g for g in g_lines if g["after"] == si + 1]
        if r["q"] == 0:
            add("D GLINE %s k=%s q=0 has exactly one following G" % (tag, r["k"]),
                len(gs) == 1, "found=%d" % len(gs))
        else:
            add("D GLINE %s k=%s q=1 has no following G" % (tag, r["k"]),
                len(gs) == 0, "found=%d" % len(gs))

    # ---- E: G-line self-consistency ----
    for g in g_lines:
        n = g["nums"]
        try:
            s = int(n["s"]); a44 = int(n["a44"]); a45 = int(n["a45"])
            rd = int(n["rd"]); at = int(n["at"])
        except (TypeError, ValueError):
            add("E GCONS %s G@%d numeric fields" % (tag, g["after"]), False, repr(n))
            continue
        add("E GCONS %s s in 1..5" % tag, 1 <= s <= 5, "s=%d" % s)
        add("E GCONS %s a44/a45 in {0,1}" % tag, a44 in (0, 1) and a45 in (0, 1),
            "a44=%d a45=%d" % (a44, a45))
        add("E GCONS %s rd in 0..5" % tag, 0 <= rd <= 5, "rd=%d" % rd)
        add("E GCONS %s at in 1..3" % tag, 1 <= at <= 3, "at=%d" % at)
        raws = g["raw"]
        if not re.fullmatch(r"[0-9A-F]{12}|-{12}", raws):
            add("E GCONS %s raw is 12 upper-hex or 12 dashes" % tag, False, "%r" % raws)
        elif raws != "-" * 12:
            b = bytes.fromhex(raws)
            add("E GCONS %s raw CRC-8 recomputed" % tag,
                crc8_gxht(b[0:2]) == b[2] and crc8_gxht(b[3:5]) == b[5],
                "crc_t=%02X vs %02X, crc_h=%02X vs %02X"
                % (crc8_gxht(b[0:2]), b[2], crc8_gxht(b[3:5]), b[5]))
        # result code vs address-ACK facts.  EV-009 registered the tension observed on the
        # pre-fix build (s=2 printed together with a44=1, so a reader could not tell
        # "sensor absent" from "sensor present but unreadable").  The ITEM-002 fix reconciles
        # them, so both directions are now hard gates instead of an observation:
        #   s=2 (NO_DEVICE)          => neither candidate may have ACKed in this period
        #   s in {3,4,5} (IO/CRC/RANGE) => at least one candidate must have ACKed, because all
        #                                 three can only be reached after an address ACK
        if s == 2 and (a44 == 1 or a45 == 1):
            add("E GCONS %s G@%d s=2 requires no ACKed address" % (tag, g["after"]), False,
                "s=2 a44=%d a45=%d -> %r" % (a44, a45, g["_msg"]))
        else:
            add("E GCONS %s G@%d s=2 requires no ACKed address" % (tag, g["after"]), True)
        if s in (3, 4, 5) and not (a44 == 1 or a45 == 1):
            add("E GCONS %s G@%d s=%d requires an ACKed address" % (tag, g["after"], s), False,
                "s=%d a44=%d a45=%d -> %r" % (s, a44, a45, g["_msg"]))
        elif s in (3, 4, 5):
            add("E GCONS %s G@%d s=%d requires an ACKed address" % (tag, g["after"], s), True)

    # ---- F/G: cadence and previous-value advancement ----
    for a, b in zip(periods, periods[1:]):
        if b["k"] >= a["k"]:
            add("F CADENCE %s k %d->%d delta==%d" % (tag, a["k"], b["k"], SAMPLE_TICKS),
                b["k"] - a["k"] == SAMPLE_TICKS, "delta=%d" % (b["k"] - a["k"]))
        if a["q"] == 0:
            add("G ADVANCE %s k=%d failure does not advance prev (next p=%s)"
                % (tag, a["k"], b["p"]), b["p"] == a["p"],
                "p: %s -> %s" % (a["p"], b["p"]))

    for r in periods:
        print("    S %s k=%-4s q=%s t=%-6s h=%-5s p=%-6s H=%s V=%s o=%s n=%s x=%s a=%s D=%s r=%s s=%s y=%s"
              % (tag, r["k"], r["q"], r["t"], r["h"], r["p"], r["H"], r["V"], r["o"],
                 r["n"], r["x"], r["a"], r["D"], r["r"], r["s"], r["y"]))
    for g in g_lines:
        print("    %s" % g["_msg"])
    for io in io_lines:
        print("    %s" % io["_msg"])
    for s in sig_lines:
        print("    %s" % s["_msg"])
    for b in bus_lines:
        print("    %s" % b["_msg"])
    return periods, g_lines


def _selftest_capture(iosig_line, bus_line=b"BUS idle=3 scl=1 sda=1 ack=none",
                      iosig_after_bus=False):
    """构造一段最小合成捕获 (仅供 --selftest 使用, 不参与任何实板判定)。"""
    boot = b"BOOT fw=FD-002r5 uid=6A002C00 rst=0040 uart=9600\r\n"
    iot = b"IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none\r\n"
    s0 = (b"S k=0 p=0 H=0 V=0 o=8 n=3419 x=3425 a=3422 D=0 t=0 h=0 q=0 r=0 s=0 y=0\r\n")
    g = b"G s=2 a44=0 a45=0 rd=0 at=3 raw=------------\r\n"
    if iosig_line is None:
        return boot + iot + bus_line + b"\r\n" + s0 + g
    if iosig_after_bus:
        return boot + iot + bus_line + b"\r\n" + iosig_line + b"\r\n" + s0 + g
    return boot + iot + iosig_line + b"\r\n" + bus_line + b"\r\n" + s0 + g


def selftest():
    """定向变异负对照: 证明 L 组既能通过也能失败 (不是恒过的空验)。"""
    import os
    import tempfile

    cases = [
        ("baseline (correct signature, no device)",
         b"IOSIG scl=95555 sda=30303", b"BUS idle=3 scl=1 sda=1 ack=none", False, []),
        ("scl clock shape wrong",
         b"IOSIG scl=9555F sda=30303", b"BUS idle=3 scl=1 sda=1 ack=none", False,
         ["scl readback == driven clock shape"]),
        ("sda data bit wrong (address bit 7 flips)",
         b"IOSIG scl=95555 sda=10303", b"BUS idle=3 scl=1 sda=1 ack=none", False,
         ["sda data bits read back == driven address bits"]),
        ("ACK slot mixed (10)",
         b"IOSIG scl=95555 sda=30302", b"BUS idle=3 scl=1 sda=1 ack=none", False,
         ["ACK-slot bits self-consistent", "sda value is one of the two valid readings"]),
        ("device-ACK signature vs BUS ack=none (contradiction)",
         b"IOSIG scl=95555 sda=30300", b"BUS idle=3 scl=1 sda=1 ack=none", False,
         ["cross-check with BUS scan"]),
        ("device-ACK signature vs BUS ack=44 (consistent)",
         b"IOSIG scl=95555 sda=30300", b"BUS idle=3 scl=1 sda=1 ack=44", False, []),
        ("no-device signature vs BUS ack=44 (contradiction)",
         b"IOSIG scl=95555 sda=30303", b"BUS idle=3 scl=1 sda=1 ack=44", False,
         ["cross-check with BUS scan"]),
        ("IOSIG printed after BUS (order broken)",
         b"IOSIG scl=95555 sda=30303", b"BUS idle=3 scl=1 sda=1 ack=none", True,
         ["order BOOT<IOTEST<IOSIG<BUS"]),
        ("IOSIG line missing",
         None, b"BUS idle=3 scl=1 sda=1 ack=none", False,
         ["exactly one IOSIG in the completed boot run"]),
    ]
    bad = 0
    for name, line, bus, after_bus, expect_fail_substrings in cases:
        del results[:]
        rawp = _selftest_capture(line, bus, after_bus)
        fd, path = tempfile.mkstemp(suffix=".cap")
        try:
            os.write(fd, rawp)
            os.close(fd)
            check_file(path)
        finally:
            os.unlink(path)
        fails = [n for n, ok, _ in results if not ok]
        ok = True
        for sub in expect_fail_substrings:
            if not any(sub in f for f in fails):
                ok = False
                print("    SELFTEST MISS: %s -> expected a FAIL containing %r, got %r"
                      % (name, sub, fails))
        if not expect_fail_substrings and fails:
            ok = False
            print("    SELFTEST UNEXPECTED FAIL: %s -> %r" % (name, fails))
        if not ok:
            bad += 1
        print("    SELFTEST %-56s %s" % (name, "ok" if ok else "MISMATCH"))
    # 直接给定原始字节的用例 (A 组的行终止规则)
    s_tail = b"H=0 V=0 o=8 n=3419 x=3425 a=3422 D=0 t=0 h=0 q=0 r=0 s=0 y=0\r\n"
    g_tail = b"G s=2 a44=0 a45=0 rd=0 at=3 raw=------------\r\n"
    raw_cases = [
        ("reset-cut line (partial IOSIG then BOOT banner) is a note",
         b"BOOT fw=FD-002r5 uid=6A002C00 rst=0040 uart=9600\r\n"
         b"IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none\r\n"
         b"IOSIG scl=95555 sda=3030"                                   # cut mid-line
         b"BOOT fw=FD-002r5 uid=6A002C00 rst=0240 uart=9600\r\n"
         b"IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none\r\n"
         b"IOSIG scl=95555 sda=30303\r\n"
         b"BUS idle=3 scl=1 sda=1 ack=none\r\n"
         b"S k=0 p=0 " + s_tail + g_tail,
         []),
        ("line missing CRLF followed by the next S line (EV-006 D-ITEM001-1 class) fails",
         b"BOOT fw=FD-002r5 uid=6A002C00 rst=0040 uart=9600\r\n"
         b"IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none\r\n"
         b"IOSIG scl=95555 sda=30303\r\n"
         b"BUS idle=3 scl=1 sda=1 ack=none\r\n"
         b"S k=0 p=0 " + s_tail.replace(b"\r\n", b"") + b"S k=3 p=0 " + s_tail + g_tail,
         ["A TERM", "A CRLF"]),
        ("truncated garbage (not a prefix of any intact line) fails",
         b"BOOT fw=FD-002r5 uid=6A002C00 rst=0040 uart=9600\r\n"
         b"IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none\r\n"
         b"IOSIG scl=95555 sda=30303\r\n"
         b"BUS idle=3 scl=1 sda=1 ack=none\r\n"
         b"S k=0 p=0 " + s_tail.replace(b"\r\n", b"XX") + b"S k=3 p=0 " + s_tail + g_tail,
         ["A TERM"]),
    ]
    for name, rawp, expect_fail_substrings in raw_cases:
        del results[:]
        fd, path = tempfile.mkstemp(suffix=".cap")
        try:
            os.write(fd, rawp)
            os.close(fd)
            check_file(path)
        finally:
            os.unlink(path)
        fails = [n for n, ok, _ in results if not ok]
        ok = True
        for sub in expect_fail_substrings:
            if not any(sub in f for f in fails):
                ok = False
                print("    SELFTEST MISS: %s -> expected a FAIL containing %r, got %r"
                      % (name, sub, fails))
        if not expect_fail_substrings and fails:
            ok = False
            print("    SELFTEST UNEXPECTED FAIL: %s -> %r" % (name, fails))
        cases.append((name, None, None, None, expect_fail_substrings))
        if not ok:
            bad += 1
        print("    SELFTEST %-56s %s" % (name[:56], "ok" if ok else "MISMATCH"))
    print("\n===== SELFTEST: %s (%d/%d cases as expected) ====="
          % ("PASS" if bad == 0 else "FAIL", len(cases) - bad, len(cases)))
    return 0 if bad == 0 else 1


def main():
    if len(sys.argv) >= 2 and sys.argv[1] == "--selftest":
        return selftest()
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    all_periods = []
    for path in sys.argv[1:]:
        periods, _ = check_file(path)
        all_periods.extend(periods)
    all_periods.sort(key=lambda r: r["_off"])  # stable per file order is preserved below

    # ---- H: acceptance ----
    print("\n===== AGGREGATE =====")
    # order periods by (capture index, k) is already the chronological order of the files given
    seq = []
    for path in sys.argv[1:]:
        tag = path.split("/")[-1].split("\\")[-1]
        seq.extend([r for r in all_periods if r["_tag"] == tag])
    runs, cur = [], 0
    for r in seq:
        if r["q"] == 1:
            cur += 1
            runs.append(cur)
        else:
            cur = 0
    best = max(runs) if runs else 0
    q1 = sum(1 for r in seq if r["q"] == 1)
    q0 = sum(1 for r in seq if r["q"] == 0)
    print("    periods observed: %d  (q=1: %d, q=0: %d)" % (len(seq), q1, q0))
    print("    longest run of consecutive q=1 periods: %d" % best)
    for r in seq:
        print("      %s k=%-4s q=%s t=%s h=%s" % (r["_tag"], r["k"], r["q"], r["t"], r["h"]))
    add("H ACCEPT >=3 consecutive q=1 periods with t/h in domain", best >= 3,
        "longest run = %d (needed >= 3)" % best)

    # ---- K: registration of the E1 self-check readings across all resets ----
    print("\n===== IOTEST READINGS (E1) =====")
    seen = []
    for path in sys.argv[1:]:
        data = open(path, "rb").read()
        for m in re.finditer(rb"IOTEST [^\r\n]*", data):
            line = m.group(0).decode("ascii", "replace")
            seen.append(line)
            print("    %s" % line)
    if seen:
        uniq = sorted(set(seen))
        print("    distinct readings: %d of %d occurrences" % (len(uniq), len(seen)))
        for u in uniq:
            print("      x%d  %s" % (seen.count(u), u))
        # classification of the self-check (the tester does not decide the physical cause;
        # it records which branches the reading is compatible with)
        can_low = all(("sda_lo=0" in u) and ("scl_lo=0" in u) for u in uniq)
        rel_hi = all("idle=3" in u for u in uniq)
        no_swap = all("swap=none" in u for u in uniq)
        add("K IOTEST all readings: host drives BOTH lines low and reads them back low",
            can_low, "%d distinct reading(s)" % len(uniq))
        add("K IOTEST all readings: both lines read high after release (idle=3)",
            rel_hi, "%d distinct reading(s)" % len(uniq))
        add("K IOTEST all readings: no SDA/SCL role swap detected (swap=none)",
            no_swap, "%d distinct reading(s)" % len(uniq))
    else:
        add("K IOTEST at least one IOTEST reading observed", False, "no IOTEST line captured")

    # ---- L/M: registration of the E1b in-transaction bit-signature readings ----
    print("\n===== IOSIG READINGS (E1b) =====")
    exp_scl, exp_sda_none = iosig_expected(SIG_ADDR7, False)
    _e, exp_sda_ack = iosig_expected(SIG_ADDR7, True)
    print("    independently derived expectation: scl=%05X  sda=%05X (no device) / "
          "%05X (device ACK at 0x%02X)" % (exp_scl, exp_sda_none, exp_sda_ack, SIG_ADDR7))
    if SIG_READINGS:
        for t, n, line, scl, sda, dev_ack in SIG_READINGS:
            print("    [%s reset#%d] %s" % (t, n, line))
        shape_ok = all(scl == exp_scl and (sda >> 2) == (exp_sda_none >> 2)
                       for _, _, _, scl, sda, _ in SIG_READINGS)
        add("M IOSIG all readings: pin-level clock shape AND address data bits exactly as "
            "driven (=> H12 'bits never reach the pins' excluded)", shape_ok,
            "%d reading(s)" % len(SIG_READINGS))
        any_ack = any(d for _, _, _, _, _, d in SIG_READINGS)
        add("M IOSIG registration: no reading shows a device ACK at 0x%02X" % SIG_ADDR7,
            not any_ack, "device-ACK readings=%d" % sum(1 for r in SIG_READINGS if r[5]))
    else:
        print("    (no IOSIG line in these captures)")
        add("M IOSIG at least one IOSIG reading observed", False, "no IOSIG line captured")

    npass = sum(1 for _, ok, _ in results if ok)
    nfail = sum(1 for _, ok, _ in results if not ok)
    print("\n===== CHECKS =====")
    for name, ok, detail in results:
        if not ok:
            print("    FAIL  %s%s" % (name, ("  [%s]" % detail) if detail else ""))
    print("    (%d checks passed, %d failed)" % (npass, nfail))
    print("\n===== RESULT: %s =====" % ("PASS" if nfail == 0 else "FAIL"))
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
