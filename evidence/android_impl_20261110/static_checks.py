"""AA-005 G-1..G-7 实现后静态核查（可复跑）。

只读解析 Android 应用源码与清单，输出 PASS/FAIL 行；退出码 0 仅当全部 PASS。
不访问网络、设备或数据库。
"""
from __future__ import annotations

import io
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
APP = os.path.join(ROOT, "dengbei_care", "app")
MAIN = os.path.join(APP, "src", "main")
JAVA = os.path.join(MAIN, "java", "com", "jinyuni", "dengbei_care")


def read(path: str) -> str:
    return io.open(path, encoding="utf-8").read()


def walk(root: str):
    for base, _dirs, files in os.walk(root):
        for name in files:
            if name.endswith((".kt", ".xml", ".md", ".txt")):
                yield os.path.join(base, name)


results: list[tuple[str, bool, str]] = []


def check(name: str, ok: bool, detail: str = "") -> None:
    results.append((name, ok, detail))


# 1) 硬编码网关 BLE 地址只允许出现在迁移默认常量一处
mac_hits = []
for path in walk(MAIN):
    for i, line in enumerate(read(path).splitlines(), 1):
        if "84:C2:E4:03:02:02" in line:
            mac_hits.append((os.path.relpath(path, MAIN), i))
check(
    "A1 hardcoded gateway MAC only once (migration default)",
    len(mac_hits) == 1 and mac_hits[0][0].endswith("GatewayIdentity.kt"),
    str(mac_hits),
)

# 2) 旧转发/零值哨兵状态清零
forbidden = ["bleTempData", "bleHumiData", "bletime", "bleZeroCount", "isZeroValue"]
hits = []
for path in walk(JAVA):
    text = read(path)
    for token in forbidden:
        if token in text:
            hits.append((os.path.relpath(path, JAVA), token))
check("A2 no last-batch state / zero-sentinel", not hits, str(hits))

# 3) 不含 50 台静默截断
hits = []
for path in walk(JAVA):
    text = read(path)
    if "take(50)" in text or "maxCount = 50" in text:
        hits.append(os.path.relpath(path, JAVA))
check("A3 no 50-device silent truncation", not hits, str(hits))

# 4) 载荷构造只在 BleBatchForwarding 一处，且为 time:temps:humis、无空格
payload_sites = []
for path in walk(JAVA):
    text = read(path)
    if ':$temps:$humis' in text or re.search(r'\+ \',\' \+', text):
        payload_sites.append(os.path.relpath(path, JAVA))
check(
    "A4 single payload construction (time:temps:humis)",
    payload_sites == ["telemetry\\BleBatchForwarding.kt"] or payload_sites == ["telemetry/BleBatchForwarding.kt"],
    str(payload_sites),
)

# 5) 规则文案不再出现“下降超过 0.9”
hits = []
for path in walk(MAIN):
    if "下降超过 0.9" in read(path):
        hits.append(os.path.relpath(path, MAIN))
check("A5 no drop-direction rule text in app sources", not hits, str(hits))

# 6) 未新增鉴权头/令牌
hits = []
for path in walk(JAVA):
    text = read(path)
    for token in ("addHeader(\"Authorization", "Bearer ", "Authorization:"):
        if token in text:
            hits.append((os.path.relpath(path, JAVA), token))
check("A6 no unverified auth headers", not hits, str(hits))

# 7) 读路径使用实际返回字节数与 MTU 守卫
mqtt = read(os.path.join(JAVA, "MqtttService.kt"))
check(
    "A7 read completeness uses value.size + MTU guard",
    "characteristic.value" in mqtt
    and "val size = value?.size ?: 0" in mqtt
    and "lengthVerdict(size, parsed.devCount)" in mqtt
    and "mtuVerdict(negotiatedMtu" in mqtt,
)

# 8) 绑定容量与读取容量前置
notif = read(os.path.join(JAVA, "ui", "notifications", "NotificationsFragment.kt"))
check(
    "A8 binding capacity guard <= 9",
    "GatewayReadGuard.MAX_DEVICES" in notif and "GatewayReadGuard.MAX_DEVICES" in mqtt,
)

# 9) 版本号递增
gradle = read(os.path.join(APP, "build.gradle.kts"))
check(
    "A9 version bumped to 1.9 / 10",
    "versionCode = 10" in gradle and 'versionName = "1.9"' in gradle,
)

# 10) 时间语义注释已同步
api = read(os.path.join(JAVA, "ui", "zhuce", "ApiService.kt"))
cadence = read(os.path.join(JAVA, "telemetry", "SensorCadence.kt"))
check(
    "A10 time comment + cadence direction synced",
    "不是物理采样时刻" in api and "升高 >0.9" in cadence,
)

# 11) SQLite 表结构未改
db = read(os.path.join(JAVA, "TemperatureDatabaseHelper.kt"))
import subprocess
unchanged = subprocess.run(
    ["git", "diff", "--quiet", "HEAD", "--", "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/TemperatureDatabaseHelper.kt"],
    cwd=ROOT,
).returncode == 0
check(
    "A11 SQLite helper unchanged in this round (v3 schema intact)",
    unchanged and "DATABASE_VERSION = 3" in db,
)

# 12) pending_uploads 复合主键仍为 (device_id, time)
check(
    "A12 outbox key (device_id,time) intact",
    "PRIMARY KEY($COLUMN_DEVICE_ID, $COLUMN_TIME)" in db and "TABLE_PENDING_UPLOADS" in db,
)

# 13) 读前容量前置：>9 不连接（代码路径顺序）
idx_capacity = mqtt.find("BINDING_CAPACITY_EXCEEDED")
idx_getremote = mqtt.find("getRemoteDevice(address)")
check(
    "A13 capacity check precedes GATT connect",
    0 <= idx_capacity < idx_getremote,
    f"capacity@{idx_capacity} getRemoteDevice@{idx_getremote}",
)

# 14) 无新增 BLE 服务/控制命令（仅 FFE0/FFE2/FFE3 既有 UUID）
uuids = set(re.findall(r"0000[0-9a-fA-F]{4}-0000-1000-8000-00805[fF]9[bB]34[fF][bB]", mqtt + notif + read(os.path.join(JAVA, "ui", "gateway", "GatewayBleSettingsDialog.kt"))))
check(
    "A14 no new GATT UUIDs introduced",
    uuids <= {"0000ffe0-0000-1000-8000-00805F9B34FB", "0000ffe2-0000-1000-8000-00805F9B34FB", "0000ffe3-0000-1000-8000-00805F9B34FB"},
    str(sorted(uuids)),
)

print("AA-005 static checks")
for name, ok, detail in results:
    print(("PASS " if ok else "FAIL ") + name + ((" | " + detail) if detail and not ok else ""))
failed = [r for r in results if not r[1]]
print("RESULT:", "OK" if not failed else f"FAILED({len(failed)})")
sys.exit(0 if not failed else 1)
