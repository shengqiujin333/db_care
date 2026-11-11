# 产品文档独立校核报告（DOC-VERIFY-001）

- 校核角色：软件测试（独立校核，不修改产品源码与文档正文）
- 校核时点：2026-11-11（Asia/Shanghai；命令时间为 UTC）
- 仓库 HEAD：`df4942c23c743e0ad29483177ea4e415f1485763`（文档提交）
- 校核对象：`docs/README.md` 及 `docs/01`–`docs/06`（DOC-PKG-001 rev 1）
- 覆盖候选：
  - 传感器固件 FD-002 rev 8 发布包 `fw_release/fw_sensor_FD-002r8.zip`（md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`）
  - 安卓 App `com.jinyuni.dengbei_care` versionCode 10 / versionName 1.9，debug 候选 APK SHA-256 `d20542219faeaf4be0d73ce4650c57e3ea25bc54183ff0117a3f61c0e41bd8b6`
  - 平台服务既有部署（BE-EXP-004 只读核验）
- 结论：**TEST_FAIL**。文档包存在可复现的文档缺陷；关键用户步骤的界面文字与实际 App 不一致，且部分限制描述与既有独立测试证据相反。未发现本次新测范围内的固件/App 功能回归（仅完成安装、启动与两处界面核对，未做完整业务验收）。

> 本报告是独立校核证据，不是产品总功能验收；未执行项不写成通过。

---

## 1. 本轮必需读者任务与覆盖方式

| # | 读者任务 | 覆盖方式 | 结果 |
|---|---|---|---|
| 1 | 按 02 文档校验发布包并确认烧录身份 | 静态核对 zip/成员 MD5、PACKAGE.txt、EV-036 原始下载与上电证据；本能力无 `mdk_flash`/`serial_capture` 工具，未重新烧录 | 身份一致；烧录步骤未由本能力复跑 |
| 2 | 按 02/03 文档接串口并判读上电五行与 `S`/`G`/`P` 行 | 静态核对源码字段与格式；交叉核对 EV-036 原始捕获（五行、6 个 `S` 行、I²C） | 字段/样本一致；未重新采集 |
| 3 | 按 04 文档安装/覆盖升级 App | **真机执行**：`android_test install` 安装当前候选，核对包名/版本/SHA-256；`dumpsys package` 核对时间戳 | 安装/升级保留数据通过 |
| 4 | 按 04 文档完成“我的”页绑定设备与网关 | **真机执行**：启动、切“我的”、打开“网关与上传设置”，读取真实 UI 树 | **发现文档与实际界面不一致（D-01/D-02/D-05）** |
| 5 | 按 04 文档处理上传阻塞与手动重试 | 静态核对 `CloudUploadRepository`/`GatewayBleSettingsDialog`；界面控件存在 | 行为描述与源码一致；未触发真实 401/403 |
| 6 | 按 05 文档核对平台入口与客户端配置 | 静态核对 `CloudConfig.kt`、`MqtttService.kt`、`network_security_config.xml`；交叉核对 BE-EXP-004 | **发现明文放行清单描述错误（D-04）** |
| 7 | 按 06 文档排障与披露未执行项 | 静态核对限制表、未执行清单、证据路径 | **发现报告按钮可点击性描述错误（D-03）** |

---

## 2. 缺陷清单（文档缺陷与产品缺口分开）

### D-01（高）docs/04 §4.1 的界面文字与当前 App 实际可见文字不一致

- 文档位置：`docs/04_安卓App安装与使用.md` §4.1（第 92–99 行）
- 文档写：进入「我的」→ 在「**传感器地址**」填入网关地址并点「**发送**」→ 点「**添加**」
- 真机实际（Redmi 8A / Android 10 / 当前候选 `d2054221…`，`ui_my_page.xml`）：
  - 字段标签：`请输入网关编码：`（`sensorAddressLabel`）
  - 按钮：`保存`（`macSendButton`）
  - 添加按钮：`注册传感器`（`addsensorid`）
  - 另有按钮：`打开传感器列表`（`opensensorlistbutton`）
- 影响：用户按文档找不到「传感器地址」「发送」「添加」三个文字；绑定/注册步骤无法照文档复现。
- 证据：`evidence/doc_verification_20261111/ui_my_page.xml`（第 13、16、22、23 行）；`evidence/android_package_check_20261110/test_report.md` §3 也使用“网关编码/注册传感器”描述。
- 修正建议：把步骤改为实际可见文字，并明确“先登录/注册账号”（未登录时点“保存”会提示“mac 设置失败，请先登录”）。
- 受影响同源文档：`dengbei_care/用户使用说明书.md` §4.1 有相同错误（「传感器地址」「发送」「添加」，且多写“进入「我的」->「传感器 ID」”）；该文件不在 docs/ 目录，需由文档/Android 责任方同步修正。

### D-02（高）docs/04 §4.1 “（写入网关的绑定列表）”与实现不符

- 文档位置：`docs/04_安卓App安装与使用.md` §4.1 第 95 行
- 文档写：点「发送」（写入网关的绑定列表）
- 实际实现：
  - `NotificationsFragment.macSendButton` 只接受长度 12 的输入，调用 `saveInput`；
  - `saveInput` 把值写入 `MyPrefs` 的 `mac_addr`，并在已登录时调用平台 `POST /setmac`；
  - 未登录时保存本地值但提示“mac 设置失败，请先登录”；
  - 仓库中 `BLE_ENQUEUE_WRITE_IDS` 只有接收端（`MqtttService.kt:1312`），没有发送端，未发现 App 在该步骤写网关绑定列表的路径。
- 影响：文档把“平台 MAC 登记 + 本机保存”描述成“写网关绑定列表”，会误导现场联调与排障。
- 证据：`NotificationsFragment.kt:84–91, 226–262`；`MqtttService.kt:1312`；`ui_my_page.xml`。
- 修正建议：改为“本机保存网关编码并提交平台 /setmac”；若产品意图确为写网关绑定列表，则属 Android 实现缺口，需由 Android 实现确认 `BLE_ENQUEUE_WRITE_IDS` 的调用路径，文档不得先行宣称已完成。

### D-03（高）docs/06 §3 #17 把“查看今日报告”描述为“功能可点”，与既有独立测试相反

- 文档位置：`docs/06_维护排障与已知限制.md` §3 第 17 项
- 文档写：`网关/报告按钮可能与输入区视觉重叠，功能可点`
- 既有独立测试证据（`evidence/android_package_check_20261110/test_report.md` §6）：
  - O-PKG-1：新增“网关与上传设置”按钮与“传感器别称/注册传感器”在底部约 56 px 重叠；两控件上部仍可见可点。
  - O-PKG-2：既有“查看今日报告”按钮 `[32,1280][688,1376]` 整体位于底部导航 `[0,1264][720,1424]` 之下；**实际点击被导航栏拦截（点击后页签变为“设定”）**，不是“功能可点”。
- 影响：支持/验收人员会误以为报告按钮可用；实际需通过早报通知进入 `ReportActivity`。
- 证据：`evidence/android_package_check_20261110/ui_05_my_tab.xml`、`ui_09_after_report_click.xml`、`test_report.md` §6。
- 修正建议：拆成 O-PKG-1 与 O-PKG-2 两条；明确“查看今日报告”在 720×1520 下不可点击（被底部导航拦截），报告内容走通知/`ReportActivity`；不要把两个不同问题合并为“功能可点”。

### D-04（高）docs/05 对 `network_security_config.xml` 明文放行清单的描述错误

- 文档位置：`docs/05_平台服务器配置与联调前提.md` §1 第 28 行、§2 表格第 39 行
- 文档写：
  - “旧主机退役：`117.72.84.210` 已不再使用，**App 内不得出现该地址（已静态核查）**”
  - `network_security_config.xml`：放行 `8.140.23.253` 明文 HTTP，**仅该主机**；不得放开全局明文
- 实际文件 `dengbei_care/app/src/main/res/xml/network_security_config.xml`：
  - 3 个 `domain-config`，均为 `cleartextTrafficPermitted="true"`；
  - 域名集合 = `{8.140.23.253, 117.72.84.210, 192.168.4.1}`；
  - 旧主机 `117.72.84.210` 确实仍以明文放行条目出现（设计为“追加而非替换”）。
- 影响：安全/配置审计会误判 App 只放行一个新主机；文档内部“不得出现”与文件实际内容自相矛盾。
- 证据：`network_security_config.xml:4–14`；`evidence/software_static_check_server_migration.py` D 段明确期望 3 个域名；`evidence/software_test.md` 兼容面检查。
- 修正建议：改为“旧主机不再作为连接目标；XML 中保留 1 处历史明文放行条目（设计追加）”；表格写明 3 个域名及各自用途（平台、旧主机历史条目、网关本地配网热点），并说明“不得新增其它主机/不得全局放开”。

### D-05（中）docs/04 §4.1 未说明“网关编码”输入格式，实际只接受 12 个字符

- 文档写：在「传感器地址」填入网关地址（未给格式/长度）
- 实际：`macsaveButton` 仅当 `input.length == 12` 才调用 `saveInput`，否则提示“请输入12个字符”；输入框提示“请输入网关上的字母串”。若用户按 §4.2 的 BLE 地址格式输入 `AA:BB:CC:DD:EE:FF`（17 字符）会失败。
- 影响：用户按文档操作可能直接卡在“请输入12个字符”。
- 证据：`NotificationsFragment.kt:84–91`；`strings.xml` 的 `sensor_address`/`sensor_address_hint`；`ui_my_page.xml`。
- 修正建议：明确“12 位网关编码（无分隔符，建议大写十六进制）”；与 §4.2 的 `AA:BB:CC:DD:EE:FF` BLE 地址区分。若产品应在输入时规范化/校验十六进制，则同时交 Android 实现确认（当前只校验长度）。

### D-06（中）docs/04 §4.2 的“必要时滚动或旋转屏幕”缺少依据

- 文档位置：`docs/04_安卓App安装与使用.md` §4.2 第 137 行
- 文档写：控件重叠时“必要时滚动或旋转屏幕”
- 既有证据：`evidence/android_package_check_20261110/ui_08_my_tab_scrolled.xml` 显示“我的”页滑动前后 bounds 不变，页面不是可滚动容器；O-PKG-1 的建议正是“将页面改为可滚动容器”。
- 影响：用户按建议滚动无效。
- 修正建议：删除“滚动”建议；只保留“旋转屏幕（若实测有效）”或直接说明当前布局固定、需后续修复。

### D-07（低）docs/03 §4 的 `IOTEST gc` 字段只解释了一个取值

- 文档写：`gc` = 上电一次性 general-call 复位尝试结果（0=未做，地址已应答）
- 实际 `measure.c` 取值：0=未尝试（器件已应答 0x44/0x45）、1=0x00 ACK 但 0x06 未 ACK、2=复位命令已接受、3=0x00 也不应答。
- 影响：现场看到 `gc=1/2/3` 时无法按手册判读。
- 证据：`USER/src/measure.c:161–233`。
- 修正建议：补全 0..3 的含义。

### D-08（低）docs/02 §3.3 的“设计判读表”引用不准确

- 文档写：设计判读表将 2700–3100 mV 视为新鲜电池范围
- 实际该范围出现在 `artifacts/firmware_implementation.md:594`，未在 `artifacts/firmware_design.md` 中检出。
- 修正建议：改引 `artifacts/firmware_implementation.md` 的具体表，或删除“设计判读表”的归属。

### D-09（低）docs/06 §3 #15 的章节引用不准确

- 文档写：共享固定后缀的 MQTT 口令，依据“05 文档 §3”
- 实际认证约定（用户名/密码后缀）在 `docs/05` §2；§3 是凭据与敏感信息处理。
- 修正建议：改为“05 文档 §2/§3”。

### D-10（低）docs/04 §7 的登录次数提示文字与阈值不精确

- 文档写：提示“登录次数过多”（累计 30 次），需重装应用
- 实际：`LOGIN_CNT > 30` 时提示“您登录/退出次数太多，需要重新安装软件”（第 31 次起拦截）。
- 修正建议：使用实际提示原文并写明“>30 次（第 31 次起）”。

### D-11（提示，非失败项）docs/README §5 #7 的安装字节状态可更新

- 文档写：实机记录来自前次构建 `6383c4a5…`，当前候选 `d2054221…`；要求“被安装字节=被测字节”时需重跑安装用例。
- 本轮已实际安装当前候选 `d2054221…` 并核对包名/版本/SHA-256、覆盖升级保留数据、启动与“我的/网关设置”界面；但未重跑扫描/GATT/上传等关键路径。
- 建议：文档更新为“当前候选已安装并核对身份；扫描/GATT/上传关键路径未重跑”。

---

## 3. 产品缺口（与文档缺陷分开）

| ID | 观察 | 证据 | 责任/处理 |
|---|---|---|---|
| P-01 | “查看今日报告”按钮位于底部导航之下，点击被拦截（O-PKG-2，基线既有布局问题） | `evidence/android_package_check_20261110/test_report.md` §6；`ui_09_after_report_click.xml` | Android 实现；文档先按实际限制描述，布局修复另立任务 |
| P-02 | “网关与上传设置”按钮与“传感器别称/注册传感器”在 720×1520 下约 56 px 重叠（O-PKG-1，本轮新增按钮引入） | 同上；`ui_05_my_tab.xml`；本轮 `ui_my_page.xml` bounds | Android 实现；文档按实际“上部仍可点”描述 |
| P-03 | `BLE_ENQUEUE_WRITE_IDS` 只有接收端，未发现 App 发送端；若产品意图为“保存时写网关绑定列表”，则功能未接线 | `MqtttService.kt:1312`；全仓检索无发送调用 | Android 实现确认；文档在确认前不得宣称已写入网关绑定列表 |

上述 P-01/P-02 已在既有独立测试中登记为观察项，不是本轮新发现的功能回归；P-03 是静态接线观察，需实现方确认设计意图。

---

## 4. 已执行的真实设备步骤与原始观察

### 4.1 设备与安装

- `android_test devices`：`12f046690506  device product:olivelite model:Redmi_8A`（在线）。
- 安装前 `dumpsys package`：`versionCode=10`、`versionName=1.9`、`firstInstallTime=2026-11-10 19:25:25`。
- `android_test install` 安装当前候选：
  - `success=true`，`installed_version_verified`；
  - `package=com.jinyuni.dengbei_care`、`version_code=10`、`version_name=1.9`；
  - `sha256=d20542219faeaf4be0d73ce4650c57e3ea25bc54183ff0117a3f61c0e41bd8b6`；
  - `confirmation_clicks=0`。
- 安装后 `dumpsys package`：`versionCode=10`、`versionName=1.9`、`firstInstallTime=2026-11-10 19:25:25` 保持、`lastUpdateTime=2026-11-11 13:57:18` 更新 → 覆盖升级、数据保留。
- 设备：Redmi 8A / Android 10 / API 29。
- 原始：`android_install.json`、`device_package_state.txt`。

### 4.2 启动与界面核对

- `android_test launch` 成功；首页渲染“爱守护”、空态“还没有添加设备 / 前往「我的」→「传感器列表」添加设备”、底部“首页/设定/我的”三页签。
- 点击“我的”后 UI 树（`ui_my_page.xml`）实际可见：
  - `用户：未登录`、`账户`；
  - `请输入网关编码：`、`请输入网关上的字母串`、`保存`；
  - `请输入传感器编码和别称：`、`请输入传感器的字母串`、`注册传感器`；
  - `网关与上传设置`、`查看今日报告`、`打开传感器列表`。
- 打开“网关与上传设置”后 UI 树（`ui_gateway_dialog.xml`）实际可见：
  - `未配置网关地址 / 平台身份（mac_addr）：`；
  - `网关 BLE 地址（AA:BB:CC:DD:EE:FF）`；
  - `搜索附近网关`、`扩大搜索（不过滤）`、`清除网关地址`、`保存网关地址`；
  - `云端上传状态：无（待发 0 条）`、`重试上传`、`关闭`。
- 这些文字与 docs/04 §4.2 的按钮名称一致；与 docs/04 §4.1 的“传感器地址/发送/添加”不一致（D-01）。

### 4.3 身份与包核对

- 发布包：`134791 B`，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`；zip CRC 测试无坏成员。
- 成员：
  - `fw_sensor_FD-002r8_b6a15fed.axf` 297380 B，md5 `b6a15fed2ed5e3d0c4adc51ac87a4830`；
  - `fw_sensor_FD-002r8_4d728dec.hex` 75275 B，md5 `4d728dec9b0d28a352c46436b7586c72`；
  - `PACKAGE.txt` 3202 B，md5 `a58f67fcf5b760767ff5b65888c73dfb`。
- APK：10,365,460 B；md5 `58de16b8fef549f004b92ffcec0104d9`；SHA-256 `d2054221…`。
- JDK：`openjdk version "21.0.2"`；Gradle wrapper 8.4；AGP 8.3.2（`libs.versions.toml`）。
- USB 枚举：COM43 = Prolific `VID_067B&PID_2303`；CW-DAPLink = `VID_C251&PID_F001` 序列号 `87094109484987710672FF50`；COM26 = 同 VID/PID 的 CDC；手机 = `12F046690506`。
- 原始：`static_identity.txt`、`usb_enumeration.txt`。

### 4.4 源码/证据交叉核对（静态）

- 固件常量：`SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3`、`SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350`、`LIGHT_DARK_THRESHOLD_CODE=0`；GXHT40 0x44/0x45、0xFD、6 B、双 CRC-8、20 ms；UART1 PA06/PA05、9600 8N1、96 B 行预算。
- `sensor_decide_report`：严格 `cur>350`；否则 `have_prev && dark && (cur-prev)>9`。
- `light.c`：`valid = adc_ok && T!=0`；`dark = light_is_dark(mean, valid, T)`；全超时 `mean=0`，不合成暗态。
- `measure.c`：`q=0` 不上报、不推进前值、不构造 0 值；`G` 行仅失败周期；`P` 行仅上报判定或发送尝试。
- EV-036 原始捕获：五行有序、`VDD code=1480 bgrmv=1189 mv=3289`、`IOSIG scl=95555 sda=30300`、`BUS ack=44`、`S k=0/3/6/9/15/18` 全 `q=1 V=0 D=0`、`a=2860..2886`、双字 CRC 匹配。
- EV-035 原始捕获：`VDD mv=3287`、6 周期全 `q=1 V=0 D=0`、`a=2831..2840`、双 CRC 匹配。
- 文档 03 §4 样本值与 EV-036 逐字一致；文档 02 §2 成员表与 zip 逐字一致。
- 平台：`CloudConfig.kt` = `8.140.23.253` / 8883 / 5000；`MqtttService.kt` = `clientId="android_client"`、`TLSv1.2`、`jd-ca.crt`、`/<mac>/a/r0` 订阅、`/<mac>/a/s0` 发布；`jd-ca.crt` md5 `f1a8212e3c690d57894b8a3a5fd901ab`；`SensorCadence.SAMPLE_PERIOD_SECONDS=180`；`GatewayReadGuard.MAX_DEVICES=9`。
- BE-EXP-004：EMQX 5.8.0、uptime 17 d 9 h、4 listener、`current_conn=0`；`user_script` active、`abr.py` 8949 B/mtime 2026-10-22、5000 监听；6 路由 405+Allow；OPTIONS 200；POST 14 条/末次 2026-10-22；MySQL Up 2 weeks；磁盘 68 G/内存 2010 MiB。
- 文档 05 §4 与 BE-EXP-004 逐项一致（除 D-04 的 XML 清单描述）。

---

## 5. 未执行 / 未复跑项及原因

| 项 | 状态 | 原因 |
|---|---|---|
| 固件重新烧录、上电五行与 COM43 重新采集 | **未复跑** | 本能力有效工具仅 `android_test`、`usb_list_devices`；无 `mdk_flash`/`serial_capture`。已交叉核对 EV-036 原始证据，未把未复跑写成通过 |
| 固件 I²C 位级重新采集 | 未复跑 | 无 `sigrok_capture` 工具 |
| 平台 `/upload_data` 真实写入/幂等/回读 | 未执行 | 无写授权与数据库授权；仅交叉核对 BE-EXP-004 |
| 网关→手机跨链路 | 未执行 | 用户明确安排后续联调阶段 |
| 真实升温 >0.9℃ / >35℃、调温、改变光强、遮光、功耗 | 不安排 | 用户明确当前不能调温/调光/测功耗 |
| Android 扫描/GATT/上传/401 恢复的当前候选重跑 | 未执行 | 本轮只完成安装、启动、“我的”页与网关设置对话框核对；未改设备网络/网关状态 |
| Android release 变体/生产签名 | 未产出 | 离线缺 lint 依赖（既有结论） |

---

## 6. 给技术文档能力的可操作修正

1. **docs/04 §4.1 重写**：使用真实可见文字“请输入网关编码：”“保存”“注册传感器”；明确 12 位网关编码格式；删除“写入网关的绑定列表”，改为“本机保存并提交平台 /setmac”；补“需先登录/注册账号”。
2. **docs/04 §4.2 注记**：删除“滚动”建议；只保留经实测有效的操作。
3. **docs/06 §3 #17 拆分**：O-PKG-1（网关按钮重叠、上部可点）与 O-PKG-2（查看今日报告被底部导航拦截、不可点，报告走通知）分开描述。
4. **docs/05 §1/§2 修正明文放行清单**：写明 3 个域名及用途；旧主机只作为历史明文条目保留、不是连接目标；引用静态核查脚本的精确规则。
5. **docs/03 §4 补全 `gc` 0..3 含义**。
6. **docs/02 §3.3 修正“设计判读表”引用**。
7. **docs/06 §3 #15 修正章节引用**。
8. **docs/04 §7 使用实际登录限制提示原文与阈值**。
9. **同步修正 `dengbei_care/用户使用说明书.md` §4.1**（相同界面文字错误；该文件由 Android/文档责任方维护）。
10. 修正后提升文档包版本号，并重新按 §1 的读者任务核对关键步骤；不要复用本报告作为新版本通过证据。

---

## 7. 原始证据索引

| 文件 | 内容 |
|---|---|
| `evidence/doc_verification_20261111/static_identity.txt` | HEAD、发布包 md5/成员、APK 大小/MD5/SHA-256、JDK 版本 |
| `evidence/doc_verification_20261111/device_package_state.txt` | 安装后包版本、首次安装/更新时间、设备型号/API |
| `evidence/doc_verification_20261111/android_install.json` | `android_test install` 原始返回（包名/版本/SHA-256/确认点击次数） |
| `evidence/doc_verification_20261111/ui_my_page.xml` | “我的”页真实 UI 树（网关编码/保存/注册传感器/网关与上传设置/查看今日报告） |
| `evidence/doc_verification_20261111/ui_gateway_dialog.xml` | “网关与上传设置”对话框真实 UI 树 |
| `evidence/doc_verification_20261111/ui_home.xml` | 首页真实 UI 树（三页签、空态） |
| `evidence/doc_verification_20261111/usb_enumeration.txt` | COM43/CW-DAPLink/COM26/手机/fx2lafw 枚举 |
| `evidence/ev036/capture_analysis.txt` | EV-036 上电五行/6 周期/I²C 原始解析 |
| `evidence/ev035/capture_analysis.txt` | EV-035 上电五行/6 周期/I²C 原始解析 |
| `evidence/android_package_check_20261110/test_report.md` | R3 安装/启动/UI/观察项 O-PKG-1/O-PKG-2 |
| `evidence/software_static_check_server_migration.py` | 明文放行 3 域名与旧主机 1 处保留的独立静态核查 |
| `artifacts/backend_feasibility_current.md` | BE-EXP-004 平台只读核验 |
| `fw_release/fw_sensor_FD-002r8.zip`、`fw_release/PACKAGE.txt` | 固件发布包与自述 |

---

## 8. 判定

- **TEST_FAIL**：D-01/D-02/D-03/D-04 为高影响文档缺陷，均可由当前源码、真实 UI 树或既有独立证据复现；D-05/D-06 为中等影响步骤缺陷；D-07–D-10 为低影响完整性问题。
- 产品缺口 P-01/P-02 为既有布局观察项，P-03 为静态接线观察，均需 Android 实现确认；本轮未发现新的固件功能回归。
- 未执行项已在本报告 §5 明示，不构成通过。
