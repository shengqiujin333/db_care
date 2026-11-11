"""Independent UI-structure checks for the Android D-01/D-02/D-03/D-04a re-verification.

Parses raw uiautomator XML dumps captured on the real device (Redmi 8A,
serial 12f046690506) after installing the rebuilt APK
sha256 71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8.

Read-only: no network, no device access. Exit code 0 = all checks PASS.
"""
import sys
import xml.etree.ElementTree as ET

BASE = "evidence/android_check_20261111/"
NAV_TOP = 1264  # bottom navigation bar top edge observed on all dumps (720x1520)


def load(path):
    return ET.parse(BASE + path).getroot()


def nodes(root):
    for e in root.iter("node"):
        yield e.attrib


def by_id(root, rid):
    hits = [a for a in nodes(root) if a.get("resource-id") == rid]
    return hits[0] if len(hits) == 1 else None


def bounds(a):
    b = a.get("bounds", "")
    # bounds="[x1,y1][x2,y2]"
    left, top = b[1:].split("]")[0].split(",")
    right, bottom = b.split("][")[1].rstrip("]").split(",")
    return int(left), int(top), int(right), int(bottom)


def intersects(a, b):
    ax1, ay1, ax2, ay2 = a
    bx1, by1, bx2, by2 = b
    return ax1 < bx2 and bx1 < ax2 and ay1 < by2 and by1 < ay2


def texts(root):
    return [a.get("text", "") for a in nodes(root) if a.get("text")]


results = []


def check(cid, desc, ok, detail):
    results.append((cid, desc, ok, detail))
    print(f"{'PASS' if ok else 'FAIL'} {cid} {desc} :: {detail}")


# D-01 / D-02: quick entries must sit above the bottom navigation bar.
my = load("ui_my_after_install.xml")
nav = by_id(my, "com.jinyuni.dengbei_care:id/navigation_home")
nav_bounds = bounds(nav) if nav else (0, NAV_TOP, 240, 1424)
for rid, cid, name in (
    ("com.jinyuni.dengbei_care:id/viewReportButton", "D-01", "查看今日报告"),
    ("com.jinyuni.dengbei_care:id/opensensorlistbutton", "D-02", "打开传感器列表"),
):
    n = by_id(my, rid)
    if n is None:
        check(cid, f"{name} 控件存在且不被底部导航覆盖", False, "控件未在 UI 树中唯一出现")
        continue
    bb = bounds(n)
    overlap = intersects(bb, nav_bounds)
    check(cid, f"{name} 控件存在且不被底部导航覆盖",
          bb[3] <= nav_bounds[1] and not overlap,
          f"bounds={bb} nav={nav_bounds} overlap={overlap}")

# D-01: real click led to ReportActivity with report content.
rep = load("ui_report_activity.xml")
t = texts(rep)
has_report = any("早起报告" in x for x in t) and any(
    "重新生成" in x for x in t)
check("D-01", "点击后打开报告页（非切换页签）", has_report,
      f"report texts={[x for x in t if '报告' in x or '重新生成' in x][:4]}")

# D-02: real click led to SensorIdListFragment.
lst = load("ui_sensor_id_list_after_fix.xml")
t = texts(lst)
has_list = any("已保存的 ID（0）" in x for x in t) and any(
    "删除选中" in x for x in t)
check("D-02", "点击后进入传感器列表页（显示已保存的 ID（0））", has_list,
      f"list texts={[x for x in t if 'ID' in x or '删除' in x or '点选' in x][:4]}")

# D-03: register-sensor and gateway-settings controls do not overlap and the
# register click does not open the gateway dialog.
scrolled = load("ui_my_scrolled.xml")
reg = by_id(scrolled, "com.jinyuni.dengbei_care:id/addsensorid")
gw = by_id(scrolled, "com.jinyuni.dengbei_care:id/gatewayBleButton")
if reg and gw:
    rb, gb = bounds(reg), bounds(gw)
    check("D-03", "注册传感器与网关与上传设置 bounds 不相交",
          not intersects(rb, gb), f"register={rb} gateway={gb}")
    check("D-03", "注册传感器点击中心不落在底部导航",
          rb[1] < NAV_TOP and (rb[1] + rb[3]) // 2 < NAV_TOP,
          f"register center=({(rb[0] + rb[2]) // 2},{(rb[1] + rb[3]) // 2}) nav_top={NAV_TOP}")
else:
    check("D-03", "注册传感器与网关与上传设置 bounds 不相交", False,
          "控件未在滚动后 UI 树中唯一出现")

after = load("ui_after_addsensor_click.xml")
has_dialog = any(a.get("resource-id") == "android:id/alertTitle" and
                 a.get("text") == "网关与上传设置" for a in nodes(after))
check("D-03", "空输入点击注册传感器不打开网关与上传设置对话框", not has_dialog,
      f"alertTitle 网关与上传设置 present={has_dialog}")

gw_dump = load("ui_gateway_dialog.xml")
has_gw_dialog = any(a.get("resource-id") == "android:id/alertTitle" and
                    a.get("text") == "网关与上传设置" for a in nodes(gw_dump))
check("REGRESSION", "网关与上传设置按钮仍能打开对话框", has_gw_dialog,
      f"dialog present={has_gw_dialog}")

# D-04a: home empty-state copy points at the real entry; 去添加 opens the list.
home = load("ui_home_after_install.xml")
t = texts(home)
new_copy = any("点击下方「去添加」添加设备" in x and "打开传感器列表" in x for x in t)
old_copy = any("前往「我的」→「传感器列表」" in x for x in t)
check("D-04a", "首页空态文案指向实际可达入口", new_copy and not old_copy,
      f"new_copy={new_copy} old_copy={old_copy}")
via_add = load("ui_home_add_click.xml")
t = texts(via_add)
check("D-04a", "首页「去添加」进入传感器列表页",
      any("已保存的 ID（0）" in x for x in t),
      f"texts={[x for x in t if 'ID' in x or '删除' in x][:3]}")

# Regression: guard start/stop and cold start.
started = load("ui_guard_started.xml")
stopped = load("ui_guard_stopped.xml")


def guard_text(root):
    n = by_id(root, "com.jinyuni.dengbei_care:id/startGuardButton")
    return n.get("text") if n else None


check("REGRESSION", "守护可启动（按钮→停止）", guard_text(started) == "停止",
      f"text={guard_text(started)}")
check("REGRESSION", "守护可停止（按钮→启动）", guard_text(stopped) == "启动",
      f"text={guard_text(stopped)}")
cold = load("ui_after_cold_start_2.xml")
t = texts(cold)
check("REGRESSION", "冷启动后首页正常显示", any("还没有添加设备" in x for x in t),
      f"home texts={[x for x in t if '爱守护' in x or '添加' in x][:3]}")

failed = [r for r in results if not r[2]]
print(f"\nRESULT: {'PASS' if not failed else 'FAIL'} "
      f"({len(results) - len(failed)}/{len(results)})")
sys.exit(0 if not failed else 1)
