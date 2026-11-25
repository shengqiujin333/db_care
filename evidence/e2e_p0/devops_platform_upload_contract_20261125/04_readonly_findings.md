# 修复初次检查发现的基础设施问题 —— 只读核查记录（2026-11-25）

- 负责角色：运维工程（基础设施管理）
- 能力边界：只做基础设施核查与可复现配置制作；本轮**未修改**平台文件、**未重启**服务、**未连接数据库**、**未发送可写入的 POST**。
- 目标对象：既有平台 `8.140.23.253`，`user_script.service`（Flask 5000），部署对象 `/root/abr.py`
- 上游依据：`artifacts/software_failure_analysis_cert8883_upload401.md`（REPAIR_REQUIRED）、
  `artifacts/software_test_report_cert8883.md`（UPLOAD-04 FAIL / FINAL-05 TEST_FAIL）
- 交付入口：`deploy/platform_upload_contract/container.yaml`

## 1. 本次（重试后的）只读核查事实

| 项 | 实测值 | 判读 |
|---|---|---|
| 主机/时间 | `iZ2zeih0gmrk47og5r4z7sZ`，2026-11-25T10:13:47Z（CST 18:13:47），up 33 天 20:34 | 与历史一致 |
| 服务单元 | `user_script.service` ActiveState=active SubState=running，NRestarts=0，ExecMainPID=214002，启动于 2026-11-15 06:42:51 CST | 未重启；启动晚于脚本 mtime，运行的就是磁盘这一版 |
| 部署对象 | `/root/abr.py` 8949 B、mode 644、inode 1088447、mtime 2026-10-22 19:31:04、sha256 `534dd40a…0c63` | 与失败测试所用版本一致（同 inode/大小/mtime） |
| 上传门禁 | 第 21 行硬编码 `UPLOAD_TOKEN`（48 字符，值未复制）；第 212–213 行要求 `X-Upload-Token` 相等，否则 401 | 缺陷仍原样存在 |
| 备份 | `/root/abr.py.bak_20260926`（7048 B）、`abr_bkp.py`（7048 B）、`abr_20241221.py`（8416 B） | 2026-09-26 版本不含该门禁（与归因一致） |
| 路由形状 | GET /upload_data=405，OPTIONS=200，GET /=404，GET /login=405 | 与缺陷基线一致 |
| 8883 | `ssl:default` running=true、current_conn=0；叶子 CN=8.140.23.253、issuer=Tiktok Root CA、serial `…B83`、SAN 含 8.140.23.253 与 117.72.84.210、有效至 2028-11-24 | 证书无漂移，本次问题与证书无关 |
| 今日上传请求 | 5 次 `POST /upload_data`（16:21:31 / 16:27:17 / 16:28:57×2 / 16:38:31），码 401/401/400/401/401，零 200；18:13 前无新请求 | 行为与 401 门禁检查顺序一致 |
| 端口复查（10:23:33Z） | 1883/8883/8083/8084/18083/5000/3306 全部 OPEN（7/7，仅 TCP connect） | 服务面未变化，非网络可达性问题 |

## 2. 与上游归因的差异

无实质差异。新增两点：

1. 部署对象 sha256 已记录，可作为后续漂移判据（`534dd40a…0c63`）。
2. 该门禁在**同一进程**内仍可观测导致 401 的代码路径；本次没有任何平台侧变更发生过（PID 与启动时间未变）。

## 3. 安全与影响（供决策）

- 移除门禁即恢复 IC-002 §8 的“无认证头”契约；代价是 5000 端口上的 `/upload_data` 对公网无认证，
  任何可达客户端都能写 `sensor_data`（`INSERT IGNORE` 只防重复，不防灌入）。
- 若安全上必须保留门禁，则只能走契约变更 + App 改造 + 新 APK 身份（方案 B），不能在客户端塞共享令牌。
- 该决策属安全策略与产品变更，按角色约束需用户明确授权；本轮未执行。

## 4. 未执行（不得推断为通过）

- 未修改 `/root/abr.py`、未重启 `user_script.service`；
- 未执行真实上传、未回读 `sensor_data`、未验证幂等；
- 未改动 8883 证书与 listener（仅复核无漂移）；
- 未处理上传限流/防滥用（取决于方案选择）。

## 5. 文件说明

| 文件 | 用途 |
|---|---|
| `platform_upload_contract_probe.py` | 只读探针：身份、单元、部署对象、处理器摘录、令牌元数据、路由码、listener、叶子证书、当日日志 |
| `handler_structure_probe.py` | 只读结构图：路由/函数/请求头检查/401 分支的行号（值脱敏） |
| `scrub_evidence.py` | 证据脱敏复查脚本（仅替换值，保留结构） |
| `01_platform_upload_contract_state.txt` | 首次只读输出（已脱敏：口令、AccessKey、令牌值均已替换） |
| `02_handler_structure_map.txt` | 处理器结构图（值脱敏） |
| `03_evidence_redaction_check.txt` | 脱敏复查结果（两次通过、仅替换值） |
| `04_readonly_findings.md` | 本文件（只读小结） |
| `05_patch_selftest.txt` | 补丁离线自测：dry-run/apply/幂等/语法/删除完整性/漂移与结构门禁，OVERALL=PASS |
| `06_service_port_recheck.txt` | 端口可达性复查（7/7 OPEN，仅 TCP connect） |
| `patch_selftest.py`、`port_recheck.py` | 可复跑脚本：补丁自测（合成 fixture）、端口复查 |

> 说明：`01` 中的 `Environment=` 行原样包含服务单元的 AccessKey 变量名，值已替换为占位符；
> `UPLOAD_TOKEN` 仅保留“存在/长度/行号”，不含值。令牌值未被写入任何交付文件。
