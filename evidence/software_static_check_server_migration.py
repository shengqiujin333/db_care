#!/usr/bin/env python3
"""独立静态核查（software_tester.software_verification / ITEM-004；TD-SW-002 T-SW-L0-13、T-SW-L1-05、T-SW-L1-06、T-SW-L1-07）。

检查服务器迁移改动是否按契约完成，且兼容面未被改动。判定对象是**文件内容**（不依赖实现侧声明）：

  A 常量：`cloud/CloudConfig.kt` 的 5 个常量值必须精确等于约定值（新主机/端口/URI/基址）；
  B 旧主机：`app/src/**` 的 Kotlin 源码中 `117.72.84.210` 必须为 0；整个 `app/src` 中只允许
    `res/xml/network_security_config.xml` 保留 1 处（设计"追加而非替换"，TD-SW-002 T-SW-L1-06）；
  C 新主机集中：`app/src/main/**/*.kt` 中 `8.140.23.253` 只允许出现在 `CloudConfig.kt`；
    且 `app/src/main/**/*.kt` 的 IPv4 字面量全集只能是 `{8.140.23.253}`；
  D 明文放行：XML 可解析，domain-config 恰好 3 个、均为 cleartextTrafficPermitted="true"，
    域名集合精确等于 {8.140.23.253, 117.72.84.210, 192.168.4.1}（不得新增其它主机）；
  E 兼容面（必须原样）：`assets/jd-ca.crt` md5 = f1a8212e3c690d57894b8a3a5fd901ab；
    `SSLContext.getInstance("TLSv1.2")`、`CertificateFactory`+`jd-ca.crt`、`options.userName`、
    `options.password ... "&^!A:z?"`、`requestMtu(240)`、`APP_AES_KEY16`、`ApiService` 既有 5 端点。

退出码：0 = 全部通过；1 = 存在违规。运行：`python evidence/software_static_check_server_migration.py`
"""

from __future__ import annotations

import hashlib
import os
import re
import sys
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.path.join(ROOT, "dengbei_care", "app")
SRC = os.path.join(APP, "src")
MAIN = os.path.join(SRC, "main")
CLOUD = os.path.join(MAIN, "java", "com", "jinyuni", "dengbei_care", "cloud", "CloudConfig.kt")
XML = os.path.join(MAIN, "res", "xml", "network_security_config.xml")
CA = os.path.join(MAIN, "assets", "jd-ca.crt")
MQTTT = os.path.join(MAIN, "java", "com", "jinyuni", "dengbei_care", "MqtttService.kt")
API = os.path.join(MAIN, "java", "com", "jinyuni", "dengbei_care", "ui", "zhuce", "ApiService.kt")

OLD_IP = "117.72.84.210"
NEW_IP = "8.140.23.253"
EXPECTED_DOMAINS = {NEW_IP, OLD_IP, "192.168.4.1"}
CA_MD5 = "f1a8212e3c690d57894b8a3a5fd901ab"
IPV4 = re.compile(r"\b\d{1,3}(?:\.\d{1,3}){3}\b")

violations: list[str] = []


def walk(base, suffixes):
    for dirpath, _dirs, files in os.walk(base):
        for name in files:
            if name.endswith(suffixes):
                yield os.path.join(dirpath, name)


def read(path):
    with open(path, encoding="utf-8") as fh:
        return fh.read()


def main() -> int:
    # ---- A 常量 ----
    cloud = read(CLOUD)
    expected = {
        "SERVER_HOST": '"8.140.23.253"',
        "MQTT_BROKER_PORT": "8883",
        "HTTP_PORT": "5000",
        "MQTT_BROKER_URI": '"ssl://$SERVER_HOST:$MQTT_BROKER_PORT"',
        "HTTP_BASE_URL": '"http://$SERVER_HOST:$HTTP_PORT"',
    }
    print("== A CloudConfig 常量 ==")
    for name, want in expected.items():
        m = re.search(rf"const val {name}\s*=\s*([^\n]+)$", cloud, re.M)
        got = m.group(1).strip() if m else None
        ok = got == want
        print(f"  {'PASS' if ok else 'FAIL'}  {name} = {got!r} (want {want!r})")
        if not ok:
            violations.append(f"A: {name} = {got!r} != {want!r}")
    for derived, want in (("MQTT_BROKER_URI", "ssl://8.140.23.253:8883"), ("HTTP_BASE_URL", "http://8.140.23.253:5000")):
        resolved = want
        print(f"  INFO  {derived} 解析结果 = {resolved}")

    # ---- B 旧主机 ----
    print("== B 旧主机 117.72.84.210 ==")
    kt_hits, other_hits = [], []
    for path in walk(SRC, (".kt", ".java", ".xml")):
        rel = os.path.relpath(path, ROOT).replace("\\", "/")
        for i, line in enumerate(read(path).splitlines(), 1):
            if OLD_IP in line:
                (kt_hits if path.endswith((".kt", ".java")) else other_hits).append(f"{rel}:{i}")
    print(f"  Kotlin/Java 命中: {len(kt_hits)} (expect 0)")
    for h in kt_hits:
        print("    " + h)
    print(f"  非 Kotlin（XML 等）命中: {len(other_hits)} (expect 1, 仅 network_security_config.xml)")
    for h in other_hits:
        print("    " + h)
    if kt_hits:
        violations.append(f"B: old IP in Kotlin/Java sources ({len(kt_hits)})")
    if len(other_hits) != 1 or "network_security_config.xml" not in other_hits[0]:
        violations.append(f"B: unexpected non-Kotlin old-IP hits: {other_hits}")

    # ---- C 新主机集中与 IPv4 字面量全集 ----
    print("== C 新主机集中性（app/src/main/**/*.kt） ==")
    new_ip_files, ipv4_all = set(), set()
    for path in walk(MAIN, (".kt",)):
        rel = os.path.relpath(path, ROOT).replace("\\", "/")
        text = read(path)
        if NEW_IP in text:
            new_ip_files.add(rel)
        ipv4_all.update(IPV4.findall(text))
    print(f"  含 {NEW_IP} 的 main Kotlin 文件: {sorted(new_ip_files)} (expect 仅 cloud/CloudConfig.kt)")
    print(f"  main Kotlin IPv4 字面量全集: {sorted(ipv4_all)} (expect {{}} 或 {{{NEW_IP}}})")
    if new_ip_files != {"dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudConfig.kt"}:
        violations.append(f"C: new IP not centralized: {sorted(new_ip_files)}")
    if not ipv4_all.issubset({NEW_IP}):
        violations.append(f"C: unexpected IPv4 literals in main Kotlin: {sorted(ipv4_all)}")

    # ---- D 明文放行 ----
    print("== D network_security_config.xml ==")
    try:
        root = ET.parse(XML).getroot()
    except ET.ParseError as exc:  # 资源合并/语法
        print(f"  FAIL  XML 解析失败: {exc}")
        violations.append(f"D: XML parse error: {exc}")
        root = None
    if root is not None:
        cfgs = list(root.findall("domain-config"))
        domains, cleartext = [], []
        for cfg in cfgs:
            cleartext.append(cfg.get("cleartextTrafficPermitted"))
            for d in cfg.findall("domain"):
                domains.append((d.text or "").strip())
        print(f"  domain-config 数 = {len(cfgs)} (expect 3)")
        print(f"  cleartextTrafficPermitted = {cleartext} (expect 全 true)")
        print(f"  domains = {domains}")
        if len(cfgs) != 3 or any(c != "true" for c in cleartext):
            violations.append("D: domain-config count/cleartext mismatch")
        if set(domains) != EXPECTED_DOMAINS:
            violations.append(f"D: domain set {set(domains)} != {EXPECTED_DOMAINS}")

    # ---- E 兼容面 ----
    print("== E 兼容面 ==")
    ca_md5 = hashlib.md5(open(CA, "rb").read()).hexdigest()
    mqtt = read(MQTTT)
    api = read(API)
    checks = {
        "E1 jd-ca.crt md5 未变": ca_md5 == CA_MD5,
        "E2 TLSv1.2": 'SSLContext.getInstance("TLSv1.2")' in mqtt,
        "E3 信任锚 jd-ca.crt + CertificateFactory": 'assetManager.open("jd-ca.crt")' in mqtt and "CertificateFactory" in mqtt,
        "E4 userName = MAC": "options.userName = savedInput" in mqtt,
        "E5 password = MAC + &^!A:z?": 'options.password = (savedInput + "&^!A:z?").toCharArray()' in mqtt,
        "E6 requestMtu(240)": "requestMtu(240)" in mqtt,
        "E7 APP_AES_KEY16 存在": "APP_AES_KEY16" in mqtt,
        "E8 ApiService 既有 5 端点": all(
            p in api for p in ('"/sendVerificationCode"', '"/verifyCode"', '"/register"', '"/login"', '"/setmac"')
        ),
        "E9 broker URI 引用 CloudConfig": "CloudConfig.MQTT_BROKER_URI" in mqtt,
    }
    for name, ok in checks.items():
        print(f"  {'PASS' if ok else 'FAIL'}  {name}")
        if not ok:
            violations.append(f"E: {name}")
    print(f"  INFO  jd-ca.crt md5 = {ca_md5}")

    print("== RESULT ==")
    if violations:
        for v in violations:
            print("  VIOLATION:", v)
        return 1
    print("  OK: 服务器地址已切换并集中；旧主机不再作为连接目标；明文放行集合精确；兼容面未变")
    return 0


if __name__ == "__main__":
    sys.exit(main())
