# 产品文档独立校核报告（DOC-VERIFY-003）

- 校核角色：软件测试（独立校核；未修改产品源码与文档正文）
- 校核时点：2026-11-11（Asia/Shanghai；命令/采集时间为 UTC）
- 仓库 HEAD：`eac58ee26a0cf0936f9f1bfa2087215584289f7a`（文档 rev 3 提交，工作树干净）
- 校核对象：`docs/README.md` 及 `docs/01`–`docs/06`（DOC-PKG-001 **rev 3**）
- 覆盖候选：
  - 传感器固件 FD-002 rev 8 发布包 `fw_release/fw_sensor_FD-002r8.zip`（134,791 B，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`）
  - 安卓 App `com.jinyuni.dengbei_care` versionCode 10 / versionName 1.9，debug 候选 APK SHA-256 `d20542219faeaf4be0d73ce4650c57e3ea25bc54183ff0117a3f61c0e41bd8b6`
  - 平台服务既有部署（BE-EXP-004 只读核验）
- 上一轮：`evidence/doc_verification_20261111_rev2/DOC-VERIFY-002.md`（rev 2，TEST_FAIL，D-12..D-15）
- **结论：TEST_FAIL**。rev 3 已修正 DOC-VERIFY-002 的 D-12/D-13/D-14/D-15，逐项复核见 §2；但 rev 3 新增/残留 2 项可操作文档缺陷：
  - **D-16（中）**：`docs/06 §2.1` 排障第 5 步仍要求“删除多余绑定”，与同文档 §3 #20、`docs/04 §4.1` 步骤 4 的“当前无 UI 路径删除”直接矛盾，排障流程不可执行。
  - **D-18（中）**：`docs/06 §2.3` 写“自动退避重试，不需手动干预”，未说明上传重试上限为 20 次；达到上限后待发行停止自动重试，必须手动「重试上传」才能恢复。
  - 另有 **D-17（低/提示）**：注册传感器失败提示的“仅两类原因”未覆盖“编码已存在”。

> 本报告是独立校核证据，不是产品总功能验收。未执行项在 §5 明示，不写成通过；本轮判定由已核对/实测的文档缺陷驱动。

---

## 1. 本轮必需读者任务与覆盖方式

| # | 读者任务 | 覆盖方式 | 结果 |
|---|---|---|---|
| 1 | 按 02 校验发布包并确认烧录身份 | 复算 zip/成员 MD5、大小；核对 Keil 产物与工程保存的 Device/Flash 算法；交叉核对 EV-036 | 一致；本轮无 `mdk_flash`/`flash_device`，未重新烧录（§5） |
| 2 | 按 02/03 接串口并判读上电五行与 `S`/`G`/`P` 行 | 静态核对 `debug_trace.c`/`debug_trace.h`/`sensor_config.h`；逐字对照 `evidence/ev036/capture_analysis.txt` | 一致；本轮无 `serial_capture`，未重新采集（§5） |
| 3 | 按 04 安装/覆盖升级 App | 核对设备已安装 APK 与主机候选 SHA-256/版本；上一轮已真实执行安装与覆盖升级保留数据 | 一致（本轮未重装） |
| 4 | 按 04 §4.1 登记网关编码、注册传感器、查看/删除设备 | **真机执行**：启动、切「我的」、读取真实 UI 树；点击「打开传感器列表」复现 | 文档已如实描述不可用（D-12 已修正）；产品 P-04 仍在 |
| 5 | 按 04 §4.2 配置网关 BLE 地址与上传状态 | **真机执行**：打开对话框并读取真实 UI 树；静态核对 `GatewayBleSettingsDialog.kt` | 文字/行为一致 |
| 6 | 按 04 §5 / 06 §2.3 理解上传失败分类与重试 | 静态核对 `UploadResult.kt`、`CloudUploadRepository.kt`、`UploadOutbox.kt`、对话框状态文案 | **D-18：未说明 20 次上限后停止自动重试** |
| 7 | 按 06 §2.1 执行“手机端无数据”排障 | 静态核对排障流程与 §3 限制表、04 §4.1 | **D-16：第 5 步“删除多余绑定”不可执行** |
| 8 | 按 05 核对平台入口与客户端配置 | 静态核对 `CloudConfig.kt`、`MqtttService.kt`、`network_security_config.xml`、`jd-ca.crt` | 一致；未连接服务器（§5） |
| 9 | 按 01 理解系统组成与数据链路 | 静态核对源码、网关 APP 目录、发布/构建证据 | 一致（D-14 已修正） |
| 10 | 按 04 §1/§3 核对包身份、权限 | APK 摘要/签名/`apk_badging.txt`；设备已安装包摘要 | 一致 |
| 11 | 按 06 §4 披露未执行/延期项 | 对照运行时延期记录与证据 | 一致，未写成通过 |

---

## 2. DOC-VERIFY-002 的 D-12..D-15 逐项复核（rev 3）

| 上轮 ID | rev 3 修正 | 独立复核证据 | 结果 |
|---|---|---|---|
| D-12 高：列表入口被导航拦截却写成可用 | 04 §4.1 步骤 4 改为“当前不可用”，给出 bounds 与唯一代码路径；06 §3 新增 #20 | 真机 `ui_my_page_rev3.xml`：`opensensorlistbutton=[164,1296][456,1392]`，`nav_view=[0,1264][720,1424]`；点击后 `ui_after_opensensorlist_click_rev3.xml` 中 `navigation_dashboard selected=true`，未进入列表；`NotificationsFragment.kt:312-317` 为唯一入口，`mobile_navigation.xml:47-48` 无第二入口 | **已修正** |
| D-13 中：408/429 被写成全局暂停 | 04 §5 拆出 408/429 行；06 §2.3 改为“除 401/403/408/429 外” | `UploadResult.kt:25-33`：401/403→AuthRejected；408/429→Retryable；其余 4xx→RequestRejected；`CloudUploadRepository.kt:184-198` 只有 Auth/RequestRejected 置全局暂停 | **已修正** |
| D-14 低：网关“并转 MQTT” | 01 §1 改为“不自带 MQTT/HTTP/WiFi，云端转发由 App 完成” | `beiwov2/APP` 检索 `mqtt/broker/HTTP/WiFi` 0 命中；MQTT/HTTP 客户端在 Android | **已修正** |
| D-15 提示：注册失败提示/字符集不全 | 04 §4.1 步骤 3 补齐 8 位十六进制、别名格式与两类提示 | `NotificationsFragment.kt:93-116`；`MacIdBox.kt:74-103` 规则一致 | **已修正**；但见 D-17 的重复编码遗漏 |

**小结**：上一轮 4 项均已按要求修正，未发现“只改文字、未改事实”的情况。

---

## 3. 本轮新发现（文档缺陷与产品缺口分开）

### D-16（中，文档缺陷）docs/06 §2.1 排障第 5 步仍要求“删除多余绑定”，与当前无 UI 删除路径矛盾

- 文档位置：`docs/06_维护排障与已知限制.md:36`
  ```
  5) 网关是否在线、绑定是否 ≤9 台？ → 删除多余绑定；确认网关供电与蓝牙可发现
  ```
- 同文档 `§3 #20`（行 92）已写明：“无法查看/删除已绑定传感器；绑定已满 9 台时‘先删除多余设备’无 UI 路径”；
  `docs/04 §4.1` 步骤 4（行 124-128）也写明当前版本没有可用 UI 途径。
- 实测与源码：见 §4.2；`NotificationsFragment.kt:312-317` 是唯一入口，点击被 `nav_view` 拦截。
- 影响：这是“手机端无数据”主排障路径的第 5 步。用户/运维按步骤 5 操作时找不到“删除”入口，且该步骤与后续限制表自相矛盾；绑定超限时排障流程实际中断。
- 责任划分：**文档**（修正流程并交叉引用 #20，给出可执行的替代/如实说明）；**产品 P-04** 由 Android 实现修复。

### D-18（中，文档缺陷）docs/06 §2.3 写“自动退避重试，不需手动干预”，未说明 20 次上限后停止自动重试

- 文档位置：
  - `docs/06_维护排障与已知限制.md:54`：“待发条数持续增长、显示‘重试中’ | 网络/服务端 5xx 或 408/429（超时/限流）：会自动退避重试，**不需手动干预**；长期不恢复再查网络/服务端”
  - `docs/04_安卓App安装与使用.md:190,192,194`：只写“退避重试/按退避继续”，未写上限；行 195 只列出手动「重试上传」的操作，未说明何时必须使用。
- 实际实现：
  - `CloudUploadRepository.kt:228-229`：`DEFAULT_MAX_ATTEMPTS = 20`；`DEFAULT_MAX_BACKOFF_SECONDS = 1800`。
  - `CloudUploadRepository.kt:184-188`：`Retryable` 只执行 `outbox.markFailed(...)` 与 `gate.markUploadRetryScheduled(...)`。
  - `UploadOutbox.kt:74-76`：`dueBatch(maxAttempts, ...)` 明确“`attempts >= maxAttempts` 的行不再自动重试，保留在待发箱”。
  - `CloudUploadRepository.kt:209-213`：只有手动 `retryBlockedNow()` 会 `resetForManualRetry` 并立即触发上传。
- 影响：长时间断网/服务端持续 5xx 累计 20 次失败后，待发数据仍保留但不再自动重试；网络恢复后也不会自行补传，必须人工点「重试上传」。手册“不需手动干预”会让运维误判为持续等待即可恢复。
- 修正建议：在 04 §5 与 06 §2.3 明确“单行最多自动重试 20 次（退避上限 30 分钟）；达到后保留数据但停止自动重试，需手动「重试上传」恢复”，并把 06 §2.3 第一行的“不需手动干预”限定在 20 次以内。

### D-17（低/提示，文档完整性）注册传感器失败提示的“两类原因”未覆盖“编码已存在”

- 文档位置：`docs/04_安卓App安装与使用.md:118-120`
  - “失败提示分两类：编码长度不等于 8 或别称为空 → `请输入传感器编码和别称`；长度满足但不是 8 位十六进制、或别称不符合格式 → `添加传感器失败,请联系客服`（这是输入格式问题，不是设备故障）。”
- 实际：`MacIdBox.kt:24-33` 中 `if (map.containsKey(id)) return false`——编码已绑定同样返回 false，`NotificationsFragment.kt:106-111` 弹出同一提示 `添加传感器失败,请联系客服`。这不是输入格式问题，也不是设备故障。
- 影响：用户重复注册同一编码时，按手册会误判为格式错误，无法定位原因。
- 建议：在该条补充“或该编码已绑定”，并相应调整“这是输入格式问题”的限定语。

### 产品缺口（与文档缺陷分开）

| ID | 观察 | 证据 | 责任/处理 |
|---|---|---|---|
| P-04（沿用，本轮再实测） | 「打开传感器列表」按钮整体位于底部导航之下，点击被拦截并切到「设定」，查看/删除已绑定传感器的唯一入口不可用 | `ui_after_opensensorlist_click_rev3.xml`；`NotificationsFragment.kt:312-317`；`mobile_navigation.xml:47-48` | Android 实现：调整布局/导航层级或提供可用入口；文档已按实际限制描述 |
| P-05（沿用） | 传感器编码/别称格式不合法或编码重复时提示“添加传感器失败,请联系客服”，未区分原因 | `NotificationsFragment.kt:106-111`；`MacIdBox.kt:24-33,74-103` | Android 实现可选改进；不单独构成本次文档失败 |
| P-06（安全观察） | Android 源码 `cloud/CloudConfig.kt` 注释中包含 MQTT 共享固定后缀明文；文档未复制该值 | `CloudConfig.kt` 注释（本报告不复述值） | Android 实现：改为引用配置或被脱敏的占位，不在源码注释保存口令；不构成本次文档失败 |

---

## 4. 已执行的真实设备步骤与原始观察

### 4.1 设备与身份

- `android_test devices`：`12f046690506  device product:olivelite model:Redmi_8A`（在线）。
- `getprop`：Android 10 / API 29。
- 已安装包：`versionCode=10`、`versionName=1.9`、`firstInstallTime=2026-11-10 19:25:25`、`lastUpdateTime=2026-11-11 13:57:18`。
- 设备上 `base.apk` 的 SHA-256 `d2054221…`，与主机候选 APK 逐字节一致（10,365,460 B，MD5 `58de16b8…`）。
- 本轮**未重装**；04 §2 安装/升级文本在 rev 2→rev 3 未变化，上一轮已真实执行安装与覆盖升级保留数据。

### 4.2 真机 UI 复现

- 启动 App 成功；初始落在「设定」页（上一会话遗留），底部三页签可切换。
- 「我的」页真实文字（`ui_my_page_rev3.xml`）：
  - `用户：未登录`、`账户`；
  - `请输入网关编码：`、`使用摄像头扫一扫`、`请输入网关上的字母串`、`保存`；
  - `请输入传感器编码和别称：`、`使用摄像头扫一扫`、`请输入传感器的字母串`、`请输入传感器别称用于显示`、`注册传感器`；
  - `网关与上传设置`、`查看今日报告`、`打开传感器列表`。
  - 与 04 §4.1 行 95 逐字一致。
- 关键 bounds（真机）：
  - `opensensorlistbutton` `[164,1296][456,1392]`；`nav_view` `[0,1264][720,1424]`；
  - `viewReportButton` `[32,1280][688,1376]`；
  - `gatewayBleButton` `[32,1168][688,1264]`；`sensornameinput` `[33,1104][533,1224]`；`addsensorid` `[452,1128][688,1224]`（重叠 56 px）。
  - 与 04 §4.2 O-PKG-1/O-PKG-2/P-04 的数值一致。
- 点击「打开传感器列表」：页面未进入列表，`navigation_dashboard`（设定）`selected=true`（`ui_after_opensensorlist_click_rev3.xml`）。
- 点击「查看今日报告」：同样切到「设定」，未进入 `ReportActivity`（`ui_after_report_click_rev3.xml`）。
- 「网关与上传设置」对话框真实文字（`ui_gateway_dialog_rev3.xml`）：
  - `未配置网关地址\n平台身份（mac_addr）：`；
  - `网关 BLE 地址（AA:BB:CC:DD:EE:FF）`；
  - `搜索附近网关`、`扩大搜索（不过滤）`、`清除网关地址`、`保存网关地址`；
  - `云端上传状态：无（待发 0 条）`、`重试上传`、`关闭`。
  - 与 04 §4.2 表格一致。
- 未执行写操作：未输入/保存网关编码、未注册/删除传感器、未改网关/网络状态、未卸载/清数据。

### 4.3 静态/证据交叉核对

- 固件发布包：zip md5 `5df0c6d4…`；成员 axf `b6a15fed…`（297,380 B）、hex `4d728dec…`（75,275 B）、`PACKAGE.txt` `a58f67fc…`（3,202 B）；与 02 §2 表一致；当前 Keil 产物摘要相同。
- 工程保存配置：Device `CW32L010F8`、Flash 算法 `FlashCW32L010_TEST.FLM`、UL2CM3；与 02 §1/§5 一致。
- 固件字段：`sensor_config.h`（RTC 1 min×3、RISE=9、HIGH=350、T=0、PA06/PA05/9600）；`debug_trace.c`（S/G/P 行格式与 ≤96 B 预算）；EV-036 五行与 6 个 `S` 行逐字一致。
- USB：COM43 Prolific `067B:2303`；COM26 为 `C251:F001` CDC；CW-DAPLink 序列号 `87094109484987710672FF50`；fx2lafw 在线。
- pyocd 0.36.0 可执行，`pyocd list` 识别 CW-DAPLink，`cortex_m` target 存在；本轮**未执行 reset/烧录**。
- Android：`UploadResult.kt`/`CloudUploadRepository.kt`/`UploadOutbox.kt`/`GatewayBleSettingsDialog.kt`/`MacIdBox.kt`/`NotificationsFragment.kt` 与 04/06 的对应段落一致，除 D-17/D-18 所述遗漏。
- 平台：`network_security_config.xml` 3 个域名；`CloudConfig.kt` 主机/端口唯一来源；`jd-ca.crt` md5 一致；topic/clientId/TLS 与 05 §2 一致。
- 网关 APP 目录无 MQTT/HTTP/WiFi 通路，01 §1 已修正。
- 详见 `source_checks.txt`、`static_identity.txt`。

---

## 5. 未执行 / 未复跑项及原因

| 项 | 状态 | 原因 |
|---|---|---|
| 固件重新烧录、上电五行与 COM43 重新采集 | **未复跑** | 本能力有效工具仅 `android_test`、`usb_list_devices`；无 `mdk_flash`/`serial_capture`/`flash_device`。已逐字交叉核对 EV-036 原始证据，未写成通过 |
| 固件 I²C 位级重新采集 | 未复跑 | 无 `sigrok_capture` 工具 |
| pyocd reset（采集期间） | 未执行 | 无 `serial_capture` 配套入口；仅做只读发现（版本/探针/target），未触发 reset |
| 平台 `/upload_data` 真实写入、幂等与回读 | 未执行 | 无写授权与数据库授权；仅交叉核对 BE-EXP-004 |
| 服务器只读 SSH 复核 | 未执行 | `existing_service_ssh_inspection` 对本能力为 `unavailable_for_current_capability`（缺 `inspect_existing_service_readonly`）；沿用既有 BE-EXP-004 证据 |
| 网关→手机跨链路（433+BLE） | 未执行 | 用户明确安排后续联调阶段 |
| 真实升温 >0.9 ℃ / >35 ℃、调温、改变光强、遮光、功耗 | 不安排 | 用户明确当前不能调温/调光/测功耗 |
| `FW-LIGHT-DEF2-ENABLE` | 未执行（`SKIP_CONFIRMED`，2026-11-11T03:50:42Z） | 人工确认“无法准备，先跳过”；文档 01/06 已如实披露，本轮不重测 |
| Android 扫描/GATT/上传/401 恢复在当前候选重跑 | 未执行 | 本轮只做安装身份核对、「我的」页/对话框/两个底部按钮的真实 UI 复现；未改网络或网关状态 |
| Android release 变体/生产签名 | 未产出 | 离线缺 lint 依赖（既有结论） |
| 重新执行 Gradle 构建 | 未执行 | 本轮依赖既有构建证据与 APK 摘要核对；未改动源码或构建配置 |

上述未执行项不作为本轮 TEST_FAIL 的依据；本轮失败由 D-16/D-18（及 D-17 提示）的静态可复现缺陷直接支持。

---

## 6. 给技术文档能力的可操作修正

1. **docs/06 §2.1 第 5 步（D-16）**：改为“核对绑定是否 ≤9 台；**当前构建无 UI 路径查看/删除绑定**，若超限按 §3 #20 处理（等待 Android 修复；清除应用数据会删除全部本地绑定与未上传记录，仅在确认可丢弃时使用）”。删除“删除多余绑定”这一不可执行指令，或明确标注“当前不可执行”。
2. **docs/06 §2.3 第 1 行、docs/04 §5（D-18）**：补充“单行最多自动重试 20 次（退避上限 30 分钟）；达到上限后保留数据但停止自动重试，需手动「重试上传」恢复”；把“不需手动干预”限定为 20 次以内。
3. **docs/04 §4.1 步骤 3 第 4 条（D-17，可选）**：在失败原因中补充“或该编码已绑定”；不要把同一提示全部归为“输入格式问题”。
4. **版本控制**：修正后提升文档包版本号（rev 4），重新按 §1 的读者任务核对；不得复用本报告作为新版本通过证据。
5. **产品侧（P-04）**：由 Android 实现修复“打开传感器列表”入口的布局/导航拦截；在修复前文档必须保持一致地描述为不可用。
6. **产品侧（P-06，安全）**：Android 实现移除源码注释中的 MQTT 共享固定后缀明文，改为配置引用或脱敏占位；不构成本次文档失败。

---

## 7. 原始证据索引

| 文件 | 内容 |
|---|---|
| `evidence/doc_verification_20261111_rev3/ui_my_page_rev3.xml` | 「我的」页真实 UI 树（按钮文字与 bounds、三页签） |
| `evidence/doc_verification_20261111_rev3/ui_after_opensensorlist_click_rev3.xml` | 点击「打开传感器列表」后的 UI 树（`navigation_dashboard` 选中，未进入列表） |
| `evidence/doc_verification_20261111_rev3/ui_after_report_click_rev3.xml` | 点击「查看今日报告」后的 UI 树（`navigation_dashboard` 选中，复现 O-PKG-2） |
| `evidence/doc_verification_20261111_rev3/ui_gateway_dialog_rev3.xml` | 「网关与上传设置」对话框真实 UI 树 |
| `evidence/doc_verification_20261111_rev3/device_package_state.txt` | 设备/包版本、安装时间、设备 APK 摘要与真机操作记录 |
| `evidence/doc_verification_20261111_rev3/static_identity.txt` | HEAD、固件发布包/成员、Keil 产物、APK、USB、pyocd、平台静态配置 |
| `evidence/doc_verification_20261111_rev3/source_checks.txt` | 文档逐条对应的源码/UI/证据核对（含 D-16/D-17/D-18 行号） |
| `evidence/doc_verification_20261111_rev2/DOC-VERIFY-002.md` | 上一轮 rev 2 校核报告（D-12..D-15） |
| `evidence/doc_verification_20261111/DOC-VERIFY-001.md` | 更早 rev 1 校核报告（D-01..D-11） |
| `evidence/ev036/capture_analysis.txt` | EV-036 上电五行/6 周期/I²C 原始解析 |
| `evidence/android_build_20261110/apksigner_verify.txt`、`apk_badging.txt` | APK 签名、版本与权限身份 |
| `fw_release/fw_sensor_FD-002r8.zip`、`fw_release/PACKAGE.txt` | 固件发布包与自述 |

---

## 8. 判定

- **TEST_FAIL**：
  - **D-16（中）**：`docs/06 §2.1` 第 5 步“删除多余绑定”与同文档 §3 #20、`docs/04 §4.1` 步骤 4 矛盾；当前构建无 UI 删除路径，主排障步骤不可执行。
  - **D-18（中）**：`docs/06 §2.3`/`docs/04 §5` 未说明上传重试上限 20 次；达到上限后停止自动重试且需手动「重试上传」，与“不需手动干预”不符。
  - **D-17（低/提示）**：注册失败提示未覆盖“编码已存在”。
- DOC-VERIFY-002 的 D-12/D-13/D-14/D-15 **全部已修正**，未发现残留。
- 产品缺口 P-04 仍存在（Android 布局/导航）；P-05/P-06 为观察项，不单独构成本次文档失败。
- 未执行项（固件重烧/串口/I²C、平台写接口、跨链路、升温/调光/功耗）已在 §5 明示，不构成通过。
- 本报告不替代最终产品总功能测试；文档缺口交技术文档修正，产品缺口交对应实现。
