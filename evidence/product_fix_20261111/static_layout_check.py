"""D-01/D-02/D-03/D-04a 布局与文案静态核查（可复跑）。

只读解析 Android 资源与相关 Kotlin 文件；不访问设备/网络。
退出码 0 仅当全部 PASS。
"""
from __future__ import annotations

import io
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MAIN = os.path.join(ROOT, "dengbei_care", "app", "src", "main")
RES = os.path.join(MAIN, "res")
JAVA = os.path.join(MAIN, "java", "com", "jinyuni", "dengbei_care")

results: list[tuple[str, bool, str]] = []


def read(path: str) -> str:
    return io.open(path, encoding="utf-8").read()


def check(name: str, ok: bool, detail: str = "") -> None:
    results.append((name, ok, detail))


activity = read(os.path.join(RES, "layout", "activity_main.xml"))
notif = read(os.path.join(RES, "layout", "fragment_notifications.xml"))
home = read(os.path.join(RES, "layout", "fragment_home.xml"))
frag = read(os.path.join(JAVA, "ui", "notifications", "NotificationsFragment.kt"))

# A. NavHost 不再使用 match_parent 高度，底部止于 nav_view 顶部
m = re.search(
    r'<fragment[^>]*android:id="@\+id/nav_host_fragment_activity_main"(.*?)/>',
    activity,
    re.S,
)
nav_block = m.group(1) if m else ""
check(
    "A1 NavHost height=0dp (constraint-bounded)",
    'android:layout_height="0dp"' in nav_block,
    "height not 0dp" if m else "NavHost block not found",
)
check(
    "A2 NavHost bottom constrained to nav_view",
    'app:layout_constraintBottom_toTopOf="@id/nav_view"' in nav_block,
)

# B. 「我的」页：滚动容器、无 parent 底部锚定、控件齐全且顺序正确
check(
    "B1 notifications root is NestedScrollView with fillViewport",
    notif.lstrip().startswith("<?xml") and "<androidx.core.widget.NestedScrollView" in notif.split('<LinearLayout', 1)[0]
    and 'android:fillViewport="true"' in notif,
)
check(
    "B2 notifications has no bottom_toBottomOf=parent",
    'bottom_toBottomOf="parent"' not in notif,
)
required_ids = [
    "usernameText",
    "registerButton",
    "sensorAddressLabel",
    "sensorAddressInput",
    "macSendButton",
    "macScanButton",
    "sensoridinfo",
    "sensoridinput",
    "sensoridscan",
    "sensornameinput",
    "addsensorid",
    "opensensorlistbutton",
    "viewReportButton",
    "gatewayBleButton",
]
missing = [i for i in required_ids if f'@+id/{i}"' not in notif]
check("B3 all required control ids preserved", not missing, str(missing))

idx_add = notif.find('@+id/addsensorid"')
idx_gateway = notif.find('@+id/gatewayBleButton"')
check(
    "B4 addsensorid precedes gatewayBleButton in vertical LinearLayout (no overlap)",
    0 <= idx_add < idx_gateway and notif.count("orientation=\"vertical\"") >= 1,
    f"add@{idx_add} gateway@{idx_gateway}",
)
check(
    "B5 quick entries placed before address section (top reachable)",
    notif.find('@+id/viewReportButton"') < notif.find('@+id/sensorAddressLabel"')
    and notif.find('@+id/opensensorlistbutton"') < notif.find('@+id/sensorAddressLabel"'),
)
# 所有旧按钮仍由该 Fragment 绑定并设置回调
for btn, handler in [
    ("viewReportButton", "viewReportButton.setOnClickListener"),
    ("opensensorlistbutton", "opensensorlistbutton.setOnClickListener"),
    ("addsensorid", "sensoridsaveButton.setOnClickListener"),
    ("gatewayBleButton", "gatewayBleButton.setOnClickListener"),
]:
    check(f"B6 {btn} click handler present", handler in frag)

# C. 首页引导文案指向实际可达入口
check(
    "C1 home hint uses real path (去添加 / 打开传感器列表)",
    "去添加" in home and "打开传感器列表" in home,
)
check(
    "C2 old wrong hint removed",
    "「我的」→「传感器列表」" not in home,
)

# D. 版本与兼容面未变
gradle = read(os.path.join(ROOT, "dengbei_care", "app", "build.gradle.kts"))
check(
    "D1 version unchanged 10 / 1.9",
    "versionCode = 10" in gradle and 'versionName = "1.9"' in gradle,
)
# 底部导航仍由 nav_view 承载
check(
    "D2 bottom navigation preserved",
    'android:id="@+id/nav_view"' in activity and "@menu/bottom_nav_menu" in activity,
)

print("D-01/D-02/D-03/D-04a static checks")
for name, ok, detail in results:
    print(("PASS " if ok else "FAIL ") + name + ((" | " + detail) if detail and not ok else ""))
failed = [r for r in results if not r[1]]
print("RESULT:", "OK" if not failed else f"FAILED({len(failed)})")
sys.exit(0 if not failed else 1)
