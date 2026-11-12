# 安卓失败分派转交报告（ANDROID-ASSIGNMENT-REFERRAL-002）

- 负责角色：软件测试
- 能力：软件失败分析（`product_android_triage`）
- 分派对象：Android 当前候选实现（`artifacts/android_implementation.md`；包 `com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，debug；APK SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`）
- 原始失败：`evidence/product_acceptance_20261112/PRODUCT-ACCEPTANCE-002.md` **TEST_FAIL**（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）
- 本次调查：`evidence/product_acceptance_20261112/SOFTWARE-TRIAGE-002.md`（含对象身份、证据与责任边界）
- 分析时 HEAD：`b6fb1586006af9100e4508d707df4fff7a691dfb`，工作区干净

**转交结论：本次失败分派的 Android 实现对象在已执行范围内没有可修复的实测缺陷，返回 `ASSIGNMENT_DECLINED`（转交）。**

本报告不是修复成功，不是测试通过，也不表示其他对象已合格；原始 `TEST_FAIL` 与所有 `NOT EXECUTED` 项保持有效。未修改产品源码、测试期望、固件、文档、权限或环境配置。

---

## 1. 为什么不属本次 Android 返工

| 事实 | 原始证据 | 判读 |
|---|---|---|
| 前次 Android 实测缺陷 D-01/D-02/D-03/D-04a 在当前 APK `71a041d3…` 上逐项关闭 | `PRODUCT-ACCEPTANCE-002.md` §3.2 / §4 | 已关闭，不是本次返工对象 |
| 「查看今日报告」直达 `ReportActivity`；「打开传感器列表」进入 `SensorIdListFragment`；两控件 bounds 不相交；首页文案与实际入口一致 | 原始验收真实设备 Activity 栈与 UI 树 | 当前候选已执行范围成立 |
| 会话 logcat 尾部 5,667 行：0 条本应用 `FATAL EXCEPTION`、0 条 `ANR` | 原始验收 `logcat_session_tail.txt` | 无崩溃证据 |
| 报告原文：“本轮未发现新的软件实测缺陷。” | `PRODUCT-ACCEPTANCE-002.md` §4 / §9 | 没有可升级为返工的 Android 缺陷 |
| 主机候选 APK 大小/SHA-256/MD5 与设备在装包一致；Android 主源码自 `db53202` 无变化 | 本轮 `sha256sum`/`md5sum`/`stat`、`git diff db53202..HEAD -- dengbei_care/app/src/main`；原始验收 `device_apk_hash.txt` | 对象与版本闭合，不是旧包/旧源码结论 |
| 无光判据默认禁用：`LIGHT_DARK_THRESHOLD_CODE=0u`，`light.valid=false`、`dark=false` | 传感器固件 `USER/inc/sensor_config.h:128-129`、`USER/src/light.c:203-204` | 需求 4 无光分支属固件/需求上游，不是 Android |
| 上报判据与 3 分钟采样在传感器固件 | `USER/src/fw_core.c:109-125`、`USER/inc/sensor_config.h:26-27` | Android 不承担传感器门控，App 不补偿、不推断光照 |
| Android 主源码无光照/照度/无光判定逻辑 | `dengbei_care/app/src/main` 有界检索（仅 `DetailPresentation.kt:9` 注释） | App 没有可修改的无光判据 |

---

## 2. 剩余失败项的转交对象

| # | 项 | 当前状态 | 转交责任对象 | 待查事实/完成条件 |
|---|---|---|---|---|
| R-1 | 需求 4 “无光”分支（`FW-LIGHT-DEF2-ENABLE`） | `SKIP_CONFIRMED` / NOT EXECUTED；`T=0` | **固件/需求上游**（判据接受与条件性实现） | 先形成判据形制/数值接受记录，并取得同板同构建“有光/完全遮光”双态原始 `o/n/x/a`；之后由固件实现回填阈值、置启用并重建。不得用暗态单组或估算值回填，不得重复询问已确认跳过的同一测项 |
| R-2 | 真实升温 >0.9 ℃ / 稳定 >35 ℃ 触发与 433 实发 | NOT EXECUTED | **用户/现场 + 嵌入式测试** | 用户当前不能调温；具备安全调温条件后按 `docs/03` 判读，记录 `r/s/y` 与 `P` |
| R-3 | 真实光强双态 | NOT EXECUTED（人工确认） | **用户/现场 + 嵌入式测试** | 提供完全遮光窗口（遮住 J4 受光路径，≥2 个采样周期）与有光参考；记录遮光方式、时间、原始 `o/n/x/a` |
| R-4 | CR2032 峰值/深睡电流（`E2E-04`） | NOT EXECUTED（已批准延期） | **硬件测试** | 具备电流表/SMU/示波器与量程/接线记录；用户明确当前不能测功耗 |
| R-5 | 433→网关→BLE→手机同笔一致性 | NOT EXECUTED | **后续联调阶段（网关/手机/测试）** | 网关供电/身份/配对；手机绑定传感器；同一时间窗记录同一笔 ID/温湿度 |
| R-6 | `/upload_data` 真实写入、幂等、回读 | NOT EXECUTED | **后端/联调（按授权）** | 平台写接口/数据库授权；服务端 `UNIQUE(dev_id,time)` 与 `INSERT IGNORE` 的实际行为；2xx 不等于持久化 |
| R-7 | 真实 BLE/GATT/MTU、告警、上传状态、数据展示 | NOT EXECUTED | **后续联调阶段（Android 测试 + 网关）** | 需网关与绑定数据；当前不判定 Android 通过或缺陷 |
| R-8 | 注册/登录、网关编码保存、真实告警参数下发 | NOT EXECUTED | **联调/后端（按授权）** | 需账号/网关编码等外部数据；不写入平台、不改用户数据 |
| R-9 | 有真实绑定数据时的传感器列表列出/删除 | NOT EXECUTED | **后续联调阶段（Android 测试）** | 需真实绑定数据；当前仅有空态入口与删除控件证据 |
| R-10 | API 31+ 权限分支、9 台真实快照、第 10 台拒绝 | NOT EXECUTED | **后续 Android 测试（设备条件具备时）** | 需 API 31+ 真机与真实多设备源；宿主机矩阵不替代真机 |
| R-11 | 文档旧表述“整产品验收尚未重跑” | 观察项 | **技术文档** | `docs/README.md:81`、`docs/06:122` 下次修订更新并引用 `PRODUCT-ACCEPTANCE-002`；只复核受影响条目 |
| R-12 | iOS | 不在本轮范围 | **不在本次分派** | 用户明确暂不修改 iOS |

---

## 3. 竞争归因与待查事实（不作已证实根因）

| 假设 | 当前状态 | 待查事实/最小实验 |
|---|---|---|
| H1 Android 仍存在未发现的实测缺陷 | **无证据**；已执行范围无缺陷，未执行项不是缺陷 | 真实 BLE/上传/注册/列表/API 31+ 条件具备后逐项执行；不得用宿主机结论替代 |
| H2 `T=0` 是已确认固件缺陷 | **未证实**；交付默认禁用，待上游判据接受与双态证据 | 判据形制/数值接受记录；同板同构建双态原始 `o/n/x/a`；满足后才谈回填与重建 |
| H3 无线链路/平台故障 | **未证实**；未连接、未写入 | 网关供电/配对、手机绑定、平台授权后的同窗实测与回读 |
| H4 环境光/光路/分压电气异常 | **未证实**；ADC 原始值不作好坏判据 | 有光/遮光双态与硬件侧光路/分压电气测量（硬件测试） |
| H5 文档旧表述导致功能失败 | **不成立** | 文档时效问题；不影响 Android 行为 |

> 因果未经验证前保留为候选；不把“未执行”“缺条件”或“推测”写成根因，也不据此要求任何对象返工。

---

## 4. 边界声明

1. 本报告只转交失败归属与待查事实，**不替代实现、重建或原目标独立复测**。
2. Android 对象在已执行范围内不需要返工；其未执行功能保持 **NOT EXECUTED**，须在条件具备时由测试能力按原验收标准执行。
3. 需求 4 无光分支的 `T=0` 当前不是已确认缺陷；上游判据接受与双态证据齐备后，才由固件实现进行条件性回填与重建，再由原受影响测试复验。
4. 原始 `TEST_FAIL` 仍然有效；本转交不表示产品通过、不表示外部条件已具备、不表示任何未执行项已通过。
5. 本轮未修改产品源码、测试期望、固件、文档、权限或环境配置；未安装/卸载 App、未烧录/复位、未访问平台写入接口或数据库。
