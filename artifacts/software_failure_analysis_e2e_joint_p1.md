# E2E-JOINT-P1 第 1 轮 TEST_FAIL 失败分析（软件归因）

- 负责角色：软件测试
- 能力：软件失败分析（`software_tester.software_failure_analysis`）
- 原始失败：`artifacts/software_test_report_e2e_joint_p1_r1.md` —— `software_tester.software_verification` 对 `E2E-JOINT-P1` 返回 **`TEST_FAIL`**，用例整例 `NOT EXECUTED`（P1 不通过、不得部分通过）。
- 原始失败窗口（UTC）：2026-11-26T04:51:58Z ~ 05:02:56Z（CST 12:51:58 ~ 13:02:56）
- 被测对象：
  - 传感器：既有标准发布构建 `fw_sensor_FD-002r8`（md5 `5df0c6d4…`、sha256 `16ad3f69…`），`uid4=6A002C00`；本轮未烧录/未复位。
  - 网关：BLE `84:C2:E4:03:02:02`，`GwID=B1:17:9C:7A:4E:E0`。
  - Android App：`com.jinyuni.dengbei_care` vC10 / vN1.9，APK SHA-256 `80232c50…de4b`。
  - 平台：`8.140.23.253`（EMQX 5.8.0 `8883` / Flask `user_script` `/root/abr.py`）。
  - 8883 叶子证书：serial `77ADCFD924BA0F87F845C81934D4A3F346D4BB83`，SAN `IP:8.140.23.253, IP:117.72.84.210`，notAfter `2028-11-24T02:55:16Z`；App 信任锚 `jd-ca.crt` 未更换。
- 本轮分析时点（UTC）：2026-11-26T05:17Z 起；仓库 HEAD `1b85dbf`。
- **本轮结论：`EXTERNAL_CONDITION_MISSING`** —— 原验证缺少 P1 所需的测试构建/注入、已确认测试身份、平台字段级回读授权与嵌入式板上观测入口；**未证实当前 App、固件、网关或平台实现存在应当修复的功能缺陷**。

> 本文件只做失败归因与责任边界判断，不是复验通过、不是修复，也不代表 P1、UPLOAD-04 或产品整体通过。
> 本轮未修改产品源码/固件/APK/平台/prefs/数据库；未烧录/复位；未打开 COM43；未直连数据库；未调用平台写接口；未重启或部署任何服务。
> 本轮对设备与服务器的访问仅为读取既有证据与只读源码/配置核对；未回显任何凭据。

---

## 1. 结论摘要

1. **原始失败成立且应保持。** `E2E-JOINT-P1` 的四段（嵌入式、手机、MQTT、平台）都没有 `uid4+367+h` 事件的实际证据；按 `software_test_design_cert8883.md` §3.6 与 `e2e_joint_test_design.md` §4.3，任一段缺失即整例不通过，判定 `NOT EXECUTED` 正确。
2. **证书/TLS 维度在同一窗口确已解除阻断，但只限该维度。** 主机侧用 App 信任锚执行 `-verify_ip 8.140.23.253 -verify_return_error` 得到 `Verify return code: 0 (ok)`；App 窗口内 `SSLHandshakeException` / `No subjectAltNames` / `PKIX` / 证书链错误均为 0；服务端在同一连接上记录到 `AUTHN authentication_failure`，说明 TLS 已完成并进入 MQTT CONNECT。CONNACK `rc=5 无权连接` 属认证层，不是证书/SAN/链失败。
3. **P1 的直接阻塞是前置交付缺失，不是已证实的实现缺陷。**
   - 固件源码/工程/发布物中不存在 `SENSOR_TEST_INJECT`/`TINJ` 测试构建，也没有测试 target；`measure_sample()`（`measure.c:460`）成功后直接调用 `sensor_decide_report()`（`measure.c:504-506`），二者之间无注入点。
   - App 的 MQTT 身份为 `mac_addr=0200000000A1`（合成值）；EMQX 对该 username 连续返回 `not_authorized`。缺的是**已确认来源并可认证的测试身份**，不是 App 连接逻辑缺陷。
   - 平台只有 `POST` 路由（`GET /upload_data` 返回 405），`upload_data` 为 `INSERT IGNORE INTO sensor_data`，无字段级回读入口；本轮无只读 DB 查询授权。UPLOAD-04 的人工延期（“不用回读，传上去就行了”）**不覆盖** P1。
   - 本能力当前有效工具只有 `android_test`、`usb_list_devices`，无 `mdk_flash`/`serial_capture`；板上 `S/G/P/TINJ` 观测不属于本能力边界。
4. **N1 等价基线成立但不等同 N1 独立 PASS。** 网关缓存实际值（12:46:27 `23.5/35.6`、12:54:37 `22.4/36.4`）与“无温度 36.6–36.8”已记录，P1 注入值 `367` 可区分；这是设计允许的等价记录。
5. **结果判断：`EXTERNAL_CONDITION_MISSING`。** 缺少的是测试构建、身份/凭据、回读授权与嵌入式观测入口等外部条件/交付，而不是已证实的产品实现错误。该结果不表示 PASS，也不授权跳过 P1 或任何测项；条件补齐后必须重做同一 P1 用例。

---

## 2. 对象与版本闭合

| 端 | 本轮核对 | 来源 |
|---|---|---|
| 传感器发布包 | `fw_release/fw_sensor_FD-002r8.zip`，134,791 B，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`，sha256 `16ad3f69…f4d0`；成员 axf `b6a15fed…` + hex `4d728dec…` + `PACKAGE.txt` | 本轮重跑审计：`evidence/software_triage_e2e_joint_p1_20261126/01_fw_testbuild_audit_rerun.txt` |
| 传感器运行态 | App 读到 `id=6A002C00`；本轮未烧录/未复位；板上镜像身份按同版本引用，未重新 `Verify OK` | `evidence/e2e_joint_p1_20261126/20_logcat_window.txt:1826` |
| 网关 | `GwID=B1:17:9C:7A:4E:E0`、`dev=1`、`valueSize=160`、`mtu=240`；BLE `84:C2:E4:03:02:02` | `20_logcat_window.txt:1825`、`15_prefs_before.xml` |
| App | vC10/vN1.9；设备侧 `base.apk` SHA-256 `80232c50…de4b`；PID 14224 | `software_test_report_e2e_joint_p1_r1.md` §2 |
| 手机/网络 | Redmi 8A、API 29、serial `12f046690506`；默认网络 101（WIFI/VALIDATED），SSID `ZTE_607157` | 同报告 §2/§3.6 |
| 平台服务 | `user_script` active/running，PID 312368；`/root/abr.py` 8,688 B，sha256 `a062b05f…4017c`；EMQX PID 299216 | `22_platform_readonly.txt` |
| 8883 证书 | 叶子 `CN=8.140.23.253`、SAN 含 `IP:8.140.23.253`/`IP:117.72.84.210`、issuer `CN=Tiktok Root CA`、notAfter `2028-11-24` | `24c_host_leaf_fields.txt`、`24_host_tls_verify_ip.txt` |
| App 信任锚 | `jd-ca.crt` 未更换；`CloudConfig.MQTT_BROKER_URI = ssl://8.140.23.253:8883` | 原始报告 §2 |

> 版本闭合：本轮只读核对的发布包摘要与原始失败报告一致；源码/工程/App 行为来自当前仓库与同一 APK 的既有窗口证据，不是从其他版本外推。

### 2.1 P1 前置逐项

| 前置 | 本轮状态 | 依据 |
|---|---|---|
| P0 已完成 | **满足（引用）** | 引用同版本 `E2E-JOINT-P0` 第 1 轮 PASS（同 APK SHA-256、同手机、同网关绑定/读取就绪、平台只读基线、`CONNACK rc=5` 分类）；本轮未重跑 P0 |
| N1 或等价干净基线 | **等价记录（非独立 N1 PASS）** | 网关缓存 `23.5/35.6`、`22.4/36.4` 与“无温度 36.7”已记录；注入值 `367` 可区分；见 `37_n1_equivalent_baseline.md` |
| 固件测试构建（注入 `367`/`TINJ`/正式排除/`Verify OK`） | **缺失** | §3.3 审计；源码/工程/发布包均无注入与测试 target |
| 隔离测试身份与已确认 `mac_addr` | **缺失** | `mac_addr=0200000000A1`（合成）、`registerphone=14000000001`（模拟）；EMQX `not_authorized` |
| 平台字段级回读授权 | **缺失** | 仅 POST 路由、无回读；无只读 DB 授权；UPLOAD-04 延期不覆盖 P1 |
| `wifi_ZTE_607157` 默认网络 | **满足（原报告记录）** | 默认网络 101（WIFI/VALIDATED），SSID `ZTE_607157`、wlan0 `192.168.0.103/24` |

---

## 3. 原始失败的复核（同一证据、同一边界）

### 3.1 证书/TLS 维度（原报告 PASS，本轮复核一致）

- 主机侧：`openssl s_client -connect 8.140.23.253:8883 -servername 8.140.23.253 -CAfile dengbei_care/app/src/main/assets/jd-ca.crt -verify_ip 8.140.23.253 -verify_return_error` → `Verification: OK`、`Verify return code: 0 (ok)`。
- App 侧窗口（12:52:00–13:02:56）：`SSLHandshakeException`=0、`No subjectAltNames`=0、`PKIX/Trust anchor/CertPathValidatorException/SSLPeerUnverifiedException`=0。
- 服务端同一连接：`AUTHN ... username: 0200000000A1, reason: not_authorized`（12:53:57.824、12:58:57.854，CST）。能到达 AUTHN 本身就要求 TLS 握手与 MQTT CONNECT 已完成。
- **边界**：这只证明证书/TLS 维度已解除阻断，不证明 P1、MQTT 业务发布或平台持久化通过；CONNACK `rc=5` 是认证结果。

### 3.2 P1 四段没有可判定的实际事件

- 窗口内 `T=36.7` 计数 0、`TINJ` 计数 0；本地库 140 行温度中无 36.6–36.8 行（`19_local_db_summary.json`、`30_local_db_after_summary.json`）。
- 观测到的是网关当前缓存值 `22.4°C / 36.4%` 的真实 BLE 读取、本地入库与一次 HTTP 上传（平台 200）；该上传是缓存复读语义，不是新物理采样，更不是 P1 注入事件。
- 因此嵌入式/手机/MQTT/平台四段都不能判 PASS；`NOT EXECUTED` 与“不得部分通过”的处置正确。

### 3.3 本轮新增的独立核对

| 核对 | 实际 | 结论 |
|---|---|---|
| 固件源码注入标记 | `SENSOR_TEST_INJECT`/`TINJ` 命中 0 | 无测试注入接口 |
| Keil target | 仅 `Project`；`Define` 为空 | 无测试构建配置 |
| 发布包测试字符串 | `TINJ`/`SENSOR_TEST_INJECT`/`INJECT` = `NONE` | 正式包不含测试符号 |
| `measure_sample()`→`sensor_decide_report()` 之间 | 无测试分支 | 设计要求的注入点未实现 |
| App MQTT 凭据 | `userName=mac_addr`、`password=mac_addr+"&^!A:z?"`（`MqtttService.kt:1609-1618`） | 合成 `mac_addr` 无对应认证记录 |
| `shouldPublishBle()` | `device_mac.isNotBlank() && !startsWith("1010")`（`:1031-1032`） | `0200…` 不以 `1010` 开头，产品分支为“应发布”，不是 N/A |
| 平台回读 | 仅 POST 路由；`GET /upload_data` 405；`INSERT IGNORE` | 无字段级回读入口 |
| 本地库 36.7 | `rows_36_7=[]`；但存在历史**湿度** 36.7 行 | 温度 36.7 可区分；披露湿度同值避免混淆 |

详见 `evidence/software_triage_e2e_joint_p1_20261126/02_source_and_config_crosscheck.md` 与 `01_fw_testbuild_audit_rerun.txt`。

---

## 4. 竞争假设判定

| # | 假设 | 判定 | 依据 |
|---|---|---|---|
| H1 | 8883 证书/SAN/链仍未修复，P1 被 TLS 阻断 | **排除** | 主机 `verify_ip` rc=0；App 无 SAN/链错误；服务端已到 AUTHN；`rc=5` 是认证层 |
| H2 | 固件测试构建/注入已存在但未生效 | **排除（不存在）** | 源码/工程/发布包均无注入宏、测试 target 或 `TINJ`；发布包无测试字符串 |
| H3 | App MQTT 连接逻辑缺陷导致认证失败 | **不成立** | App 按既有方案用 `mac_addr` 作用户名；合成值 `0200000000A1` 不在 EMQX 认证库；属身份/凭据缺口 |
| H4 | 平台回读只是“暂未找到”，实际存在查询入口 | **排除（就当前部署）** | 路由清单全部 POST；`GET /upload_data` 405；`upload_data` 仅 `INSERT IGNORE`；无只读 DB 授权 |
| H5 | App 读取/解码/本地消费/上传存在缺陷 | **未证实** | 缓存读取成功（MTU 240、valueSize 160、dev=1、DATA_OK）、本地行生成、HTTP 200；但未发生注入，不能替 P1 事件，也不能据缓存成功推及“不完整帧整帧拒绝”分支 |
| H6 | 网关/433/传感器物理链路故障 | **未证实** | 12:54:37 有真实 BLE 读取成功；12:57–13:02 出现间歇 `CONNECT_FAILED`，但当时无注入、无 P1 事件，不能归因；无 COM43/433 电气证据 |
| H7 | 平台 `INSERT IGNORE` 幂等语义与资料不一致 | **未验证** | 无回读授权/入口；本轮不声称行数或字段，不能用 HTTP 2xx 替代 |
| H8 | 证书修复本身破坏上传或 App 行为 | **未证实** | 上传走 HTTP 5000，与 8883 TLS 独立；窗口见过 `POST /upload_data 200`；无崩溃/证书相关异常 |

**未确定且不影响本次责任对象**：间歇 BLE `CONNECT_FAILED` 的根因（网关供电/无线/App 重连的哪一侧）、EMQX 认证库中真实设备记录的存在性与字段、平台 `sensor_data` 唯一键与行数。它们都需要在条件具备后按 P1 的原判据实测，不能被本轮推测替代。

---

## 5. 缺口、责任方与准备动作

| # | 缺口 | 责任方 | 证据 | 人工/实现侧准备动作 | 复验时必须重做 |
|---|---|---|---|---|---|
| G-1 | 无固件测试构建：默认关闭、仅测试启用；`measure_sample()` 成功后、`sensor_decide_report()` 前注入 `367`；可区分 `TINJ`；正式包排除；P1 后恢复标准构建并 `Verify OK` | **固件实现**（提供构建/埋点）；**嵌入式能力**（烧录与 COM43 观测） | 源码/工程/发布包审计；`measure.c:460/504-506` | 由用户决定是否把该实现交付纳入本轮/后续；实现提供测试 target 或文档化 `-D` 构建、构建摘要与正式包排除证据；不得由测试自行改产品源码启用 | 测试构建身份 + `Verify OK` + 第 1/2/3 次 `S/TINJ/P` + 恢复标准构建并再次 `Verify OK` |
| G-2 | 无已确认来源的 `mac_addr` 与可用测试身份（EMQX 认证库可接受） | **用户/身份提供方**（不得使用真实用户数据；不得手改 prefs/DB） | `mac_addr=0200000000A1`；EMQX `not_authorized`；`MqtttService.kt:1609-1618` | 提供隔离测试手机号与可追溯的 12 位 HEX 网关平台 MAC，并按既有注册/`setMacAddress`/认证库流程使其可认证；不能用合成值替代后声称业务发布 | CONNACK 实际结果、`/<mac>/a/r0` 载荷与 App 侧身份闭合 |
| G-3 | 无平台 `sensor_data` 字段级只读回读授权/入口 | **平台/运维** | 路由清单无回读；`GET /upload_data` 405；无 DB 授权 | 提供只读 SQL 入口或等效回读结果；UPLOAD-04 的“不用回读”延期**不扩展到 P1** | 按 `(dev_id,time)` 回读 `(phone,dev_id,time,temperature=36.7,humidity=h)` 与同键行数 |
| G-4 | 本能力无 COM43/烧录入口，板上 `S/G/P/TINJ` 无法观测 | **嵌入式能力** | 当前有效工具仅 `android_test`/`usb_list_devices` | 由嵌入式能力在同一测试构建窗口执行端口先开后复位/下载、采集原文并记录 `Verify OK` | 第 1 次真实 `S r=0`；第 2 次 `TINJ/S t=367 q=1 r=1 s=1`、`P t=367 h`；第 3 次 `t≈real p=367 r=0` |
| G-5 | N1 独立板上基线未观测 | 嵌入式能力 | 本能力无 COM43 | 与 G-4 同窗口取得 | 等价记录可保留，但 N1 独立 PASS 仍需板上观测 |

> 说明：G-1 是“测试构建/观测接口”的实现交付，不是人工物理条件；`e2e_joint_test_design.md` §3.1 明确“若执行轮拿不到，P1 记 NOT EXECUTED，附最小实现请求与复测判据，交固件实现闭环”。本报告不代固件实现改代码，也不把缺构建判成固件功能缺陷。

---

## 6. 结果判断与理由（为什么是 `EXTERNAL_CONDITION_MISSING`）

### 6.1 选择 `EXTERNAL_CONDITION_MISSING`

- 原验证缺的是：测试构建/注入（G-1）、已确认身份/凭据（G-2）、平台回读授权（G-3）、嵌入式板上观测入口（G-4）等**外部条件、授权与交付**。
- 既有实测与源码核对**没有证实** App、固件、网关或平台存在应当修复的功能错误：TLS/证书维度通过；App 行为符合既有认证/发布/离线分支；平台上传受理成功。
- 该结果不表示测试 PASS，也不授权跳过 P1；它只把“缺哪些条件、谁负责、补齐后怎么重做”写清。

### 6.2 不选其他结果码的理由

| 结果码 | 不选的理由 |
|---|---|
| `REPAIR_REQUIRED` | 没有已证实的特定可修改对象需要修复；主阻塞是缺失前置（测试构建/身份/回读/观测入口），不是功能缺陷。且图中该分支声明的修改能力是 `devops_engineer.infrastructure_management`（基础设施/部署），**无权**处理固件源码的测试构建或 App 身份；把固件测试构建写成 `REPAIR_REQUIRED` 会越过图中已声明职责。 |
| `RESPONSIBILITY_UNRESOLVED` | 责任已可定位：固件实现（G-1）、用户/身份提供方（G-2）、平台/运维（G-3）、嵌入式能力（G-4）。不是“无法确定责任”。需要的是条件与授权，不是继续调查对象。 |
| `REPLAN_REQUIRED` | 当前图的缺项分支已声明：`EXTERNAL_CONDITION_MISSING` → 报告缺项并交用户决定；测试设计 §3.6 也明确前置不满足时整例 `NOT EXECUTED`、保留原目标。是否把固件实现/身份/回读纳入后续是用户决定后的规划事项，不改变本轮“缺外部条件”的事实；本分析不代规划新增节点，也不把缺条件升级为返工。 |
| `ASSIGNMENT_DECLINED` | 本能力被分派的对象是 `E2E-JOINT-P1` 的失败分析，对象与职责匹配；不存在需要转交的替代分析对象。 |
| `WORK_FAILED` | 分析已完成，证据可读、可复核；未发生工具/环境导致的执行失败。 |

---

## 7. 影响范围与复验判据

### 7.1 影响

1. **测试结论**：`E2E-JOINT-P1` 保持 `NOT EXECUTED` / 不通过；四段不得任何部分通过；`CERT8883-CLI-02` 或 P0 的既有 PASS 不能替代 P1。
2. **证书结论**：App→EMQX 的 TLS/SAN/链阻断已解除（限同窗口、同版本、同 App 信任锚）；CONNACK `rc=5` 是认证层结果，需与证书结论分开报告。
3. **上传/持久化**：窗口内 HTTP `/upload_data` 受理 200 真实发生，但没有 `uid4+367+h` 事件与字段级回读，不能判平台段通过；UPLOAD-04 的延期不改变 P1 判据。
4. **非影响**：本轮没有证据表明证书修复破坏了 App、网关读取或 HTTP 上传；间歇 BLE `CONNECT_FAILED` 保留为独立观察，不归因于证书或 P1。

### 7.2 条件补齐后的复验（同一用例、同一判据）

1. 记录测试构建身份 + `Verify OK`；同一 COM43 窗口取第 1 次真实 `S r=0`、第 2 次 `TINJ real=<real> inj=367` + `S t=367 q=1 r=1 s=1` + `P t=367 h=<h>`、第 3 次 `t≈real p=367 r=0`；P1 后恢复标准构建并 `Verify OK`、确认无 `TINJ`。
2. 手机段：`MTU≥35`（期望 240）、`valueSize≥32` 且 16 B 对齐、`devCount=1`、`id=uid4`、`T=36.7`、`H=h`、`DATA_OK`；本地 temperature 行 `(uid4, receiveTime, 36.7, h)`；上传按成功/失败实际记录；“读取不完整整帧拒绝”需另行受控刺激。
3. MQTT：`/<mac>/a/r0` 载荷 `<epoch>:36.7:<h>`，`epoch` 与本地接收时间一致；`mac_addr` 以 `1010` 开头时按既有分支记 N/A；应发布而认证失败时记录实际原因，不得用 HTTP 成功替代。
4. 平台：HTTP 2xx **且**按 `(dev_id,time)` 回读到字段一致的行；同键重试不新增；仅 2xx 或无回读入口不通过。
5. 证书维度：同窗口再次记录无 SAN/链错误、TLS 完成、CONNACK 实际结果；证书维度 PASS 仍不得当作 P1 PASS。

---

## 8. 本轮未执行 / 未知（不写成 PASS）

| 项 | 状态 | 原因/缺失条件 |
|---|---|---|
| 重跑 P1 四段 | **NOT EXECUTED** | 无测试构建/注入、无已确认身份、无回读授权；重跑只会复现同一缺项 |
| 固件烧录/复位、COM43 采集 | **NOT EXECUTED** | 不在本能力边界；当前无 `mdk_flash`/`serial_capture` 入口 |
| 修改固件/App/平台/prefs/DB | **未执行** | 不属于失败分析；且会破坏未授权边界 |
| 直连数据库 / 调用平台写接口 | **未执行** | 无授权；本轮只读核对路由与既有日志 |
| 注册 EMQX 测试凭据 / 强改 `mac_addr` | **未执行** | 需用户/平台授权；不得伪造身份绕过 |
| 间歇 BLE `CONNECT_FAILED` 根因 | **未确定** | 无 P1 注入窗口；不足以归因网关/App/无线 |
| 平台 `sensor_data` 行数/字段/唯一键实测 | **未验证** | 无回读入口/授权；不以 HTTP 2xx 替代 |
| 真实物理 >35℃ / 升温>0.9℃ 且无光 / 光强双态 / 功耗 / 433 电气 | **NOT EXECUTED** | 用户条件与已批准延期；不安排、不估算冒充 |

---

## 9. 边界与脱敏

- 本轮只读核对源码、工程、发布包、既有 App/平台证据与既有日志；未修改产品源码/固件/APK/平台配置/prefs/数据库；未烧录/复位；未重启/部署服务；未使用 `sudo`；未直连数据库。
- 未回显任何密码、token、私钥或数据库口令；EMQX 仅记录 `username` 与 `not_authorized` 结果。
- 本文件不代表修复完成或复验通过；`EXTERNAL_CONDITION_MISSING` 仅表示原验证缺少外部条件/交付且未证实产品实现错误。原 `TEST_FAIL` 与 `NOT EXECUTED` 保持有效。

---

## 10. 证据索引

| 文件 | 用途 |
|---|---|
| `artifacts/software_test_report_e2e_joint_p1_r1.md` | 原始失败报告（P1 四段 NOT EXECUTED、证书维度 PASS、口径与边界） |
| `artifacts/software_test_design_cert8883.md` §3.6 | P1 前置与“不满足前置→整例 NOT EXECUTED”判据 |
| `artifacts/e2e_joint_test_design.md` §3.1/§4.3/§8 | 测试构建要求、四段判据、缺项责任表 |
| `artifacts/test_design.md` §6.2 | 注入接口与测试构建默认关闭要求 |
| `evidence/software_triage_e2e_joint_p1_20261126/01_fw_testbuild_audit_rerun.txt` | 本轮重跑的固件测试构建/正式包排除审计（只读） |
| `evidence/software_triage_e2e_joint_p1_20261126/02_source_and_config_crosscheck.md` | 固件注入点、App 发布分支/凭据、平台路由、TLS 与本地库的可区分性核对 |
| `evidence/e2e_joint_p1_20261126/21_fw_testbuild_audit.txt` | 原始轮固件测试构建审计（与本轮重跑一致） |
| `evidence/e2e_joint_p1_20261126/24_host_tls_verify_ip.txt` / `24c_host_leaf_fields.txt` | 用 App 信任锚校验 SAN/IP 通过；叶子字段 |
| `evidence/e2e_joint_p1_20261126/26_emqx_window_1254.txt` / `31_emqx_window_1259.txt` | EMQX 同窗口 `authentication_failure`/`not_authorized` 原文 |
| `evidence/e2e_joint_p1_20261126/22_platform_readonly.txt` | 平台服务身份、路由清单、无回读入口、`GET /upload_data` 405 |
| `evidence/e2e_joint_p1_20261126/20_logcat_window.txt` | App 同窗口 TLS/CONNACK、BLE 读取、本地入库、上传与间歇链路原文 |
| `evidence/e2e_joint_p1_20261126/15_prefs_before.xml` / `29_prefs_after.xml` | `mac_addr=0200000000A1`、`registerphone=14000000001`、BLE 地址；前后未变 |
| `evidence/e2e_joint_p1_20261126/19_local_db_summary.json` / `30_local_db_after_summary.json` | 温度行数/最新行/无温度 36.7；湿度 36.7 的历史行披露 |
| `evidence/e2e_joint_p1_20261126/37_n1_equivalent_baseline.md` | 网关缓存实际状态与缓存复读语义（等价基线） |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt:374/1031-1032/1609-1618` | 发布分支、用户名/口令来源、离线保留行为 |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/measure.c:460/504-506` | 采样→判定的现有顺序；二者之间无注入分支 |
