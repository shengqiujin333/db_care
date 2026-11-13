import os

REPO = r"D:/mypro/beiwo2/AIP"
EVID = os.path.join(REPO, "evidence", "aa006_software_test_20261113")
MANIFEST = os.path.join(REPO, "file_manifest.txt")
REL_DIR = "evidence/aa006_software_test_20261113"


def purpose(name):
    if name == "ST-AA006.md":
        return "AA-006 软件独立验证报告：身份/宿主 300 项/静态 54+12+15/真机 N-1/N-2/N-3 逐用例证据/未执行披露/观察项/结论"
    if name == "test_summary.txt":
        return "AA-006 用例判定汇总（PASS/NOT EXECUTED 与观察项）"
    if name == "identity.txt":
        return "本轮提交、构建命令、APK 大小/SHA-256/MD5 与宿主用例计数"
    if name.startswith("TEST-com.jinyuni"):
        return "本轮新增独立验证用例 JUnit XML 副本"
    if name.startswith("logcat"):
        return "真机原始 logcat 切片（扫描权限/读取终态/报告/守护周期/清理）"
    if name.startswith("ui_") and name.endswith(".xml"):
        return "真机 UI 原始树"
    if name.startswith("ui_") and name.endswith(".txt"):
        return "UI 树文本/bounds 提取"
    if name.startswith("screen_"):
        return "真机截图（Toast/状态/页面）"
    if name.startswith("device_package"):
        return "设备包状态与运行时权限快照"
    if name.startswith("device_"):
        return "设备身份/状态或已安装 base.apk 摘要"
    if name.startswith("myprefs"):
        return "应用 MyPrefs 快照（网关/守护/上传状态）"
    if name.startswith("services_"):
        return "前台服务状态快照"
    if name.endswith(".log"):
        return "本轮命令原始输出（构建/单测/静态/门禁）"
    if name.endswith(".py"):
        return "本轮可复跑测试脚本"
    if name == "unit_test_summary.txt":
        return "JUnit XML 汇总（套件/用例/失败/跳过）"
    if name == "activity_state.txt":
        return "Activity/休眠状态只读快照"
    return "AA-006 软件验证原始证据"


def main():
    names = sorted(os.listdir(EVID))
    with open(MANIFEST, "r", encoding="utf-8") as fh:
        existing = fh.read()
    lines = []
    dir_entry = REL_DIR + "/"
    if dir_entry not in existing:
        lines.append(dir_entry + " | active | AA-006 N-1/N-2/N-3 软件独立验证原始证据目录（真机 UI 树/日志/截图/权限状态/构建与单测）")
    for name in names:
        rel = REL_DIR + "/" + name
        if rel in existing:
            continue
        lines.append(rel + " | active | " + purpose(name))
    if not lines:
        print("no new entries")
        return
    with open(MANIFEST, "a", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    print("appended %d entries" % len(lines))


main()
