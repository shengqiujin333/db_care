# 产品整体功能验收报告（PRODUCT-ACCEPTANCE-003）

- 负责角色：软件测试
- 能力：产品整体功能验收（`product_check`）
- 时点：2026-11-13 16:55–17:25（Asia/Shanghai；工具/设备时间戳为 UTC 08:55–09:25）
- 验收对象：当前完整候选
  - 传感器固件：FD-002 rev 8 发布包 `fw_release/fw_sensor_FD-002r8.zip`（FW-REL-001 rev 3，包字节自上一轮未变）
  - 安卓：AB-003 候选 APK（`80232c50…`，versionCode 10 / versionName 1.9，debug）
  - 既有网关（CH592 `beiwov2`，本轮未修改/未烧录）与既有平台服务（用户确认复用，本轮未连接、未写入）
  - 文档包：DOC-PKG-001 rev 14（`docs/README.md`、`docs/01`–`docs/06`、随包用户说明书）
- 验收仓库：`D:\mypro\beiwo2\AIP`，本轮开始时 HEAD `5d3d4bfbbed7da2a57620e2ca7c4be350694c077`，工作树仅本证据目录未跟踪
- **本轮结论：`TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）**
  - 已执行范围内**未发现可复现的软件/固件功能缺陷**。
  - `FW-LIGHT-DEF2-ENABLE` 为人工已确认的 **`APPROVED_DEFERRED` / NOT EXECUTED**：本轮不重测、不重复询问、**不作为返工或整体失败依据**，仅按人工范围决定披露。
  - 完整产品业务链路（真实升温/超温触发与 433 实发、网关→手机→平台同窗、`/upload_data` 写入/回读）与功耗仍未执行；真实光强双态为用户条件限制、跨链路为后续联调阶段、平台写入无授权、功耗已批准延期。上述均为 **NOT EXECUTED（外部条件/后续阶段）**，不是已测缺陷。
  - 按能力契约，未执行的真实链路不得被汇总为整体通过，故整体验收不能返回 `TEST_PASS`；本轮不把阶段内通过或人工延期项写成产品通过。

> 本报告只记录本轮实际执行、原始结果与可复核证据。未执行项、人工决定与观察项逐项披露；不把组件测试、构建成功、页面可打开或局部通过当作整体通过。

---

## 1. 候选身份（本轮实际复算/核对）

| 对象 | 身份 | 本轮证据 |
|---|---|---|
| 仓库 | HEAD `5d3d4bfbbed7da2a57620e2ca7c4be350694c077`；`git status --porcelain` 仅 `?? evidence/product_acceptance_20261113/` | `identity_checks.txt` |
| 传感器固件发布包 | `fw_release/fw_sensor_FD-002r8.zip`，134,791 B，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`；zip CRC 测试通过、3 成员完整 | `identity_checks.txt`（`md5sum`、`unzip -t` 输出） |
| 包成员 | axf `b6a15fed2ed5e3d0c4adc51ac87a4830`；hex `4d728dec9b0d28a352c46436b7586c72`；`PACKAGE.txt` `a58f67fcf5b760767ff5b65888c73dfb` | 同上（流式 `unzip -p \| md5sum` 复算） |
| 固件产品源 | `git diff --stat 82a7e74 HEAD -- <sensor>/{USER,MDK,EWARM,UM2005C,COMMON}` **为空** | `identity_checks.txt` |
| 板件 | 上电 `uid=6A002C00`、`rst=0040`，与 EV-040/EV-038 同板；板上镜像哈希不可回读 | `bot01_run.txt`、`bot01_boot.log` |
| 板上固件与交付包的关系 | EV-040（2026-11-13T07:09Z，同一发布包 md5）以 `mdk_flash {}` 下载并 `Verify OK`，下载镜像与独立重建/包内 axf、hex 逐字节一致；本轮该构建可启动、节拍与串口格式一致 | `evidence/test.md`（EV-040）+ 本轮 `bot01_run.txt` / `serial_capture_meta.txt` |
| 安卓候选 APK | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk`，10,370,964 B，SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`，MD5 `a97ed9bef658ee89cdf6e2b49447ce69` | `identity_checks.txt` |
| 安卓产品源码 | `git diff --stat a12d12b HEAD -- dengbei_care`：3 个测试侧独立验证用例 + 随包用户说明书；`app/src/main` 无产品源码差异 | `identity_checks.txt` |
| 设备在装包 | `com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，debug；设备 `base.apk` 10,370,964 B、SHA-256 `80232c50…`、MD5 `a97ed9be…`（**与候选逐字节一致**） | `device_package_state.txt`、`device_apk_hash.txt`、安装工具 `installed_version_verified` |
| 设备 | Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506`，本机在线 | `android_test devices`、`device_package_state.txt` |
| 文档包 | DOC-PKG-001 rev 14（7 份文档 + 用户说明书）；本轮执行到的手册步骤与真实候选一致 | `docs/README.md`、`docs/03`、`docs/04` 对照 + `ui_*` 原始树 |
| 平台服务 | 既有部署（用户确认复用）；候选 `CloudConfig.kt` 为 `8.140.23.253` / `ssl://…:8883` / `http://…:5000`；本轮未连接、未写入、未连库 | `logcat_session.txt`（设备侧 `ENETUNREACH`）、源码静态核对 |

---

## 2. 人工范围决定复核与原检查者范围错误（沿用已纠正口径）

### 2.1 当前人工范围决定

| 项 | 内容 |
|---|---|
| 测项 | `FW-LIGHT-DEF2-ENABLE`（无光判据启用与实板双态） |
| 人工结论 | **`APPROVED_DEFERRED` / NOT EXECUTED**（原回复「无法准备，先跳过」，2026-11-11T03:50:42Z；`deferral_allowed=true`；`HR-8A11935AD0A7`） |
| 原因 | 缺少完全遮光窗口与同板同构建的光强双态证据；无判据形制/数值接受记录；板上保持 `T=0` |
| 本轮处置 | 不重测、不重复询问同一 `test_item_id`、**不作为返工依据与整体失败依据**；在证据与交付中显式列为 NOT EXECUTED |

### 2.2 原检查者的范围错误（历史纠正，本轮继续采用）

| # | 原报告位置 | 范围错误 | 本轮口径 |
|---|---|---|---|
| E-1 | `PRODUCT-ACCEPTANCE-002` §5 R4 | 把人工已确认延期项与「未满足」混同 | R4 拆分为：判定公式实现（已实现且一致）；无光判据启用 = NOT EXECUTED（人工延期）；真实升温/超温触发 = NOT EXECUTED（温度条件） |
| E-2 | 同报告最终判定 | 以「无光分支仍禁用」作为整体 `TEST_FAIL` 依据之一 | 本轮整体失败依据只保留**其余未执行的真实链路/外部条件**；人工延期项不计入 |
| E-3 | 同报告 §6 N-1 vs §5 R4 | 同一报告内披露与标签自相矛盾 | 以人工决定与 NOT EXECUTED 披露为准，修正 R4 标签与失败依据 |

本轮不重复询问该项，也不因该项判定产品缺陷。该纠正已在 `PRODUCT-ACCEPTANCE-RECHECK-002` §2 与 `SOFTWARE-TRIAGE-003` 落地，本轮继续采用。

---

## 3. 执行方式、授权与环境边界

| 项 | 本轮做法 |
|---|---|
| 串口观测 | MCP `serial_list_ports` + `serial_capture`（COM43 / 9600 8N1）；未发送任何字节 |
| 上电五行（BOT-01 重评） | 测试侧自编脚本 `capture_bot01.py`：先开 COM43，再以 `pyocd reset -t cortex_m -M attach -m hw` 经 CW-DAPLink 请求外部 HW reset；仅复位，**未烧录、未写内存、未改选项字节**。依据：`readme.txt` 补充条件 2 明确授权尝试 pyocd；当前原生工具 `Bash` 可用 |
| 手机操作 | MCP `android_test`（devices/install/launch/ui_dump/ui_click/ui_press/ui_swipe）；ADB 只读 `dumpsys package`、`pm path`、设备侧 `stat/md5sum/sha256sum`、`run-as ls`、`logcat -d` |
| USB 枚举 | MCP `usb_list_devices`（只证明连接/身份） |
| 静态核对 | 限定目录 Read/Grep（固件 `USER/`、Android `app/src/main/`、`docs/`） |
| 未使用 | `mdk_flash`/`flash_device`、sigrok 逻辑采集、平台 SSH/写接口、`/upload_data`、数据库直连、调温/调光/遮光刺激、功耗测量、网关烧录/配置 |
| 原因 | 当前能力有效工具为 `android_test`、`usb_list_devices`、`serial_list_ports`、`serial_capture`；温度/光强/功耗受用户明确条件限制；跨链路按用户安排属后续联调阶段；平台只读 SSH 入口对本能力不可用；无平台写/数据库授权 |

---

## 4. 本轮真实执行记录

### 4.1 传感器 COM43 被动观测（真实目标）

| 窗口 | 结果 | 证据 |
|---|---|---|
| 2026-11-13T08:59:19.583Z–09:04:19.793Z（300 s） | **80 B，1 行**；md5 `823648c5760ec47da5d6527a7c8440bd`；SHA-256 `a2b2ef5cfbc33af463f1c841e86dad4fffdced5308621b623d4c1308c66ba703` | `serial_capture_com43_passive_01.bin` |
| 2026-11-13T09:04:58.388Z–09:09:58.611Z（300 s） | **160 B，2 行**；md5 `ba58aa019f6aa5f74210ffcbcd5fd97e`；SHA-256 `e62616c14f48c3d9a4e786ae6d61443bf6853484859ea0260311684474c0758a` | `serial_capture_com43_passive_02.bin` |

原始行（逐字节保留；CRLF 结尾）：

```text
S k=102 p=309 H=1 V=0 o=8 n=3869 x=3881 a=3876 D=0 t=299 h=334 q=1 r=0 s=0 y=0
S k=105 p=299 H=1 V=0 o=8 n=3884 x=3896 a=3888 D=0 t=296 h=338 q=1 r=0 s=0 y=0
S k=108 p=296 H=1 V=0 o=8 n=3903 x=3916 a=3909 D=0 t=293 h=344 q=1 r=0 s=0 y=0
```

判读（只记录事实）：

1. **采样节拍**：`k` 102→105→108，步长 **3**；与 `SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3` 一致。
2. **前值链**：`p(k=105)=299 = t(k=102)`，`p(k=108)=296 = t(k=105)`，`H=1`，前值语义自洽。
3. **采样结果**：全部 `q=1`、`o=8`；`t` 29.9/29.6/29.3 ℃、`h` 33.4/33.8/34.4 %RH 为合理实测量；`n/x/a` 为光敏 ADC 原始事实（本周期确有 ADC 采样）。
4. **判据默认态**：`V=0 D=0`（交付默认 `LIGHT_DARK_THRESHOLD_CODE=0`；**不得**解释为「已识别有光」）；`r=0 s=0 y=0`，无 `G`/`P`，本窗口无上报。温度逐周期下降（−10/−3/−3 个 0.1 ℃），不满足升温 >0.9 ℃ 分支。
5. **行长**：每条 80 B（含 CRLF），≤96 B 预算；采样间期无其它字节。
6. **与 EV-040 同一启动会话的连续性**：EV-040 第二次 HW 复位发生在 `2026-11-13T07:20:52.905Z`。按 `k` 每分钟 1 拍推算，`k=102` 应约在 `09:02:52.905Z`、`k=105` 在 `09:05:52.905Z`、`k=108` 在 `09:08:52.905Z`，三个时点分别落入本轮两个采集窗口；**与直接观测精确吻合**。本轮 09:16:24.853Z 的 BOT-01 复位后取得 `S k=0`，该会话随后结束。该连续性支持「本轮被动窗口运行的仍是 EV-040 下载的同一板端构建」，但不以 `k` 反推 LSI 精度。

### 4.2 BOT-01 上电诊断（本轮新采集，自动方式）

| 步骤 | 实际结果 | 证据 |
|---|---|---|
| 探针 | `pyocd 0.36.0`；`P&S CW-DAPLINK(CMSIS-DAP)` UID `87094109484987710672FF50` 在位；CW32 专用 target 未识别（`Target=n/a`） | 本轮 `pyocd list` 输出 |
| 编排 | COM43 先开（09:16:23.152Z）→ `pyocd reset -t cortex_m -M attach -m hw`（09:16:24.853–09:16:25.960Z，rc=0）→ 有界采集至 09:16:56.068Z | `bot01_run.txt` |
| 原始结果 | **271 B**；md5 `68ecd96568e69bf4c314e17d16976aa6`；SHA-256 `090b9d400bc8f19cbe46a99732a7acd006982abcc6f33313e1da8ee9f30a2fa1` | `bot01_boot.cap`、`bot01_run.txt` |
| 五行内容 | `BOOT` / `VDD` / `IOTEST` / `IOSIG` / `BUS` 各一次且顺序正确；首周期 `S k=0` | 见下 |
| 判定 | `BOOT_LINES_OK=PASS`、`BOT01_RESULT=PASS`、脚本 EXIT=0 | `bot01_run.txt` |

```text
BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600
VDD ok=1 code=1481 bgrmv=1189 mv=3287
IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=0
IOSIG scl=95555 sda=30300
BUS idle=3 scl=1 sda=1 ack=44
S k=0 p=0 H=0 V=0 o=8 n=3938 x=3948 a=3943 D=0 t=291 h=346 q=1 r=0 s=0 y=0
```

- `4095 × 1189 ÷ 1481 = 3287`（整数除法）与 `VDD mv=3287` 一致；`ok=1`。
- `BUS ack=44` 与 GXHT40 地址一致；随后 `q=1`，与 GXHT40 读数路径自洽。
- `BOOT fw=FD-002r4` 是字节冻结横幅，**不以横幅判版本**；版本由发布包/成员摘要与产品源零差异闭合（§1）。
- 自动方式成立：测试侧脚本 + pyocd 通用 `cortex_m` HW reset 可在 COM43 采集期间稳定取得上电五行，**不需要固件新增测试接口**；`capture_bot01.py` 仅位于证据目录，未进入发布包。

### 4.3 安卓真实设备执行（当前候选 APK）

| 步骤 | 实际结果 | 证据 |
|---|---|---|
| 安装 | `android_test install`：`installed_version_verified`，包名/versionCode 10/versionName 1.9/SHA-256 `80232c50…`；`confirmation_clicks=0` | `device_package_state.txt`、`device_apk_hash.txt` |
| 覆盖升级语义 | `firstInstallTime=2026-11-10 19:25:25` 保留；`lastUpdateTime=2026-11-13 17:10:01` 更新；`MyPrefs.xml` 保留 | `device_package_state.txt`、`logcat_session.txt` |
| 启动与首页 | 首页「爱守护」「启动」「还没有添加设备」「去添加」；三页签渲染正常；前台服务通知存在 | `ui_00_home.xml` |
| 我的页（顶部） | `用户：未登录`、「账户」、「查看今日报告」`[32,327][688,423]`、「打开传感器列表」`[32,431][688,527]` | `ui_01_my_page.xml` |
| 我的页（上滑） | `注册传感器` `[32,976][688,1072]`、`网关与上传设置` `[32,1120][688,1216]`，两控件**不相交**且均在底部导航 `[0,1264]` 之上 | `ui_02_my_scrolled.xml` |
| 网关与上传设置 | 标题、地址输入、搜索/扩大搜索/清除/保存、`云端上传状态：无（待发 0 条）`、重试上传、关闭 | `ui_03_gateway_dialog.xml` |
| 查看今日报告 | 进入 `ReportActivity`：`早起报告` / `尚未生成过报告` / `暂无报告…` / `重新生成` | `ui_04_report.xml`、`activity_report.txt` |
| 重新生成（零设备） | `ReportScheduler: Enqueued one-shot daily report`（17:13:56.900）→ `DailyReportWorker: No saved devices, skipping report`（17:13:57.019）→ Worker `SUCCESS`；App 发出 Toast（17:13:57.139）；页面保持未生成 | `logcat_report_regen.txt` |
| 打开传感器列表 | `已保存的 ID（0）`、`点选可多选，删除选中可批量移除`、`删除选中` | `ui_05_sensor_list.xml`、`activity_sensorlist.txt` |
| 设定页默认值 | `温湿度下降报警(且)` 2 / 0.1；`阈值报警(或)` 35 / 29 / 75；与文档默认值一致 | `ui_06_settings.xml` |
| 守护启动 | 点「启动」后按钮变「停止」；首页链路状态「未配置网关地址（搜索附近网关）」 | `ui_07_guard_started.xml` |
| 守护停止 | 再点后按钮恢复「启动」 | `ui_08_guard_stopped.xml` |
| 崩溃/ANR | 本应用 `FATAL EXCEPTION` 0 条、`ANR in com.jinyuni.dengbei_care` 0 条；crash buffer 为空 | `logcat_session.txt` |

未在本轮重跑、但**同一 APK 字节**已有独立真机证据的项：N-1 扫描权限自动续跑/拒绝/蓝牙关/定位关、N-2 真实网关读取至 `GATEWAY_NO_DEVICE` 终态（MTU 240、devCount=0、周期 182.7 s）、应用数据保留。依据：`evidence/android_package_check_20261113/`（ST-APK-007，被测 SHA-256 `80232c50…`，与本轮设备在装包逐字节一致）。本轮无需重采，因为字节身份已闭合。

### 4.4 文档包 rev 14（总测对象的一部分）

- 版本头：`docs/README.md` 标题为 DOC-PKG-001 rev 14；`docs/01`–`docs/06` 头部一致；随包用户说明书为 1.9。
- 本轮按手册实际执行的关键步骤与真实候选一致：
  - `docs/02` 的包身份/成员摘要：与 §1 复算一致；下载/`Verify OK` 步骤由 EV-040 在同一包上执行（本能力无烧录授权）。
  - `docs/03` 的 COM43/9600 8N1、上电五行顺序与字段、`S` 行格式/字段/行长：与本轮原始捕获逐项一致（§4.1/§4.2）。
  - `docs/04` 的 APK 身份、覆盖升级语义、我的/网关/报告/传感器列表/设定页文字与入口：与本轮真机 UI 树一致（§4.3）。
  - `docs/05`/`docs/06` 的平台地址/端口与已知限制：静态核对与候选一致；未连接、未写入。
- 独立校核 `evidence/doc_verification_20261113_rev14/DOC-VERIFY-014.md` 已对 rev 14 给出限定 `TEST_PASS`；本报告的总测确认其关键读者步骤与真实候选/实板一致。**文档校核不替代总测**。
- **观察 O-1**：`docs/README.md` §5 #11 仍写「AB-003 仅完成 AA-006 限定范围验证，整产品验收未在 AB-003 上重跑」。该表述在文档发布时属实；本轮已重跑，下一次文档修订应更新为 PRODUCT-ACCEPTANCE-003 的实际结论（交技术文档，不影响当前候选功能判定）。

### 4.5 平台配置静态核对

候选 `cloud/CloudConfig.kt:18/21/24/27/30`：`SERVER_HOST=8.140.23.253`、`MQTT_BROKER_PORT=8883`（`ssl://`）、`HTTP_PORT=5000`（明文 HTTP），与 `artifacts/backend_feasibility_current.md`（BE-EXP-004）记录的既有部署一致。边界：本轮**未连接服务器、未调用任何接口、未读取服务端文件、未直连数据库**；设备侧无外网（`ENETUNREACH`），该核对只证明「最终候选配置指向既有服务对象」，**不证明** `/upload_data` 可用。

### 4.6 正式产品产物的测试入口排除检查（按当前候选形态）

| 检查 | 结果 |
|---|---|
| APK 内容 | 1,135 个 zip 条目；未命中 `androidTest`、`/test/`、`instrument`、`verification`、`mock` 等测试专用条目；无 instrumentation | `apk_contents.txt` |
| 产品源码差异 | `a12d12b..HEAD` 中 `app/src/main` 无产品源码差异；仅 3 个测试侧独立验证用例与随包说明书 | `identity_checks.txt` |
| 候选形态 | 当前候选为 **debug 变体**（`DEBUGGABLE`），`release` 变体因离线缓存缺 lint 依赖未产出；无生产签名 | `device_package_state.txt`、`docs/04` §1、`artifacts/android_build.md` |

结论：当前 **debug 候选**内未发现混入测试专用组件；但**没有可用的 release/正式签名产物**，因此「正式发布包的测试入口排除」无法在本轮以正式产物完成。该项作为**交付形态限制**记录，不冒充正式包已核验，也不据此判定功能缺陷。

---

## 5. 必需功能与验收条件逐项结果

| # | 需求/验收项 | 结果 | 依据与边界 |
|---|---|---|---|
| R1 | 温度传感器改为 GXHT40 | **通过（读数路径，有限）** | 本轮 `q=1`、`t/h` 合理；上电 `IOTEST`/`BUS ack=44`；产品源对发布提交零差异。未做长期稳定性与精度标定 |
| R2 | 光敏电阻+分压电路与光照采样 | **部分（采样有效；判据未启用）** | 固件 ADC 采样有效（`o=8`，`n/x/a` 逐周期变化）；`V=0 D=0` 为 `T=0` 交付默认，**不得**判为「已识别有光」。真实有光/无光双态 = NOT EXECUTED（人工延期 N-1/N-3）。电路电气特性不在本能力范围 |
| R3 | 采集时间改为 3 分钟 | **通过（实板节拍）** | 本轮 `k` 102→105→108，步长 3；两根 COM43 窗口与 EV-040 复位后的 `k` 连续性精确吻合；源码常量一致 |
| R4 | 上传规则：ΔT>0.9 ℃ 且无光，或 >35 ℃ | **实现一致；真实触发未执行** | 源码为 `(cur>350) || (have_prev && (cur-prev)>9 && dark)`（严格大于），与需求逐项一致。交付默认 `T=0` ⇒ 无光分支不触发（明示安全默认，**不是遗漏**）；`FW-LIGHT-DEF2-ENABLE` = NOT EXECUTED（人工延期）；真实升温/超温触发与 433 实发 = NOT EXECUTED（N-2）。本轮 `r=0 s=0 y=0`、无 `P` |
| R5 | 平台 broker/服务信息 | **部分（静态一致，未连接）** | 候选常量指向 `8.140.23.253:8883/5000`，与既有服务一致；无平台写/读授权，设备无外网，未连接、未写入、未回读（N-5） |
| R6 | 同步修改安卓 App（iOS 不改） | **通过（当前 APK 已执行范围）** | 候选已安装且字节一致；首页/我的/设定/网关对话框/报告/传感器列表/守护启停可用；重新生成反馈与 Worker 终态正确；无 FATAL/ANR。真实 BLE 有效批次、上传、注册/登录、有数据列表、API 31+ 未执行（N-7）；iOS 未修改、未验证（用户明确不改） |
| R7 | 当前有光 | **条件确认（未判定光路）** | 用户声明当前有光；本轮未改变光强、未遮光；`a=3876–3909` 仅作事实记录，不据此判「有光」或光路好坏 |
| R8 | UART1 调试串口初始化与打印 | **通过（本轮新采集）** | 本轮 09:16:24Z 复位后取得 `BOOT/VDD/IOTEST/IOSIG/BUS` 与 `S k=0`；周期行 ≤80 B、CRLF 完整；`debug_trace.h` 明确 PA06 TXD/PA05 RXD、9600 8N1、行预算与关闭语义 |
| R9 | 取消光通讯与霍尔识别 | **通过（静态）** | 固件仅在退役注释中出现 hall/OPTCFG；`interrupts_cw32l010.c:186` 注明 PB04 已改 `LIGHT_ADC`、不再有霍尔 EXTI；Android 主源码未命中 hall/霍尔/光通讯/OPTCFG；产品源零差异 |
| E2E | 传感器→433→网关→BLE→手机→平台完整业务链路 | **未执行** | 跨链路按用户安排属后续联调阶段；网关未配置、手机未绑定、平台无写入授权；本轮无同窗记录与平台回读（N-4/N-5） |

---

## 6. 未执行项、延期与人工决定（必须随交付披露）

| ID | 项 | 状态 | 原因/后续责任 |
|---|---|---|---|
| N-1 | `FW-LIGHT-DEF2-ENABLE`（无光判据启用与实板双态） | **NOT EXECUTED（人工 `APPROVED_DEFERRED`，`deferral_allowed=true`；原回复「无法准备，先跳过」，2026-11-11T03:50:42Z）** | 缺完全遮光窗口与同板同构建的光强双态证据；无判据形制/数值接受记录；`T` 保持 0。本报告不重测、不重复询问、**不作为返工依据**。后续：上游接受判据形制/数值并由固件实现回填、重建后复验 |
| N-2 | 真实 ΔT>0.9 ℃、>35.0 ℃ 触发与 433 实发 | **未执行** | 用户明确当前不能调节温度；本轮无 `P`/`r=1`/`s=1`。属用户条件限制，不据此判定缺陷 |
| N-3 | 光强变化/遮光刺激 | **未执行（不安排）** | 用户明确当前不能改变光强；与 N-1 的人工延期一致 |
| N-4 | 433→网关→BLE→手机跨链路与同笔一致性 | **未执行** | 用户安排后续联调阶段；网关身份、供电与配对未验证；手机未配置网关地址 |
| N-5 | `/upload_data` 真实写入、幂等与回读 | **未执行** | 当前能力无写接口/数据库授权；设备侧无外网；平台只读 SSH 入口对本能力不可用 |
| N-6 | `E2E-04` CR2032 峰值/深睡电流 | **未执行（已批准延期记录在案）** | 无电流表/SMU/示波器；用户明确不能测功耗 |
| N-7 | Android 真实 BLE 有效批次 `DATA_OK`、全占位 `GATEWAY_NO_DATA`、多设备/MTU 边界、注册/登录、有数据列表/删除、API 31+ | **未执行** | 无真实有效批次/多设备数据源、无测试账号、无 API 31+ 真机；网关 `devCount=0`；属后续联调/外部数据条件 |
| N-8 | `release` 变体与生产签名 | **未产出** | 离线缓存缺 lint vital 依赖；本候选为 debug，仅用于内部测试；未改构建配置、未扩权 |
| N-9 | 本轮由本能力烧录固件 | **未执行** | 当前能力无 `mdk_flash`/`flash_device`；EV-040 已于 2026-11-13T07:09Z 用同一发布包下载并 `Verify OK`，本轮以发布包/源身份 + 实板启动/节拍/串口证据闭合当前构建 |
| N-10 | iOS | **不在本轮范围** | 用户明确本轮不修改 iOS |

**人工决定汇总（明示）**：

1. `FW-LIGHT-DEF2-ENABLE`：经人工确认**未执行**（`APPROVED_DEFERRED` / `SKIP_CONFIRMED`，「无法准备，先跳过」，2026-11-11T03:50:42Z）；保持 `T=0`，本报告不写成通过，也不作为返工依据。
2. `E2E-04`：已批准 `DEFERABLE`，本轮无仪器，未测并在此列明。
3. 真实升温/调光/遮光：用户明确当前不能改变温度/光强，不安排相关硬件测试。
4. 网关→手机跨链路：用户明确安排在后续联调阶段，本轮不据此反复退回固件或 Android 实现。
5. BOT-01：已按人工要求以自动方式重新执行并 PASS（本轮新采集上电五行）。

---

## 7. 观察项（不构成本轮整体失败依据）

| ID | 观察 | 影响/责任 |
|---|---|---|
| O-1 | `docs/README.md` §5 #11 仍写「整产品验收未在 AB-003 上重跑」 | 文档时效性；本轮已重跑，下次文档修订更新，不影响当前候选功能 |
| O-2 | 守护按钮显示「启动」时，前台服务通知「爱守护通知：正在运行」仍存在；BLE/MQTT 循环不随按钮停止 | 既有已披露设计（`docs/06` #21–#25 / 用户说明书）；非本轮需求缺陷 |
| O-3 | 设备无外网时 `MqttService` 记录 `ENETUNREACH`（`8.140.23.253:8883`） | 设备网络条件；非应用崩溃或功能缺陷 |
| O-4 | 当前仅有 debug 候选，无 release/生产签名包 | 交付形态限制；不冒充正式发布包已核验 |

---

## 8. 证据清单（本目录）

| 文件 | 内容 |
|---|---|
| `PRODUCT-ACCEPTANCE-003.md` | 本报告 |
| `identity_checks.txt` | HEAD/status、固件包与成员 md5、CRC、产品源零差异、APK 大小/SHA-256/MD5、Android 源码差异 |
| `device_package_state.txt` / `device_apk_hash.txt` | 设备版本、安装时间、在装 `base.apk` 大小/摘要（与候选逐字节一致） |
| `serial_capture_com43_passive_01.bin` / `_02.bin` / `serial_capture_meta.txt` / `capture_windows.txt` | COM43 原始捕获 80 B + 160 B、md5/SHA-256、窗口参数 |
| `capture_bot01.py` / `bot01_run.txt` / `bot01_boot.cap` / `bot01_boot.log` | BOT-01 测试侧编排脚本、原始运行日志、271 B 原始捕获、行文本 |
| `ui_00_home.xml` … `ui_08_guard_stopped.xml` | 首页/我的/上滑/网关对话框/报告/传感器列表/设定/守护启停 UI 原始树 |
| `activity_report.txt` / `activity_sensorlist.txt` | 报告页与传感器列表 Activity 栈 |
| `logcat_report_regen.txt` / `logcat_session.txt` | 报告再生成 Worker/Toast、本次会话日志、crash buffer、`MyPrefs` 存在性 |
| `apk_contents.txt` / `third_party_packages.txt` | APK 条目清单（测试入口排除检查）与设备三方包列表 |
| `parse_ui.py` | 可复跑的 UI 树文本提取脚本（测试侧） |

`*.bin` / `*.cap` 为原始字节；按仓库 `.gitignore` 属本机留存，仓库内以文本/哈希与脚本复现复核。

---

## 9. 证据局限

1. 板端镜像哈希不可回读；身份由发布包/成员摘要、产品源零差异、EV-040 的同包下载 `Verify OK` 与本轮新的上电五行（`uid=6A002C00`）闭合。本轮未自行烧录。
2. BOT-01 使用 pyocd 通用 `cortex_m` target 请求外部 HW reset；CW32 专用 target 未被 pyocd 识别。该方式只证明复位与串口观测可行，不证明 pyocd 的 CW32 flash 算法可用。
3. 未调温、未调光、未遮光；无真实触发、无 `P`/发送失败样本；`V=0 D=0` 不是「有光已识别」的证据。
4. 未配置网关/传感器编码，手机端有效批次、真实 BLE/GATT/MTU、告警、上传均未执行；同字节 APK 的既有证据只覆盖其声明范围。
5. 未访问平台写入接口或数据库；`CloudConfig` 静态一致不等于 `/upload_data` 可用。
6. 未测量电气、光路与功耗；ADC 原始值不用于判定光路好坏。
7. 真机 UI 覆盖当前候选；APK 字节与候选一致（设备侧复算）。
8. 本报告不把组件测试、构建成功、页面可打开或部分通过当作整体通过。

---

## 10. 最终判定

**`TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）。**

- **已执行范围内未发现可复现的软件/固件功能缺陷**：固件发布包身份与源一致；实板采样节拍、前值链、默认判据行为、UART1 上电五行与周期行均符合当前设计；Android 当前候选安装字节一致，关键用户路径可用，无 FATAL/ANR；文档 rev 14 关键步骤与真实候选一致。
- **人工已确认跳过的 `FW-LIGHT-DEF2-ENABLE`**：明确列为 NOT EXECUTED（`APPROVED_DEFERRED`），不重测、不重复询问、**不作为整体失败或返工依据**；原检查者把它写成「未满足/整体失败依据」的范围错误已按人工范围纠正。
- **整体不能通过的依据**只保留其余**未执行的真实必需链路/外部条件**：真实升温/超温触发与 433 实发（用户不能调温）、真实光强双态（用户不能改光强且该项已人工延期）、网关→手机→平台跨链路（后续联调阶段）、`/upload_data` 写入/幂等/回读（无授权、设备无外网）、功耗（已批准延期）、Android 有效批次/账号/API 31+ 等外部数据项。上述均为 NOT EXECUTED，不是已测缺陷，也不得写成通过。
- 当前仅有 debug 候选、无 release/生产签名产物；不作为功能缺陷，但不得称为可发布的正式产品包。
