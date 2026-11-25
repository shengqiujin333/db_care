#!/usr/bin/env python
"""Maintain file_manifest.txt for the third reliability round (path | status | purpose)."""
import io
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
MANIFEST = os.path.join(ROOT, "file_manifest.txt")
BASE = "evidence/e2e_p0/devops_reliability_8883_20261125/"

NEW = [
    ("11_resume3_platform_window.txt",
     "第三轮平台只读窗口：EMQX 单元/uptime/NRestarts、listener 计数器与 128 秒配对增量、证书 serial/SAN/文件摘要、平台内 10 次握手、AUTHN 原文、上传服务进程与文件变更、端口与 HTTP 基线、证书回退点"),
    ("12_resume3_phone_window.txt",
     "第三轮手机只读窗口：设备/APK 版本、前台进程、MyPrefs 上传状态、应用日志分类（TLS 错误、CONNACK、401 提及、崩溃计数）"),
    ("13_resume3_external_tls.txt",
     "外网 8883 TLS 12 次握手抽样（App 信任锚 + IP SAN 主机名校验）、时延与叶子证书摘要"),
    ("14_resume3_app_crash.txt",
     "22:00:24 App FATAL EXCEPTION 完整栈与系统 Force finishing 记录原文（只读 logcat）"),
    ("15_resume3_upload_service_change.txt",
     "上传服务变更事实：abr.py 大小/摘要/mtime、变更前副本、user_script 单元时间线、启动日志、/root 近期变更"),
    ("16_resume3_deploy_context.txt",
     "实现角色 21:59:02 重部署目录清单与 deploy_v2.log 尾部（仅作可靠性窗口上下文）"),
    ("17_resume3_upload_gate_apply_log.txt",
     "上传受理门补丁 apply.log 全文：pre/post 校验、401→400 行为、备份摘要、自述待真实客户端复测"),
    ("18_resume3_upload_outcomes.txt",
     "16:00-22:45 POST /upload_data 逐次状态与来源统计（401=6、400=40、2xx=0）"),
    ("resume3_check.py", "第三轮平台窗口只读探针（限时、脱敏、不重启、不写入、不连 MQTT/DB）"),
    ("resume3_phone_probe.py", "第三轮手机日志/状态只读探针（不交互、不安装、不清数据）"),
    ("resume3_external_tls.py", "外网 8883 TLS 抽样只读脚本（App 信任锚 + IP 主机名校验，不做 MQTT CONNECT）"),
    ("resume3_change_check.py", "上传服务变更事实只读探针（文件身份/单元时间线/启动日志）"),
    ("resume3_deploy_context.py", "实现侧重部署上下文只读读取（脱敏，仅清单与日志尾部）"),
    ("resume3_apply_log.py", "上传受理门 apply.log 只读导出脚本（脱敏）"),
    ("resume3_upload_outcomes.py", "上传受理结果只读统计脚本（按状态码与来源聚合访问日志）"),
]

UPDATED = {
    "HEALTH_EVIDENCE.md":
        "8883 证书变更后健康证据：H1-H11 逐项判定（三轮），证书/TLS PASS、MQTT 会话未达成、上传 401 实测失败且修复后未证实、App 崩溃 1 次、回滚点 PASS、监控 GAP",
    "RELIABILITY_REPORT.md":
        "8883 证书变更后可靠性观测与监控方案（三轮，含 21:59:02 上传服务重部署）：TLS/认证/上传受理分段指标、日志关联、症状与影响分离、M1-M11 监控项、回退条件与影响范围",
}


def main():
    with io.open(MANIFEST, "r", encoding="utf-8") as fh:
        lines = fh.read().splitlines()
    present = {l.split("|")[0].strip(): i for i, l in enumerate(lines) if "|" in l}

    for name, purpose in UPDATED.items():
        path = BASE + name
        if path in present:
            lines[present[path]] = "%s | active | %s" % (path, purpose)

    anchor = present.get(BASE + "ssl_error_attribution.py")
    block = ["%s%s | active | %s" % (BASE, name, purpose) for name, purpose in NEW
             if BASE + name not in present]
    if block:
        at = (anchor + 1) if anchor is not None else len(lines)
        lines[at:at] = block

    with io.open(MANIFEST, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
    print("block_written=%d updated=%d" % (len(block), len(UPDATED)))
    for name, _ in NEW:
        print("check", BASE + name, os.path.exists(os.path.join(ROOT, BASE + name)))


if __name__ == "__main__":
    raise SystemExit(main())
