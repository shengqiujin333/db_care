# DOC-VERIFY-011 产品手册独立校核报告

- 校核对象：`docs/README.md` + `docs/01_产品说明与版本变更.md` … `docs/06_维护排障与已知限制.md`（DOC-PKG-001 **rev 11**）及随包 `dengbei_care/用户使用说明书.md`
- 对照候选：传感器固件 FD-002 rev 8（`fw_release/fw_sensor_FD-002r8.zip`，134,791 B，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`，FW-REL-001 rev 2）；安卓 1.9（`com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，debug 候选，10,365,464 B，SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`，AB-002）
- 校核时点：2026-11-12（Asia/Shanghai；工具时间 UTC）
- 仓库 HEAD：`ed6b96d060dd45642051de5738a1da54e260ac89`（rev 11 文档提交；开始时 `git status` clean，仅本轮新建证据目录）
- 设备：Redmi 8A（Android 10 / API 29，serial `12f046690506`）本轮在线；已装 versionCode 10 / versionName 1.9；设备 `base.apk` md5 `1e17e0c2…` 与主机候选逐字节一致
- **结论：TEST_FAIL**。rev 11 对 DOC-VERIFY-010 的 D-41、D-42、D-43 与 O-1～O-3 的修正在本轮独立复核中**均已落实且与真实文件/证据一致**；但 rev 11 清理清单通配条目时删除了 `evidence/software_verify_*_item0NN.py`，却只逐个列出 9 个脚本中的 5 个，导致 **4 个受版本控制、且被 `evidence/software_test.md` 与门禁日志引用的现存脚本重新变成清单未记录文件（D-44）**。按能力规则，存在未修正的共享清单缺陷即不得判通过。未执行项继续按“未执行”披露，不写成通过。

---

## 1. 授权、方法与证据边界

| 类别 | 本轮做法 | 证据 |
|---|---|---|
| 静态核对 | 逐项复核 rev 11 的 8 个变更文件与受影响段落；检查全部随包手册的路径引用、版本头、D-41/D-43 新增行、O-1～O-3 | `static_checks.txt` |
| 身份复算 | 复算固件包大小/md5/三成员摘要、PACKAGE.txt；复算候选 APK 大小/SHA-256/MD5；读取设备在装版本与 `base.apk` md5 | `identity_checks.txt` |
| 真机核验 | 使用已授权 ADB/`android_test` 入口：启动 App、进入「我的」页、上滑、点击「查看今日报告」与「打开传感器列表」，抓取 UI 树与 Activity 栈 | `identity_checks.txt` §四、`ui_my_page_live.txt`、`ui_my_scrolled_live.txt`、`ui_sensor_list_live.txt`、`logcat_session_tail.txt` |
| 复用有效证据 | EV-038 实板捕获分析（7 周期、VDD 3287 mV、全 `V=0 D=0`）；同候选安装/覆盖升级与 D-01～D-04a 复验记录 | `static_checks.txt` §D |
| 未执行 | 固件重烧、COM43 采集、真实升温/超温、光强变化/遮光、功耗、网关→手机跨链路、`/upload_data` 真实写入回读、有数据时的列表删除、Android 回退、P-08～P-13 真机行为 | 见 §5 |
| 人工跳过项 | `FW-LIGHT-DEF2-ENABLE` 人工确认 `SKIP_CONFIRMED`（2026-11-11T03:50:42Z），**NOT EXECUTED**；本轮不重测、不重复询问 | 06 §4、README §5 #1 |

说明：本轮对 rev 11 新声明的入口/布局在真实在装候选上重新走了一遍；未重新安装 APK，改为读取设备在装版本并比对 `base.apk` 摘要。固件未重烧，安装/串口步骤按既有 EV-038 原始证据静态对照。未执行的步骤没有被当作通过。

---

## 2. 本轮必需读者任务与执行状态

| # | 读者任务 | 文档位置 | 本轮状态 |
|---|---|---|---|
| T1 | 识别当前固件/APK/文档版本与来源 | README §1/§4、01 §5 | 已核对：HEAD `ed6b96d`、DOC-PKG-001 rev 11、固件包 md5 `5df0c6d4…`、APK `71a041d3…`；设备在装 10/1.9 且 `base.apk` 字节一致 |
| T2 | 校验并安装传感器固件发布包 | 02 §1–§5 | 静态核对：包 134,791 B / md5 一致；axf `b6a15fed…`、hex `4d728dec…`、PACKAGE.txt 与文档一致；本轮未重烧 |
| T3 | 下载后核对串口上电五行与周期 `S` 行 | 02 §6、03 | 静态对照 EV-038：五行有序、7 个 `S` 行、VDD 3287 mV、全 `q=1 V=0 D=0`；未重新采集 |
| T4 | 安装/覆盖升级安卓 App | 04 §1–§2、用户手册 §2.1 | 身份一致；设备在装 10/1.9、`base.apk` md5 一致；未重复安装 |
| T5 | 首次启动、权限与隐私政策 | 04 §3、用户手册 §2.2/§2.3 | 本轮实机启动成功；复用既有权限/隐私页真机证据 |
| T6 | 注册/登录、登记网关编码、注册传感器 | 04 §4.1、用户手册 §3/§4.1 | 本轮实机确认「用户：未登录」「账户」「请输入网关编码」「注册传感器」等控件文字与可点击性 |
| T7 | 配置网关 BLE 地址与上传状态 | 04 §4.2、用户手册 §4.4 | 本轮实机上滑后确认「网关与上传设置」`[32,1120][688,1216]` 可达，与「注册传感器」`[32,976][688,1072]` 不重叠 |
| T8 | 查看/删除已绑定传感器 | 04 §4.1 步骤 4、06 #20 | 实机确认空态入口可进入（`已保存的 ID（0）`、`删除选中`）；有数据时的列出/删除**未执行** |
| T9 | 设置全局/每设备报警参数 | 用户手册 §7、04 §6 | 复用既有核对；本轮未改 |
| T10 | 启动守护、理解条件上报与上传状态 | 04 §4.3/§5、用户手册 §5/§6 | 复用既有结论；P-08～P-13 仍为源码静态结论 |
| T11 | 维护排障与已知限制 | 06 | 编号 1–25 连续；D-43 三行已补；D-44 为清单侧遗留 |
| T12 | 固件升级/回退 | 02 §8 | 静态核对；无 OTA/Bootloader 结论与发布产物一致 |
| T13 | 平台地址/端口/联调前提 | 05 | 静态核对；未连接服务器 |
| T14 | 查看今日报告 | 04 §4.2/§4.3、用户手册 §9 | **实机确认**点击后 `mResumedActivity=com.jinyuni.dengbei_care/.report.ReportActivity` |

---

## 3. 发现的问题与可操作修正

### D-44（共享文件清单缺陷，责任：本轮清单编辑者/技术文档）— 删除通配条目后 4 个现存脚本未入清单

- 位置：`file_manifest.txt`（rev 11 提交 `ed6b96d` 删除了 `evidence/software_verify_*_item0NN.py` 一行）
- 实际：该通配条目删除后，只逐个列出了 9 个 `evidence/software_verify_*_item0NN.py` 中的 5 个（item007/009/011/012/014）。以下 4 个**存在于工作区且已被 Git 跟踪**的脚本没有任何清单记录：
  1. `evidence/software_verify_db_migration_item006.py`
  2. `evidence/software_verify_upload_payload_item008.py`
  3. `evidence/software_verify_orchestration_item010.py`
  4. `evidence/software_verify_report_wording_item013.py`
- 交叉引用：这 4 个脚本被 `evidence/software_test.md` 以及 `evidence/android_package_check_20261111/gates.log`、`evidence/product_android_rebuild_20261111/gates.log`、`evidence/product_fix_20261111/gates.log` 等门禁日志引用，属仍在使用的证据工具，不是可丢弃文件。
- 影响：后续角色按 `file_manifest.txt` 复核 ITEM-006/008/010/013 时找不到脚本；违反“清单须同步新增/删除/重命名/用途变化文件”的共享仓库约定。rev 11 自查只验证了“清单内 1286 条非目录路径均存在”，未验证“现存受控文件是否都在清单内”，因此未发现该缺口。
- 修正建议：在 `file_manifest.txt` 增加上述 4 条独立条目（`path | active | 用途`），或恢复一个可核验的目录级条目；不要再用通配路径代替具体文件。

### O-4（编辑性观察，不单独构成失败）— 06 §5 的 `release_identity.txt` 归属可更明确

- 位置：`docs/06_维护排障与已知限制.md:131`
- 原文：`evidence/ev037/`、`evidence/ev036/`（含历史 `release_identity.txt`）
- 实际：`release_identity.txt` 只在 `evidence/ev036/` 存在，`ev037/` 没有。括号紧跟两个目录，读者可能误在 ev037 查找。
- 建议：写成 `evidence/ev036/release_identity.txt`，与同文件版本记录“EV-036 保留历史 `release_identity.txt`”一致。D-41 的“引用不存在文件”问题本身已消除。

### O-5（清单完整性观察，不单独构成失败）— `.bin` 本机留存标注不一致

- 位置：`file_manifest.txt` 中 ev028/ev033/ev034/ev035/ev036/ev037 的 `.bin` 条目
- 实际：rev 11 给 14 条改名条目补了“本机 `.bin` 留存（`.gitignore` 忽略，不随仓库分发）”，但同目录其余同样未被 Git 跟踪的 `.bin`（如 `evidence/ev033/cap02_k9.bin`、`evidence/ev034/cap01_k3_k6.bin`）没有该标注。路径本身都存在，不是 D-42 类错误。
- 建议：下一版统一标注，避免读者误以为所有 `.bin` 都随仓库分发。

---

## 4. 已核对一致的主要内容（含 rev 11 修正复核）

1. **D-41 已修正**：`docs/06:131` 不再引用不存在的 `evidence/ev038/release_identity.txt`；改为 `tool_results.txt`（含包/成员复算与下载日志）与 `capture_windows.txt`，二者均存在；EV-038 的 5 个 `.cap` 采集文件确已入库（Git 跟踪，md5 与清单逐项一致：`a5cc0252…`、`bd707221…`、`beac7054…`、`36b79433…`、`fe3aa6d2…`）。
2. **D-43 已修正**：`docs/06 §4` 新增“有数据时列表/删除”“当前候选真实 BLE/GATT/上传/401”“Android 回退”三行，状态均为“未执行”；分别与 `04 §8`、`README §5 #7`、`06 #9/#20`、`04 §2.4` 一致。
3. **D-42 已修正**：ev028/ev033/ev034/ev035/ev036/ev037 共 **14 条** `.cap` 条目已恢复为真实 `.bin` 路径并标注本机留存；22 个对应 `.bin` 全部在工作区存在，无残留 `.cap` 路径。
4. **O-1/O-2/O-3 已处理**：`docs/01` 与 `docs/04` 的源码一致性措辞改为“`dengbei_care/app` 与构建脚本相对 `dacf888` 零差异（随包说明书在之后更新了 APK 身份行）”，与实测 `git diff --stat dacf888..HEAD -- dengbei_care`（仅说明书 1 行）一致；`docs/03 §11` 改引 `EV-038 §2–§3`、`EV-036 §3–§4`；`docs/01:67` 补充 EV-038 的 7 周期。
5. **版本头一致**：01–06 与 README 全部为 `DOC-PKG-001 rev 11`；版本记录含 rev 11 条目；`README §6.2` 自查记录补 rev 11 行，并明确“尚未经独立文档校核复验”。
6. **路径引用**：对 7 份 docs 加随包用户手册运行路径检查，`checked=151 missing=0`。
7. **包/版本身份**：固件包 134,791 B、md5 `5df0c6d4…`、三成员摘要与 PACKAGE.txt 一致；APK 10,365,464 B、SHA-256 `71a041d3…`、MD5 `1e17e0c2…`，设备 `base.apk` md5 相同。
8. **实机入口/布局**：本轮重新确认「查看今日报告」「打开传感器列表」可点击并分别打开 `ReportActivity` / `SensorIdListFragment`；上滑后两个按钮 bounds 与文档引用完全一致；会话 logcat 未见 `FATAL EXCEPTION`/`ANR`。
9. **固件实板证据**：EV-038 解析输出确认上电五行有序、VDD 3287 mV、7 个 `S` 行（k=0/3/6/9/12/15/21）全 `q=1 V=0 D=0`，与 01/03/06 的表述一致。
10. **未执行披露**：无光判据 `T=0`、真实升温、光强改变、跨链路、平台写入/回读、功耗、回退、有数据删除、P-08～P-13 在 01/04/06/README 与用户手册中口径一致；整产品验收未重跑在 01/04/06/README 中一致声明。

---

## 5. 未执行项与限制（必须随交付披露）

| 项 | 状态 | 原因/边界 |
|---|---|---|
| `FW-LIGHT-DEF2-ENABLE` | **NOT EXECUTED（人工 SKIP_CONFIRMED，2026-11-11T03:50:42Z）** | 缺完全遮光窗口与同板同构建双态证据；不重测、不重复询问；`T=0` 保持 |
| 固件重新烧录 / COM43 采集 | 未执行 | 当前能力工具边界不含 `mdk_flash`/`serial_capture`；引用 EV-038/EV-037 原始证据 |
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

## 6. 给技术文档角色 / 清单维护者的可操作修正清单

1. **D-44（必须）**：在 `file_manifest.txt` 补回 `evidence/software_verify_db_migration_item006.py`、`..._upload_payload_item008.py`、`..._orchestration_item010.py`、`..._report_wording_item013.py` 四条独立条目（或改为可核验的目录级条目），保持 `path | status | purpose` 格式。
2. **O-4（建议）**：将 `docs/06:131` 的 `release_identity.txt` 明确写成 `evidence/ev036/release_identity.txt`。
3. **O-5（建议）**：统一未跟踪 `.bin` 条目的“本机留存、不随仓库分发”标注。
4. 修正后提升文档包版本（rev 12），只复核本轮修改与受影响步骤；不得复用本报告作为新版本通过证据。

---

## 7. 附件（本目录）

| 文件 | 内容 |
|---|---|
| `identity_checks.txt` | HEAD、固件包/成员/PACKAGE.txt、APK 与设备字节、设备版本、Activity 栈、UI bounds、logcat 结论 |
| `static_checks.txt` | rev 11 变更逐项核对、D-41/D-42/D-43/O-1～O-5、版本头、路径引用、清单统计 |
| `ui_my_page_live.txt` | 本轮「我的」页顶部 UI 文本/可点击性/bounds |
| `ui_my_scrolled_live.txt` | 本轮上滑后 UI 文本与「注册传感器」「网关与上传设置」bounds |
| `ui_sensor_list_live.txt` | 本轮「打开传感器列表」空态 UI（`已保存的 ID（0）`、`删除选中`） |
| `logcat_session_tail.txt` | 本轮会话 logcat 尾部（0 条 FATAL/ANR） |
| `check_doc_refs.py` | 可复跑的手册路径引用检查脚本（本轮 `151 checked / 0 missing`） |
| `check_manifest.py` | 可复跑的清单—工作区—Git 一致性检查脚本（本轮 `1286` 条非目录路径存在、4 条受控文件未入清单） |
| `extract_ui.py` / `filter_activity.py` | 可复跑的 UI 文本提取与 Activity 过滤脚本 |

**最终结论：TEST_FAIL。** rev 11 的文档侧修正（D-41、D-42、D-43、O-1～O-3）经静态与真机复核正确；唯一阻断项是共享清单在删除通配条目后遗漏 4 个现存受控脚本（D-44）。§5 的未执行项继续保持“未执行”，不得写成通过。
