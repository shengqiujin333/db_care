# CERT8883-UPLOAD-04 上传 401 失败分析（软件归因）

- 负责角色：软件测试
- 能力：软件失败分析（`software_tester.software_failure_analysis`）
- 原始失败：`artifacts/software_test_report_cert8883.md` 中 **CERT8883-UPLOAD-04 FAIL（实测 HTTP 401）**，并导致 **CERT8883-FINAL-05 TEST_FAIL**
- 被测对象：
  - 平台：既有服务器 `8.140.23.253`，Flask `5000`（`/root/abr.py`，`user_script.service`，PID `214002`，8949 B，inode `1088447`，mtime `2026-10-22 19:31:04`）
  - 客户端：Android App `com.jinyuni.dengbei_care` vC10 / vN1.9，设备侧 `base.apk` SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`
  - 测试身份：模拟手机号 `14000000001`、合成测试 MAC `0200000000A1`（原始测试报告 §5）
- 分析时点：2026-11-25T09:35Z ~ 09:42Z（CST 17:35 ~ 17:42）
- **本轮结论：`REPAIR_REQUIRED`（对象：既有平台服务的 `POST /upload_data` 受理逻辑）**

> 本文件只做失败归因与责任边界判断，不是复验通过、不是修复，也不代表 UPLOAD-04、P1 或产品整体通过。
> 本轮未修改产品源码、测试期望、服务、数据库、权限或环境配置；未重部署、未重启、未连接数据库、未再次调用写接口。
> 本轮对既有平台的访问仅为**只读**：`systemctl show`、`stat`、读取部署脚本与备份脚本。输出中的口令类值已脱敏，未回显任何凭据。

---

## 1. 结论摘要

1. **401 的直接机制已由同版本部署对象证实，不再是竞争假设。** 部署中的 `/root/abr.py` 在 `upload_data()` 中先做：

   ```python
   if request.headers.get('X-Upload-Token') != UPLOAD_TOKEN:
       return jsonify({"error": "unauthorized"}), 401
   ```

   `UPLOAD_TOKEN` 是服务端硬编码的 48 字符常量（非环境变量）。App 的 `RetrofitUploadApi` 只发送 `Content-Type: application/json`，`CloudApiClient` 的拦截器注入点为 `null`，**不发送 `X-Upload-Token`**，因此每次上传都在进入业务校验和数据库写入之前被拒。
2. **该要求与已批准的接口契约冲突。** `artifacts/interface_contract.md`（IC-002 v4.0）§8 明确写明：`POST /upload_data`、`Content-Type: application/json`，“**不增加认证头、token 用法**、响应字段或表结构”。`服务器迁移记录.md` §八的请求体契约同样只有 `phone/mac/readings`。App 的实现与这两个文档一致，未偏离。
3. **责任对象是平台侧部署的受理逻辑，不是 App、手机、网关或证书。** 失败发生在 Flask `5000` 的应用层 401 分支；8883 TLS/MQTT、BLE 读取与 HTTP 端口可达性均与本次失败无因果关系。原始测试报告中的 H2（网关 MAC 白名单）与 H3（短信验证状态）被部署源码直接排除：该处理器**完全不读取 `mac`**，也不读取任何验证码状态。
4. **这是服务端后加的访问控制与既有客户端契约之间的不一致。** 平台备份 `/root/abr.py.bak_20260926`（199 行）不含 `X-Upload-Token`/`UPLOAD_TOKEN`；当前部署对象（254 行）含该检查。当前 mtime 与 2026-10-22 19:32:28 的 `401/401/200` 历史日志吻合，支持“该检查在 2026-09-26 上传功能上线之后被加入”的判断（加入动机未见变更记录，保留为未知）。
5. **影响：所有真实 App 上传都会被 401 拒绝，云端持久化不可达。** 原始测试中真实 App 上传 2 次、同身份直连 1 次、未注册身份 1 次均为 401；空 JSON 为 400（与代码检查顺序一致）；无 `abr.py:222 mysql.connect()` 行，证明 401 在数据库写入之前。CERT8883-UPLOAD-04 不能判 PASS，E2E-JOINT-P1 平台段被阻断，FINAL-05 交付整体不能判 PASS。
6. **结果判断：`REPAIR_REQUIRED`。** 修复对象为既有平台 `/root/abr.py` 的 `upload_data()` 受理条件，责任领域为平台后端/部署（图中平台失败分支声明的处理能力为 `backend_engineer.backend_integration_debug`）。最小、与现行契约一致的修复是移除/停用该 header 门禁；若产品决定保留门禁，则必须走“修改 IC-002 + 更新 App + 重建 APK + 重新采集身份”的完整契约变更路径，不能在客户端临时塞入共享 token 绕过。

---

## 2. 对象身份与版本闭合

| 项 | 本轮实测 | 含义 |
|---|---|---|
| 平台主机 | `iZ2zeih0gmrk47og5r4z7sZ`（8.140.23.253），Ubuntu 24.04 | 与原始测试一致 |
| 服务单元 | `user_script.service` `ActiveState=active`、`SubState=running`、`NRestarts=0`、`ExecMainPID=214002`、`ExecMainStartTimestamp=2026-11-15 06:42:51 CST` | 原始失败（2026-11-25 16:21–16:38 CST）发生的同一进程仍在运行 |
| 部署对象 | `/root/abr.py`，`size=8949`，`mtime=2026-10-22 19:31:04`，`inode=1088447` | 与 `docs/05`、IC-002 §9 记录的部署对象元数据一致 |
| 源码规模 | Python 读取 254 行、8703 字符（文件 8949 字节，UTF-8 中文造成字节数差异） | 本轮实际读取的是运行中的部署脚本 |
| 服务端 token | `UPLOAD_TOKEN` 已定义、`dynamic=False`（硬编码字面量）、长度 48、SHA-256 前 8 位 `fa25db12`；文件内 `X-Upload-Token` 引用 2 处 | 说明是真实的固定共享密钥门禁；**未读取、未记录其值** |
| 备份对比 | `/root/abr.py.bak_20260926`：199 行、无 `X-Upload-Token`、无 `UPLOAD_TOKEN` 常量 | 该门禁不在 2026-09-26 上传功能初始版本中 |
| App | vC10/vN1.9，`ApiService.uploadData` 仅声明 `Content-Type: application/json`；`CloudApiClient.interceptorInjectionPoint = null` | 客户端不发送任何认证头/token（符合 IC-002） |
| 接口契约 | IC-002 v4.0 §8：“不增加认证头、token 用法、响应字段或表结构” | 服务端行为偏离已批准契约 |

> 版本闭合：原始测试的 401 响应来自 PID `214002`（见原始报告 §5.3 与 `evidence/software_verify_cert8883_r2_20261125/33_platform_http_log_upload_window.txt`）；本轮读取的是同一 PID 正在运行的同一 inode/大小/mtime 的脚本。因此源码证据与失败版本闭合，不是从其他版本外推。

---

## 3. 原始失败与复现事实（引用原始证据）

| 时间（CST） | 请求 | 实测 | 来源 |
|---|---|---|---|
| 16:21:31 | App 真实上传（1 条 BLE 读数） | `POST /upload_data` → **401** | 原始报告 §5.2；`33_platform_http_log_upload_window.txt` |
| 16:27:17 | 同账号/同 MAC/同读数直连探针 | **401** `{"error":"unauthorized"}` | `28_upload_direct.hdr/body` |
| 16:28:57 | 空 JSON 直连探针 | **400** `{"error":"No json data"}` | `29_upload_empty.hdr/body` |
| 16:28:57 | 未注册手机号 + 不同 MAC | **401** `{"error":"unauthorized"}` | `30_upload_unregistered.hdr/body` |
| 16:38:31 | 用户界面“重试上传”（5 条） | **401** | `32_prefs_after_retry.xml`、`35_ui_upload_401_status.xml` |

- 同一窗口 5 次 `POST /upload_data`，**零个 200**；无 `abr.py:222` 的 `mysql.connect()` 行，证明 401 在数据库路径之前。
- App 侧 401 后保留待发箱、暂停自动上传、界面显示“云端上传被拒绝（HTTP 401），数据已保留（待发 5 条）”，与 `CloudUploadRepository` 的 `AuthRejected` 分类一致，未发现 App 崩溃、丢数据或绕过门控。

---

## 4. 决定性证据：部署处理器源码（已脱敏）

`/root/abr.py` 第 201–213 行（只读摘录）：

```python
201| @app.route('/upload_data', methods=['POST'])
202| def upload_data():
...
208|     data = request.get_json()
209|     if not data:
210|         return jsonify({"error": "No json data"}), 400
211|
212|     if request.headers.get('X-Upload-Token') != UPLOAD_TOKEN:
213|         return jsonify({"error": "unauthorized"}), 401
214|
215|     phone = data.get('phone')
216|     readings = data.get('readings')
...
222|     conn = mysql.connect()
```

判读：

1. **空 JSON 400 先于 token 检查**，与实测“空 JSON → 400”完全一致。
2. **token 检查先于 `phone`/`readings` 校验与 `mysql.connect()`**，与实测“已注册与未注册手机号同为 401、无数据库告警行”完全一致。
3. 处理器**没有读取 `data.get('mac')`**，也没有任何网关白名单/用户验证状态查询；因此 H2/H3 不成立。
4. 该函数只有一个 401 分支，且返回体与实测 `{"error":"unauthorized"}` 逐字一致；不存在代理/WAF 参与的必要。

---

## 5. 竞争假设判定

| # | 假设 | 判定 | 依据 |
|---|---|---|---|
| H1 | 平台要求额外凭证/请求头（原始报告 H1） | **成立（直接原因）** | 部署源码第 212 行要求 `X-Upload-Token` 等于服务端常量；App 不发送该头；响应体与检查顺序逐项吻合 |
| H2 | 平台要求已确认的网关 MAC/白名单（原始报告 H2） | **排除** | `upload_data()` 不读取 `mac`；401 在任何身份/载荷校验和数据库连接之前返回；合成 MAC 与真实 MAC 的差异不影响该分支 |
| H3 | 模拟账号缺少短信验证状态（原始报告 H3） | **排除** | `upload_data()` 不检查 `verification_codes`；`/verifyCode` 的状态只被 `verify_code()` 自身使用 |
| H4 | 8883 证书变更引入回归 | **排除** | 上传走明文 HTTP 5000；8883 TLS 与 MQTT CONNACK 在原始报告第 1/2 轮均正常；401 是 Flask JSON 响应 |
| H5 | App 实现缺陷（少发必要头） | **不成立** | App 与 IC-002 §8、`服务器迁移记录.md` §八一致；契约明确禁止增加认证头/token；不能在客户端临时加头绕过 |
| H6 | 代理/WAF/网关设备返回 401 | **排除** | 响应由 `Werkzeug/3.1.8` 直接返回 JSON；代码分支与顺序可解释全部观测（400/401） |
| H7 | 测试身份/密码错误导致 401 | **排除** | `/login` 200；未注册手机号同为 401；token 检查不查询用户表 |
| H8 | 服务端 token 为环境变量、部署方可能已配置 | **不成立** | 常量为硬编码字面量（`dynamic=False`），不读取环境变量；客户端仍不发送该头 |

**尚未确定（不影响责任对象）**：该 token 门禁的加入动机、维护责任人与变更记录。平台备份证明它不属于 2026-09-26 初始上传契约；是否出于防滥用目的，需由平台/产品负责人说明。该未知不改变“当前部署对象与现行契约和客户端不一致”的实测结论。

---

## 6. 责任边界与结果判断

### 6.1 主责对象（REPAIR_REQUIRED）

| 项 | 内容 |
|---|---|
| 对象 | 既有平台 `8.140.23.253` 的 Flask 服务（`user_script.service`，`/root/abr.py`）中的 `POST /upload_data` 受理逻辑 |
| 对象身份 | PID `214002`；文件 8949 B、inode `1088447`、mtime `2026-10-22 19:31:04`；运行中、未重启 |
| 故障机制 | 处理器要求请求头 `X-Upload-Token == UPLOAD_TOKEN`；App 按 IC-002 只发送 `Content-Type`，不发送该头，故在 `phone`/`readings` 校验与数据库写入之前返回 401 |
| 证据 | 部署源码第 201–213 行（只读摘录）；`UPLOAD_TOKEN` 为 48 字符硬编码常量；原始测试 5 次实际请求（401/401/400/401/401）；App `ApiService`/`CloudApiClient` 静态核对；IC-002 §8 契约；2026-09-26 备份无该检查 |
| 影响范围 | 所有真实 App 上传被拒；`sensor_data` 无受理行；CERT8883-UPLOAD-04 FAIL；E2E-JOINT-P1 平台段不可执行；CERT8883-FINAL-05 整体 TEST_FAIL |
| 修复责任领域 | 平台后端/部署；图中平台失败分支声明的处理能力为 `backend_engineer.backend_integration_debug`。实际变更需要平台变更授权与维护窗口，本轮分析能力不代改服务 |
| 复验目标 | 同一 App 真实上传路径、同一测试身份；`POST /upload_data` 返回 2xx；在获准的回读入口按 `(phone, dev_id, time)` 核对 `temperature`/`humidity`；同一 `(dev_id,time)` 重试不增行；同时保持 8883 证书与不受影响路径结论不变 |

### 6.2 修复选项（供实现/产品决策，测试不代改）

- **方案 A（最小、与现行契约一致）**：移除或停用 `upload_data()` 中的 `X-Upload-Token` 检查，保留 `phone`/`readings` 校验与 `INSERT IGNORE` 幂等写入；不改变 App、不改 APK 身份。适用于 token 门禁没有正式产品需求的情况。
- **方案 B（若 token 门禁是正式安全需求）**：将认证方式作为正式契约变更：更新 IC-002（明确请求头/凭证来源与轮换）、更新 App 客户端、重建并记录新 APK 身份（SHA-256/versionCode），再重跑 UPLOAD-04 与 P1。**不得**把服务端 48 字符常量直接硬编码进 APK 作为“共享密钥”而不做契约评审；这会引入可提取的凭据并改变客户端身份。
- **不可接受**：仅在客户端拦截器临时加 `X-Upload-Token` 以让测试变绿；或把 401 当作“测试身份不足”而跳过。

### 6.3 App / 手机 / 网关 / 证书

| 对象 | 判断 | 依据 |
|---|---|---|
| Android App | **本次 401 不归因于 App 缺陷** | 与 IC-002 §8 和迁移记录契约一致；不发送认证头是契约要求；401 后的保留/暂停/手动重试行为符合设计 |
| 手机/Wi-Fi | **正常** | 原始测试：默认网络 `ZTE_607157`、HTTP `/register` `/login` `/setmac` 均 200 |
| 网关/BLE | **正常** | 原始测试：真实 BLE `DATA_OK`（`6A002C00`，18.0℃/35.8%），上传载荷来自真实读数 |
| 8883 证书 | **与本次失败无关** | 上传走 HTTP 5000；证书维度在原始报告第 1/2 轮独立 PASS；本轮未重测证书 |
| 数据库 | **未被触达** | 401 在 `mysql.connect()` 之前；未直连数据库 |

---

## 7. 影响范围与未执行/未知

### 7.1 影响

1. **功能**：当前部署下所有 App 上传都会 401；云端温湿度持久化不可用。App 本地待发箱保留数据，不丢数据，但不会自动恢复，除非用户手动重试（仍会 401）。
2. **测试**：UPLOAD-04 不能判 PASS；P1 的平台受理/回读段被阻断；FINAL-05 不能判交付整体 PASS。
3. **非影响**：8883 TLS/MQTT、BLE 读取、注册/登录/设置网关编码、端口与 Flask 路由存在性；本轮不把这些通过项外推为上传通过。
4. **安全观察**：服务端使用单一硬编码共享 token；若走方案 B，需要在契约中定义可轮换、不可从客户端轻易提取的机制，而不是把常量复制进 APK。

### 7.2 未执行 / 未知

- **未再次 POST `/upload_data`**：本轮为失败分析，不持有写接口授权；直接使用原始测试已采集的 5 次实测请求 + 部署源码证据，且响应体/检查顺序/“无 DB 行”三者互相印证。
- **未连接/查询 MySQL**：无数据库授权；因此本次不声称 `sensor_data` 的行数或字段，只声称“401 发生在写入之前”。
- **未重部署、未重启、未修改服务**：修复需要平台变更授权与维护窗口，属于后续修复能力。
- **未读取 token 值、未读取数据库口令、未回显任何凭据**：仅报告 token 的存在性与长度/摘要前缀。
- **token 门禁的加入动机与责任人未知**：无变更记录；备份只证明其不在 2026-09-26 初始版本中。
- **修复后回读入口仍未确定**：UPLOAD-04 的持久化回读需要获准的只读查询或接口；本轮未取得该授权，修复复验时必须先补齐。
- 真实物理条件（>35℃、升温+无光、光强、功耗、433 电气）、release 签名、iOS、API 31+ 等仍不在本轮范围。

---

## 8. 最小下一步与复验判据

### 8.1 修复侧（平台后端/部署）

1. 取得平台变更授权与维护窗口；确认 token 门禁是否为正式需求。
2. 若按方案 A：移除/停用 `X-Upload-Token` 检查，保留现有请求体校验与 `INSERT IGNORE`；备份原文件并记录回退点。
3. 若按方案 B：先更新 IC-002 与 App 契约，再实现可轮换的认证方式；重建 APK 并记录新身份。
4. 修复后由修复能力自证：同一请求体、同一测试身份，`POST /upload_data` 返回 2xx；服务端日志出现 `mysql.connect()`/写入路径；不得以口头声明代替。

### 8.2 测试侧（原用例复验）

1. 使用同一 App 真实上传路径（或方案 B 下的新 APK 身份）与同一测试身份，重跑 CERT8883-UPLOAD-04。
2. 通过判据：
   - `POST /upload_data` 2xx，且 App 待发箱对应行被 ack 删除；
   - 在获准的只读入口按 `(phone, dev_id, time)` 回读到对应行，字段数值一致；
   - 同一 `(dev_id, time)` 重试不新增行；
   - 8883 证书维度与 REG-03 回归不被本次修复改变。
3. 若仍为 401/4xx/5xx：保留请求时间、响应码/体与平台日志，交回责任能力；不得以“测试账号不足”或“证书已修复”解释。

### 8.3 未确定项与最小判别实验

| 项 | 状态 | 最小下一步 |
|---|---|---|
| token 门禁是否为正式需求 | 未知 | 由平台/产品负责人说明；无记录时按方案 A 处理 |
| 修复后持久化回读入口 | 未授权/未确定 | 取得只读查询或接口授权；否则 UPLOAD-04 仍不能判 PASS |
| 是否存在其他服务端访问控制（如 rate limit） | 未观察到 | 修复后以真实上传与少量重试观察响应码；不预先假设 |
| 服务端 `/login` 返回静态 `some-jwt-token` | 已观察，非本次根因 | 属相关设计观察，交产品/平台评估；不据此改动客户端 |

---

## 9. 边界声明与脱敏

- 本轮只读访问既有平台：`systemctl show`、`stat`、读取 `/root/abr.py` 与其 2026-09-26 备份；未使用 `sudo`，未写入文件，未启动/停止/重启服务，未连接数据库，未调用写接口。
- 证据文件保留的是脱敏后的处理器摘录；token 只记录存在性、长度与摘要前缀，未记录值；未记录 SSH/DB 口令或短信密钥。
- 本文件不代表修复完成或复验通过；`REPAIR_REQUIRED` 仅表示责任对象与故障机制已由实测证据确认，后续实现/部署与独立复测仍需执行。

---

## 10. 证据索引

| 文件 | 用途 |
|---|---|
| `artifacts/software_test_report_cert8883.md` | 原始失败报告（UPLOAD-04 FAIL、FINAL-05 TEST_FAIL、5 次 401 与竞争假设） |
| `evidence/software_verify_cert8883_r2_20261125/33_platform_http_log_upload_window.txt` | 平台 HTTP 只读日志窗口（PID 214002，16:21–16:38，5 次 POST） |
| `evidence/software_verify_cert8883_r2_20261125/28_upload_direct.hdr/.body` | 同身份直连探针 401 原始响应 |
| `evidence/software_verify_cert8883_r2_20261125/29_upload_empty.hdr/.body` | 空 JSON 400 原始响应（印证检查顺序） |
| `evidence/software_verify_cert8883_r2_20261125/30_upload_unregistered.hdr/.body` | 未注册身份 401 原始响应 |
| `evidence/software_verify_cert8883_r2_20261125/41_test_run_notes.txt` | 测试身份、真实读数、上传结果与残留状态 |
| `evidence/software_triage_cert8883_upload401_20261125/01_abr_upload_handler_redacted.txt` | 本轮只读 SSH 原始输出（服务身份、部署对象元数据、脱敏处理器摘录、token 元数据、备份对比） |
| `evidence/software_triage_cert8883_upload401_20261125/read_upload_handler_ssh.py` | 本轮可复跑的只读 SSH 探针 |
| `evidence/software_triage_cert8883_upload401_20261125/check_no_secret_leak.py` | 本地脱敏检查（仅统计，不输出值） |
| `artifacts/interface_contract.md` IC-002 §8 | 已批准契约：`/upload_data` 不增加认证头/token |
| `服务器迁移记录.md` §八 | 上传功能请求体契约与初始验证记录 |
| `docs/05_平台服务器配置与联调前提.md` §4/§7 | 历史 401 现象与“不在客户端临时加鉴权头”的既有约束 |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/ui/zhuce/ApiService.kt` | App 上传接口仅声明 `Content-Type` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt` | 拦截器为空实现、401 分类/保留/暂停逻辑 |
