# 嵌入式测试设计（TD-EMB-ALM-001 rev 1.0）

状态：嵌入式测试设计产物（`embedded_tester.embedded_test_design`，输出端口 `test_design`）。**只做设计，不执行**编译、烧录、复位、串口采集、手机操作或平台访问；§1.3 的只读枚举除外。
对象：`dengbei14` 当前成果上的 **传感器（CW32L010 + GXHT40 + 光照分压 + UM2005C 433 + UART1）→ 既有 CH592 网关（仅转发）→ 安卓 App（Redmi 8A，`com.jinyuni.dengbei_care`）** 真机链路。
本轮目标来源：当前 Project Goal（核对实际固件/App 版本；覆盖温湿度采集、网关转发、手机显示、正常读数不误报、异常报警；优先复现“反复报警”“点停止关不掉”；修复后重建并用真机独立复测；必须验证取消后铃声/震动停止、停止守护后不再报警、重新启动守护后新事件仍报警；最后用修复版完整复测传感器到手机链路；网关只转发，平台/云上传/数据库/iOS 不纳入）。
关联资料：`artifacts/test_design.md`（TD-002 rev 10.0，固件/注入机制与 UART 观测）、`artifacts/interface_contract.md`（IC-002）、`artifacts/firmware_design.md`（FD-002）、`artifacts/firmware_release.md`（FW-REL-001）、`artifacts/android_build.md`（AB-003）、`artifacts/codegraph_alarm_chain_index.md`、`docs/03_传感器调试串口手册.md`、`docs/04_安卓App安装与使用.md`、`dengbei_care/用户使用说明书.md`。

**结论先行**：本方案在当前条件下可设计；在下列前置满足时可执行，但有两个必须写明的可执行性事实：

1. **当前量产固件在“现场有光、室温、`LIGHT_DARK_THRESHOLD_CODE=0`”下不会自然产生任何 433 上报**；`sensor_decide_report` 只保留“严格 >35.0 ℃”分支，而用户明确不能调温、不能改光强。因此“传感器→网关→手机”的正向链路、手机显示与报警用例**必须**使用由固件实现提供的**测试专用构建**（受控注入/强制上报）。本设计给出最小接口、可区分观测行、正式版排除与复测判据（§4）。测试员不改产品源码；若执行阶段拿不到该测试构建，正向用例记 `NOT EXECUTED`，不得写成通过，交已审批的失败分析→固件实现闭环。
2. **当前 App 的停止路径存在可复现风险**：`HomeFragment.toggleButton()` 的“停止”分支只发 `START_STOP_GUARD`，没有调用 `RingtonePlayer.stopAlarm()` / `VibrationPlayer.stopVibration()`；两者只由 `BaseActivity.onTouchEvent()` 调用，而按钮等子控件会消费触摸事件，Activity 的 `onTouchEvent` 不保证收到。另有网关缓存复读 + 每设备 900 s 节流（紧急 >65 ℃ 不节流）可导致“同一未变化读数反复报警”。这些是本轮要复现的缺陷假设，不是本轮设计失败；用例给出区分实验（§3.7），修复后按同一用例复测。

**`DESIGN_READY` 只表示本方案在当前条件下一致、可执行，不表示任何固件/App 功能已执行或通过。**

---

## 0. 设计范围与非目标

### 0.1 本轮必须覆盖

| 编号 | 目标 | 目标来源 |
|---|---|---|
| G1 | 核对实际运行的传感器固件与 App 版本/身份 | Project Goal 第 1 句 |
| G2 | 真实温湿度采集（每 3 min 一条 `S`） | Project Goal；readme 修改点 3/8 |
| G3 | 经既有网关 433→BLE 转发到手机 | Project Goal；IC-002 |
| G4 | 手机显示读数（卡片/详情/本地库） | Project Goal |
| G5 | 正常读数不误报（在配置阈值内不产生报警副作用） | Project Goal |
| G6 | 异常报警（超限/下降/紧急） | Project Goal |
| G7 | 复现“反复报警”并验证修复后同一未变化事件不重复报警 | Project Goal |
| G8 | 复现“点停止关不掉”并验证停止后铃声+震动停止 | Project Goal |
| G9 | 停止守护后不再产生普通报警副作用 | Project Goal |
| G10 | 重新启动守护后新事件仍能报警 | Project Goal |
| G11 | 修复版完整复测传感器→手机链路 | Project Goal 最后一句 |

### 0.2 明确不纳入

- 平台/云上传/平台数据库/iOS：不做 MQTT 平台一致性、`/upload_data`、MySQL 回读、iOS 的任何判定；测试期间建议手机保持离线（§3.5），平台侧证据不进入本轮通过条件。
- 网关固件：不烧录、不修改、不读取版本；网关只作为 433 接收与 BLE FFE2 缓存的转发者。
- 温度、光强、功耗的主动调节/测量：不安排调温、遮光/照射、电流/功耗测量；真实板件在现有环境下的 `S/V/D/VDD` 只作观测，不作为光照/功耗/现场结论。
- 传感器产品源码与 App 产品源码：本能力不修改；测试专用固件与 App 修复分别由对应实现能力提供（§4、§8）。

---

## 1. 当前基线（设计时只读核对）

### 1.1 仓库与源码身份

| 项 | 值 | 来源/说明 |
|---|---|---|
| 仓库/分支 | `D:\mypro\beiwo2\AIP`，`main`，HEAD `b4b8c3c91ca177aac480f7f4e5b697fcc18dfcd3`，接手 `git status` 干净 | `git status --porcelain`、`git rev-parse HEAD` |
| 传感器产品源 | `USER` 最近提交 `82a7e74e5d6bae855a573e1f23f83936ba8db049`（2026-11-10） | `git log -1 -- <sensor>/USER` |
| 传感器发布包 | `fw_release/fw_sensor_FD-002r8.zip`，134,791 B，MD5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`，SHA-256 `16ad3f69a9c83fc4984cf6efa2d9c0f43b5c5498716b5f253df090ef955bf4d0` | `md5sum`/`sha256sum`；FW-REL-001 |
| 传感器交付包成员 | axf `b6a15fed…`、hex `4d728dec…`、`PACKAGE.txt` `a58f67fc…`（FW-REL-001 rev 3） | 执行时必须重新复算 |
| Android 产品源 | `dengbei_care/app/src/main` 最近实现提交 `a12d12b570d541a9d348a1b6d6846275d871c7d4`（2026-11-13）；当前 APK 与该源一致 | `git log -1`；AB-003 |
| Android 候选 | `app-debug.apk` 10,370,964 B，SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`，`versionCode 10` / `versionName 1.9`，debug | `sha256sum`；AB-003 |
| 网关源 | `CH592EVT/EVT/EXAM/BLE/beiwov2/APP` 最近提交 `c3baaaa1f113c2d73986df18a922dde5bdd144f9`（2026-09-15）；本轮不修改 | `git log -1` |
| 代码图 | 传感器/网关/Android 三个本地根已索引，状态 `complete`、`reindexRecommended=false`；索引源 HEAD `43fe3b3`，其后仅新增索引/证据 | `artifacts/codegraph_alarm_chain_index.md` |
| 当前版本声明 | `truth/current_versions.yaml` 为空 `{}`；`truth/engineering_baseline.yaml` 为 `null` | 项目真值目录；不能依赖其推断版本 |

### 1.2 关键行为事实（当前源码）

| 项 | 事实 | 位置 |
|---|---|---|
| 采样节拍 | RTC 每 1 分钟一拍，累计 3 拍（180 s）置一次采样 | `USER/src/main.c`、`sensor_config.h` `SENSOR_SAMPLE_TICKS=3` |
| 上报判据 | `cur > 350`（严格 >35.0 ℃）或（有前值且 `cur-prev > 9` 且 `dark`）；`dark` 仅在 `LIGHT_DARK_THRESHOLD_CODE != 0` 且有效时可能为真 | `USER/src/fw_core.c` `sensor_decide_report`；`sensor_config.h` |
| 当前光照判据 | `LIGHT_DARK_THRESHOLD_CODE=0`（唯一禁用值）⇒ `light_valid=0`、`dark=0`；现有环境下升温+无光分支不触发 | `sensor_config.h`、`light.c` |
| UART1 | PA06=TXD/PA05=RXD，9600 8N1；上电 `BOOT/VDD/IOTEST/IOSIG/BUS`，每采样一条 `S`，失败追加 `G`，有上报判定/发送尝试追加 `P`；行 ≤96 B；采样间期无输出 | `USER/src/debug_trace.c`、`docs/03` |
| `S` 行 | `S k= p= H= V= o= n= x= a= D= t= h= q= r= s= y=`；`t/p` 单位 0.1 ℃，`h` 单位 0.1 %RH；`q` 采样有效，`r` 判定，`s` 发送结果（0/1/2） | `debug_trace.c` |
| `P` 行 | 冻结快照（组帧实际使用值）`t/h`、`rq`、`s`、`y`；仅在本周期有上报判定或发送尝试时出现 | `debug_trace.c`、`measure.c` |
| 网关缓存 | 收到有效 433 帧后更新对应绑定设备的 8 B 记录并保留；无新帧时 `timeoutcnt` 递减（约 10 s/次、上限 100 次≈1000 s）后才清零为占位；期间 BLE FFE2 反复返回**同一缓存值** | `APP/app_um2006A.c` |
| BLE 帧 | 头 16 B（`gateway_id6|devCount@6|pad9`）+ 每设备 16 B（前 8 B：`sensor_id4|humidity_u16be@4|temperature_s16be@6`）；AES-ECB/NoPadding；FFE2；MTU 需≥`16*(1+N)+3` | `protocol/GatewayFrameCodec.kt`、`GatewayReadGuard.kt`、`IC-002` |
| App 读取 | 网关地址存在时每 `SensorCadence.SAMPLE_PERIOD_MS=180000 ms` 读取一次；关闭守护**不停止**读取 | `MqtttService.startBluetoothTask` |
| App 报警判定 | `AlarmEvaluator`：`temp>tempMax || temp<tempMin || hum>humiMax || hum<humiMin`（严格）；下降=基准温度差>tempDrop **且** 基准湿度差>humiDrop（5/10/15 min 时间窗，基准来自本地库）；`temp>65.0` 紧急，优先于一切、与开关无关 | `telemetry/AlarmEvaluator.kt` |
| App 节流 | 普通（下降/超限）按设备内存 `lastAlarmTime` 做 900 s 节流；紧急不节流、每次上报都通知 | `MqtttService.processTemperatureHumidityData` |
| App 守护门控 | `homeViewModel.startData.value==1` 才触发普通报警副作用；`storeTemperatureReading` 仅 `storeflag==1` 入库；`alarm_events` 不受守护门控（仍记录） | `MqtttService`、`TemperatureDatabaseHelper` |
| App 播放器 | `RingtonePlayer` 单例循环播放；`VibrationPlayer` API26+ 使用 `createWaveform(..., repeat=0)` 无限循环；`stopAlarm`/`stopVibration` 只在 `BaseActivity.onTouchEvent` 调用 | `RingtonePlayer.kt`、`VibrationPlayer.kt`、`BaseActivity.kt` |
| App 停止按钮 | `HomeFragment.toggleButton()` 的停止分支只 `startService(START_STOP_GUARD, guardaction="0.0")`，并写 `MyPrefs.measure_state="starting"`；不调用播放器停止 | `ui/home/HomeFragment.kt` |
| App 首装默认 | `tempdrop=2`、`humdrop=0.1`、`tempmax=35`、`tempmin=29`、`hummax=75`、`hummin=40`、`guardtime=8`、`delayalarm=10`、四个报警开关=1 | `MainActivity.setupDefaultPreferences` |
| App 本地库 | `bwtemperature_database1`：`temperature(time,device_id,temperature,humidity)` 主键 `(time,device_id)`；`alarm_events(time,device_id,type,message)`；类型 `drop|threshold|emergency` | `TemperatureDatabaseHelper.kt` |
| App 通知 | 渠道 `alarm_channel`，标题 `[设备名] 报警`，内容为报警文案；`areNotificationsEnabled()` 为假时静默不发 | `MqtttService.sendAlarmNotification` |

### 1.3 设计时只读枚举（原始返回已核对，2026-11-26T14:24Z）

| 工具/入口 | 结果要点 | 能说明 / 不能说明 |
|---|---|---|
| `serial_list_ports` | `COM43` = Prolific `VID:PID=067B:2303`（`LOCATION=1-9.4.2`）；`COM26` = `C251:F001`，SN `87094109484987710672FF50` | COM43 登记入口在位；**不证明**传感器板上电、UART1 输出、波特率/接线正确。COM26 是调试器 CDC，不是传感器调试口 |
| `usb_list_devices` | CW-DAPLink `C251:F001`（SN `87094109484987710672FF50`）；目标手机 `18D1:4EE7` 实例含 `12F046690506`；Cypress FX2 `04B4:8613`（Unknown Device #1）；未枚举到示波器/电流表/SMU | 调试器、手机、FX2 的 USB 身份在位；**不证明**协议功能、目标在线或采集可用 |
| `sigrok_info` | `enabled=true`；`driver=fx2lafw`；`connection=2.32`；`channels=[D0,D1]`；`signal_labels={D0:scl,D1:sda}`；`triggers={D1:f}`；`samplerate_hz=4000000`；`max_samples=20000000`；`online=not_checked` | 配置与历史接线一致；**未探测、未采集**。本轮不把逻辑分析仪作为任何必需用例的条件；只在需要时补充 I²C 被动证据 |
| ADB（本机只读） | `adb devices -l`：`12f046690506 device product:olivelite model:Redmi_8A`；Android `10`/API `29`；蓝牙 `enabled:true/state:ON`；飞行模式 `0`；`pm list packages` 无 `com.jinyuni.dengbei_care`，`pm path` exit 1 | 手机在线并已授权 ADB；**App 当前未安装**。安装、版本核对与业务验证属执行阶段 |
| 传感器板 | 未打开串口、未复位、未烧录 | 板上镜像与在线状态**未核验**；不得用历史 `COM42`/旧镜像/旧 uid 代替本轮证据 |

> 执行阶段必须先重新枚举端口/USB；设备重插后 USB 地址可能变化，不从旧日志猜地址。

---

## 2. 链路、接口与关联键

### 2.1 链路

```text
GXHT40 温湿度 + 光照分压（PB05 供电/PB04 ADC）
  → CW32L010 采样（每 180 s）→ sensor_decide_report（>35.0 ℃ 或 升温>0.9 ℃ 且无光）
  → UM2005C 433 10 B 加密帧（uid4|temp_le2|hum_le2|crc16_le2）
  → 既有 CH592 网关：433 接收 → 按绑定 ID 组 16 B 块 → AES-ECB → FFE2 160 B 缓存
  → 安卓 App：每 180 s BLE 读取 → 解密/长度守卫/逐 ID 消费
  → AlarmEvaluator（超限/下降/紧急）→ 本地库 + 卡片 + 铃声/震动/通知
```

### 2.2 关联键与可区分载荷

- 传感器 `BOOT uid=<8 位 hex>` 的前 4 字节（下文 `uid4`）是板件与 App 绑定 ID 的关联键；同一板件稳定，复位后不变。
- 传感器不发送 RTC 时间；`S/P` 的 `k` 是 RTC 累计分钟节拍（正常步长 3），只作窗口辅助，不作为绝对时间。
- App 的 BLE 接收时间是**手机读取时刻**，不是传感器采样时刻；网关缓存复读会产生“同值不同秒”的 App 时间戳。
- 可区分载荷：测试构建注入的 `(temp_x10, hum_x10)` 取固定、易识别的组合（§4.2），并在 `TINJ` 行同时打印真实采样值与注入值；App 日志/本地库/UI 必须与注入值逐项一致。真实量产样本不出现这些固定组合（尤其 `660`、`670`）。
- 同一事件的关联顺序：`TINJ/P` 冻结值 → 网关缓存 → App `id=uid4, T/H` 日志 → 本地库 `temperature` 行 → 卡片 UI → 报警副作用/`alarm_events`。不以“时间接近”猜事件；用 `uid4 + 注入值 + 采集窗口` 关联。

### 2.3 观测方法（执行阶段）

| 端 | 观测 | 具体方法 | 能证明 / 不能证明 |
|---|---|---|---|
| 传感器 | UART1 原始行 | COM43 9600 8N1，先开端口再复位/下载复位；保存原始字节+时间戳；解析 `BOOT/VDD/IOTEST/IOSIG/BUS/S/G/P/TINJ` | 采样/判定/帧值/发送结果；**不证明** 433 电气、网关内部、App 消费 |
| 网关 | 无直接调试口 | 不做网关串口/固件操作；以 App BLE 读取结果作为转发观测 | App 收到即证明 433→网关→BLE 的实际转发；不证明网关固件版本 |
| App | logcat | `adb logcat -v threadtime`，关注 tag `MqttService`/`DataProcessor`/`AlarmNotify`/`RingtonePlayer`/`VibrationPlayer`/`TempDb`/`AndroidRuntime`/`ActivityManager`；保存整窗原始日志 | 读取/解码/判定/入库路径；不证明物理量 |
| App | 铃声/震动 | `adb shell dumpsys vibrator`（`mCurrentVibration`、历史启动记录）；`adb shell dumpsys audio`（活跃 player 的 uid/pid/state 与 new/releasing 历史）；必要时操作员听感/录像 | 当前是否在震动/播放；不能量化音量/幅度 |
| App | 通知 | `adb shell dumpsys notification`（标题/内容/渠道）；`dumpsys notification --noredact`（若设备允许） | 报警通知是否存在；不证明用户已看到 |
| App | 本地库 | debug 包 `adb exec-out run-as com.jinyuni.dengbei_care cat databases/bwtemperature_database1`（连同 `-wal/-shm`）到主机副本；只读查询 `temperature`/`alarm_events` | 入库与报警事件计数；**不证明**平台/云端 |
| App | UI | `uiautomator dump` + 截图；卡片温度/湿度/状态 chip/报警文案；首页按钮文字；设定页数值 | 显示与守护状态；不证明内部状态 |
| 版本 | App | `dumpsys package com.jinyuni.dengbei_care`（versionCode/versionName/firstInstallTime/lastUpdateTime）；`pm path` + 拉取 `base.apk` 在主机复算 SHA-256 | 实际安装字节；不证明业务功能 |
| 版本 | 传感器 | `mdk_flash` 下载测试/发布镜像的 `Verify OK` + axf/hex 摘要 + `BOOT` 行 | 实际下载镜像；`BOOT fw=FD-002r4` 是冻结横幅，**不是版本号** |
| 版本 | 网关 | App 扫描得到的 BLE MAC/名称；App 日志 `GwID=<6 B>` | 网关身份；**无版本回读**，固件版本未知 |

---

## 3. 测试策略与刺激

### 3.1 分层策略

| 层 | 使用条件 | 目的 | 证据边界 |
|---|---|---|---|
| L0 真实采集（发布构建） | 现有板件、现有环境、不调温/不调光 | 证明 `S` 行每 180 s、`q=1`、真实 T/H、`r=0/s=0`、`V=0/D=0`（判据未启用） | 采集与判定观测；不证明上报/手机 |
| L1 真实链路+受控注入（测试构建） | 测试构建已由实现提供 | 用注入值走真实判定/冻结/组帧/CRC/Feistel/433/网关/BLE/App，证明显示、报警、停止、守护 | 注入值不是物理量；不证明传感器对注入值的物理响应 |
| L2 App 真实操作 | 同一真机/同一构建 | 首页启停、设定值、通知、铃声/震动、本地库 | 与 L1 同一窗口 |
| L3 修复版完整复测 | 修复后的 App 构建 + 同一测试构建 | 复跑全部强制用例 | 只对实际构建字节与窗口有效 |

### 3.2 为什么必须用测试构建

当前发布固件 `LIGHT_DARK_THRESHOLD_CODE=0`，用户现场有光且不能改光强；`sensor_decide_report` 只剩 `cur > 350` 分支。现有室温不开温，传感器**不会**自然发送 433 帧；网关缓存为空/占位，App 收不到任何有效读数。因此：

- 真实采集（L0）可做；
- “显示/正常不误报/异常报警/反复报警/停止/守护/修复版复测”（L1–L3）**必须**有受控注入/强制上报的测试构建；
- 这不是把功能失败改成前置缺失，而是当前固件判据与用户环境共同决定的**真实可执行性缺口**；测试构建由实现提供，测试员不修改产品源码。

### 3.3 App 参数与刺激选择

**不修改 App 的报警参数**，使用首装默认值即可覆盖本轮三类报警：

| 参数 | 首装默认 | 用途 |
|---|---|---|
| `tempdrop` / `humdrop` | 2 ℃ / 0.1 %RH | 下降报警：注入 34.9→32.0（温差 2.9>2）、湿度 60.0→55.0（差 5.0>0.1） |
| `tempmax` / `tempmin` | 35 / 29 ℃ | 超限：注入 36.0>35；正常：注入 30.0 在 [29,35] 内 |
| `hummax` / `hummin` | 75 / 40 %RH | 注入 55.0/60.0 均在范围内，避免湿度超限干扰 |
| 下降/超限开关 | 1 / 1 | 两类判定都启用；超限优先于下降（`AlarmEvaluator`） |
| 震动/响铃开关 | 1 / 1 | 观察铃声/震动；紧急 >65 不受开关影响 |
| `guardtime` | 8 h | 测试期间不会自动到时停止 |
| `delayalarm` | 10 s | 当前不参与本地节流（固定 900 s），不在本案判据中使用 |

> 执行前从「设定」页读取并记录实际值；若存在某设备的独立参数覆盖，先清除或改用全局值，保证四台设备/一个传感器使用上表默认值。执行后恢复原值；不得把改参数当作通过手段。

### 3.4 绑定与网关前置

1. 传感器 `BOOT uid4` 必须以 8 位大写 HEX 加入 App 设备列表（「注册传感器」/传感器列表），名称任意。
2. App 必须在「网关与上传设置」中通过正常扫描发现并保存实际网关（默认迁移值 `84:C2:E4:03:02:02` 仅供参考，**不得**当作已确认）；记录 MAC/名称/RSSI。
3. 网关的绑定列表应包含该 `uid4`；若网关已绑定其它设备，记录 `devCount` 与各设备 ID，其他设备的有效块/报警不得混入本案判据；若出现非目标设备报警，单独记录并排除，不修改网关固件。
4. 若 App 的 A1 写入路径可用且确有必要，按 App 正常绑定流程写入；执行前后记录 App `MacIdBook` 与网关 `devCount`/`GwID`，不烧录网关。

### 3.5 手机环境

- 使用 Redmi 8A / `12f046690506` / Android 10（API 29）；重新确认 ADB 授权。
- 蓝牙开启；Android 10 扫描需定位总开关+`ACCESS_FINE_LOCATION`；通知权限/渠道开启；系统默认闹钟铃声存在、媒体/闹钟音量>0；关闭勿扰或确认不拦截报警。
- **为排除平台/云上传**，建议手机进入飞行模式后单独开启蓝牙；不登录/不注册平台身份（`registerphone` 为空），不配置平台 `mac_addr`。若现场不能离线，记录实际网络状态，并把任何 MQTT/HTTP/平台观测排除在本轮判据之外；平台/云上传为 NOT EXECUTED。
- 记录 `MyPrefs` 关键键（`measure_state`、`guardtime`、四个报警开关、`registerphone`、`mac_addr`、`gateway_ble_addr`）的值，但证据中不出现任何凭据。

### 3.6 测试相位与版本绑定

| 相位 | App 构建 | 传感器构建 | 目的 |
|---|---|---|---|
| A 缺陷复现 | 当前候选（AB-003 或实际安装的当前字节） | 发布固件 + 测试构建 | 复现“反复报警/点停止关不掉”，记录当前行为 |
| B 修复验证 | 实现修复后重建的 App 测试候选（记录新 SHA-256/versionCode） | 同一测试构建 | 全部强制用例逐项复测 |
| C 完整回归 | B 的修复版 App | 测试构建 + 发布固件 | 最后完整复测传感器→手机；随后恢复发布固件 |

每个相位开始时记录仓库 HEAD、App APK SHA-256/设备 `base.apk` 摘要、传感器 axf/hex 摘要与 `Verify OK`、`BOOT uid`；相位之间不得混用旧结论。

### 3.7 诊断假设与区分实验

| 假设 | 机制（当前源码事实） | 区分实验 | 失败归属 |
|---|---|---|---|
| H1 同一网关缓存读数据被 App 反复当作新读数 | 网关保留最后有效块约 ≤1000 s；App 每 180 s 读取并以当前时刻入库；普通报警 900 s 节流、紧急不节流 | `EMB-REP-01/02`：同一注入值重复上报；传感器 `r/s` 与 App 每 180 s 读取次数、`dumpsys vibrator` 时间戳对照 | 若同一未变化值在 900 s 后再报警/紧急每读都报警，属 App 报警状态/去重缺陷 |
| H2 停止按钮不停止已播放报警 | `HomeFragment.toggleButton` 停止分支无 `stopAlarm/stopVibration`；`BaseActivity.onTouchEvent` 被子控件消费 | `EMB-STP-01`：报警中点击「停止」按钮；`EMB-STP-02`：点击空白区域对照；观察 `dumpsys vibrator/audio` | 按钮路径不停/空白路径停 ⇒ App 停止路径缺陷 |
| H3 正常读数被默认下界误报 | 首装 `tempmin=29`；室温若 <29 会触发超限，但现场固件不会自然上报 | `EMB-ACQ-01` 记录真实 `S`；`EMB-NF-01` 注入 30.0（在默认范围内）；不把真实室温超限直接判为“误报” | 若 30.0 仍报警 ⇒ App 判定缺陷；若真实室温 <29，属使用环境与默认参数不匹配，需产品决策 |
| H4 紧急 >65 绕过守护与节流 | `processTemperatureHumidityData` 紧急块在 `startData` 门控之前且不更新 `lastAlarmTime` | `EMB-ALM-03`、`EMB-REP-02`、`EMB-GRD-03`：守护开/关、同一 66.0 重复 | 重复紧急报警属“反复报警”缺陷；守护关后是否仍报警是产品安全例外/范围决策（§7.2） |
| H5 App 进程重启丢失报警状态 | `deviceHistory`、`lastAlarmTime` 在内存；重启后清零 | `EMB-REP-03`：同一缓存值下 force-stop 后重启 | 重启后同一未变化值再报警 ⇒ App 状态持久化缺陷 |

---

## 4. 测试构建的最小接口要求（交固件实现）

> 本能力不修改产品源码；以下是实现者需提供的最小测试专用能力。可选用**编译期宏**或**受控 UART1 RX 命令**实现；无论哪种，必须满足本节的观测、默认关闭、正式版排除与复测要求。

### 4.1 注入点

`USER/src/measure.c` 的 `temperature_process()` 中，`measure_sample()` 成功之后、`sensor_decide_report()` 之前：

- 用注入的 `tempvalue`/`huminityvalue` 替换**判定输入**；
- 保留真实采样、真实 `s_light`、真实 CRC/换算/I²C、真实 `encode_frame10`、真实 UM2005C 发送、真实 `S/G/P` 格式；
- `FORCE_REPORT` 模式仅在 `sensor_decide_report()` 返回假时置 `report_req=1` 并冻结注入值；不得覆盖尚未发送成功的旧冻结快照；
- 不修改 `sensor_decide_report` 的阈值/方向语义，不修改帧布局/字节序/Feistel/CRC；
- 下一次复位/上电可重复；测试构建不得留在交付板件上。

### 4.2 模式与固定值

| 模式 | 注入 `temp_x10` | 注入 `hum_x10` | 强制上报 | 用途 |
|---|---|---|---|---|
| `OFF` | — | — | 否 | 真实采集基线（= 发布行为；无 `TINJ`） |
| `NORM` | 300（30.0 ℃） | 550（55.0 %RH） | 是 | 正常显示、正常不误报 |
| `THR` | 360（36.0 ℃） | 550 | 是 | 超限报警、反复报警 |
| `THR_NEW` | 362（36.2 ℃） | 550 | 是 | 重启守护后的“新事件”超限对照 |
| `DROP_BASE` | 349（34.9 ℃） | 600（60.0 %RH） | 是 | 下降报警的基准样本 |
| `DROP_CUR` | 320（32.0 ℃） | 550 | 是 | 下降报警的当前样本（与基准约隔 6 min / 2 个采样周期） |
| `EMG` | 660（66.0 ℃） | 550 | 是 | 紧急报警、反复紧急 |
| `EMG_NEW` | 670（67.0 ℃） | 550 | 是 | 紧急“新事件”对照 |

- `THR/EMG` 在重复用例中需每采样周期持续生效（或提供有界重复次数≥10）；`THR_NEW`/`EMG_NEW` 为单次新事件对照；`DROP_BASE→DROP_CUR` 按成功采样序号触发（基准后第 2 个成功采样为当前值）。
- 所有值均为 0.1 单位；App 端期望 30.0/55.0、36.0/55.0、36.2/55.0、34.9/60.0、32.0/55.0、66.0/55.0、67.0/55.0。

### 4.3 可区分观测行

测试构建在每个注入周期的 `S/G` 之后、`P` 之前或之后追加一行稳定格式（≤96 B、CRLF），例如：

```text
TINJ k=<tick> mode=<NORM|THR|THR_NEW|DROP_BASE|DROP_CUR|EMG|EMG_NEW> rt=<real_t> rh=<real_h> it=<inj_t> ih=<inj_h> f=<0|1> r=<decision> s=<send>
```

- `rt/rh` = 当次真实采样值（0.1 单位）；`it/ih` = 注入值；`f` = 是否强制上报；`r` = 真实判定结果；`s` = 发送结果。
- 正式/发布构建不得含 `TINJ` 字符串、`SENSOR_TEST_INJECT*` 符号或注入函数；默认宏 0 时行为与发布基线一致。

### 4.4 构建与排除验证（交实现，测试侧独立复核）

1. 默认/发布构建 `SENSOR_TEST_INJECT_ENABLE=0`，0 Error；记录编译器、Code/RO/RW/ZI、axf/hex MD5/SHA-256。
2. 发布 axf/hex 做字符串/符号检查：无 `TINJ`、无测试宏/函数；map/`nm` 无注入符号；发布包成员仅标准构建。
3. 源码差异审计：只允许 `measure.c` 及必要的 `sensor_config.h`/测试构建配置出现默认关闭的注入代码；不得改 `sensor_decide_report`、`encode_frame10`、发送/CRC/Feistel、`S/G/P` 格式或光判据。
4. 宿主双构建 harness：宏 0 行为与基线一致；宏 1 只注入判定输入、强制上报、`TINJ` 正确；释放后无测试符号。
5. 测试结束恢复发布镜像 `fw_release/fw_sensor_FD-002r8.zip`，`mdk_flash` 下载并 `Verify OK`，重新取得 `BOOT uid`，确认无 `TINJ`。**测试构建不得留在交付板件上。**

> 若执行阶段拿不到该测试构建：`EMB-CHN-01`、`EMB-NF-01`、`EMB-ALM-01/02/03`、`EMB-REP-01/02/03`、`EMB-STP-01`、`EMB-GRD-01/02`、`EMB-RET-01` 记 `NOT EXECUTED`，原因写“缺测试专用注入/强制上报构建”；不得写成通过，交已审批的失败分析→固件实现闭环。`EMB-ACQ-01`、`EMB-VER-*`、`EMB-BLD-01` 仍可在发布构建执行。

---

## 5. 用例矩阵

> 每个用例独立判定。`A` = 缺陷复现相位，`B` = 修复验证相位，`C` = 完整回归。未特别说明时，前置均包括 §3.4/§3.5 的版本、绑定、手机与离线准备，且执行前重新枚举端口/USB。

### 5.1 版本与身份

| ID | 目标/来源 | 前置 | 方法 | 判据 | 证据 |
|---|---|---|---|---|---|
| `EMB-VER-01` | G1：传感器实际镜像/板件身份 | 发布/测试镜像已确认；CW-DAPLink 在位 | 记录构建命令/编译器/提交；`mdk_flash {}` 下载；同窗口 COM43 取上电五行；复算 axf/hex 摘要 | `Verify OK`；axf/hex 摘要与声明一致；`BOOT` 行有序、`uid4` 可读、`uart=9600`；`VDD ok=1` 时 `mv=4095*bgrmv/code`；明确 `fw=FD-002r4` 是冻结横幅、不是版本号 | 构建日志、下载日志、axf/hex 摘要、COM43 原始五行、板件照片/接线 |
| `EMB-VER-02` | G1：App 实际安装字节 | 手机在线、ADB 授权、APK 已构建 | `dumpsys package` 取 versionCode/versionName/安装时间；`pm path` + 拉取 `base.apk` 复算 SHA-256；与构建产物比对 | 设备侧 `base.apk` SHA-256 与交付 APK 一致；versionCode/Name 与交付声明一致；记录 debug/release 与签名 | `dumpsys package` 片段、`base.apk` 摘要、`apksigner`/`aapt` 输出 |
| `EMB-VER-03` | G3：网关身份与转发前置 | 网关上电；手机蓝牙/定位/权限 | App「网关与上传设置」正常扫描；记录候选 MAC/名称/RSSI 并保存；核对 `gateway_ble_addr`；不烧录网关 | 扫描到实际网关并经用户确认（非仅默认迁移值）；App 状态区显示目标 MAC；网关在传感器 433 范围内；网关固件版本明确记为“未知/无回读” | 扫描截图、保存后的设置页、`GwID=` 日志、网关实物/供电照片 |

### 5.2 采集与正常链路

| ID | 目标/来源 | 前置 | 刺激 | 观察与判据 | 证据 |
|---|---|---|---|---|---|
| `EMB-ACQ-01` | G2：真实采集 + 当前判据不上报 | 发布构建（或测试构建 `OFF`）；守护可关；不调温/不调光 | 上电/复位后静置 ≥10 min（≥4 个采样机会） | COM43：`S` 每 180 s 一条、`k` 步长 3；`q=1`；`r=0`、`s=0`、`y=0`；无 `P`；`V=0/D=0` 仅表示判据未启用，不得写成“已识别有光/无光”；`t/h` 为真实值并记录环境；`VDD ok=1` 时复算 `mv=4095*bgrmv/code`，`ok=0` 时如实记录为无效、不伪造；行 ≤96 B、间期无输出 | 原始串口日志、解析表、`VDD`、环境温度/光强文字说明（不作测量结论） |
| `EMB-CHN-01` | G3/G4：网关→手机显示，正常值 | 测试构建 `NORM`；App 已绑定 `uid4`/网关；守护启动；手机离线 | 等待 ≥2 个采样/读取周期 | 传感器 `TINJ it=300 ih=550 f=1`，`P t=300 h=550`，`s=1`；App log `id=uid4, T=30.0, H=55.0`、`dev=1 valid=1`；卡片显示 `30.0°C/55%`；本地库 `temperature` 有新行；守护运行（`startData=1`）；无报警副作用 | 串口 `TINJ/P`、App 日志、UI dump/截图、DB 查询、`dumpsys vibrator/audio` 无活动 |
| `EMB-NF-01` | G5：正常读数不误报 | 测试构建 `NORM`；**全新安装/清空 App 数据后先执行 `EMB-CHN-01`，再执行本用例**（避免旧高值成为下降基准）；默认参数 | 同 `EMB-CHN-01`，持续 ≥3 个读取周期；同一 30.0/55.0 不构成下降（基准与当前同值） | `30.0/55.0` 在默认 [29,35]/[40,75] 内；`AlarmEvaluator` 输出 `NONE/无报警`；`dumpsys vibrator mCurrentVibration=null`、无 App 活跃 player、无 `alarm_channel` 通知；`alarm_events` 无新行；卡片状态 `正常` | 同上 + `alarm_events` 计数前后对比 |

> 说明：`EMB-NF-01` 的 30.0 ℃ 是**代表性被窝内正常值**，不是当前室温；当前室温由 `EMB-ACQ-01` 的真实 `S` 记录。若室温低于默认 `tempmin=29`，不得把真实室温超限直接判为“误报”，也不得用其替代本用例的范围内正常值。

### 5.3 异常报警

| ID | 目标/来源 | 前置 | 刺激 | 观察与判据 | 证据 |
|---|---|---|---|---|---|
| `EMB-ALM-01` | G6：超限报警 | 测试构建 `THR`；默认参数；守护启动；无旧报警/旧基准干扰 | 注入 36.0/55.0，等待 1 个读取周期 | `T=36.0>35`；`AlarmEvaluator` 输出 `THRESHOLD/温湿度超限报警/WARNING`；`alarm_events` 新增 `type=threshold`；卡片 `警告`；`dumpsys vibrator` 有当前震动、`dumpsys audio` 有 App 活跃 player（或操作员确认响铃）、`alarm_channel` 通知存在；默认 900 s 节流内不重复 | `TINJ`、App 日志、`alarm_events` 行、UI、vibrator/audio/notification |
| `EMB-ALM-02` | G6：下降报警 | 测试构建 `DROP_BASE`→`DROP_CUR`；守护启动；本地库已存基准；默认 `tempdrop=2/humdrop=0.1` | 基准 34.9/60.0，约 6 min 后当前 32.0/55.0；等待当前读取 | 基准周期 `THRESHOLD` 不触发（34.9 在 [29,35] 内）；当前 `32.0` 也在范围内，但 `34.9-32.0=2.9>2` 且 `60.0-55.0=5.0>0.1` ⇒ `DROP/温度湿度下降报警`；`alarm_events type=drop`；卡片 `警告`；报警副作用存在 | 两个 `TINJ/P`、每次 App 日志、DB 两条 `temperature` 行、`alarm_events`、UI/播放器 |
| `EMB-ALM-03` | G6：紧急报警 | 测试构建 `EMG`；守护启动；默认开关 | 注入 66.0/55.0 | `>65.0` ⇒ `EMERGENCY/紧急报警，温度超过65度，谨防火灾/ALARM`；即使震动/响铃开关关闭也应震动+响铃（紧急不受开关影响）；`alarm_events type=emergency`；卡片 `紧急` | 同 `EMB-ALM-01` + 文案逐字核对 |

### 5.4 重复报警（缺陷复现与修复判据）

| ID | 目标/来源 | 前置 | 刺激 | 当前（A）预期/修复（B）判据 | 证据 |
|---|---|---|---|---|---|
| `EMB-REP-01` | G7：同一未变化超限读数反复报警 | 测试构建 `THR`；默认参数；守护启动；App 前台/后台均可但保持进程；每次注入相同 36.0/55.0，持续 ≥20 min（覆盖 ≥6 个采样/读取周期与 ≥1 个完整 900 s 窗口） | 同值每 3 min 强制上报，App 每 180 s 读取 | **A（复现）**：第一次超限报警后，在 `lastAlarmTime+900 s` 后的下一次读取再次出现震动/响铃/通知（记录第 2 次时间戳），证明“反复报警”。**B（修复）**：同一未变化值在一个报警“episode”内只产生一次报警；第一次后 ≥900 s 且 ≥3 个读取周期内无第 2 次震动/响铃/通知；只有值变化/条件清除后重新越限才允许新 episode。 | 每个 `TINJ`、App 每次 `id/T/H` 日志、`alarm_events` 行时间、`dumpsys vibrator` 历史、`dumpsys audio` new/releasing、通知时间、UI |
| `EMB-REP-02` | G7：同一未变化紧急读数反复报警 | 测试构建 `EMG`；守护启动 | 66.0/55.0 持续 ≥20 min（≥6 个读取周期，覆盖 ≥1 个 900 s 窗口）；随后切换 `EMG_NEW` 67.0 一次 | **A**：同一 66.0 在每次 BLE 读取（约 180 s）都触发紧急报警（紧急不节流）。**B**：同一 66.0 只产生一次紧急报警；第一次后 ≥900 s 且 ≥3 个读取周期无重复；67.0 作为新值应产生新的紧急报警（明确“新事件仍报警”）。 | 同 `EMB-REP-01` + 67.0 对照 |
| `EMB-REP-03` | G7：App 进程重启后同一缓存值再报警 | 测试构建 `THR` 或 `EMG`；已产生一次报警；网关仍缓存同值 | 报警结束后 `am force-stop com.jinyuni.dengbei_care`，重新启动 App/服务，保持守护运行，等待下一次 BLE 读取 | **A**：因 `deviceHistory/lastAlarmTime` 在内存清零，同一未变化值再次报警。**B**：同一未变化值在重启后不重复报警；只有新值/条件重新越限才报警。记录重启时刻、首次读取时刻、是否报警。 | `am force-stop`/Activity 启动日志、App 日志、`alarm_events`、vibrator/audio/通知 |

> `EMB-REP-01` 的 20 min 窗口是区分“900 s 节流后复报”与“修复后不重复”的最小窗口；执行可分段（同一构建/同一 App 状态），但必须记录窗口起止与间隙，不得把分段拼成连续未验证。

### 5.5 停止与守护

| ID | 目标/来源 | 前置 | 刺激 | 判据 | 证据 |
|---|---|---|---|---|---|
| `EMB-STP-01` | G8：点「停止」后铃声+震动停止（核心缺陷） | 同一相位内 App/传感器构建固定；`EMB-REP-01`/`EMB-ALM-01` 的报警正在响/震动；App 在首页 | 用 `uiautomator` 定位 `startGuardButton`（文字“停止”）并点击；记录点击时刻 | 点击后 **≤2 s**：`dumpsys vibrator mCurrentVibration=null`；`dumpsys audio` 中 App 的活跃 player 变为 idle/releasing/消失；主页按钮文字变“启动”；`startData=0`/`measure_state=starting`；此后同一未变化 36.0 在 ≥900 s 且 ≥3 个读取周期内**不得**再次响铃/震动/通知。**A 预期复现**：按钮点击后仍在响/震动；空白区域点击可能停（区分实验 `EMB-STP-02`）。 | 点击前后 UI dump/截图、`dumpsys vibrator` 前后、`dumpsys audio` 前后、logcat、后续窗口无新报警 |
| `EMB-STP-02` | G8 回归：空白区域取消（现有行为） | 同上，换一次报警 | 在首页空白区域点击（非按钮/非卡片）；记录时刻 | 与 `EMB-STP-01` 相同的 ≤2 s 停止判据；不得因修复按钮路径而破坏该路径 | 同 `EMB-STP-01` |
| `EMB-GRD-01` | G9：停止守护后不再普通报警 + 停止当前报警 | 报警正在响/震动（36.0）；守护运行 | 点击首页「停止」（即 `EMB-STP-01` 的守护停止路径）；随后用 `THR`/`NORM` 注入新读数，观察 ≥2 个读取周期（约 6.5 min） | 点击后 ≤2 s 铃声/震动停止；`startData=0`；新注入的普通超限/正常读数不再产生铃声/震动/通知；App 仍可每 180 s 读 BLE、更新卡片/`alarm_events`（按现有文档 P-11/P-12，不作为失败）；**不得**出现新的 `alarm_channel` 通知。 | 点击前后 UI/播放器、新 `TINJ`/App 日志、通知/vibrator/audio、`alarm_events` 说明 |
| `EMB-GRD-02` | G10：重新启动守护后新事件仍报警 | `EMB-GRD-01` 后守护停止；距上次报警 ≥900 s（或修复已确认重置节流） | 点击「启动」；注入 `THR_NEW` 36.2/55.0（与上次 36.0 可区分）；等待 1 个读取周期 | `startData=1`；`36.2>35` ⇒ 新的超限报警（铃声/震动/通知/`alarm_events`）；卡片/UI 更新；记录与上次报警的时间差；若修复仍保留 900 s 节流，必须等到窗口后再判，不得把节流内不报警误判为失败 | 同 `EMB-ALM-01` + 上次报警时间 |
| `EMB-GRD-03` | G9 的安全例外/范围决策：停止守护 + 紧急 >65 | 守护停止；测试构建 `EMG` | 注入 66.0/55.0，等待 1 个读取周期 | **当前文档行为**：紧急报警仍震动/响铃/通知（用户说明书 §6.2、P-13）。**本轮判据**：作为独立观察项记录；若用户的“停止守护后不再报警”按字面包含紧急报警，则该用例应改为“静默”，属产品安全规则变更，需当前用户明确决定后才可作为强制判据。未确认前，不用它阻塞 `EMB-GRD-01` 的普通报警通过条件。 | 同 `EMB-ALM-03` + 守护状态 |

### 5.6 修复版完整复测与构建恢复

| ID | 目标/来源 | 前置 | 刺激 | 判据 | 证据 |
|---|---|---|---|---|---|
| `EMB-RET-01` | G11：修复版完整复测传感器→手机 | B 相位修复版 App；测试构建；全新安装/清数据；默认参数；网关/手机准备完成 | 在同一会话按顺序复跑：`EMB-ACQ-01`（`OFF`/发布构建）→ `EMB-CHN-01`/`EMB-NF-01`（`NORM`）→ `EMB-ALM-01`（`THR`）→ `EMB-ALM-02`（`DROP`）→ `EMB-ALM-03`（`EMG`）→ `EMB-STP-01`（报警中点击停止）→ `EMB-GRD-01`（停止守护）→ `EMB-GRD-02`（重启守护+新值） | 每个子用例按自身判据通过；版本/构建身份在会话开始与结束一致；App 无 `FATAL`/`ANR`；网关只转发，平台/云无判定；`EMB-REP-01/02/03` 的修复判据在该会话中至少各复跑一次 | 统一证据索引：版本摘要、串口原始日志、App logcat、dumpsys、UI/DB、每用例判据表 |
| `EMB-BLD-01` | 测试构建排除与交付板件恢复 | 测试构建已使用；发布包可用 | 按 §4.4 重建发布构建并做字符串/符号检查；`mdk_flash` 下载 `fw_release/fw_sensor_FD-002r8.zip`；COM43 取 `BOOT` | 发布构建无 `TINJ`/测试符号/字符串；`Verify OK`；`BOOT uid` 可读；板上无测试构建；记录恢复时间与摘要 | 构建日志、nm/字符串检查、下载日志、`BOOT` 行、摘要 |

### 5.7 回归与稳定性

| ID | 目标 | 方法 | 判据 |
|---|---|---|---|
| `EMB-REG-01` | 修复不破坏既有 UI/服务/通知 | 冷启动；首页/我的/设定/报告入口/传感器列表；守护启停；设置页默认值显示；通知渠道 | 页面可达、无 `FATAL`/`ANR`；默认值显示与首装值一致（2/0.1/35/29/75/40/8/10/四开关=1）；通知渠道存在；不出现平台/云上传相关失败被误判为报警失败 |
| `EMB-REG-02` | 长窗口稳定性（有界） | 保持守护/测试构建 `NORM` 或 `THR`，观察 ≥6 个采样/读取周期（约 18 min） | 无崩溃/ANR；`S` 行连续、`k` 步长 3；App 每周期有读取终态；无未解释的重启/断连；失败率/长期稳定性只记录，不扩成通过结论 |

---

## 6. 证据规范与关联模板

### 6.1 每个用例必须保存

1. **版本**：仓库 HEAD、传感器 axf/hex 摘要与下载日志、App APK/设备 `base.apk` SHA-256 与 versionCode/Name、网关 MAC/名称/`GwID`。
2. **刺激**：测试构建模式、注入值、`TINJ` 行、App 参数实际值、守护状态、点击/注入时刻。
3. **原始观测**：COM43 原始串口日志（字节+时间戳）；App logcat 整窗；`dumpsys vibrator/audio/notification` 前后快照；UI dump/截图；本地库副本/查询结果。
4. **判据表**：逐项写“期望/实际/通过/失败/NOT EXECUTED”，不得只写总结。
5. **清理**：停止测试注入、停止守护/服务、恢复 App 参数、恢复发布固件、关闭实验室会话。

### 6.2 关联模板

```text
uid4=<BOOT uid 前 4B>
sensor_build=<axf/hex sha256 + Verify OK>
app_build=<versionCode/versionName + APK sha256 + 设备 base.apk sha256>
gateway=<BLE MAC/name + GwID（如有）>
window=<start/end + timezone>
stimulus=<mode, inj_t, inj_h, force, repeat>
sensor_evidence=<TINJ/P 行原文 + k>
app_evidence=<MqttService id/T/H 行 + 读取时刻>
db_evidence=<temperature 行 / alarm_events 行>
ui_evidence=<截图/UI dump 路径>
alarm_evidence=<vibrator/audio/notification 时间戳>
verdict=<pass/fail/not_executed + 逐项依据>
```

### 6.3 证据边界

- 串口 `s=1` 只证明 UM2005C 发送函数返回成功，不证明网关收到；App 收到匹配值才证明转发。
- `dumpsys vibrator/audio` 证明系统层面的当前震动/播放状态；不证明用户实际听到/感到。
- 本地库/UI 证明 App 消费；不证明平台/云端。
- 注入值不是物理量；不证明传感器对 30.0/36.0/66.0 的真实响应、电气/模拟指标、功耗或现场条件。
- `BOOT fw=FD-002r4` 是冻结字符串；版本以源码提交 + 构建产物摘要 + `Verify OK` 为准。

---

## 7. 退出准则（执行阶段）

### 7.1 本阶段通过的强制条件

执行阶段在 **B 相位修复版 App + 同一测试构建** 上，以下用例全部有原始证据并逐项通过：

- 版本/准备：`EMB-VER-01`、`EMB-VER-02`、`EMB-VER-03`；
- 采集/显示/正常：`EMB-ACQ-01`、`EMB-CHN-01`、`EMB-NF-01`；
- 异常报警：`EMB-ALM-01`、`EMB-ALM-02`、`EMB-ALM-03`；
- 反复报警：`EMB-REP-01`、`EMB-REP-02`、`EMB-REP-03` 的**修复判据**；
- 停止/守护：`EMB-STP-01`、`EMB-GRD-01`、`EMB-GRD-02`；
- 完整复测：`EMB-RET-01`；
- 构建恢复：`EMB-BLD-01`；
- 回归：`EMB-REG-01`。

任一强制用例实际行为不符、仍适用的验证未完成或证据不足 ⇒ 执行阶段返回 `TEST_FAIL`，写明用例、期望/实际、原始证据缺口；交实现修复后在同一用例/同一构建身份上复测。**未执行始终不等于通过；阶段通过不等于完整产品通过。**

### 7.2 不计入强制通过、但必须记录的项

| 项 | 处理 |
|---|---|
| `EMB-STP-02` 空白区域取消 | 回归项；若失败，记录为修复引入的回归，交实现；不作为本轮新增产品要求 |
| `EMB-GRD-03` 停止守护 + 紧急 >65 | 现有产品安全例外；按当前文档记录实际行为。是否把“停止守护后不再报警”扩展到紧急报警，属产品/安全范围决策，需当前用户明确；未确认前不作为 `EMB-GRD-01` 的阻塞条件，也不写成通过 |
| `EMB-REG-02` 长窗口 | 有界稳定性观测；失败率/长期稳定性不扩成结论 |
| 真实室温下的默认阈值 | `EMB-ACQ-01` 记录真实值；若低于 `tempmin=29`，属环境/默认参数匹配问题，不得直接判 App“误报”；被窝内真实摆放为可选增强，不作为未执行项强行要求 |
| 平台/云/数据库/iOS | 明确不在本轮范围；NOT EXECUTED，不作为通过或失败依据 |
| 光照/温度/功耗调节与测量 | 用户明确不做；NOT EXECUTED，诱因与缺口保留，不用模拟冒充 |

### 7.3 不得写成通过的情形

- 没有测试构建、只做了发布构建真实采集 ⇒ 正向链路/报警用例 `NOT EXECUTED`，不能写“链路通过”。
- 只看到 `s=1` 或只看到 App 有日志、没有匹配的 `T/H/uid4` ⇒ 不能证明同一事件。
- 只点了空白区域能停、没点「停止」按钮 ⇒ 不能证明“点停止关不掉”已修复。
- 只验证了同一值不重复，未验证新值仍报警 ⇒ 不能证明“重新启动守护后新事件仍报警”。
- 只验证了正常值不报警，未验证异常值能报警 ⇒ 不能证明报警链路。
- 测试构建留在板上 ⇒ 不能作为发布/交付状态。

---

## 8. 限制、未决与交接

### 8.1 关键限制

1. **测试构建依赖**：正向链路/报警/停止/守护用例依赖实现提供 §4 的测试构建；这是当前固件判据与用户环境共同决定的真实缺口。若执行阶段未取得，按 §4.4 末尾处理，不转给人工、不写成通过。
2. **App 未安装**：设计时 `pm list packages` 无本 App；执行阶段先安装当前/修复候选，并记录实际字节。
3. **网关无版本回读**：只能记录 BLE MAC/名称/`GwID`；网关固件版本未知；不烧录、不修改。若网关已有其它绑定设备，需按 §3.4 记录并排除。
4. **手机通知/铃声/震动环境**：MIUI 可能限制通知、后台、精确闹钟或默认铃声；执行前必须逐项确认并记录；环境不满足时，相关用例不得用“没响”直接判代码失败，应记录为环境/前置缺口并区分。
5. **物理量未证明**：注入值只证明判定/组帧/发送/网关/App 消费；不证明 GXHT40 对注入温度的物理响应、模拟幅度、电气、功耗或现场条件。
6. **紧急 >65 的守护例外**：当前产品文档明确 `>65 ℃` 紧急报警不受守护停止影响；本轮按安全例外单列（`EMB-GRD-03`），不擅自改动；若用户要求“停止守护后连紧急报警也静默”，需明确范围决定后更新本设计与产品规则。

### 8.2 交接

| 事项 | 责任 | 完成条件 |
|---|---|---|
| 测试专用注入/强制上报构建（§4） | 固件实现 | 默认关闭、发布排除、`TINJ` 可区分、宿主双构建/字符串检查、`Verify OK`；提供构建目标/摘要 |
| App 停止/重复报警/守护修复 | Android 实现 | 基于 A 相位证据定位并修复；重建测试候选；记录新 APK SHA-256/versionCode；不改变未授权范围 |
| App 安装、版本/UI/logcat/dumpsys/DB 观测 | Android/软件测试执行能力 | 按 §2.3/§6 保存原始证据；与嵌入式窗口对齐 |
| 传感器串口/烧录/复位/测试构建复测 | 嵌入式测试执行能力 | 按 §5 执行；测试后恢复发布镜像（`EMB-BLD-01`） |
| 紧急 >65 守护例外是否纳入“不再报警” | 当前用户/验收范围所有者 | 明确决定后更新 `EMB-GRD-03` 与退出准则；未决定前不阻塞 `EMB-GRD-01` |
| 平台/云/DB/iOS | 不在本轮 | 不创建下游节点、不修改 |

### 8.3 现实条件与未执行边界

- 当前不能调节温度、不能改变光强、不能测功耗：本设计不安排这些实验；真实 `S/V/D/VDD` 只作观测，不能据此宣称光照/功耗/现场通过。
- 逻辑分析仪 `sigrok_info` 已配置但本轮未探测；不把 I²C 波形作为任何必需条件；若执行时使用，另存原始 `.sr` 再离线解码，并保持 `D0=SCL/D1=SDA/GND 共地` 与输入电平核实。
- 平台/云上传/数据库/iOS：NOT EXECUTED；不写通过，不作为本阶段失败依据。
- 真实被窝内正常值：可选增强；不因未做而阻塞 `EMB-NF-01` 的注入代表性正常值，但必须披露物理响应未证明。

---

## 9. 设计自检（提交前逐项对照）

- [x] 每个强制用例都有目标来源、被测版本、前置、刺激、观察点、判据、证据与清理。
- [x] 判据使用实际源码行为（严格 >35.0、严格阈值、5/10/15 min 下降窗、900 s 普通节流、紧急不节流、守护门控、`startData`/`measure_state` 反向命名）。
- [x] 正常/异常/重复/停止/守护/重启/回归均有独立用例；没有把“未执行”写成“通过”。
- [x] 退出准则只汇总 §5 中当前有效范围内的强制用例；`EMB-GRD-03`、`EMB-STP-02`、`EMB-REG-02`、平台/云/功耗等单列为非强制或 NOT EXECUTED，未反向阻止本阶段退出。
- [x] 测试构建依赖、App 未安装、网关无版本回读、手机通知/铃声/震动环境等缺口已写明最小下一步与责任，没有转给人工或写成已排除实现问题。
- [x] 未安排调温、调光、功耗测量；未修改产品源码/网关/平台；未创建下游流程节点。
- [x] 新增文件已登记 `file_manifest.txt`；本仓库 `scripts/check_file_manifest.py` 不存在，已用只读路径存在/回读检查核对新增条目。
