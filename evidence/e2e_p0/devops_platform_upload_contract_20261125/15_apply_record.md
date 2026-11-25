# 平台 `/upload_data` 受理条件修复执行记录（2026-11-25 21:58–22:01 CST）

- 负责角色：运维工程
- 能力：基础设施管理
- 目标对象：既有平台 `8.140.23.253`（`iZ2zeih0gmrk47og5r4z7sZ`），`user_script.service`（Flask 5000），部署文件 `/root/abr.py`
- 交付入口：`deploy/platform_upload_contract/container.yaml`
- 上游依据：`artifacts/software_failure_analysis_cert8883_upload401.md`（`REPAIR_REQUIRED`）、
  `artifacts/software_test_report_cert8883.md`（UPLOAD-04 FAIL）、`evidence/e2e_p0/devops_8883_cert_20261125/DEPLOYMENT_EVIDENCE.md`（8883 已修复）

## 1. 授权链（本轮变更的依据与范围）

| 项 | 内容 |
|---|---|
| 项目目标 | 处理 8883 证书与客户端地址不匹配问题；修复后复测安卓到平台上传及受影响端到端用例 |
| 上一轮询问 | 2026-11-25 本能力向用户提出 A/B/C：A=移除 `X-Upload-Token` 门禁；B=保留门禁走契约变更+App 改造；C=仅记录不修 |
| 用户答复 | **“选择A”**（2026-11-25，本轮同一任务的人工回复） |
| 解释 | 用户作为该生产服务与安全策略的决策人，明确选择最小修复方案 A：删除 4 行门禁，保留请求体校验，不改 App / 不改 APK 身份 |
| 授权范围 | 仅 `/root/abr.py` 的 `upload_data()` 门禁移除与 `user_script.service` 重启；不含数据库直连、不含其他 listener/证书、不含重装或新建服务、不含限流策略实现 |
| 维护窗口 | 用户未指定具体窗口；以当时实际观测为准：16:38 之后至变更前无真实上传请求（`09_postfix_state.txt` 日志窗口），中断为一次服务重启（同秒内 `OPTIONS` 恢复 200） |

## 2. 变更前状态（执行前同一分钟内实测）

| 项 | 值 |
|---|---|
| 服务 | `user_script.service` active/running，PID 214002，NRestarts=0，启动 2026-11-15 06:42:51 CST |
| 文件 | `/root/abr.py` 8949 B、mode 644、inode 1088447、mtime 2026-10-22 19:31:04 |
| 摘要 | sha256 `534dd40aac40c00597b06710069cbcc70533650e01318e0637cd924851740c63`（与失败测试所用版本一致） |
| 行为基线 | `POST /upload_data {"probe":1}` → **401** `{"error":"unauthorized"}`；`{}` → 400 `No json data`；GET → 405 |
| 结构 | 254 行；门禁为 1 行注释 + 1 行 `UPLOAD_TOKEN` 常量 + `if`/`return` 两行 |

## 3. 实际变更（逐字节受控）

- 执行脚本：`deploy/platform_upload_contract/apply_remote.sh --confirm`（先 dry-run 复核，再落盘）
- 服务器工作目录：`/root/rd_upload_contract_20261125T135855Z/`（含 `apply.log`、脚本副本、`BACKUP_SHA256SUMS.txt`，保留为变更记录）
- 备份：`/root/abr.py.bak_20261125T135902Z`，`600 root:root`，sha256 `534dd40a…0c63`（= 变更前摘要）
- 补丁自检：`RESULT=PATCHED removed=4 lines_after=250`，先 `py_compile`（服务解释器 `/root/venv/bin/python`）通过后原子替换
- 变更后摘要：sha256 `a062b05f524dc8f6bce596e4571d339f697ef909b318503ad418ee0add24017c`，8688 B，250 行

结构差异（`10_apply_diff.txt`，令牌值已脱敏）：

```diff
@@ -18,6 +18,4 @@
-# 传感器数据上传共享密钥(App 需在请求头携带 X-Upload-Token)
-UPLOAD_TOKEN = "<token-value-redacted>"
@@ -208,4 +206,2 @@
-    if request.headers.get('X-Upload-Token') != UPLOAD_TOKEN:
-        return jsonify({"error": "unauthorized"}), 401
```

仅此 4 行被删除；`request.get_json()`、`No json data` 400、`phone`/`readings` 校验、`INSERT IGNORE` 与 `mysql.connect()` 位置未动（`09_postfix_state.txt` handler 结构：200 `def upload_data` / 206 `get_json` / 211 `phone` / 212 `readings` / 218 `mysql.connect`）。

## 4. 变更后核验（服务器侧 + 公网侧）

| 判据 | 实测 | 结论 |
|---|---|---|
| 单元恢复 | `active/running`，PID 312368，NRestarts=0，启动 2026-11-25 21:59:02 CST | PASS |
| 门禁移除 | 本机与公网 `POST {"probe":1}` 均由 401 变为 **400 `phone is required`** | PASS |
| 请求体校验保留 | `POST {}` 仍 400 `No json data` | PASS |
| 路由形状 | GET `/upload_data` 405、OPTIONS 200、GET `/login` 405 | PASS |
| 未触达数据库 | 探针在 `phone` 校验处返回；日志无 `mysql.connect()`/DB 告警 | PASS（未连接、未写库） |
| 端口面 | 1883/8883/8083/8084/18083/5000/3306 全部 OPEN（公网 TCP connect） | PASS |
| 8883 证书未受影响 | 叶子 serial `…B83`、SAN `8.140.23.253,117.72.84.210`、issuer `Tiktok Root CA`、notAfter 2028-11-24；用 App 信任锚 `jd-ca.crt` 实握手链校验 OK；错误主机名仍被拒（verify_code=62） | PASS |
| 其他本地客户端 | `/root` 下除备份与本轮补丁脚本外，无脚本引用 `X-Upload-Token`；`abr.py` 内引用数 0 | PASS（移除不破坏其他本机客户端） |
| 真实 App 上传 2xx + 落库回读 | **NOT EXECUTED**（属软件测试能力；本能力未写任何业务数据） | 待下游 |

原始输出：`07_apply_dryrun.log`、`08_apply_execution.log`、`09_postfix_state.txt`、`10_apply_diff.txt`、
`11_external_postfix_probe.txt`、`12_artefact_check.txt`、`13_cleanup.txt`、`14_impact_scope_and_final_state.txt`。

## 5. 中断与清理

- 一次 `systemctl restart user_script.service`（21:59:02），同秒内 `OPTIONS /upload_data` 返回 200；无重复重启、无 `NRestarts` 增长。
- 补丁 `py_compile` 产生的 `/root/__pycache__/abr_patch_gqo8a3lg.cpython-312.pyc` 已删除；既有 `abr.cpython-312.pyc`（2026-10-22）保留未动（`13_cleanup.txt`）。
- 服务器上未安装任何软件、未改防火墙、未改其他 unit、未改证书与配置。

## 6. 回退（已具备，未做生产演练）

```bash
# 任选其一
bash /root/rd_upload_contract_20261125T135855Z/rollback_remote.sh \
     --backup /root/abr.py.bak_20261125T135902Z
# 或手工
cp -a /root/abr.py.bak_20261125T135902Z /root/abr.py && systemctl restart user_script.service
```

- 回退后即回到**已知失败**基线（`POST {"probe":1}` → 401），不得计为健康。
- 未在生产演练回退：备份摘要与变更前逐字节一致（`534dd40a…`），回退脚本逻辑已在离线自测覆盖（`05_patch_selftest.txt`），且 `apply_remote.sh` 的失败路径会在后置核验不通过时自动回退。演练需再中断两次 5000，未获该额外窗口授权。

## 7. 残留风险与后续建议（不由本能力自行实施）

1. **安全面**：`/upload_data` 现恢复为契约声明的“无认证写入”，5000 对公网开放，任何可达客户端都可写 `sensor_data`（`INSERT IGNORE` 只防重复，不防灌入）。建议由产品/安全决策后续加限流、来源白名单或正式鉴权（如走方案 B 的契约变更），本能力未实施。
2. **令牌常量已从部署对象移除**：服务端不再持有该共享密钥；备份文件仍包含原值（`600`，仅 root 可读）。
3. **真实业务闭环未验证**：`sensor_data` 是否按 `(phone, dev_id, time)` 落库、App 待发箱是否 ack 清空、重放是否幂等，须由软件测试用同一 APK/同一手机执行；持久化回读仍需只读查询授权。
4. 该修复不改变 8883 证书维度结论，也不把 8883 的 PASS 外推为上传 PASS。
