#!/usr/bin/env python3
"""独立静态核查（software_tester.software_verification / ITEM-002；TD-SW-002 T-SW-L1-03）。

检查"小端 humidity|temperature 解析入口"是否已从 Android 源工程移除，以及 BLE/MQTT 两条**在用入口**
是否只经 `protocol/GatewayFrameCodec`。检查对象与判定范围：

  A. `dengbei_care/app/src/**`（Android 编译源码，含 main 与 test）：
     A1 `parseHexData` 出现次数必须为 0（定义、调用、注释一律不允许）；
     A2 不得存在"小端 16 bit 组合"写法 `X or (Y shl 8)`（低字节在前）；
        允许大端写法 `(X shl 8) or Y`，例如 `GatewayFrameCodec.u16be`。
  B. 在用入口（`dengbei_care/app/src/main/**`）：
     B1 BLE 读取回调经 `decryptAndParseEcbFrame` 且按 `GatewayFrameCodec.Result.Rejected/Success` 分支；
     B2 `decodeAggregatedFrame` 在 app/src/main 中只由 `MqtttService` 的解密入口调用（即唯一在用入口）。
  C. 仓库范围内的其它命中只作为**信息**输出（文档/参考快照/被忽略的构建产物），不计入 A/B 判定。

退出码：0 = A/B 全部通过；1 = 存在违规。运行：`python evidence/software_static_check_no_little_endian_parser.py`
"""

from __future__ import annotations

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP_SRC = os.path.join(ROOT, "dengbei_care", "app", "src")
MQTTT = os.path.join(APP_SRC, "main", "java", "com", "jinyuni", "dengbei_care", "MqtttService.kt")

KT_SUFFIX = (".kt", ".java")
LITTLE_ENDIAN = re.compile(r"\bor\s*\(\s*[\w\.\[\]]+(?:\.toInt\(\))?\s*shl\s*8\s*\)")


def iter_sources(base):
    for dirpath, _dirnames, filenames in os.walk(base):
        for name in filenames:
            if name.endswith(KT_SUFFIX):
                yield os.path.join(dirpath, name)


def main() -> int:
    violations: list[str] = []

    # ---- A. Android 编译源码 ----
    a1 = []
    a2 = []
    for path in iter_sources(APP_SRC):
        rel = os.path.relpath(path, ROOT).replace("\\", "/")
        with open(path, encoding="utf-8") as fh:
            for lineno, line in enumerate(fh, 1):
                if "parseHexData" in line:
                    a1.append(f"{rel}:{lineno}: {line.strip()[:120]}")
                if LITTLE_ENDIAN.search(line):
                    a2.append(f"{rel}:{lineno}: {line.strip()[:120]}")

    print("== A1 parseHexData in app/src (expect 0) ==")
    print("\n".join(a1) if a1 else "  0 hits")
    print("== A2 little-endian combine `X or (Y shl 8)` in app/src (expect 0) ==")
    print("\n".join(a2) if a2 else "  0 hits")
    if a1:
        violations.append(f"A1: parseHexData still present ({len(a1)})")
    if a2:
        violations.append(f"A2: little-endian combine present ({len(a2)})")

    # ---- B. 在用入口 ----
    with open(MQTTT, encoding="utf-8") as fh:
        mqtt = fh.read()
    checks = {
        "B1a uses decryptAndParseEcbFrame": "decryptAndParseEcbFrame(value)" in mqtt,
        "B1b handles Result.Rejected": "GatewayFrameCodec.Result.Rejected ->" in mqtt,
        "B1c handles Result.Success": "GatewayFrameCodec.Result.Success ->" in mqtt,
        "B1d no local frame parse implementation": "fun parsePlainFrame" not in mqtt,
    }
    print("== B 在用入口 ==")
    for name, ok in checks.items():
        print(("  PASS  " if ok else "  FAIL  ") + name)
        if not ok:
            violations.append(f"B: {name}")

    decode_callers = []
    main_root = os.path.join(APP_SRC, "main")
    for path in iter_sources(main_root):
        rel = os.path.relpath(path, ROOT).replace("\\", "/")
        with open(path, encoding="utf-8") as fh:
            for lineno, line in enumerate(fh, 1):
                if "decodeAggregatedFrame" in line and "fun decodeAggregatedFrame" not in line:
                    decode_callers.append(f"{rel}:{lineno}: {line.strip()[:120]}")
    print("== B2 decodeAggregatedFrame call sites in app/src/main ==")
    print("\n".join(decode_callers) if decode_callers else "  (none)")
    # 在用代码中只允许 MqtttService 的解密入口调用它（codec 自身为定义处）
    unexpected = [c for c in decode_callers
                  if "GatewayFrameCodec.kt" not in c and "MqtttService.kt" not in c]
    if not decode_callers:
        violations.append("B2: no live call site for decodeAggregatedFrame")
    if unexpected:
        violations.append("B2: decodeAggregatedFrame called from unexpected main-source file")

    # ---- C. 仓库范围信息（不计入判定） ----
    print("== C 仓库其它命中（信息，不计入判定） ==")
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in (".git", ".gradle")]
        for name in filenames:
            path = os.path.join(dirpath, name)
            rel = os.path.relpath(path, ROOT).replace("\\", "/")
            if rel.startswith("dengbei_care/app/src"):
                continue
            try:
                with open(path, encoding="utf-8") as fh:
                    for lineno, line in enumerate(fh, 1):
                        if "parseHexData" in line:
                            print(f"  {rel}:{lineno}: {line.strip()[:110]}")
            except (UnicodeDecodeError, OSError):
                continue

    print("== RESULT ==")
    if violations:
        for v in violations:
            print("  VIOLATION:", v)
        return 1
    print("  OK: 小端解析入口已从 Android 源工程移除；在用入口只经 GatewayFrameCodec")
    return 0


if __name__ == "__main__":
    sys.exit(main())
