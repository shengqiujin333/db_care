#!/usr/bin/env python3
"""独立 Python 参考：BLE 聚合帧解码表的 FNV-1a 哈希（software_tester.software_verification / ITEM-001）。

用途：为 `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/verification/` 下的独立验证测试
提供**不依赖被测实现**的期望常量。期望值直接由接口契约 IC-002 §4 / AA-002 §5.1 推导：

  头块 16 B : gateway_id(6 B) + devCount(1 B @6)
  设备块    : 前 8 B 有效 = id(4 B 原字节序) | humidity_x10(u16be@4) | temperature_x10(s16be@6)
  帧长      : 16 * (1 + devCount)

哈希定义（与 Kotlin 测试内一致）：
  h = 0xCBF29CE484222325
  for i in 0..65535:  h ^= i; h *= 0x100000001B3; h ^= v(i); h *= 0x100000001B3   (全部按 64 bit 无符号回绕)
  温度表 v(i) = i - 0x10000 if i & 0x8000 else i     # 有符号
  湿度表 v(i) = i                                    # 无符号

运行：`python software_reference_frame_codec.py`
"""

MASK = (1 << 64) - 1
PRIME = 0x100000001B3


def fnv1a(pairs):
    h = 0xCBF29CE484222325
    for i, v in pairs:
        h ^= (i & 0xFFFFFFFF)
        h = (h * PRIME) & MASK
        h ^= (v & 0xFFFFFFFF)
        h = (h * PRIME) & MASK
    return h


def signed16(raw):
    return raw - 0x10000 if raw & 0x8000 else raw


if __name__ == "__main__":
    temp = [(raw, signed16(raw)) for raw in range(0x10000)]
    hum = [(raw, raw) for raw in range(0x10000)]
    print("TEMPERATURE_TABLE_FNV1A = 0x%016X" % fnv1a(temp))
    print("HUMIDITY_TABLE_FNV1A    = 0x%016X" % fnv1a(hum))
    # 关键向量（供人工复核）
    for raw in (0x00FA, 0xFF9C, 0xFE70, 0x04E2, 0x8000, 0x7FFF):
        print("temp raw=0x%04X -> %d (%.1f)" % (raw, signed16(raw), signed16(raw) / 10.0))
