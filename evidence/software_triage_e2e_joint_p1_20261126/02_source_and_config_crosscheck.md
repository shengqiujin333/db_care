# E2E-JOINT-P1 失败分析支撑：源码与配置交叉核对（只读）

- 分析窗口（UTC）：2026-11-26T05:17Z
- 仓库 HEAD：`1b85dbf`
- 范围：只做失败归因所需的最小只读核对；未修改产品源码、固件、APK、平台、prefs 或数据库。

## 1. 固件测试构建 / 注入接口

| 核对项 | 实际结果 | 证据 |
|---|---|---|
| `measure_sample()` 注入点前后 | 存在采样函数 `measure_sample()`（`USER/src/measure.c:460`），其成功分支直接调用 `sensor_decide_report(...)`（`measure.c:504-506`）；二者之间无任何测试注入分支 | `measure.c:460`、`measure.c:504-506` |
| 注入宏/观测标记 | 固件源码树中 `SENSOR_TEST_INJECT` / `TINJ` 命中 **0** 个文件 | 本轮重跑 `evidence/e2e_joint_p1_20261126/audit_fw_test_build.py`，输出见 `01_fw_testbuild_audit_rerun.txt` |
| Keil 工程 target | 仅 `Project` 一个 target；`Define` 条目为空；工程文本不含 `SENSOR_TEST_INJECT`/`TINJ` | 同上 |
| 发布包成员 | 仅 `fw_sensor_FD-002r8_b6a15fed.axf`、`fw_sensor_FD-002r8_4d728dec.hex`、`PACKAGE.txt`；二进制内测试字符串命中 `NONE` | 同上；md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf` |
| 仓库内测试构建包 | 仅有 `fw_release/fw_sensor_FD-002r8.zip` 一个发布包，无测试构建包 | 同上 |

判读：当前源码与发布物中不存在 P1 所需的“默认关闭、仅测试启用”注入构建或 `TINJ` 观测；不能据此判固件功能缺陷，但 P1 的嵌入式触发条件不具备。

## 2. App 身份、发布分支与 TLS

| 核对项 | 实际结果 | 证据 |
|---|---|---|
| App MQTT 凭据来源 | `MqtttService.kt:1610-1614`：`savedInput = prefs.getString("mac_addr", ...)`；`userName = savedInput`，`password = savedInput + "&^!A:z?"` | `dengbei_care/.../MqtttService.kt:1609-1618` |
| 发布分支 `shouldPublishBle()` | `device_mac.isNotBlank() && !device_mac.startsWith("1010")` | `MqtttService.kt:1031-1032`；另有 `:374` 同条件 |
| 当前 `mac_addr` | `0200000000A1`（合成测试值，来源未确认） | `evidence/e2e_joint_p1_20261126/15_prefs_before.xml`、`29_prefs_after.xml` |
| 当前 `registerphone` | `14000000001`（模拟测试号） | 同上 |
| 当前网关 BLE 地址 | `84:C2:E4:03:02:02`（与 `mac_addr` 不是同一字段） | 同上 |
| 服务端证书链/地址 | 叶子 `CN=8.140.23.253`，SAN `IP:8.140.23.253, IP:117.72.84.210`，issuer `CN=Tiktok Root CA`，notAfter `2028-11-24T02:55:16Z` | `evidence/e2e_joint_p1_20261126/24c_host_leaf_fields.txt` |
| 用 App 信任锚校验 | `Verification: OK`、`Verify return code: 0 (ok)` | `evidence/e2e_joint_p1_20261126/24_host_tls_verify_ip.txt` |

## 3. 服务端认证与平台回读入口

| 核对项 | 实际结果 | 证据 |
|---|---|---|
| EMQX 认证结果 | `username: 0200000000A1`、`reason: not_authorized`，日志位于 AUTHN 阶段；说明 TLS 已完成并已进入 MQTT CONNECT/认证 | `evidence/e2e_joint_p1_20261126/26_emqx_window_1254.txt:462+`、`31_emqx_window_1259.txt` |
| 平台路由 | `/sendVerificationCode`、`/verifyCode`、`/register`、`/login`、`/setmac`、`/upload_data` 全部 `POST`；无回读/查询路由 | `evidence/e2e_joint_p1_20261126/22_platform_readonly.txt`（`handler_routes_readonly`） |
| `GET /upload_data` | HTTP `405 METHOD NOT ALLOWED`，`Allow: OPTIONS, POST` | 同上（`upload_http_routes`） |
| 写入方式 | `INSERT IGNORE INTO sensor_data`；未发现回读查询 | 同上（`handler_upload_readback_search`） |

## 4. App 真实读取/上传观测（仅作旁证，不是 P1 事件）

- 12:54:36.871 `MTU negotiated: mtu=240 ... expectedDevices=1, requiredMtu=35`
- 12:54:37.614 `GwID=B1:17:9C:7A:4E:E0, dev=1, valueSize=160, mtu=240`（160 = 10×16 B，16 B 对齐）
- 12:54:37.615 `id=6A002C00, H=36.4%, T=22.4°C`
- 12:54:37.635 `Save reading devId=6A002C00 time=1795668877`
- 12:54:37.645 `link_state=DATA_OK`
- 12:54:37.646 `MQTT offline: keep latest BLE batch (bounded=1)`
- 12:54:37.834 `CloudUpload uploaded 1 reading(s)`；平台 12:54:40 `POST /upload_data 200`
- 12:53:56.005 / 12:58:56.024 `无权连接 (5)`
- 12:58:35.330、13:02:48.090 `link_state=CONNECT_FAILED`（BLE 链路间歇；与注入无关，本轮无注入）

来源：`evidence/e2e_joint_p1_20261126/20_logcat_window.txt`。

## 5. 本地库可区分性

- `temperature_count=140`，`rows_36_7=[]`（无 **温度** 36.6–36.8 的行）。
- **披露**：`latest10` 中存在 **湿度** 36.7 的历史行（如 `[1795665077, "6A002C00", 21.9, 36.7]`）。相关键是温度 `T=36.7`，湿度 36.7 不构成 P1 温度事件；两者字段不同，不应混淆。

来源：`evidence/e2e_joint_p1_20261126/19_local_db_summary.json`、`30_local_db_after_summary.json`。
