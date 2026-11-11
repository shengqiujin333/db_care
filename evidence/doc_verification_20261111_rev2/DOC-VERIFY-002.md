# 产品文档独立校核报告（DOC-VERIFY-002）

- 校核角色：软件测试（独立校核；未修改产品源码与文档正文）
- 校核时点：2026-11-11（Asia/Shanghai；命令/采集时间为 UTC）
- 仓库 HEAD：`1b1b5fa80a73b7510f13a1f1d3579922d0b521db`（文档 rev 2 提交，工作树干净）
- 校核对象：`docs/README.md` 及 `docs/01`–`docs/06`（DOC-PKG-001 **rev 2**）
- 覆盖候选：
  - 传感器固件 FD-002 rev 8 发布包 `fw_release/fw_sensor_FD-002r8.zip`（md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`）
  - 安卓 App `com.jinyuni.dengbei_care` versionCode 10 / versionName 1.9，debug 候选 APK SHA-256 `d20542219faeaf4be0d73ce4650c57e3ea25bc54183ff0117a3f61c0e41bd8b6`
  - 平台服务既有部署（BE-EXP-004 只读核验）
- 上一轮：`evidence/doc_verification_20261111/DOC-VERIFY-001.md`（rev 1，TEST_FAIL，D-01..D-11）
- **结论：TEST_FAIL**。rev 2 已修正上一轮全部 11 项文档缺陷（逐项复核见 §2），但本轮新发现 3 项可复现的文档缺陷（D-12/D-13/D-14）和 1 项提示级缺口（D-15），其中 D-12 使“查看/删除已绑定传感器”这一必需运维步骤无法按手册执行，并对应一个实测产品缺陷 P-04。

> 本报告是独立校核证据，不是产品总功能验收。未执行项在 §5 明示，不写成通过；本轮判定由已实测缺陷驱动，不依赖未执行项。

---

## 1. 本轮必需读者任务与覆盖方式

| # | 读者任务 | 覆盖方式 | 结果 |
|---|---|---|---|
| 1 | 按 02 校验发布包并确认烧录身份 | 静态复算 zip/成员 MD5、CRC、Keil 产物摘要；交叉核对 EV-036 原始下载/上电证据；本能力无 `mdk_flash`，未重新烧录 | 身份一致；烧录步骤未由本能力复跑（§5） |
| 2 | 按 02/03 接串口并判读上电五行与 `S`/`G`/`P` 行 | 静态核对 `debug_trace.h`/`measure.c`/`sensor_config.h`；逐字对照 `evidence/ev036/capture_analysis.txt` | 一致；未重新采集（§5） |
| 3 | 按 04 安装/覆盖升级 App | 核对当前设备已安装包与候选 SHA-256/版本/时间戳；上一轮已完成真实安装与覆盖升级保留数据 | 一致（本轮未重装） |
| 4 | 按 04 §4.1 登记网关编码、注册传感器、查看/删除设备 | **真机执行**：启动、切「我的」、读取真实 UI 树；按文档文字核对控件；点击「打开传感器列表」复现 | **D-12：列表入口被底部导航拦截，步骤不可执行** |
| 5 | 按 04 §4.2 配置网关 BLE 地址与上传状态 | **真机执行**：打开对话框并读取真实 UI 树；静态核对 `GatewayBleSettingsDialog.kt` | 文字/行为一致 |
| 6 | 按 04 §5 理解上传失败分类与手动重试 | 静态核对 `UploadResult.kt`、`CloudUploadRepository.kt`、对话框状态文案 | **D-13：408/429 被文档写成全局暂停，实际为退避重试** |
| 7 | 按 05 核对平台入口与客户端配置 | 静态核对 `CloudConfig.kt`、`MqtttService.kt`、`network_security_config.xml`、`jd-ca.crt`、`UploadPayload.kt`；交叉核对 BE-EXP-004 | 一致（上一轮 D-04 已修正） |
| 8 | 按 06 排障与披露未执行项 | 静态核对限制表、未执行清单、证据路径；真机复现「查看今日报告」被拦截 | 报告按钮描述正确；**D-12 的同类限制未登记** |
| 9 | 按 01 理解系统组成与数据链路 | 静态核对源码、网表/BOM、发布/构建证据 | **D-14：网关“并转 MQTT”与实现不符**；其余一致 |

---

## 2. 上一轮 D-01..D-11 的逐项复核（rev 2）

| 上轮 ID | rev 2 修正 | 独立复核证据 | 结果 |
|---|---|---|---|
| D-01 高：§4.1 界面文字错误 | 改为「请输入网关编码：」「保存」「注册传感器」「打开传感器列表」 | 真机 UI 树 `ui_my_page_rev2.xml` 逐字一致；`strings.xml`/`layout` 同名资源一致 | **已修正** |
| D-02 高：误称写网关绑定列表 | 明确“本机保存 + 平台 `/setmac`”，不写绑定列表，列为 P-03 | `NotificationsFragment.kt:226-263` 只写 `MyPrefs.mac_addr` 并调用 `setMacAddress`；全仓 `BLE_ENQUEUE_WRITE_IDS` 仅接收端（`MqtttService.kt:1312`） | **已修正** |
| D-03 高：报告按钮“功能可点” | 拆分 O-PKG-1/O-PKG-2，明确报告按钮被导航拦截、不可用 | 真机点击「查看今日报告」后 `navigation_dashboard` 选中（`ui_after_report_click.xml`）；与 06 §3 #18 一致 | **已修正** |
| D-04 高：明文放行清单错误 | 写明 3 个域名及用途，旧主机为历史条目 | `network_security_config.xml` 恰为 3 个 `domain-config`：`8.140.23.253`、`117.72.84.210`、`192.168.4.1` | **已修正** |
| D-05 中：未说明 12 字符限制 | 写明 12 位、无分隔符、长度外不校验字符集 | `NotificationsFragment.kt:84-91` 仅 `length == 12`；提示 `请输入12个字符` | **已修正** |
| D-06 中：无效“滚动”建议 | 删除滚动建议，写明页面不是可滚动容器 | `ui_08_my_tab_scrolled.xml`（既有证据）滑动前后 bounds 不变；rev 2 文本一致 | **已修正** |
| D-07 低：`gc` 只解释一个值 | 补全 0..3 | `measure.c:160-165,226-233` 逐值一致 | **已修正** |
| D-08 低：VDD 判读表出处错误 | 改引 `artifacts/firmware_implementation.md` §2 | 该文件 590-596 行确有 `mv ≈ 2700–3100` 表 | **已修正** |
| D-09 低：章节引用错误 | 改为 05 文档 §2/§3 | 05 §2 为认证约定、§3 为凭据处理 | **已修正** |
| D-10 低：登录限制提示/阈值不精确 | 使用原文 `您登录/退出次数太多，需要重新安装软件`，写明 `LOGIN_CNT > 30`（第 31 次起） | `ZhuCeFragment.kt:204-210` 逐字一致 | **已修正** |
| D-11 提示：安装字节状态 | 更新为“当前候选已安装并核对，关键路径未重跑” | `android_install.json` SHA-256 `d2054221…`；`dumpsys package` versionCode 10 / 1.9；README §5 #7、04 §8.2、06 #9 一致 | **已修正** |

**小结**：上一轮 11 项均已按要求修正，未发现旧缺陷残留或“只改文字、未改事实”的情况。

---

## 3. 本轮新发现（文档缺陷与产品缺口分开）

### D-12（高，文档缺陷）docs/04 §4.1 把「打开传感器列表」写成可用的查看/删除入口，实测被底部导航拦截

- 文档位置：`docs/04_安卓App安装与使用.md:118-120`（步骤 4）
- 文档写：
  - “点「打开传感器列表」进入列表页。”
  - “该按钮位于页面底部、与底部导航区域重叠（同类布局问题，见 §4.2 与 **06 文档 §3**）；若点击无响应，不要反复点击，按布局缺陷登记。”
- 实测（Redmi 8A / 720×1520 / 当前候选 `d2054221…`）：
  - `opensensorlistbutton` bounds `[164,1296][456,1392]`，**整体**位于底部导航容器 `[0,1264][720,1424]` 之内；
  - `ui_click(text="打开传感器列表")` 后，页面未进入传感器列表，底部页签变为「设定」（`navigation_dashboard selected=true`）；
  - 证据：`evidence/doc_verification_20261111_rev2/ui_after_opensensorlist_click.xml`。
- 代码/导航：
  - `NotificationsFragment.kt:312-317` 是进入 `SensorIdListFragment` 的**唯一**代码路径；
  - `res/navigation/mobile_navigation.xml:47-48` 只有 `action_notificationsFragment_to_SensorIdListFragment`，没有其它入口。
- 交叉引用错误：`docs/06 §3` 的 19 项限制中**没有**「打开传感器列表」条目（#17 是网关按钮重叠、#18 是报告按钮被拦截）；因此“见 06 文档 §3”指向不存在的内容。
- 影响：用户/运维按手册无法查看或删除已绑定传感器。手册同时在 §4.1 步骤 3 与 §7 要求“最多 9 台、超限先删除多余设备”，但当前唯一删除入口不可用，容量超限的处置路径实际中断。
- 责任划分：
  - **文档**：不得把该步骤写成“进入列表页”；应明确当前构建下该入口被导航拦截、不可用，并给出可执行的替代（若没有替代，必须如实说明“当前版本无可用删除路径”），同时在 06 §3 增加对应限制项。
  - **产品**：见 P-04（Android 布局/导航拦截），由 Android 实现修复。

### D-13（中，文档缺陷）docs/04 §5 与 docs/06 §2.3 把“其他 4xx”一律描述为全局暂停，实际 408/429 为退避重试

- 文档位置：
  - `docs/04_安卓App安装与使用.md:181-185`：“其他 4xx | 视为请求被拒：同样全局暂停并保留数据”
  - `docs/06_维护排障与已知限制.md:56`：“显示‘请求被拒绝（HTTP 4xx）’”
- 实际实现：
  - `UploadResult.kt:25-31`：`401/403 → AuthRejected`；**`408`、`429` → Retryable**；其余 `400..499 → RequestRejected`；`5xx → Retryable`。
  - `CloudUploadRepository.kt:184-198`：`Retryable` 只做退避重试；只有 `AuthRejected/RequestRejected` 才 `park` 并置全局暂停。
  - 状态文案：408/429 时对话框显示“云端上传失败，重试中（待发 N 条）”，不是“请求被拒绝”。
- 影响：现场遇到 429（限流）或 408（超时）时，手册会引导按“永久拒绝/全局暂停”处理，与实际自动退避行为不符，可能触发不必要的手动重试或误判服务端鉴权问题。
- 修正建议：把行改为“除 401/403 与 408/429 外的其他 4xx”，并单列 408/429 为可重试；06 §2.3 同步。

### D-14（低，文档缺陷）docs/01 把中继器描述为“并转 MQTT”，网关固件无 MQTT/网络通路

- 文档位置：`docs/01_产品说明与版本变更.md:20`
- 文档写：中继器“接收 433 帧、缓存最近记录、经 BLE 供 App 读取、**并转 MQTT**”。
- 实际：
  - `CH592EVT/EVT/EXAM/BLE/beiwov2`（含 `APP/`）源码中检索 `mqtt/broker/HTTP/WiFi` **0 命中**；
  - 网关只有 433 接收 + BLE GATT；MQTT/TLS 与 HTTP 上传由 Android App 完成（`CloudConfig.kt`、`MqtttService.kt`）；
  - 文档自身 §1 拓扑图也把 MQTT/HTTP 画在 Android App 侧，表格与图自相矛盾。
- 修正建议：删除“并转 MQTT”，改为“经 BLE 供 App 读取；由 App 转发 MQTT/HTTP”。

### D-15（提示级，文档完整性）传感器注册的失败提示与编码字符集未完整说明

- 文档位置：`docs/04_安卓App安装与使用.md:111-116`
- 文档写：8 位传感器编码与别称；编码或别称为空/长度不符时提示 `请输入传感器编码和别称`。
- 实际：
  - `NotificationsFragment.kt:93-116` 只校验 `name.length > 0 && input.length == 8`；
  - `MacIdBook.kt:74-108` 还要求传感器编码为 **8 位十六进制**、别称为纯中文 ≤3 字或纯英数 1–8 位；不满足时返回 false，界面提示 `添加传感器失败,请联系客服`，而不是校验提示。
  - 文档没有说明“非十六进制字符/别名格式不符”的真实提示。
- 影响：用户输入 8 位非十六进制字符或格式不符的别名时，会看到“请联系客服”，按手册无法判断是输入格式问题。
- 建议：文档补充编码为 8 位十六进制、别名格式与失败提示；产品侧可另立改进（见 P-05，非本次文档失败的独立依据）。

### 产品缺口（与文档缺陷分开）

| ID | 观察 | 证据 | 责任/处理 |
|---|---|---|---|
| P-04（本轮新实测） | 「打开传感器列表」按钮整体位于底部导航之下，点击被导航拦截并切到「设定」，导致查看/删除已绑定传感器的唯一入口不可用 | `ui_after_opensensorlist_click.xml`；`fragment_notifications.xml`；`NotificationsFragment.kt:312-317`；`mobile_navigation.xml:47-48` | Android 实现：调整布局/导航层级，或提供可用的列表入口；文档先按实际限制描述 |
| P-05（观察） | 传感器编码/别称格式不合法时提示“添加传感器失败,请联系客服”，未区分输入格式错误 | `NotificationsFragment.kt:106-111`；`MacIdBook.kt:74-108` | Android 实现可选改进；不单独构成本次文档失败 |
| P-01/P-02/P-03（沿用） | 报告按钮被拦截（O-PKG-2）、网关按钮与输入区重叠（O-PKG-1）、App 未发现写网关绑定列表发送路径 | 上一轮 `DOC-VERIFY-001` §3；本轮报告按钮复现 | 既有观察，文档 rev 2 已按实际描述；P-03 待 Android 确认 |

---

## 4. 已执行的真实设备步骤与原始观察

### 4.1 设备与身份

- `android_test devices`：`12f046690506  device product:olivelite model:Redmi_8A`（在线）。
- `getprop`：Android 10 / API 29。
- 已安装包：`versionCode=10`、`versionName=1.9`、`firstInstallTime=2026-11-10 19:25:25`、`lastUpdateTime=2026-11-11 13:57:18`。
- 主机 APK：10,365,460 B，SHA-256 `d2054221…`、MD5 `58de16b8…`，与上一轮安装候选逐字节一致（`android_install.json`）。
- 本轮**未重装**；04 §2 安装/升级文本在 rev 1→rev 2 未变化，上一轮已真实执行安装与覆盖升级保留数据。
- 原始：`device_package_state.txt`、`static_identity.txt`、`usb_enumeration.txt`。

### 4.2 真机 UI 复现

- 启动 App（`android_test launch`）成功；首次 dump 直接显示「网关与上传设置」对话框（上一会话遗留窗口）。
- 对话框真实文字（`ui_gateway_dialog_rev2.xml`）：
  - `未配置网关地址\n平台身份（mac_addr）：`；
  - `网关 BLE 地址（AA:BB:CC:DD:EE:FF）`；
  - `搜索附近网关`、`扩大搜索（不过滤）`、`清除网关地址`、`保存网关地址`；
  - `云端上传状态：无（待发 0 条）`、`重试上传`、`关闭`。
  - 与 04 §4.2 表格一致。
- 关闭对话框后「我的」页真实文字（`ui_my_page_rev2.xml`）：
  - `用户：未登录`、`账户`；
  - `请输入网关编码：`、`使用摄像头扫一扫`、`请输入网关上的字母串`、`保存`；
  - `请输入传感器编码和别称：`、`使用摄像头扫一扫`、`请输入传感器的字母串`、`请输入传感器别称用于显示`、`注册传感器`；
  - `网关与上传设置`、`查看今日报告`、`打开传感器列表`。
  - 与 04 §4.1 页面文字逐字一致（D-01 已修正）。
- 点击「打开传感器列表」：页面未跳转，底部页签变为「设定」（`ui_after_opensensorlist_click.xml`）→ D-12/P-04。
- 点击「查看今日报告」：同样切到「设定」（`ui_after_report_click.xml`）→ 与 04 §4.2 O-PKG-2、06 §3 #18 描述一致。
- 未执行写操作：未输入/保存网关编码、未注册/删除传感器、未改网关/网络状态、未卸载/清数据。

### 4.3 静态/证据交叉核对

- 固件发布包：zip md5 `5df0c6d4…`；成员 axf `b6a15fed…`（297,380 B）、hex `4d728dec…`（75,275 B）、`PACKAGE.txt` `a58f67fc…`（3,202 B）；zip CRC 无坏成员；与 02 §2 表逐字一致。
- 当前 Keil 产物 `MDK/output/exe/Project.axf` md5 `b6a15fed…`、hex `4d728dec…`，与发布包一致。
- 产品源码：`git diff 82a7e74..HEAD` 对传感器 `USER/MDK/EWARM/UM2005C/COMMON/gcc` 为 0 行；`git diff 28b78b0 f516a73 -- dengbei_care` 无输出 → 01 §5 的源码提交声明成立。
- 固件字段：`debug_trace.h`、`measure.c`、`sensor_config.h`、`fw_core.c` 与 01 §3、02 §6、03 §4/§5 一致；EV-036 原始捕获逐字一致（VDD 3289 mV、五行有序、6 个 `S` 行全 `q=1 V=0 D=0`、`a=2860..2886`、双字 CRC 匹配）。
- 网表/BOM：`LIGHT_ADC` 含 J4.1/R3.1/U7 PB04，`LIGTHT_POWER` 含 R3.2/U7 PB05，R3=5 MΩ → 01 §2 光敏分压描述一致。
- 平台：`network_security_config.xml` 3 个域名；Kotlin 源码 `117.72.84.210` 0 命中；`CloudConfig.kt` 主机/端口唯一来源；`jd-ca.crt` md5 一致；`UploadPayload.kt` 字段/单位一致 → 05 §1/§2 一致。
- 网关源码无 MQTT/HTTP/WiFi 通路 → 01 §1 的“并转 MQTT”不成立（D-14）。
- 详见 `source_checks.txt`。

---

## 5. 未执行 / 未复跑项及原因

| 项 | 状态 | 原因 |
|---|---|---|
| 固件重新烧录、上电五行与 COM43 重新采集 | **未复跑** | 本能力有效工具仅 `android_test`、`usb_list_devices`；无 `mdk_flash`/`serial_capture`。已逐字交叉核对 EV-036 原始证据，未写成通过 |
| 固件 I²C 位级重新采集 | 未复跑 | 无 `sigrok_capture` 工具 |
| 平台 `/upload_data` 真实写入/幂等/回读 | 未执行 | 无写授权与数据库授权；仅交叉核对 BE-EXP-004 |
| 网关→手机跨链路（433+BLE） | 未执行 | 用户明确安排后续联调阶段 |
| 真实升温 >0.9 ℃ / >35 ℃、调温、改变光强、遮光、功耗 | 不安排 | 用户明确当前不能调温/调光/测功耗 |
| `FW-LIGHT-DEF2-ENABLE` | 未执行（`SKIP_CONFIRMED`，2026-11-11T03:50:42Z） | 人工确认“无法准备，先跳过”；文档 01/06 已如实披露，本轮不重测 |
| Android 扫描/GATT/上传/401 恢复在当前候选重跑 | 未执行 | 本轮只做安装身份核对与「我的」/对话框/两个底部按钮的真实 UI 复现；未改网络或网关状态 |
| Android release 变体/生产签名 | 未产出 | 离线缺 lint 依赖（既有结论） |

上述未执行项不作为本轮 TEST_FAIL 的依据；本轮失败由 D-12/D-13/D-14 的实测/静态可复现缺陷直接支持。

---

## 6. 给技术文档能力的可操作修正

1. **docs/04 §4.1 步骤 4（D-12）**：删除“点「打开传感器列表」进入列表页”的可用性表述；改为“当前构建下该按钮整体位于底部导航之下，点击被导航拦截并切到「设定」，**无法进入传感器列表**；因此当前版本无通过 UI 查看/删除绑定传感器的可用路径”。同时修正对 06 §3 的引用。
2. **docs/06 §3（D-12）**：新增限制项（与 O-PKG-1/O-PKG-2 并列）：`打开传感器列表` 入口被底部导航拦截，影响删除与 ≤9 容量处置；引用 `ui_after_opensensorlist_click.xml`。
3. **docs/04 §5、docs/06 §2.3（D-13）**：将“其他 4xx”限定为“除 401/403、408、429 外的其他 4xx”；408/429 单列为可重试（退避），状态为“重试中”。
4. **docs/01 §1（D-14）**：删除中继器“并转 MQTT”，改为“经 BLE 供 App 读取，由 App 转发 MQTT/HTTP”。
5. **docs/04 §4.1 步骤 3（D-15，可选）**：补充传感器编码为 8 位十六进制、别名格式，以及格式不符时的实际提示 `添加传感器失败,请联系客服`。
6. **版本控制**：修正后提升文档包版本号（rev 3），重新按 §1 的读者任务核对；不得复用本报告作为新版本通过证据。
7. **产品侧（P-04）**：由 Android 实现修复“打开传感器列表”入口的布局/导航拦截；在修复前文档必须按“不可用”描述，不得以改文档降低产品要求。

---

## 7. 原始证据索引

| 文件 | 内容 |
|---|---|
| `evidence/doc_verification_20261111_rev2/ui_my_page_rev2.xml` | 「我的」页真实 UI 树（网关编码/保存/注册传感器/网关与上传设置/查看今日报告/打开传感器列表及 bounds） |
| `evidence/doc_verification_20261111_rev2/ui_gateway_dialog_rev2.xml` | 「网关与上传设置」对话框真实 UI 树 |
| `evidence/doc_verification_20261111_rev2/ui_after_opensensorlist_click.xml` | 点击「打开传感器列表」后的 UI 树（`navigation_dashboard` 选中，未进入列表） |
| `evidence/doc_verification_20261111_rev2/ui_after_report_click.xml` | 点击「查看今日报告」后的 UI 树（`navigation_dashboard` 选中，复现 O-PKG-2） |
| `evidence/doc_verification_20261111_rev2/device_package_state.txt` | 设备/包版本、安装时间、APK 摘要 |
| `evidence/doc_verification_20261111_rev2/static_identity.txt` | HEAD、发布包/成员、Keil 产物、APK、源码提交与平台静态身份 |
| `evidence/doc_verification_20261111_rev2/usb_enumeration.txt` | COM43/CW-DAPLink/COM26/手机/fx2lafw 枚举 |
| `evidence/doc_verification_20261111_rev2/source_checks.txt` | 文档逐条对应的源码/UI/证据核对（含 D-12/D-13/D-14 行号） |
| `evidence/doc_verification_20261111/DOC-VERIFY-001.md` | 上一轮 rev 1 校核报告（D-01..D-11） |
| `evidence/ev036/capture_analysis.txt` | EV-036 上电五行/6 周期/I²C 原始解析 |
| `evidence/android_build_20261110/apksigner_verify.txt`、`apk_badging.txt` | APK 签名与版本身份 |
| `fw_release/fw_sensor_FD-002r8.zip`、`fw_release/PACKAGE.txt` | 固件发布包与自述 |
| `artifacts/firmware_implementation.md` §2 | VDD 2700–3100 mV 判读表出处 |
| `artifacts/backend_feasibility_current.md` | BE-EXP-004 平台只读核验 |

---

## 8. 判定

- **TEST_FAIL**：
  - **D-12（高）**：文档承诺的查看/删除传感器入口实测被底部导航拦截；06 §3 未登记且交叉引用错误；对应产品缺陷 P-04。
  - **D-13（中）**：408/429 的实际重试语义与文档“其他 4xx 全局暂停”不符。
  - **D-14（低）**：网关“并转 MQTT”与网关源码不符。
  - **D-15（提示）**：传感器注册失败提示/字符集说明不完整。
- 上一轮 D-01..D-11 **全部已修正**，未发现残留。
- 未执行项（固件重烧/串口/I²C、平台写接口、跨链路、升温/调光/功耗）已在 §5 明示，不构成通过。
- 本报告不替代最终产品总功能测试；产品缺口 P-04/P-05 交 Android 实现，文档缺口交技术文档修正。
