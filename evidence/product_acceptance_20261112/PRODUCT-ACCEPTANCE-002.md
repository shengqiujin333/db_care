# 产品整体功能验收证据（PRODUCT-ACCEPTANCE-002）

- 负责角色：软件测试
- 能力：产品整体功能验收（`product_check`）
- 时点：2026-11-12 10:56–11:10（Asia/Shanghai；工具时间戳为 UTC 02:56–03:08）
- 修订：2026-11-12 11:47–11:54（UTC 03:47–03:54）按人工 REASSESS（HR-5BE4F69D9F33）追加 §3.5 BOT-01 上电五行复用复核；其余本轮实测内容未变，未重跑既有用例
- 验收对象：当前完整候选（传感器固件 FD-002 rev 8 发布包 `fw_sensor_FD-002r8.zip` / 安卓 1.9 debug 候选 AB-002 / 既有网关与平台服务 / 文档包 DOC-PKG-001 rev 12）
- 验收仓库：`D:\mypro\beiwo2\AIP`，接手时 HEAD `db5320291b47ee93dd0d44611c1bf85ac874f189`，工作树干净（仅本轮新建证据目录）
- 前次总测：`evidence/product_acceptance_20261111/PRODUCT-ACCEPTANCE-001.md`（旧 APK `d2054221…`，`TEST_FAIL`/D-01～D-04）
- **结论：`TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）**
  - 前次实测缺陷 **D-01/D-02/D-03/D-04a 在当前候选 APK `71a041d3…` 上已逐项重新实测关闭**；本轮未发现新的软件实测缺陷。
  - 但需求 4 的“无光”分支仍按交付默认 `LIGHT_DARK_THRESHOLD_CODE=0` 禁用（`FW-LIGHT-DEF2-ENABLE` 为人工确认的 `SKIP_CONFIRMED`，NOT EXECUTED）；真实升温/超温触发、真实光强双态、功耗、网关→手机跨链路、`/upload_data` 写入回读均未执行；上电五行按人工 REASSESS 复核后复用同镜像既有证据（§3.5），不是本轮新采集。按本能力规则，未执行/未核验的必需项保持未满足，不能以可执行子集汇总通过。

> 本报告只记录本轮实际执行与可复核证据；未执行、延期与人工决定逐项披露。旧代次的局部 PASS 没有被用来宣布整体通过。

---

## 1. 候选身份（本轮实际复算/核对）

| 对象 | 身份 | 本轮证据 |
|---|---|---|
| 仓库 | HEAD `db5320291b47ee93dd0d44611c1bf85ac874f189`；接手时 `git status` 仅本轮证据目录未跟踪 | `identity_checks.txt` §HEAD/status |
| 传感器固件发布包 | `fw_release/fw_sensor_FD-002r8.zip`，134,791 B，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf` | 本机 `md5sum` 复算一致 |
| 包成员 | axf `b6a15fed2ed5e3d0c4adc51ac87a4830`（297,380 B）/ hex `4d728dec9b0d28a352c46436b7586c72`（75,275 B）/ PACKAGE.txt `a58f67fcf5b760767ff5b65888c73dfb` | 流式 `unzip -p | md5sum` 复算一致 |
| 固件产品源 | 提交 `82a7e74e5d6bae855a573e1f23f83936ba8db049`（FD-002 rev 8）；`git diff --stat 82a7e74..HEAD -- <sensor>/USER MDK EWARM` **为空** | `static_checks.txt` §git |
| 板上镜像 | EV-038 于 `2026-11-11T12:00:11.910Z` 以 `mdk_flash {}` 下载同一包成员（`Verify OK`）并复位（`pyocd` HW reset，`12:00:23Z` 起 `k=0`）；本轮 `k=906`，与 EV-038 复位后经过时间（约 902.3 min）相差 **+0.41%**，与“同一启动会话连续运行、未再复位”一致；镜像哈希无法从板端回读，属推断 | §3.1、`evidence/ev038/capture_windows.txt` |
| 安卓候选 APK | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk`，10,365,464 B，SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`，MD5 `1e17e0c281715fddd26d308246d6424c` | `identity_checks.txt` §APK |
| 设备在装包 | `com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，debug；设备 `base.apk` 大小 10,365,464 B、SHA-256 `71a041d3…`、MD5 `1e17e0c2…`（**与候选逐字节一致**，设备侧复算） | `device_package_dump.txt`、`device_apk_hash.txt` |
| 设备 | Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506`，本机在线 | `android_test devices`、§3.2 |
| 网关 | 既有 CH592 `beiwov2`；本轮未修改、未烧录、未接线核验 | 用户约束 + 未执行 |
| 平台服务 | 既有部署（用户确认复用）；候选 `CloudConfig.kt` 为 `8.140.23.253` / `ssl://…:8883` / `http://…:5000`；本轮**未连接、未写入、未连库** | `static_checks.txt` §CloudConfig.kt |
| 文档包 | DOC-PKG-001 rev 12；`git diff 92bfcb2..HEAD -- docs dengbei_care/用户使用说明书.md` **为空**；DOC-VERIFY-012 独立校核 `TEST_PASS`（限定 rev 12 改动与受影响步骤） | `evidence/doc_verification_20261112_rev12/DOC-VERIFY-012.md` |

---

## 2. 执行方式、授权与环境边界

| 项 | 本轮做法 |
|---|---|
| 串口观测 | MCP `serial_list_ports` + `serial_capture`（COM43 / 9600 8N1，300 s）；未发送任何字节 |
| 手机操作 | MCP `android_test`（devices/launch/ui_dump/ui_click/ui_swipe/ui_press/ui_info）；ADB 只读 `dumpsys package`、`pm path`、设备侧 `stat/sha256sum/md5sum`、`uiautomator dump`、`logcat -d` |
| USB 枚举 | MCP `usb_list_devices`（只证明连接/身份） |
| 静态核对 | 限定目录 Grep/Read + 可复跑脚本 `collect_static.py`、`parse_nodes.py`；运行清单检查脚本 |
| 未使用 | `mdk_flash`/烧录/复位（`sensor_programming` 对本能力不可用，pyocd 未登记）、sigrok 逻辑采集、平台 SSH/写接口、`/upload_data`、数据库直连、调温/调光/遮光刺激、功耗测量 |
| 原因 | 当前能力有效工具为 `android_test`、`usb_list_devices`、`serial_list_ports`、`serial_capture`；温度/光强/功耗受用户明确条件限制；跨链路按用户安排属后续联调阶段 |

---

## 3. 本轮真实执行记录

### 3.1 传感器 COM43 周期观测（真实目标）

- 窗口：`2026-11-12T03:02:42.532Z – 03:07:42.747Z`（300 s）；COM43 = Prolific `067B:2303`，9600 8N1；收到 **160 B**，md5 `85720b39028a8adb67a829f23a7e587a`，SHA-256 `c12b04866f50b78a4fc454e299b2b27d191078b2394d4cff2fdfd0974bffcd1a`。
- 原始两行（CRLF 结尾，逐字节保留于 `serial_capture_com43_20261112.bin`；仓库内以 `serial_capture_hex.txt` 复核）：

```text
S k=906 p=245 H=1 V=0 o=8 n=3131 x=3137 a=3133 D=0 t=246 h=408 q=1 r=0 s=0 y=0
S k=909 p=246 H=1 V=0 o=8 n=3119 x=3126 a=3122 D=0 t=247 h=413 q=1 r=0 s=0 y=0
```

判读（只记录事实）：

1. **采样节拍**：300 s 窗口内 `k` 从 906 到 909，步长 **3**，与 `SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3`（每 3 分钟采样）一致；两行之间无其它字节。
2. **前值链**：`p(k=909)=246 = t(k=906)`，`H=1`，前值语义自洽。
3. **采样结果**：两周期 `q=1`、`o=8`；`t` 24.6/24.7 ℃、`h` 40.8/41.3 %RH 为合理实测量；`n/x/a` 为光敏 ADC 原始事实（`x` 与 `a` 在 `n` 上下小幅波动，说明本周期确有 ADC 采样）。
4. **判据默认态**：`V=0 D=0`（交付默认 `LIGHT_DARK_THRESHOLD_CODE=0`；该状态**不得**解释为“已识别有光”）；`r=0 s=0 y=0`，无 `G`/`P`，本窗口无上报。
5. **启动会话推断**：EV-038 复位参考 `2026-11-11T12:00:23Z` 起 `k=0`；本轮 `k=906` 对应约 906 min，实际经过 902.3 min（+0.41%，属 LSI 量级偏差）。在“`k` 于复位时归零”的前提下，当前板卡自 EV-038 下载后连续运行、未被再次复位。**镜像哈希不可从板端回读**，身份由“EV-038 下载记录 + `k` 连续 + 源码零差异”闭合，属推断而非独立哈希证明。
6. **未取得（本轮新采集）**：本轮没有重新取得上电五行。按人工 REASSESS 复核后确认既有 EV-038 上电五行与当前固件身份一致，**复用**已核实证据（依据见 §3.5、`bot01_reassessment.txt`），不再请求复位/烧录授权；该复用不是本轮新采集。

原始：`serial_capture_com43_20261112.bin`、`serial_capture_hex.txt`、`serial_capture_meta.txt`。

### 3.2 安卓真实设备执行（当前候选 APK）

| 步骤 | 实际结果 | 证据 |
|---|---|---|
| 设备在线 | `12f046690506 device product:olivelite model:Redmi_8A` | `android_test devices` |
| 在装身份 | versionCode 10 / 1.9，debug（`pkgFlags=[DEBUGGABLE…]`）；`base.apk` 与候选逐字节一致 | `device_package_dump.txt`、`device_apk_hash.txt` |
| 启动 | `launch` 成功；`MainActivity` 前台；进程存活（pid 18030） | `activity_final.txt` |
| 首页 | 「爱守护」「启动」「去添加」；文案“点击下方「去添加」添加设备；也可在「我的」→「打开传感器列表」查看已保存 ID”；链路状态“未配置网关地址（搜索附近网关）” | `ui_home_launch.xml/.txt` |
| 我的页（顶部） | 「查看今日报告」`[32,327][688,423]`、「打开传感器列表」`[32,431][688,527]`，均在底部导航 `[0,1264][720,1424]` 之上 | `ui_my_page.xml/.txt` |
| 我的页（上滑） | 「注册传感器」`[32,976][688,1072]`、「网关与上传设置」`[32,1120][688,1216]`，**两控件不相交（间隔 48 px）**且均在导航之上 | `ui_my_scrolled.xml/.txt` |
| 查看今日报告 | 点击后 `mResumedActivity=com.jinyuni.dengbei_care/.report.ReportActivity`；页面“早起报告 / 尚未生成过报告 / 重新生成” | `activity_after_report_click.txt`、`ui_report_activity.xml/.txt` |
| 打开传感器列表 | 点击后进入 `sensorIdListRoot`（`recyclerSensorIds`），显示“已保存的 ID（0）”“删除选中”；`MainActivity` 未跳页签 | `ui_sensor_id_list.xml/.txt`、`activity_after_sensorlist_click.txt` |
| 首页「去添加」 | 同样进入“已保存的 ID（0）”列表页 | `ui_home_add_route.xml/.txt` |
| 崩溃检查 | 会话 logcat 尾部 5,667 行：0 条本应用 `FATAL EXCEPTION`、0 条 `ANR in com.jinyuni…` | `logcat_session_tail.txt` |

### 3.3 平台配置静态核对（候选实际使用既有服务对象）

候选 `cloud/CloudConfig.kt:18/21/24/27/30` 的实际常量：`SERVER_HOST=8.140.23.253`、`MQTT_BROKER_PORT=8883`（`ssl://`）、`HTTP_PORT=5000`（明文 HTTP），与 `artifacts/backend_feasibility_current.md`（BE-EXP-004，2026-11-10 只读核验）记录的既有部署一致。边界：本轮**未连接服务器、未调用任何接口、未读取服务端文件、未直连数据库**；该核对只证明“最终候选配置指向既有服务对象”，**不证明** `/upload_data` 可用。

### 3.4 文档与实现一致性

DOC-VERIFY-012（rev 12，独立校核）结论 `TEST_PASS`（限定 rev 12 改动与受影响步骤）：151 个路径引用全部可解析、固件/APK 身份与文档一致、真机入口与 bounds 一致、未执行项披露一致。本轮确认 `docs/` 与随包用户说明书自 rev 12 提交后**零差异**，未复用旧版本校核结论。

### 3.5 BOT-01 上电五行复用复核（人工 REASSESS 后）

人工回复（HR-5BE4F69D9F33，2026-11-12T03:41:48Z）：选择 **REASSESS**，明确“不跳过。先核对已有上电五行实测证据是否仍对应当前固件，匹配则复用并说明依据，不无理由重采”。本轮据此只做一致性核对，未复位、未烧录、未改固件、未重采串口。

| 核对项 | 实际结果 | 证据 |
|---|---|---|
| 发布包身份本轮复算 | `fw_sensor_FD-002r8.zip` 134,791 B md5 `5df0c6d4…`；成员 axf `b6a15fed…`、hex `4d728dec…` 与 EV-038 记录逐项一致 | `bot01_reassessment.txt` [A] |
| EV-038 镜像来源 | EV-038 由 `82a7e74` 源码独立重建，axf/hex 与发布包成员逐字节一致；`mdk_flash {}` 2026-11-11T11:59:59.554–12:00:11.910Z `Verify OK` | `evidence/ev038/tool_results.txt` |
| 上电行采集条件 | 端口先开（12:00:20.923Z），pyocd HW reset 12:00:22.932–12:00:23.645Z（rc=0，外部复位源、无 flash/内存写）在窗口内；271 B，md5 `a5cc0252…` | `evidence/ev038/capture_windows.txt`、`pyocd_reset_boot.txt` |
| 五行原始内容 | `BOOT`/`VDD`/`IOTEST`/`IOSIG`/`BUS` 各一次且顺序正确，另有首周期 `S k=0`；独立解析 PASS=129 FAIL=0 | `evidence/ev038/bot01_hw.cap`、`capture_analysis.txt` |
| 产品源是否仍一致 | HEAD `db53202`；`git diff 82a7e74..HEAD -- <sensor>/USER MDK EWARM` **为空**；差异仅在 `test/` 宿主 harness/mock（5 文件），不入发布包；发布包自 `5b9b7aa` 后无变更 | `bot01_reassessment.txt` [C] |
| 横幅 `fw=FD-002r4` 是否错配 | **否**：`fw_release/PACKAGE.txt` 明示“fw string intentionally unchanged”；当前 `debug_trace.c:214` 仍输出同一字符串 | `PACKAGE.txt`、`static_checks.txt` |
| 板端是否在 EV-038 后重烧/复位 | 以 EV-038 复位为基准：窗口起点 902.3 min 对应 `k=906`（+0.41%），终点 907.3 min 对应 `k=909`（+0.19%）；与“连续运行、未再复位”一致 | 上述串口原始捕获 |

**判定：MATCH，复用 EV-038 上电五行作为当前候选的 BOT-01 证据。** 复用成立的三条硬条件：①采集对象与当前发布包为同一二进制（两次独立复算哈希一致）；②固件产品源与发布包自 EV-038 后零差异；③板端 `k` 连续，支持该镜像持续运行。**这不是本轮新采集**，且若产品源、包成员或板端镜像任一变化即失效，须重新采集。局限：板端镜像哈希不可回读，身份属“哈希 + 零差异 + k 连续”闭合推断（见 §9）。

---

## 4. 前次实测缺陷的关闭复核（当前候选 APK）

| ID | 前次事实（旧 APK `d2054221…`） | 本轮结果（当前 APK `71a041d3…`） | 证据 |
|---|---|---|---|
| D-01 | 「查看今日报告」被底部导航拦截，点击切到「设定」 | **已关闭**：点击直达 `ReportActivity` | §3.2 |
| D-02 | 「打开传感器列表」被底部导航拦截 | **已关闭**：点击进入 `SensorIdListFragment` | §3.2 |
| D-03 | 「网关与上传设置」与「注册传感器」bounds 重叠 | **已关闭**：`[32,976][688,1072]` 与 `[32,1120][688,1216]` 不相交 | §3.2 |
| D-04a | 首页文案“前往「我的」→「传感器列表」”与实际路径不一致 | **已关闭**：文案改为“点击下方「去添加」…也可在「我的」→「打开传感器列表」…”，两条路径均实测可达 | §3.2 |
| D-04b | 手册“当前无 UI 路径查看/删除”事实错误 | **已关闭（文档侧）**：rev 12 已修正并独立校核；有数据时的列出/删除仍未执行 | DOC-VERIFY-012 |

**本轮未发现新的软件实测缺陷。** 观察项：`docs/README.md:81`、`docs/06:122` 仍写“整产品验收尚未重跑”，该表述在 rev 12 时点属实；本报告产生后已过时，需在下次文档修订中更新（交技术文档，不影响当前候选功能）。

---

## 5. 必需功能与验收条件逐项结果

| # | 需求/验收项 | 结果 | 依据与边界 |
|---|---|---|---|
| R1 | 温度传感器改为 GXHT40 | **通过（读数路径，有限）** | 本轮 `q=1`、`t/h` 合理；EV-038 位级 I²C 读 `0x44`、双 CRC 复算 26.3 ℃/37.7 %RH；产品源自发布提交零差异。未做长期稳定性 |
| R2 | 光敏电阻+分压电路与光照采样 | **部分（采样有效，判据未启用）** | 固件 ADC 采样有效（`o=8`、`n/x/a` 域内且逐周期变化）；`V=0 D=0` 为 `T=0` 交付默认，**不得**判为“已识别有光”。真实遮光/有光双态未执行（N-1/N-3）。电路电气特性不在本能力范围 |
| R3 | 采集时间改为 3 分钟 | **通过（实板节拍）** | 300 s 窗口 `k` 906→909 步长 3；源码 `SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3`；未声称 LSI 精度 |
| R4 | 上传规则：ΔT>0.9 ℃ 且无光，或 >35 ℃ | **未满足/未执行** | 源码条件为 `(cur-prev)>9 且 dark` 或 `cur>350`（严格大于）；但交付默认 `T=0` ⇒ 无光分支永不触发，仅保留 >35 ℃ 分支。本轮本窗口 `r=0 s=0`、无 `P`；真实升温/超温与 433 实发受用户条件限制未执行（N-2）。见 N-1 |
| R5 | 平台 broker/服务信息 | **部分（静态一致，未连接）** | 候选常量指向 `8.140.23.253:8883/5000`，与既有服务一致；本轮无平台只读 SSH 外部动作与写授权，未连接、未写入、未连库（N-5） |
| R6 | 同步修改安卓 App（iOS 不改） | **通过（当前 APK 已执行范围）** | 候选已安装且字节一致；D-01～D-04a 逐项关闭；首页/我的/报告/列表/网关对话框可用、无崩溃。真实 BLE/上传链路未执行（N-4/N-8） |
| R7 | 当前有光 | **条件确认（未判定光路）** | 用户声明当前有光；本轮未改变光强、未遮光；`a` 仅作事实记录，不据此判“有光”或光路好坏 |
| R8 | UART1 调试串口初始化与打印 | **通过（周期行实测；上电五行复用已核实证据）** | 本轮取得两条 `S` 行（≤96 B、CRLF 完整）；`debug_trace.c/h` 拥有 PA06 TXD/PA05 RXD、9600 8N1、上电横幅与周期行。上电五行未重采；按人工 REASSESS 复核确认 EV-038 证据与当前固件同镜像后**复用**（§3.5、N-7） |
| R9 | 取消光通讯与霍尔识别 | **通过（静态）** | 当前源码仅在退役注释中出现 hall/OPTCFG；`interrupts_cw32l010.c:186` 注明 PB04 已改 `LIGHT_ADC`、不再有霍尔 EXTI；产品源自发布提交零差异；EV-038 静态 21/21 |
| E2E | 传感器→433→网关→BLE→手机→平台完整业务链路 | **未执行** | 跨链路按用户安排属后续联调阶段；网关未配置、手机未绑定；平台未写入（N-3/N-4） |

---

## 6. 未执行项、延期与人工决定（必须随交付披露）

| ID | 项 | 状态 | 原因/后续责任 |
|---|---|---|---|
| N-1 | `FW-LIGHT-DEF2-ENABLE`（无光判据启用与实板双态） | **NOT EXECUTED（人工确认 `SKIP_CONFIRMED`，2026-11-11T03:50:42Z，回复「无法准备，先跳过」）** | 缺完全遮光窗口与同板同构建的光强双态证据；无判据形制/数值接受记录；`T` 保持 0。本报告不重测、不重复询问，也不写成通过。后续：上游接受判据形制/数值并由固件实现回填、重建后复验 |
| N-2 | 真实 ΔT>0.9 ℃、>35.0 ℃ 触发与 433 实发 | **未执行** | 用户明确当前不能调节温度；本窗口无 `P`/`r=1`/`s=1`。后续：具备调温条件时按 `docs/03` 判读 |
| N-3 | 光强变化/遮光刺激 | **未执行（不安排）** | 用户明确当前不能改变光强；与 N-1 的人工延期一致 |
| N-4 | 433→网关→BLE→手机跨链路与同笔一致性 | **未执行** | 用户安排后续联调阶段；网关身份、供电与配对未验证；手机未配置网关地址 |
| N-5 | `/upload_data` 真实写入、幂等与回读 | **未执行** | 当前能力无写接口/数据库授权；平台只读 SSH 入口对本能力不可用 |
| N-6 | `E2E-04` CR2032 峰值/深睡电流 | **未执行（已批准延期记录在案）** | 无电流表/SMU/示波器；用户明确不能测功耗 |
| N-7 | 上电五行（BOT-01） | **已按人工 REASSESS 复核并复用（非本轮新采集）** | 人工回复 2026-11-12T03:41:48Z：不跳过、先核对已有证据，匹配则复用。核对结果 MATCH：EV-038 采集对象与当前发布包同一二进制（axf `b6a15fed…`/hex `4d728dec…`）、产品源 `82a7e74..HEAD` 零差异、板端 `k` 连续；故复用 `bot01_hw.cap` 五行（§3.5、`bot01_reassessment.txt`）。未复位/未烧录/未改固件。板端哈希不可回读，属推断；产品源、包成员或镜像任一变化即失效 |
| N-8 | 手机端真实 BLE/GATT/MTU、数据展示、告警、上传状态 | **未执行** | 未配置网关、无已绑定传感器（0 条）；不自行扫描/配对（跨链路阶段） |
| N-9 | 注册/登录、网关编码保存、真实告警参数下发 | **未执行** | 无账号/网关编码等外部数据；不写入平台、不改用户数据 |
| N-10 | 「打开传感器列表」有真实绑定数据时的列出/单项/批量删除 | **未执行** | 当前无绑定数据；仅验证空态入口与删除控件存在 |
| N-11 | API 31+ 权限分支、9 台真实快照、第 10 台拒绝 | **未执行** | 设备为 API 29；无真实多设备源 |
| N-12 | iOS | **不在本轮范围** | 用户明确暂不修改 iOS |

**人工决定汇总（明示）**：

1. `FW-LIGHT-DEF2-ENABLE`：经人工确认**未执行**（`SKIP_CONFIRMED`，「无法准备，先跳过」，2026-11-11T03:50:42Z）；保持 `T=0`，本报告不写成通过。
2. `E2E-04`：已批准 `DEFERABLE`，本轮无仪器，未测并在此列明。
3. 真实升温/调光/遮光：用户明确当前不能改变温度/光强，不安排相关硬件测试。
4. 网关→手机跨链路：用户明确安排在后续联调阶段，本轮不据此反复退回固件实现。
5. BOT-01 上电五行：人工选择 **REASSESS（不跳过）**；本轮按要求核对后判定既有 EV-038 证据与当前固件匹配并**复用**（§3.5），未请求新的复位/烧录授权，也未重采。

---

## 7. 影响范围与后续入口

| 入口 | 说明 |
|---|---|
| 固件实现/需求 | R4 无光分支仍为 `T=0`：待判据形制/数值接受记录与同板同构建双态证据后回填 `T` 并重建；本轮不修改常量、不强推 |
| 后续联调 | 网关供电/身份/BLE 配对、433 实发、手机同笔一致性、`/upload_data` 写入与回读、功耗测量按各自授权执行 |
| 技术文档 | 下次修订更新“整产品验收尚未重跑”的表述并引用本报告；只复核受影响条目，不复用旧版本校核结论 |
| 硬件测试 | 光路/分压电气特性、功耗与 CR2032 供电能力由硬件测试在具备仪器时提供独立证据 |

---

## 8. 证据清单（本目录）

| 文件 | 内容 |
|---|---|
| `PRODUCT-ACCEPTANCE-002.md` | 本报告 |
| `identity_checks.txt` | HEAD/status、固件包与成员 md5、APK 大小/SHA-256/MD5、文档版本头 |
| `device_package_dump.txt` / `device_apk_path.txt` / `device_apk_hash.txt` | 设备 `dumpsys package`、安装路径、设备侧 APK 大小/哈希（与候选逐字节一致） |
| `serial_capture_com43_20261112.bin` / `serial_capture_hex.txt` / `serial_capture_meta.txt` | COM43 原始 160 B、hex、md5/sha256 与窗口参数（`.bin` 命中 `.gitignore`，本机留存；随仓库以 hex 文本复核） |
| `ui_home_launch.xml/.txt` | 首页 UI 原始树/文本（新文案与「去添加」） |
| `ui_my_page.xml/.txt` / `ui_my_scrolled.xml/.txt` | 我的页顶部/上滑后 UI 树与 bounds |
| `ui_report_activity.xml/.txt` / `activity_after_report_click.txt` | 点击「查看今日报告」后的报告页与 Activity 栈 |
| `ui_sensor_id_list.xml/.txt` / `activity_after_sensorlist_click.txt` | 点击「打开传感器列表」后的列表页与 Activity 栈 |
| `ui_home_add_route.xml/.txt` | 首页「去添加」到达列表页 |
| `activity_final.txt` | 本轮结束时 Activity 状态 |
| `logcat_session_tail.txt` | 本轮会话 logcat 尾部（0 条应用 FATAL/ANR） |
| `static_checks.txt` | 采样节拍/上报判据/光判据宏、UART1 引脚、平台常量、hall/OPTCFG 残留检查、产品源零差异 |
| `manifest_check_output.txt` | 共享清单检查：本报告主体时点 1404 条、0 缺失、0 坏格式、0 通配；BOT-01 修订后复核 1434 条（新增证据已入清单）、0 缺失、0 坏格式、0 通配（文末追加节） |
| `bot01_reassessment.txt` | BOT-01 复用复核原始记录：包/成员哈希复算、EV-038 镜像与复位窗口、产品源 `82a7e74..HEAD` 差异、板端 `k` 连续性计算、复用判定与局限 |
| `collect_static.py` / `parse_nodes.py` | 可复跑的静态收集与 UI 节点有界解析脚本 |

---

## 9. 证据局限

1. 板上镜像哈希不可回读；身份由 EV-038 下载记录、`k` 计数连续（+0.41%）与源码零差异闭合，属推断。
2. 上电五行未在本轮重新采集；经人工 REASSESS 核对，EV-038 证据与当前固件为同一镜像（§3.5），故复用之，并明确不是本轮新采集。板端哈希不可回读，复用属“哈希 + 零差异 + k 连续”推断；任一要素变化即失效。
3. 未调温、未调光、未遮光；无真实触发、无 `P`/发送失败样本；`V=0 D=0` 不是“有光已识别”的证据。
4. 未配置网关/传感器编码，手机端数据展示、告警、BLE 与上传均未执行；未访问平台写入接口或数据库。
5. 未测量电气/光路/功耗；ADC 原始值不用于判定光路好坏。
6. 真机 UI 覆盖当前候选；APK 字节与候选一致（设备侧复算）。
7. 本报告不把组件测试、构建成功、页面可打开或部分通过当作整体通过。

---

**最终判定：`TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）。** 当前候选 APK 上 D-01～D-04a 已逐项实测关闭，已执行范围内未发现新的软件实测缺陷；BOT-01 上电五行经人工 REASSESS 复核后与当前固件匹配并复用（非本轮新采集）。但需求 4 的无光分支仍禁用、真实升温/超温与光强双态、跨链路、平台写入回读与功耗均未执行或未满足，故整体产品验收不能通过。上述未执行项按人工决定与用户条件如实保留，不得解释为通过。
