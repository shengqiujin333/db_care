#!/usr/bin/env python
"""Append/update this round's file_manifest lines (path | status | purpose).

Reads the existing manifest, replaces any line whose path already exists, and
appends new lines in sorted order. Idempotent.
"""
import io
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
MANIFEST = os.path.join(ROOT, "file_manifest.txt")
BASE = "evidence/e2e_p0/devops_reliability_upload_contract_20261126/"

ENTRIES = [
    (BASE + "RELIABILITY_REPORT.md", "上传受理修复后可靠性观测与监控方案（MonitoringArtifact）：变更后 7.5 小时服务/证书/上传受理/客户端四层指标、症状与影响分离、M1-M14 监控项、回退条件与影响范围、跨组件交接"),
    (BASE + "HEALTH_EVIDENCE.md", "上传受理修复后健康证据（HealthEvidence）：H1-H16 逐项判定（候选版本、服务稳定、TLS、401 归零、上传未达成、写库未执行、回退点、采集停滞、监控 GAP、公网暴露）"),
    (BASE + "19_r4_platform_window.txt", "平台只读窗口：单元状态/NRestarts/启动时间、资源、EMQX 状态、listener 计数与 128 秒增量、证书身份与摘要、平台内 10 次握手、AUTHN 原文、上传服务文件身份、上传状态与逐次请求、端点分布、错误扫描、端口与 HTTP 语义、两处回退点、可观测性现状"),
    (BASE + "20_r4_phone_window.txt", "手机只读窗口：设备/APK、进程时长、网络、prefs 上传与身份状态、崩溃缓冲、应用日志尾部（BLE 周期/CONNACK）、分类计数"),
    (BASE + "21_r4_external_tls.txt", "外网 8883 12 次握手（App 信任锚 + IP SAN 校验）、时延、叶子摘要、错误主机名负例"),
    (BASE + "22_r4_endpoint_and_ops_gaps.txt", "上传来源/状态聚合、公网扫描原文、Restart 策略与单元文件（凭据已脱敏）、EMQX 告警节流原文、日志轮转、定时任务、磁盘、备份保留"),
    (BASE + "23_r4_upload_caller_matrix.txt", "POST /upload_data 逐次来源+状态、首末请求时间、状态汇总（19 次全 400、2xx=0、5xx=0）"),
    (BASE + "24_r4_local_db_snapshot.db", "手机本地库只读快照（run-as cat 复制，未写回设备）"),
    (BASE + "24_r4_local_db_snapshot-journal", "手机本地库回滚日志只读副本（与快照同次采集，用于一致性核对）"),
    (BASE + "25_r4_local_pending_rows.txt", "待发箱 pending_uploads 逐行内容（设备、传感器时间、温湿度、重试次数）"),
    (BASE + "26_r4_phone_collection_gap.txt", "本地库快照分析：温度表按小时计数、待发行明细、告警表最新写入、落库停滞机器判据"),
    (BASE + "r4_platform_probe.py", "第四轮平台只读探针（限时、脱敏、不重启、不部署、不写接口、不连 DB）"),
    (BASE + "r4_external_tls.py", "外网 8883 TLS 抽样只读脚本（App 信任锚 + IP 主机名校验 + 负例，不做 MQTT CONNECT）"),
    (BASE + "r4_phone_probe.py", "手机日志/状态只读探针（不交互、不安装、不清数据）"),
    (BASE + "r4_endpoint_and_ops_gaps.py", "上传来源与状态、重启策略、扫描噪声、日志保留只读探针（凭据脱敏）"),
    (BASE + "r4_upload_caller_matrix.py", "上传请求来源/状态矩阵只读统计脚本"),
    (BASE + "r4_local_snapshot_analysis.py", "本地库快照只读分析脚本（温度表分布、待发行、告警表、落库停滞判据）"),
    (BASE + "maintain_manifest_r4.py", "第四轮 file_manifest 维护脚本（按 path 替换/追加本轮条目）"),
    (BASE + "manifest_readback_r4.py", "第四轮清单回读脚本（核对条目对应实际文件、列出未登记文件）"),
    ("artifacts/monitoring_reliability_upload_contract.yaml", "MonitoringArtifact 指针：上传受理修复后可靠性观测与监控方案"),
    ("artifacts/health_evidence_reliability_upload_contract.yaml", "HealthEvidence 指针：上传受理修复后健康证据逐项判定"),
]


def main():
    with io.open(MANIFEST, encoding="utf-8") as fh:
        lines = fh.read().splitlines()
    index = {}
    for i, line in enumerate(lines):
        if " | " in line and not line.startswith("#"):
            index[line.split(" | ", 1)[0]] = i
    added = updated = 0
    for path, purpose in ENTRIES:
        entry = "%s | active | %s" % (path, purpose)
        if path in index:
            lines[index[path]] = entry
            updated += 1
        else:
            lines.append(entry)
            added += 1
    with io.open(MANIFEST, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines).rstrip() + "\n")
    print("added=%d updated=%d total=%d" % (added, updated, len(lines)))


if __name__ == "__main__":
    raise SystemExit(main())
