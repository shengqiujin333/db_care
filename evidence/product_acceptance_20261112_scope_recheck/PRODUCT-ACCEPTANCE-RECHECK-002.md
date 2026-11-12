# 产品整体功能验收复核报告（PRODUCT-ACCEPTANCE-RECHECK-002）

- 负责角色：软件测试
- 能力：产品整体功能验收（`product_check`）
- 时点：2026-11-12 18:04–18:24（Asia/Shanghai；工具时间戳 UTC 10:04–10:24）
- 验收对象：当前完整候选
  - 传感器固件：FD-002 rev 8 发布包 `fw_release/fw_sensor_FD-002r8.zip`
  - 安卓：1.9 debug 候选 AB-002（`71a041d3…`）
  - 既有网关与平台服务（用户确认复用）
  - 文档包 DOC-PKG-001 rev 12
- 验收仓库：`D:\mypro\beiwo2\AIP`，本轮开始时 HEAD `6aabc565e15ec7335099cd60d84863cc16f2d031`，工作树干净（仅本证据目录未跟踪）
- 原始失败：`evidence/product_acceptance_20261112/PRODUCT-ACCEPTANCE-002.md`（`TEST_FAIL` / `EXTERNAL_CONDITION_UNEXECUTED`）
- **本轮结论：`TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）**
  - 已执行范围内**未发现新的实测缺陷**；前次 D-01～D-04a 在当前 APK 上逐项复核仍关闭。
  - `FW-LIGHT-DEF2-ENABLE` 为人工已确认的 **`APPROVED_DEFERRED` / NOT EXECUTED**：本轮不重测、不重复询问、**不作为返工依据**，并按人工范围决定在证据中显式披露。
  - 原检查者把该人工跳过项写成 R4「未满足」并作为整体失败依据之一，属**范围错误**；本报告按人工范围重评（见 §2）。
  - 真实升温/超温触发、网关→手机跨链路、平台 `/upload_data` 写入/回读、功耗测量仍未执行（用户条件限制/后续联调阶段/无授权），故整体产品验收不能通过。

> 本报告只记录本轮实际执行与可复核证据。未执行、人工延期与人工决定逐项披露；不把可执行子集汇总为整体通过，也不把人工跳过项当作缺陷。

---

## 1. 候选身份（本轮实际复算/核对）

| 对象 | 身份 | 本轮证据 |
|---|---|---|
| 仓库 | HEAD `6aabc565e15ec7335099cd60d84863cc16f2d031`；`git status` 仅本证据目录未跟踪 | `identity_checks.txt` |
| 传感器固件发布包 | `fw_release/fw_sensor_FD-002r8.zip`，134,791 B，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf` | 本机 `md5sum` 复算一致 |
| 包成员 | axf `b6a15fed2ed5e3d0c4adc51ac87a4830`（297,380 B）/ hex `4d728dec9b0d28a352c46436b7586c72`（75,275 B）/ PACKAGE.txt `a58f67fcf5b760767ff5b65888c73dfb` | 流式 `unzip -p \| md5sum` 复算一致 |
| 固件产品源 | 提交 `82a7e74e5d6bae855a573e1f23f83936ba8db049`；`git diff --stat 82a7e74..HEAD -- <sensor>/{USER,MDK,EWARM,UM2005C,COMMON}` **为空** | `identity_checks.txt` |
| 安卓候选 APK | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk`，10,365,464 B，SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`，MD5 `1e17e0c281715fddd26d308246d6424c` | `identity_checks.txt` |
| 设备在装包 | `com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，debug；设备 `base.apk` 10,365,464 B、SHA-256 `71a041d3…`、MD5 `1e17e0c2…`（**与候选逐字节一致**，设备侧复算） | `device_package_state.txt`、`device_package_dump_full.txt`、`device_apk_hash.txt` |
| 设备 | Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506`，本机在线 | `android_test devices`、`device_package_state.txt` |
| 网关 | 既有 CH592 `beiwov2`；本轮未修改、未烧录、未接线核验 | 用户约束 + 未执行 |
| 平台服务 | 既有部署（用户确认复用）；候选 `CloudConfig.kt` 为 `8.140.23.253` / `ssl://…:8883` / `http://…:5000`；本轮**未连接、未写入、未连库** | `static_checks.txt` |
| 文档包 | DOC-PKG-001 rev 12；`git log -1 -- docs dengbei_care/用户使用说明书.md` 仍为 `92bfcb2`，自 rev 12 后无文档提交 | `identity_checks.txt`、`static_checks.txt` |
| 板端镜像 | 本轮 **10:18:35Z 由 pyocd HW reset 后直接取得 `BOOT/VDD/IOTEST/IOSIG/BUS` 与首周期 `S k=0`**；该启动会话为本轮新采集 | `bot01_run.txt`、`bot01_hw_20261112_scope_recheck.bin` |

---

## 2. 人工范围决定复核与原检查者范围错误（本轮重点）

### 2.1 当前人工范围决定（引用，不改写）

| 项 | 内容 |
|---|---|
| 测项 | `FW-LIGHT-DEF2-ENABLE`（无光判据启用与实板双态） |
| 人工结论 | **`APPROVED_DEFERRED` / NOT EXECUTED**（原回复「无法准备，先跳过」，2026-11-11T03:50:42Z；`deferral_allowed=true`） |
| 原因 | 缺少完全遮光窗口与同板同构建的光强双态（有光/无光）被动采集证据；无判据形制/数值接受记录；板上保持 `T=0` |
| 本轮处置 | 不重测、不重复询问同一 `test_item_id`、**不作为返工依据**；在证据与交付中显式列为 NOT EXECUTED |

### 2.2 原检查者需要重评的范围错误

| # | 原报告位置 | 原文/事实 | 范围错误 | 本轮重评 |
|---|---|---|---|---|
| E-1 | `PRODUCT-ACCEPTANCE-002` §5 R4 | 上传规则标为「**未满足/未执行**」 | 把人工已确认延期的启用项与“未满足”混同 | R4 **判定公式实现**与需求逐项一致；**无光判据启用** = NOT EXECUTED（人工延期）；**真实升温/超温触发** = NOT EXECUTED（外部条件） |
| E-2 | 同报告最终判定 | 以「需求 4 的无光分支仍禁用」作为整体 `TEST_FAIL` 依据之一 | 以人工跳过项作为整体失败/返工依据 | 整体 `TEST_FAIL` 的依据只保留**其余外部条件/后续阶段项**；人工延期项不作为返工依据 |
| E-3 | 同报告 §7 | 「R4 无光分支仍为 `T=0`：待判据接受记录与双态证据后回填」 | 后续入口本身正确，但未与“本轮不返工”区分 | 保留为需求/接受侧未决事项；固件本轮不因该未决项返工 |
| E-4 | 同报告 §6 N-1 | 已正确记录 `SKIP_CONFIRMED`、不重测、不写成通过 | 与 §5 R4「未满足」标签及最终判定不一致 | 以 N-1 的披露为准，修正 R4 标签与最终判定依据 |

### 2.3 重评后的正确分解

1. **上报公式实现**：`report = (cur > 35.0℃) 或 (有前值 且 (cur-prev) > 0.9℃ 且 dark)`；严格大于边界，与 `readme.txt` 需求 4 一致（`fw_core.c:112-126`、`sensor_config.h:138-139`）。属**已实现项**，非缺陷。
2. **无光判据启用**：`FW-LIGHT-DEF2-ENABLE` = **NOT EXECUTED（人工 APPROVED_DEFERRED）**，不是固件实现缺陷，不是本轮返工依据。
3. **真实升温/超温触发**：NOT EXECUTED（用户明确当前不能调温），属外部条件。
4. **整体验收**：仍不能宣布 PASS，但失败依据改为“其余外部条件/后续阶段项未执行”，不能把人工延期项算作固件缺口。

---

## 3. 执行方式、授权与环境边界

| 项 | 本轮做法 |
|---|---|
| 串口观测 | MCP `serial_list_ports` + `serial_capture`（COM43 / 9600 8N1，300 s）；未发送任何字节 |
| 上电五行（BOT-01 重评） | **测试侧自编脚本** `capture_bot01.py`：先开 COM43，再以 `pyocd reset -t cortex_m -M attach -m hw` 通过 CW-DAPLink 请求外部 HW reset；仅复位，**未烧录、未写内存、未改选项字节**。依据：`readme.txt` 补充条件 2 明确授权尝试 pyocd；当前原生工具 `Bash` 可用 |
| 手机操作 | MCP `android_test`（devices/launch/ui_dump/ui_click/ui_swipe/ui_press/ui_info）；ADB 只读 `dumpsys package`、`pm path`、设备侧 `stat/md5sum/sha256sum`、`logcat -d` |
| USB 枚举 | MCP `usb_list_devices`（只证明连接/身份） |
| 静态核对 | 限定目录 Read/Grep + 可复跑脚本 `collect_static.py`、`parse_ui.py` |
| 未使用 | 烧录/`mdk_flash`、sigrok 逻辑采集、平台 SSH/写接口、`/upload_data`、数据库直连、调温/调光/遮光刺激、功耗测量 |
| 原因 | 当前能力有效工具为 `android_test`、`usb_list_devices`、`serial_list_ports`、`serial_capture`；温度/光强/功耗受用户明确条件限制；跨链路按用户安排属后续联调阶段；平台只读 SSH 入口对本能力不可用 |

---

## 4. 本轮真实执行记录

### 4.1 传感器 COM43 周期观测（真实目标）

- 窗口：`2026-11-12T10:12:35Z – 10:17:35Z`（300 s）；COM43 = Prolific `067B:2303`，9600 8N1；收到 **162 B**，md5 `6522c77599290f4b4328255ce8016c85`，SHA-256 `0f38ec68b1d6a0a8da1298fd04a60a2049d6d9ff754b003dc16f641967183a35`。
- 原始两行（CRLF 结尾，逐字节保留于 `serial_capture_com43_20261112_scope_recheck.bin`；仓库内以 `serial_capture_hex.txt` 复核）：

```text
S k=1338 p=279 H=1 V=0 o=8 n=3979 x=3984 a=3982 D=0 t=278 h=352 q=1 r=0 s=0 y=0
S k=1341 p=278 H=1 V=0 o=8 n=3979 x=3986 a=3981 D=0 t=274 h=358 q=1 r=0 s=0 y=0
```

判读（只记录事实）：

1. **采样节拍**：`k` 1338→1341，步长 **3**；与 `SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3` 一致；窗口内无其它字节。
2. **前值链**：`p(k=1341)=278 = t(k=1338)`，`H=1`，前值语义自洽。
3. **采样结果**：两周期 `q=1`、`o=8`；`t` 27.8/27.4 ℃、`h` 35.2/35.8 %RH 为合理实测量；`n/x/a` 为光敏 ADC 原始事实（本周期确有 ADC 采样）。
4. **判据默认态**：`V=0 D=0`（交付默认 `LIGHT_DARK_THRESHOLD_CODE=0`；该状态**不得**解释为“已识别有光”）；`r=0 s=0 y=0`，无 `G`/`P`，本窗口无上报。
5. **与上一窗口连续性（复位前）**：上一窗口 05:11:31Z `k=1035/1038`；本窗口 10:12:35Z `k=1338/1341`。Δk=303、Δt=301.07 min，偏差 +0.64%（LSI 量级），支持复位前同一启动会话。**该连续性在 10:18:35Z 复位后终止**。

原始：`serial_capture_com43_20261112_scope_recheck.bin`、`serial_capture_hex.txt`、`serial_capture_meta.txt`。

### 4.2 BOT-01 自动测试方式重评与执行（本轮新采集）

人工重评要求（`reconsidered_test_items` BOT-01）：测试能力自行编写脚本；需要固件测试接口时说明最小接口；不得把未验证目标判为通过。本轮做法与结果：

| 步骤 | 实际结果 | 证据 |
|---|---|---|
| pyocd 探针识别 | pyocd 0.36.0；探针 `P&S CW-DAPLINK(CMSIS-DAP)` UID `87094109484987710672FF50` 在位；**CW32 专用 target 未识别**（`pyocd list --targets --name CW32` 为空，探针 Target=n/a） | `pyocd_check.txt` |
| 自动测试方法 | 使用 pyocd 内置通用 `cortex_m` target 的 `-M attach -m hw` 外部复位；**仅复位请求**，无 flash/内存写 | `capture_bot01.py` |
| 执行 | COM43 先开（10:18:32.870Z）→ pyocd HW reset 10:18:34.872–10:18:35.585Z（rc=0）→ 有界采集 25 s | `bot01_run.txt` |
| 原始结果 | **271 B**，md5 `56ed15a56e4d996a73098ddf2252c699`，SHA-256 `81d2e6c4abd9858e3b9d9baf86282c3315f6ae132452a6d8c942b7032d6427d6` | `bot01_meta.txt`、`bot01_hex.txt` |
| 五行内容 | `BOOT` / `VDD` / `IOTEST` / `IOSIG` / `BUS` 各一次且顺序正确；首周期 `S k=0` | 见下 |
| 判定 | `BOOT_LINES_FOUND=BOOT,VDD,IOTEST,IOSIG,BUS`；`BOT01_RESULT=PASS`；脚本 EXIT=0 | `bot01_run.txt` |

```text
BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600
VDD ok=1 code=1480 bgrmv=1189 mv=3289
IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=0
IOSIG scl=95555 sda=30300
BUS idle=3 scl=1 sda=1 ack=44
S k=0 p=0 H=0 V=0 o=8 n=3978 x=3985 a=3981 D=0 t=275 h=357 q=1 r=0 s=0 y=0
```

**重评结论**：
- 自动测试方式成立：测试侧脚本 + pyocd 通用 Cortex-M HW reset，可在 COM43 采集期间稳定取得上电五行，**不需要固件新增测试接口**。
- 正式交付包不含测试代码：发布包仅 `axf` / `hex` / `PACKAGE.txt` 三成员（本轮 `unzip -l` 复核），`capture_bot01.py` 仅位于 `evidence/` 证据目录，**未被纳入发布包**。
- `BOOT fw=FD-002r4` 横幅是字节冻结字符串（`PACKAGE.txt` 与 `docs/README.md` §4 均明示），**不以横幅判版本**；版本由包/成员哈希与源码基线闭合。
- 复位后 `k=0`，本板进入新的启动会话；本轮上电五行是**直接新采集**，不再依赖旧 EV-038 的 `k` 连续性推断。

### 4.3 安卓真实设备执行（当前候选 APK）

| 步骤 | 实际结果 | 证据 |
|---|---|---|
| 设备在线 | `12f046690506 device product:olivelite model:Redmi_8A` | `android_test devices` |
| 在装身份 | versionCode 10 / 1.9，debug；`base.apk` 与候选逐字节一致 | `device_package_dump_full.txt`、`device_apk_hash.txt` |
| 启动 | `launch` 成功；恢复上次页面后返回首页；`MainActivity` 前台 | `ui_after_back.xml`、`activity_final.txt` |
| 首页 | 「爱守护」「启动」「去添加」；文案“点击下方「去添加」添加设备；也可在「我的」→「打开传感器列表」查看已保存 ID”；链路状态“未配置网关地址（搜索附近网关）” | `ui_home_launch.xml/.txt`、`ui_after_back.xml/.txt` |
| 我的页（顶部） | 「查看今日报告」`[32,327][688,423]`、「打开传感器列表」`[32,431][688,527]`，均在底部导航 `[0,1264][720,1424]` 之上 | `ui_my_page.xml/.txt` |
| 我的页（上滑） | 「注册传感器」`[32,976][688,1072]`、「网关与上传设置」`[32,1120][688,1216]`，**两控件不相交（间隔 48 px）**且均在导航之上 | `ui_my_scrolled.xml/.txt` |
| 查看今日报告 | 点击后 `mResumedActivity=…/.report.ReportActivity`；页面“早起报告 / 尚未生成过报告 / 重新生成” | `activity_after_report_click.txt`、`ui_report_activity.xml/.txt` |
| 重新生成（零设备） | 点击后 `ReportScheduler: Enqueued one-shot daily report`，随后 `DailyReportWorker: No saved devices, skipping report`；页面保持“尚未生成过报告” | `logcat_session_summary.txt`、`ui_report_generated.xml/.txt` |
| 打开传感器列表 | 点击后进入 `titleSavedIds` 列表页，显示“已保存的 ID（0）”“删除选中”；`MainActivity` 未跳页签 | `ui_sensor_id_list.xml/.txt`、`activity_after_sensorlist_click.txt` |
| 网关与上传设置 | 对话框：未配置网关地址、平台身份空、搜索附近网关/扩大搜索/清除/保存、云端上传状态“无（待发 0 条）”、重试上传、关闭 | `ui_gateway_dialog.xml/.txt` |
| 守护启动 | 点「启动」后按钮变「停止」；`MqtttService` 前台服务 `isForeground=true` | `ui_guard_started.xml/.txt`、`services_guard_running.txt` |
| 守护停止 | 点「停止」后按钮恢复「启动」 | `ui_guard_stopped.xml/.txt` |
| 设定页默认值 | 2 / 0.1 / 35 / 29 / 75（页面下方 `40` 未进入本次可视区域）；与既有文档默认值一致 | `ui_settings_page.xml/.txt` |
| 崩溃检查 | 本应用 `FATAL EXCEPTION` 0 条、`ANR in com.jinyuni…` 0 条；`MqttService` 因设备无网络出现 `ENETUNREACH` 连接失败日志（既有披露现象，非崩溃） | `logcat_session_summary.txt` |

### 4.4 平台配置静态核对（候选实际使用既有服务对象）

候选 `cloud/CloudConfig.kt:18/21/24/27/30`：`SERVER_HOST=8.140.23.253`、`MQTT_BROKER_PORT=8883`（`ssl://`）、`HTTP_PORT=5000`（明文 HTTP），与 `artifacts/backend_feasibility_current.md`（BE-EXP-004，2026-11-10 只读核验）记录的既有部署一致。边界：本轮**未连接服务器、未调用任何接口、未读取服务端文件、未直连数据库**；该核对只证明“最终候选配置指向既有服务对象”，**不证明** `/upload_data` 可用。

### 4.5 文档与实现一致性

DOC-PKG-001 rev 12；`docs/` 与随包用户说明书自 `92bfcb2` 后无提交，DOC-VERIFY-012 独立校核 `TEST_PASS`（限定 rev 12 改动与受影响步骤）。本轮静态核对确认文档披露了“无光判据未启用”的当前限制（`dengbei_care/用户使用说明书.md:279`）。**观察项 O-1**：`docs/README.md:81`、`docs/06:122`、`docs/01:110`、`docs/04:241` 仍写“整产品验收尚未重跑”；该表述在 rev 12 时点属实，但 `PRODUCT-ACCEPTANCE-002` 已执行，需在下次文档修订中更新（交技术文档，不影响当前候选功能）。

---

## 5. 必需功能与验收条件逐项结果

| # | 需求/验收项 | 结果 | 依据与边界 |
|---|---|---|---|
| R1 | 温度传感器改为 GXHT40 | **通过（读数路径，有限）** | 本轮 `q=1`、`t/h` 合理；上电 `IOTEST`/`BUS ack=44`；产品源自发布提交零差异。未做长期稳定性 |
| R2 | 光敏电阻+分压电路与光照采样 | **部分（采样有效，判据未启用）** | 固件 ADC 采样有效（`o=8`、`n/x/a` 域内且逐周期变化）；`V=0 D=0` 为 `T=0` 交付默认，**不得**判为“已识别有光”。真实遮光/有光双态未执行（人工延期 N-1/N-3）。电路电气特性不在本能力范围 |
| R3 | 采集时间改为 3 分钟 | **通过（实板节拍）** | 300 s 窗口 `k` 1338→1341 步长 3；源码 `SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3`；未声称 LSI 精度 |
| R4 | 上传规则：ΔT>0.9 ℃ 且无光，或 >35 ℃ | **实现一致；触发未执行** | 源码条件为 `(cur-prev)>9 且 dark` 或 `cur>350`（严格大于），与需求逐项一致。交付默认 `T=0` ⇒ 无光分支不触发（明示交付默认，**不是遗漏**）；`FW-LIGHT-DEF2-ENABLE` = NOT EXECUTED（人工延期）；真实升温/超温触发未执行（N-2）。本窗口 `r=0 s=0`、无 `P` |
| R5 | 平台 broker/服务信息 | **部分（静态一致，未连接）** | 候选常量指向 `8.140.23.253:8883/5000`，与既有服务一致；本轮无平台写授权，未连接、未写入、未连库（N-5） |
| R6 | 同步修改安卓 App（iOS 不改） | **通过（当前 APK 已执行范围）** | 候选已安装且字节一致；D-01～D-04a 逐项关闭；首页/我的/报告/列表/网关对话框/守护启停可用、无崩溃。真实 BLE/上传链路未执行（N-4/N-8） |
| R7 | 当前有光 | **条件确认（未判定光路）** | 用户声明当前有光；本轮未改变光强、未遮光；`a=3981/3982` 仅作事实记录，不据此判“有光”或光路好坏 |
| R8 | UART1 调试串口初始化与打印 | **通过（本轮新采集上电五行 + 周期行）** | 本轮 10:18:35Z 复位后直接取得 `BOOT/VDD/IOTEST/IOSIG/BUS` 五行与 `S k=0`；周期行 `S k=1338/1341` ≤96 B、CRLF 完整；`debug_trace.c/h` 拥有 PA06 TXD/PA05 RXD、9600 8N1 |
| R9 | 取消光通讯与霍尔识别 | **通过（静态）** | 当前源码仅在退役注释中出现 hall/OPTCFG；`interrupts_cw32l010.c:186` 注明 PB04 已改 `LIGHT_ADC`、不再有霍尔 EXTI；产品源自发布提交零差异 |
| E2E | 传感器→433→网关→BLE→手机→平台完整业务链路 | **未执行** | 跨链路按用户安排属后续联调阶段；网关未配置、手机未绑定；平台未写入（N-3/N-4） |

---

## 6. 未执行项、延期与人工决定（必须随交付披露）

| ID | 项 | 状态 | 原因/后续责任 |
|---|---|---|---|
| N-1 | `FW-LIGHT-DEF2-ENABLE`（无光判据启用与实板双态） | **NOT EXECUTED（人工 `APPROVED_DEFERRED`，`deferral_allowed=true`；原回复「无法准备，先跳过」，2026-11-11T03:50:42Z）** | 缺完全遮光窗口与同板同构建的光强双态证据；无判据形制/数值接受记录；`T` 保持 0。本报告不重测、不重复询问、**不作为返工依据**。后续：上游接受判据形制/数值并由固件实现回填、重建后复验 |
| N-2 | 真实 ΔT>0.9 ℃、>35.0 ℃ 触发与 433 实发 | **未执行** | 用户明确当前不能调节温度；本窗口无 `P`/`r=1`/`s=1`。后续：具备调温条件时按 `docs/03` 判读 |
| N-3 | 光强变化/遮光刺激 | **未执行（不安排）** | 用户明确当前不能改变光强；与 N-1 的人工延期一致 |
| N-4 | 433→网关→BLE→手机跨链路与同笔一致性 | **未执行** | 用户安排后续联调阶段；网关身份、供电与配对未验证；手机未配置网关地址 |
| N-5 | `/upload_data` 真实写入、幂等与回读 | **未执行** | 当前能力无写接口/数据库授权；平台只读 SSH 入口对本能力不可用 |
| N-6 | `E2E-04` CR2032 峰值/深睡电流 | **未执行（已批准延期记录在案）** | 无电流表/SMU/示波器；用户明确不能测功耗 |
| N-7 | BOT-01 上电五行 | **已执行（本轮自动方式，PASS）** | 测试侧脚本 + pyocd 通用 `cortex_m` HW reset；见 §4.2。未烧录、未写内存 |
| N-8 | 手机端真实 BLE/GATT/MTU、数据展示、告警、上传状态 | **未执行** | 未配置网关、无已绑定传感器（0 条）；不自行扫描/配对（跨链路阶段） |
| N-9 | 注册/登录、网关编码保存、真实告警参数下发 | **未执行** | 无账号/网关编码等外部数据；不写入平台、不改用户数据 |
| N-10 | 「打开传感器列表」有真实绑定数据时的列出/单项/批量删除 | **未执行** | 当前无绑定数据；仅验证空态入口与删除控件存在 |
| N-11 | API 31+ 权限分支、9 台真实快照、第 10 台拒绝 | **未执行** | 设备为 API 29；无真实多设备源 |
| N-12 | iOS | **不在本轮范围** | 用户明确暂不修改 iOS |

**人工决定汇总（明示）**：

1. `FW-LIGHT-DEF2-ENABLE`：经人工确认**未执行**（`APPROVED_DEFERRED` / `SKIP_CONFIRMED`，「无法准备，先跳过」，2026-11-11T03:50:42Z）；保持 `T=0`，本报告不写成通过，也不作为返工依据。
2. `E2E-04`：已批准 `DEFERABLE`，本轮无仪器，未测并在此列明。
3. 真实升温/调光/遮光：用户明确当前不能改变温度/光强，不安排相关硬件测试。
4. 网关→手机跨链路：用户明确安排在后续联调阶段，本轮不据此反复退回固件实现。
5. BOT-01：人工要求重评自动测试方式；本轮自编脚本并以 pyocd 通用 Cortex-M HW reset 执行，取得五行并判 PASS（§4.2）。

---

## 7. 观察项（不构成本轮整体失败依据）

| ID | 观察 | 影响/责任 |
|---|---|---|
| O-1 | 文档仍写“整产品验收尚未重跑”（`docs/README.md:81`、`docs/06:122`、`docs/01:110`、`docs/04:241`） | 文档时效性；交技术文档在下次修订更新，不影响当前候选功能 |
| O-2 | 零已保存设备时点击报告页「重新生成」：Worker 记录 `No saved devices, skipping report`，页面保持“尚未生成过报告”，无可见反馈 | 产品既有设计（无设备无可汇总数据）；属 UX/文档说明缺口，不构成本次需求缺陷。若要求“无设备也生成空报告”，需由需求方明确 |
| O-3 | 设备无网络时 `MqttService` 持续记录 `ENETUNREACH` 连接失败 | 既有披露现象；设备侧网络条件，不是应用崩溃或功能缺陷 |

---

## 8. 证据清单（本目录）

| 文件 | 内容 |
|---|---|
| `PRODUCT-ACCEPTANCE-RECHECK-002.md` | 本报告 |
| `identity_checks.txt` | HEAD/status、固件包与成员 md5、APK 大小/SHA-256/MD5、产品源零差异、文档提交 |
| `device_package_state.txt` / `device_package_dump_full.txt` / `device_apk_hash.txt` | 设备属性、`dumpsys package`、安装路径、设备侧 APK 大小/哈希（与候选逐字节一致） |
| `serial_capture_com43_20261112_scope_recheck.bin` / `serial_capture_hex.txt` / `serial_capture_meta.txt` | COM43 原始 162 B、hex、md5/sha256 与窗口参数（`.bin` 命中 `.gitignore`，本机留存；随仓库以 hex 文本复核） |
| `capture_bot01.py` / `bot01_run.txt` / `bot01_hw_20261112_scope_recheck.bin` / `bot01_hex.txt` / `bot01_meta.txt` | BOT-01 自动测试脚本、原始运行日志、271 B 原始捕获、hex、md5/sha256（`.bin` 本机留存） |
| `pyocd_check.txt` | pyocd 版本、CW-DAPLink 探针 UID、CW32 专用 target 未识别 |
| `ui_home_launch.xml/.txt` / `ui_after_back.xml/.txt` | 首页 UI 原始树/文本（新文案与「去添加」） |
| `ui_my_page.xml/.txt` / `ui_my_scrolled.xml/.txt` | 我的页顶部/上滑后 UI 树与 bounds |
| `ui_report_activity.xml/.txt` / `ui_report_generated.xml/.txt` / `activity_after_report_click.txt` | 报告页、零设备「重新生成」结果、Activity 栈 |
| `ui_sensor_id_list.xml/.txt` / `activity_after_sensorlist_click.txt` | 传感器列表空态与 Activity 栈 |
| `ui_gateway_dialog.xml/.txt` | 网关与上传设置对话框 |
| `ui_guard_started.xml/.txt` / `ui_guard_stopped.xml/.txt` / `services_guard_running.txt` | 守护启停与前台服务状态 |
| `ui_settings_page.xml/.txt` | 设定页默认参数 |
| `activity_final.txt` | 本轮结束时 Activity 状态 |
| `logcat_session_summary.txt` | 本应用 FATAL/ANR 计数（0）、报告/Activity 关键行、`ENETUNREACH` 说明、缓冲尾部 |
| `static_checks.txt` | 采样节拍/上报判据/光判据宏、UART1 引脚、平台常量、hall/OPTCFG 残留、文档版本头 |
| `collect_static.py` / `parse_ui.py` | 可复跑的静态收集与 UI 节点有界解析脚本 |

---

## 9. 证据局限

1. 板端镜像哈希不可回读；身份由发布包/成员哈希、产品源零差异与本轮新采集的上电五行（`BOOT/VDD/IOTEST/IOSIG/BUS`、`S k=0`）闭合。本轮复位后不再依赖旧 EV-038 的 `k` 连续性推断。
2. BOT-01 使用 pyocd 通用 `cortex_m` target 请求外部 HW reset；CW32 专用 target 未被 pyocd 识别。该方式只证明复位与串口观测可行，不证明 pyocd 的 CW32 flash 算法可用（本轮未烧录）。
3. 未调温、未调光、未遮光；无真实触发、无 `P`/发送失败样本；`V=0 D=0` 不是“有光已识别”的证据。
4. 未配置网关/传感器编码，手机端数据展示、告警、BLE 与上传均未执行；未访问平台写入接口或数据库。
5. 未测量电气/光路/功耗；ADC 原始值不用于判定光路好坏。
6. 真机 UI 覆盖当前候选；APK 字节与候选一致（设备侧复算）。
7. 本报告不把组件测试、构建成功、页面可打开或部分通过当作整体通过。

---

**最终判定：`TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）。**

- 已执行范围内**未发现新的软件/固件实测缺陷**；D-01～D-04a 在当前候选 APK 上仍关闭。
- `FW-LIGHT-DEF2-ENABLE` 为人工 `APPROVED_DEFERRED` / NOT EXECUTED：本报告显式披露、不重测、不作为返工依据；原检查者把该项写成 R4「未满足」并作为整体失败依据的范围错误已按人工范围重评。
- BOT-01 已按人工要求以自动方式重新执行并 PASS（新采集上电五行）。
- 真实升温/超温、跨链路、平台写入/回读、功耗仍未执行；整体产品验收不能通过。上述未执行项按用户条件、人工决定与后续阶段安排如实保留，不得解释为通过。
