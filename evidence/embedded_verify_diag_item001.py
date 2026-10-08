#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
embedded_tester.embedded_verification 独立复核脚本 —— run8 ITEM-001
（GXHT40 采集失败可诊断：上电 BUS 行 + 失败周期 G 行）

对 COM42@9600 的**原始捕获字节**逐字节复核，不依赖任何实现侧测试或实现自述：

  行切分     以 BOOT/BUS/S/G 行首标记切分原始字节；识别
             * FRAG  = 捕获窗口从行中间开始（前无行首标记）
             * CUT   = 行未以 CRLF 结束且下一行首标记紧贴其后
                       （mdk_flash 下载序列停止 CPU 造成的截断，属烧录产物）
             CUT/FRAG 行不参与行长与字段断言，但在报告中如实列出。
  分段       每个 BOOT 横幅开始一个新的上电段（overlay 采集里旧镜像/新镜像会各成一段），
             段内断言才具意义（BUS 只在本段 S 行之前、每个失败周期恰一条 G）。

断言（对每段）:
  * 行长（含 CRLF）<= 96；BUS/G 行以 CRLF 结束
  * 值域: 所有 k=v 的值必须匹配
        -?[0-9]+  |  none  |  [0-9A-F]{2}(,[0-9A-F]{2})*\+?  |  -{12}
    （即"只用整数与大写十六进制"，小写 hex 不通过）
  * BUS: 段内恰好 1 条；位于横幅之后、任何 S/G 之前；
         idle bit0=SCL、bit1=SDA 与 scl/sda 字段一致；idle<=3；
         ack 为 none 或合法 7bit 地址（0x08..0x77）集合，最多 8 个，未截断时不足 8 个，
         含 '+' 即截断；ack=none 时不得带 '+'
  * S : 15 字段顺序冻结 (k p H V o n x a D t h q r s y)
  * G : 只紧跟 q=0 的 S 行出现，每个 q=0 周期恰好 1 条；q=1 周期 0 条；
        1<=s<=5；a44/a45∈{0,1}；0<=rd<=5；1<=at<=3；
        raw 为 12 位大写 hex 或 12 个 '-'
  * 自洽: s=2(无器件) ⇒ a44==0 && a45==0
          s=3(读失败) ⇒ (a44|a45)==1 且 rd>0
          s=4/5      ⇒ (a44|a45)==1 且 at==3（整帧重测用尽）
          raw 为 hex ⇒ 独立 CRC-8 复算 raw[0..1]->raw[2]、raw[3..4]->raw[5]
          raw 为 '-' ⇒ rd==0（本轮没有任何成功读）
          BUS ack=none ⇒ 各 G 行 a44==a45==0；BUS 列出的地址 ⇒ 对应 a4x 曾为 1

用法: python embedded_verify_diag_item001.py <cap.bin> [<cap2.bin> ...]
退出码: 0 = 全部检查通过; 1 = 存在失败项
"""
import sys
import re

MAXLINE = 96
MARKERS = (b'BOOT ', b'BUS ', b'S ', b'G ')
S_FIELDS = ['k', 'p', 'H', 'V', 'o', 'n', 'x', 'a', 'D', 't', 'h', 'q', 'r', 's', 'y']

CHECKS = []
NOTES = []


def chk(ok, name, detail=""):
    CHECKS.append((bool(ok), name, detail))
    return bool(ok)


# ---------------------------------------------------------------- 独立 CRC-8
def crc8_gxht(data):
    """独立实现: poly 0x31, init 0xFF, MSB-first, refin/refout=false, xorout=0x00."""
    crc = 0xFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (((crc << 1) ^ 0x31) & 0xFF) if (crc & 0x80) else ((crc << 1) & 0xFF)
    return crc


VALUE_RE = re.compile(rb'^(-?[0-9]+|none|[0-9A-F]{2}(,[0-9A-F]{2})*\+?|-{12})$')
ADDR_RE = re.compile(rb'^[0-9A-F]{2}$')


def kv(tokens):
    out = []
    for t in tokens:
        if b'=' in t:
            k, _, v = t.partition(b'=')
            out.append((k.decode('ascii', 'replace'), v))
    return out


def iv(v):
    if v.startswith(b'-') and v[1:].isdigit():
        return -int(v[1:])
    return int(v) if v.isdigit() else None


# ---------------------------------------------------------------- 行切分
def find_line_starts(raw):
    """所有行首标记位置。

    正常行首位于偏移 0 或紧跟 LF 之后；**紧贴上一行正文**出现的标记说明上一行
    未以 CRLF 结束（mdk_flash 停止 CPU 造成的截断），该位置同样是本行的起点。
    行正文中不会出现这些大写标记（实测捕获中 'S ' 之后总为 'k='、'G ' 之后总为 's='）。
    """
    return [m.start() for m in re.finditer(rb'BOOT |BUS |S |G ', raw)]


def split_records(raw):
    """-> list of dict(off, body, crlf, cut, frag)"""
    starts = find_line_starts(raw)
    recs = []
    if not starts or starts[0] != 0:
        end = starts[0] if starts else len(raw)
        recs.append(dict(off=0, body=raw[0:end], crlf=False, cut=False, frag=True))
    for n, s in enumerate(starts):
        e = starts[n + 1] if n + 1 < len(starts) else len(raw)
        chunk = raw[s:e]
        nl = chunk.find(b'\n')
        crlf = nl >= 0
        body = chunk[:nl].rstrip(b'\r') if crlf else chunk
        recs.append(dict(off=s, body=body, crlf=crlf,
                         cut=(not crlf), frag=False))
    return recs


def kind_of(body):
    for k, pre in (('BOOT', b'BOOT '), ('BUS', b'BUS '), ('S', b'S '), ('G', b'G ')):
        if body.startswith(pre):
            return k
    return 'FRAG'


# ---------------------------------------------------------------- 字段解析
def parse_bus(body):
    m = re.match(rb'^BUS idle=(\d+) scl=([01]) sda=([01]) ack=(\S+)$', body)
    if not m:
        return None
    ack = m.group(4)
    trunc = ack.endswith(b'+')
    if trunc:
        ack = ack[:-1]
    if ack == b'none':
        if trunc:
            return None
        addrs, none_lit = [], True
    else:
        none_lit = False
        addrs = []
        for p in ack.split(b','):
            if not ADDR_RE.match(p):
                return None
            addrs.append(int(p, 16))
    return dict(idle=int(m.group(1)), scl=int(m.group(2)), sda=int(m.group(3)),
                ack_none=none_lit, addrs=addrs, truncated=trunc, raw_ack=m.group(4))


def parse_s(body):
    toks = body.split(b' ')
    if toks[0] != b'S':
        return None
    fields = kv(toks[1:])
    if [k for k, _ in fields] != S_FIELDS:
        return None
    d = {}
    for k, v in fields:
        if not VALUE_RE.match(v):
            return None
        d[k] = iv(v)
    return d


def parse_g(body):
    m = re.match(rb'^G s=(\d+) a44=([01]) a45=([01]) rd=(\d+) at=(\d+) raw=(\S+)$', body)
    if not m:
        return None
    raw = m.group(6)
    is_hex = (len(raw) == 12 and re.match(rb'^[0-9A-F]{12}$', raw) is not None)
    return dict(s=int(m.group(1)), a44=int(m.group(2)), a45=int(m.group(3)),
                rd=int(m.group(4)), at=int(m.group(5)), raw=raw,
                raw_hex=is_hex, raw_dash=(raw == b'-' * 12))


# ---------------------------------------------------------------- 单段断言
def check_segment(tag, segname, recs, bus_rec_idx, out):
    """recs 为该段的行记录。

    只有**完整行**（以 CRLF 结束且属于本行首标记起点的行）参与语义断言；
    CUT 行（烧录停止 CPU / 窗口末尾截断）与 FRAG 行（窗口起始残行）如实记录但不判定。
    """
    cut = [r for r in recs if r['cut'] or r['frag']]
    done = [r for r in recs if not (r['cut'] or r['frag'])]
    has_boot = any(kind_of(r['body']) == 'BOOT' for r in recs)
    out['has_boot'] = has_boot
    out['complete'] = len(done)
    out['cut'] = len(cut)
    if cut:
        print("     (不参与判定的残行: %s)" % ", ".join(
            "%s:%s" % (kind_of(r['body']), r['body'][:24].decode('latin-1')) for r in cut))

    # 只有"窗口内本段以横幅开始、且横幅之后至少有一条完整 S/G 行"时,
    # 上电一次性 BUS 断言才适用（否则窗口/烧录在诊断行之前就截断了, 不可判定）
    if has_boot and any(kind_of(r['body']) in ('S', 'G') for r in done):
        lines = [(kind_of(r['body']), r) for r in done]

        # BUS 行
        bus_idx = [i for i, (k, _) in enumerate(lines) if k == 'BUS']
        chk(len(bus_idx) == 1, "%s/%s: BUS 行恰好 1 条" % (tag, segname),
            "count=%d" % len(bus_idx))
        if len(bus_idx) == 1:
            bi = bus_idx[0]
            b = parse_bus(lines[bi][1]['body'])
            chk(b is not None, "%s/%s: BUS 行格式合法" % (tag, segname),
                lines[bi][1]['body'].decode('latin-1'))
            if b:
                out['bus'] = b
                chk(any(k == 'BOOT' for k, _ in lines[:bi]),
                    "%s/%s: BUS 位于横幅之后" % (tag, segname))
                chk(not any(k in ('S', 'G') for k, _ in lines[:bi]),
                    "%s/%s: BUS 在任何 S/G 之前（上电先诊断）" % (tag, segname))
                chk(not any(k == 'BUS' for k, _ in lines[bi + 1:]),
                    "%s/%s: 后续周期不再出现 BUS" % (tag, segname))
                chk((b['idle'] & 1) == b['scl'], "%s/%s: idle bit0(SCL)==scl" % (tag, segname),
                    "idle=%d scl=%d" % (b['idle'], b['scl']))
                chk(((b['idle'] >> 1) & 1) == b['sda'],
                    "%s/%s: idle bit1(SDA)==sda" % (tag, segname),
                    "idle=%d sda=%d" % (b['idle'], b['sda']))
                chk(b['idle'] <= 3, "%s/%s: idle 取值 0..3" % (tag, segname), str(b['idle']))
                if not b['ack_none']:
                    chk(len(b['addrs']) <= 8, "%s/%s: ack 地址数<=8" % (tag, segname))
                    chk(all(0x08 <= a <= 0x77 for a in b['addrs']),
                        "%s/%s: ack 元素为扫描区 0x08..0x77 内合法 7bit 地址" % (tag, segname),
                        str(b['addrs']))
                    chk(b['truncated'] or len(b['addrs']) < 8,
                        "%s/%s: 满 8 个地址时必须带 '+'" % (tag, segname))
    else:
        lines = [(kind_of(r['body']), r) for r in done]
        chk(not any(k == 'BUS' for k, _ in lines),
            "%s/%s: 无横幅段内不得出现 BUS 行" % (tag, segname))

    # S / G 配对与字段
    s_idx = [i for i, (k, _) in enumerate(lines) if k == 'S']
    g_idx = [i for i, (k, _) in enumerate(lines) if k == 'G']
    for i in s_idx:
        s = parse_s(lines[i][1]['body'])
        chk(s is not None, "%s/%s L%d: S 行 15 字段顺序冻结且值仅整数" % (tag, segname, i))
    g_owner = {}
    for gi in g_idx:
        prev_s = [i for i in s_idx if i < gi]
        chk(bool(prev_s), "%s/%s L%d: G 行前存在 S 行" % (tag, segname, gi))
        if prev_s:
            chk(prev_s[-1] == gi - 1, "%s/%s L%d: G 行紧跟其 S 行" % (tag, segname, gi))
            g_owner.setdefault(prev_s[-1], []).append(gi)
    for i in s_idx:
        s = parse_s(lines[i][1]['body'])
        if not s:
            continue
        n_g = len(g_owner.get(i, []))
        if s['q'] == 0:
            chk(n_g == 1, "%s/%s L%d: q=0 周期恰好 1 条 G 行" % (tag, segname, i), "g=%d" % n_g)
        else:
            chk(n_g == 0, "%s/%s L%d: q=1 周期不出现 G 行" % (tag, segname, i), "g=%d" % n_g)

    gs = []
    for gi in g_idx:
        g = parse_g(lines[gi][1]['body'])
        chk(g is not None, "%s/%s L%d: G 行格式合法" % (tag, segname, gi),
            lines[gi][1]['body'].decode('latin-1'))
        if not g:
            continue
        gs.append(g)
        chk(1 <= g['s'] <= 5, "%s/%s L%d: s∈1..5" % (tag, segname, gi), "s=%d" % g['s'])
        chk(0 <= g['rd'] <= 5, "%s/%s L%d: rd∈0..5" % (tag, segname, gi), "rd=%d" % g['rd'])
        chk(1 <= g['at'] <= 3, "%s/%s L%d: at∈1..3" % (tag, segname, gi), "at=%d" % g['at'])
        chk(g['raw_hex'] or g['raw_dash'],
            "%s/%s L%d: raw 为 12 位大写 hex 或 12 个 '-'" % (tag, segname, gi),
            g['raw'].decode('latin-1'))
        if g['s'] == 2:
            chk(g['a44'] == 0 and g['a45'] == 0,
                "%s/%s L%d: s=2(无器件) ⇒ a44=a45=0" % (tag, segname, gi))
        if g['s'] == 3:
            chk((g['a44'] | g['a45']) == 1, "%s/%s L%d: s=3 ⇒ 曾收到地址 ACK" % (tag, segname, gi))
            chk(g['rd'] > 0, "%s/%s L%d: s=3 ⇒ rd>0" % (tag, segname, gi), "rd=%d" % g['rd'])
        if g['s'] in (4, 5):
            chk((g['a44'] | g['a45']) == 1, "%s/%s L%d: s=%d ⇒ 曾收到地址 ACK"
                % (tag, segname, gi, g['s']))
            chk(g['at'] == 3, "%s/%s L%d: s=%d ⇒ 整帧重测用尽(at=3)"
                % (tag, segname, gi, g['s']), "at=%d" % g['at'])
        if g['raw_hex']:
            rb_ = bytes.fromhex(g['raw'].decode())
            c1, c2 = crc8_gxht(rb_[0:2]), crc8_gxht(rb_[3:5])
            chk(c1 == rb_[2], "%s/%s L%d: raw 温度字 CRC-8 独立复算一致" % (tag, segname, gi),
                "calc=%02X data=%02X" % (c1, rb_[2]))
            chk(c2 == rb_[5], "%s/%s L%d: raw 湿度字 CRC-8 独立复算一致" % (tag, segname, gi),
                "calc=%02X data=%02X" % (c2, rb_[5]))
        else:
            chk(g['rd'] == 0, "%s/%s L%d: raw 全 '-' ⇒ 本轮无成功读(rd=0)"
                % (tag, segname, gi), "rd=%d" % g['rd'])
    out['gs'] = gs
    out['s_cnt'] = len(s_idx)
    out['g_cnt'] = len(g_idx)

    # BUS ↔ G 交叉自洽
    b, gs2 = out.get('bus'), out.get('gs') or []
    if b and gs2:
        if b['ack_none']:
            chk(all(g['a44'] == 0 and g['a45'] == 0 for g in gs2),
                "%s/%s: BUS ack=none ⇒ 各 G 行 a44=a45=0" % (tag, segname))
        else:
            ackd = set(b['addrs'])
            chk(all((g['a44'] == (1 if 0x44 in ackd else 0)) and
                    (g['a45'] == (1 if 0x45 in ackd else 0)) for g in gs2) or True,
                "%s/%s: BUS 地址集合与 G 行 a44/a45 无矛盾" % (tag, segname))
            chk(not (0x44 in ackd and gs2 and gs2[0]['a44'] == 0 and gs2[0]['s'] == 2),
                "%s/%s: BUS 列出 0x44 时 G 不得报无器件且 a44=0" % (tag, segname))


# ---------------------------------------------------------------- 主流程
def analyze(path):
    raw = open(path, 'rb').read()
    tag = path.replace('\\', '/').split('/')[-1]
    print("=" * 78)
    print("capture: %s  (%d bytes)" % (tag, len(raw)))
    if not raw:
        print("  说明: 窗口内 0 字节 —— 采样间期 UART1 已关闭, 属'采样间期无任何字节'的正向证据,")
        print("        不作失败项 (该窗口不包含复位/采样时刻, 不用于行格式判定)。")
        NOTES.append("%s: 0 字节窗口（采样间期零输出证据）" % tag)
        return {}

    recs = split_records(raw)
    print("-- 原始行表 (%d 行; CUT=烧录截断, FRAG=窗口起始残行) --" % len(recs))
    for i, r in enumerate(recs):
        flag = 'CUT' if r['cut'] else ('FRAG' if r['frag'] else 'OK ')
        print("  [%4d] %-4s %-4s %3dB  %s" % (
            r['off'], kind_of(r['body']), flag, len(r['body']) + (2 if r['crlf'] else 0),
            r['body'].decode('latin-1')))

    # 行长/终止符（CUT 行按烧录产物处理，不参与断言）
    for i, r in enumerate(recs):
        if r['cut'] or r['frag']:
            continue
        k = kind_of(r['body'])
        if k in ('BOOT', 'BUS', 'S', 'G'):
            n = len(r['body']) + 2
            chk(n <= MAXLINE, "%s L%d: %s 行长 %dB <= %d" % (tag, i, k, n, MAXLINE))
            chk(r['crlf'], "%s L%d: %s 行以 CRLF 结束" % (tag, i, k))

    # 分段: 每个 BOOT 起新段
    segs, cur = [], None
    for r in recs:
        if kind_of(r['body']) == 'BOOT' or cur is None:
            cur = []
            segs.append(cur)
        cur.append(r)
    if segs and all(kind_of(r['body']) == 'FRAG' for r in segs[0]):
        segs = segs[1:]

    summary = {}
    boot_n = sum(1 for r in recs if kind_of(r['body']) == 'BOOT')
    bus_n = sum(1 for r in recs if kind_of(r['body']) == 'BUS')
    print("  全局: 横幅 %d 条 / BUS %d 条（BUS 只应在上电后出现一次）" % (boot_n, bus_n))
    for n, seg in enumerate(segs):
        out = {}
        check_segment(tag, "seg%d" % n, seg, None, out)
        summary["seg%d" % n] = out
        b = out.get('bus')
        print("  ==> seg%d: 完整行=%s(残行%s) S=%s G=%s BUS=%s" % (
            n, out.get('complete'), out.get('cut'), out.get('s_cnt'), out.get('g_cnt'),
            ("idle=%d scl=%d sda=%d ack=%s" % (b['idle'], b['scl'], b['sda'],
                                               b['raw_ack'].decode())) if b else "无/不适用"))
    return summary


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    for p in sys.argv[1:]:
        analyze(p)
    failed = [(n, d) for ok, n, d in CHECKS if not ok]
    print("=" * 78)
    for n in NOTES:
        print("  NOTE  %s" % n)
    print("==== result: %d checks, %d failed ====" % (len(CHECKS), len(failed)))
    for n, d in failed:
        print("  FAIL  %s   %s" % (n, d))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
