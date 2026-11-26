# P1 前置：N1 等价干净基线记录（2026-11-26 12:52–13:00 CST）

- 用例：`E2E-JOINT-P1` 前置第 2 项（`clean_or_explained_gateway_cache_baseline`）
- 被测对象：既有传感器（标准发布构建 `fw_sensor_FD-002r8`，uid4 `6A002C00`，本轮未烧录/未复位）+ 网关 + App vC10/vN1.9
- 方式：真实 App 产品路径的 BLE 读取、本地 `bwtemperature_database1` 只读快照、平台只读日志；未改网关/App/平台，未注入。

## 1. 网关缓存实际状态（可区分）

| App 读取时刻 (CST) | 读数来源 | id | T (°C) | H (%) | 证据 |
|---|---|---|---|---|---|
| 12:10:45 | 真实 BLE `DATA_OK` | 6A002C00 | 21.1 | 37.6 | P0/UPLOAD-04 既有证据 |
| 12:46:27 | 真实 BLE `DATA_OK`（本窗口前） | 6A002C00 | 23.5 | 35.6 | `19_local_db_summary.json` |
| 12:54:37 | 真实 BLE `DATA_OK`（本窗口） | 6A002C00 | 22.4 | 36.4 | `20_logcat_window.txt`、`30_local_db_after_summary.json` |

- 网关 `GwID=B1:17:9C:7A:4E:E0`、`dev=1`、`valueSize=160`、`mtu=240`，读取成功时 `link_state=DATA_OK`。
- 本窗口 12:57:37–12:58:35 另有一次 BLE 连接重试用尽 → `CONNECT_FAILED`（网关链路间歇，既有观察；不是注入结果）。
- **未出现 `T=36.7` / 0.1 单位 `367`**：本地库 `temperature` 全表 140 行中无 36.6–36.8 的行；窗口 logcat 无 `T=36.7`。
- 因此当前实际缓存值与 P1 注入值 `367` 可区分；若后续出现 36.7，不能解释为本轮缓存基线。

## 2. 缓存复读语义（披露）

- 12:54:37 的 `TempDb` 行 `time=1795668877`（= 2026-11-26 04:54:37Z / 12:54:37 CST）对应 App 本次 `receiveTime`；`CloudUpload` 12:54:37.834 发出 1 条，平台 12:54:40 `POST /upload_data 200`（`25_platform_window_1254.txt`）。
- App 每个读取周期对网关当前缓存值生成新的 `receiveTime` 并上传，这是既有产品语义；本记录不把它当作新的物理采样，也不作为 P1 注入证据。

## 3. 边界

- 本能力无 COM43/`mdk_flash` 入口，未观测板上 `S/G/P` 的 `r=0/s=0`；按 `e2e_joint_test_design.md` §4.2 允许的“等价干净基线”记录实际网关缓存状态。
- 未烧录、未复位、未改网关缓存、未注入 367。
- 本记录只服务 P1 前置判定，不构成 `E2E-JOINT-N1` 的独立 PASS。
