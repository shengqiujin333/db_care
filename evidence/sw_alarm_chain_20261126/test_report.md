# 软件验证 TestEvidence（EV-SW-ALM-20261127 / 真机 Android 报警、停止与守护链路）

- 证据编号：`EV-SW-ALM-20261127`
- 对应能力 / 工作项：`software_tester.software_verification`（当前调用：在原 Project Goal 下执行软件验证）
- 被测对象（subject）：`D:\mypro\beiwo2\AIP\dengbei_care` 当前源码构建的 debug APK，实际安装于 Redmi 8A；数据源为既有 CH592 网关的真实 BLE FFE2 缓存
- 结论：**TEST_FAIL**（失败原因：`MEASURED_DEFECT`；同时存在必须由实现补齐的可测性缺口）
- 执行时间：2026-11-26T15:39Z – 2026-11-27T16:50Z（设备本地 UTC+8，本报告内时间均为设备本地时间，另有 UTC 标注者除外）
- 关联设计：`artifacts/software_test_design_alarm_chain.md`（TD-SW-ALM-001）、`artifacts/software_e2e_plan_alarm_chain.md`（E2E-SW-ALM-001）、`artifacts/embedded_test_design_alarm_chain.md`（TD-EMB-ALM-001）

> 本报告只对本轮实际安装字节、实际网关缓存值、实际手机状态和实际时间窗有效。传感器固件本轮未重烧/未复位；网关本轮未烧录、未修改；平台/云/数据库/iOS 未纳入、未执行。测试侧未修改任何产品源码。

---

## 1. 版本与身份（实际核对，不是构建产物推断）

| 项 | 值 | 证据 |
|---|---|---|
| 仓库 | `D:\mypro\beiwo2\AIP`，`main`，开工 HEAD `8d9dc06` | `git rev-parse HEAD`；工作区干净 |
| Android 产品源 | 相对实现基线 `a12d12b`（2026-11-13 12:35）零差异；本轮无修复构建 | `git diff --stat a12d12b..HEAD -- dengbei_care/app/src/main dengbei_care/app/build.gradle.kts` 为空 |
| 被测 APK | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk`，10,370,964 B，SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b` | `sha256sum`；`output-metadata.json`（versionCode 10 / versionName 1.9） |
| 实际安装 | `com.jinyuni.dengbei_care`（爱守护），versionCode **10** / versionName **1.9**，`firstInstallTime=lastUpdateTime=2026-11-26 23:39:36`，targetSdk 34 / minSdk 26，签名 v2（摘要 `3a34b52a`） | `android_test install` 工具回读设备侧 `base.apk` SHA-256 = `80232c50…`；`evidence/…/01_dumpsys_package.txt` |
| 手机 | Redmi 8A `olivelite`，序列号 `12f046690506`，Android 10 / API 29，定位模式 3，蓝牙开启 | `adb devices -l`；`02_env_state.txt` |
| 传感器（身份） | CW32L010F8 + GXHT40，`BOOT uid=6A002C00`（沿用嵌入式 EV-041 同一次连续运行，本轮未复位/未重读） | `evidence/embedded_verify_identity_20261126/*` |
| 网关（实际扫描） | 名称 `xxx`，BLE MAC `84:C2:E4:03:02:02`，RSSI −44，广播含 **FFE0**；App 日志 `GwID=B1:17:9C:7A:4E:E0`，`dev=1`，`valueSize=160`，`MTU=240` | 网关设置页扫描候选；`06/07/…logcat`；`GwID` 日志行 |
| 绑定 | App `macid_name_map={"6A002C00":"t1"}` | `04_prefs_after_register.txt`；传感器列表页“t1 6A002C00” |
| 报警参数 | 全局默认：`tempmin=29 tempMax=35 hummin=40 hummax=75 tempdrop=2 humdrop=0.1 guardtime=8 delayalarm=10`，四开关初值=1 | `04/05/37/52_prefs*.txt` |

> `SW-VER-01`：**PASS** —— 设备侧安装字节与候选 APK 一致；versionCode/Name 与声明一致。本轮未使用 `apksigner`，以 `dumpsys` 的 v2 签名摘要补充（未因此豁免字节核对）。
> `SW-VER-02`：**PASS** —— 网关地址为实际扫描候选（不是默认值推断），`uid4` 绑定与传感器一致，真实 BLE 读取返回 `dev=1, id=6A002C00`。
> `SW-VER-03`（修复版身份）：**NOT EXECUTED** —— 本轮执行的是未修复的当前构建；实现侧尚未提供修复版。

---

## 2. 真实刺激与链路事实

### 2.1 传感器侧（只读引用，不在本能力重做）

嵌入式证据（`evidence/embedded_verify_identity_20261126/test_report.md`）显示：板件运行 FD-002r8，`q=1`，每 180 s 一条 `S`，`V=0 D=0`（`LIGHT_DARK_THRESHOLD_CODE=0`），`r=0 s=0 y=0`，**无 P/G 发送行**；最近真实采样 `t=336`（33.6 ℃）、`h=258`（25.8 %RH）。即：本轮传感器**没有产生新的 433 上报**。

### 2.2 网关缓存（本轮实测）

App 每 180 s 通过 BLE 读取 FFE2，实际成功读到（8 次成功，见 §5.6）：

```text
GwID=B1:17:9C:7A:4E:E0, dev=1, valueSize=160, mtu=240
id=6A002C00, H=35.8%, T=23.4°C
link_state=DATA_OK
```

该 23.4 ℃ / 35.8 %RH **与传感器当前 S 读数（33.6 ℃ / 25.8 %RH）明显不同**，说明它是网关保留的旧缓存，而非本轮采样。网关源码 `APP/app_um2006A.c:150-231`：只有 `radio_recv_data` 成功但 `decode_frame10` 失败时才 `timeoutcnt--`；**完全没有 433 接收时不会递减**，因此无新帧时旧缓存可长期保留（网关本轮不改，仅记录为现象根因之一）。

### 2.3 本轮执行的操作边界

- 使用：`android_test`（install/ui_*/tap/screenshot）、ADB 只读/测试数据查询（`dumpsys`、`logcat`、`run-as` 拉取数据库与 `shared_prefs`）、宿主机 `sqlite3` 只读查询、Gradle 宿主机单测。
- 未使用：烧录/复位传感器、修改网关、访问平台/云/数据库、调温/调光/功耗测量。
- 测试侧新增：`evidence/sw_alarm_chain_20261126/query_db.py`、`poll_alarm.py`（只读解析/轮询脚本），未改产品源码。

---

## 3. 时间线（关键事件，均有原始证据）

| 时间（本地 UTC+8） | 事件 | 证据 |
|---|---|---|
| 23:39:36 | 安装 vC10/vN1.9，设备字节 SHA-256=`80232c50…` | `01_dumpsys_package.txt` |
| 23:40:03 | 接受隐私政策，进入首页 | UI dump |
| 23:42:43 | 绑定传感器 `6A002C00`/`t1` | `04_prefs_after_register.txt` |
| 23:45:28 | 扫描并保存网关 `84:C2:E4:03:02:02`（RSSI −44，FFE0） | 扫描候选 UI；`GatewayBleSettings` 日志 |
| 23:46:28 | **首次真实 BLE 读取成功**：`id=6A002C00, H=35.8, T=23.4, DATA_OK`；守护未开 → 未写读数行、记 1 条 `alarm_events` | `07_logcat_apptags_2.txt`；`db2` |
| 23:47:51 | 首页卡片：`t1`、状态“警告”、`23.4°C`、`36%`、“温湿度超限报警” | `62_ui_home_alarm_card.xml`；`60_home_after_alarm.png` |
| 23:48:04 | 启动守护（按钮“停止”，`measure_state=stopping`） | UI dump；prefs |
| 23:54:37 | **A1 报警**：阈值（23.4 < 29），震动开始、MediaPlayer 播放、`alarm_events`/`temperature` 落库 | `14_after_alarm_dumpsys.txt`；`db2` |
| 23:55:56 | **点「停止」按钮** | `16_after_stop_click_5s.txt` |
| 23:56:05 / 23:56:41 | 停止按钮后 **+9 s / +45 s 仍在震动与播放**（同一 `startTime 23:54:37`，MediaPlayer `state:started`） | `16/17_*.txt` |
| 23:57:00 / 23:57:14 | 点卡片下方空白区域 (360,1150) 与标题栏空白 (360,250)：**均未停止** | `18/19_*.txt` |
| 23:57:31 | 点内容顶部窄条 (360,120)：震动=null，App MediaPlayer 消失 → 停止 | `20_after_tap120.txt` |
| 00:03:43 | 守护关闭下**成功读取同一越限值**：无铃声/震动、不写读数行、`alarm_events` 仍记录 | `25/26`；`db3` |
| 00:13:34 | **A2 报警**（守护开启，距 A1 >900 s）：震动/铃声；落库 | 震动历史（`30_sideeffect_history.txt`）；`db3` |
| 00:14:04 / 00:14:13 | 出现非测试触发的 App Toast；系统 `SwipeUpClean` **强制停止** App（pid 29700 被杀） | `03_app_logcat.txt:29687/30326-30426` |
| 00:16:39 | App 重启后守护状态恢复为“停止”按钮（on），卡片“未收到/--” | `63_ui_home_relaunch.xml` |
| 00:21:24 | 成功读取并落库；但此刻 `zhendongalarm/xianglingalarm` 已为 0 → 无铃声/震动（**前置条件被破坏，不作为产品结论**） | `db4`；`35_prefs_now.txt` |
| 00:27:10 | 通过「设定」页显式恢复四开关=1 并保存（Toast“已保存全局默认参数”） | `37_prefs_restored_switches.txt` |
| 00:28:54 | **A4 报警**：震动（startTime 00:28:54）+ 铃声（MediaPlayer 727 `state:started`）；落库 | `39/40_*.txt`；`db_final` |
| 00:29:16 | 顶部窄条轻触 → 铃声/震动停止；**守护仍为开启** | `41_after_silence_A4.txt` |
| 00:37:03 | 同一值再次成功读取：仍在 900 s 节流内 → 无铃声/震动（仅落库） | `db_final` |
| 00:45:10 | **A5 报警**：同一未变化值（23.4/35.8）再次震动；距 A4 16 分 16 秒（900 s 节流到期后的下一次成功读取） | `39_alarm_poll.txt`（startTime 00:45:10）；`42_repeat_alarm_A5.txt` |
| 00:45:27–31 | 出现 3 次非测试触发的 App Toast；00:45:31 `MiuiGesture: Detected`；00:45:33 再次 `SwipeUpClean` 强制停止 App | `03_app_logcat.txt:54267-54623` |
| 00:47:14–00:48:46 | 重新启动 App；点「停止」关闭守护；「设定」显式恢复四开关=1 并保存 | `52_prefs_final.txt` |
| 00:48:20–00:48:59 | 宿主机单测复跑（`--rerun-tasks`）：**39 套件 / 300 项，0 失败 0 错误** | `51_gradle_unit_tests_rerun.log`；test-results XML |

---

## 4. 缺陷与失败（实现需修复/调查）

### D1（阻塞，已实测）点「停止」只关守护，不停止当前铃声与震动

- 期望：点击「停止」后 ≤2 s 铃声与震动停止（Project Goal 明确要求“取消后铃声和震动停止”）。
- 实际：23:55:56 点击「停止」；23:56:05（+9 s）与 23:56:41（+45 s）`dumpsys vibrator` 仍为同一 `mCurrentVibration`（`startTime 23:54:37, mRepeat=0`），`dumpsys audio` 中 App 的 `MediaPlayer` 仍 `state:started`；按钮已变“启动”、`measure_state=starting`（守护已关）。
- 代码根因（只读定位）：`HomeFragment.toggleButton()` 停止分支只 `startService(START_STOP_GUARD, "0.0")`，不调用 `RingtonePlayer.stopAlarm()` / `VibrationPlayer.stopVibration()`；停止播放器仅在 `BaseActivity.onTouchEvent(ACTION_DOWN)`。
- 证据：`15/16/17_*.txt`；`03_app_logcat.txt` 对应时段。

### D2（阻塞，已复现）同一未变化读数按 900 s 周期反复报警

- 期望（Project Goal / TD-SW-ALM-001 修复判据）：同一未变化值在一个报警 episode 内只产生一次普通报警副作用；只有值变化/条件清除后重新越限才允许新 episode。
- 实际：A4 = 00:28:54，A5 = 00:45:10，值始终为网关缓存的 23.4 ℃ / 35.8 %RH（期间 00:37:03 的读取也未变化），守护始终开启；A4 后轻触顶部静音、未停止守护；900 s 到期后的下一次成功读取再次震动（并在 A4 时实测有铃声）。报警副作用间隔 16 分 16 秒 ≈ 900 s 节流 + 180 s 读取节拍。
- 代码根因（只读定位）：`MqtttService.processTemperatureHumidityData()` 对每个 BLE 样本重新评估，普通报警仅用 `now - history.lastAlarmTime > 900_000 ms` 节流，不按“值是否变化/同一 episode”去重；网关在无 433 新帧时会长期返回同一缓存值（`app_um2006A.c` 的超时只在解码失败时递减），因此用户会持续收到同一旧值的重复报警。
- 证据：`39_alarm_poll.txt`、`40_alarm_A4_state.txt`、`42_repeat_alarm_A5.txt`、`54_db_final_query.txt`（连续 6 条 23.4/35.8 读数、8 条 threshold 事件）。

### D3（次要，已实测）“空白区域取消”只在内容顶部窄条生效

- 实际：点卡片下方空白 (360,1150) 与标题栏空白 (360,250) 均未停止报警；只有内容顶部窄条 (360,120，y≈55–183) 触达 `BaseActivity.onTouchEvent` 并停止。
- 影响：用户按“点空白处静音”的习惯操作时大概率无效；与 D1 叠加后，用户几乎只能等 900 s 或被系统清理。
- 证据：`18/19/20_*.txt`。

### D4（需调查，已记录）报警开关 `zhendongalarm` / `xianglingalarm` 在无测试保存动作时被改为 0

- 事实：00:27:10 通过「设定」显式保存为 1（`37`，文件内容=1）；A4（00:28:54）实测有震动+铃声；00:45:36 读取 prefs 时两键已为 0，且 A5 之后系统再次 `SwipeUpClean`。期间测试侧没有执行任何「设定」保存动作。
- 伴随证据：00:45:27–31 出现 3 次 App Toast（内容未捕获），00:45:31 `MiuiGesture: Detected`，00:45:33 系统以 `SwipeUpClean` 强制停止 App；00:14:04 有同类 Toast 与 00:14:13 强制停止；输入设备列表只有真实触摸屏 `ILITEK_TDDI`，无虚拟输入设备/无障碍服务。
- 判定：**根因未确定**。可能是系统界面/触摸异常（幽灵触摸）或 MIUI 行为，也可能是 App 某条未识别的写 prefs 路径；本轮不把它当作已确认的产品缺陷，但其会破坏“报警开关应保持”的前置条件，需实现者结合现场设备复现调查。测试侧已在最终状态把两键恢复为 1（`52_prefs_final.txt`）。

### D5（现象/需跨组件确认）网关无新 433 帧时旧缓存长期有效，App 把陈旧值当“最新读取”

- 实际：App 每 180 s 显示“最近读取 HH:mm”，但值 23.4/35.8 与传感器实时 33.6/25.8 不符；UI 已注明“显示的是手机读取网关缓存的时刻，不是传感器采样时刻”，但没有“数据陈旧/超时”判据。
- 网关源码：`timeoutcnt` 仅在“收到但解码失败”时递减，完全无接收时不递减，旧缓存不会被占位清零。
- 范围：网关本轮明确只转发、不修改；此条作为跨组件观察与背景，不要求本轮改网关，但实现/产品需评估 App 侧是否应对长期不变的缓存值做陈旧判定或报警去重（与 D2 相关）。

### D6（环境/可测性）BLE 读取成功率低、报警通知不可见

- 读取：约 1 小时窗口内 **8 次成功读取 / 10 次达到最大重试失败**（另有进行中窗口），失败多为 `Connect error status=133` 与 `Timed out waiting for 16000 ms`；成功多出现在第 3–5 次重试。属稳定性风险，可能与手机/网关现场 BLE 环境有关，记录但不单独判定产品失败。
- 通知：`alarm_channel` 存在（IMPORTANCE_HIGH，声音/震动已启用），`POST_NOTIFICATION: allow`，代码对普通报警调用 `sendAlarmNotification()`；但 A1/A4 时 `dumpsys notification` 的 `NotificationRecord` 只有前台服务通知（`MqttServiceChannel` id=1），无 `alarm_channel` 记录；MIUI `post_frequency` 对本次 App 显示 `count=11, muted=11/11`。**结论：报警通知在本机未观察到用户可见记录；无法区分是 App 未成功 post 还是 MIUI 静音/过滤**。需在标准 Android 或关闭 MIUI 通知过滤后复测，或由实现补充通知投递诊断。

---

## 5. 用例逐项判定

> `A` = 缺陷复现相位（本轮）；`B/C`（修复版/完整回归）未开始。所有 PASS 仅对本轮实际安装字节与时间窗有效。

### 5.1 版本与显示

| 用例 | 结果 | 依据 / 缺口 |
|---|---|---|
| `SW-VER-01` App 实际安装字节 | **PASS** | 设备侧 SHA-256 与候选一致；vC10/vN1.9；安装时间 23:39:36 |
| `SW-VER-02` 链路身份 | **PASS** | 扫描到 `84:C2:E4:03:02:02`（FFE0，RSSI −44）；`uid4=6A002C00` 已绑定；`GwID=B1:17:9C:7A:4E:E0 dev=1 valueSize=160`；网关固件版本无回读（记录“未知”） |
| `SW-VER-03` 修复版构建身份 | **NOT EXECUTED** | 无修复版构建；源码相对 `a12d12b` 未变 |
| `SW-DSP-01` 网关缓存→App 显示 | **PASS** | 日志 `id=6A002C00 H=35.8 T=23.4 DATA_OK`；卡片 `23.4°C`/`36%`/“警告”/“温湿度超限报警”；`temperature` 行 `(1795708477, 6A002C00, 23.4, 35.8)`。注意：这是网关缓存值，非本轮新 433 事件 |
| `SW-DSP-02` 受控注入值显示 | **NOT EXECUTED** | 缺固件测试构建（NORM 30.0/55.0）；量产固件在“有光、常温、V/D 禁用”下不会上报 |

### 5.2 正常读数与异常报警

| 用例 | 结果 | 依据 / 缺口 |
|---|---|---|
| `SW-NF-01` 正常读数不误报（注入 30.0/55.0） | **NOT EXECUTED** | 需固件注入构建；无干净基线 + 受控正常值 |
| `SW-NF-02` 真实读数与默认阈值 | **观察记录（非阻塞）** | 真实缓存 23.4 ℃/35.8 %RH 低于默认 `tempmin=29`/`hummin=40` → 判为超限属默认参数与环境不匹配；且缓存值比传感器实时读数陈旧，不能据此判 App 误报。已记录真实 `S` 与缓存差异 |
| `SW-ALM-01` 超限报警 | **部分通过（L0 真实缓存路径）** | 实测：阈值判定、`alarm_events type=threshold`、卡片警告/文案、震动（多次 dumpsys 历史）、铃声（MediaPlayer started）均成立；**报警通知未观察到**；注入 36.0/55.0 的指定值未执行（NOT EXECUTED） |
| `SW-ALM-02` 下降报警 | **NOT EXECUTED** | 需注入 `DROP_BASE→DROP_CUR` |
| `SW-ALM-03` 紧急报警 | **NOT EXECUTED** | 需注入 `EMG 66.0`；现场不可调温 |

### 5.3 重复报警

| 用例 | 结果 | 依据 / 缺口 |
|---|---|---|
| `SW-REP-01` 同一未变化普通读数反复报警 | **失败（已复现，A 相位）** | A4 00:28:54 → 轻触静音（守护保持） → A5 00:45:10 同一值 23.4/35.8 再次震动；16m16s ≈ 900 s 节流到期 + 读取节拍；见 D2 |
| `SW-REP-02` 紧急重复 | **NOT EXECUTED** | 需 EMG 注入；紧急是否去重属产品安全范围决定 |
| `SW-REP-03` 进程重启后同一缓存再报警 | **未隔离（NOT EXECUTED）** | 00:14:13 系统强制停止后 00:28:54 确实再次报警，但距上次副作用已 >900 s，无法区分“进程重启清零”与“节流到期”；需固定窗口单独复测 |

### 5.4 停止与守护

| 用例 | 结果 | 依据 / 缺口 |
|---|---|---|
| `SW-STP-01` 点「停止」后铃声+震动停止 | **失败（已实测）** | 点击后 +9 s/+45 s 仍在震动与播放；守护已关但播放器未停；见 D1 |
| `SW-STP-02` 空白区域取消（回归） | **部分失败（已实测）** | 内容顶部窄条 (360,120) 可停；卡片下方/标题栏空白不能停；见 D3 |
| `SW-STP-03` 紧急报警中停止 | **NOT EXECUTED** | 需 EMG 注入 |
| `SW-GRD-01` 停止守护后不再普通报警 | **PASS（窗口内）** | 守护关（23:55:56 起）；00:03:43 成功读取同一越限值：无铃声/震动；`temperature` 行未新增；`alarm_events` 仍按设计记录 |
| `SW-GRD-02` 重新启动守护后新事件仍报警 | **部分通过** | 00:07:18 重启守护，00:13:34 同一未变化值再次报警（节流已过）→ “守护重启后可再报警”成立；**“新值/新事件”未验证**（无注入构建，值无法改变） |
| `SW-GRD-03` 停止守护 + 紧急 >65 | **NOT EXECUTED** | 需 EMG 注入；产品安全例外需范围决定 |

### 5.5 完整复测、构建与回归

| 用例 | 结果 | 依据 / 缺口 |
|---|---|---|
| `SW-RET-01` 修复版完整复测传感器→手机 | **NOT EXECUTED** | 无修复版；传感器本轮未产生新 433 上报；本轮只证明“网关缓存→手机” |
| `SW-BLD-01` 交付身份与测试钩子 | **PASS（当前候选）** | 安装字节=`80232c50…`；vC10/vN1.9；未引入任何测试钩子/测试 action；debug 构建（非正式发布包） |
| `SW-REG-01` 页面/服务/通知回归 | **PASS（可达性）** | 首页/设定/我的/传感器列表/早起报告均可达；首装默认值显示 35/29/75/40/8/10；通知渠道 `alarm_channel`、`MqttServiceChannel` 存在；无 FATAL/ANR |
| `SW-REG-02` 有界稳定性 | **通过（有限）+ 风险记录** | ~1 h 无崩溃/ANR；BLE 读取 8 成功 / 10 次最大重试失败；两次系统 `SwipeUpClean` 强制停止（外部）；见 D6 |
| `SW-REG-03` 宿主机纯逻辑回归 | **PASS** | `./gradlew :app:testDebugUnitTest --offline --rerun-tasks`：39 套件 / 300 项 / 0 失败 0 错误 0 跳过；只能证明纯逻辑，不替代真机 |

---

## 6. 未执行项（明确写 NOT EXECUTED，不写成通过）

| 项 | 原因 | 需要的条件/责任 |
|---|---|---|
| 注入 `NORM/THR/THR_NEW/DROP_BASE/DROP_CUR/EMG/EMG_NEW` 全部受控值 | 当前量产固件 `LIGHT_DARK_THRESHOLD_CODE=0`、现场有光、不可调温/遮光，传感器不产生 433 上报；无固件测试构建 | **固件实现**：按 TD-EMB-ALM-001 §4 提供默认关闭、可排除出正式构建、带 `TINJ/P` 可区分标记的测试构建 |
| 正常值不误报（30.0/55.0） | 同上 | 同上（并需清理旧基准） |
| 下降报警、紧急报警、紧急中停止、停止守护+紧急 | 同上 | 同上；紧急去重/守护例外属产品安全范围决定 |
| “重新启动守护后**新事件**报警”的新值部分 | 值无法改变（仅同一缓存 23.4/35.8） | 固件测试构建 |
| 传感器→网关**新 433 事件**→手机同一事件 | 传感器本轮无发送；`S` 行 `r=0`、无 `P/G` | 固件测试构建 + 嵌入式执行对齐窗口 |
| 修复版完整复测（B/C 相位） | 无修复版构建 | Android 实现修复 D1/D2/D3 后重建 |
| 报警通知用户可见性 | MIUI 未显示 `alarm_channel` 记录（`muted 11/11`）；无法区分 App post 与 ROM 过滤 | 标准 Android/关闭 MIUI 过滤复测；或实现补充通知诊断 |
| 平台/云/数据库/iOS | 明确不在本轮范围 | — |
| 调温/调光/功耗测量 | 用户明确当前不能提供 | — |

---

## 7. 原始证据索引（均在 `evidence/sw_alarm_chain_20261126/`）

| 文件 | 内容 |
|---|---|
| `01_dumpsys_package.txt` | 安装包身份、versionCode/Name、安装时间、签名摘要、权限 |
| `02_env_state.txt` | 定位/蓝牙开关、运行时权限、数据库目录 |
| `03_app_logcat.txt` | **整窗连续原始 logcat**（2026-11-26 23:40 – 2026-11-27 00:50，约 6.5 MB；含 BLE、数据库、系统事件、强制停止） |
| `04/05/35/37/44/52_prefs*.txt` | 注册、网关保存、开关被改、恢复、最终状态的 SharedPreferences（未含凭据） |
| `06/07/11/12/13/23/24/25/26/27/28/29/32/33/38_*.txt` | 按时间窗的 App 标签日志（MqttService/Gateway*/DataProcessor 等） |
| `10/14/31/40/41/53_notification*.txt` | 报警前后 `dumpsys vibrator/audio/notification` 原文 |
| `15/16/17/18/19/20_*.txt` | **停止按钮与空白区域**点击前后震动/播放器状态（D1/D3 直接证据） |
| `39_alarm_poll.txt` | 每 12 s 的 `mCurrentVibration` 轮询：记录 A4 `startTime 00:28:54`、A5 `startTime 00:45:10` |
| `42_repeat_alarm_A5.txt` | A5 时读取日志、守护/开关状态 |
| `30_sideeffect_history.txt`（工具原始输出另存于会话） | 震动历史（含 23:54:37、00:13:34、00:28:54、00:45:10） |
| `db_bwtemperature_database1.sqlite`、`db2/3/4/final.sqlite` (+journal) | 各阶段本地库副本：`temperature` 行、`alarm_events` 行 |
| `54_db_final_query.txt` | 最终库查询：6 条读数、8 条 threshold 事件 |
| `60/61_*.png`、`62/63_*.xml` | 首页报警卡片、重启后首页、报警时 UI 树 |
| `50/51_gradle_unit_tests*.log` | 宿主机单测（rerun=实际执行）；39 套件/300 项/0 失败 |
| `45_input_accessibility.txt` | 无障碍服务为空、输入设备仅真实触摸屏（用于 D4 归因边界） |
| `query_db.py`、`poll_alarm.py` | 可复跑的只读测试脚本 |

---

## 8. 结论与交接

1. **本轮结论：TEST_FAIL。** 已实测的产品缺陷：
   - **D1**：点「停止」不停止铃声/震动（Project Goal 核心要求失败）；
   - **D2**：同一未变化缓存值按约 900 s 周期反复报警（Project Goal 核心要求失败）；
   - **D3**：空白区域静音仅在内容顶部窄条生效；
   - **D4**：报警开关在无测试保存动作时变为 0（根因未定，需调查）；
   - **D6**：报警通知在本机不可见（App vs MIUI 未分离）。
2. 已通过的部分仅证明：当前安装字节身份、真实网关→App 的 BLE 读取/显示/落库、阈值报警的铃声+震动路径、停止守护后的普通报警门控、页面可达性和宿主机单测。**不代表传感器→网关新 433 事件、正常值不误报、下降/紧急、修复版完整链路或正式发布通过。**
3. 交回实现（Android）：
   - 修复 `HomeFragment.toggleButton()` 停止分支，调用 `RingtonePlayer.stopAlarm()`/`VibrationPlayer.stopVibration()`（并保留既有空白触摸路径），修后重测 D1/D3；
   - 普通报警按“同一 episode/值未变化”去重，而不只是 900 s 时间节流；明确进程重启后策略；修后以同一缓存值复测 D2；
   - 调查 D4（报警开关被改为 0）是否与 App 写 prefs 路径有关；补充必要的诊断日志；
   - 排查报警通知投递（`alarm_channel`/`sendAlarmNotification`）在 MIUI 上的可见性，并给出可验证的投递结果（或说明系统过滤并给出用户设置指引）。
4. 交回实现/嵌入式（固件）：提供默认关闭、正式构建排除的测试注入/强制上报构建（`NORM/THR/THR_NEW/DROP_BASE/DROP_CUR/EMG/EMG_NEW` + `TINJ/P`），以完成正常值、下降、紧急、新事件与传感器→手机同一事件验证。
5. 环境提示：测试期间出现两次非测试触发的 `MiuiGesture`/`SwipeUpClean` 强制停止与 Toast（00:14、00:45），已在证据中标注；建议后续在无幽灵触摸/关闭 MIUI 清理策略的稳定条件下复测关键窗口。

**修复后必须对新的 App 构建（记录新的 SHA-256/versionCode/源码提交）在同一真实网关、同一 `uid4`、默认参数下重做：`SW-VER-01/02/03`、`SW-REP-01`、`SW-STP-01/02`、`SW-GRD-01/02`；并在取得固件测试构建后补齐 `SW-DSP-02`、`SW-NF-01`、`SW-ALM-02/03`、`SW-RET-01`。**
