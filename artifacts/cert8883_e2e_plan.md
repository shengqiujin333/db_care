# 平台 8883 证书修复：跨系统端到端测试计划（E2E-CERT8883-001）

状态：软件测试设计产物（`software_tester.software_test_design`，输出端口 `e2e_plan`）。**只做计划，不执行**。
版本：1.0（与 `artifacts/software_test_design_cert8883.md` TD-CERT8883-001 配套；逐用例判据以 TD 为准，本文件负责跨系统旅程、刺激顺序与阻塞边界）。
范围：证书修复后的 **平台证书 → App TLS → MQTT CONNECT →（可行时）HTTP 上传/持久化 → 受影响联合用例 → 本轮交付限定整体验收** 的关键旅程；不重做传感器固件物理验证，不重做与本修复无关的历史验收。
上游：当前 Project Goal；`evidence/e2e_p0/P0_platform_stage_report.md`（原始 TEST_FAIL）；`evidence/e2e_p0/P0_platform_failure_triage.md`（REPAIR_REQUIRED）；`evidence/e2e_p0/P0_backend_cert_issuance_and_rollback.md`；`evidence/e2e_p0/P0_backend_cert_ops_runbook.md`；`artifacts/e2e_joint_test_design.md`（TD-E2E-SW-006）；`artifacts/e2e_cases.yaml`；`artifacts/interface_contract.md`（IC-002 v4.0）；`artifacts/android_build.md`（AB-003）；`docs/05_平台服务器配置与联调前提.md`。

执行原则：

- 每条旅程至少给出 **两类独立证据**（例如：主机证书原始输出 + App 侧日志/UI；或 App 上传日志 + 平台回读行），并记录同一 UTC 时间窗与关联键。
- 真实环境优先：真实 App、真实手机 `12f046690506`、既有平台 `8.140.23.253`、既有网关；不得用桩、模拟器或旧证据冒充当前组合。
- 先准备接收端/观测，再触发：先记录平台服务/证书/端口基线与 App 日志窗口，再让 App 重连；上传旅程先确认平台回读入口与身份，再触发 App 上传。
- 仅使用已授权入口。执行能力当前没有服务器变更、SSH 只读、串口/烧录、DB 直连授权；这些由对应能力或用户提供，缺项如实记录。
- 不具备前置时该旅程记 **NOT EXECUTED** 并写明缺失条件、责任方与恢复后必须重做的步骤；不得记通过。
- 禁止以 `--insecure`、关闭主机名校验、HTTP 2xx、占位/缓存数据、旧 serial/旧 APK 结果替代真实证据。

---

## 1. 环境与版本基线（执行轮重新核对）

| 项 | 设计时值 | 来源 |
|---|---|---|
| 平台 | `8.140.23.253`；EMQX 5.8.0；Flask 5000（Werkzeug/3.1.8 Python/3.12.3）；MySQL 9.0.1 | `P0_platform_stage_report.md`、`platform_stage_retest_*` |
| 8883 证书基线（缺陷） | `CN=117.72.84.210`；issuer `Tiktok Root CA`；serial `77ADCFD924BA0F87F845C81934D4A3F346D4BB82`；SAN 仅 `IP:117.72.84.210`；至 2026-12-21 | `backend_cert_recheck_20261124_1015Z.txt` |
| App 信任锚 | `assets/jd-ca.crt`；SHA-256 `75:33:09:B3:…:15:2B`；至 2034-10-10 | 本地 `openssl x509` |
| App | `com.jinyuni.dengbei_care` vC 10 / vN 1.9；SHA-256 `80232c50…de4b`；设备 Redmi 8A / API 29 / `12f046690506` | `P0_phone_stage_report.md`、`android_build.md` |
| 手机网络 | `ZTE_607157` 默认网络（WIFI） | `P0_phone_stage_report.md` §3.3 |
| 网关 | BLE `84:C2:E4:03:02:02`；`GwID=B1:17:9C:7A:4E:E0`；`dev=1`、`valueSize=160`、`mtu=240`；读取终态 `ALL_PLACEHOLDER` | 手机段 |
| 传感器 | FW FD-002r8；`uid=6A002C00` | EV-041、`firmware_release.md` |
| 测试身份 | `registerphone` 空、`mac_addr` 空/`000000000000` | 手机段 |
| 主机探测工具 | `openssl 3.x`、`curl`、`node`、`adb`、`android_test`、Playwright/Edge | 统一环境 |

---

## 2. 关键旅程

### E2E-CERT-01 平台证书修复验收（服务器侧结果独立核验）

**目标**：独立证明线上 8883 叶子证书已由 App 当前信任锚签发、SAN 命中当前地址、serial 已从缺陷基线变更、判据未被放宽；并与部署/签发记录一致。

**前置**：部署能力已完成证书替换与 listener 重载并回传执行记录；`jd-ca.crt` 未被替换；主机可只读访问 8883。

**步骤**
1. 记录 UTC 时间、主机、`openssl version`、`jd-ca.crt` SHA-256。
2. 运行 `scripts/emqx_8883_cert_verify.sh 8.140.23.253 8883 dengbei_care/app/src/main/assets/jd-ca.crt --expect-new`。
3. 取回叶子证书 `subject/issuer/serial/dates/fingerprint/SAN` 原文。
4. 与部署记录的新 serial/SAN/指纹逐项对照；核对 `notAfter` 与 CA 有效期。
5. 记录负例 `-verify_ip 8.140.23.254` 不通过。

**证据**：脚本完整输出（含 `RESULT` 行）、`openssl s_client/x509/verify` 原文、`jd-ca.crt` 指纹、部署执行记录（引用，不替代）。
**判定**：`tls_handshake=OK`、`chain_verify=OK`、`san_contains_target=YES`、`ip_san_match=OK`、`negative_control=OK`、`serial_changed=YES`、issuer=`Tiktok Root CA`、信任锚指纹不变 → PASS；仍为旧 SAN/旧 serial → FAIL（修复未生效）。
**阻塞**：部署未完成 → NOT EXECUTED（不得判 PASS）；无公开 8883 网络 → NOT EXECUTED 并记录实际错误。

### E2E-CERT-02 App 真实消费端：TLS 与 MQTT CONNECT/CONNACK

**目标**：证明 App 对 `ssl://8.140.23.253:8883` 不再出现证书主机名错误，完成 TLS 并进入 MQTT CONNECT；如实记录 CONNACK/认证结果。

**前置**：E2E-CERT-01 通过；手机在线；APK/设备摘要与候选一致；`ZTE_607157` 为默认网络；App 守护运行；`mac_addr` 实际值已记录（可能是空/占位）。

**步骤**
1. 记录 App/设备/网络身份、`mac_addr`/`registerphone`（脱敏）、logcat 窗口起点。
2. 启动/重启 App 或等待重连周期；不修改代码、不关校验。
3. 采集 `MqttService`/`MqtttService` 日志：TLS 异常、连接成功/失败、CONNACK/原因。
4. 记录 App UI broker 状态；同一窗口记录网关 BLE 读取是否正常。
5. 与 E2E-CERT-01 的 UTC 窗口对照。

**证据**：logcat 切片（原始时间戳/栈）、UI dump/截图（脱敏）、设备/APK 摘要、默认网络原文、主机证书输出。
**判定**：无 `No subjectAltNames`/链/信任锚错误；TLS 完成并进入 CONNECT；CONNACK 实际结果分类记录。认证失败属另一层，TLS 维度 PASS；仍 SAN/链失败 → FAIL。
**阻塞**：手机/ADB/App 不可用 → NOT EXECUTED；证书未修复 → FAIL（不是未执行）。

### E2E-CERT-03 不受影响路径回归

**目标**：证书替换未改变其他 listener、HTTP 路由、MySQL 端口语义与 App 非 MQTT 路径，未新增诊断/免校验入口。

**前置**：E2E-CERT-01 已执行；只读访问。

**步骤**
1. 主机/手机只读探测 1883、8083、8084、18083 `GET /status`、5000 `GET /`、`/login`、`/upload_data`、3306 握手。
2. 不 POST、不登录、不连库、不调管理 API。
3. App 启动/守护/一次网关读取，记录链路终态。
4. 与 `服务器迁移记录.md`/`platform_stage_retest_*` 基线对照，列差异。

**证据**：端口/HTTP/握手原始输出、App logcat/UI、差异表。
**判定**：8883 仍 TLS；18083 200 `emqx is running`；5000 路由行为与基线一致；3306 握手可辨；无新增诊断/免校验入口；App 无新增崩溃/ANR。8083/8084 既有超时如实记录，不归因本次变更；可归因于本次变更的回归才 FAIL。
**阻塞**：网络路径不可用 → 对应子项 NOT EXECUTED，不伪造。

### E2E-CERT-04 安卓到平台上传：受理 + 持久化回读（条件式）

**目标**：证明 App 真实路径将一条可关联读数 `POST /upload_data` 到平台，HTTP 受理且 MySQL `sensor_data` 可回读、字段一致、同 `(dev_id,time)` 重试不增行。

**前置**：隔离测试身份（`registerphone` 非空，经 App 既有注册路径）；已确认的平台网关 `mac_addr`；一条合法读数（真实 BLE `DATA_OK` 或经授权的受控注入）；平台写/回读授权；守护运行；`ZTE_607157` 默认网络。

**步骤**
1. 记录待发箱/本地库行数与上传状态区基线。
2. 形成一条读数（优先真实 BLE 有效批次；记录来源与版本）。
3. 观察 `CloudUpload` 日志、HTTP 状态/响应、状态区。
4. 按授权入口回读 `sensor_data (phone, dev_id, time, temperature, humidity)`。
5. 若产品路径允许，重复同一 `(dev_id,time)` 并核对不增行。
6. 记录测试行清理或保留披露。

**证据**：App 日志/UI、HTTP 请求/响应（脱敏）、平台回读行、行数前后、身份设置路径（脱敏）。
**判定**：2xx **且** 行存在且字段数值一致且幂等 → PASS；2xx 无行 → 持久化未成立（FAIL/交平台）；门控关闭/无身份/无数据/无回读授权 → NOT EXECUTED（写明缺项）。
**阻塞**：缺任一前置 → NOT EXECUTED；不得手工写库/待发箱或伪造身份绕过。

### E2E-CERT-05 受影响的既有联合用例 P0/P1（条件式）

**目标**：在修复后的当前组合版本上重做 `E2E-JOINT-P0` 平台段，并在条件具备时重做 `E2E-JOINT-P1`；证明证书修复解除了 App→EMQX 的 TLS 阻断，且不把 P0/P1 其他缺口写成通过。

**前置**：
- P0：E2E-CERT-01/02/03；同版本手机段身份复核（APK/设备/网络/守护/网关绑定/读取就绪）。
- P1：P0；固件测试构建注入 `367`（默认关闭、正式包排除、P1 后恢复标准构建）；隔离测试身份与已确认 `mac_addr`；平台回读授权；干净基线。

**步骤**：按 `artifacts/e2e_joint_test_design.md` §4.1/§4.3 执行；每段先准备接收/采集，再触发；记录同一 `uid4+367+h` 与时间窗；证书维度引用 E2E-CERT-01/02。

**证据**：P0 三端身份/平台基线/C1/C2；P1 的 COM43 原始 `TINJ/S/P/BOOT`（由嵌入式能力执行）、App BLE/上传日志、MQTT 载荷（按产品分支）、平台回读行、测试构建排除/恢复证据。
**判定**：P0 平台段按 C1/C2；P1 四段全成立才 PASS。P1 的测试构建/身份/回读缺口不因证书修复消失；缺任一段 → NOT EXECUTED/FAIL，不部分通过。
**阻塞**：无串口/烧录/测试构建/身份/回读 → P1 NOT EXECUTED；不得用 C1/C2 冒充 P1。

### E2E-CERT-06 本轮交付限定整体验收

**目标**：在当前组合版本上形成本轮交付的最终判定，明确“证书修复维度”与“完整产品”的边界。

**前置**：E2E-CERT-01…05 已执行或已给出 NOT EXECUTED 判定；证据索引齐全。

**步骤**
1. 汇总身份（证书/信任锚/APK/设备/传感器/网关/平台/时间窗）。
2. 汇总各旅程 PASS/FAIL/NOT EXECUTED 与证据。
3. 核对未执行项与清理/披露。
4. 按 TD-CERT8883-001 §3.7/§7 形成结论。

**证据**：证据索引、判定表、未执行清单、清理记录。
**判定**：
- C1/C2/C3/C4 全部实际通过，且 P0 通过、P1 通过或由用户明确批准本阶段不纳入 P1 → 本轮交付 TEST_PASS（仍非完整产品通过）；
- C1/C2/C3 通过但 C4 未执行，或 P1 未执行且无用户明确批准本阶段不纳入 → 证书修复维度通过、交付整体 TEST_FAIL（`EXTERNAL_CONDITION_UNEXECUTED`），不得写整体通过；
- 任一实测缺陷 → TEST_FAIL 并交责任方。
**阻塞**：证据不足 → 不得判 PASS。

---

## 3. 旅程顺序与依赖

| 顺序 | 旅程 | 依赖 | 结果单位 |
|---:|---|---|---|
| 1 | E2E-CERT-01 证书验收 | 部署完成 | PASS/FAIL/NOT EXECUTED |
| 2 | E2E-CERT-02 App TLS/CONNACK | 01；手机/App/网络 | PASS/FAIL/NOT EXECUTED |
| 3 | E2E-CERT-03 回归 | 01；只读 | PASS/FAIL/NOT EXECUTED |
| 4 | E2E-CERT-04 上传 | 身份+数据+回读授权 | PASS/FAIL/NOT EXECUTED |
| 5 | E2E-CERT-05 P0/P1 | 01/02/03；P1 另需测试构建/身份/回读 | PASS/FAIL/NOT EXECUTED |
| 6 | E2E-CERT-06 整体验收 | 上述实际状态 | 本轮交付 PASS/FAIL |

**关联键**：证书 serial+SAN+指纹；`jd-ca.crt` 指纹；APK SHA-256+versionCode；设备 serial/API；`uid4`；`(dev_id,time)`；UTC 时间窗。**不得靠时间接近猜同一事件。**

---

## 4. 证据与脱敏

- 原始输出优先，保留命令、时间、主机、工具版本、退出码；日志切片保留时间戳与 PID。
- 至少两类独立证据：证书旅程=主机证书输出+部署记录/App 日志；上传旅程=App 上传日志+平台回读行。
- Wi-Fi 密码只经 `ui_set_text` + `text_env` 注入；MQTT 固定后缀、SSH/DB/短信/Dashboard 凭据、私钥/CA 私钥不进入证据。
- 手机号掩码；服务日志只保留与本用例相关的必要行；测试身份/测试行/绑定的保留或清理必须披露。

---

## 5. NOT EXECUTED 与阻塞（如实披露）

| 项 | 缺失条件 | 责任/恢复后动作 |
|---|---|---|
| 证书替换未执行 | 服务器变更入口/CA 私钥 | 部署/基础设施；不可用则转信任锚变更分支交用户决定 |
| 上传（E2E-CERT-04） | 测试身份、`mac_addr`、合法读数、平台写/回读授权 | 用户/手机段/平台段；条件具备后重做 |
| P1（E2E-CERT-05） | 固件测试构建+烧录、身份、回读、干净基线 | 固件/嵌入式+手机/平台；条件具备后按 TD-E2E-SW-006 §4.3 重做 |
| 服务日志/连接数/DB 回读 | 只读 SSH/DB 授权 | 平台段执行能力；无入口则保持 NOT EXECUTED |
| 真实物理 >35℃/升温+无光/光强/功耗/433 电气 | 用户条件/仪器 | 不安排、不估算、不冒充 |
| release/生产签名、iOS、API 31+、9 台真实快照 | 未产出/无设备/用户范围 | 不在本轮；阶段通过不代表产品通过 |

---

## 6. 停止条件

本计划在写出本文件、`artifacts/software_test_design_cert8883.md`、`artifacts/cert8883_cases.yaml` 并同步 `file_manifest.txt` 后结束，返回 `DESIGN_READY`。执行、部署、设备操作、写库/回读、身份准备均不属本能力。
