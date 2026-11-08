# 软件测试设计（TD-SW-003）

状态：软件测试设计产物（`software_tester.software_test_design`）
版本：2.0（替代 TD-SW-002 v1.0）
对象：**Android App（`dengbei_care`，Kotlin）** 当前候选，含本轮四项增量（G-1 BLE 权限模型、G-2 网关身份发现与绑定、G-3 BLE 批次转发与零值哨兵、G-4 上报规则文案方向）以及完整交付回归。
只做测试设计（范围、环境、方法、刺激、观测、判据、证据、判定规则），**不执行测试、不修改被测 App 源码、不做固件/硬件/服务器端改动**。执行与逐项判定由后续软件测试执行/验证能力按本设计承接。

上游输入：
- `readme.txt`（权威需求，八项修改点）
- `artifacts/android_architecture.md`（AA-003 rev 2.0，本能力 `subject`）
- `artifacts/android_tasks.yaml`（本轮四项任务与验收条件，`tasks` 输入）
- `artifacts/android_feasibility.md`（AF-001：Redmi 8A / API 29 只读事实、F-1 权限门、F-2 网关身份）
- `artifacts/interface_contract.md`（IC-002 v3.0：升温 >0.9℃、严格 >35.0℃、BLE 记录布局、MQTT 批量格式、§7 Android 同步、D-03）
- `artifacts/firmware_requirements.md` / `artifacts/firmware_design.md`（传感器侧采样与上报语义，用于区分职责边界）
- `artifacts/test_design.md`（TD-002，嵌入式侧联测边界）
- as-built 源码 `dengbei_care/`（本文件 §3 逐条核对）
- 本项目统一环境与授权（`runtime/environment.yaml` / `runtime/permissions.yaml` 的本次合成视图）

**本版结论**：`DESIGN_READY`。四项增量均有可在宿主机独立断言的纯逻辑目标与可在真机独立观测的接线目标，完整交付有明确的分层用例与最终判定规则。当前环境缺口（真机测试工具未对本能力开放、网关未上电/身份未确认、手机无网络、服务入口未核验）已按 §8/§9 记录为**未满足的执行前置**，不改变用例目标，也不把未执行项写成通过。

---

## 1. 本版变更（相对 TD-SW-002 v1.0）

| 变更 | 内容 |
|---|---|
| 对象更新 | 由 AA-002 的 14 项实现改为 AA-003 rev 2.0 的 **G-1…G-4 四项增量 + 既有 as-built 完整回归** |
| 新增纯逻辑测试面 | `ble/BlePermissionPolicy`、`protocol/GatewayIdentity`、`telemetry/BleBatchForwarding` 的宿主机矩阵与边界 |
| 新增真机测试面 | API 29 无定位授权直连、API 31+ 运行时权限拒绝/授予/撤销、有界扫描与 `stopScan`、候选呈现与保存、多设备批次发布、首页链路状态 |
| 修正方向 | 规则文案由“较上次**下降**超过 0.9℃”更正为“较上次**升高**超过 0.9℃”（readme 点 4、IC-002 §2、AA-003 G-4/D-22）；TD-SW-002 的 T-SW-L3-04 等旧表述作废 |
| 明确判定 | 新增 §10：任一必需项 FAIL / NOT EXECUTED / CONDITIONAL / 原因未知 / 环境缺口均不得判为整体通过 |
| 保留 | TD-SW-002 中仍适用的 as-built 用例编号（L0-01…L0-14、L0d-01…L0d-07、L1-01…L1-09、L2-01…L2-15、L3-01…L3-08、REG-01…REG-06）继续有效，本版不重复展开其细节，仅在本文件 §6.2/§6.4/§6.5/§6.6/§6.7 给出适用性与新增补充 |

---

## 2. 需求与验收追踪矩阵

“必需”列定义本轮**完整交付**判定中的必需项；“证据类别”列区分宿主机（H）、静态（S）、真机软件（D）、真服务（V）。任何 D/V 项不得以 H/S 证据替代。

| ID | 来源 | 要求/验收 | 用例 | 证据类别 | 必需 |
|---|---|---|---|---|---|
| R-01 | readme 1 | GXHT40 替换后单位/字节序/负温不变 | T-SW-L0-01..03、REG-02 | H/D | 是 |
| R-02 | readme 2 | 光照不上空口/BLE，App 不推断、不显示照度或光照状态（规则原文“无光”除外） | T-SW-L1-08、L3-12 | H/S/D | 是 |
| R-03 | readme 3 | 采样 180 s；App 读取节拍 180 s；无上报属正常 | T-SW-L0-14、L2-32、L3-04/05/07 | H/S/D | 是 |
| R-04 | readme 4 | 条件上报：本次较上次**升高 >0.9℃ 且无光**，或**>35.0℃**；App 不重算门控、不合成样本 | T-SW-L0-25、L1-14/20、L2-29/32、L3-12 | H/S/D | 是 |
| R-05 | readme 5 | broker `ssl://8.140.23.253:8883`、HTTP `http://8.140.23.253:5000` 与既有 TLS/认证不变 | T-SW-L1-23、L2-31 | S/D/V | 是 |
| R-06 | readme 6 | 只改 Android；不改 iOS；帧/服务/schema 保持 | T-SW-L1-18/24 | S | 是 |
| R-07 | readme 7 | 无光阈值取全暗 1/3 | 嵌入式/硬件责任（TD-002）；App 仅不推断 | —（边界） | 否（非软件项） |
| R-08 | readme 8 | UART1 调试通道 | 嵌入式责任（TD-002） | —（边界） | 否（非软件项） |
| G-1 | AA-003 G-1 / D-16 / D-19 | API 26–30 直连 GATT 不再要求 `ACCESS_FINE_LOCATION`；API 31+ 直连需 `BLUETOOTH_CONNECT`；扫描另按 API；缺失状态可见、无静默返回 | T-SW-L0-15..17、L1-12/16、L2-16..20、L3-09 | H/S/D | 是 |
| G-2 | AA-003 G-2 / D-17 / D-18 | 网关身份经有界发现或手输产生并持久化；硬编码仅一处迁移默认；`gateway_ble_addr` 唯一 GATT 目标；`mac_addr` 语义不变且不互推 | T-SW-L0-18..20、L1-11/15/21/22、L0d-09、L2-21..25、L3-10/11 | H/S/D | 是 |
| G-3 | AA-003 G-3 / D-20 / D-21 / IC-002 D-03 | 一次读取全部读数按绑定顺序完整转发；数量/ID/顺序不符拒绝且无部分载荷；`0.0/0.0` 为合法读数 | T-SW-L0-21..24、L1-13/19、L0d-10、L2-26..28 | H/S/D | 是 |
| G-4 | AA-003 G-4 / D-22 / IC-002 §7 | 规则文案逐字为升温方向；严格边界；App 自身“温度湿度下降报警”不变；版本/文档同步 | T-SW-L0-25、L1-14/17、L3-12/13、REG-10 | H/S/D | 是 |
| T-01a | tasks ITEM-001 验收 1 | 策略纯逻辑；SDK 26/29/30/31/33 × 直连/扫描 × 授予/拒绝矩阵；API<31 直连恒 `canConnect=true` 且缺失项不含定位 | T-SW-L0-15..17 | H | 是 |
| T-01b | tasks ITEM-001 验收 2 | 直连前置不查 `ACCESS_FINE_LOCATION`；API 31+ 缺 `BLUETOOTH_CONNECT` 按策略拒绝 | T-SW-L1-12、L2-16/17 | S/D | 是 |
| T-01c | tasks ITEM-001 验收 3 | 按 API 请求已声明权限；仅 31+ 请求蓝牙权限、仅 33+ 请求通知；无不可达分支 | T-SW-L1-16、L2-17/18 | S/D | 是 |
| T-01d | tasks ITEM-001 验收 4 | 拒绝/不再询问时首页显示缺失项与动作；日志含 `sdkInt`/缺失权限/`needScan`；无单行静默 | T-SW-L2-18/19、L3-09 | D | 是 |
| T-01e | tasks ITEM-001 验收 5 | 权限恢复无需重启；库与待上传队列不清空 | T-SW-L2-20、L0d-08 | D | 是 |
| T-01f | tasks ITEM-001 验收 6 | 既有宿主机测试通过、`assembleDebug` 成功、无新增告警 | T-SW-L1-10、L0-26 | H/S | 是 |
| T-01g | tasks ITEM-001 验收 7 | 真机弹窗/拒绝/GATT 由软件测试独立验收 | T-SW-L2-16..20 | D | 是 |
| T-02a | tasks ITEM-002 验收 1 | 硬编码 MAC 仅迁移常量一处；连接目标只来自 `gateway_ble_addr`；缺失用默认并标“未确认” | T-SW-L1-11/15、L0d-09、L2-25、L3-11 | H/S/D | 是 |
| T-02b | tasks ITEM-002 验收 2 | 规范化/校验/排序/去重/绑定状态宿主机覆盖 | T-SW-L0-18..20 | H | 是 |
| T-02c | tasks ITEM-002 验收 3 | 扫描 ≤10 s；超时/取消/销毁/蓝牙关闭/权限撤销均 `stopScan`，无泄漏 | T-SW-L1-21、L2-21 | S/D | 是 |
| T-02d | tasks ITEM-002 验收 4 | 无结果显示“未找到网关”并保留手输；不自动连接；多候选显式选择 | T-SW-L2-22/23、L3-10 | D | 是 |
| T-02e | tasks ITEM-002 验收 5 | 保存后下一轮使用；日志同条记录两地址并标注；不互推/覆盖 | T-SW-L2-24/25、L1-15 | H/S/D | 是 |
| T-02f | tasks ITEM-002 验收 6 | API<31 仅扫描请求定位并提示总开关；31+ 仅 `BLUETOOTH_SCAN` | T-SW-L0-16、L2-21、L3-10 | H/S/D | 是 |
| T-02g | tasks ITEM-002 验收 7 | 既有单测通过、构建成功、无新增告警 | T-SW-L1-10、L0-26 | H/S | 是 |
| T-02h | tasks ITEM-002 验收 8 | 真实广播/候选/保存后重连由真机独立验收 | T-SW-L2-21..25 | D | 是 |
| T-03a | tasks ITEM-003 验收 1 | 单设备载荷与冻结格式逐字一致 `<epoch>:<temp>:<humi>`（℃/%RH） | T-SW-L0-21、L1-19 | H/S | 是 |
| T-03b | tasks ITEM-003 验收 2 | 多设备按绑定顺序完整；数量/未知/重复/顺序不符拒绝且无部分载荷 | T-SW-L0-22/23 | H | 是 |
| T-03c | tasks ITEM-003 验收 3 | 空批次/devCount=0/帧拒绝 → 不发布、不入库、不上传 | T-SW-L0-23、L2-26/27 | H/D | 是 |
| T-03d | tasks ITEM-003 验收 4 | 转发拒绝只影响发布；本地告警/入库/入队仍执行 | T-SW-L2-27 | D | 是 |
| T-03e | tasks ITEM-003 验收 5 | 无“最后一笔”状态与 `isZeroValue` 哨兵；`0.0/0.0` 正常处理 | T-SW-L1-13、L0d-10、L2-28 | S/H/D | 是 |
| T-03f | tasks ITEM-003 验收 6 | Forward/Refuse 全分支宿主机覆盖；既有单测/构建通过 | T-SW-L0-21..24、L1-10 | H/S | 是 |
| T-03g | tasks ITEM-003 验收 7 | 真实多设备批次与 MQTT 实际发布由真机联调验收 | T-SW-L2-26 | D | 是 |
| T-04a | tasks ITEM-004 验收 1 | 详情文案逐字为“仅当温度较上次升高超过 0.9℃且无光，或温度超过 35.0℃ 时才上报”，严格边界 | T-SW-L0-25、L3-12 | H/D | 是 |
| T-04b | tasks ITEM-004 验收 2 | 工程无“较上次下降超过 0.9”；说明书/变更记录同步；App 自身下降报警不变 | T-SW-L1-14、L0-25 | S/H | 是 |
| T-04c | tasks ITEM-004 验收 3 | 不显示光照/照度/lux 或推断原因；不新增 0.9/35 门控计算 | T-SW-L0-25、L1-20、L3-12 | H/S/D | 是 |
| T-04d | tasks ITEM-004 验收 4 | 新增/更新宿主机断言；全部通过、构建成功 | T-SW-L0-25/26、L1-10 | H/S | 是 |
| T-04e | tasks ITEM-004 验收 5 | `versionCode`/`versionName` 递增且产物一致；`file_manifest.txt` 同步 | T-SW-L1-17/24 | S | 是 |
| T-04f | tasks ITEM-004 验收 6 | 真机界面截图验收 | T-SW-L3-12/13 | D | 是 |
| C-01 | IC-002 §7 Android 同步 | RF→BLE 向量一致、ID 归因、MQTT 数目拒绝、D-03 多设备、规则文案升温 | T-SW-L2-26..29、L3-12 | H/D | 是 |
| C-02 | IC-002 D-03 | 不得只保留最后一笔、不得静默错配 | T-SW-L0-21..23、L1-13、L2-26/27 | H/S/D | 是 |
| DLV-01 | 完整交付 | 正式构建入口成功、APK 可安装启动、资源/依赖装载、关键用户路径可用 | T-SW-REG-07/08 | S/D | 是 |
| DLV-02 | 完整交付 | 本地库/待发箱/告警/卡片/详情/早报回归不被破坏 | T-SW-REG-03..06、L0-26 | H/D | 是 |
| DLV-03 | 完整交付 | 真实跨端链路（网关 BLE → App → MQTT → `/upload_data` → 服务持久化与回读） | T-SW-REG-09 | D/V | 是 |
| DLV-04 | 完整交付 | 版本、APK 身份、文档与清单一致 | T-SW-REG-10、L1-17/24 | S | 是 |

覆盖性结论：readme 的软件相关点（1–6）、AA-003 的 G-1…G-4/D-16…D-23、四项任务的每一条验收条件均有对应用例与证据类别；readme 7/8 与非软件项已在责任边界内标明，不缩成“通过”。无“只针对已实现部分”的裁剪：四项增量在测试设计时尚未实现（见 §3.1），用例按任务验收目标编写。

---

## 3. 被测对象、版本基线与环境事实

### 3.1 版本与 as-built 基线（设计时核对）

| 项 | 事实 | 来源 |
|---|---|---|
| 仓库/分支 | `D:\mypro\beiwo2\AIP`，`main`，HEAD `999069b`（工作区干净） | `git status` / `git log` |
| 源码版本 | `versionCode 9` / `versionName "1.8"`；`minSdk 26`、`targetSdk 34`、`compileSdk 34` | `app/build.gradle.kts` |
| 四项增量实现状态 | **尚未实现**：`ble/` 目录不存在，`protocol/GatewayIdentity.kt`、`telemetry/BleBatchForwarding.kt` 不存在；`DetailPresentation.REPORT_RULE_TEXT` 仍为“下降” | 源码目录/文件核对 |
| 当前权限门 | `MqtttService.checkBluetoothPermission()`：API≥31 查 `BLUETOOTH_CONNECT`，否则查 `ACCESS_FINE_LOCATION`；门失败仅一行日志后返回 | `MqtttService.kt:552-590` |
| 当前连接目标 | 常量 `84:C2:E4:03:02:02`；`getRemoteDevice(...)` 直连；无扫描/绑定/持久化 | `MqtttService.kt:261,598` |
| 当前批次转发 | 遍历读数时覆盖 `bletime/bleTempData/bleHumiData` 为最后一笔；`isZeroValue` 连续 3 次才处理；发布串为 `time,hum,temp`（逗号分隔） | `MqtttService.kt:743-762,1281-1292` |
| 当前规则文案 | `DetailPresentation.REPORT_RULE_TEXT` 与 `fragment_device_detail.xml` 的 `tools:text` 仍写“较上次下降超过 0.9℃”；说明书 `:110,:241`、`dengbei_care/readme.txt:26` 同 | 源码/文档核对 |
| 既有宿主机测试 | 最近一次执行结果：**188 项，0 失败 0 跳过**（24 个套件）；本次复跑 `:app:testDebugUnitTest` 为 `BUILD SUCCESSFUL`（UP-TO-DATE，源码未变） | `app/build/test-results/`；本次命令输出 |
| 既有验证资产 | `app/src/test/.../verification/` 下 ITEM-001…013 独立验证用例；`evidence/software_verify_*.py`、`software_static_check_*.py` | 仓库文件 |
| 兼容面 | AES/ECB + `APP_AES_KEY16`、`assets/jd-ca.crt`、`TLSv1.2`、用户名=MAC/密码=MAC+`&^!A:z?`、topic `/<mac>/a/r0|r1|s0`、MTU 240、GATT FFE0/FFE2、`0xA1` 写入头、上传 JSON、SQLite 既有表、65℃、900 s、三条报警文案、`MyPrefs` 既有键 | AA-003 §9、IC-002 §5/§6 |

### 3.2 本次设计时的环境探测（实测，非历史记录）

| 项 | 实测结果 | 证明边界 |
|---|---|---|
| 宿主机测试运行器 | `dengbei_care/gradlew --offline :app:testDebugUnitTest` → `BUILD SUCCESSFUL in 18s`（25 tasks up-to-date） | 仅证明运行器可用、源码可编译；不证明新用例存在 |
| Android 设备（登记入口 `adb -s 12f046690506`） | 在线；`Redmi 8A`，Android 10 / SDK 29 / MIUI V125 | 仅身份与在线；不证明 BLE/业务 |
| 蓝牙/定位/网络（同一入口） | `bluetooth_on=0`（关闭）、`location_mode=3`（开启）、`Active default network: none`、`pm list packages com.jinyuni` 为空（未安装） | 当前不可执行真机 BLE/UI/MQTT/上传用例 |
| USB 枚举（`usb_list_devices`） | 手机 `VID_18D1&PID_4EE7` 序列 `12F046690506` 在位；Prolific `COM43` 在位；主机蓝牙适配器在位 | 仅枚举；不证明协议、配对或业务 |
| 真机测试工具 | `android_app_testing` 对**当前能力** `effective_status = unavailable_for_current_capability`（缺设备工具 `android_test`） | 本能力不能安装/启动/点按；执行能力需该工具 |
| 只读 ADB | `android_readonly_inspection` 为 `configured_unverified`，本次有界只读查询成功 | 仅身份/状态；不允许安装/改设置 |
| 网关 | 登记为既有 BLE 网关；上电、无线链路、广播内容、配对与真实地址**均未验证** | 相关用例为 D 类，不可用模拟替代 |
| broker/HTTP | `8.140.23.253:8883`、`:5000` 已登记为既有服务；本轮无网络且无授权入口，未探测 | V 类证据当前不可得 |
| 服务端数据库 | 无管理/重部署/DB 直连授权入口 | 行级持久化/回读需获准入口；不得用 App 日志冒充 |

> 以上是“当前条件”，不是测试通过结论。执行阶段的任何用例只有在**当轮**具备前置并产生原始证据后才可判 PASS。

### 3.3 目标设备矩阵与证明边界

| 编号 | 目标 | 可用性 | 能证明 | 不能证明 |
|---|---|---|---|---|
| D1 | Redmi 8A / Android 10 / API 29 / 12f046690506 | 在线，未安装 App，蓝牙关、无网络 | API<31 直连权限模型、真实 GATT/扫描（需网关与蓝牙开启）、MIUI 后台长跑、界面 | API 31+ 权限分支；模拟器/主机不能替代 |
| D2 | Android 12+（API 31+）真机或 API 34 模拟器 | 未确认可用 | `BLUETOOTH_CONNECT/SCAN` 请求/拒绝/设置跳转、通知权限分支 | 模拟器无射频：不能证明真实 GATT、RSSI、配对、MTU |
| G | 既有 433→BLE 网关 | 上电/身份/广播未确认 | 真实 BLE 读取、候选、多设备批次与发布 | 无法用主机模拟替代“真实链路通过” |
| S | 已部署 EMQX/HTTP/MySQL | 网络与授权未确认 | MQTT TLS、HTTP 幂等、持久化/回读 | 不能用 App 日志替代服务端持久化 |

---

## 4. 测试层次与总体策略

| 层 | 名称 | 环境 | 目的 |
|---|---|---|---|
| L0 | 宿主机纯逻辑 | JVM `:app:testDebugUnitTest --offline` | 权限策略、网关身份、批次转发、规则文案、既有纯逻辑回归 |
| L0d | 设备端存储与偏好 | 真机 debug 包（instrumented 或 `run-as` 导出） | SQLite v3、真实时间戳、待发箱、`gateway_ble_addr` 持久化、权限恢复不清库 |
| L1 | 构建与静态核查 | Gradle + 受限 `rg`/读取 | 可构建、无新增告警、静态禁词/唯一来源/兼容面 |
| L2 | 设备与链路集成 | D1/D2 + 网关 + broker/HTTP | 权限流程、发现绑定、真实批次、转发拒绝、零值、既有编排回归 |
| L3 | 展示与文案 | D1/D2 截图 + logcat/uiautomator | 首页链路状态、网关选择、详情规则、报警面板 |
| REG | 完整交付回归 | 上述组合 | 构建/安装/启动/关键路径 + 既有功能与兼容面 |
| E2E | 跨系统旅程 | 设备 + 网关 + broker + 服务 | 见 `artifacts/software_e2e_plan.md`（E2E-SW-003） |

原则：

1. **真实环境优先**：本轮涉及真机、既有网关与已部署服务。具备可访问入口与授权时，必须在该目标上规划当前版本的真实刺激与观察；主机单测/模拟器只用于局部逻辑、边界与故障补充，并逐项注明其不能证明的真实链路。
2. **两类独立证据**：凡涉及链路的结论，至少两类独立证据（App 状态/截图/日志 + 本地库/HTTP/ broker 侧记录）。
3. **刺激可重复**：优先使用可脚本化软件手段（MQTT 注入、HTTP 探测、`adb` 截图/logcat、debug 包数据库导出）；真实 BLE 必须使用真实网关，不得以注入替代“BLE 链路通过”。
4. **范围受限与有限等待**：搜索限定 `dengbei_care/app/src`、排除 `iOS开发资料/`；每条命令设定显式超时（普通只读 ≤60 s，构建/测试按任务规模设上限）；超时即记录，不原样无限重试。
5. **不伪造**：空抓包、无结果、未执行均如实记录为 NOT EXECUTED/NOT PROVEN，不写成通过。

---

## 5. 用例编号约定

- 用例编号（`T-SW-*`）仅作本文件内部索引，不代表 Runtime 任务/节点。
- “前置”决定执行时的适用性，不改变用例目标；前置缺失记 NOT EXECUTED。
- 既有 TD-SW-002 用例编号继续有效；本版新增从 `L0-15`、`L0d-08`、`L1-10`、`L2-16`、`L3-09`、`REG-07` 起编。

---

## 6. 用例设计

### 6.1 L0 · 本轮新增纯逻辑（JVM）

| 编号 | 行为 | 前置 | 方法/刺激 | 证据 | 判定 |
|---|---|---|---|---|---|
| T-SW-L0-15 | 权限策略：直连矩阵 | 无（纯函数） | `evaluate(sdk, granted, needScan=false)`，sdk ∈ {26,29,30,31,33}，granted ∈ {∅,{CONNECT},{SCAN},{CONNECT,SCAN},{FINE_LOCATION}} | JUnit 断言 | sdk<31：`canConnect=true` 且 `missing` **不含** `FINE_LOCATION`/`CONNECT`/`SCAN`；sdk≥31：`canConnect == (CONNECT ∈ granted)`，缺失时 `missing` 含 `CONNECT` 且不含 `FINE_LOCATION`。26/29/30 行为一致 |
| T-SW-L0-16 | 权限策略：扫描矩阵 | 无 | `needScan=true`，同上 sdk/授予组合 | 同上 | sdk<31：`FINE_LOCATION` 未授予 → `missing` 含之，`requiresLocationServices=true`；已授予 → 扫描缺失为空但仍 `requiresLocationServices=true`（调用方须查定位总开关）；`canConnect` 不因扫描权限变化。sdk≥31：`missing` 含 `BLUETOOTH_SCAN`（未授予时），`requiresLocationServices=false`，`canConnect` 仅由 `CONNECT` 决定 |
| T-SW-L0-17 | 权限策略：纯函数性、提示语义与通知边界 | 无 | 同输入重复调用；反射/字节码检查无 `android.*` 类型；`missing` 各项与 `userMessage` 对应；`POST_NOTIFICATIONS` 拒绝/授予 | 断言 + 反射 | 输出确定、不修改入参；`canConnect=false` 时 `userMessage` 非空且含可执行动作（授权/开蓝牙/开定位/去设置之一）；`missing` 顺序稳定；`POST_NOTIFICATIONS` 不改变 `canConnect`，且不作为 BLE 阻断项；API<33 不请求通知 |
| T-SW-L0-18 | 网关身份：规范化与校验 | 无 | 向量：`84:c2:e4:3:2:2`、`84C2E4030202`、`84-C2-E4-03-02-02`、首尾空白 → 均为 `84:C2:E4:03:02:02`；非法：空/null/11/13 位/非法字符/段长>2/连续分隔符/首尾分隔符；`.` 分隔按实现文档（若支持须一致） | 同上 | 合法输入规范化后恒为 6 组 2 位大写 HEX；非法输入 `normalize==null` 且 `isValid=false`；合法与非法集合无交集；不产生部分规范化结果 |
| T-SW-L0-19 | 网关身份：候选排序与去重 | 无 | 乱序候选（含 FFE0 真/假、RSSI 高低、同 MAC 重复、空名、非法 MAC） | 同上 | 按规范化 MAC 唯一；FFE0=true 组优先；组内 RSSI 降序；完全并列按 MAC 升序稳定；重复 MAC 保留最强 RSSI（FFE0 取或）；非法 MAC 不进入可选列表；输入列表不被修改 |
| T-SW-L0-20 | 网关身份：绑定状态与不互相推导 | 无 | `planBinding(selected, ...)`：null/空白 → 未配置；合法 → 已选择/已保存；非法 → 非法；另测 `mac_addr`（12 位 HEX，如 `1010100000A1`）被设置时 `gateway_ble_addr` 不被写入或推导 | 同上 + 反射 | 三态判定明确且互斥；工程内不存在“平台 MAC ↔ BLE MAC”自动换算 API；`mac_addr` 赋值不改变连接目标；迁移默认值与用户清除可区分（见 T-SW-L0d-09）。`boundIds` 若无可观察语义须由实现说明，测试不臆造 |
| T-SW-L0-21 | 批次转发：单设备冻结载荷 | 无 | `plan([sample(A,24.5,55.0)], [A], 1699999999)`；另测 T=-3.2、T=0.0/H=0.0 | JUnit 断言 | `Forward("1699999999:24.5:55.0")`；负温保留 `-`；恒一位小数；**冒号分隔、温度在前湿度在后、无空格**；`Refuse` 不得携带载荷字段。**注意 F-01**：当前 as-built 发布串为 `time,hum,temp`，与本冻结格式不一致，实现必须按 IC-002/任务验收改正 |
| T-SW-L0-22 | 批次转发：多设备按绑定顺序 | 无 | `boundOrder=[A,B,C]`，samples 同序；值含负温/0；时间透传 | 同上 | `Forward("<time>:<ta,tb,tc>:<ha,hb,hc>")`；顺序即绑定顺序；无截断/重排；时间原样 |
| T-SW-L0-23 | 批次转发：Refuse 全分支 | 无 | ①samples 空；②bound 空；③数量不等；④未知 ID；⑤重复 ID；⑥集合相同但顺序不同；⑦重复且数量不等（优先级） | 同上 | 每种均 `Refuse`，无任何部分载荷；原因与设计枚举一致（`EMPTY_BATCH`/`NO_BOUND_DEVICE`/`COUNT_MISMATCH`/`UNKNOWN_ID`/`DUPLICATE_ID`/`ORDER_MISMATCH`，优先级须文档化）；异常输入不抛未捕获异常 |
| T-SW-L0-24 | 批次转发：纯度、本地化与确定性 | 无 | 默认 `Locale` 设为 `Locale.GERMANY`/`Locale.CHINA` 各测一次；重复调用；50 台批次；输入列表快照比对 | 同上 | 小数分隔符恒为 `.`（实现须显式 `Locale.ROOT/US`）；同一输入恒同输出；不修改入参；50 台不截断（若设上限须按设计拒绝而非静默截断） |
| T-SW-L0-25 | 规则文案、禁词与 App 报警边界 | 无 | 读取 `DetailPresentation.REPORT_RULE_TEXT`、详情布局文本、说明书/变更记录规则句、三条报警文案常量 | 断言 + 文件读取 | 规则文案逐字等于 `采样周期 3 分钟。仅当温度较上次升高超过 0.9℃且无光，或温度超过 35.0℃ 时才上报；未满足条件时不会上报，因此可能长时间没有新数据。`；含“升高”，不含 `较上次下降`/`下降超过 0.9`/`下降 >0.9`；含 `35.0℃`；`DetailPresentation` 与详情布局中无 `lux`/`光照`/`照度`（规则原文“无光”允许）；无 App 推断的上报原因（如“因…上报”）；App 自身三条报警文案逐字不变；说明书/变更记录规则句同步为升温且注明严格边界 |
| T-SW-L0-26 | 既有宿主机用例回归 | 无 | `:app:testDebugUnitTest --offline` | 测试报告 + 退出码 | 退出码 0；**188 项既有 + 本轮新增**全部通过、0 失败 0 跳过；旧方向断言必须被**更正**为升温（`DetailPresentationTest`、`DetailPresentationItem012VerificationTest`），不得以删除/跳过掩盖；任何跳过须逐条说明并视为未验证 |

### 6.2 L0 · 既有用例的适用性（TD-SW-002 保留）

| 既有用例 | 本轮是否适用 | 说明 |
|---|---|---|
| T-SW-L0-01..03（帧解码/多设备归属） | 适用 | 本轮不改字节布局；作为 G-3 的上游输入回归 |
| T-SW-L0-04..05（MQTT 归因/拒绝整批） | 适用 | G-3 不得破坏 MQTT 路径；与 `BleBatchForwarding` 互不影响 |
| T-SW-L0-06..10（告警） | 适用 | App 自身下降报警/阈值/紧急语义保持；D-22 要求不被文案改动污染 |
| T-SW-L0-11..12（上传载荷/编排） | 适用 | 完整交付回归 |
| T-SW-L0-13（服务器常量） | 适用 | 本轮不应改服务器地址；L1-23 复核 |
| T-SW-L0-14（卡片状态逻辑） | 适用 | 完整交付回归 |

### 6.3 L1 · 构建与静态核查

| 编号 | 行为 | 前置 | 方法 | 证据 | 判定 |
|---|---|---|---|---|---|
| T-SW-L1-10 | 构建、全量单测、告警、产物 | Gradle/JDK/SDK | `./gradlew :app:testDebugUnitTest --offline`、`:app:assembleDebug --offline`、`:app:compileDebugKotlin --offline --rerun` | 原始日志、退出码、APK 列表、JUnit XML | 两条 Gradle 命令退出码 0；新增用例全部通过；编译告警集合与基线（既有 11 条）一致，无新增；APK 生成 |
| T-SW-L1-11 | 硬编码 MAC 唯一迁移常量 | 源码 | 在 `app/src/main` 检索 `84:C2:E4:03:02:02`（仅作字面核对，范围受限） | 命中位置与上下文 | 恰 1 处，位于迁移默认常量定义；`connectToBluetoothDevice`/连接入口不直接引用该常量，而引用 `gateway_ble_addr` 读取结果；`app/src/test`、`iOS开发资料/` 不计入产品判定但须分别记录 |
| T-SW-L1-12 | 直连路径权限口径唯一、无静默返回 | 源码 | 检索 `ACCESS_FINE_LOCATION`、`checkBluetoothPermission`、`isBluetoothBusy`、`connectToBluetoothDevice`、`Bluetooth permission not granted` | 关键行 + 代码路径审阅 | `MqtttService` 直连路径不以 `ACCESS_FINE_LOCATION` 为前置（扫描流程除外）；`checkBluetoothPermission/isBluetoothBusy/connectToBluetoothDevice` 共用同一策略结果；权限失败时写入可读状态且日志含 `sdkInt`/缺失权限/`needScan`；不存在“仅一行日志后 return”的路径 |
| T-SW-L1-13 | 无“最后一笔”与零值哨兵 | 源码 | 检索 `bleTempData`/`bleHumiData`/`bletime`/`bletimebkp`/`isZeroValue`/`bleZeroCount` | 命中 | `app/src/main` 0 命中；转发状态只存在于 `BleBatchForwarding` 的纯函数结果中；`0.0/0.0` 无特殊分支 |
| T-SW-L1-14 | 规则方向禁词与文档同步 | 源码 + 文档 | 精确检索：`较上次下降`、`下降超过 0.9`、`下降 >0.9`；允许 `温度湿度下降报警`、`tempdrop` 等 App 报警语境 | 命中清单 | 规则语境 0 命中；App 报警语境保留；`用户使用说明书.md`、`dengbei_care/readme.txt` 的规则句为升温且含严格边界；`SensorCadence` 等注释同步 |
| T-SW-L1-15 | 连接身份唯一、两身份不互推 | 源码 | 检索 `gateway_ble_addr`、`mac_addr`、`getRemoteDevice`；审阅写入点 | 写入/读取点清单 | `gateway_ble_addr` 是唯一 GATT 目标来源；`mac_addr` 仅用于 topic 与 `/upload_data.mac`；无任何代码路径由一方写另一方；日志同条记录两者并分别标注 |
| T-SW-L1-16 | 权限请求集合与 Manifest 一致、无不可达分支 | 源码 + Manifest | 对照 `requestPermissions` 调用与 `<uses-permission>`；按 API 条件审阅 | 集合对照表 | 请求集合 ⊆ Manifest 已声明；`BLUETOOTH_CONNECT/SCAN` 仅 sdk≥31；`POST_NOTIFICATIONS` 仅 sdk≥33；`ACCESS_FINE_LOCATION` 仅用于 API<31 扫描流程；无“通知回调里再请求定位”等不可达/无关分支 |
| T-SW-L1-17 | 版本与清单 | 构建脚本/APK/清单 | 读取 `versionCode/versionName`；解包 APK 的 `AndroidManifest.xml`；核对 `file_manifest.txt` | 值 + APK 摘要 + 清单条目 | `versionCode>9` 且 `versionName>1.8`；APK 内版本与源码一致；`file_manifest.txt` 已登记本轮新增/修改文件且 active 路径存在 |
| T-SW-L1-18 | 兼容面 diff | 改动前后源码/资源 | 对比 AES/模式/密钥、`jd-ca.crt` 摘要、`TLSv1.2`、用户名/密码拼接、topic、MTU、GATT UUID、`0xA1` 头、上传 JSON、SQLite 既有 DDL、65℃、900 s、三条报警文案、既有 `MyPrefs` 键 | diff/摘要 | 全部与基线一致；任何差异须有设计决策支撑，否则记为偏差 |
| T-SW-L1-19 | BLE 转发载荷格式契约核对 | 源码 | 读取 `BleBatchForwarding` 与 `connectToBroker` 发布串；与 IC-002 §6/任务验收比对 | 逐字字符串 | 发布串为 `<epoch>:<temps>:<humis>`，冒号分隔、温度在前；**当前 as-built 的 `time,hum,temp` 不得保留**（F-01）；小数点为 `.` |
| T-SW-L1-20 | 无第二套上报门控 | 源码 | 检索 App 中 `0.9`/`9`/`35` 相关比较；审阅 `processTemperatureHumidityData` | 命中清单 | App 不实现“传感器是否上报”的门控；`35.0` 仅用于详情高温强调；App 自身报警阈值（默认 35/29 等）属既有功能，保持不变且不冒充实测上报判据 |
| T-SW-L1-21 | 扫描有界与停止路径静态核对 | 源码 | 审阅 `GatewayDiscovery` 的启动/超时/取消/`onDestroy`/蓝牙关闭/权限撤销/异常分支 | 分支清单 | 每条结束路径都有 `stopScan` 与回调释放；存在 ≤10 s 上限常量；无未取消的延时任务；不因无结果自动重试成无限扫描 |
| T-SW-L1-22 | 扫描唯一性 | 源码 | 检索 `BluetoothLeScanner`/`startScan`/`ScanFilter`；区分既有 Wi-Fi 扫描 | 命中 | BLE 扫描只存在于 `GatewayDiscovery`；不新增第二处 BLE 扫描；既有 Wi-Fi 配网扫描不受本项影响 |
| T-SW-L1-23 | 服务常量与网络安全配置未变 | 源码/资源 | 检索 `8.140.23.253`、`network_security_config.xml` 放行域 | 命中 | 服务器地址仍只在 `CloudConfig` 定义；明文放行集合仍为既有 3 个主机，无新增；旧主机不成为连接目标 |
| T-SW-L1-24 | 清单与仓库卫生 | 仓库 | 逐行检查 `file_manifest.txt` active 条目路径存在（排除 coarse 目录）；`git status` 无未提交人工作业（由 Runtime 提交前核对） | 检查输出 | 0 条无效 active 路径；无八进制转义残留；本轮修改文件均已登记 |

### 6.4 L0d · 设备端存储与偏好

既有 T-SW-L0d-01..07 继续适用。本轮新增：

| 编号 | 行为 | 前置 | 方法 | 证据 | 判定 |
|---|---|---|---|---|---|
| T-SW-L0d-08 | 权限变化不清空本地数据 | 真机 debug 包；库中有 `temperature`/`alarm_events`/`pending_uploads` 行 | 记录三类行数 → 撤销/授予蓝牙权限（或停止/恢复守护）→ 等待至少一轮 → 再查 | `run-as` 导出 DB + 查询结果 | 三类行数与内容不变；服务继续轮询；无 `DROP`/`DELETE`/重建 |
| T-SW-L0d-09 | `gateway_ble_addr` 持久化、迁移与清除区分 | 真机 debug 包 | ①升级/首次运行（键从未存在）→ 读默认并标注“未确认”；②保存候选/手输 → 键为规范化值；③清除 → 回到未配置，**不得**静默回到旧常量；④重启 App → 读取已保存值 | `MyPrefs` XML/`run-as` + 界面 | 四步行为符合 D-17/§6.2；迁移默认只在“从未迁移”时生效（需迁移标记，见 F-02）；保存值规范化为大写冒号 MAC |
| T-SW-L0d-10 | `0.0/0.0` 正常入库 | 真机 debug 包；守护开 | 以 `(devId, time=T, 0.0, 0.0, storeflag=1)` 写入一次并重复一次；再以 `storeflag≠1` 写入 | 查询行 | 首次写入 1 行且 `time==T`；重复写入仍 1 行；守护关闭 0 行；无“连续 3 次”条件 |

### 6.5 L2 · 设备与链路集成

前置（逐用例可收窄，但不得替代）：D1 或 D2 已安装**当前版本** debug 包；蓝牙/GATT 与网络按用例开启；测试账号/绑定数据为非生产；网关按用例上电；所有设备状态改变由获准执行者完成并记录。当前能力无 `android_app_testing`，故本组在**本设计阶段均未执行**。

| 编号 | 行为 | 前置 | 刺激/步骤 | 证据 | 判定 |
|---|---|---|---|---|---|
| T-SW-L2-16 | API 29 直连不受定位权限阻断（G-1 核心） | D1；App 未授予定位；蓝牙开；网关上电或在范围内；守护开 | 冷启动 → 开启守护 → 观察策略与连接尝试。**不**授予定位 | logcat（策略字段与连接尝试）、`pm dump` 权限、截图 | 日志显示 `sdkInt=29`、`needScan=false`、缺失项为空、`canConnect=true`；出现对 `gateway_ble_addr` 的 `connectGatt` 尝试；**不出现**仅 “Bluetooth permission not granted” 即返回；手机未授予定位仍可尝试。连接是否成功取决于网关，单列判定 |
| T-SW-L2-17 | API 31+ 直连权限拒绝/授予/撤销（G-1） | D2 | ①首次开启守护，拒绝 `BLUETOOTH_CONNECT`；②系统设置授予；③运行中撤销 | logcat + 截图 + 权限状态 | ①不发起连接，首页显示缺失项与动作；②下一轮（≤180 s 或立即读取）恢复连接尝试，**不重启进程**；③下一轮停止连接并显示状态；日志含 `sdkInt`/缺失项/`needScan` |
| T-SW-L2-18 | 拒绝与“不再询问”动作（G-1/D-19） | D1/D2 | 拒绝权限；选择“不再询问”后再次触发；点“去授权/系统设置” | 截图 + logcat | 首页状态区显示**具体**缺失项与可执行动作；点击动作进入系统弹窗或应用设置；无静默空转；不出现每 180 s 仅一行日志 |
| T-SW-L2-19 | 蓝牙关闭与未配置状态（D-19） | D1/D2 | 关闭蓝牙；清除/不设置网关；清除平台 MAC 身份（测试账号） | 截图 + logcat | 分别显示“蓝牙未开启”“未配置网关/使用默认（未确认）”“未配置平台身份”等真实原因；不把数据龄当连通性；不静默返回 |
| T-SW-L2-20 | 权限恢复不清数据（T-01e） | D1/D2；库与待发箱有测试数据 | 撤销权限 → 恢复 → 观察下一轮 | 截图 + DB 行数 + logcat | 无需重启进程即可继续；`temperature`/`alarm_events`/`pending_uploads` 计数与内容不变 |
| T-SW-L2-21 | 有界扫描与 `stopScan` 全路径（G-2） | D1（定位权限/总开关）与 D2（`BLUETOOTH_SCAN`）；网关上电 | ①正常扫描；②10 s 超时；③用户取消；④页面销毁；⑤扫描中关蓝牙；⑥扫描中撤销权限 | logcat 时间戳 + 回调计数 + 等待 ≥15 s 的静默观察 | 每次扫描从开始到 `stopScan` ≤10 s；停止后无新回调（≥15 s）；无持续扫描/泄漏；API<31 无定位权限时不执行无效扫描并提示；API31+ 仅请求 `BLUETOOTH_SCAN` |
| T-SW-L2-22 | 候选呈现、排序、去重、不自动连接（G-2） | 网关上电；D1/D2 | 扫描；观察候选名称/MAC/RSSI；若网关不广播 FFE0，执行显式无过滤回退扫描 | 截图 + logcat + 网关侧身份记录 | 候选去重、FFE0 优先、RSSI 降序（与 L0-19 一致）；无自动连接；多候选由用户显式选择；记录网关实际是否广播 FFE0/名称（U-1 未证实） |
| T-SW-L2-23 | 无候选与手动输入（G-2） | 网关不上电/不在范围 | 扫描至超时；手动输入合法/非法 MAC | 截图 + logcat | 显示“未找到网关”并保留手动输入；合法保存成功；非法输入明确拒绝、不崩溃、不写入；不自动连接其他设备 |
| T-SW-L2-24 | 保存后下一轮使用、日志双地址（G-2/D-18） | D1/D2；网关上电 | 选择候选保存 → 触发下一轮读取 → 重启 App 再读 | `MyPrefs` + logcat + 截图 | `gateway_ble_addr` 持久化为规范化 MAC；下一轮与重启后使用该地址；同条日志含 `gateway_ble_addr` 与 `mac_addr` 且分别标注用途；修改 `mac_addr` 不改变 `gateway_ble_addr` |
| T-SW-L2-25 | 默认值“未确认”与清除后不回退（G-2/D-17） | 从旧版本升级场景或键从未存在；另测用户清除 | ①首次升级读取默认 → 观察标注；②用户清除 → 观察是否回到默认 | 截图 + `MyPrefs` | ①标注“使用默认网关地址（未确认）”，允许尝试但不宣称已确认；②清除后为未配置，**不静默回退**到旧常量（否则判 FAIL，见 F-02） |
| T-SW-L2-26 | 真实多设备 BLE 批次：本地逐条 + 完整发布（G-3/D-03） | 真实网关带 ≥2 设备；`MacIdBook` 顺序与帧一致；broker 可达；独立 MQTT 订阅端 | 触发一次真实读取；记录 App 解析、本地库/待发箱、broker 收到的发布串 | logcat + DB 导出 + broker 原文 + 时间线 | 每条读数按自身 ID 完成告警/入库/入队；发布串按绑定顺序包含全部读数；格式/单位与 T-SW-L0-21/22 一致；客户端发布时间与读取时间对应 |
| T-SW-L2-27 | 转发拒绝不影响本地处理（G-3） | 同上；用测试账号把 `MacIdBook` 调成与帧数量/顺序不一致（记录并复原） | 触发读取；观察发布与本地库/待发箱 | broker 静默 + DB 行 + logcat | 无任何发布；日志给出拒绝原因；本次全部读数仍按各自 ID 完成告警、入库（真实接收时刻）与入队；不产生部分载荷；测试后恢复绑定 |
| T-SW-L2-28 | `0.0/0.0` 正常处理（G-3） | D1/D2；MQTT 注入可达；BLE 0.0 源按可得性 | ①MQTT 注入 `T:0.0:0.0`（测试绑定、守护开、报警开关可关以隔离）；②真实 BLE 帧含 0.0 时观察（若有源） | DB 行 + logcat + 待发箱 + broker | 首次即进入处理路径：入库/入队/卡片更新；无“连续 3 次”门槛；BLE 0.0/0.0 的真实端到端若无可控源，按 §9 记 NOT EXECUTED，不得用 MQTT 结果冒充 |
| T-SW-L2-29 | MQTT/BLE 既有格式与归因回归 | broker/测试客户端可达 | 注入正常/负温/零值/数目不符/非法值；BLE 真实帧 | logcat + DB + 待发箱 | 正常载荷按 `MacIdBook` 顺序归因；数目不符/非法整批拒绝；负温/0 值正确；既有 topic 与 JSON 不变 |
| T-SW-L2-30 | 告警/入库/上传既有编排回归 | 守护开；网络可用 | 注入下降触发/阈值/紧急/900 s 内重复；断网与恢复 | 通知/截图 + `alarm_events` + 待发箱 + HTTP | 时间窗下降、阈值、`>65℃` 每次通知、900 s 去抖、`(dev_id,time)` 幂等、门控与失败不阻塞主链路均保持 |
| T-SW-L2-31 | 服务链路：MQTT TLS 与 `/upload_data` 幂等 | D1/D2 联网；服务入口获准 | 启动 App 观察 TLS 连接；构造载荷 `POST /upload_data` 两次；核对响应 | logcat/状态 + HTTP 状态码/响应体 | 连接 `ssl://8.140.23.253:8883` 且信任锚仍为 `jd-ca.crt`；2xx 成功；重复 `(dev_id,time)` 不产生第二行（`inserted=0` 或等价）；失败可观测、可退避 |
| T-SW-L2-32 | 3 分钟节拍与前台服务长跑 | D1（MIUI） | 守护开，息屏/充电/省电模式连续 ≥2 个周期（≥6 min），记录读取时刻与服务存活 | logcat 时序 + `dumpsys activity services` + 截图 | 读取间隔 ≈180 s（允许调度抖动并记录）；服务不被杀；无网络/无上报时不空转刷屏；周期内无崩溃 |
| T-SW-L2-33 | 守护关闭/无手机号门控 | D1/D2 | 关闭守护；清空 `registerphone`（测试设备） | DB/待发箱/HTTP/通知 | 不落库、不入队、不发请求、不通知；历史不回溯删除 |
| T-SW-L2-34 | 断网累积与恢复补传 | D1/D2 | 飞行模式注入多条 → 恢复网络 | 待发箱/HTTP/logcat | 断网读数保留；恢复后补传；每 `(dev_id,time)` 仅一行；退避有界；不阻塞主链路 |

### 6.6 L3 · 展示与文案（真机截图 + logcat/uiautomator）

界面视觉/交互的专业评审属 UI 能力；本节只做**功能呈现核对**（文案、状态、可执行动作、数据与规则一致性），不代替 UI 专业评审。所有 L3 用例同时保留 logcat/文本 dump，避免只凭截图下结论。

| 编号 | 行为 | 前置 | 方法 | 证据 | 判定 |
|---|---|---|---|---|---|
| T-SW-L3-09 | 首页链路状态区 | D1/D2 | 构造：权限缺失（API29 扫描场景/API31+ 直连）、蓝牙关闭、未配置网关、默认网关未确认 | 截图 + 文本 dump | 每种状态显示**具体原因**与可执行动作；不显示“离线”作为链路状态；权限缺失/蓝牙关闭不被静默吞掉 |
| T-SW-L3-10 | 网关选择页 | D1/D2；网关上电 | 进入选择页 → 扫描 → 候选列表 → 手动输入 → 无结果 | 截图 + 文本 | 显示名称/MAC/RSSI 与 FFE0 标记；支持手动输入；无结果显示“未找到网关”并保留输入；不自动连接 |
| T-SW-L3-11 | 默认网关“未确认”标注 | 迁移场景 | 打开首页/设置 | 截图 | 标注明确，不与“已确认/已连接”混淆；`mac_addr` 与 `gateway_ble_addr` 分开展示 |
| T-SW-L3-12 | 详情规则文案（G-4 核心） | D1/D2 | 打开设备详情 | 截图 + 文本 dump | 规则句逐字为升温方向（T-SW-L0-25 的冻结文本）；严格边界可见（“超过 0.9℃/超过 35.0℃”）；无光照/照度/lux；无推断原因 |
| T-SW-L3-13 | App 自身报警面板不变 | D1/D2 | 打开报警设置/触发下降报警 | 截图 + 通知 | “温度湿度下降报警”面板、阈值、去抖、三条文案均与改动前一致 |
| T-SW-L3-14 | 既有卡片/详情/早报回归 | D1/D2 | 卡片墙、未收到、40 min 无数据、24 h 图表、35.0/35.1、早报 | 截图 + 通知 | 不出现“离线”；未收到显示 `--`；24 h 上限；仅 >35.0 强调；早报“昨日无上报/非等间隔” |

### 6.7 REG · 完整交付回归

| 编号 | 内容 | 适用性 | 方法 | 判定 |
|---|---|---|---|---|
| T-SW-REG-01 | 既有 5 个端点 | 服务可达 | 无效数据调用 `/login`、`/setmac`（测试号码，避免真实短信/写库） | 端点存活、签名/结构与基线一致 |
| T-SW-REG-02 | BLE 解码兼容 | 真实网关 | 一次真实上报与网关记录逐字段对照 | 与 IC-002 §4 逐位一致；负温正确；无温湿度互换 |
| T-SW-REG-03 | 旧库升级与既有查询 | 设备 | T-SW-L0d-01/02/06 | 全部通过；数据不丢 |
| T-SW-REG-04 | 告警/报告/文案/参数键 | 设备 | T-SW-L2-30、L3-07、L3-13、L1-18 | 全部通过 |
| T-SW-REG-05 | 守护开关与卡片交互 | 设备 | 开关开/关注入；卡片进详情；清除报警 | 语义与改动前一致 |
| T-SW-REG-06 | 兼容面静态 diff | 本机 | T-SW-L1-18 | 全部一致 |
| T-SW-REG-07 | 完整交付门禁 | 本机 | 正式构建入口 + 全量宿主机测试 + 静态核查 + APK 身份 | `assembleDebug` 成功；188 项既有 + 本轮新增全部通过；静态项全 PASS；APK 版本/摘要记录；无新增告警 |
| T-SW-REG-08 | 应用级关键用户路径 | D1（必要时 D2） | 安装当前 debug APK → 启动 → 隐私政策 → 首页 → 开启守护 → 权限处理 → 选择/确认网关 → 收到读数 → 卡片/详情显示 → 上传触发 | 每一步有截图/logcat；应用可启动、依赖/资源装载正常、无崩溃；失败步骤记录为 FAIL/NOT EXECUTED |
| T-SW-REG-09 | 真实跨端链路 | 网关 + D1/D2 + broker + 服务 | 真实传感器/网关 → BLE → App → MQTT → `/upload_data` → 服务持久化与回读 | 逐字段一致（devId/time/temperature/humidity）；服务端持久化/回读需获准入口，缺失则 NOT EXECUTED，不得以 HTTP 2xx 单独宣告持久化通过 |
| T-SW-REG-10 | 版本与文档一致性 | 本机 | `versionCode/versionName`、APK、`file_manifest.txt`、说明书/变更记录 | 版本递增且一致；文档规则句为升温；清单同步 |

---

## 7. 数据采集与证据规范

| 数据 | 采集方式 | 保存要求 |
|---|---|---|
| 宿主机测试 | Gradle 报告/JUnit XML + stdout | 原始报告路径、退出码、执行时间、**被测提交号**、测试总数/失败/跳过 |
| 构建 | Gradle 日志 + APK | 原始日志、APK 路径、SHA-256、`versionCode/versionName`（从 APK 内读取） |
| 设备运行日志 | `adb -s <serial> logcat`（按 `MqttService`/`DataProcessor`/权限/扫描相关 tag 过滤） | 原始日志文件 + 起止时间 + 关键行标注；不得只保存截取后的结论行 |
| 界面状态 | `adb exec-out screencap` / uiautomator dump | PNG/XML + 操作步骤 + 时间；截图须含状态区/规则文案原文 |
| 本地库/偏好 | debug 包 `run-as` 导出 DB 与 `MyPrefs` XML 后只读解析 | 文件摘要 + 查询结果（行数/时间戳/字段值）+ 操作前后对比 |
| MQTT 刺激与观测 | 独立 `clientId` 的测试客户端；发布/订阅原文 | topic、payload 原文、时间、客户端日志；禁止冒用真实网关 `clientId` |
| broker 侧发布观测 | 测试订阅端订阅对应 topic | 原始消息（含时间戳）+ 订阅端日志；与 App 发布时间对齐 |
| 云上传 | App 待发箱变化 + HTTP 请求/响应 | 请求体、状态码、响应体、时间；测试数据限定测试号码/devId，不保存真实用户隐私 |
| 判定 | 每用例：期望、实测、结论、证据路径、版本/设备/网络/网关状态 | 失败项含最小复现步骤、现象时间点、日志片段；未执行项写明缺失前置 |

证据要求：

- 每个结论必须能由保存的原始证据复现；不保存“凭印象”的结论。
- 每条链路结论至少两类独立证据（§4 原则 2）。
- 真实 BLE、真实服务、模拟器、主机单测四类证据**分开标注**，不得混写为同一结论。
- 版本三元组必须可关联：被测提交/APK 摘要 + 设备型号/API + 网关身份（若适用）+ broker/服务版本（若适用）。

### 7.1 清理要求（每用例执行后）

| 对象 | 清理动作 |
|---|---|
| 测试偏好/绑定 | 复原 `MacIdBook`、`mac_addr`、`gateway_ble_addr`、`registerphone` 与报警参数到执行前快照；不得留下测试值污染后续用例 |
| 本地库/待发箱 | 仅对测试账号/测试 devId 的数据做清理；删除前先导出；不得清空用户真实数据 |
| 设备状态 | 关闭测试用扫描、断开 GATT、恢复蓝牙/定位/网络到记录的原状态；不卸载用户应用、不清应用数据 |
| 测试 MQTT 客户端 | 断开并释放独立 `clientId`；确认无残留订阅（避免继续接收/发布） |
| 服务端 | 不调用删除/管理接口；测试数据按服务端责任方约定处理；不修改生产配置 |
| 进程/后台任务 | 结束本次自建的测试进程与临时服务；确认无遗留扫描回调、定时器或后台命令；记录已停止 |
| 证据 | 清理前后各留一份关键状态证据（行数/偏好/设备状态），证明清理完成且未误删 |

## 8. 前置条件、依赖与环境缺口

### 8.1 当前能力可执行与不可执行

| 项 | 当前状态 | 影响 | 恢复/交接 |
|---|---|---|---|
| 宿主机测试运行器 | **可用**（本次 `BUILD SUCCESSFUL`） | L0/L1 可执行 | 执行能力直接使用；新增测试/夹具可写入 `app/src/test` |
| 只读 ADB | **可用**（configured_unverified，本次查询成功） | 身份/状态/日志查询可执行 | 不改设备设置；安装/点按不可 |
| `android_app_testing` | 对当前能力 **unavailable**（缺 `android_test`） | L2/L3/REG-08 无法在本能力执行 | 执行阶段需该工具；本设计不修改权限文件 |
| 设备当前状态 | App 未安装、蓝牙关、无网络、定位开 | 真机用例前置不满足 | 由获准执行者安装当前 APK、开蓝牙/网络、记录状态 |
| 真实网关 | 上电/广播/身份/配对未验证 | 真实 BLE/多设备用例不可执行 | 人工上电与范围内确认；身份经 App 发现流程确认（U-1..U-3） |
| API 31+ 设备/模拟器 | 未确认 | 31+ 权限分支无法验证 | 提供真机或 AVD；模拟器结论仅限权限/UI |
| broker/HTTP | 未探测（无网络/无授权；本次 allowed_external_actions 为空） | MQTT/上传用例不可执行 | 执行阶段需网络与授权入口 |
| 服务端持久化/回读 | 无 DB/管理授权入口 | 跨端“持久化”证据不可得 | 需获准服务入口；否则 DLV-03/NOT EXECUTED |
| instrumented SQLite | 历史记录 UTP 依赖缺失/无外网 | 设备端行级验证可能受阻 | 优先 `run-as` 导出 + 宿主机只读解析；不得以宿主机单测替代 |

### 8.2 测试环境自修复边界

- 本轮 `environment_repair` 范围为 test，`allow_target_changes=false`、`allow_system_packages=false`。可准备：宿主测试脚本、夹具、数据构造脚本、证据解析脚本、隔离测试配置；不可：安装系统包、改设备/目标设置、刷机、改网络/驱动、修改权限文件。
- 已确认宿主运行器可用，无需修复；缺 `android_test` 属工具授权/配置缺口，不由本能力自行扩大。
- 任何自修复尝试须限时、保留可恢复配置，并记录原因/命令/版本/改动/复核结果；次数不超过 `max_attempts_per_issue=3`。

### 8.3 最小恢复顺序（执行阶段）

0. **有界探测**（每项设显式超时，只读优先）：`adb devices`/系统属性/蓝牙与网络状态（≤30 s）；broker 端口与 HTTP 健康探测（≤10 s/次，有限次数）；网关是否在 BLE 广播中出现（≤10 s 扫描，须先具备扫描权限）。探测失败只记录，不推断设备损坏或功能失败。
1. 为软件测试执行授予 `android_app_testing`；确认 D1 在线。
2. 用当前源码构建 debug APK，记录提交号与 APK SHA-256。
3. 安装并启动；接受隐私政策；确认应用可运行（REG-08 前半）。
4. 开启蓝牙与网络；给网关上电并确认在范围内。
5. 先跑 L0/L1（当前即可），再跑 D1 的 API 29 权限/发现链路，再跑真实网关批次，最后跑服务链路。
6. 需要 API 31+ 分支时启用 D2；需要多设备时准备 ≥2 台已绑定设备且顺序可控。
7. 服务端行级持久化需单独获准入口；未获准前保持 NOT EXECUTED。

---

## 9. 覆盖性检查、发现与交接

### 9.1 覆盖性自检

- [x] readme 软件相关点（1–6）均有可观测行为与用例；点 7/8 明确归属固件/嵌入式，不从软件测试范围删除也不由本设计判通过。
- [x] AA-003 G-1…G-4、D-16…D-23 逐条落到用例。
- [x] 四项任务的每一条验收条件在 §2 映射到用例与证据类别。
- [x] 正常路径（连接/发现/转发/文案）与异常路径（权限拒绝/不再询问/蓝牙关闭/无候选/扫描取消/数量不符/未知 ID/重复/顺序错/空批次/服务不可达）均有用例。
- [x] 真实环境与主机/模拟证据分开，逐项注明主机/模拟不能证明的真实链路。
- [x] 完整交付（构建、安装、启动、关键用户路径、既有回归、跨端链路）有独立用例。
- [x] 未把“未执行/无前置/原因未知”写成通过。

### 9.2 设计发现（交实现与执行阶段，不在此判产品缺陷）

| ID | 发现 | 影响 | 处置 |
|---|---|---|---|
| F-01 | as-built BLE 发布串为 `time,hum,temp`（逗号、湿度在前），而 IC-002 §6 与 tasks ITEM-003 验收冻结为 `<epoch>:<temp>:<humi>`（冒号、温度在前） | 若实现“保持现状”，转发数据无法被自身 `ReadingAttribution` 或契约消费者解析，且 T-03a 必失败 | 实现按冻结格式改正；测试以冻结格式断言（T-SW-L0-21、L1-19） |
| F-02 | “迁移默认值”与“用户清除后不回退”需要区分“键从未存在”和“键被清除”；仅凭键缺失无法区分 | 用户清除后可能静默回到 `84:C2:E4:03:02:02`，违反 AA-003 §6.2 第 5 步 | 实现需迁移标记（如一次性迁移键）；测试按 T-SW-L0d-09/T-SW-L2-25 两路径判定 |
| F-03 | 现有测试与文档仍写“下降”：`DetailPresentation.kt`、`fragment_device_detail.xml`、`用户使用说明书.md:110/241`、`dengbei_care/readme.txt:26`、`SensorCadence.kt` 注释、`DetailPresentationTest` 注释、`DetailPresentationItem012VerificationTest` 断言 | 只改产品文案会使旧测试失败；删除测试又会掩盖回归 | 实现更正产品/文档/实现侧测试；测试能力在验证阶段把独立验证用例的期望值更正为升温，不得删除或跳过 |
| F-04 | AA-003 `planBinding(selected, boundIds)` 的 `boundIds` 语义未定义（设备 ID 与网关 MAC 不是同一集合） | 绑定状态判定可能无法从外部断言 | 实现须说明该参数的可观察语义；测试只断言 §6.2 明确的未配置/已保存/非法三态，不臆造网关绑定模型 |
| F-05 | BLE 分支的 `0.0/0.0` 端到端只有在可控 BLE 源时才可实测；主机/静态与 MQTT 路径不能替代 | 该真实链路可能保持未验证 | 执行阶段如实记 NOT EXECUTED；不以 MQTT 注入结果冒充 BLE 链路通过 |
| F-06 | `isBluetoothBusy()` 当前在权限缺失时返回 `true`，与“权限缺失”语义混淆 | 可能继续造成静默返回 | 实现须统一使用策略对象并输出真实原因；T-SW-L1-12/T-SW-L2-19 检查 |
| F-07 | App 自身“温度湿度下降报警”与传感器“升温上报”是不同语义，禁词检查若粗暴匹配“下降”会误伤既有功能 | 可能误改报警功能 | 静态检查只禁“较上次下降/下降超过 0.9/下降 >0.9”等上报规则语境；`温度湿度下降报警`、`tempdrop` 等保留并单独断言 |

### 9.3 不阻塞但需执行阶段确认的未知项

| ID | 未知 | 用例处置 |
|---|---|---|
| U-1 | 网关是否广播 FFE0 服务与设备名 | 以“FFE0 过滤优先 + 用户显式无过滤回退”覆盖；记录实际广播；不因未广播判网关故障 |
| U-2 | `mac_addr` 与 BLE MAC 是否同一物理地址 | 两者分别展示与验证，不互推；不把未证实假设写成规则 |
| U-3 | 默认值 `84:C2:E4:03:02:02` 的真实网关是否存在 | 仅作迁移默认并标“未确认”；不作为连接成功判据 |
| U-4 | MIUI 12.5 后台/省电对 180 s 轮询的影响 | T-SW-L2-32 真机长跑；模拟器不能替代 |
| U-5 | `/upload_data` 与 MySQL 持久化/回读 | T-SW-L2-31/REG-09；无获准入口时 NOT EXECUTED |
| U-6 | 固件判据仍与需求冲突（AA-003 U-6） | 固件责任；App 不变更门控；跨端条件上报的“真实触发”结论需固件修复后另测 |
| U-7 | 设备端 instrumented 依赖受阻 | 用 `run-as` 导出替代；仍不可执行则 NOT EXECUTED |
| U-8 | 全暗标定/真实光照证据 | 固件/硬件责任；App 无光照字段 |

---

## 10. 准入与退出准则（最终判定规则）

### 10.1 准入（开始执行的条件）

- 本轮四项实现已完成并有可构建的当前候选；`file_manifest.txt` 已同步。
- L0/L1 可运行（宿主运行器已验证可用）。
- D1/D2、网关、broker/服务按用例前置确认；设备状态在每轮开始与结束时记录。
- 测试数据使用测试账号/测试 devId；不污染生产数据；MQTT 注入使用独立 `clientId`。

### 10.2 逐用例判定

- **PASS**：期望与实际一致，且保存了原始证据；真实链路用例须具备 D/V 证据。
- **FAIL**：期望与实际不一致；附最小复现、现象时间点、日志/截图/数据。
- **NOT EXECUTED**：前置缺失（无工具/未安装/无网关/无网络/无授权/设备状态不满足）；不得记为通过。
- **CONDITIONAL**：期望值依赖未确认产品/实现语义（如 F-04 的 `boundIds`）；记录实测与偏差，保留为未满足项。
- **UNKNOWN CAUSE**：现象异常但根因未定位；保留为未满足项，不归因于“外部条件”。

### 10.3 最终判定（不得豁免）

1. **局部任务**按其自身明确目标判定：该任务的全部必需用例 PASS 才算该任务通过；任一必需用例 FAIL/NOT EXECUTED/CONDITIONAL/UNKNOWN → 该任务**不通过**。
2. **完整交付**只有在**全部必需项**（§2 中“必需=是”）均为 PASS 且证据可复现时才能判为通过；**不得**以“适用用例通过”解释为排除不可执行必需项后的通过。
3. 任何本轮必需项失败、未执行或未验证，由执行测试者返回 **TEST_FAIL**，附当前版本 test 失败报告交实现调查；环境缺口与实现缺陷分别标注，但都保留为未满足项。
4. 真实 BLE / 真实服务链路不得由宿主机单测、模拟器、MQTT 注入或 HTTP 2xx 单独替代结论；对应证据缺失即该必需项未通过。
5. 原因未知不等于外部条件缺失；不得据此提前终止或改写目标。

### 10.4 缺陷归属

- 功能失败、文案/逻辑/接线与验收不符 → 交 Android 实现调查。
- 固件判据、光照标定、433 空口、UART1 → 交固件/嵌入式（TD-002）。
- 电气/器件/上电/信号 → 交硬件测试。
- 服务端部署/数据库/管理配置 → 交服务器责任方；需获准入口。
- 测试工具/依赖/夹具问题 → 本能力按其 test 范围有限修复；超出授权的缺项如实登记。

---

## 11. 停止条件

本能力在 `artifacts/software_test_design.md`（本文件）、`artifacts/software_e2e_plan.md`（E2E-SW-003）产出并登记 `file_manifest.txt` 后结束，返回 `DESIGN_READY`。测试执行、缺陷修复、App 实现、固件/硬件/服务器改动均不属本能力；发现需改动时按 §9 登记交接后停止在能力边界。
