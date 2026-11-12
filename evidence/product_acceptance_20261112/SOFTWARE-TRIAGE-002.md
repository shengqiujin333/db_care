# 安卓对象失败归因（SOFTWARE-TRIAGE-002）

- 负责角色：软件测试
- 能力：软件失败分析（`product_android_triage`）
- 分析对象：`artifacts/android_implementation.md`（Android 当前候选实现；APK `71a041d3…`，`com.jinyuni.dengbei_care` versionCode 10 / versionName 1.9 debug）
- 原始失败：`evidence/product_acceptance_20261112/PRODUCT-ACCEPTANCE-002.md` **TEST_FAIL**（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）
- 代码索引：`artifacts/codegraph_android_index.md`（projectPath `D:\mypro\beiwo2\AIP\dengbei_care`；索引基线 `9b3df8d`）
- 分析时点：2026-11-12 12:01–12:10（Asia/Shanghai；设备工具时间为 UTC）
- 分析时 HEAD：`b6fb1586006af9100e4508d707df4fff7a691dfb`，工作区干净
- **本轮结论：`ASSIGNMENT_DECLINED`（本次分派的 Android 实现对象在已执行范围内没有可修复的实测缺陷；剩余失败项按责任边界转交固件/需求、现场与联调、硬件测试与文档，并保留 Android 自身未执行项为 NOT EXECUTED）**

> 本文件只做失败归因与转交，不是复验通过，不是修复成功，也不代表产品合格。
> 本轮未修改产品源码、测试期望、固件、文档、权限或环境配置；未安装/卸载 App、未清数据、未烧录/复位、未访问平台写入接口或数据库。
> 本轮没有重跑整产品验收；对 Android 已执行范围的结论引用 `PRODUCT-ACCEPTANCE-002` 的已验证原始证据，并对对象身份做了独立闭合（§3）。未执行项保持未执行，不写成通过；`TEST_FAIL` 仍然有效。

---

## 1. 结论摘要

1. **原始失败的直接原因已被失败证据明确记录为外部条件未执行，而不是已测出的 Android 软件缺陷**。`PRODUCT-ACCEPTANCE-002` 对当前候选 APK `71a041d3…` 的结论是：前次实测缺陷 **D-01/D-02/D-03/D-04a 已逐项重新实测关闭**，本轮“未发现新的软件实测缺陷”；整体仍为 `TEST_FAIL`，因为需求 4 的无光分支仍按交付默认禁用，且真实升温/超温、光强双态、功耗、网关→手机跨链路、`/upload_data` 写入回读等必需项未执行（§4 / §5 / §6 逐项列出）。
2. **本次分派的对象（Android 实现）不是当前剩余失败的可修复对象**：
   - Android App 不承担光照判定，也没有光照/照度字段或补偿逻辑；无光分支完全由传感器固件 `light.c` / `fw_core.c` 的判据决定，App 只展示规则原文（§3.3）。
   - 剩余未执行项的阻塞条件是物理刺激（温度/光强）、仪器（功耗）、外部数据与授权（网关、手机绑定、平台写入）或用户明确安排的后续联调阶段；这些不是 Android 源码可修改的对象（§5）。
   - 需求 4 的无光判据默认值为 `LIGHT_DARK_THRESHOLD_CODE=0`，属固件/需求上游“判据形制与数值接受 + 同板同构建双态证据”的待决事项；本能力不把它升级为 Android 返工，也不把它写成固件缺陷（§5.1）。
3. **D-01～D-04a 的关闭结论成立且对象身份闭合**：本轮独立复算主机候选 APK 为 10,365,464 B、SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`，与原始失败证据中的候选及设备在装包一致；`git diff db53202..HEAD -- dengbei_care/app/src/main` 为空（§3.1 / §3.2）。
4. **本轮没有发现可归因于 Android 的新缺陷**；也没有把“未执行”当作“证据排除”。真实 BLE/GATT/MTU、真实 `/upload_data`、注册/登录、有数据时的传感器列表、API 31+ 权限分支、9 台真实快照与第 10 台拒绝等 Android 侧行为仍是 **NOT EXECUTED**，须在具备外部条件的联调阶段执行，不能写成通过，也不能写成已确认缺陷（§5.6 / §6）。
5. **不采用 `EXTERNAL_CONDITION_MISSING` 回原测试节点重复询问**：无光测项 `FW-LIGHT-DEF2-ENABLE` 已由人工确认 `SKIP_CONFIRMED`（2026-11-11T03:50:42Z，「无法准备，先跳过」），当前工作流明确不得重复排队同一测项；温度/光强/功耗受用户明确条件限制，跨链路已由用户安排到后续联调阶段。把同一缺口再次交给原测试节点不会产生新的可执行条件。
6. **不采用 `REPAIR_REQUIRED`**：没有实测证据证明当前 Android 对象需要修复；推测、外部条件缺失或根因未明都不能升级为返工。
7. **不采用 `RESPONSIBILITY_UNRESOLVED`**：责任边界不是“未定”，而是“已确定为不在本次 Android 返工范围”，可逐项转交（§7）。未知根因本应继续调查；本轮已把可调查到的对象、版本、接口、日志与条件缺口逐项关联，剩余是未执行而非未定位。
8. **结果：`ASSIGNMENT_DECLINED`**，附 `ANDROID-ASSIGNMENT-REFERRAL-002.md`。这不是修复完成、不是测试通过，也不表示其他对象已被验证合格。

---

## 2. 原始失败引用与对象身份

### 2.1 原始失败（引用，不改写）

来源：`evidence/product_acceptance_20261112/PRODUCT-ACCEPTANCE-002.md`（负责角色：软件测试；能力：产品整体功能验收）。

| 项 | 原文事实 |
|---|---|
| 验收对象 | 传感器固件 FD-002 rev 8 发布包、安卓 1.9 debug 候选 AB-002、既有网关与平台服务、文档包 DOC-PKG-001 rev 12 |
| 结论 | `TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`） |
| Android 实测关闭项 | D-01「查看今日报告」直达 `ReportActivity`；D-02「打开传感器列表」进入 `SensorIdListFragment`；D-03 两控件 bounds 不相交；D-04a 首页文案与实际入口一致 |
| 本轮 “新缺陷” | 报告原文：“本轮未发现新的软件实测缺陷。” |
| 整体未通过原因 | 需求 4 无光分支仍禁用（`FW-LIGHT-DEF2-ENABLE` 为人工 `SKIP_CONFIRMED`，NOT EXECUTED）；真实升温/超温、真实光强双态、功耗、网关→手机跨链路、`/upload_data` 写入回读均未执行；上电五行为人工 REASSESS 后的复用复核，非本轮新采集 |
| 观察项（非阻断） | `docs/README.md:81`、`docs/06:122` 仍写“整产品验收尚未重跑”，报告产生后已过时；属技术文档修订 |

### 2.2 分析对象与版本（本轮独立核对）

| 项 | 值 | 本轮核对方式 |
|---|---|---|
| 分析对象 | `artifacts/android_implementation.md`（Android 当前候选实现记录） | 阅读 |
| 包名/版本 | `com.jinyuni.dengbei_care`，`versionCode=10`，`versionName=1.9`，debug | 原始验收证据 §1；设备在线 |
| 主机候选 APK | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk`，10,365,464 B，SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`，MD5 `1e17e0c281715fddd26d308246d6424c` | **本轮本机 `sha256sum`/`md5sum`/`stat` 复算一致** |
| 设备在装 APK | `/data/app/com.jinyuni.dengbei_care-…/base.apk`，大小与 SHA-256/MD5 **与主机候选逐字节一致** | 原始验收证据 `device_apk_hash.txt`（设备侧 `stat`/`sha256sum`/`md5sum`）；本轮未重新安装 |
| 设备 | Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506`，本轮在线 | **本轮 `android_test devices` 复核** |
| Android 产品源 | 最后一次 `app/src/main` 变更提交 `dacf888`；`git diff db53202..HEAD -- dengbei_care/app/src/main` **为空** | **本轮 `git log` / `git diff` 复核** |
| 当前 HEAD | `b6fb158`（原始验收证据提交）；工作区干净 | **本轮 `git rev-parse` / `git status` 复核** |
| 代码索引 | `artifacts/codegraph_android_index.md`，索引基线 `9b3df8d`，projectPath `dengbei_care` | 阅读；本轮用其确认目录与入口，未用图结果替代直接源码核对 |

**身份闭合**：主机候选 APK、设备在装包（原始验收设备侧哈希）与 `android_implementation.md` 交付记录三者一致；Android 主源码自原始验收时点未变化。因此，原始失败中的 Android 实测结论对应当前同一候选，不是旧包或旧源码的结论。

---

## 3. 本轮获准的最小调查（真实对象与源码）

### 3.1 对象身份复核（第一手）

```text
HEAD   b6fb1586006af9100e4508d707df4fff7a691dfb
status (clean)
APK    10,365,464 bytes
       71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8  app-debug.apk
       1e17e0c281715fddd26d308246d6424c                              app-debug.apk
last main-source commit: dacf888
git diff --stat db53202..HEAD -- dengbei_care/app/src/main: (empty)
android_test devices: 12f046690506  device  product:olivelite model:Redmi_8A
```

结论：原始失败针对的 Android 候选、设备在装包与当前交付对象一致；本轮没有替换包、降级或改源码。

### 3.2 原始失败中 Android 已执行范围的证据（引用已验证证据）

原始失败证据在真实设备上记录了以下第一手结果，且与当前候选身份闭合：

- 设备在线，`versionCode 10 / versionName 1.9`，debug；设备 `base.apk` 与候选逐字节一致。
- 「查看今日报告」点击后 `mResumedActivity=com.jinyuni.dengbei_care/.report.ReportActivity`。
- 「打开传感器列表」点击后进入 `sensorIdListRoot` / `recyclerSensorIds`。
- 首页「去添加」同样进入“已保存的 ID（0）”列表页。
- 「注册传感器」与「网关与上传设置」bounds 不相交（间隔 48 px）。
- 会话 logcat 尾部 5,667 行：0 条本应用 `FATAL EXCEPTION`、0 条 `ANR in com.jinyuni…`。

这些是真实设备证据，不是 mock 或宿主机推演。本 triage 不重复执行同一批点击，避免重复已完成工作；其结论用于确认“当前 Android 候选在已执行范围内没有与原始失败对应的缺陷”。

### 3.3 源码级责任边界（本轮直接读取）

| 事实 | 位置 | 对归因的意义 |
|---|---|---|
| 光判据默认禁用：`#define LIGHT_DARK_THRESHOLD_CODE 0u /* 0 = 未启用 (唯一禁用值) */` | 传感器固件 `USER/inc/sensor_config.h:128-129` | 需求 4 的“无光”分支由固件默认值关闭，不在 Android 实现内 |
| `r.valid = r.adc_ok && (LIGHT_DARK_THRESHOLD_CODE != 0); r.dark = light_is_dark(...)` | 传感器固件 `USER/src/light.c:203-204` | `T=0` 时 `valid=false`、`dark=false`；“无光”分支永不成立。这是固件判据状态，不是 App 缺陷 |
| 上报规则：`(cur > 35.0C) 或 (有前值 且 (cur - prev) > 0.9C 且 dark)` | 传感器固件 `USER/src/fw_core.c:109-125` | 3 分钟采样与上报判据在传感器固件，Android 只接收/展示/上传 |
| 采样节拍：`SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3` | 传感器固件 `USER/inc/sensor_config.h:26-27` | 采集周期是固件职责；原始验收 R3 已在实板测得 `k` 步长 3 |
| Android 主源码无光照/照度/无光判定逻辑；仅 `ui/detail/DetailPresentation.kt:9` 的注释说明“界面不显示环境光状态或照度数值” | `dengbei_care/app/src/main` 有界检索 | App 没有可修改的“无光”判据；它不补偿、不推断光照，也不应被要求替代固件判据 |
| 3 分钟/条件上报是展示文案与接收时序，不是 App 侧判据 | `ui/detail/DetailPresentation.kt`、`telemetry/SensorCadence.kt` | 修改 App 不能启用传感器无光分支，也不能制造真实升温/超温 |

### 3.4 本轮未做的事（如实登记）

- 未重跑整产品 UI 验收、未重新安装 APK、未清数据、未改系统设置。
- 未采集 COM43（引用原始验收的实板周期），未烧录/复位，未使用 mdk_flash 或 pyocd。
- 未调温、未调光、未遮光、未测功耗；用户明确当前不能进行这些操作。
- 未连接平台、未调用 `/upload_data`、未连数据库。
- 未读取或修改 `runtime/permissions.yaml` / `runtime/environment.yaml`。
- 未修改产品源码、测试期望、固件、文档或他人证据；仅新增本归因与转交文件并维护清单。

---

## 4. Android 已执行范围的逐项判读

| Android 相关项 | 原始失败记录 | 本 triage 判读 |
|---|---|---|
| D-01 报告入口 | 当前候选点击直达 `ReportActivity` | **已关闭**；无 Android 返工依据 |
| D-02 传感器列表入口 | 当前候选进入 `SensorIdListFragment` | **已关闭**；无 Android 返工依据 |
| D-03 控件重叠 | `[32,976][688,1072]` 与 `[32,1120][688,1216]` 不相交 | **已关闭**；无 Android 返工依据 |
| D-04a 首页文案 | 文案与实际路径一致 | **已关闭**；无 Android 返工依据 |
| R6 App 基本可用 | 首页/我的/报告/列表/网关对话框可用、无崩溃 | **在已执行范围内成立**；不等于真实 BLE/上传链路通过 |
| 真实 BLE/GATT/MTU、数据展示、告警、上传状态 | 未执行（无网关、无已绑定传感器） | **NOT EXECUTED**；不是已确认缺陷 |
| 注册/登录、网关编码保存、真实告警参数下发 | 未执行（无账号/网关编码等外部数据；不写平台） | **NOT EXECUTED**；不是已确认缺陷 |
| 有真实绑定数据时的传感器列表列出/删除 | 未执行（当前 0 条绑定） | **NOT EXECUTED**；不是已确认缺陷 |
| API 31+ 权限分支、9 台真实快照、第 10 台拒绝 | 未执行（设备 API 29；无真实多设备源） | **NOT EXECUTED**；不是已确认缺陷 |
| `/upload_data` 真实写入、幂等与回读 | 未执行（无写接口/数据库授权） | **NOT EXECUTED**；不是已确认缺陷 |
| 文档“整产品验收尚未重跑”表述 | 报告产生后已过时 | 属技术文档修订；不是 Android 缺陷 |

> 上表区分“已执行并通过的 Android 范围”与“未执行的 Android 范围”。未执行不等于通过，也不等于缺陷；不得为了关闭本 triage 而把未执行项写成 PASS。

---

## 5. 剩余失败项的责任归属（逐项）

### 5.1 需求 4 无光分支（固件/需求，待判据接受与双态证据）

| 项 | 事实 | 责任与边界 |
|---|---|---|
| `FW-LIGHT-DEF2-ENABLE` | 人工确认 **`SKIP_CONFIRMED`**（2026-11-11T03:50:42Z，「无法准备，先跳过」），NOT EXECUTED；`LIGHT_DARK_THRESHOLD_CODE=0`，`light.valid=false`、`dark=false` | **不是 Android 实现缺陷**。上游需先形成判据形制/数值接受记录，并在同板同构建上取得“有光/完全遮光”双态证据，之后由固件实现回填阈值、置启用并重建；本 triage 不重复询问该已确认跳过的测项，也不把 `T=0` 升级为固件返工 |
| 无光分支的实际表现 | 原始验收 COM43 两周期 `V=0 D=0`，`r=0 s=0 y=0`，无 `P` | 与“判据未启用”一致；**不得**把 `V=0 D=0` 解释为“已识别有光”，也不得据此判 App 或固件故障 |

### 5.2 真实升温/超温触发（用户/现场 + 嵌入式测试）

- 用户明确“当前不能调节温度”，不得安排调温测试；原始验收窗口内无 `P`/`r=1`/`s=1`。
- 该条件属于传感器固件与 433 实发路径的验证条件，不是 Android 可修改对象；App 不重算传感器门控，不合成样本。
- 状态：**NOT EXECUTED**；不是已确认缺陷。

### 5.3 真实光强双态（用户/现场 + 嵌入式测试）

- 用户明确“当前不能改变光强”；同一测项已人工 `SKIP_CONFIRMED`。
- 需要完全遮光窗口（遮住 J4 受光路径，≥2 个采样周期）与有光参考，记录原始 `o/n/x/a`；之后才谈固件阈值回填。
- 状态：**NOT EXECUTED（人工确认）**；不是 Android 缺陷；本工作流不得重复排队同一测项。

### 5.4 功耗（硬件测试）

- 用户明确“不能测功耗”；原始验收已把 `E2E-04` 记为已批准延期，且未枚举到电流表/SMU/示波器。
- 属硬件测试职责；Android 与固件实现都不能替代仪器测量。
- 状态：**NOT EXECUTED**。

### 5.5 网关→手机跨链路与平台写入（后续联调 + 授权）

| 项 | 状态 | 责任/缺口 |
|---|---|---|
| 433→网关→BLE→手机同笔一致性 | 未执行 | 用户安排后续联调阶段；网关供电/身份/配对未验证，手机未配置网关地址、无绑定传感器 |
| 真实 BLE/GATT/MTU、告警、上传状态 | 未执行 | 同上；属联调阶段，具备网关与绑定数据后由测试能力执行 |
| `/upload_data` 写入、幂等、回读 | 未执行 | 当前能力无平台写/数据库授权；平台只读 SSH 入口对本能力不可用；属后端/联调按授权执行 |
| 注册/登录、网关编码保存、告警参数下发 | 未执行 | 无账号/网关编码等外部数据；不写入平台、不改用户数据 |
| 平台服务是否一致 | 仅静态一致 | 候选 `CloudConfig` 指向既有 `8.140.23.253:8883/5000`；不等价于服务可用或写入成功 |

这些项不是 Android 源码可修复的对象：Android 侧代码路径存在但未被执行，**没有被证明为缺陷**。将其写成 `REPAIR_REQUIRED` 会把“缺条件”误升级为“实现错误”。

### 5.6 Android 侧未执行项

真实 BLE/GATT/MTU、真实 `/upload_data`、注册/登录、有数据时的传感器列表、API 31+ 权限分支、9 台真实快照与第 10 台拒绝等 Android 行为，只能在具备网关、绑定数据、平台授权或 API 31+ 设备时执行。当前状态为 **NOT EXECUTED**；本轮不据此判定 Android 通过或失败。

---

## 6. 竞争假设与证据判断

| # | 假设 | 判断 | 依据 |
|---|---|---|---|
| H1 | 当前 Android 候选仍存在原始失败中的 D-01～D-04a 缺陷 | **不成立** | 原始验收在当前 APK `71a041d3…` 上逐项实测关闭；对象身份本轮闭合（APK hash、源码 diff、设备在线） |
| H2 | D-01～D-04a 的关闭是点击工具/选择器造成的假象 | **不成立** | 原始验收使用 Activity 栈与 UI 树留证，两个入口均到达正确 Activity/Fragment；无崩溃 |
| H3 | Android 应负责“无光”判定，App 未实现或实现错误 | **不成立** | App 主源码无光照/照度/无光判定逻辑；判据完全在固件 `light.c`/`fw_core.c`；默认 `T=0` 使分支关闭 |
| H4 | 3 分钟采样或上传规则是 Android 实现职责 | **不成立** | 采样常量与上报判据在传感器固件；Android 只接收、展示、入队上传；R3 已在实板测得 3 分钟节拍 |
| H5 | 剩余未执行项只是“现场可以立即准备”的外部条件，回原测试节点询问即可 | **不成立（不足以作为本次路径）** | 无光测项已人工 `SKIP_CONFIRMED` 且不得重复排队；温度/光强/功耗受用户明确限制；跨链路已安排后续联调；平台写入无授权 |
| H6 | `LIGHT_DARK_THRESHOLD_CODE=0` 是已确认的固件缺陷，应立即返工 | **不成立（未证实）** | 它是交付时的有意禁用默认值，待上游判据接受与双态证据；人工已确认跳过；没有实测缺陷签名。不能把“未启用”猜测升级为返工 |
| H7 | Android 的未执行功能（BLE/上传/注册等）已被证明有缺陷 | **不成立** | 没有执行证据，也没有失败签名；只能记为 NOT EXECUTED。缺乏证据不等于缺陷，也不等于排除 |
| H8 | 平台/服务端已被证明故障 | **不成立（未证实）** | 本轮未连接、未写入、未连库；仅有配置静态一致。服务端行为需联调/授权后验证 |
| H9 | 文档旧表述导致产品功能失败 | **不成立** | `docs/README.md:81`、`docs/06:122` 是文档时效问题，报告已明确交技术文档，不影响 Android 功能 |

**证据排除与缺乏证据的区分**：

- **已被证据排除（针对 Android）**：D-01/D-02/D-03/D-04a；Android 侧光照判据责任；Android 侧采样节拍/上报判据责任；已执行 UI 路径的点击工具假象；候选/设备/源码版本不一致。
- **仍缺乏证据（未执行，不是缺陷）**：真实 BLE/GATT/MTU、真实上传与平台回读、注册/登录、有数据时的列表、API 31+、9 台真实快照、真实升温/超温、光强双态、功耗、网关跨链路。
- **未确定为缺陷（保留未知）**：`T=0` 的启用时机与阈值取值、平台服务实际可用性、网关身份与配对状态、光敏器件/光路电气特性。它们需要外部条件或上游决定，不能由本能力写成已证实根因。

---

## 7. 责任边界与结果判断

| 对象 | 判断 |
|---|---|
| Android 实现（本次分派对象） | **已执行范围内无实测缺陷，不构成本次返工对象**；D-01～D-04a 已关闭。未执行的 Android 功能保持 NOT EXECUTED，交后续联调 |
| 传感器固件/需求上游 | 需求 4 无光分支 `T=0` 待“判据形制/数值接受 + 同板同构建双态证据”；满足后由固件实现回填并重建。**当前不升级为返工** |
| 用户/现场 + 嵌入式测试 | 真实升温/超温、光强双态、遮光窗口；用户当前不能准备，测项保持未执行 |
| 硬件测试 | 功耗与光路/分压电气特性；无仪器，保持未执行 |
| 联调阶段（网关/手机/平台） | 433→网关→BLE→手机同笔一致性、真实 BLE、`/upload_data` 写入回读；需网关供电/配对、绑定数据与平台授权 |
| 技术文档 | `docs/README.md:81`、`docs/06:122` 旧表述待下次修订更新；不影响 Android 功能 |
| 责任是否未定 | **否**。Android 无返工依据；其余对象的缺口与前置条件已逐项列出 |
| 外部条件是否可立即补齐 | **否（当前）**。无光项已人工跳过；温度/光强/功耗受用户限制；联调按用户安排在后续阶段 |
| 结果码 | **`ASSIGNMENT_DECLINED`**（附 `ANDROID-ASSIGNMENT-REFERRAL-002.md`） |
| 不等于 | 不等于 Android 全部功能通过、不等于产品验收通过、不等于其他对象无问题；原始 `TEST_FAIL` 仍然有效 |

---

## 8. 复验触发条件（供责任方与后续测试）

只有满足以下条件，才需要对相应对象重新执行原受影响测试；本 triage 不授权跳过、不改变验收标准：

| 项 | 触发条件 | 复验目标 |
|---|---|---|
| 需求 4 无光分支 | 上游形成判据形制/数值接受记录；同板同构建取得有光/完全遮光双态原始 `o/n/x/a`；固件完成条件性回填并重建 | 原产品验收按 `FW-LIGHT-DEF2-ENABLE` 重新判定；不得把历史 `V=0 D=0` 当通过 |
| 真实升温/超温 | 用户/现场具备安全调温条件 | 传感器固件触发、433 实发与 App 展示/上传一致性 |
| 光强双态 | 用户/现场提供完全遮光与有光窗口 | 判据启用后的分类与上报 |
| 功耗 | 具备电流表/SMU/示波器 | `E2E-04` 峰值/深睡电流 |
| 网关/手机/平台 | 网关供电与配对、手机绑定传感器、平台写/回读授权 | 跨链路同笔一致性、真实 BLE、`/upload_data` 幂等与回读 |
| Android 未执行项 | 上述联调条件具备 | 真实 BLE/GATT/MTU、注册/登录、有数据列表、API 31+、9 台快照与第 10 台拒绝 |

---

## 9. 未执行项与证据局限

1. 本轮未重跑整产品验收；Android 已执行范围的结论来自 `PRODUCT-ACCEPTANCE-002` 的已验证真实设备证据，本轮只独立闭合对象身份（APK hash、源码 diff、设备在线）。
2. 未重新安装/启动 App；未复测 UI 点击；未读取本轮新的 UI 树或截图。
3. 未采集 COM43、未烧录/复位、未使用逻辑分析仪；固件侧判读来自直接源码与原始验收的串口事实。
4. 未调温、未调光、未遮光、未测功耗；这些是用户明确限制或人工已确认跳过的条件，不安排相关硬件测试。
5. 未连接平台、未调用写接口、未连数据库、未执行只读 SSH；平台结论仅为静态配置一致，不是服务可用性结论。
6. `LIGHT_DARK_THRESHOLD_CODE=0` 的启用时机、阈值数值、判据形制接受记录仍未形成；不能由本能力替代上游决定。
7. Android 真实 BLE/上传/注册/列表/API 31+/9 台等行为仍未执行；不能写成通过或缺陷。
8. 板上镜像哈希不可从板端回读；原始验收的固件身份属“下载记录 + 源码零差异 + `k` 连续”闭合推断，本 triage 不重新证明板端镜像，也不据此扩大结论。

---

## 10. 本轮实际改动

- 新增：`evidence/product_acceptance_20261112/SOFTWARE-TRIAGE-002.md`（本文件）。
- 新增：`evidence/product_acceptance_20261112/ANDROID-ASSIGNMENT-REFERRAL-002.md`（分派转交报告）。
- 更新：`file_manifest.txt`（登记上述两个文件；不修改既有 rev11/rev12 文档行与既有证据条目）。
- **未修改**：`dengbei_care/**` 产品源码与资源、`app/src/test/**`、固件与 Keil 工程、`docs/**`、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载 App、未烧录/复位、未改设备或平台数据。
