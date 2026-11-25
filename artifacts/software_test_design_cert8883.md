# 平台 8883 证书与客户端地址不匹配修复：软件测试设计（TD-CERT8883-001）

状态：软件测试设计产物（`software_tester.software_test_design`，输出端口 `test_design`）。**只做设计，不执行**；本能力不登录服务器、不操作手机/网关/传感器、不采集串口、不访问平台、不写库、不改产品源码、不改验收判据。
版本：1.0（本轮证书修复与上传/受影响端到端验证轮次）。本文件不替代 `artifacts/e2e_joint_test_design.md`（TD-E2E-SW-006 v1.0）的联合用例细节，也不替代 `artifacts/test_design.md`（TD-002 rev 10.0）与 `artifacts/software_test_design.md`（TD-SW-005 v5.0）；三者在各自范围内继续有效。本轮把"证书修复验收 + 安卓到平台上传 + 受影响的既有端到端用例 + 本轮交付限定整体验收"整理为同一份可执行方案。
对象：既有平台 `8.140.23.253` 上 EMQX 5.8.0 的 **8883 TLS listener 服务器证书**及其修复后的客户端/平台链路；当前 Android 候选 AB-003（`com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`）；既有传感器 FW FD-002r8 与既有网关（不烧录）。
上游依据：当前 Project Goal；`服务器迁移记录.md`；`docs/05_平台服务器配置与联调前提.md`；`docs/06_维护排障与已知限制.md`；`artifacts/e2e_joint_test_design.md`（TD-E2E-SW-006）；`artifacts/e2e_cases.yaml`；`artifacts/interface_contract.md`（IC-002 v4.0）；`artifacts/android_build.md`（AB-003）；`evidence/e2e_p0/P0_platform_stage_report.md`（原始 TEST_FAIL）；`evidence/e2e_p0/P0_platform_failure_triage.md`（REPAIR_REQUIRED）；`evidence/e2e_p0/P0_backend_repair_scope.md`；`evidence/e2e_p0/P0_backend_cert_issuance_and_rollback.md`；`evidence/e2e_p0/P0_backend_cert_ops_runbook.md`；`evidence/e2e_p0/backend_cert_recheck_20261124_1015Z.txt`；`scripts/emqx_8883_cert_verify.sh`；`artifacts/codegraph_tls_index.md`；当前统一环境说明。

**结论先行**

1. `DESIGN_READY` 只表示本方案在下列条件下可执行，**不表示证书已修复、任何用例已执行或通过，也不表示产品整体验收通过**。
2. 已实测缺陷是**服务器证书主机名标识与客户端连接地址不一致**：8883 叶子证书 `subject CN=117.72.84.210`、`SAN` 仅 `IP Address:117.72.84.210`，而 App 连接 `ssl://8.140.23.253:8883`；链可信（`issuer CN=Tiktok Root CA`，App 信任锚 `assets/jd-ca.crt`），失败点是 SAN 不匹配（`SSLHandshakeException: No subjectAltNames on the certificate match`）。
3. 修复的唯一允许边界：以 App 当前信任锚 `Tiktok Root CA` 签发并部署包含 `SAN=IP:8.140.23.253` 的 8883 叶子证书，保持 App 地址/端口/信任锚/MQTT 认证格式与其他 listener/服务不变。**不得**关闭主机名校验、`--insecure`、换公共 CA、自签替代或回退 App 到旧地址。
4. 本设计的**可执行核心**是：服务器侧独立证书核验（`scripts/emqx_8883_cert_verify.sh` + `openssl`）与**真实 App 对 `ssl://8.140.23.253:8883` 的 TLS/MQTT CONNECT/CONNACK 复验**。这两项不依赖传感器数据或测试手机号。
5. **安卓到平台上传**（App 真实路径 `POST /upload_data` → MySQL `sensor_data` 回读）与**受影响的联合正向用例 P1**（受控注入同一笔 `uid4+367+h` 四段一致）需要当前未具备的条件：隔离测试身份（`registerphone` 非空）、已确认的平台网关 `mac_addr`、一条真实/受控读数、平台写与回读授权、固件测试构建（P1）。条件具备时必须执行；不具备时对应用例记 `NOT EXECUTED` 并交回缺口，**不得**用 HTTP 2xx 冒充持久化、用占位/缓存冒充新采样、用 TLS/CONNACK 成功冒充上传通过。
6. 真实物理 >35℃、真实升温 >0.9℃ 且无光、光照双态、功耗、433 射频电气、9 台真实快照、API 31+、release/生产签名包、iOS 仍为 `NOT EXECUTED`（用户条件/已批准延期/后续阶段）；它们不是本轮证书修复的退出条件，但也绝不是通过。
7. 本文件与 `artifacts/cert8883_cases.yaml` 是同一方案的两面：本文件是完整判据，清单为机器消费摘要；二者不得出现两套标准。

---

## 0. 设计时基线（执行轮必须重新核对，不得沿用旧结论）

### 0.1 当前版本与身份

| 端 | 设计时事实 | 来源 | 执行轮要求 |
|---|---|---|---|
| 平台 | `8.140.23.253`；EMQX 5.8.0（`emqx@127.0.0.1` running、Dashboard/status 200）；Flask 5000（Werkzeug/3.1.8 Python/3.12.3）；MySQL 9.0.1（仅端口握手） | `P0_platform_stage_report.md` §3；`platform_stage_retest_*` | 重新记录当次服务/端口/证书；只读，不登录、不重启、不写 |
| 8883 叶子证书（缺陷基线） | `CN=117.72.84.210`；`issuer CN=Tiktok Root CA, O=LinkValue`；`serial=77ADCFD924BA0F87F845C81934D4A3F346D4BB82`；SHA-256 `48:76:E7:30:CC:AB:3E:35:4C:5B:38:19:B9:D7:0D:F1:10:63:51:91:6E:06:EE:EF:22:6A:37:7C:E3:54:AD:75`；有效期 2024-10-12–2026-12-21；`SAN` 仅 `IP:117.72.84.210` | `backend_cert_recheck_20261124_1015Z.txt` | 先记录基线；修复后 serial 必须不同且 SAN 含目标 IP |
| App 信任锚 | `dengbei_care/app/src/main/assets/jd-ca.crt`；`subject/issuer CN=Tiktok Root CA, O=LinkValue`；有效期至 2034-10-10；SHA-256 `75:33:09:B3:9F:1B:8F:85:B4:D7:57:76:BF:D3:8A:42:52:0D:C1:A0:5E:A2:0B:A3:11:7E:B3:9C:BA:B2:15:2B` | 本地 `openssl x509` 只读 | 修复不得替换；若替换则进入 §9 信任锚变更分支 |
| Android | AB-003：`com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，debug；APK 10,370,964 B；SHA-256 `80232c50…de4b`；设备 Redmi 8A / Android 10 / API 29 / serial `12f046690506`；安装字节一致 | `P0_phone_stage_report.md` §1/§3；`android_build.md` | 重新核对版本、主机/设备侧摘要；APK 变化则按新组合重采 |
| App 连接配置 | `CloudConfig.SERVER_HOST="8.140.23.253"`；`MQTT_BROKER_PORT=8883`；`MQTT_BROKER_URI="ssl://8.140.23.253:8883"`；`HTTP_BASE_URL="http://8.140.23.253:5000"` | `cloud/CloudConfig.kt` | 核对未因修复被改；不改地址/端口 |
| App TLS 行为 | Paho 1.2.5；`MqttConnectOptions()` 默认 `httpsHostnameVerificationEnabled=true`；`setupSSL()` 用 `jd-ca.crt` 构造 `TrustManagerFactory`，未关闭主机名校验；`SSLContext("TLSv1.2")` | `MqtttService.kt`、`libs.versions.toml`、字节码核对 | 不改校验行为；不得以自定义空校验绕过 |
| 传感器/网关 | 传感器 FW FD-002r8；`uid=6A002C00`；网关 BLE `84:C2:E4:03:02:02`、`GwID=B1:17:9C:7A:4E:E0`、`dev=1`、`valueSize=160`、`mtu=240`；读取终态 `ALL_PLACEHOLDER`（无有效数据） | 手机段/EV-041 | 重新核对；不烧录网关；不改绑定 |
| 手机网络 | `ZTE_607157` 为当前默认网络（WIFI，IP 192.168.0.103/24） | `P0_phone_stage_report.md` §3.3 | 重新确认 SSID 与默认网络；密码只经 `ui_set_text` + `text_env` 注入，不落证据 |
| 测试身份 | `registerphone` 空、`mac_addr` 空/`000000000000` | 手机段 | 无身份时上传/P1 段 `NOT EXECUTED`；不得伪造或改 prefs/DB |

### 0.2 本能力与执行能力可用入口

本设计能力可用：`Read`、`Glob`、`Grep`、有界只读 `Bash`、`Write`、`Edit`、只读 `usb_list_devices`；`allowed_external_actions=[]`。**本能力不执行任何设备、平台、网络或写操作。**

执行阶段由 `software_verification` 承担，其当前角色授权为：`android_test`、`usb_list_devices`、`operate_test_equipment`、`operate_test_browser` + `Bash`。由此：

| 入口 | 执行能力可用性 | 本设计处理 |
|---|---|---|
| 主机只读 TLS/HTTP 探测（`openssl`、`curl`；公开端口） | 普通 CLI 工作，无需逐条登记；只读、不带凭据 | 用于 C1/C3；不得 `--insecure`、不得登录、不得 POST 写 |
| `android_app_testing` / `android_test`（`12f046690506`） | 可用（按其实际授权） | 用于 C2/C4/P0/P1 手机段；实际调用前确认设备在线与授权 |
| `existing_service_browser_test`（Playwright + Edge） | 可用 | 只用于只读页面/API 观测；不作为上传写入通道 |
| `existing_service_ssh_inspection` / 串口 / 烧录 / 逻辑分析仪 / DB 直连 | **不在该执行能力当前授权内** | 服务端内部证书文件、CA 私钥、listener 配置由部署能力交付证据；串口/注入/烧录由嵌入式/固件能力负责；DB 回读需单独授权。缺这些条件时对应子项 `NOT EXECUTED`，不写成通过 |

> 入口不可用只是当前能力的工具条件，不等于目标离线或用户未提供。执行阶段以自己的有效授权为准；发现需要额外入口时按契约申请，不自行扩权。

### 0.3 当前缺口（不改写、不豁免）

| ID | 缺口 | 对本轮的影响 |
|---|---|---|
| G-1 | 8883 叶子证书仍是旧 SAN（截至 2026-11-24T10:19Z 四次基线一致） | 修复未生效时 C1/C2/P0 平台段直接 FAIL，不得判 NOT EXECUTED |
| G-2 | 执行能力无服务器变更入口与 CA 私钥访问 | 真实签发/部署由 `devops_engineer.infrastructure_management` + `deployment_operations` 完成；软件测试只独立核验结果，不代做 |
| G-3 | 无隔离测试身份（`registerphone`/`mac_addr`） | C4 上传与 P1 上传/持久化段 `NOT EXECUTED`；需用户/手机段通过 App 既有注册与 `/setmac` 路径准备 |
| G-4 | 网关当前无有效数据（`ALL_PLACEHOLDER`），无固件注入构建/烧录入口 | C4 缺少合法读数来源；P1 需要测试构建注入 `367`，由固件/嵌入式能力提供 |
| G-5 | 平台写与 DB 回读无授权 | C4/P1 持久化回读 `NOT EXECUTED`；HTTP 2xx 只能证明受理 |
| G-6 | 执行能力无串口采集 | N1/P1 的嵌入式观察不能由本执行能力重采；证书修复不改变该段源码。若已有同版本通过证据可引用并标注适用范围；**没有**通过证据时不得引用，N1/P1 相应段按 `NOT EXECUTED` 记录，需嵌入式能力重采 |
| G-7 | 无真实物理 >35℃/升温+无光/光强变化/功耗仪器 | 真实物理分支 `NOT EXECUTED`；不安排调温、调光、遮光、功耗测试 |
| G-8 | 无 release/生产签名包 | 只验证当前 debug 候选；不冒充正式发布 |

---

## 1. 修复对象、受影响路径与回归边界

### 1.1 受影响路径（本修复的核心）

```text
Android App (Paho 1.2.5, TLSv1.2, 信任锚 assets/jd-ca.crt)
  --TLS ClientHello/SNI--> 8.140.23.253:8883 (EMQX 5.8.0 ssl listener)
     服务器证书: issuer CN=Tiktok Root CA; SAN 必须含 IP:8.140.23.253
  <--ServerHello/Certificate--
  App 校验: 链可信（jd-ca.crt） + 主机名/IP SAN（Paho 默认 HTTPS endpoint identification）
  --> MQTT CONNECT (username=mac_addr, password=既有固定后缀；不改)
  <--CONNACK: 记录实际结果（认证失败属另一层，不与 TLS 混判）
```

**修复通过的必要条件**：修复后的叶子证书由 `Tiktok Root CA` 签发、`SAN` 含 `IP:8.140.23.253`、`openssl verify -CAfile jd-ca.crt -verify_ip 8.140.23.253` 为 `OK`；App 对同一地址不再出现 `No subjectAltNames`，TLS 握手完成并进入 MQTT CONNECT。

### 1.2 同一次修复后需要回归、但不属于证书缺陷的边界

| 边界 | 为什么纳入本轮 | 判定方式 |
|---|---|---|
| 其他 EMQX listener（1883/8083/8084）、Dashboard 18083 | 证书替换不得改变其他 listener 语义 | 只读端口/HTTP 探测；记录与迁移资料的差异，不把既有差异当本次回归 |
| Flask 5000（`/`、`/login`、`/upload_data`）与 MySQL 3306 | 修复不得改路由、认证格式、表结构 | 只读 GET/OPTIONS + 端口握手；不 POST、不登录、不连库 |
| MQTT 认证（用户名=网关 MAC、固定后缀） | 修复不得改认证库/口令 | 部署记录声明与 CONNACK 实际结果；无测试身份时认证失败单独记录 |
| App 启动/守护/BLE 读取 | 证书修复不应影响非 MQTT 路径 | App 启动、守护状态、网关读取终态；与手机段既有 PASS 对齐 |
| HTTP `/upload_data` 上传 | 用户要求复测；且不得被证书修复破坏 | 见 C4；无身份/数据/回读授权时 `NOT EXECUTED`，不以 2xx 代替 |

**关闭判据分层**：证书修复本身的关闭判据是 C1 + C2 + C3；HTTP 上传/持久化（C4）与联合正向用例（P1）是用户目标中的独立受影响项，不因证书修复自动满足，也不被证书修复结果替代；C7 按各项实际状态给出本轮交付结论。

### 1.3 同一事件/同一对象的关联键

1. **证书身份**：叶子证书 `serial` + SHA-256 指纹 + `notBefore/notAfter` + SAN 原文；部署记录中的新 serial 必须与线上握手取回的一致。
2. **信任锚身份**：`jd-ca.crt` SHA-256 指纹（基线 `75:33:09:B3:…:15:2B`）；修复前后必须一致，否则进入信任锚变更分支。
3. **客户端身份**：APK SHA-256 + versionCode/versionName + 设备 serial/型号/API + 安装侧 `base.apk` 摘要。
4. **上传事件**：`uid4`（8 位 HEX）+ `receiveTime`/平台 `time`（Unix 秒）+ 温湿度数值 + `(dev_id,time)`；不得靠时间接近猜同一笔。
5. **时间窗**：主机 TLS 探测、App 日志、平台只读观测必须记录同一 UTC 窗口；主机探测与 App 观测需相邻且可解释。

### 1.4 需求来源到用例/证据的映射

| 需求/验收来源 | 本设计中的可执行目标 | 必需证据类别 | 当前可行性 |
|---|---|---|---|
| Project Goal：处理平台 8883 证书与客户端地址不匹配 | C1（线上证书/链/IP SAN/负例）、C2（App TLS/CONNACK） | 主机 `openssl` 原始输出 + 部署执行记录；App logcat/UI | 证书部署后即可执行 |
| Project Goal：核实原 CA 能否签发 | C1 的 issuer=CN=Tiktok Root CA、`chain_verify` 以 `jd-ca.crt` 通过、`jd-ca.crt` 指纹不变；部署记录中的 CA 私钥使用证据（引用） | 证书字段/链校验 + 部署记录；软件测试不直接读取 CA 私钥 | 依赖部署完成；CA 私钥可用性由部署方核实 |
| Project Goal：复测安卓到平台上传 | C2（TLS/MQTT，上传的前置）+ C4（POST `/upload_data` + `sensor_data` 回读 + 幂等） | App 日志/UI + HTTP 响应 + 平台回读行 | C2 可执行；C4 依赖身份/读数/回读授权 |
| Project Goal：受影响的端到端用例 | C5（P0 平台段复验）、C6（P1 四段） | 联合用例四段证据 + 当前组合版本身份 | P0 平台段可执行；P1 依赖注入构建/身份/回读 |
| Project Goal：若需更换 App 信任锚，报告并交用户决定 | §9 信任锚变更分支 + C7 的影响披露 | 新 CA/新 APK 身份 + 影响范围 | 仅在原 CA 不可用时触发；不由测试自行决定 |
| 修复不得破坏其他服务 | C3（其他 listener/HTTP/MySQL 与 App 非 MQTT 回归） | 只读端口/HTTP/握手输出 + App 运行状态 | 可执行 |
| 交付可运行软件时的整体检查 | C7（本轮交付限定整体验收） | 上述证据索引 + 未执行清单 + 清理记录 | 取决于 C1–C6 的实际状态 |

**测试侧自备工具**：执行能力可在 test 权限内编写/运行解析脚本、日志切片脚本、只读探测脚本与 fixture（不修改产品源码、不改权限/环境配置、不绕过业务路径）；现有 `scripts/emqx_8883_cert_verify.sh` 可直接复用。需要产品增加受控注入/诊断接口时，按 C4/P1 的"实现配合"要求提出，不由测试自行改产品。

## 2. 测试层次与可证明边界

| 层 | 环境/手段 | 能证明 | 不能证明 |
|---|---|---|---|
| L0 | 主机 `openssl`（对公开 8883） | 线上叶子证书字段、链与 IP SAN、负例、serial 变化 | App 运行时信任路径、MQTT CONNECT、上传 |
| L1 | App 真机日志/UI（`android_test`/adb） | App 对同一地址不再报 SAN 错、TLS 完成、CONNECT/CONNACK 实际结果、非 MQTT 路径未回归 | 服务端内部配置、CA 私钥使用、上传持久化 |
| L2 | 平台只读探测（公开端口/HTTP） | 其他端口/路由/服务身份未回归、无新增诊断入口 | 服务端文件/配置/认证库变更、DB 行 |
| L3 | App 真实上传 + 平台回读 | App 真实路径受理与持久化、幂等 | 需要身份/数据/回读授权 |
| L4 | 联合用例（同 `uid4+数值+时间窗`） | 跨端同一事件可追溯 | 物理感知、433 电气、功耗；注入只证明判定之后 |

**纪律**：L0 证书正确不证明 App 运行通过；App `Connected to broker` 不证明认证后业务可用；HTTP 2xx 不证明持久化；CONNACK 认证失败不等于证书缺陷；宿主机/桩结果不替代真机/真实服务。

---

## 3. 用例设计

> 通用约定：每例先记录"平台服务身份 + 线上证书字段 + App/设备身份 + 网络 + 时间窗"，再按步骤执行；保存原始输出与判定表；单段缺失不得用其他段通过冒充。`input_refs` 摘要见 `artifacts/cert8883_cases.yaml`，本文件为准。
> 执行时限：证书握手/`openssl` 探测每项 ≤20 s（脚本内已有 `timeout 20`）；App 重连观察覆盖 ≥2 个重连周期且总时长 ≤5 min；上传触发后回读窗口 ≥1 个上传周期并记录实际起止；任何超时记录实际耗时与状态，空结果/超时不等于通过。

### 3.1 C1 — 服务器 8883 证书与 App 信任锚一致性独立核验（`CERT8883-SRV-01`）

**目标**：不依赖部署方自述，独立确认线上 8883 返回的叶子证书已由 App 当前信任锚 `jd-ca.crt` 签发、SAN 命中 `8.140.23.253`、serial 已从缺陷基线变更，且判据未被放宽。

**准备**：
1. 记录执行时间（UTC）、主机、`openssl version`；确认 `dengbei_care/app/src/main/assets/jd-ca.crt` 可读且 SHA-256 为基线值（不一致即停止并转 §9）。
2. 记录缺陷基线 serial `77ADCFD924BA0F87F845C81934D4A3F346D4BB82` 与旧 SAN，作为对照。
3. 只读网络探测；不使用任何账号/口令；不 `--insecure`；不改本地信任库。

**刺激/命令（只读）**：
```bash
bash scripts/emqx_8883_cert_verify.sh 8.140.23.253 8883 \
  dengbei_care/app/src/main/assets/jd-ca.crt --expect-new
openssl s_client -connect 8.140.23.253:8883 -CAfile dengbei_care/app/src/main/assets/jd-ca.crt \
  -verify_ip 8.140.23.253 -verify_return_error </dev/null 2>&1 | tail -40
openssl x509 -in dengbei_care/app/src/main/assets/jd-ca.crt -noout -fingerprint -sha256
```

**观察点**：`tls_handshake`、叶子 `subject/issuer/serial/dates/fingerprint`、`SAN` 原文、`chain_verify`、`ip_san_match`、`negative_control`、`serial_changed`、`RESULT overall`。

**通过判据（全部满足）**：
1. `tls_handshake=OK`；
2. `chain_verify=OK`（以 `jd-ca.crt` 为唯一 CA）；
3. `SAN` 原文包含 `IP Address:8.140.23.253`，`san_contains_target=YES`；
4. `ip_san_match=OK`（`openssl verify -verify_ip 8.140.23.253` 返回 0；这是 IP 端点的权威判据；`-verify_hostname` 会回退比较 CN，不得单独使用）；
5. `negative_control=OK`（`-verify_ip 8.140.23.254` 不通过，证明判据未放宽）；
6. `serial_changed=YES` 且线上 serial 与部署/签发记录一致，`notAfter` 晚于执行时刻且不晚于 CA 的 2034-10-10；注意脚本在链/IP SAN/负例通过时即使 `serial_changed=NO` 也可能给出 `overall=PASS`，**执行者必须另行要求 `serial_changed=YES`**，否则视为未替换；
7. 叶子 `issuer` 为 `CN=Tiktok Root CA, O=LinkValue`；`jd-ca.crt` SHA-256 指纹仍为 `75:33:09:B3:…:15:2B`；
8. 若证书仍是旧 SAN/旧 serial：**FAIL（修复未生效或未部署）**，不是 `NOT EXECUTED`。

**证据**：`scripts/emqx_8883_cert_verify.sh` 完整原始输出（含 `RESULT` 行）、`openssl s_client` 尾部原文、`jd-ca.crt` 指纹原文、执行时间/主机/`openssl version`。

**清理**：无（只读，无临时文件留在项目内；脚本自清理临时目录）。

### 3.2 C2 — App 真实 TLS 连接与 MQTT CONNECT/CONNACK 复验（`CERT8883-CLI-02`）

**目标**：在原始失败的真实消费端上，证明 App 对 `ssl://8.140.23.253:8883` 不再出现证书主机名错误，完成 TLS 并进入 MQTT CONNECT；如实记录 CONNACK/认证结果，不把认证失败误判为证书缺陷。

**准备**：
1. 手机 `12f046690506` 在线；记录型号/API、设备侧 `base.apk` SHA-256；确认 APK 与 AB-003 一致（若实现重建，记录新 SHA-256/versionCode 并以新组合执行）。
2. 确认 `ZTE_607157` 为当前默认网络（保留系统 SSID/默认路由原文）；蓝牙/定位/权限按既有手机段条件。
3. 启动 App 守护并确认 `MqtttService` 前台运行；记录 App PID。
4. 记录 `mac_addr`/`registerphone` 实际值（掩码敏感值）；无测试身份时先记录缺口，不阻断本用例的 TLS 判定。
5. 清空/标记 logcat 时间窗起点；记录主机 C1 探测的 UTC 时间作为对照。

**刺激**：让 App 自然重连（冷启动或等待重连周期）。不得改 App 代码、不得关校验、不得用系统 `curl -k` 结果代替 App。

**观察点**：
- logcat（`MqttService`/`MqtttService`）中的 TLS/连接日志与异常栈；
- 是否出现 `SSLHandshakeException`、`No subjectAltNames on the certificate match`、`Trust anchor for certification path not found`、`PKIX path building failed`；
- 是否出现 `Connected to broker` 或 `Failed to connect` 及其异常/原因码；
- App UI 的 broker 状态（“网关：已联网”/“网关：断网”）；
- 同一窗口内网关 BLE 读取是否仍正常（不受影响的反证）。

**通过判据（本缺陷维度）**：
1. 同一时间窗内**不再出现** `No subjectAltNames on the certificate match`，也不出现链/信任锚类 TLS 错误；
2. 有证据表明 TLS 握手完成并进入 MQTT CONNECT（Paho 日志/回调/栈不再停在 TLS 校验）；
3. 记录实际 CONNACK/认证结果：
   - 若 `Connected to broker` → 记录成功（TLS + 认证均通过）；
   - 若 `Failed to connect` 且原因为认证/拒绝（如 username/password、not authorized）→ 记录为**认证层结果**，本用例对“证书不匹配缺陷”判 PASS；该认证失败另列入缺口，**不得**写成 TLS 失败，也不得据此判 P1 上传通过；
4. App UI 状态与日志一致；
5. 若仍出现 SAN/链错误 → **FAIL**，保留证书输出与完整异常栈，交回平台修复；不得复用旧证据。

> 本用例只证明“App 作为真实消费端接受了修复后的证书并进入 MQTT 层”。它不证明 MQTT 业务授权、上传、持久化或传感器数据；这些分别由 C4/P1 判定。

**证据**：`adb logcat` 切片（含时间戳/异常栈/连接结果）、UI dump 或截图（脱敏）、设备/APK 摘要、默认网络原文、与 C1 同一窗口的主机证书输出。

**清理**：保持 App/守护按用户测试目标运行；记录状态；不改网络、不卸载、不清数据。

### 3.3 C3 — 不受影响路径回归（`CERT8883-REG-03`）

**目标**：确认证书替换没有改变其他 listener、HTTP 服务、数据库端口语义，也没有为排障新增公网诊断/免校验入口；App 非 MQTT 路径未回归。

**准备/刺激（全部只读）**：
1. 主机/手机只读探测：8883（TLS，C1 已覆盖）、1883 TCP、8083/8084 TCP、18083 `GET /status`、5000 `GET /`、`GET /login`、`GET /upload_data`、3306 端口握手。
2. 不在 5000 上 POST、不登录 Dashboard/SSH/DB、不调用管理 API、不写任何数据。
3. App 启动/守护/网关读取：观察一次读取终态（`DATA_OK`/`GATEWAY_NO_DEVICE`/`GATEWAY_NO_DATA`/`READ_INCOMPLETE` 等），记录实际值；不做保护性改动。
4. 对比 `服务器迁移记录.md`/`platform_stage_retest_*` 的既有观测，列出差异。

**通过判据**：
1. 8883 仍为 TLS listener（C1）；1883 可连；18083 `/status` 返回 200 且为 `emqx is running`；5000 三个路由的 `GET` 行为与既有观测一致（`/` 404、`/login` 与 `/upload_data` 405 + `Allow: POST, OPTIONS`）；
2. 3306 返回 MySQL 握手（版本记录；不认证、不读库）；
3. 8083/8084 的观测：若仍与既有“手机侧超时”一致，记录为**既有部署差异**，不因本次证书变更判回归；若本次出现新的不可达/异常且可归因于变更，判 FAIL 并交回部署；
4. 未发现新的公网免校验/诊断端口或放宽的鉴权边界；
5. App 能启动、守护运行、网关读取进入明确终态，无新增崩溃/ANR；
6. 任何写操作、登录、POST 或 DB 直连均未发生（自证与证据）。

**证据**：只读命令原始输出（端口/HTTP/握手）、App logcat/UI、与既有基线的差异表。

### 3.4 C4 — 安卓到平台上传受理与持久化（`CERT8883-UPLOAD-04`）

**目标**：用 App **真实上传路径**证明一条可关联读数经 `POST /upload_data` 被平台受理并在 MySQL 可回读，字段一致、同一 `(dev_id,time)` 重试不新增行；不写 prefs/DB、不伪造成功、不以 HTTP 2xx 代替持久化。

**前置（缺一不可，全部满足才执行）**：
1. **隔离测试身份**：测试手机号经 App 既有注册/登录路径设置，`registerphone` 非空；不得使用 `user_table` 真实用户数据，不得编辑 SharedPreferences/数据库或伪造短信验证。
2. **已确认的平台网关 MAC**（`mac_addr`，12 位 HEX）：来自用户/网关标签/已确认资料，经 App 既有“保存网关编码”/`setMacAddress` 路径设置；不得猜测，不得把 BLE 地址直接当平台 MAC。
3. **一条合法读数来源**：App 经真实 BLE 读取到网关有效数据（`DATA_OK`），或由固件/嵌入式能力提供且经授权的受控注入读数（记录来源与版本）；不得手工写本地库/待发箱。
4. **平台写与回读授权**：允许本次 `POST /upload_data` 与按 `(dev_id,time)` 的只读回读；清理测试行需单独授权。
5. 守护运行（`measure_state=stopping`）且手机为 `ZTE_607157` 默认网络。

**刺激**：
1. 记录 App 待发箱初始状态（`pending_uploads` 行数/最近状态）与本地库行数。
2. 让 App 通过真实路径消费一条读数（BLE 读取或已授权的受控注入），触发 `enqueueAndUpload`。
3. 观察 `CloudUpload` 日志、HTTP 状态/响应、上传状态区（待发条数、HTTP 状态、是否被拒）。
4. 若成功，用授权入口按 `(dev_id,time)` 回读 `sensor_data`；再触发同一 `(dev_id,time)` 重试（若产品路径允许）核对不新增行。
5. 记录测试行是否清理或保留披露。

**通过判据（全部满足）**：
1. App 门控打开（`guardActive=true` 且 `registerphone` 非空）；日志出现真实上传尝试，载荷含 `devId`、`time`、温度、湿度；
2. HTTP 返回 2xx；**且**平台回读存在对应行 `(phone, dev_id=uid4, time, temperature, humidity)`，数值逐项一致（`36.70` 与 `36.7` 按数值相等，记录实际表示）；
3. 同 `(dev_id,time)` 重试不新增行（记录 `INSERT IGNORE`/唯一键的实际表现；若服务端返回结构不同，以回读行数为准）；
4. 未写 prefs/DB、未伪造、未绕过门控；401/403/4xx 按产品分类保留待发并可见（若出现，记录为平台受理/鉴权结果，交平台，不以重试冒充成功）；
5. 测试数据清理或保留披露。

**不满足前置时的判定**：
- `registerphone` 空或 `mac_addr` 未确认 → 记录 `upload gated off`/身份缺口，**`NOT EXECUTED`**；
- 无合法读数来源 → **`NOT EXECUTED`**（不得用占位、历史缓存或手写数据充当新读数）；
- 无平台回读授权 → HTTP 2xx 只能记“受理，未证明持久化”，**本用例不得判 PASS**；
- 上述 `NOT EXECUTED` 必须写明缺失条件、责任方与恢复后必须重做的步骤。

**证据**：App logcat（`CloudUpload`/门控/HTTP）、上传状态区截图/UI、HTTP 请求/响应（脱敏）、平台回读行原文、待发箱/本地库前后行数、身份设置路径证据（脱敏）。

### 3.5 C5 — 受影响联合用例 P0 平台段复验（`E2E-JOINT-P0`）

**目标**：在修复后的当前组合版本上，重新完成 `E2E-JOINT-P0` 中受证书影响的**平台段**（App→EMQX TLS/CONNACK、平台只读基线），并与同一用例的嵌入式/手机段身份闭合；不得沿用旧平台段证据。

**准备**：
1. 记录当前传感器构建/下载身份、`BOOT uid`、网关地址/GwID、APK SHA-256、设备 serial/API、平台服务身份；若与 `P0_phone_stage_report.md` 同版本，可引用其手机段 PASS 并重新确认设备在线/APK 摘要/默认网络/守护/网关读取就绪；若版本变化，重做受影响手机段。
2. 平台只读基线按 C3；证书按 C1；App 连接按 C2。
3. 关联键：`uid4`、APK 摘要、网关身份、时间窗。

**通过判据**：
1. 三端身份与 §0.1 当次记录一致或差异已解释；
2. 手机段（同版本）身份/网络/守护/网关绑定/0xA1/读取就绪有本轮复核证据；
3. 平台段：C1 与 C2 均通过（无 SAN 错误，TLS 完成，CONNACK 实际结果记录）；平台只读基线完成（日志/连接数/DB 回读无授权时如实 `NOT EXECUTED`，不冒充）；
4. 关联键写入同一证据索引；不使用时间接近猜事件；
5. 任一必需项失败 → 该段 FAIL；未授权项 → `NOT EXECUTED`，P0 整体不得判 PASS。

**证据**：C1/C2/C3 证据 + 当次身份/关联索引 + 手机段复核记录。

### 3.6 C6 — 受影响联合正向用例 P1 与上传四段一致（`E2E-JOINT-P1`）

**目标**：在证书修复后的当前组合版本上，验证 `E2E-JOINT-P1` 的受影响段（App→EMQX MQTT 与 App→HTTP 上传）不再被 TLS 阻断，并在条件具备时完成 `uid4+367+h` 四段一致；证书修复只是解除 TLS 阻断，不自动提供测试构建、身份或回读授权。

**前置（缺一不可）**：
1. P0 已完成（网关绑定/读取就绪、手机网络/身份状态按可得性记录）；
2. N1 或等价的干净基线已记录（若无法执行，记录实际网关缓存状态并可区分）；
3. 固件测试构建：默认关闭、仅测试启用，在 `measure_sample()` 成功之后、`sensor_decide_report()` 之前注入 `367`，产生可区分 `TINJ`（或等价）观测；正式构建排除；P1 后恢复标准构建并 `Verify OK`；由固件/嵌入式能力提供与烧录；
4. 隔离测试身份与已确认 `mac_addr`（同 C4）；
5. 平台回读授权（同 C4）。

**刺激/步骤**：按 `artifacts/e2e_joint_test_design.md` §4.3 执行（嵌入式注入/冻结/发送 → App BLE 读取/解码/本地消费/上传 → MQTT 按产品分支 → 平台受理与回读），并新增本轮的证书维度：在同一时间窗内记录 C1 证书字段与 C2 的 TLS/CONNACK 结果。

**通过判据**：完全沿用 `TD-E2E-SW-006` §4.3/§8 的四段判据；本轮补充：
1. App→EMQX 的 TLS 段必须无 SAN/链错误；CONNACK 实际结果按产品分支记录；
2. 上传段按 C4 的受理 + 行回读判据；
3. 任一段缺失/不一致 → P1 不通过；`NOT EXECUTED` 必须写明缺项；
4. 证书修复本身不使 P1 的测试构建/身份/回读缺口消失；不得把 C1/C2 通过当作 P1 通过。

**不满足前置时**：整例 `NOT EXECUTED`（写明缺项：测试构建/身份/回读/基线），保留原目标；不得部分通过。该状态不否认 C1/C2 的证书结论。

### 3.7 C7 — 本轮交付限定整体验收（`CERT8883-FINAL-05`）

**目标**：在当前组合版本上形成本轮交付的最终判定：证书修复、App TLS/CONNACK、不受影响路径回归、上传/P1 的实际状态；阶段结论与完整产品结论分开表述。

**前置**：C1–C6 已按顺序执行或已给出 `NOT EXECUTED` 判定；证据索引与版本身份齐全。

**通过判据**：
1. **身份闭合**：平台服务身份/证书 serial/SAN/指纹；`jd-ca.crt` 指纹；APK SHA-256/版本/设备摘要；传感器/网关身份；时间窗；
2. **证书维度**：C1 PASS；C2 PASS（或 FAIL 有原始证据）；
3. **回归维度**：C3 PASS（既有差异如实记录）；
4. **上传维度**：C4 的实际状态；若 `NOT EXECUTED`，本交付验收**不得判 PASS**，按 `TEST_FAIL`/`EXTERNAL_CONDITION_UNEXECUTED` 报告缺口；
5. **联合维度**：P0 复验状态；P1 的实际状态（未执行不得写成通过）；
6. **边界**：只声明“本轮当前组合版本在已执行范围内的结论”；真实物理触发、功耗、射频电气、release、iOS、9 台、API 31+ 等仍 `NOT EXECUTED`；
7. **清理/披露**：测试身份/测试行/绑定的保留或清理、凭据未泄露、未执行项逐项列出。

**结论规则**：
- 只有 C1、C2、C3、C4 全部实际通过，且 P0 通过、P1 通过（或由用户明确批准本阶段不纳入 P1）才可判本轮交付 `TEST_PASS`；
- C1/C2/C3 通过但 C4 未执行，或 P1 未执行且无用户批准 → 本轮**证书修复维度通过，交付整体未通过**（`TEST_FAIL`/`EXTERNAL_CONDITION_UNEXECUTED`），不得写“整体验收通过”；
- 任一实测缺陷（证书仍不匹配、App 仍 SAN 失败、回归失败、上传/持久化不符）→ `TEST_FAIL`，按责任交回实现/部署并复测；
- 阶段通过 ≠ 完整产品通过。

---

## 4. 异常与分支

| 分支 | 刺激/条件 | 期望 | 处置 |
|---|---|---|---|
| 证书未替换 | C1 仍取回旧 SAN/旧 serial | `overall=FAIL` / `serial_changed=NO` | 平台修复未生效；交部署/基础设施，不放宽判据 |
| SAN 命中但链不受信 | 新证书由其他 CA 签发，或 `jd-ca.crt` 被替换 | `chain_verify=FAIL` 或信任锚指纹变化 | 停止；不得以公共 CA/自签替代；按 §9 信任锚分支交用户决定 |
| 判据被放宽 | `-verify_ip 8.140.23.254` 也通过 | `negative_control=FAIL` | 说明校验/证书错误；不得接受 |
| App 仍报 SAN/链错误 | C2 logcat 复现 `No subjectAltNames`/`PKIX` | FAIL | 保留新证书输出与栈，交平台/App 责任方；不得复用旧证据 |
| App 认证失败 | CONNACK 用户名/密码/未授权 | 记录为认证层结果，TLS 维度 PASS | 不是证书缺陷；列入身份缺口；不得判 P1/上传通过 |
| 其他 listener 回归 | 1883/8083/8084/18083/5000 新异常 | 记录差异；可归因于变更则 FAIL | 交部署；不把既有 8083/8084 超时当本次回归 |
| 上传门控关闭 | `registerphone` 空/未登录 | `upload gated off`，无请求 | C4 `NOT EXECUTED`；不是 App 缺陷 |
| HTTP 2xx 无行 | 回读无对应 `(dev_id,time)` | 持久化未成立 | 交平台；2xx 不改判通过 |
| 401/403/4xx | 平台拒绝 | 保留待发、暂停自动重试、状态可见 | 记录实际分类；不伪造鉴权头；不判证书缺陷 |
| 回退发生 | 部署触发 R1–R4 并回退 | 回到已知失败基线（旧 serial/SAN） | 记录回退；证书修复未完成；不得把回退当通过 |
| APK 重建/信任锚更新 | 新 APK/新 `jd-ca.crt` | 记录新身份并重做 C2/P0 手机段 | 进入 §9；旧手机段证据作废 |

---

## 5. 执行顺序、依赖与独立判定

| 顺序 | 用例 | 依赖 | 结果单位 |
|---:|---|---|---|
| 1 | `CERT8883-SRV-01` | 部署已完成（新证书已在线） | PASS/FAIL/NOT EXECUTED |
| 2 | `CERT8883-CLI-02` | C1 通过；手机/App/网络可用 | PASS/FAIL/NOT EXECUTED |
| 3 | `CERT8883-REG-03` | C1 执行后即可；只读 | PASS/FAIL/NOT EXECUTED |
| 4 | `CERT8883-UPLOAD-04` | 测试身份 + `mac_addr` + 合法读数 + 平台回读授权 | PASS/FAIL/NOT EXECUTED |
| 5 | `E2E-JOINT-P0` | C1/C2/C3；手机段身份复确认 | PASS/FAIL/NOT EXECUTED |
| 6 | `E2E-JOINT-P1` | P0；测试构建；身份；回读；干净基线 | PASS/FAIL/NOT EXECUTED |
| 7 | `CERT8883-FINAL-05` | 上述实际状态 | 本轮交付 PASS/FAIL（含未执行） |

**独立判定**：每个用例的结论只由本文件对应判据推出；C1 通过不证明 C2；C2 的 TLS 通过不证明 CONNACK 业务授权；CONNACK 通过不证明上传；HTTP 2xx 不证明持久化；P0 不证明 P1；任一 `NOT EXECUTED` 必须写明缺失条件、责任阶段与恢复后必须重做的实测。

---

## 6. 证据规范

执行阶段建立独立目录（建议 `evidence/cert8883_<YYYYMMDD>/`；本设计不预建）：

1. **身份**：HEAD、`git status`；平台服务/端口/版本；证书 serial/SHA-256/SAN/有效期；`jd-ca.crt` 指纹；APK SHA-256/版本/设备侧摘要；设备 serial/型号/API；传感器/网关身份与时间窗。
2. **环境**：手机 Wi-Fi SSID/默认网络/IP、蓝牙/定位/权限；守护状态；网关供电/读取终态；平台只读基线；执行主机/`openssl version`。
3. **原始证据**：`emqx_8883_cert_verify.sh` 完整输出（含 `RESULT`）；`openssl s_client/x509/verify` 原文；`adb logcat` 切片（TLS/CONNACK/CloudUpload）；UI dump/截图（脱敏）；HTTP 请求/响应原文（脱敏）；平台回读行（脱敏）；端口/HTTP 只读输出。
4. **判定表**：用例 → 子项 → PASS/FAIL/NOT EXECUTED/CONDITIONAL → 证据路径 → 备注。
5. **清理**：测试身份/绑定/测试行的保留或清理声明；App/守护最终状态；无凭据泄露。
6. **未执行项**：逐项原因、缺失条件、责任阶段、恢复后必须重做的验证；不以空文件/无输出冒充。

**脱敏**：Wi-Fi 密码只经 `ui_set_text` + `text_env` 注入，不落盘/不回显；MQTT 固定后缀、SSH/DB/短信/Dashboard 凭据不复制；手机号掩码；服务器日志只保留与本用例相关的必要行；私钥/CA 私钥一律不进证据。

---

## 7. 退出准则（执行能力必须逐项回引）

1. **基线先记**：执行前记录缺陷基线（旧 serial/SAN、`jd-ca.crt` 指纹、APK 摘要）；修复后必须证明 serial 变化与 SAN 命中。
2. **C1**：`tls_handshake=OK`、`chain_verify=OK`、`san_contains_target=YES`、`ip_san_match=OK`、`negative_control=OK`、`serial_changed=YES`、issuer 与信任锚指纹符合；否则 FAIL/未执行按实际。
3. **C2**：同一 App/手机/网络下无 SAN/链错误，TLS 完成并进入 MQTT CONNECT；CONNACK 实际结果分类记录；否则 FAIL。
4. **C3**：其他 listener/HTTP/数据库端口语义未回归；无新增诊断/免校验入口；App 非 MQTT 路径无回归；既有差异如实记录。
5. **C4**：真实 App 路径 + HTTP 2xx + 平台行回读 + 幂等；缺任一前置 → `NOT EXECUTED`，不得 PASS。
6. **P0**：身份闭合 + 平台段 C1/C2 + 手机段复核；未执行项不冒充。
7. **P1**：四段全成立才 PASS；证书修复不豁免测试构建/身份/回读缺口。
8. **C7**：阶段结论与完整产品结论分开；C4/P1 未执行时不得判交付整体 PASS。
9. **不伪造**：不得用主机 `--insecure` 结果代替 App、用 2xx 代替回读、用占位/缓存代替新采样、用 C1 代替 P1、用旧证据代替当前组合。
10. **修复复测**：任一 FAIL 交对应实现/部署；修复后用当前组合版本重新采集与判定；不得改判据或删失败证据。

---

## 8. 未执行、延期与真实边界（必须原样披露）

| 项 | 状态/原因 | 本设计处理 |
|---|---|---|
| 真实物理 >35℃ 加热 | 用户明确不能调节温度 | `NOT EXECUTED`；受控注入只证明判定之后链路，不证明 GXHT40/热学响应 |
| 真实升温 >0.9℃ 且无光 | 不能调温/改光；`T=0`；无光判据启用为人工延期 | `NOT EXECUTED`；不安排、不自行回填阈值 |
| 光照受光/遮光双态 | 不能改变光强 | `NOT EXECUTED` |
| 功耗 / CR2032 | 无仪器；用户明确不能测 | `NOT EXECUTED`；不估算冒充 |
| 433 射频电气/天线/距离 | 无 433 仪器/授权 | 只证明网关功能接收；电气未证明 |
| 9 台真实快照/第 10 台拒绝 | 当前仅 1 台真实设备/1 个绑定 | 宿主逻辑可回归；真实容量 `NOT EXECUTED` |
| API 31+ 权限分支 | 无 API 31+ 设备 | `NOT EXECUTED`；API 29 结果不外推 |
| 服务端内部证书文件/CA 私钥/listener 配置 | 执行能力无 SSH/变更入口 | 由部署/基础设施能力交付证据；软件测试独立核验证书结果，不代判私钥可用性 |
| 服务日志/连接数基线 | 无只读 SSH 入口 | `NOT EXECUTED`；不因证书修复自动补齐 |
| DB 直连/清理 | 无授权 | C4/P1 回读 `NOT EXECUTED`；不直连、不写、不删 |
| 测试身份/合法读数 | 未准备 | C4/P1 `NOT EXECUTED`；不以真实用户数据替代 |
| release/生产签名包、iOS | 未产出/用户明确不改 | 只验证 debug 候选；不在范围 |
| 安卓上传四段/P1 | 需注入构建+身份+回读 | 设计保留；条件具备即执行；不具备时如实 `NOT EXECUTED` |

> 上述项不因 C1/C2/C3 通过而变成通过；阶段通过 ≠ 完整产品通过。已批准延期项不重测、不重复询问、不作为返工依据。

---

## 9. 信任锚变更分支（原 CA 不能签发时的处理，交用户决定）

**触发**：部署侧确认 `Tiktok Root CA` 私钥不可用，且无已签好且 SAN 正确的新证书；或产品决定更换信任锚。

**事实边界**：更换信任锚会使 `assets/jd-ca.crt`、APK 身份（SHA-256/versionCode）与发布范围发生变化；旧手机段证据作废。测试侧**不自行**改 App、不自行决定更换信任锚。

**测试设计（条件具备时）**：
1. 记录新 CA 证书 SHA-256/有效期、新服务器证书 serial/SAN/issuer、新 APK SHA-256/versionCode/设备侧摘要；
2. 执行 C1（以新信任锚为 CA；旧信任锚下应失败，证明未混用）；
3. 重做 C2（新 APK 在手机上的 TLS/CONNACK）与 P0 手机段；
4. 记录影响范围：APK 重装/升级、旧包回退能力、其他信任库行为、发布签名要求；若重建 APK，使用项目正式构建入口（`cd dengbei_care && ./gradlew :app:assembleDebug --offline`，以实际工程与文档为准）并保存构建日志、签名核对与 `base.apk` 摘要；
5. 本设计不要求 Android 测试专用构建。若后续为上传路径引入任何 Android 测试入口/开关，只允许存在于测试 APK，正式 APK 必须排除，并重做 APK 身份与手机段验证；
6. 结果交项目协调/用户决定；在用户批准前不得把新组合写成通过。

---

## 10. 风险与缓解

| 风险 | 影响 | 缓解 |
|---|---|---|
| 证书未真正部署/仅口头声明 | 假通过 | C1 独立取证书，不看自述；serial/SAN/链三重判据 |
| 判据被放宽（`-insecure`/`-verify_hostname` CN 回退） | 误判通过 | 强制 `openssl verify -verify_ip` + 负例；禁止 `--insecure` |
| 信任锚被替换却未披露 | 旧证据失效 | 记录 `jd-ca.crt` 指纹；变化即转 §9 |
| App 认证失败被误判为证书失败 | 责任错派 | CONNACK/异常分类；TLS 与认证分层记录 |
| HTTP 2xx 被当成持久化 | 上传假通过 | 必须行回读；无授权记 `NOT EXECUTED` |
| 占位/缓存被当成新采样 | P1/上传假通过 | 记录网关读取终态与 `receiveTime`；缓存复读单独披露 |
| APK 重建未重采 | 旧证据外推 | 记录 APK 摘要；变化即新组合 |
| 其他端口既有差异被误判回归 | 错误返工 | 与既有基线对照；只对可归因于本次变更的回归判 FAIL |
| 执行能力无串口/DB/SSH | 部分项不可执行 | 明确责任与 `NOT EXECUTED`；不伪造 |
| 测试数据/绑定残留 | 现场副作用 | 记录并披露；清理需授权；不改网关固件 |

---

## 11. 交接与最小下一步

| 项 | 内容 | 归属 |
|---|---|---|
| 证书签发与部署 | 按 `P0_backend_cert_issuance_and_rollback.md`/`P0_backend_cert_ops_runbook.md` 以原 CA 签发 SAN 含 `8.140.23.253` 的叶子证书，备份/重载/复核；回传 `RESULT` 齐全的原始输出 | `devops_engineer.infrastructure_management` / `deployment_operations` |
| CA 私钥可用性 | 只读确认 `Tiktok Root CA` 私钥或已签好证书；不可用则转 §9 | 部署/基础设施；用户决定信任锚 |
| 可靠性检查 | 变更后 listener/服务状态、其他端口/Flask 回归、回退路径 | `devops_engineer.reliability_operations` |
| C1/C2/C3 独立验证 | 证书字段/链/IP SAN/负例；App TLS/CONNACK；回归 | `software_tester.software_verification`（本设计） |
| C4 测试身份/读数/回读 | 隔离测试手机号 + 已确认 `mac_addr` + 合法读数 + 写/回读授权 | 用户/手机段/平台段；条件具备后执行 |
| P1 测试构建 | 默认关闭的注入宏/构建目标、可区分 `TINJ`、正式包排除、构建/摘要与恢复 | 固件实现/嵌入式能力 |
| 平台只读基线/DB 回读 | 只读 SSH 日志/连接数；如获准则按 `(dev_id,time)` 回读 | 平台段执行能力（需入口授权） |
| 信任锚变更影响报告 | 实际核查证据、缺项、影响范围，交用户决定 | `project_manager.failure_resolution`（按流程） |

**最小下一步（按顺序）**：
1. 部署能力核实原 CA 私钥/已签证书，按 §4–§6 签发/替换/重载，回传执行记录；不可用则停止并转 §9。
2. 软件测试执行 C1（证书独立核验）→ C2（App TLS/CONNACK）→ C3（回归），记录同一时间窗证据。
3. 条件具备时执行 C4（上传受理+回读）与 P0/P1；不具备时如实 `NOT EXECUTED` 并交缺口。
4. 执行 C7，按 §7 形成本轮交付结论；未执行项逐项披露。
5. 任一 FAIL 交对应实现/部署，修复后以当前组合版本复测；不改判据。

---

## 12. 自检

- [x] 修复对象与判据与原始失败/归因一致：服务器证书 SAN 与客户端地址不匹配；不把 App 默认主机名校验判为缺陷。
- [x] 正常（证书修复/L1/L2/上传有数据）与异常（未替换、链不受信、负例失败、仍 SAN 失败、认证失败、端口回归、2xx 无行、回退、APK 重建）均有设计。
- [x] 每个用例写清准备、刺激、观测、判据、证据与清理；验证顺序为证书→App→回归→上传/P0/P1→整体。
- [x] 真实环境优先：真实 App、真实手机、既有平台、既有网关；受控注入只用于不可自然产生的输入并明确边界；不以模拟/桩冒充真实链路。
- [x] 未安排调温、改变光强、遮光、功耗测试；人工延期项不重测、不作为返工依据。
- [x] 测试身份、平台回读、注入构建、串口/DB/SSH 入口等缺口逐项列出，未把缺条件必需项豁免为通过，也未把未测项写成通过。
- [x] `DESIGN_READY` 不表示任何用例通过；阶段结论与整体产品结论分开表述。
- [x] 本文件只做设计；未修改产品源码/固件/网关/服务/权限/环境，未执行任何设备或平台操作。

---

## 13. 停止条件

本能力在写出本文件、`artifacts/cert8883_e2e_plan.md`、`artifacts/cert8883_cases.yaml` 并同步 `file_manifest.txt` 后结束，返回 `DESIGN_READY`。证书签发/部署、App/固件/网关修改、测试身份准备、平台写/回读、设备操作与最终实测均不属本能力；本能力只给方法与判据。若执行阶段发现方案内部矛盾，先修本方案；不能消除时按失败结果报告，不要求实现修改业务来迎合错误判据。
