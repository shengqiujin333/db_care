# 需求追溯与核对证据（TR-001）

状态：TRACEABILITY_RECORDED（需求侧证据；不是实板/App/服务器验收证据）
能力：`product_definition.product_requirements`
日期：2026-10-20（UTC）
基线：Git `f38b5a21e5f13120c1b8a09f6b98c7447779d584`；接手/交付前 `git status` 干净
配套：`artifacts/product_requirements.md`（PDR-001）、`artifacts/product_specification.md`（PSPEC-001）

---

## 1. 方法与边界

本能力只做产品需求定义、来源核对和现状登记。实际执行：

- 读取 `readme.txt`、`file_manifest.txt`、`项目地址.txt`、`服务器迁移记录.md`、`gxht40.pdf`（按需）、网表/BOM 描述、既有需求/接口/设计/证据文档；
- 用范围受限的原生 Read/Grep 核对当前源码（未使用 shell grep/find 通配；仓库根无 `.codegraph/` 索引）；
- 运行只读的 `git status` / `git log` / `git show` / `git rev-parse`；
- **未执行**：实板烧录、串口采集、逻辑分析仪采集、ADB 操作、构建、服务器/网络请求、数据库写入。因此本文所有“验证状态”均为**源码/文档核对**，不冒充产品验收。

限制：
- `runtime/environment.yaml` 中本能力可用的 device tools 为空，设备入口 `effective_status` 为 `unavailable_for_current_capability`；本轮不调用硬件。
- 服务器在线性、账号/网络授权未提供证据；未做连通性判断。
- 全暗基准、参考温湿度计、真实温度刺激窗口未提供证据。

---

## 2. 来源清单与身份

| 来源 | 版本/标识 | 用途 | 可信度 |
|---|---|---|---|
| `readme.txt` | 工作树；Git `b38c0af`（最新）；方向文本变更见 `4220a78` | 权威产品需求 8 点 | 权威 |
| Git `4220a78` diff | `readme.txt`：`小于0.9℃` → `大于0.9℃` | 确认现行方向为升温，非笔误 | 权威历史 |
| `readme.txt` 点 8 | `b38c0af` 增加 UART1 调试要求 | PR-010 | 权威 |
| `gxht40.pdf` | V2.6（工作树） | 地址/命令/CRC/换算/量程/时序 | 器件手册 |
| `sensor_hardware/pstxnet.dat` | 工作树 | `PA04=SDA`、`PA03=SCL`、`PB04=LIGHT_ADC`、`PB05=LIGTHT_POWER`、`PB06=NC`、`U9.3` 与 `U7.9` 同 `BAT` | 硬件事实 |
| `sensor_hardware/MAIN_BOARD.BOM` | 工作树 | U9=GXHT40；光敏/分压元件 | 硬件事实 |
| `服务器迁移记录.md` | 工作树 | `8.140.23.253`、8883/5000、证书、认证、`/upload_data`、幂等 | 迁移记录（未在线复核） |
| `artifacts/interface_contract.md` | IC-002 v3.0 | 433/BLE/MQTT/上传接口与边界 | 既有设计（仅一致性核对） |
| `artifacts/firmware_requirements.md` | FWR-002 v5.0 | 固件域需求 | 既有设计（非产品权威） |
| `dengbei_care/readme.txt` | V1.8 | App 既有行为与文案 | 现状 |
| `dengbei_care/用户使用说明书.md` | 1.8 | 用户操作/文案 | 现状 |
| `runtime/environment.yaml` | 本轮 | COM43、手机序列、网关、逻辑分析仪、限制 | 项目环境事实 |
| 现有源码 | HEAD `f38b5a2` | as-built 核对 | 现状 |

构建/镜像背景（仅背景，不作为本轮通过证据）：
- 固件源码最后提交 `8ae4a33`；`evidence/build.md` 记录 axf md5 `5fc16235ccd043f4edb48b16ee44da0b`，`Code=20100 RO=644 RW=116 ZI=1676`。
- `evidence/test.md` EV-017 记录该受测件 7/7 周期 `q=0`、`BUS ack=none`、`gc=3`、VDD≈3.289 V。
- 以上只能说明“上一轮交付件在该板上的状态”，不能说明本轮需求已满足。

---

## 3. readme 修改点 → 需求映射

| readme 点 | 原文要点 | 需求 | 规格 | 现状核对 | 结论 |
|---|---|---|---|---|---|
| 1 | 温度传感器改为 GXHT40 | PR-001、PR-013 | §2.1–2.2、§4 | `gxht40.c` 存在 0x44/0x45/0xFD/双 CRC；EV-017 实板无 ACK | 需求已定义；实板未通过（DEV-03） |
| 2 | 增加光敏电阻和电阻分压 | PR-002 | §2.4、§4 | 网表 PB04/PB05；`light.c` 有供电/ADC/均值/关断；未标定 | 需求已定义；标定缺失（DEV-02） |
| 3 | 采集时间改为 3 分钟 | PR-003、PR-008 | §2.1、§2.7、§3 | `main.c` RTC 1 min ×3；`SensorCadence=180` | 需求已定义；待实板计时验收 |
| 4 | 本次比上次大于 0.9℃ 且无光，或超过 35℃ | PR-004、PR-005、PR-008 | §2.3、§2.6、§3.2 | `fw_core.c` 实为降温且 35.0 提前返回；Android 文案写“下降” | 需求已冻结；实现偏差（DEV-01/DEV-04） |
| 5 | 平台 broker/服务器信息已放入项目 | PR-009 | §3.4、§5.1 | `CloudConfig` 新 IP；迁移记录完整 | 需求已定义；在线未验证（DEV-09） |
| 6 | 同步 Android；不改 iOS；取消光通讯/霍尔；不改硬件 | PR-007、PR-008、PR-011、C-01–C-04 | §3、§7 | Android 已重构解析/告警/上传；sensor 已退役 hall/OPTCFG；网关源码未改 | 需求已定义；需回归证据 |
| 7 | 无光阈值取完全没有光的 1/3 | PR-002 | §2.4 | `light_is_dark()` 用 `3*mean >= C_dark`；但 `C_dark=0`/未标定 | 需求已定义；标定缺失（DEV-02） |
| 8 | UART1 调试串口，初始化并打印 | PR-010 | §2.5 | `sensor_config.h` PA06/PA05 9600；`debug_trace.c` 行渲染；EV-017 使用 COM42 | 需求已定义；入口已变更，须用 COM43 |
| 项目目标 | 设计/实现/独立验证/操作手册/真实条件总功能测试 | PR-014 | §5、§6 | 尚未交付本轮手册与总测 | 需求已定义；待后续能力 |

---

## 4. 需求 → 来源 → 验证状态

图例：
- **DEFINED**：需求已定义，来源明确。
- **AS-BUILT-OK（源码）**：源码与需求一致，但未实测。
- **AS-BUILT-DEVIATION**：源码/文档与需求冲突。
- **NOT-VERIFIED**：无本轮真实证据。
- **REAL-TARGET-FAIL**：真实目标实测失败（EV-017 为旧提交/旧构建，仅背景，不作为本轮结论）。
- **BLOCKED-BY-DATA**：缺必要条件（如全暗基准、服务器授权）。

| 需求 | 来源 | 源码/资料核对 | 状态 | 责任/下一步 |
|---|---|---|---|---|
| PR-001 | readme#1、gxht40.pdf | `gxht40.c` 地址/命令/CRC/换算存在；EV-017 实板 `a44=0 a45=0`、`BUS ack=none` | REAL-TARGET-FAIL（EV-017）/NOT-VERIFIED | 硬件检查/返修 + 重新烧录复测 |
| PR-002 | readme#2/#7、网表/BOM | `light.c` 供电/等待/8 次均值/关断；`light_is_dark()` 1/3；`LIGHT_DARK_CALIBRATED=0` | BLOCKED-BY-DATA | 全暗标定、回填、复测 |
| PR-003 | readme#3 | `main.c` RTC 1 min ×3；`SensorCadence=180` | AS-BUILT-OK（源码）/NOT-VERIFIED | 实板计时验收 |
| PR-004 | readme#4、`4220a78` | `fw_core.c` 使用 `prev-cur>9` 且 `cur==350` 提前 false；Android 文案“下降” | AS-BUILT-DEVIATION | 固件修正 + Android 文案修正 + 向量复验 |
| PR-005 | readme#6、IC-002 §4 | `encrytogate.c` 10 B/Feistel；`measure.c` 200 ms×3 | AS-BUILT-OK（源码）/NOT-VERIFIED | 真实 RF→网关回归 |
| PR-006 | readme#6 | 网关源码本轮无改动；布局与 IC-002 一致 | AS-BUILT-OK（源码）/NOT-VERIFIED | 端到端回归 + `git diff` 证明 |
| PR-007 | readme#6、IC-002 §4/§5 | `GatewayFrameCodec` 16 B 块、u16be/s16be、拒绝非法帧；`MqtttService` 归因 | AS-BUILT-OK（源码）/NOT-VERIFIED | 单测/仪器化 + 真实 BLE |
| PR-008 | readme#3/#4/#6、App V1.8 | `SensorCadence=180`；`AlarmEvaluator` 时间窗；`DetailPresentation` 文案“下降” | AS-BUILT-DEVIATION（文案） | Android 修正 + 回归 |
| PR-009 | readme#5、迁移记录 | `CloudConfig` 新 IP；`UploadOutbox`/`UploadPayload` 存在；未联网 | NOT-VERIFIED | 真机 + 服务器独立核验 |
| PR-010 | readme#8、environment | PA06/PA05 9600；`debug_trace` 行格式；旧证据 COM42 | AS-BUILT-OK（源码）/NOT-VERIFIED | COM43 实板捕获 |
| PR-011 | readme#6、网表 | hall/OPTCFG/params/history 路径已退役（manifest 与源码说明）；PB04/05/06 用途 | AS-BUILT-OK（源码）/NOT-VERIFIED | 静态检查 + 实板启动 |
| PR-012 | readme 产品说明 | 深睡/关 UART/PB05 低/有界发送存在于源码 | AS-BUILT-OK（源码）/NOT-VERIFIED | 被动观测/电流记录（如具备） |
| PR-013 | IC-002 §2/§3 | 有界重试/重探/总线恢复/失败不更新 prev 存在 | AS-BUILT-OK（源码）/NOT-VERIFIED | 故障注入 + 实板恢复 |
| PR-014 | 项目目标 | 本轮尚无手册/总测产物 | NOT-VERIFIED | 下游能力 + 独立验证 |

---

## 5. 关键判据的逐条核对记录

### 5.1 上报方向

- 现行 `readme.txt`：`当本次采集到的温度数据比上次采集的温度数据大于0.9℃...`
- Git `4220a78`（“docs: record updated dengbei requirements”）将该句从 `小于0.9℃` 改为 `大于0.9℃`；`b38c0af` 保留该文本并追加 UART1 要求。
- 因此 PDR-001 冻结为**升温**方向；这不是实现偏好，也不是从旧文档继承。
- 现有固件 `sensor_decide_report(prev, cur)` 返回 `prev - cur > 9`，语义是降温；且 `cur == 350` 提前返回 false。
- 现有 Android `DetailPresentation.REPORT_RULE_TEXT` 与 `dengbei_care/readme.txt` V1.8 第 2 条写“下降超过 0.9℃”。
- 结论：三项实现/文档与权威需求冲突，登记 DEV-01/DEV-04；需求不因此改变。

### 5.2 严格边界

- `大于0.9℃`：严格 `>`；0.1℃ 量化 → 最小 1.0℃。PDR-001 向量 #2/#3 固化。
- `超过了35度`：严格 `>`；35.0 不触发超温；但 35.0 仍可走升温分支。PDR-001 向量 #7/#10 固化。
- 现有 `fw_core.c` 在 `cur == 350` 直接 `return false`，会把 34.0→35.0 且无光的应上报场景判为不上报；这是 DEV-01 的一部分。

### 5.3 1/3 无光判据

- `readme.txt` 点 7：`把没有光的阈值改为完全没有光的时候的1/3`。
- 网表/BOM：PB05→R3 5M→PB04→光敏电阻→GND；暗 → 阻值大 → ADC 码高。
- 现有 `light_is_dark()`：`3*mean >= C_dark`（整数、边界相等为暗），与“1/3 阈值”一致。
- 现有配置 `LIGHT_DARK_REF_CODE=0`、`LIGHT_DARK_CALIBRATED=0`；`valid` 恒 false。故升温分支当前不可用。
- 这是**数据缺失**（无全暗实测），不是判据本身需要变更；登记 DEV-02/OQ-01。

### 5.4 3 分钟节拍

- `main.c`：RTC 1 分钟中断累计 `SENSOR_SAMPLE_TICKS=3` 置 `sample_flag`；`sample_flag=1` 仅消费一次。
- `SensorCadence.SAMPLE_PERIOD_SECONDS=180`；BLE 读取周期使用该值。
- 源码一致；未做真实计时。LSI 精度与允许误差见 OQ-03。

### 5.5 调试串口

- `sensor_config.h`：`DEBUG_UART_BaudRate=9600`、PA06 TXD/PA05 RXD、`SENSOR_DEBUG_UART_MAXLINE=96`；ARM 构建默认开。
- `debug_trace.c/h`：BOOT/VDD/IOTEST/IOSIG/BUS/S/G 行渲染；manifest 记录了行预算。
- `runtime/environment.yaml`：本轮调试串口为 `COM43` 9600 8N1；旧 EV-017 使用 COM42，不能复用。
- 未做本轮捕获；PR-010 仍为 NOT-VERIFIED。

### 5.6 服务器与上传

- `CloudConfig`：`8.140.23.253`、8883、5000。
- 迁移记录：EMQX 5.8.0、MySQL 9.0.1、`abr.py`、CA `jd-ca.crt`、认证 `MAC`/`MAC&^!A:z?`、`sensor_data` 唯一键。
- 未联网、未查库；PR-009 为 NOT-VERIFIED。记录中的明文 SSH/DB/Dashboard 凭据登记为安全项 OQ-10，未写入 App。

---

## 6. 与既有工件的冲突处理

| 既有工件 | 内容 | 与 PDR-001 关系 | 处理 |
|---|---|---|---|
| `artifacts/interface_contract.md` v3.0 | 冻结升温 `cur-prev>9`、35.0 可走升温分支、1/3 判据 | 一致 | 作为接口一致性来源引用 |
| `artifacts/firmware_requirements.md` v5.0 | 正确冻结升温/边界/标定门槛 | 一致 | 引用；不作为产品权威 |
| `artifacts/android_architecture.md` v1.0 | 多处按“下降 >0.9℃”描述本轮上报 | 冲突 | 登记 DEV-05；由 Android 能力修订 |
| `dengbei_care/readme.txt` V1.8 | 第 2 条写“下降超过 0.9℃” | 冲突 | 登记 DEV-04；用户可见文案须修正 |
| `evidence/test.md` EV-017 | 受测件 `8ae4a33` 实板 7/7 失败 | 不构成需求变更 | 登记 DEV-03；仅背景 |
| `evidence/build.md` | axf md5/构建身份 | 不构成功能通过 | 仅引用构建身份 |
| `artifacts/codegraph_index.md` | 历史索引；仓库根当前无 `.codegraph/` | 无 | 未使用图查询，改用有界原生读取 |

---

## 7. 证据缺口与不可宣称事项

本轮**不能**宣称：

1. GXHT40 在真实目标上采集正常（EV-017 为失败，且为旧提交镜像）。
2. 无光判据已标定/已验收（`C_dark` 未实测）。
3. 条件上报规则已在固件/Android 正确实现（DEV-01/DEV-04）。
4. UART1 本轮已在 COM43 验证（仅源码核对）。
5. App 已连接新服务器、已上传云端、已真机运行（未执行）。
6. 网关端到端兼容已回归（未执行）。
7. iOS 兼容已验证（未执行，仅要求不改）。
8. 操作部署手册已交付或已独立核验（未执行）。
9. 真实条件下产品总功能测试通过（未执行）。

上述事项分别由后续实现/嵌入式测试/软件验证/独立验证/运维能力完成；本能力不越权代替，也不提前宣告交付。

---

## 8. 自检清单

| 检查 | 结果 |
|---|---|
| 每条 readme 修改点都有需求 ID | 是（§3） |
| 每条 P0 需求有可观察验收行为 | 是（PDR-001 §2/§9） |
| 边界值（0.9、35.0、1/3、35.0→35.1、负温）明确 | 是（PR-004/PR-002/PR-007） |
| 异常/恢复/不伪造有明确行为 | 是（PR-013、PSPEC §4） |
| 现有实现与需求冲突已登记且未取消需求 | 是（PDR-001 §8） |
| 未把未提供资源写成已有条件 | 是（PDR-001 §7、本文 §7） |
| 未把旧运行/旧 COM42 证据当作本轮结果 | 是（§2/§5.5） |
| 工件未包含 Runtime ID/路由/元数据 | 是 |
| 产品源码零改动 | 是（仅新增 3 个文档 + 更新清单） |
| 文件清单已维护 | 是（新增 3 条） |

---

## 9. 交付物

| 文件 | 角色 |
|---|---|
| `artifacts/product_requirements.md` | RequirementsArtifact（PDR-001） |
| `artifacts/product_specification.md` | ProductSpecification（PSPEC-001） |
| `evidence/product_requirements_traceability.md` | RequirementTraceabilityEvidence（TR-001） |

本能力完成后由 Runtime 负责提交/发布；本能力不执行 `git add/commit/push`。
