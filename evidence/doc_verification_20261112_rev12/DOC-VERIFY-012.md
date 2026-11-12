# DOC-VERIFY-012 产品手册独立校核报告

- 校核对象：`docs/README.md` + `docs/01_产品说明与版本变更.md` … `docs/06_维护排障与已知限制.md`（DOC-PKG-001 **rev 12**）及随包 `dengbei_care/用户使用说明书.md`
- 对照候选：传感器固件 FD-002 rev 8（`fw_release/fw_sensor_FD-002r8.zip`，134,791 B，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`，FW-REL-001 rev 2）；安卓 1.9（`com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，debug 候选，10,365,464 B，SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`，AB-002）
- 校核时点：2026-11-12（Asia/Shanghai；工具时间 UTC 2026-11-12T02:38–02:43）
- 仓库 HEAD：`92bfcb257a6630ca705cace68985878bd086f7bf`（rev 12 文档/清单提交；开始时 `git status` clean）
- 设备：Redmi 8A（Android 10 / API 29，serial `12f046690506`）本轮在线；已装 versionCode 10 / versionName 1.9；设备 `base.apk` md5 `1e17e0c2…` 与主机候选逐字节一致
- **结论：TEST_PASS（限定 rev 12 改动与受影响步骤）**。rev 11 唯一阻断项 **D-44 已修正**：4 个受控验证脚本全部补入清单且用途与脚本内容相符；O-4、O-5 已按建议修正。整包复核未发现新的文档—候选不一致或失效步骤。§5 未执行项继续保持“未执行”，本报告不构成整产品功能验收。

---

## 1. 授权、方法与证据边界

| 类别 | 本轮做法 | 证据 |
|---|---|---|
| 变更复核 | `git show 92bfcb2` 逐文件核对 rev 12 变更范围；确认用户使用说明书本轮未改 | `static_checks.txt` §一 |
| 清单复核 | 复跑清单—工作区—Git 一致性脚本；独立统计条目/文件/目录、项目根未覆盖文件；逐条核对 4 个补入脚本与 37 条 `.bin` 标注 | `static_checks.txt` §二/§四/§五、`check_manifest_output.txt` |
| 路径引用 | 对 7 份 docs 加用户使用说明书复跑内联路径检查 | `check_doc_refs_output.txt`（`151 checked / 0 missing`） |
| 身份复算 | 复算固件包/成员/PACKAGE.txt、APK 大小/SHA-256/MD5、设备在装包版本与 `base.apk` md5 | `identity_checks.txt` §二/§三 |
| 真机核验 | 使用已授权 ADB/`android_test`：启动 App、`ui_info`、抓取传感器列表空态/「我的」页/上滑后/首页 UI 树；读取会话 logcat 尾部 | `identity_checks.txt` §四、`ui_*_live.txt`、`logcat_session_tail.txt` |
| 复用原始证据 | EV-038（7 周期、VDD 3287 mV、全 `V=0 D=0`）与 EV-036 样本值只读重读；未把它们当作本轮新采集 | `identity_checks.txt` §五/§六 |
| 未执行 | 固件重烧、COM43 采集、真实升温/光强、功耗、跨链路、平台写入/回读、回退、有数据删除、P-08～P-13 真机行为 | 见 §5 |
| 人工跳过项 | `FW-LIGHT-DEF2-ENABLE` 人工确认 `SKIP_CONFIRMED`（2026-11-11T03:50:42Z），**NOT EXECUTED**；不重测、不重复询问 | 06 §4、README §5 #1 |

说明：rev 12 的文档变更集中在清单补齐、索引更新与版本头，未改产品行为声明，也未改安装/操作步骤的实质内容；因此本轮对安卓关键用户入口做了有界真机复验，对固件安装/串口步骤采用“包/成员身份复算 + 既有 EV-038/EV-036 原始证据重读”，并明确标注未重新烧录或采集。未执行的步骤没有被写成通过。

---

## 2. 本轮必需读者任务与执行状态

| # | 读者任务 | 文档位置 | 本轮状态 |
|---|---|---|---|
| T1 | 识别当前固件/APK/文档版本与来源 | README §1/§4、01 §5 | **已核对**：DOC-PKG-001 rev 12；固件包 md5 `5df0c6d4…`；APK `71a041d3…`；设备在装 10/1.9 且 `base.apk` md5 一致 |
| T2 | 校验并安装传感器固件发布包 | 02 §1–§5 | **静态核对**：包 134,791 B、三成员 MD5 与 02 §2 表逐项一致；本轮未重烧（无 `mdk_flash` 工具边界），不写成安装通过 |
| T3 | 下载后核对串口上电五行与周期 `S` 行 | 02 §6、03 | **静态对照 EV-038/EV-036**：五行有序、7 周期 `k=0..21` 全 `q=1 V=0 D=0`、p 链匹配；本轮未重新采集 |
| T4 | 安装/覆盖升级安卓 App | 04 §1–§2、用户手册 §2.1 | **身份核验**：候选与设备在装字节一致（md5 `1e17e0c2…`）；本轮未重复安装/升级 |
| T5 | 首次启动、权限与隐私政策 | 04 §3、用户手册 §2.2/§2.3 | 本轮启动成功、`MainActivity` 前台；权限/隐私页复用既有真机证据，未重新弹窗 |
| T6 | 注册/登录、登记网关编码、注册传感器 | 04 §4.1、用户手册 §3/§4.1 | **实机确认**「用户：未登录」「账户」「请输入网关编码」「保存」「注册传感器」等文字与可点击性 |
| T7 | 配置网关 BLE 地址与上传状态 | 04 §4.2、用户手册 §4.4 | **实机确认**上滑后「网关与上传设置」`[32,1120][688,1216]` 可达，与「注册传感器」`[32,976][688,1072]` 不重叠 |
| T8 | 查看/删除已绑定传感器 | 04 §4.1 步骤 4、06 #20 | **实机确认空态**：`已保存的 ID（0）`、`删除选中` 存在；有数据时的列出/删除**未执行** |
| T9 | 设置全局/每设备报警参数 | 用户手册 §7、04 §6 | 静态复核默认值与节流语义；本轮未改参数 |
| T10 | 启动守护、理解条件上报与上传状态 | 04 §4.3/§5、用户手册 §5/§6 | 静态复核；P-08～P-13 仍为源码静态结论 |
| T11 | 维护排障与已知限制 | 06 | 编号 1–25 连续；rev 12 版本记录/证据索引已更新；D-44 关闭 |
| T12 | 固件升级/回退 | 02 §8 | 静态核对；与“无 OTA/Bootloader”的发布事实一致 |
| T13 | 平台地址/端口/联调前提 | 05 | 静态核对 `CloudConfig.kt` 与 `network_security_config.xml`；未连接服务器 |
| T14 | 查看今日报告 | 04 §4.2/§4.3、用户手册 §9 | 入口文字与可点击性已在本轮 UI 树确认；打开 `ReportActivity` 复用同日 rev11 真机证据（同一在装 APK） |

---

## 3. 发现的问题与处理结果

### D-44（rev 11 阻断项）— 已修正

- rev 11 清单删除通配条目后遗漏 4 个受 Git 跟踪的脚本。rev 12 已在 `file_manifest.txt` 补回（rev 12 提交行号 1369/1375/1371/1373；本轮证据插入后为 1383/1389/1385/1387）：
  - `evidence/software_verify_db_migration_item006.py`
  - `evidence/software_verify_upload_payload_item008.py`
  - `evidence/software_verify_orchestration_item010.py`
  - `evidence/software_verify_report_wording_item013.py`
- 复核：4 个文件均存在且被 Git 跟踪；脚本 docstring 与清单用途描述相符；`evidence/software_test.md` 与三份门禁日志的引用仍有效。
- 结论：**关闭**。

### O-4 — 已修正

`docs/06:131` 现明确写为 `evidence/ev036/release_identity.txt`；该文件存在，ev037 目录存在。原文不再让读者误在 ev037 查找。结论：**关闭**。

### O-5 — 已修正

37 条未跟踪 `.bin` 清单条目均含“本机 `.bin` 留存（`.gitignore` 忽略，不随仓库分发）”或等价说明；`product_acceptance_20261111` 条目另有 hex 复核说明。结论：**关闭**。

### O-6（新观察，编辑性，不单独构成失败）

`file_manifest.txt` 的 `evidence/ev026/Project.axf`（rev 12 提交行 659 / 本轮插入后 673；EV-026 历史受测镜像副本，md5 `917bffe6…`，`*.axf` 被 `.gitignore` 忽略）与 `服务器迁移记录.md`（rev 12 提交行 1390 / 本轮插入后 1404；经 `.git/info/exclude` 排除的用户资料）未像 `.bin` 那样标注“本机留存/不随仓库分发”。两者路径存在、用途描述正确；Project.axf 不是当前发布包成员（发布包 axf 为 `b6a15fed…`），克隆仓库的读者无法取得该历史副本；服务器迁移记录按设计不入 Git。建议下一版统一标注，不影响当前读者步骤正确性。

---

## 4. 已核对一致的主要内容

1. **版本头**：README 与 01–06 全部为 `DOC-PKG-001 rev 12（2026-11-12）`；用户使用说明书为 1.9（2026-11）/ API 26+。
2. **清单**：rev 12 提交时为 1390 条 = 1301 个具体文件 + 89 个目录条目，`missing on disk=0`、`glob=0`、`bad format=0`；README §6.2 的“1301 条具体路径均存在、0 缺失”复算一致。写入本轮 14 条证据条目后复跑为 1404 条 = 1314 文件 + 90 目录，仍为 0 缺失。项目根 6 个交付根下无“被跟踪但未被任何条目覆盖”的文件。
3. **路径引用**：151 个内联路径引用全部可解析（0 missing），含 rev 12 新增的 DOC-VERIFY-011 索引、`evidence/ev036/release_identity.txt`、四个脚本路径。
4. **固件包身份**：134,791 B、md5 `5df0c6d4…`；axf `b6a15fed…` / hex `4d728dec…` / PACKAGE.txt `a58f67fc…` 与 02 §2 表、01 §5 一致；PACKAGE.txt 的提交、构建工具、Code/RW/ZI 与文档一致。
5. **规则与节拍**：`SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3`、`SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350`、`LIGHT_DARK_THRESHOLD_CODE=0` 与 01 §2/§3、03 §3/§5、06 #1 的表述一致；严格大于边界说明正确。
6. **UART1 手册**：`sensor_config.h` PA06 TXD→J3.1、PA05 RXD→J3.2、9600、UCLK 8 MHz、96 B 行预算；`debug_trace.h` 的 `V/D/o/n/x/a/q/r/s/y` 语义与 03 §4–§7 一致。
7. **平台配置**：`CloudConfig.kt` 主机 `8.140.23.253`、8883/5000 与 05 §1/§2 一致；`network_security_config.xml` 恰为 3 个 `domain-config`（平台、旧主机历史条目、网关热点），与 05 §2 的“3 个、不得新增”一致。
8. **Android 身份**：APK 10,365,464 B、SHA-256 `71a041d3…`、MD5 `1e17e0c2…`；设备 `base.apk` md5 相同；04 §1 与用户手册 §2.1 一致。
9. **真机 UI**：本轮重新确认「我的」页顶部报告/列表入口、`注册传感器` 与 `网关与上传设置` 的 bounds、传感器列表空态；与 04 §4.1/§4.2、06 #17/#20、用户手册 §4.1/§4.3 一致；logcat 会话尾部 0 条 `FATAL EXCEPTION`/`ANR`（`ENETUNREACH` 为手机无网络，已知条件）。
10. **未执行披露**：T=0 无光分支、真实升温、光强变化、跨链路、平台写入/回读、功耗、release、回退、有数据删除、P-08～P-13 在 01/04/06/README 与用户手册中口径一致，未写成通过；整产品验收未重跑已明确声明。

---

## 5. 未执行项与限制（必须随交付披露）

| 项 | 状态 | 原因/边界 |
|---|---|---|
| `FW-LIGHT-DEF2-ENABLE` | **NOT EXECUTED（人工 SKIP_CONFIRMED，2026-11-11T03:50:42Z）** | 缺完全遮光窗口与同板同构建双态证据；不重测、不重复询问；`T=0` 保持 |
| 固件重新烧录 / COM43 采集 | 未执行 | 当前能力有效工具不含 `mdk_flash`/`serial_capture`；引用 EV-038/EV-036 原始证据 |
| 真实升温 >0.9 ℃ / >35.0 ℃ 触发与 433 实发 | 未执行 | 用户明确当前不能调节温度 |
| 光强变化/遮光刺激 | 不安排 | 用户明确当前不能改变光强 |
| 网关→手机跨链路（433+BLE） | 未执行 | 用户安排后续联调 |
| `/upload_data` 真实写入、幂等、回读 | 未执行 | 无写授权 |
| `E2E-04` 功耗 | 未执行（已批准延期） | 无电流表/SMU/示波器 |
| Android release / 生产签名 | 未产出 | 离线缺 lint 依赖 |
| Android 回退流程 | 未执行 | 本轮未安装/回退；复用同日覆盖升级证据 |
| 「打开传感器列表」有数据时的列出/单项/批量删除 | 未执行 | 当前无绑定数据；不得写成通过 |
| P-08～P-13 真机行为 | 未执行 | 本轮仅源码静态对照 |

---

## 6. 给技术文档 / 清单维护者的建议

1. **O-6（建议，非阻断）**：给 `evidence/ev026/Project.axf` 与 `服务器迁移记录.md` 补充“本机留存/不随仓库分发”说明，与 `.bin` 口径统一。
2. rev 12 本身无需修正；若采纳 O-6，可并入下一次文档版本，并只复核受影响条目。

---

## 7. 附件（本目录）

| 文件 | 内容 |
|---|---|
| `identity_checks.txt` | HEAD、固件包/成员/PACKAGE、APK 与设备字节、设备版本、Activity/UI bounds、EV-038/EV-036 重读、版本头 |
| `static_checks.txt` | rev 12 变更范围、D-44/O-4/O-5/O-6、清单统计、路径引用、跨文档一致性、未执行项 |
| `ui_home_live.txt` / `ui_my_page_live.txt` / `ui_my_scrolled_live.txt` / `ui_sensor_list_live.txt` | 本轮真机 UI 树文本/bounds 提取 |
| `logcat_session_tail.txt` | 本轮会话 logcat 尾部（0 条 FATAL/ANR） |
| `check_doc_refs.py` / `check_doc_refs_output.txt` | 可复跑的文档路径引用检查与输出（151 checked / 0 missing） |
| `check_manifest.py` / `check_manifest_output.txt` | 可复跑的清单—工作区—Git 一致性检查与输出 |
| `extract_ui.py` | uiautomator UI 树文本/bounds 提取脚本 |

**最终结论：TEST_PASS（限定 DOC-PKG-001 rev 12 的改动与受影响步骤）。** D-44 已关闭，O-4/O-5 已修正，未发现新的文档缺陷；O-6 为编辑性建议。§5 的未执行项继续保持“未执行”，不得写成通过；本报告不替代整产品最终验收。
