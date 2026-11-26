# 软件测试设计（TD-SW-ALM-001 rev 1.0 / 安卓报警、停止与守护真机链路）

状态：软件测试设计产物（`software_tester.software_test_design`，输出端口 `test_design`）。**本文件只做设计，不执行安装、启动、ADB 操作、UI 操作、串口/烧录、网关或平台访问。** 设计时的实际检查仅限：仓库/源码/APK/文档只读核对，以及一次 `usb_list_devices` 只读枚举（§1.4）；未使用 ADB、串口、逻辑分析仪、烧录器或平台入口。

对象：`dengbei14` 当前成果上的**安卓 App（Redmi 8A / Android 10 / API 29 / `12f046690506`，`com.jinyuni.dengbei_care`）及其到既有传感器→网关的真实链路**。传感器与网关的嵌入式侧设计见 `artifacts/embedded_test_design_alarm_chain.md`（TD-EMB-ALM-001 rev 1.0），本设计与其用例 ID 对齐；跨端联合编排见 `artifacts/software_e2e_plan_alarm_chain.md`。

本轮目标来源：当前 Project Goal（核对实际固件/App 版本；覆盖温湿度采集、网关转发、手机显示、正常读数不误报、异常报警；优先复现“反复报警”“点停止关不掉”；修复后重建并真机独立复测；必须验证取消后铃声/震动停止、停止守护后不再报警、重新启动守护后新事件仍能报警；最后用修复版完整复测传感器到手机链路；网关只转发，平台/云上传/数据库/iOS 不纳入）。

**结论先行**：本方案在当前条件下一致、可设计，`DESIGN_READY` **只表示设计可执行，不表示任何 App 功能、链路或修复已执行/通过**。执行前必须知道以下可执行性事实：

1. **App 实际安装身份必须重新核对**。本能力设计时未操作手机；2026-11-26 的 E2E-JOINT-P1 窗口曾记录安装 `vC10 / vN1.9`、设备 `base.apk` SHA-256 与候选一致；嵌入式设计另一时点的只读枚举记录“未安装”。两者都不是本轮执行时的当前状态；执行阶段先按 `SW-VER-01` 取实际字节。
2. **当前源码的停止路径存在明确缺陷假设**：`HomeFragment.toggleButton()` 的“停止”分支只发送 `START_STOP_GUARD`，不调用 `RingtonePlayer.stopAlarm()` / `VibrationPlayer.stopVibration()`；两者只由 `BaseActivity.onTouchEvent()` 调用，而按钮等子控件会消费触摸。这是待真机复现的缺陷假设，不能把代码阅读当作已复现或已修复（§1.3）。
3. **优先缺陷可以先不依赖固件测试构建复现**：若执行时网关缓存的当前真实读数在默认阈值之外（默认 `tempmin=29`、`hummin=40`），App 会在守护中对其产生普通超限报警；用同一未变化缓存读数可以复现“反复报警”，也可以在报警中点击「停止」复现“关不掉”。这条路径证明的是**网关→App 的当前真实 BLE 链路**，不证明本轮传感器→网关的新 433 事件。
4. **受控的正常值、下降、紧急、新事件与“传感器→手机同一事件”必须使用固件测试构建的注入/强制上报**（TD-EMB-ALM-001 §4：`NORM/THR/THR_NEW/DROP_BASE/DROP_CUR/EMG/EMG_NEW`）。当前量产固件 `LIGHT_DARK_THRESHOLD_CODE=0`、现场有光且用户不能调温/调光，传感器不会自然满足上报判据。若执行阶段没有该测试构建，相关用例记 `NOT EXECUTED`，不得写成通过，交已审批的失败分析→固件实现闭环。
5. **停止守护后的“紧急 >65 ℃ 仍报警”和“同一紧急读数是否重复报警”是产品安全/范围决定**。当前产品文档明确 >65 ℃ 不受守护停止影响、且每次上报都提醒；本设计把普通报警的“停止后不再报警”作为强制判据，把紧急例外与紧急重复列为必须记录的观察/决定项，不擅自改写产品安全规则。

---

## 0. 范围与目标映射

### 0.1 本轮必须覆盖

| 编号 | 目标 | 本设计用例 | 备注 |
|---|---|---|---|
| G1 | 核对实际运行的 App 版本/身份 | `SW-VER-01`、`SW-VER-03` | 安装字节、versionCode/Name、签名、源码提交；不以构建产物代替安装态 |
| G2 | 真实温湿度采集 | 引用 `EMB-ACQ-01` | 传感器侧 COM43 证据；App 侧只观测转发结果 |
| G3 | 经既有网关 433→BLE 转发到手机 | `SW-DSP-01`、`SW-DSP-02`、`SW-RET-01` | 网关只转发；不烧录/不读取网关版本 |
| G4 | 手机显示读数 | `SW-DSP-01`、`SW-DSP-02` | 卡片、详情、本地库 |
| G5 | 正常读数不误报 | `SW-NF-01` | 默认参数 + 注入 30.0/55.0；真实环境读数另记 |
| G6 | 异常报警 | `SW-ALM-01/02/03` | 超限、下降、紧急 |
| G7 | 复现/修复“反复报警” | `SW-REP-01/02/03` | 普通重复为强制；紧急重复按范围决定 |
| G8 | 复现/修复“点停止关不掉” | `SW-STP-01/02/03` | 按钮路径强制；空白区域为回归 |
| G9 | 停止守护后不再报警 | `SW-GRD-01`、`SW-GRD-03` | 普通报警强制；紧急例外另记 |
| G10 | 重新启动守护后新事件仍报警 | `SW-GRD-02` | 新值 + 新 episode |
| G11 | 修复版完整复测传感器→手机 | `SW-RET-01`、`SW-BLD-01` | 固定版本/构建身份；平台不纳入 |
| — | 构建与安装身份、回归 | `SW-BLD-01`、`SW-REG-01/02/03` | 正式构建入口与实际启动 |

### 0.2 明确不纳入

- 平台/云上传/平台数据库/iOS：不做 MQTT 平台一致性、`/upload_data`、MySQL 回读或 iOS 判定；测试期间建议手机进入飞行模式后单独开启蓝牙，任何平台/云观测不进入本轮通过条件。
- 网关固件：不烧录、不修改、不读取版本；网关只作为 433 接收与 BLE FFE2 缓存的转发者。
- 温度、光强、功耗的主动调节/测量：不安排调温、遮光/照射、电流/功耗测量；真实 `S/V/D/VDD` 只作观测。
- App 产品源码修改：本能力不修改；修复由 Android 实现能力提供并重建（§8）。
- 固件测试构建本身：由固件实现能力提供（TD-EMB-ALM-001 §4）；本设计只消费其固定模式与 `TINJ` 观测。

---

## 1. 当前基线（设计时只读核对）

### 1.1 仓库与 App 源码身份

| 项 | 值 | 来源/说明 |
|---|---|---|
| 仓库/分支 | `D:\mypro\beiwo2\AIP`，`main`，HEAD `bf4327dd488426deebfd6ca5b859e5f3cd9a3a95`，接手 `git status` 干净 | `git status --porcelain`、`git rev-parse HEAD` |
| Android 产品源 | `dengbei_care/app/src/main` 与 `app/build.gradle.kts` 相对实现基线 `a12d12b570d541a9d348a1b6d6846275d871c7d4` **零差异** | `git diff --stat a12d12b..HEAD -- dengbei_care/app/src/main dengbei_care/app/build.gradle.kts` |
| App 候选 | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk`，10,370,964 B，SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`，`versionCode 10` / `versionName 1.9`，debug | 本轮 `sha256sum`/`stat`；`output-metadata.json`；AB-003 |
| 历史安装包 | `dengbei_care/爱守护.apk`，8,144,628 B，SHA-256 `a62046cc0b7b2ca401159b48daa1b95459af70becc2d6d9bbdbddeb27c38aaf8`（历史 vC1/vN1.0） | 本轮 `sha256sum`/`stat`；**不得用于本轮安装** |
| 构建入口 | `cd dengbei_care && ./gradlew :app:assembleDebug --offline`（Gradle 8.4 / AGP 8.3.2 / JDK 21.0.2） | AB-003；本轮未构建 |
| 宿主机单测入口 | `./gradlew :app:testDebugUnitTest --offline` | AB-003 记录 39 套件/300 项 0 失败；本轮未运行 |
| 版本声明 | `truth/current_versions.yaml` 为空 `{}`；`truth/engineering_baseline.yaml` 为 `null` | 不能据此推断实际安装/运行版本 |
| 代码图 | 传感器/网关/Android 三个本地根已索引、`complete`、`reindexRecommended=false`；索引源 HEAD `43fe3b3`，其后 Android 产品源未变 | `artifacts/codegraph_alarm_chain_index.md` |

> 设计时 `usb_list_devices`（2026-11-26T14:54:25Z）只证明 USB 身份在位：目标手机 `18D1:4EE7` 实例含 `12F046690506`；CW-DAPLink `C251:F001`；Prolific `COM43`；Cypress FX2 `04B4:8613`。**不证明** ADB 已授权、App 已安装、传感器在线或网关可用；执行前重新枚举。

### 1.2 App 侧关键行为事实（当前源码）

| 项 | 事实 | 位置 |
|---|---|---|
| BLE 读取周期 | 网关地址存在时每 `SensorCadence.SAMPLE_PERIOD_MS = 180000 ms` 读取一次；关闭守护**不停止**读取；保存网关地址会触发一次立即读取 | `MqtttService.kt:260/384-406`；`SensorCadence.kt:11-17` |
| 每次读取逐条处理 | 对每个有效样本调用 `processTemperatureHumidityData`，同一网关缓存值在每次读取都会被重新评估 | `MqtttService.kt:977-1028` |
| 报警判定 | `AlarmEvaluator`：紧急 `temp > 65.0` 优先；下降为 5/10/15 min 时间窗基准差 **温度>tempDrop 且 湿度>humiDrop**；超限为 `temp>max || temp<min || hum>max || hum<min`（严格）；超限文案覆盖下降 | `AlarmEvaluator.kt:104-178` |
| 普通报警节流 | 每设备内存 `lastAlarmTime`，`now-last > 900_000 ms` 才再次产生震动/铃声/通知；`alarm_events` 不受该节流 | `MqtttService.kt:1212-1231`、`:1156-1174` |
| 紧急报警 | `>65.0 ℃` 每次读取都触发震动/铃声/通知，不受 900 s 节流、不受震动/响铃开关影响 | `MqtttService.kt:1135-1147`；`AlarmEvaluator.kt:112-115` |
| 守护门控 | 普通震动/铃声/通知仅在 `homeViewModel.startData.value == 1` 时触发；本地读数仅在 `startData==1` 时入库；`alarm_events` 和卡片状态不受守护门控 | `MqtttService.kt:1176-1200`、`:1213` |
| 停止按钮 | `HomeFragment.toggleButton()` 停止分支只 `startService(START_STOP_GUARD, guardaction="0.0")` 并写 `measure_state="starting"`；**不调用播放器停止** | `HomeFragment.kt:163-210` |
| 空白区域停止 | 只有 `BaseActivity.onTouchEvent()` 的 `ACTION_DOWN` 调用 `RingtonePlayer.stopAlarm()` / `VibrationPlayer.stopVibration()` / `clearAllDeviceAlarms()` | `BaseActivity.kt:15-24` |
| 播放器 | 铃声 `MediaPlayer` 循环；震动 API26+ `createWaveform(..., repeat=0)` 无限循环；停止为单例静态字段 | `RingtonePlayer.kt:21-63`；`VibrationPlayer.kt:24-71` |
| 显示格式 | 卡片温度 `"%.1f°C"`、湿度 `"%.0f%%"`；状态 chip `未收到/正常/警告/紧急`；卡片报警文案在 severity != NORMAL 时显示 | `DeviceCardAdapter.kt:57-110` |
| BLE 帧解码 | 头部 16 B + 每设备 16 B（前 8 B：`sensor_id4 | humidity_u16be@4 | temperature_s16be@6`）；`devCount=frame[6]`；长度不足/未对齐拒绝 | `GatewayFrameCodec.kt:98-150` |
| 读取完整性守卫 | MTU ≥ `16*(1+N)+3`；返回长度 16 B 对齐且 ≥ 期望；最大 9 台 | `GatewayReadGuard.kt:17-84` |
| 读取终态 | `DATA_OK / GATEWAY_NO_DATA / GATEWAY_NO_DEVICE / READ_INCOMPLETE / READ_INVALID` | `GatewayReadState.kt:10-36` |
| 本地库 | `bwtemperature_database1`：`temperature(time,device_id,temperature,humidity)` 主键 `(time,device_id)`；`alarm_events(time,device_id,type,message)`；类型 `drop|threshold|emergency` | `TemperatureDatabaseHelper.kt:29-86/214-239` |
| 通知 | 渠道 `alarm_channel`，标题 `[设备名] 报警`，内容为报警文案；`areNotificationsEnabled()` 为假时静默不发 | `MqtttService.kt:1461-1513` |
| 首装默认 | `tempdrop=2`、`humdrop=0.1`、`tempmax=35`、`tempmin=29`、`hummax=75`、`hummin=40`、`guardtime=8`、`delayalarm=10`、四开关=1 | `MainActivity.kt:139-166` |
| 每设备参数 | 读取 `dev_<devId>_<param>`，缺省回退全局 key，再无则代码默认 | `DeviceParamsBook.kt:39-48`；`MqtttService.kt:181-200` |
| 网关缓存 | 收到有效 433 帧后更新并保留；无新帧时 `timeoutcnt=100` 递减（任务周期 10 s，约 ≤1000 s）后清零占位；期间 FFE2 反复返回**同一缓存值** | `app_um2006A.c:96/161-227` |

### 1.3 缺陷假设（只读代码事实，未复现）

| 假设 | 机制 | 本设计的区分实验 |
|---|---|---|
| H1 同一未变化读数反复报警 | 网关缓存复读；App 每 180 s 重新评估；普通报警 900 s 节流后在 `lastAlarmTime+900 s` 再次产生副作用；紧急每次读取都产生副作用 | `SW-REP-01/02`：同一注入/缓存值持续，静音后观察 ≥900 s/下一次读取 |
| H2 点「停止」关不掉当前报警 | 停止按钮不调用播放器停止；只有空白区域触摸路径停止 | `SW-STP-01`（按钮）与 `SW-STP-02`（空白）对照 |
| H3 正常读数被默认下界误报 | 首装 `tempmin=29`；真实室温约 22 ℃ 会被判为低于下界 | `SW-NF-01` 用注入 30.0/55.0；`SW-NF-02` 记录真实读数并区分“环境/默认参数不匹配” |
| H4 紧急 >65 绕过守护与节流 | 紧急块在 `startData` 门控之前且不更新 `lastAlarmTime` | `SW-REP-02`、`SW-GRD-03`；重复/守护例外按范围决定 |
| H5 App 进程重启后同一缓存值再报警 | `deviceHistory`/`lastAlarmTime` 在内存；重启清零 | `SW-REP-03`（非阻塞观察） |

---

## 2. 链路、接口与关联键

### 2.1 链路

```text
GXHT40 温湿度 + 光照分压
  → CW32L010 采样（每 180 s）→ sensor_decide_report（>35.0 ℃ 或 升温>0.9 ℃ 且无光）
  → UM2005C 433 10 B 加密帧
  → 既有 CH592 网关：433 接收 → 按绑定 ID 组 16 B 块 → AES-ECB → FFE2 160 B 缓存
  → 安卓 App：每 180 s BLE 读取 → 解密/长度守卫/逐 ID 消费
  → AlarmEvaluator → 本地库 + 卡片/详情 + 铃声/震动/通知
```

### 2.2 关联键与可区分载荷

- 传感器 `BOOT uid=<8 位 hex>` 的前 4 字节（`uid4`）是板件与 App `MacIdBook` 的关联键；同板复位不变。
- 传感器不发送 RTC 时间；`S/P` 的 `k` 是 RTC 累计分钟节拍，只作窗口辅助。
- App 的 BLE 接收时间是**手机读取时刻**，不是传感器采样时刻；网关缓存复读会产生“同值不同秒”的 App 时间戳。
- 固件测试构建的注入值按 TD-EMB-ALM-001 §4.2 固定：`NORM 30.0/55.0`、`THR 36.0/55.0`、`THR_NEW 36.2/55.0`、`DROP_BASE 34.9/60.0`、`DROP_CUR 32.0/55.0`、`EMG 66.0/55.0`、`EMG_NEW 67.0/55.0`（0.1 单位）。App 日志/本地库/UI 必须与注入值逐项一致。
- 同一事件的关联顺序：`TINJ/P` 冻结值 → 网关缓存 → App `id=uid4, T/H` 日志 → 本地库 `temperature` 行 → 卡片 UI → 报警副作用/`alarm_events`。不以“时间接近”猜事件；用 `uid4 + 注入值/模式 + 采集窗口 + 读取时刻` 关联。
- 同一未变化值的重复读取无法靠数值区分；用**模式切换**（如 `THR → THR_NEW`）、读取序号、点击/静音时刻和 `lastAlarmTime` 窗口区分。

### 2.3 观测方法（执行阶段）

| 端/对象 | 观测 | 具体方法 | 能证明 / 不能证明 |
|---|---|---|---|
| App 版本 | 安装字节 | `dumpsys package` 取 versionCode/Name/firstInstallTime/lastUpdateTime；`pm path` + 拉取 `base.apk` 复算 SHA-256；`apksigner verify --print-certs` | 实际安装字节；不证明业务功能 |
| App 日志 | 读取/判定/入库 | `adb logcat -v threadtime`，关注 `MqttService`/`DataProcessor`/`TempDb`/`AlarmNotify`/`RingtonePlayer`/`VibrationPlayer`/`AndroidRuntime`/`ActivityManager`；保存整窗原始日志 | 读取/解码/判定/入库路径；不证明物理量 |
| App UI | 卡片/详情/守护按钮 | `uiautomator dump` + `screencap`；读取 `startGuardButton` 文本、卡片 `statusChip/tempValue/humiValue/alarmMessage` | 显示与用户可见状态；不证明内部状态 |
| 铃声 | 当前播放 | `adb shell dumpsys audio`（活跃 player/uid/state/历史）；必要时操作员听感/录像并记录时刻 | 系统层面是否有 App 活跃播放；不量化音量/不证明用户听到 |
| 震动 | 当前震动 | `adb shell dumpsys vibrator`（`mCurrentVibration`/历史，按实际 ROM 输出记录）；操作员触感记录 | 当前是否在震动；不量化幅度 |
| 通知 | 报警通知 | `adb shell dumpsys notification --noredact`（标题/内容/渠道/时间） | 通知是否存在；不证明用户已看到 |
| 本地库 | 读数/报警事件 | `adb exec-out run-as com.jinyuni.dengbei_care cat databases/bwtemperature_database1`（连同 `-wal/-shm`）到主机副本；只读查询 `temperature`/`alarm_events` | App 入库与事件计数；不证明平台/云端 |
| 参数与守护状态 | MyPrefs | `run-as ... cat shared_prefs/MyPrefs.xml`；记录 `measure_state`、`guardtime`、四个报警开关、`registerphone`、`mac_addr`、`gateway_ble_addr`；证据中不回显凭据 | 本机设置/守护持久状态；`startData` 本身不可直接读，用按钮文本+行为+prefs 辅助 |
| 链路状态 | 读取终态 | App 日志 `link_state=...` / UI `linkStatusText` | 一次读取的终态；不证明传感器物理在线 |
| 网关身份 | BLE MAC/名称/`GwID` | App「网关与上传设置」扫描结果 + 日志 `GwID=` | 网关身份；无版本回读 |
| 传感器侧 | COM43/`TINJ/P/S` | 由嵌入式执行能力按 TD-EMB-ALM-001 §2.3 采集 | 采样/判定/帧值/发送结果；不证明 App 消费 |

---

## 3. 测试策略与可证明边界

### 3.1 分层

| 层 | 使用条件 | 目的 | 证据边界 |
|---|---|---|---|
| L0 真实 App + 真实网关缓存 | 当前网关缓存有有效读数；不依赖固件测试构建 | 证明当前 App 对**网关→手机**真实 BLE 读数的显示、普通报警、重复报警、停止按钮 | 不证明本轮传感器→网关新 433 事件；缓存值不是物理采样时刻 |
| L1 真实 App + 真实网关 + 固件测试构建注入 | 固件测试构建已由实现提供并烧录 | 受控 `NORM/THR/DROP/EMG` 走真实判定/冻结/组帧/CRC/Feistel/433/网关/App | 注入值不是物理量；不证明传感器对注入值的物理响应 |
| L2 App 测试版输入边界注入（条件性回退） | 固件测试构建暂不可用、需要隔离 App 缺陷时 | 在 App BLE 输入边界回放受控帧，走既有解密/守卫/判定/入库/UI/报警 | 不证明 BLE 传输、433、网关；正式候选必须去除 |
| L3 宿主机单测/静态核对 | 任意阶段 | `AlarmEvaluator`/`Codec`/`ReadGuard` 等纯逻辑与构建身份 | 不能替代真机功能/链路 |
| L4 修复版完整复测 | 修复版 App + 同一测试构建 | 复跑全部强制用例 | 只对实际安装字节/窗口有效 |

### 3.2 相位与版本绑定

| 相位 | App 构建 | 传感器构建 | 目的 |
|---|---|---|---|
| A 缺陷复现 | 执行时实际安装的当前候选（`SW-VER-01` 记录） | 当前板上/发布固件 + 固件测试构建（如可用） | 复现“反复报警/点停止关不掉”，记录当前行为 |
| B 修复验证 | Android 实现修复后重建的候选（记录新 SHA-256/versionCode/源码提交） | 同一测试构建 | 全部强制用例逐项复测 |
| C 完整回归 | B 的修复版 App | 测试构建 + 发布固件 | 最后完整复测传感器→手机；随后由嵌入式恢复发布固件 |
| D 交付身份 | B/C 的 App 字节 | 发布固件（嵌入式负责） | `SW-BLD-01` 记录交付身份与测试钩子排除 |

每个相位开始时记录仓库 HEAD、App APK/设备 `base.apk` SHA-256、源码提交、传感器 axf/hex 摘要与 `Verify OK`、`BOOT uid`、网关 MAC/`GwID`；相位之间不得混用旧结论。

### 3.3 现实条件与替代选择

- 真实环境优先：传感器、网关、手机、App 均使用当前实物；不因本机模拟方便而改用桩替代真机。
- 真实物理事件不可自然触发（不调温/不调光、量产固件判据禁用）是**当前可执行性缺口**，不是把功能失败改成前置缺失。受控注入位于传感器采样后的判定输入边界（TD-EMB-ALM-001 §4），保留真实判定/组帧/发送/网关/App 链路；注入本身不证明物理响应。
- 若固件测试构建不可用：
  - `SW-DSP-01`、`SW-ALM-01`（缓存值越限时）、`SW-REP-01`、`SW-STP-01`、`SW-GRD-01/02` 的普通路径仍可用 L0 真实缓存路径执行（若缓存值满足条件）；
  - `SW-DSP-02`、`SW-NF-01`、`SW-ALM-02/03`、`SW-REP-02`、`SW-GRD-02` 的新值路径、`SW-RET-01` 记 `NOT EXECUTED`，原因写“缺测试专用注入/强制上报构建”，不得写成通过。
- L2 App 测试版注入只用于隔离 App 缺陷/缩短定位循环，不能替代 L1/L4；其证据只证明 App 业务路径。它由 Android 实现提供，测试侧不修改产品源码。

---

## 4. 测试环境、准备与观测

### 4.1 设备与权限

- Redmi 8A / `12f046690506` / Android 10 / API 29；执行前确认 `adb devices -l` 为 `device`（不是 `unauthorized`），并确认 ADB 授权。
- 蓝牙开启；Android 10 扫描需定位总开关 + `ACCESS_FINE_LOCATION`；通知权限/渠道开启；系统默认闹钟铃声存在、媒体/闹钟音量 >0；关闭勿扰或确认不拦截；MIUI 省电策略加入白名单；保持屏幕常亮/不锁屏以便观察与点击。
- 建议飞行模式后单独开启蓝牙，排除平台/云上传；若现场不能离线，记录实际网络状态，并把任何 MQTT/HTTP/平台观测排除在本轮判据之外。
- 执行前记录 `MyPrefs` 关键键（`measure_state`、`guardtime`、四个报警开关、`registerphone`、`mac_addr`、`gateway_ble_addr`），证据中不出现任何凭据；执行后恢复原值（除非该值是本轮测试数据）。

### 4.2 绑定与前置

1. 执行时重新枚举 USB/ADB；确认手机、CW-DAPLink、COM43、FX2 的实际状态。
2. 传感器 `BOOT uid4`（嵌入式执行提供）以 8 位大写 HEX 加入 App 设备列表（「我的」→「注册传感器」，名称按规则，例如 `T1`）；记录 `MacIdBook`。
3. App「我的」→「网关与上传设置」：正常扫描发现并保存实际网关；记录 MAC/名称/RSSI；不得把默认迁移值 `84:C2:E4:03:02:02` 当作已确认，除非扫描/用户确认。保存/重存网关可触发一次立即读取（产品行为），记录时刻。
4. 记录网关 `devCount`/`GwID`/`valueSize`/MTU 及绑定设备 ID；若网关已有其它设备，记录并排除其报警，不修改网关。
5. App 报警参数使用首装默认值；执行前从「设定」页读取并记录实际值；若存在 `dev_<uid4>_*` 独立覆盖，清除或改用全局值，执行后恢复。不得把改参数当作通过手段。

### 4.3 观测准备顺序（接收端先就绪）

1. 启动 `logcat` 整窗采集（含清空缓冲或记录起点时间）。
2. 记录 UI 基线（首页/设定/我的）、`dumpsys vibrator/audio/notification` 基线、本地库行数与 `alarm_events` 计数（主机副本）。
3. 记录 App/传感器/网关版本与身份。
4. 以上就绪后，才触发传感器注入/复位/网关缓存读取；不得先触发后补观测。

### 4.4 App 测试版输入边界的条件性最小接口（仅回退时）

若需要在不依赖固件测试构建时隔离 App 缺陷，Android 实现可提供一个**仅测试构建**启用的最小输入边界钩子：

- 入口：debug/test 专用 `Intent` action（例如 `com.jinyuni.dengbei_care.TEST_FEED_FRAME`）或测试 source set 中的调用；只在测试构建编译/启用，正式候选不含该 action/组件。
- 载荷：在 BLE 读取回调拿到 FFE2 字节之后、解密之前注入受控 160 B **密文帧**（AES-ECB/NoPadding，使用当前源码中的项目 AES key，由测试侧按协议构造）；必须走既有解密、`GatewayReadGuard` 长度判定、`GatewayFrameCodec` 解码、`processBleSamples`/`processTemperatureHumidityData`、入库、UI、报警路径。
- 约束：不直接写数据库、不直接调用 `RingtonePlayer`/`VibrationPlayer`、不伪造发送成功；不改变判定阈值/文案/节流语义。
- 排除验证：正式候选 APK 中无该 action/组件/测试符号；`SW-BLD-01` 记录并复核。
- 证明边界：只证明 App 从帧解码到用户结果的路径；不证明真实 BLE/433/网关/传感器。

> 上述接口是条件性回退，不是本轮首选；首选仍是 TD-EMB-ALM-001 §4 的固件测试构建 + 真实网关。

### 4.5 测试运行器、自动化与超时（执行阶段）

- 测试侧运行器放在 `scripts/`（例如 `scripts/sw_alarm_chain_runner.py`），只使用 ADB/uiautomator/logcat/dumpsys/`run-as` 和主机 SQLite 副本；不修改产品源码、不改权限文件。执行能力按本设计自行补齐依赖（Python `sqlite3`、ADB），并在证据中记录实际版本。
- 建议超时：`adb devices -l` 30 s；安装/覆盖 120 s；`uiautomator dump` 30 s；单次 `dumpsys` 30 s；DB 拉取 60 s；`force-stop`/启动 60 s；单个等待窗口 ≤7 min（2 个读取周期+裕量）；重复/守护窗口按 §5 的 ≥20 min/≥900 s。超时记录为“未观测到”，不得自动判产品失败。
- 断言方式：UI XML 按 `resource-id`/文本定位（`startGuardButton`、`statusChip`、`tempValue`、`humiValue`、`alarmMessage`）；logcat 按 `MqttService`/`DataProcessor`/`TempDb`/`AlarmNotify` 正则解析；DB 查询 `temperature`/`alarm_events`；`dumpsys` 原文保留并人工/脚本双重核对。
- 证据目录：`evidence/sw_alarm_chain_<UTC>/`，包含 `01_version.txt`…`99_verdict.yaml`、UI dump/截图、原始 logcat、DB 副本、`dumpsys` 前后快照、用例判据表。manifest 只登记交付文件，原始证据按仓库规则保留。
- ADB/uiautomator 不可用或 `unauthorized`：属测试环境问题；在 test 权限内做有限修复（重新授权 USB 调试、`adb kill-server`/`start-server`、确认驱动），最多 3 次；仍不可用则记录实际错误，相关用例 `NOT EXECUTED`，不改产品源码、不把“USB 在位”写成“手机可操作”。
- 运行器必须能从本文件 + `software_e2e_plan_alarm_chain.md` + 证据目录独立复现；不得依赖本会话上下文。

---

## 5. 用例矩阵

> 每个用例独立判定。`A` = 缺陷复现相位，`B` = 修复验证相位，`C` = 完整回归。未特别说明时，前置均含 §4.1/§4.2 的版本、绑定、手机与观测准备；执行前重新枚举设备。

### 5.1 版本与身份

| ID | 目标/来源 | 前置 | 方法 | 判据 | 证据 |
|---|---|---|---|---|---|
| `SW-VER-01` | G1：App 实际安装字节 | 手机在线、ADB 授权 | `dumpsys package`；`pm path` + 拉取 `base.apk` 复算 SHA-256；`apksigner verify --print-certs`（若工具不可用，记录 `dumpsys package` 签名摘要并说明，不因此豁免字节核对）；记录 `firstInstallTime/lastUpdateTime` | 设备侧 `base.apk` SHA-256 与所测 APK 一致；versionCode/Name 与声明一致；debug 可调试；签名与候选一致；明确“实际安装”而非仅构建产物 | `dumpsys package` 片段、`base.apk` 摘要、`apksigner`/签名摘要、安装时间 |
| `SW-VER-02` | G3/联动前置：App 可观测链路身份 | §4.2 完成 | 读取 `MacIdBook`、`gateway_ble_addr`、App 日志 `GwID=`/`id=`/`dev=`/`mtu=`/`link_state=`；记录 `devCount`/`valueSize` | 绑定 `uid4` 与传感器 `BOOT uid4` 一致；网关 BLE 地址为用户确认/扫描保存；App 读到 `dev>=1` 且 `id=uid4`；明确网关固件版本“未知/无回读” | prefs、日志、UI、网关候选截图 |
| `SW-VER-03` | G1：修复版构建身份 | 修复版已构建 | 记录源码提交/构建命令/编译器/APK SHA-256/versionCode/Name/签名；与 B 相位一致 | 修复版身份可追溯到源码提交与实际 APK；与 A 相位可区分；设备 `base.apk` 与之一致 | 构建日志、APK 摘要、`dumpsys package` |

### 5.2 显示与正常读数

| ID | 目标/来源 | 前置 | 刺激 | 观察与判据 | 证据 |
|---|---|---|---|---|---|
| `SW-DSP-01` | G3/G4：网关缓存→App 真实 BLE 显示 | 当前网关缓存有效；App 绑定/网关地址就绪；守护可开 | 启动 App/服务，等待或保存网关触发一次读取 | App 日志 `id=uid4, H=<h>, T=<t>`；卡片温度 `%.1f°C`、湿度 `%.0f%%`（与日志四舍五入一致）、状态 `正常/警告/紧急` 与判定一致；详情页/本地库出现同一读数；`link_state` 落 `DATA_OK`（或按实际 `GATEWAY_NO_*` 记录）；无 `FATAL`/`ANR` | 日志、UI dump/截图、DB 查询、链路状态 |
| `SW-DSP-02` | G3/G4：受控注入值显示 | 固件测试构建 `NORM`；App 绑定 `uid4`/网关；**App 数据干净或 45 min 内无高位/下降基准**（否则 30.0 可能相对旧基准触发下降）；观测就绪 | 注入 30.0/55.0，等待 ≤2 个读取周期 | 传感器 `TINJ it=300 ih=550 f=1`、`P t=300 h=550`；App 日志 `id=uid4, T=30.0, H=55.0`；卡片 `30.0°C`/`55%`；详情/本地库同一值；`DATA_OK`；在干净基线前提下无报警副作用（正常值） | `TINJ/P`、日志、UI、DB |
| `SW-NF-01` | G5：正常读数不误报 | 固件测试构建 `NORM`；**在清数据/全新安装并重新完成 `uid4`/网关/默认参数配置后执行本用例**（避免旧高值成为下降基准；清数据后必须重新登记并记录）；守护启动 | 30.0/55.0 持续 ≥3 个读取周期（约 ≥9 min） | `30.0` 在默认 `[29,35]` 内、`55` 在 `[40,75]` 内；`AlarmEvaluator` 输出 `NONE`；`dumpsys vibrator` 无当前震动、`dumpsys audio` 无 App 活跃 player、无新 `alarm_channel` 通知；`alarm_events` 无新行；卡片 `正常` | 日志、UI、DB 前后计数、vibrator/audio/notification |
| `SW-NF-02` | 观察：真实环境读数与默认阈值 | 当前真实缓存/真实 `S` 读数；默认参数 | 守护中读取一次真实值 | 记录真实 T/H 与判定结果。若真实温度低于 `tempmin=29` 或湿度低于 `40` 而报警，**归类为“环境/默认参数不匹配”，不得据此判 App 误报**；若在范围内且无报警，记录为真实正常读数 | 日志、UI、DB、环境说明（不作测量结论） |

> `SW-NF-01` 的 30.0 ℃ 是代表性被窝内正常值，不是当前室温；当前室温由 `SW-NF-02`/`EMB-ACQ-01` 的真实读数记录。`SW-NF-02` 非阻塞，但必须记录。

### 5.3 异常报警

| ID | 目标/来源 | 前置 | 刺激 | 判据 | 证据 |
|---|---|---|---|---|---|
| `SW-ALM-01` | G6：超限报警 | 固件测试构建 `THR`；默认参数；守护启动；无旧报警/旧基准干扰；**距上次普通报警副作用 ≥900 s（或全新进程/数据），否则首次报警可能被 900 s 节流** | 注入 36.0/55.0，等待 ≤2 个读取周期 | `36.0 > 35` ⇒ `THRESHOLD/温湿度超限报警/WARNING`；`alarm_events` 新增 `type=threshold`；卡片 `警告` + 文案；`alarm_channel` 通知存在；App 震动/铃声响（`dumpsys` + 操作员）；首次报警发生在窗口内 | `TINJ/P`、日志、DB、UI、vibrator/audio/notification |
| `SW-ALM-02` | G6：下降报警 | 测试构建 `DROP_BASE`→`DROP_CUR`；守护启动；本地库保存基准；默认 `tempdrop=2/humdrop=0.1`；**距上次普通报警副作用 ≥900 s（或全新进程/数据）** | 基准 34.9/60.0；约 6 min 后当前 32.0/55.0；等待当前读取 | 基准不触发超限（34.9 在 `[29,35]` 内）；当前 32.0 也在范围内，但 `34.9-32.0=2.9>2` 且 `60.0-55.0=5.0>0.1` ⇒ `DROP/温度湿度下降报警`；`alarm_events type=drop`；卡片 `警告`；报警副作用存在 | 两个 `TINJ/P`、每次 App 日志、DB 两条读数、`alarm_events`、UI/播放器 |
| `SW-ALM-03` | G6：紧急报警 | 测试构建 `EMG`；守护启动；默认开关 | 注入 66.0/55.0，等待 ≤2 个读取周期 | `>65.0` ⇒ `EMERGENCY/紧急报警，温度超过65度，谨防火灾/ALARM`；`alarm_events type=emergency`；卡片 `紧急`；通知文案逐字核对；即使震动/响铃开关关闭也应震动+响铃（若执行该子项，记录实际） | 同 `SW-ALM-01` + 文案逐字 |

### 5.4 重复报警（复现与修复判据）

| ID | 目标/来源 | 前置 | 刺激 | 当前（A）预期/修复（B）判据 | 证据 |
|---|---|---|---|---|---|
| `SW-REP-01` | G7：同一未变化超限读数反复报警 | 测试构建 `THR` 或真实缓存越限值；默认参数；守护启动；**静音用空白区域点击，保持守护运行**；**距上次普通报警副作用 ≥900 s（或全新进程/数据），否则首次报警可能被节流** | 同一值每 180 s 读取，持续 ≥20 min（覆盖 ≥6 个读取周期与 ≥1 个完整 900 s 窗口）；首次报警后空白区域静音 | **A（复现）**：第一次普通报警后，在 `lastAlarmTime+900 s` 后的下一次读取再次出现震动/铃声/通知（记录第 2 次时间戳）。**B（修复）**：同一未变化值在一个报警 episode 内只产生一次普通报警副作用；第一次后 ≥900 s 且 ≥3 个读取周期内无第 2 次震动/铃声/通知；只有值变化/条件清除后重新越限才允许新 episode。`alarm_events` 的逐次记录单独统计，不作为唯一判据 | 每次 `TINJ`/App `id/T/H` 日志、播放器/通知时间、`alarm_events` 行、静音点击时刻、UI |
| `SW-REP-02` | G7：同一未变化紧急读数反复报警 | 测试构建 `EMG`；守护启动；静音用空白区域 | 66.0/55.0 持续 ≥20 min（≥6 个读取周期）；随后切换 `EMG_NEW` 67.0 一次 | **A（复现）**：同一 66.0 在每次 BLE 读取（约 180 s）都触发紧急报警（紧急不节流）。**B（修复/范围）**：是否要求同一 66.0 只报警一次属产品安全/范围决定；若用户确认“反复报警”包含紧急，则按 B 判据（同一值一次，67.0 新事件报警）；若用户确认紧急每次提醒是安全例外，则记录实际并保留。**未确认前不把它作为普通 `SW-REP-01` 的阻塞条件。** | 同 `SW-REP-01` + 67.0 对照 + 用户决定记录 |
| `SW-REP-03` | 观察：App 进程重启后同一缓存值再报警 | 测试构建 `THR`/`EMG` 或真实缓存越限；已产生一次报警 | 报警结束后 `am force-stop com.jinyuni.dengbei_care`，重新启动 App/服务，保持守护，等待下一次读取 | **A**：因内存 `deviceHistory/lastAlarmTime` 清零，同一未变化值再次报警。**B**：若修复声明覆盖进程重启，则同一值不重复；仅新值/重新越限才报警。记录重启时刻、首次读取时刻、是否报警。**非阻塞观察项**（Project Goal 未逐字要求进程重启，不据此追加退出条件）。 | `force-stop`/启动日志、App 日志、`alarm_events`、vibrator/audio/通知 |

> `SW-REP-01` 的 20 min 窗口是区分“900 s 节流后复报”与“修复后不重复”的最小窗口；执行可分段，但必须记录窗口起止与间隙，不得把分段拼成连续未验证。
>
> 静音必须用**空白区域点击**（保留守护运行）；用「停止」按钮静音会同时停止守护，普通报警门控关闭，无法观察“同一值重复报警”。这本身也是 `SW-STP-01` 与 `SW-REP-01` 的区分实验。
>
> 若空白区域点击不能静音（这本身就是 `SW-STP-02` 的失败证据），记录实际并改用「停止」按钮 + 重启守护作为**受限变体**观察 900 s 后的重复；受限变体改变了守护状态，不得当作原“连续守护”场景通过，也不能替代 `SW-STP-01` 的按钮停止判据。

### 5.5 停止与守护

| ID | 目标/来源 | 前置 | 刺激 | 判据 | 证据 |
|---|---|---|---|---|---|
| `SW-STP-01` | G8：点「停止」后铃声+震动停止（核心缺陷） | 同一相位内 App/传感器构建固定；`SW-REP-01`/`SW-ALM-01` 的普通报警正在响/震动；App 在首页 | 用 `uiautomator` 定位 `startGuardButton`（文字「停止」）并点击；记录点击时刻 | 点击后 **≤2 s**：`dumpsys vibrator` 无当前震动；`dumpsys audio` 中 App 活跃 player 变为 idle/releasing/消失（或操作员确认静音）；按钮文字变「启动」；`measure_state="starting"`；此后同一未变化普通值在 ≥900 s 且 ≥3 个读取周期内**不得**再次响铃/震动/通知。**A 预期复现**：按钮点击后仍在响/震动；空白区域点击可能停（`SW-STP-02`）。 | 点击前后 UI dump/截图、vibrator/audio 前后、logcat、后续窗口无新报警 |
| `SW-STP-02` | G8 回归：空白区域取消（现有行为） | 同上，换一次普通报警 | 在首页空白区域点击（非按钮/非卡片），记录时刻 | 与 `SW-STP-01` 相同的 ≤2 s 停止判据；不得因修复按钮路径而破坏该路径 | 同 `SW-STP-01` |
| `SW-STP-03` | 观察/决定：紧急报警中的停止 | 测试构建 `EMG` 报警正在进行 | 点击「停止」按钮；记录时刻 | **即时停止**：≤2 s 铃声/震动停止（“取消后停止”适用于紧急）。**随后**：因紧急代码在守护门控之前，下一次读取可能再次报警；记录是否再次报警、时刻。若用户要求“停止守护后连紧急也静默”，则作为范围决定单独处理（`SW-GRD-03`），不改写当前安全规则。 | 点击前后 UI、播放器/通知、后续读取日志 |

### 5.6 守护

| ID | 目标/来源 | 前置 | 刺激 | 判据 | 证据 |
|---|---|---|---|---|---|
| `SW-GRD-01` | G9：停止守护后不再普通报警 | 普通报警结束且已通过「停止」按钮停止守护（`SW-STP-01`） | 注入 `THR_NEW 36.2/55.0`（与上次 36.0 可区分）或其它普通超限值，观察 ≥2 个读取周期（约 6.5 min） | 点击后 ≤2 s 铃声/震动停止（同 `SW-STP-01`）；按钮「启动」、`measure_state="starting"`；新注入的普通超限读数不再产生铃声/震动/通知；App 仍可每 180 s 读 BLE、更新卡片/`alarm_events`（P-11/P-12，记录但不算失败）；**不得**出现新的 `alarm_channel` 通知 | 点击前后 UI/播放器、新 `TINJ`/App 日志、通知/播放器、`alarm_events` 说明 |
| `SW-GRD-02` | G10：重新启动守护后新事件仍报警 | `SW-GRD-01` 后守护停止；距上次普通报警副作用 ≥900 s（或在同一构建上确认节流已过期） | 点击「启动」；继续/注入 `THR_NEW 36.2/55.0`（或另一新值）；等待 1 个读取周期 | 按钮「停止」、`measure_state="stopping"`；`36.2>35` ⇒ 新的普通超限报警（铃声/震动/通知/`alarm_events`）；卡片/UI 更新；记录与上次报警的时间差。若修复仍保留 900 s 节流，必须等窗口后再判，不得把节流内不报警误判为失败。**若 `THR_NEW` 在守护关闭期间已被评估并导致修复后的 episode 去重不报警，记录为实现缺陷并交修。** | 同 `SW-ALM-01` + 上次报警时间 + 守护状态 |
| `SW-GRD-03` | G9 的安全例外/范围决定：停止守护 + 紧急 >65 | 守护停止；测试构建 `EMG` 或真实越限值 | 注入 66.0/55.0，等待 1 个读取周期 | **当前文档行为**：紧急报警仍震动/铃声/通知（用户说明书 §6.2、P-13）。**本轮处理**：作为独立观察项记录实际；若用户要求“停止守护后不再报警”按字面包含紧急，则该用例应改为“静默”，属产品安全规则变更，需当前用户明确决定后才可作为强制判据。未确认前，不用它阻塞 `SW-GRD-01` 的普通报警通过条件。 | 同 `SW-ALM-03` + 守护状态 + 用户决定记录 |

### 5.7 修复版完整复测、构建与回归

| ID | 目标 | 前置 | 方法 | 判据 |
|---|---|---|---|---|
| `SW-RET-01` | G11：修复版完整复测传感器→手机 | B 相位修复版 App；同一固件测试构建；全新安装/清数据；默认参数；网关/手机准备完成 | 在同一会话按顺序复跑：`SW-VER-01/02/03`→`SW-DSP-02`→`SW-NF-01`→`SW-ALM-01`→`SW-ALM-02`→`SW-ALM-03`→`SW-REP-01`→`SW-STP-01`→`SW-GRD-01`→`SW-GRD-02`；`SW-DSP-01` 若缓存有效则先做；`SW-REP-02/03`、`SW-STP-03`、`SW-GRD-03` 按范围执行并记录 | 每个强制子用例按自身判据通过；版本/构建身份会话开始与结束一致；App 无 `FATAL`/`ANR`；网关只转发，平台/云无判定；`SW-REP-01` 修复判据在该会话中至少复跑一次 |
| `SW-BLD-01` | 交付 App 构建身份与测试钩子排除 | 修复版已构建；若使用过 L2 测试钩子 | 记录 APK SHA-256/versionCode/Name/签名/源码提交；`install -r` 后核对设备 `base.apk`；检查正式候选无测试 action/组件/符号（若使用过 L2） | 设备字节与交付 APK 一致；正式候选不含测试钩子；无未授权导出组件；记录安装方式（保留数据/清数据） |
| `SW-REG-01` | 修复不破坏既有 UI/服务/通知 | 修复版安装后冷启动 | 首页/设定/我的/报告入口/传感器列表；守护启停；设置页默认值显示；通知渠道 | 页面可达、无 `FATAL`/`ANR`；首装默认值显示 `2/0.1/35/29/75/40/8/10/四开关=1`（以实际安装数据为准）；通知渠道存在；不把平台/云上传失败误判为报警失败 |
| `SW-REG-02` | 有界稳定性 | 守护/测试构建 `NORM` 或 `THR` | 观察 ≥6 个读取周期（约 18 min） | 无崩溃/ANR；App 每周期有读取终态；无未解释的重启/断连；失败率/长期稳定性只记录，不扩成通过结论。**非阻塞观察项** |
| `SW-REG-03` | 宿主机纯逻辑回归 | 修复版源码；测试环境可运行 Gradle | `cd dengbei_care && ./gradlew :app:testDebugUnitTest --offline`；重点 `AlarmEvaluatorTest`/`GatewayFrameCodecTest`/`GatewayReadGuardTest` 等 | 0 失败/0 错误；若新增测试侧用例，需与产品源码分离；**不能替代真机功能/链路** |

---

## 6. 退出准则（执行阶段）

### 6.1 强制通过条件

执行阶段在 **B 相位修复版 App + 同一固件测试构建**（或明确的 L0 真实缓存路径）上，以下用例全部有原始证据并逐项通过：

- 版本/准备：`SW-VER-01`、`SW-VER-02`、`SW-VER-03`；
- 显示/正常：`SW-DSP-02`、`SW-NF-01`（`SW-DSP-01` 若缓存有效则执行并记录）；
- 异常报警：`SW-ALM-01`、`SW-ALM-02`、`SW-ALM-03`；
- 反复报警：`SW-REP-01` 的**修复判据**；
- 停止/守护：`SW-STP-01`、`SW-GRD-01`、`SW-GRD-02`；
- 完整复测：`SW-RET-01`；
- 构建/回归：`SW-BLD-01`、`SW-REG-01`。

任一强制用例实际行为不符、仍适用的验证未完成或证据不足 ⇒ 执行阶段返回 `TEST_FAIL`，写明用例、期望/实际、原始证据缺口；交实现修复后在同一用例/同一构建身份上复测。**未执行始终不等于通过；阶段通过不等于完整产品通过。**

### 6.2 不计入强制通过、但必须记录的项

| 项 | 处理 |
|---|---|
| `SW-DSP-01` 真实缓存显示 | 缓存有效且有读数时执行；失败记录为真实 BLE→App 问题，交实现；若缓存无效/占位，记 `NOT EXECUTED`，不阻塞 `SW-DSP-02` |
| `SW-NF-02` 真实环境读数 | 记录真实 T/H 与判定；低于默认下界属于环境/默认参数不匹配，不直接判 App 误报 |
| `SW-REP-02` 紧急重复 / `SW-GRD-03` 紧急守护例外 | 现有产品安全规则；按当前文档记录实际。是否扩展“不再报警/不重复”到紧急，属产品/安全范围决定，需当前用户明确；未确认前不作为普通用例阻塞条件，也不写成通过 |
| `SW-REP-03` 进程重启后重复 | 非 Project Goal 逐字要求；记录实际，不追加为退出条件 |
| `SW-STP-02` 空白区域取消 | 回归项；若失败，记录为修复引入的回归，交实现 |
| `SW-STP-03` 紧急中停止 | 即时停止若执行则按判据记录；后续重复按范围决定 |
| `SW-REG-02` 长窗口 | 有界稳定性观测；失败率/长期稳定性不扩成结论 |
| 平台/云/数据库/iOS | 明确不在本轮范围；NOT EXECUTED，不作为通过或失败依据 |
| 光照/温度/功耗调节与测量 | 用户明确不做；NOT EXECUTED，诱因与缺口保留，不用模拟冒充 |

### 6.3 不得写成通过的情形

- 没有固件测试构建、只做了真实缓存显示/普通报警 ⇒ 不能写“传感器→手机完整链路通过”。
- 只看到 `s=1` 或只看到 App 有日志、没有匹配的 `T/H/uid4` ⇒ 不能证明同一事件。
- 只点了空白区域能停、没点「停止」按钮 ⇒ 不能证明“点停止关不掉”已修复。
- 只验证了同一值不重复，未验证新值仍报警 ⇒ 不能证明“重新启动守护后新事件仍报警”。
- 只验证了正常值不报警，未验证异常值能报警 ⇒ 不能证明报警链路。
- 只跑了宿主机单测/静态检查 ⇒ 不能证明 App 真机报警、停止、守护或链路。
- 把紧急报警的“每次提醒”安全例外与普通重复报警混为一谈，或未经用户决定擅自改产品规则。

---

## 7. 证据规范

### 7.1 每个用例必须保存

1. **版本**：仓库 HEAD、App APK/设备 `base.apk` SHA-256 与 versionCode/Name、源码提交、签名；传感器 axf/hex 摘要与下载日志、`BOOT uid`；网关 MAC/名称/`GwID`。
2. **刺激**：测试构建模式、注入值、`TINJ` 行、App 参数实际值、守护状态、点击/注入/读取时刻。
3. **原始观测**：App logcat 整窗；`dumpsys vibrator/audio/notification` 前后快照；UI dump/截图；本地库副本/查询结果；COM43 原始日志（嵌入式）。
4. **判据表**：逐项写“期望/实际/通过/失败/NOT EXECUTED”，不得只写总结。
5. **清理**：停止注入、停止守护/服务、恢复 App 参数、恢复发布固件（嵌入式）、关闭采集。

### 7.2 关联模板

```text
uid4=<BOOT uid 前 4B>
app_build=<versionCode/versionName + APK sha256 + 设备 base.apk sha256 + 源码提交>
sensor_build=<axf/hex sha256 + Verify OK + 测试构建模式>
gateway=<BLE MAC/name + GwID + devCount + valueSize + mtu>
window=<start/end + timezone>
stimulus=<mode, inj_t, inj_h, force, repeat, 注入/读取/点击时刻>
sensor_evidence=<TINJ/P 行原文 + k>
app_evidence=<MqttService id/T/H 行 + 读取时刻 + link_state>
db_evidence=<temperature 行 / alarm_events 行>
ui_evidence=<截图/UI dump 路径>
alarm_evidence=<vibrator/audio/notification 时间戳/原文>
verdict=<pass/fail/not_executed + 逐项依据>
```

### 7.3 证据边界

- App 日志/本地库/UI 证明 App 消费与用户可见结果；不证明平台/云端。
- `dumpsys vibrator/audio` 证明系统层面当前震动/播放状态；不证明用户实际听到/感到。
- `link_state=DATA_OK` 证明一次读取终态；不证明传感器在线或物理采样时刻。
- 缓存值不是新物理采样；注入值不是物理量。
- 网关无版本回读；不得推断网关固件版本。

---

## 8. 限制、未决与交接

### 8.1 关键限制

1. **固件测试构建依赖**：受控正常/下降/紧急/新事件与“传感器→手机同一事件”依赖 TD-EMB-ALM-001 §4 的测试构建；若执行阶段未取得，按 §3.3 记 `NOT EXECUTED`，不得写成通过。
2. **App 安装态未在本能力核验**：执行阶段先 `SW-VER-01`；若未安装，安装当前候选；若安装字节不同，记录并决定是否 `-r` 安装所测候选（同签名）。
3. **网关无版本回读**：只记录 BLE MAC/名称/`GwID`；不烧录/不修改。
4. **手机通知/铃声/震动环境**：MIUI 可能限制通知、后台、精确闹钟或默认铃声；执行前逐项确认并记录；环境不满足时不得用“没响”直接判代码失败，应记录为环境/前置缺口并区分。
5. **真实室温低于默认下界**：`SW-NF-02` 记录；不得把真实室温超限直接判为“误报”，也不得用它替代 `SW-NF-01` 的代表性正常值。
6. **紧急 >65 的守护/重复例外**：当前产品文档明确紧急不受守护停止影响、每次上报提醒；本轮按安全例外单列并记录，不擅自改写；若用户要求扩展，需明确范围决定后更新本设计与产品规则。
7. **L2 App 测试钩子**：若使用，必须只在测试构建启用并在正式候选排除；其证据只证明 App 业务路径，不证明 BLE/433/网关/传感器。

### 8.2 交接

| 事项 | 责任 | 完成条件 |
|---|---|---|
| 传感器测试构建/注入/`TINJ`/正式版排除 | 固件实现 | 按 TD-EMB-ALM-001 §4 提供默认关闭、发布排除、`TINJ` 可区分、宿主双构建/字符串检查、`Verify OK`、构建摘要 |
| App 停止/重复报警/守护修复 | Android 实现 | 基于 A 相位证据定位并修复；重建测试候选；记录新 APK SHA-256/versionCode/源码提交；不改变未授权范围 |
| App 安装、版本/UI/logcat/dumpsys/DB 观测 | 软件测试执行能力 | 按 §2.3/§4/§7 保存原始证据；与嵌入式窗口对齐 |
| 传感器串口/烧录/复位/测试构建复测 | 嵌入式测试执行能力 | 按 TD-EMB-ALM-001 §5 执行；测试后恢复发布镜像 |
| 紧急 >65 守护/重复是否纳入“不再报警/不重复” | 当前用户/验收范围所有者 | 明确决定后更新 `SW-REP-02`/`SW-GRD-03` 与退出准则；未决定前不阻塞普通用例 |
| 平台/云/DB/iOS | 不在本轮 | 不创建下游节点、不修改 |

### 8.3 现实条件与未执行边界

- 当前不能调节温度、不能改变光强、不能测功耗：本设计不安排这些实验；真实读数只作观测，不能据此宣称光照/功耗/现场通过。
- 平台/云上传/数据库/iOS：NOT EXECUTED；不写通过，不作为本阶段失败依据。
- 真实被窝内正常值：可选增强；不因未做而阻塞 `SW-NF-01` 的注入代表性正常值，但必须披露物理响应未证明。
- 逻辑分析仪/I²C 波形：不是任何必需用例的条件；若使用另存原始 `.sr` 再离线解码，保持接线/电平核实。

---

## 9. 设计自检（提交前逐项对照）

- [x] 每个强制用例都有目标来源、被测版本、前置、刺激、观察点、判据、证据与清理。
- [x] 判据使用实际源码行为（严格阈值、5/10/15 min 下降窗、900 s 普通节流、紧急不节流、守护门控、`measure_state` 反向命名、卡片格式）。
- [x] 正常/异常/重复/停止/守护/重启/完整复测/回归均有独立用例；没有把“未执行”写成“通过”。
- [x] 退出准则只汇总 §5 中当前有效范围内的强制用例；`SW-NF-02`、`SW-REP-02/03`、`SW-STP-02/03`、`SW-GRD-03`、`SW-REG-02`、平台/云/功耗等单列为非强制或 NOT EXECUTED，未反向阻止本阶段退出。
- [x] 未安排调温、调光、功耗测量；未修改产品源码/网关/平台；未创建下游流程节点或任务清单。
- [x] 真实环境优先；受控注入位于输入边界并明确证明边界；App 测试钩子仅条件性回退且要求正式候选排除。
- [x] 新增文件按 `file_manifest.txt` 路径/状态/用途格式登记；提交前回读实际文件。
