# 固件实现（FWI-004 rev 9）

状态：固件实现（负责角色：固件工程）
本轮范围：2026-11-13 **无代码改动交付（无需修改确认）**。对当前任务、生效测试范围（TD-002 rev 9.0）、最新实现与失败证据逐项核对后，判当前源码已覆盖本阶段可实施范围，**无具体代码改动需要实施**；本轮交付实现位置/版本/核查依据/已执行与未执行的自检，不把"无需修改"写成测试通过。**未烧录、未复位、未采集串口、未调温/改光、未测功耗、未访问平台。**
依据：`artifacts/firmware_design.md`（FD-002 rev 8）、`artifacts/firmware_requirements.md`（FWR-002 rev 8）、`artifacts/test_design.md`（TD-002 **rev 9.0**）、`readme.txt`、`evidence/product_acceptance_20261112_scope_recheck/PRODUCT-ACCEPTANCE-RECHECK-002.md`、`evidence/product_acceptance_20261112/FIRMWARE-TRIAGE-003.md`、`artifacts/failure_resolution_product_stage_delivery_gate.md`（FR-006）、`evidence/fw_impl_20261112/assignment_referral.md`（FW-REFUSAL-002）。

---

## 0. 本轮结论

| 项 | 结果 |
|---|---|
| 是否有代码改动 | **无**。产品源 `82a7e74..HEAD`（`USER/MDK/EWARM/UM2005C/COMMON`）为空；工程文件未改 |
| 是否新增测试专用代码 | **无**。TD-002 rev 9.0 明确"当前证据表明不需要固件新增测试接口"（BOT-01 已由测试侧脚本以 pyocd HW reset 自动完成） |
| 实现位置/版本 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output`（产品源提交 `82a7e74`）；交付包 `fw_release/fw_sensor_FD-002r8.zip` |
| 产物身份 | axf `b6a15fed2ed5e3d0c4adc51ac87a4830`、hex `4d728dec9b0d28a352c46436b7586c72`；包 134,791 B / `5df0c6d4…`（本轮重建与包内成员一致） |
| 已执行自检 | 独立重建 0E/1W（`Code=25964 RO=656 RW=116 ZI=1676`）；宿主 5 阶段 **222/0**；工程源列表无 `test/mock`；包身份复核 PASS |
| 未执行自检 | 烧录/复位/COM43 采集、真实升温与 `>35.0℃` 触发、光强双态、跨链路、平台写入回读、功耗（均非本能力当前条件；明细见 §5） |
| 结论含义 | 本次 `IMPLEMENTED` 仅表示**当前实现已交付且自检可复现**，**不是**需求 4 无光分支可用、**不是**产品验收通过；`RECHECK-002` 的 `TEST_FAIL` 仍有效 |

## 1. 生效的测试范围与失败证据（本轮核对）

1. `artifacts/test_design.md`（TD-002 rev 9.0）已按人工范围决定调整：`FW-LIGHT-DEF2-ENABLE`（`APPROVED_DEFERRED`/NOT EXECUTED）被**单列并排除出本阶段退出门槛**；本阶段范围为 ITEM-001/002/003/004/006；真实升温/超温、光强双态、功耗、跨链路、平台写入/回读为**阶段外项**；并明确"阶段通过 ≠ 完整产品通过"。
2. `PRODUCT-ACCEPTANCE-RECHECK-002` 仍为 `TEST_FAIL`（`EXTERNAL_CONDITION_UNEXECUTED`），但已修正原报告 R4 的范围错误：正确分解为"上报公式实现一致 / 无光判据启用 NOT EXECUTED（人工延期）/ 真实升温超温 NOT EXECUTED（外部条件）"。
3. `FIRMWARE-TRIAGE-003` 结论 `EXTERNAL_CONDITION_MISSING`：无实测证据表明固件实现需要修复；直接回填候选阈值 3590 会把设计禁止的猜测升级为返工。
4. `FR-006`（项目协调，`REPLAN_REQUIRED`）确认：固件拒接依据成立，再次分派固件返工只会重复同一结论；剩余缺口是产品级验收/交付门禁与联调职责，不是固件代码。

因此本轮**不因人工已跳过的测项或阶段外项修改代码**，也不为产生改动而重复改代码。

## 2. 现行实现与任务清单对照（本阶段）

| 任务 | 状态 | 依据 |
|---|---|---|
| FW-REG-ASBUILT | 已实现并回归（本轮重建 + 摘要一致） | `evidence/fw_impl_20261113/build_evidence.md` |
| FW-REG-REPORT | 已实现并回归（宿主 222/0 含 T1/T2/T3 向量） | `evidence/fw_impl_20261113/driver_test_evidence.md` |
| FW-LIGHT-DEF2-MECH | 已实现（`T=0` 默认，形制与旧宏清除已核） | `USER/inc/sensor_config.h`、`USER/src/fw_core.c`、`light.c` |
| FW-OBS-PASSIVE | 已由测试角色在实板执行（同 axf/hex） | EV-036/037/038 |
| FW-LIGHT-DEF2-ENABLE | **NOT EXECUTED（人工 `APPROVED_DEFERRED`）**；不作为返工依据 | `evidence/fw_impl_20261112/assignment_referral.md` §2/§5 |
| FW-DIAG-BOOT | 已由测试角色自动编排取得五行（无需固件测试接口） | EV-038、TD-002 rev 9.0 |

## 3. 本轮执行与未执行的自检

**已执行**（可直接复跑，证据在 `evidence/fw_impl_20261113/`）：

1. 源身份：`git diff --stat 82a7e74 HEAD -- <sensor>/{USER,MDK,EWARM,UM2005C,COMMON}` 为空；
2. 独立重建：`mdk_build {"action":"rebuild"}` → 0E/1W，`25964/656/116/1676`，axf/hex 摘要同包；
3. 实现侧宿主自检：`bash test/build_test.sh` → 5 阶段 222/0、exit 0；
4. 包/产物身份：zip `5df0c6d4…`、`PACKAGE.txt` `a58f67fc…`、成员摘要与重建一致；
5. 隔离核查：`Project.uvprojx` 的 27 个源项无 `test/`/`mock/` 路径。

**未执行**（保留为未测范围，不写成通过）：烧录/复位/COM43 采集（本能力无 `mdk_flash`/`serial_capture`）；真实升温与 `>35.0℃` 触发（用户禁止调温）；光强双态（人工跳过）；网关→手机跨链路（后续联调）；平台 `/upload_data` 写入/回读（无授权）；CR2032 功耗（无仪器）。

## 4. 测试专用实现与正式固件隔离（本轮清单，按契约五项）

1. **新增/修改的测试专用文件、函数/符号**：无。
2. **仅测试固件启用的编译定义与调用方式**：无新增（既有 `SENSOR_DEBUG_UART` 是产品调试通道，非测试专用开关；`-DLIGHT_DARK_THRESHOLD_CODE` 仅供宿主变体验证阈值形制）。
3. **正式固件中排除的构建规则**：`test/` 与 `mock_*` 不在 Keil 工程源列表（27 项核对），仅在宿主 `test/build_test.sh` 内编译；正式镜像仅由工程保存配置生成。
4. **验证正式镜像排除入口的方法与结果**：重建后 axf/hex 摘要（`b6a15fed…`/`4d728dec…`）与发布包成员逐字节一致，且工程源列表无测试路径——两者共同表明正式镜像不含测试代码；本轮未新增可被链接的测试符号。
5. **测试版与正式版构建配置/提交/产物**：正式构建 = `MDK/Project.uvprojx` + ARMCLANG V6.24，提交 `82a7e74`，产物 `MDK/output/exe/Project.axf|hex`（摘要同上）并已打包为 `fw_release/fw_sensor_FD-002r8.zip`；宿主测试构建 = `test/build_test.sh`（MinGW-w64 gcc 12.2.0，产物在 `test/_hostbin/`，不入库、不进包）。

## 5. 未测范围与现存问题（不因本轮交付而改变）

| 项 | 状态 |
|---|---|
| 需求 4 的"升温 >0.9℃ 且无光"分支 | 交付默认 `T=0` ⇒ 不触发；启用需 IC-002 §4.2 接受记录 + 同板同构建被动双态证据（人工已延期，不得再排） |
| 真实升温/`>35.0℃` 触发与 433 实发 | 未执行（用户禁止调温） |
| 光强双态 | NOT EXECUTED（人工 `APPROVED_DEFERRED`） |
| 网关→手机跨链路、平台写入/回读 | 未执行（后续联调/无授权） |
| CR2032 峰值/深睡电流 | 未测（无仪器，已批准 DEFERABLE） |
| 板端镜像身份 | 由测试角色以同摘要下载校验（EV-036/037/038），不属本能力行为 |

## 6. 给独立测试者的复验方法

```text
# 1) 源身份（期望：无输出）
git diff --stat 82a7e74 HEAD -- CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/{USER,MDK,EWARM,UM2005C,COMMON}

# 2) 重建（期望 0E/1W；axf b6a15fed2ed5e3d0c4adc51ac87a4830）
mdk_build {"action":"rebuild"}

# 3) 宿主自检（期望 5 阶段 222 项 0 失败、exit 0）
bash CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test/build_test.sh

# 4) 包身份（期望 5df0c6d45ecddc8aae8bddbbcfc9cfdf）
md5sum fw_release/fw_sensor_FD-002r8.zip
```

实板复验须由具备烧录与采集入口的职责重新下载并采集（不得沿用本轮，也不得把宿主结果当实板通过）；异常路径建议：`q=0` 时取同周期 `G` 行核对结果码/ACK/重读/`raw`，失败周期应不报/不推进前值/不构造 0。

---

# 固件实现（FWI-003 rev 8，历史保留，不改写）

状态：固件实现（负责角色：固件工程）
本轮范围：2026-11-10 **FW-REG-ASBUILT → FW-REG-REPORT → FW-LIGHT-DEF2-MECH**：先用当前源码重建确认 as-built 基线身份（未改源码时产物摘要与历史交付逐字符一致），再按 FD-002 rev 8 §6.2 实现无光判据形制 **LIGHT-DEF-2**（绝对阈值、单一配置常量、交付默认 `T=0` 即判据未启用），并同步实现侧自检。**未改空口/BLE 布局、采样节拍、`S/G/P` 行格式、引脚所有权；未烧录/下载/复位；未做 COM43 实板观测、判据启用、上电五行与跨链路。**FW-OBS-PASSIVE / FW-LIGHT-DEF2-ENABLE / FW-DIAG-BOOT 不在本能力当前可执行条件内（§6）。
依据：`artifacts/firmware_design.md`（FD-002 rev 8 §3.2/§6.2/§12/§13）、`artifacts/firmware_requirements.md`（FWR-002 rev 8，FWR-110/120/121/122/123）、IC-002 v4.0 §4.2、`artifacts/test_design.md`（截至 HEAD，含 MCH/ASB/EN 用例）、`gxht40.pdf` V2.6、网表/BOM 原文。

---

## 0. 本轮结论（一页）

| 项 | 结果 |
|---|---|
| 设计增量是否已实现 | **是**：`light_is_dark` 由 1/3 形制改为绝对阈值形制；配置收敛为单一常量 `LIGHT_DARK_THRESHOLD_CODE`（0=未启用）；旧宏已删 |
| 交付行为是否改变 | **否**：`T=0` ⇒ `valid=false`、`dark=false`，与 as-built（`LIGHT_DARK_CALIBRATED=0`）等价；判定/上报/空口/节拍/格式/资源未变 |
| 是否启用无光判据 | **否**：`T` 保持 0。启用需判据数值接受记录 + 同机同位置被动双态证据（FWR-120），本轮不具备 |
| 构建 | Keil `0 Error / 1 既有 Warning`；`Code=25964 RO=656 RW=116 ZI=1676`；axf `b6a15fed…`、hex `4d728dec…`（基线 `a3b08774…`/`546ce20a…`）。`gcc/build.sh` 0 error（27 项既有 warning），FLASH 34840 B |
| 实现侧自检 | 宿主 5 阶段 **222 项 0 失败**（基线 218/0） |
| 实板/下载 | **未执行**：`mdk_flash`/`flash_device` 与本能力 `sensor_programming` 入口均为 `unavailable_for_current_capability`；`serial_capture` 同样未授予 |
| 已知跨角色影响 | `test/host_fw_core_verify_ev.c`（旧 1/3 断言将失败）与 `test/host_vdd_verify_ev.c`（引用已删宏，将编译失败）需验证角色同步；本轮未修改其资产 |

## 1. 本轮任务与身份

| 项 | 值 |
|---|---|
| 接手仓库 HEAD | `ef5d1af`（接手时 `git status --porcelain` 为空） |
| 产品源码起点 | 与 HEAD 一致、无本地改动（基线重建摘要 `a3b08774…` 证明，见 `evidence/fw_impl_20261110/build_evidence.md` §2） |
| 本轮任务（按依赖顺序） | FW-REG-ASBUILT → FW-REG-REPORT → FW-LIGHT-DEF2-MECH |
| 授权工具 | `mdk_build`（设备后端）；原生 read/edit/write/grep/find/ls/bash（编译与局部自测） |
| 未授权的相关入口 | `mdk_flash`（下载）、`serial_capture`（COM43）、`sigrok_capture`、`android_test`、`inspect_existing_service_readonly`、`flash_device`/`operate_test_equipment`（外部动作） |

## 2. 本轮实际改动

产品源码（6 个文件；行为仅在 `T!=0` 时不同，`T=0` 时对外等价）：

| 文件 | 改动 |
|---|---|
| `USER/inc/sensor_config.h` | 删除 `LIGHT_DARK_REF_CODE`/`LIGHT_DARK_CALIBRATED`；新增 `LIGHT_DARK_THRESHOLD_CODE`（默认 `0u`＝未启用，`1..4095`＝阈值），注释写明形制、来源、候选 3590／带 (3514,3660]、启用门禁与禁用语义 |
| `USER/src/fw_core.c` | `light_is_dark(mean, valid, threshold_code)`：`!valid`→false；`threshold==0`→false（防御）；`mean >= threshold`（边界相等为暗） |
| `USER/inc/fw_core.h` | 接口注释改为 LIGHT-DEF-2 语义，参数名改为 `threshold_code` |
| `USER/src/light.c` | `valid = adc_ok && (LIGHT_DARK_THRESHOLD_CODE != 0)`；`dark = light_is_dark(mean, valid, (uint16_t)LIGHT_DARK_THRESHOLD_CODE)`；文件头说明更新 |
| `USER/inc/light.h` | `light_result_t` 字段语义注释更新（`valid` = adc_ok 且判据已启用） |
| `USER/inc/debug_trace.h` | 两处字段注释更新为阈值语义（仅注释，不改格式） |

实现侧自检（4 个文件，属实现角色资产）：

| 文件 | 改动 |
|---|---|
| `test/host_fw_core_pure_check.c` | `[5]` 段改为阈值形制向量（`T=0`、`T=1`、`T=3590` 边界、实测 3514/3987、`T=4095`、`T=1..4095` 全量）；交付常量断言改为 `== 0u` |
| `test/host_sensor_core_test.c` | `test_light()` 同步为阈值形制向量 |
| `test/host_light_check.c` | 打印行改为 `LIGHT_DARK_THRESHOLD_CODE`；`[5]` 段改为「未启用门禁 vs 启用后阈值」，启用变体新增 `mean==T`/`mean==T-1` 与实测参考值断言 |
| `test/build_test.sh` | `[4/4b]` 变体改用 `-DLIGHT_DARK_THRESHOLD_CODE=3590u`；`[1/4]`/`[4/4a]` 描述同步 |

明确未改动：`measure.c`/`debug_trace.c`/`main.c`/`gxht40.c`/`sf_i2c.c`/`encrytogate.c`、`UM2005C/*`、工程文件（`Project.uvprojx`/`.ewp`）、网表/BOM、网关工程、Android 工程；验证角色资产 `test/*_verify_ev.c`、`test/mock_*_ev/`、`evidence/embedded_verify_*.py` 零修改。

行为差异说明：`T=0` 时唯一路径变化是 `light_is_dark` 多了 `threshold==0` 的提前返回——该分支与随后分支返回值都是 `false`，因此 `light_result_t.valid/dark`、`S` 行 `V/D`、`r/s/y`、冻结与发送、节拍均与 as-built 相同。

## 3. 构建与自检

| 项 | 基线（未改源码） | 本轮（LIGHT-DEF-2） |
|---|---|---|
| Keil rebuild | 0E/1W；`25964/656/116/1676`；axf `a3b08774e764f174eac318f6771d0c42`、hex `546ce20a718fa69b40177cce0145f4b5` | 0E/1W（同一既有 `main.c(254)` 告警）；`25964/656/116/1676`；axf `b6a15fed2ed5e3d0c4adc51ac87a4830`、hex `4d728dec9b0d28a352c46436b7586c72` |
| `gcc/build.sh` | 0 error / 27 warning；FLASH 34836 B / RAM 1816 B | 0 error / 27 warning；FLASH 34840 B / RAM 1816 B |
| `test/build_test.sh` | 218 项 0 失败（46+61+54+28+29） | **222 项 0 失败（48+61+54+28+31）** |

证据：`evidence/fw_impl_20261110/build_evidence.md`（BUILD-003）、`driver_test_evidence.md`（DRV-003）、`md5_list.txt`、`mdk_build_*.out.json`、`gcc_build_*.log`、`host_build_test_*.log`、`args/*.json`。

## 4. 与设计验收项逐项对照（ITEM-003 / MCH）

| 验收项 | 结果 | 依据 |
|---|---|---|
| 形制向量：`T=0`→false；`mean=T-1/T`→false/true；`T=4095` 仅满量程；`valid=false` 恒 false | 通过 | 宿主 `[1/4]` 48 项（含 `T=1..4095` 全量） |
| 接线 `valid=adc_ok&&(T!=0)`、`dark=light_is_dark(mean,valid,T)` | 通过 | `light.c` diff + 宿主 `[4/4a]/[4/4b]` 59 项 |
| ADC 全超时 → `valid=false/dark=false`，不合成满量程 | 通过（宿主） | `[4/4a]/[4/4b]` 全超时用例 |
| 未启用等价性（`V=0 D=0`、格式/时序/布局不变） | 通过（源码+宿主）；实板未复观测 | §2 说明 + `measure.c`/`debug_trace.c` 零 diff；实板属 FW-OBS-PASSIVE（未执行） |
| 旧宏清除（`USER/` 与实现侧测试无残留、无同义别名） | 通过 | grep 无命中；`debug_trace.h` 遗留注释已改 |
| 不引入滞回/温补/自适应/跨周期状态 | 通过 | `light_is_dark` 纯比较；`light_result_t` 未增字段 |
| 三套构建 0 Error | 通过 | §3 |
| 未修改验证资产 | 通过 | `git status`：仅 10 个文件改动，其中 0 个 `*_verify_ev.c`/`mock_*_ev/` |

## 5. 跨角色同步清单（本轮只登记，不代改）

| 资产/文档 | 现状 | 需要的动作 |
|---|---|---|
| `test/host_fw_core_verify_ev.c` | 按 `3*mean >= c_dark` 断言（含 `c_dark=0 → 每个 valid 样本 DARK`、1365 边界、穷举网格） | 验证角色按 FWR-121 改为 `mean >= T` 向量；否则将出现非产品缺陷的失败 |
| `test/host_vdd_verify_ev.c:261` | 引用 `LIGHT_DARK_CALIBRATED` | 改为 `LIGHT_DARK_THRESHOLD_CODE == 0 ? (r.valid == false) : true`，否则编译失败 |
| `test/host_light_verify_ev.c`、`test/host_rf_report_verify_ev.c` | 此前已登记为陈旧不可编译 | 与本轮无关，仍待重写 |
| `artifacts/interface_contract.md` §4.2/§10 | 仍描述旧判据且声明“不批准替代阈值” | 判据被接受后由系统架构同步；固件在同步前保持 `T=0` |
| 测试侧 `test_design.md` | 已由测试角色同步至 LIGHT-DEF-2 | 无需本角色动作 |

## 6. 未执行项、原因与所需条件

| 项 | 状态 | 原因/所需条件 |
|---|---|---|
| 下载/烧录（`mdk_flash`） | 未执行 | `sensor_programming` 对当前能力 `unavailable_for_current_capability`（缺 `mdk_flash` 设备工具 + `flash_device` 外部动作）；未申请、未绕过 |
| COM43 实板观测（FW-OBS-PASSIVE） | 未执行 | `serial_capture` 未授予；且需先下载新镜像，二者都不在本能力条件内 |
| 判据启用（FW-LIGHT-DEF2-ENABLE） | 未执行 | 前置不满足：无判据形制/数值接受记录（IC-002 §4.2 未同步），也无同机同位置被动双态证据；按 FWR-120 保持 `T=0` |
| 上电五行/`uid`（FW-DIAG-BOOT） | 未执行 | 属测试/调试编排；pyocd 复位入口未在 `runtime/environment.yaml` 登记 |
| 真实升温/`>35.0℃` 触发、跨链路、功耗 | 未安排 | 用户约束：不得调温/改变光强/测功耗；跨链路按用户安排进入后续联调阶段 |
| 判据分离度实板复核 | 未执行 | 需被动观测窗口内的 `a` 与 `V/D`；本轮只提供数据来源与判据 |

## 7. 独立复验方式（给测试者）

1. **构建/自检复跑**（无需设备）：

```text
bash CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test/build_test.sh
  期望：5 阶段 222 项 0 失败、退出码 0
bash CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/gcc/build.sh
  期望：0 error（27 项既有 warning）；FLASH 34840 B / RAM 1816 B
mdk_build {"action":"rebuild"}
  期望：0 Error/1 既有 Warning；axf md5 b6a15fed2ed5e3d0c4adc51ac87a4830
```

2. **形制向量单独复核**（不依赖本角色断言时）：
   - `light_is_dark(4095u, true, 0u) == false`（未启用）；
   - `light_is_dark(3589u, true, 3590u) == false`、`light_is_dark(3590u, true, 3590u) == true`；
   - `light_is_dark(4094u, true, 4095u) == false`、`light_is_dark(4095u, true, 4095u) == true`；
   - `light_is_dark(x, false, T) == false`（任意 `x/T`）。

3. **实板（需先取得下载与 COM43 采集授权）**：下载 `b6a15fed…` 对应镜像后，在**不改变光强/温度**的前提下被动采集 `S` 行：`T=0` 预期 `V=0 D=0`、`q=1`、`k` 步长 3、`r/s/y=0`，`a` 记录实际码值；若观察到 `V=1` 或 `D=1`，即为交付语义不符，必须带原始捕获回退。

4. **禁用做法**：不要用验证角色旧 harness 的 1/3 断言作为新判据的通过依据（见 §5）；不要以修改 `sensor_config.h` 阈值来让实板观测“看起来通过”。

## 8. 尝试记录与本轮自我限制

| # | 动作 | 结果 |
|---|---|---|
| 1 | 基线重建（未改源码） | 0E/1W，摘要与历史交付一致 → 起点身份成立 |
| 2 | 基线宿主自检 | 218/0（含旧 1/3 向量） → 回归基线成立 |
| 3 | 实现 LIGHT-DEF-2（源码 + 实现侧自检） | 完成；宿主 222/0；Keil/gcc 双构建 0 error |
| 4 | 下载/COM43 | 未执行（授权缺项，见 §6） |
| 5 | 判据启用 | 未执行（接受记录与双态证据缺项） |

本轮没有重复历史已否定的修复（未重写 GXHT40 驱动、未改 `delay`、未改 FX2 地址、未改 1/3 阈值拟合）；没有为了通过自检而降低断言或修改验证资产。

---

# 固件实现（FWI-002 rev 7，历史保留，不改写）

状态：固件实现（负责角色：固件工程）
本轮范围：2026-11-09 当前轮 **FW-REG-ASBUILT / FW-REG-REPORT 回归**——确认 as-built 源码已实现 T1–T4/F-1；重建 Keil 工程并完成实现侧宿主自检。**未修改产品源码、未烧录/下载、未做 COM43 实板/跨链路验证**；FW-REG-GXHT40、FW-T5-LIGHT、FW-DIAG-BOOT、FW-E2E-POWER 因当前能力无设备/现场条件未执行，交测试能力按 TD-002 rev 7 执行。
依据：FD-002 rev 7、FWR-002 rev 7、FW-EXP-002 rev 2、TD-002 rev 7、IC-002 v3.0、`gxht40.pdf` V2.6

---

## 1. 当前轮身份与范围

| 项 | 值 |
|---|---|
| 仓库 HEAD | `fdfcd852b6282987d022ac7936bc8ba0d6cba5d9` |
| sensor 工程 tree | `09bef6581f39ca50ba2ecfbaa5c77ef9accd68a5` |
| `USER/` tree | `746a72d4cc995490341270441792cae85727e923` |
| 与设计基线 `ef5caa6` 的产品源码差异 | `git diff --stat ef5caa6 HEAD -- USER/ test/` 为空 |
| 本轮任务 | FW-REG-ASBUILT、FW-REG-REPORT（其余任务不属于本能力当前可执行条件） |
| 未执行 | 下载/复位/COM43/逻辑分析仪/手机/网关/功耗测量；当前能力仅授权 `mdk_build` |

当前源码已包含（静态核对，未重写）：
- T1：`sensor_decide_report` 升温方向、严格 `>35.0℃`、int32 差值；常量 `SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350`。
- T2：`measure.c` 冻结 `s_report_temp_x10/s_report_hum_x10`，组帧只读快照；`debug_trace.c` 输出 `P` 行。
- T3：`debug_trace.c` 的 `TXE/TXBUSY` 等待使用 `SENSOR_DEBUG_TX_GUARD` 有界。
- T4/F-1：GXHT40 0x44/0x45 探测、0xFD、6 B、双 CRC、有效域、重探/恢复；`S` 行 `p/H` 为判定前快照。
- 未标定门禁：`LIGHT_DARK_REF_CODE=0`、`LIGHT_DARK_CALIBRATED=0` 保持；暗态 3914/3915/3932 仅作候选，不据此回填。

## 2. 实际改动

**产品源码、固件工程配置、实现侧测试断言、验证角色资产：本轮零改动。**

- 没有重写已符合项；没有改 CRC/量程/判据/空口/BLE/UART 格式。
- 未修改 `test/*_verify_ev.c`、`test/mock_*_ev/`、`evidence/embedded_verify_*.py`。
- 仅新增本实现记录与 `evidence/fw_impl_20261109/` 构建/宿主自检证据。
- `host_fw_core_pure_check` 的编译使用 `-include ../USER/inc/sensor_config.h`（该 harness 注释的编译行缺此头），未改测试文件；`host_rf_frame_check` 链接补齐既有 `bleencrypt.c`，未改产品源码。

## 3. 构建结果

| 项 | 结果 |
|---|---|
| 命令 | `mdk_build {"action":"rebuild"}` |
| 编译器 | ARMCLANG V6.24 |
| 结果 | **0 Error / 1 Warning**（`main.c(254)` 空循环体，既有） |
| 资源 | `Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 产物 | `Project.axf` md5 `a3b08774e764f174eac318f6771d0c42`；`Project.hex` md5 `546ce20a718fa69b40177cce0145f4b5` |
| 原始日志 | `evidence/fw_impl_20261109/mdk_rebuild.result.txt`、`build_evidence.md` |

与 FW-EXP-002 rev 2 记录一致；因产品源码未改，重建可复现同一摘要。构建成功不等于实板功能通过。

## 4. 实现侧宿主自检

| 入口 | 结果 |
|---|---|
| `test/build_test.sh` | **218 passed / 0 failed**（46+61+54+28+29），exit 0 |
| `host_gxht40_check` | **60 / 0** |
| `host_sf_i2c_bus_check` | **42 / 0** |
| `host_rf_frame_check` | **22 / 0** |
| `host_fw_core_pure_check` | **46 / 0** |
| **合计** | **388 passed / 0 failed** |

覆盖 T1 九组向量/严格边界/int16 极值、T2 冻结/重试/跨采样/不覆盖/P 行、T3 卡死有界、T4/F-1 地址/CRC/换算/重探、光照未标定门禁与 1/3 边界、空口寄存器布局/网关 8 B 映射。原始日志与可复跑命令见 `evidence/fw_impl_20261109/driver_test_evidence.md`。

## 5. 未执行与交接

- **FW-REG-GXHT40**：当前构建真实目标连续多周期 `q=1`/失败路径需 `mdk_flash` + COM43，本能力无入口，未执行；不得用宿主结果替代。
- **FW-T5-LIGHT**：现场无光、无受光/遮光双态；保持 `0/0`，暗态候选不回填；待测试阶段按 TD-002 §5.4 执行。
- **FW-DIAG-BOOT / FW-E2E-POWER**：需先开捕获窗口复位、网关/手机同窗、功耗仪器；本能力未执行。
- 交接给测试：按 TD-002 rev 7 的 ASB-01..06、T1/T2/T3、T4/T5/E2E 用例复验；实现侧产物摘要/构建日志/宿主自检日志已入库。

---

> 以下 `# 修复轮 run14` 起为历史记录（rev 6.3/6.4，EV-026/EV-027/EV-028），保留原窗口、版本和结论；不得用历史 PASS 替代本轮实板结果。

# 修复轮 run14：`S` 行前值语义修正（F-1）

## 1. 本轮输入

| 证据 | 结果 |
|---|---|
| EV-026（rev 6.3，axf `917bffe6…`） | E1 实板成立：`delay` 恢复、STOP→读 START 32.525 ms、SCL 614.5 µs；16 个有证据周期 `q=1`、0 个 `G`；整体仍 `TEST_FAIL`（T4-02 上电五行/横幅、T5 全暗、INT 未执行），并登记 F-1/F-2/F-3 |
| EV-027（同镜像，长时+位级） | 新增 3 个 `S` + 1 个位级事务，均 `q=1`、双 CRC 匹配；F-1 直接实板证据：3 条新行 `p==t`、`H=1`，上电首行 `k=0 p=241 H=1`（本应 `H=0`）；F-2 重估落在实测稳态区间；F-3 两次独立实测 1.626× |

本轮只改 F-1；不重复已验证的 D2/E1，不因 F-1 修改判定/CRC/帧格式。

## 2. 实际改动

| 文件 | 改动 |
|---|---|
| `USER/src/measure.c` | 新增 `s_trace_prev_x10`、`s_trace_have_prev`；在 `temperature_process()` 推进 `s_prev_temp_x10/s_have_prev` **之前**缓存判定所用前值；`sensor_trace_fetch()` 的 `p/H` 改为读该快照。判定仍使用原变量，逻辑不变。 |
| `test/host_measure_flow_check.c` | 新增两条断言：首样本 `H=0/p=0`；第二周期 `p=200`（上一周期），即使本周期 `t=209` 也不被覆盖。 |

保持不变：D2 `SENSOR_I2C_DELAY_LOOPS=300`、E1 `delay()` volatile 实现、`GXHT40_MEASURE_WAIT_MS=20`、双字 CRC/有效域/有界重试/失败语义、S/G/P 行字段集合与顺序、433/BLE 布局。

## 3. 对 F-3 / F-2 的决策（本轮不改）

- **F-3 `delay_ms` 约 1.626× 名义值**：所有调用点（tMEAS 20 ms、tPU 2 ms、光敏稳定 100 ms、读重试 1 ms、诊断 1–2 ms）都是**下限等待**，更长方向安全，且已由两次位级采集证实（32.5 ms / 32.5 ms）。重新标定会再引入一个时序变量、可能影响刚稳定的 D2/E1 组合；保留现标定并在接口/配置注释中明确“下限等待”语义。若产品后续要求名义毫秒精度，再单独标定并实板复核。
- **F-2 首间隔 148 s**：EV-027 由窗口边界重估稳态区间 127.25–184.90 s，148 s 落在区间内，不构成异常；`k` 增量恒 3。当前未将“首间隔精确 180 s”列为要求，不改 RTC 配置。

## 4. 构建与自检

| 项 | 结果 |
|---|---|
| `mdk_build {"action":"rebuild"}` | **0 Error / 1 Warning**（既有 `main.c(254)`）；`Code=25964 RO=656 RW=116 ZI=1676`（较 rev 6.3 +24 B） |
| 产物 | axf md5 **`a3b08774e764f174eac318f6771d0c42`**；hex md5 `546ce20a718fa69b40177cce0145f4b5` |
| `test/build_test.sh` | 46+**61**+54+28+29 = **218 项 0 失败**（新增 F-1 断言） |
| `host_gxht40_check` / `host_sf_i2c_bus_check` / `host_rf_frame_check` | **60 / 42 / 22 项 0 失败** |
| 合计 | **342 项 0 失败** |

> 宿主只证明快照语义与流程；不证明实板首行 `H=0`（需下载后从 `k=0` 窗口捕获）。

## 5. 交验证方的复验方法（F-1 后）

1. 下载 axf `a3b08774…`，记录 `Verify OK`/md5/commit（若仍无法并发抓上电段，至少从复位后首个 `S` 开始）。
2. **首样本语义**：复位后首个 `S k=0` 应为 `H=0 p=0`（判定无前值）；后续周期 `p` 为**上一周期**的 `t`，不再等于本周期 `t`。
3. **回归**：跨 ≥10 周期 `q=1`、无 `G`；SCL 仍约 614 µs、STOP→读 START 仍约 32.5 ms（D2/E1 未变）。
4. **T5**：在本修复镜像上重新做完全遮光（真实 100 ms 稳定等待），取 `o/n/x/a`、UID/VDD/遮光方式/时间；交回实现回填 `C_dark`，不得复用 EV-024/EV-026 的光照值。
5. T4-02 上电五行/横幅仍属采集编排缺口，不是固件改动；不得用后续行替代。

## 6. 未完成项

- 实板首行 `H=0`、`p` 语义与 `q=1` 回归：**待新镜像实测**；不声明通过。
- T5 全暗标定、INT 上报/跨链路、T4-02 上电五行：仍未完成（条件/编排缺口）。
- F-3 名义毫秒标定：未重标定（保留下限等待，理由见 §3）。

---

# 修复轮 run13：恢复真实 `delay_ms()`（E1）

## 1. 本轮输入与结论

| 证据 | 结果 |
|---|---|
| EV-024（rev 6.2，axf `e56d8d4b…`） | **27/27 周期 `q=1`，0 个 `G`**；SCL≈1.63 kHz（半位延时 300）；T4-05 实板成功判据满足；整体仍 `TEST_FAIL` 因 T5/INT 未执行与上电行采集缺口 |
| EV-025（同镜像，位级+串口） | 新增 4 周期仍 `q=1`；位级 1 个事务 6 字节双 CRC 均匹配；**确认 `delay()` 在 axf 中为 `bx lr`**，命令 STOP→读 START 仅 0.622 ms（配置 20 ms）⇒ 所有 `delay_ms()` 等待为 0；rev 6.1 的 10/20 ms A/B 未实际施加 |
| 归因建议 | E1（本项）：使等待不可被优化消除，重建后确认 `delay` 不再是 `bx lr`；在此之前不宜做 T5 全暗标定 |

## 2. 实际改动（仅 `USER/src/main.c`）

```c
void delay(uint32_t ms)
{
    volatile uint32_t i;              /* 关键: 防止 -O2 删除整个循环 */
    for (; ms > 0u; ms--) {
        i = 796u * 2u;               /* 原标定值保持不变 */
        while (i-- > 0u) { ; }
    }
}
```

- 计数变量改为 `volatile`，并在内层循环每次真实读/写；保留原 `796*2` 标定，不改变延时系数。
- 参数类型由 `uint16_t` 改为 `uint32_t`，与 `COMMON/delay.h` 的 `void delay(uint32_t count)` 原型一致（原定义未包含该头，存在跨单元原型不一致）。
- 未改 `delay_ms()`/`delay_us()` 调用点、未改 `GXHT40_MEASURE_WAIT_MS=20`、未改 D2 的 `SENSOR_I2C_DELAY_LOOPS=300`、未改命令/CRC/量程/帧格式。

## 3. 构建与反汇编验证（本轮实际执行）

| 项 | 结果 |
|---|---|
| `mdk_build {"action":"rebuild"}` | **0 Error / 1 Warning**（既有 `main.c(254)` 空循环体）；`Code=25940 RO=656 RW=116 ZI=1676`（相对 rev 6.2 **+28 B**，与恢复延时循环一致） |
| 产物 | axf md5 **`917bffe63ba0a4945a20a9a51f4a60df`**；hex md5 `5b017c84d693ce17318fca633b27ad07` |
| `fromelf --text -c` 反汇编 | `delay` = `0x4788`，函数体为 `SUB sp / CMP r0,#0 / MOVS r1,#0xC7 / LSLS #3 / STR [sp] / LDR [sp] / SUBS / STR / CMP / BNE / SUBS r0 / BNE`，**不再是一条 `bx lr`**；内层计数值 0xC7<<3 = 1592，外层按 ms 递减 |
| 宿主机自检 | `build_test.sh` 46+57+54+28+29=**214**；GXHT40 60；软 I²C 42；空口帧 22；合计 **338 项 0 失败** |

> 反汇编只证明等待循环被保留；实际毫秒长度（时钟/标定误差）需在实板用位级 START→STOP 间隔复核，宿主不能替代。

## 4. 交验证方的复验方法（E1 后）

1. 下载 axf `917bffe6…`，记录 `Verify OK`/md5/commit。
2. **先做位级时序复核**（逻辑分析仪现已在 `2.27` 可用）：抓一次完整事务，测量命令 STOP→读 START（预期约 20 ms 量级，允许代码/调度开销，不再为 0.6 ms）；记录 SCL 半位周期仍约 614 µs（D2 保留）。
3. **再复验 CRC**：COM43 跨 ≥10 个采样周期，统计 `q=0`/`G`；预期维持 0 失败（D2 的慢时钟仍保留），若回归则记录原始帧。
4. **然后做 T5**（必须在本修复之后）：完全遮光 ≥2 个采样周期，记录真实 100 ms 稳定等待下的 `o/n/x/a`、UID/VDD/遮光方式/时间；交回实现回填 `C_dark` 并重建。旧标定（无）不适用，不得沿用。
5. 注意：真实等待恢复后，光敏稳定时间从“0”变为约 100 ms 标定量级，`o/n/x/a` 可能与 EV-024 不同；这是预期行为，需重新记录，不得用 EV-024 的光照数值当标定。

## 5. 未完成项

- 实板等待时长、`q=1` 稳定率与 CRC 回归：**待新镜像实测**；本文件不声明通过。
- T5 全暗标定、INT 上报/跨链路：仍未执行（缺遮光/加温/网关/手机同窗条件）。
- T4-02 上电五行/UID：采集编排缺口，非固件改动；本轮未改上电顺序。
- D3（三次 attempt 逐帧打印）：未实现，仍受冻结行格式约束；当前 CRC 已 0 失败，优先级降低。

---

# 修复轮 run12：读时钟速率单变量 A/B（D2）

## 1. 本轮输入与上轮结论（保留失败历史）

| 证据 | 结果 |
|---|---|
| EV-022（rev 6.1，axf `08a4c266…`） | 19 周期 **13 次 `q=0`（68.4%）**；13/13 `G s=4`；温度 CRC 错、湿度 CRC 0/13 错；签名保持（11/13 `raw[2]==raw[0]`，2/13 MSB±1）；k=36→60 连续 9 周期失败 |
| EV-023（同一镜像，未重刷） | 6 周期 3 失败（50%），签名 3/3；修复镜像合计 **25 周期 16 失败（64%）**，与旧镜像 7/18（38.9%）无显著差异（p=0.1301） |
| 归因结论 | H2“采样点过早”**作为充分原因排除**；H1“等待不足”子假设**削弱**；H1′（器件第 3 字节输出内容）、H2′（主机读时钟速率）、H5（器件型号/装配）为剩余最小可区分集合；D2 要求做**读时钟速率**单变量 A/B |

本轮不重复已证明无效的“采样相位/转换等待”，只改读时钟速率。

## 2. 实际改动（仅 2 个文件）

| 文件 | 改动 | 说明 |
|---|---|---|
| `USER/inc/sensor_config.h` | 新增 `SENSOR_I2C_DELAY_LOOPS 300u`（原硬编码 100） | 软 I2C 半位延时 ×3；单一配置点 |
| `USER/src/measure.c` | `i2c0_dev.speed`、`i2c0_swap_dev.speed` 与 `i2c0_halfbit_delay()` 改用该常量 | 采样相位保持 rev 6.1 的“SCL 高电平末段”；仅改时钟速率 |

保持不变：`0xFD` 命令、10→20 ms 等待（rev 6.1 的无效 A/B 保留，不引入新变量）、双字 CRC-8、整数换算/有效域、有界重读/重测、失败语义、433/BLE 布局、S/G/P 行格式。

未实现 D3（同周期 3 次 attempt 逐帧打印）：它会新增调试行/改变 G 行字段，与 TD-002 冻结行格式及验证解析器冲突；需测试设计同步后才能做。本轮优先执行归因排序第一的 D2，不静默改格式。

## 3. 构建与自检（本轮实际执行）

| 项 | 结果 |
|---|---|
| `mdk_build {"action":"rebuild"}` | **0 Error / 1 Warning**（既有 `main.c(245)`）；`Code=25912 RO=656 RW=116 ZI=1676`（常量改动不增尺寸） |
| 产物 | axf md5 **`e56d8d4b48c14ff29ae376781bbdc915`**；hex md5 `d872d703e0ed8ec95bc3dee88f074b33` |
| `test/build_test.sh` | 46+57+54+28+29 = **214 项 0 失败** |
| `host_gxht40_check` | **60 项 0 失败** |
| `host_sf_i2c_bus_check` | **42 项 0 失败** |
| `host_rf_frame_check` | **22 项 0 失败** |
| 合计 | **338 项 0 失败** |

> 宿主 mock 不模拟真实半位时长；测试通过只证明位序列/协议/格式未被破坏，不证明实板失败率变化。

## 4. 交验证方的 A/B 判读

1. 下载 axf `e56d8d4b…`（记录 `Verify OK`、md5、commit），COM43/9600 8N1。
2. 跨 **≥10 个采样周期**采集原始 `.cap`；统计 `q=0` 率，对每个 `G raw` 独立复算并记录 `raw[2]−raw[0]`。
3. 对照基线：rev 6.1 修复镜像 25 周期 16 失败（64%）；旧镜像 18 周期 7 失败（38.9%）。
4. 判读：
   - `q=0` 率显著下降 / 签名消失 ⇒ 支持 H2′（主机读时钟兼容性）；仍需更多周期确认。
   - 失败率与签名完全不变（仍约 50–65%、`raw[2]` 与温度 MSB 同源）⇒ H2′ 削弱，H1′/H5 增强；下一步需要 D3（3 次 attempt 帧对照，需格式/测试设计同步）或 D4（器件型号/焊点/换件）。
   - 出现新失败形态 ⇒ 回传原始帧，不套用旧结论。

## 5. 未完成项

- 实板失败率/签名是否改变：**待本轮镜像实测**；本文件不声明修复。
- D3 诊断增强：未实现，原因是会改变冻结行格式；需测试设计允许新增行后再做。
- T5 全暗标定、>35℃/无光上报、跨链路：仍未执行。
- 上电五行的证据缺口属采集工具串行限制；固件未改上电顺序。

---

# 修复轮 run11：温度 CRC 间歇错误的有界时序 A/B（H1/H2）

## 1. 失败输入与归因结论（不重写原失败）

- EV-018：当前构建 15 个周期中 5 次 `q=0`，全部 `G s=4`（温度 CRC 错）；湿度字与 CRC 始终正确；6/6 失败帧 `raw[2]==raw[0]`。
- EV-021：合并 18 个周期、8 个失败帧，**8/8** 呈现同一签名；地址 `a44=1`、`rd=0`，失败发生在 CRC 校验层；独立穷举否定“另一种 CRC 约定/切片/字节序”；旧镜像同样签名，排除 T1–T3 引入。
- 保留竞争假设：H1 器件输出/CRC 加载竞争（优先）；H2 主机读时序兼容性（未排除）；H3 总线电气（低）；H4 MCU/编译器（低）；H5 器件型号/重焊状态（未决）。
- 本轮只针对 H2 与 H1 各做**一个有界、可回退的时序变量**，不改变判定与校验；不宣称根因已确定。

## 2. 实际改动（仅 2 个文件）

| 文件 | 改动 | 目的/依据 |
|---|---|---|
| `USER/src/sf_i2c.c` | `i2c_read_byte()`：把 SDA 采样点从“SCL 上升后立即采样”移到 **SCL 高电平保持段末尾**（保持原有半位延时，半位总长不变）。 | I²C 主机应在 SCL 高电平有效窗口内采样，而不是紧贴上升沿；针对 H2。影响所有读事务（GXHT40/诊断），写路径与 ACK/NACK 不变。 |
| `USER/inc/sensor_config.h` | `GXHT40_MEASURE_WAIT_MS`：10 ms → **20 ms**（仍远大于手册 tMEAS.H 最大 8.3 ms）。 | 针对 H1（器件在输出/CRC 加载完成前被读的竞争）；只增加等待，不改变 0xFD 命令、重测次数、CRC、量程、帧格式。 |

**明确未做**：
1. 未实现“CRC 失败后不再发 0xFD、直接重读同一帧”——`gxht40.pdf` §7.1 明确“内部温湿度转换数据被读取之后就会被清除，直到下一次测量数据更新”，同帧重读不符合器件语义，不能作为修复或诊断依据；EV-021 D2① 按此修正。
2. 未放宽/跳过 CRC-8、有效域、有界重读/重测；未使用 `0x94` 或加热命令；未改空口/BLE/S 行/G 行/P 行格式。
3. 未改 `gxht40.c` 的失败语义：失败仍不上报、不推进前值、不构造 0 值、不冻结快照，下一周期重探两地址并做有界恢复。

## 3. 构建与自检（本轮实际执行）

| 项 | 结果 |
|---|---|
| `mdk_build {"action":"rebuild"}` | **0 Error / 1 Warning**（既有 `main.c(245)` 空循环体）；`Code=25912 RO=656 RW=116 ZI=1676`（与上轮相同，时序常量/采样点改动不改变代码尺寸） |
| 产物 | axf md5 **`08a4c266af6a0f058821ec809d38f78d`**；hex md5 `8fcfa1da9b7b204d83d36faf97b267e4` |
| `test/build_test.sh` | 46+57+54+28+29 = **214 项 0 失败** |
| `host_gxht40_check`（真实驱动 + 位级 mock 总线） | **60 项 0 失败**（含 0xFD/CRC/重试/重探/总线恢复/时序桩） |
| `host_sf_i2c_bus_check` | **42 项 0 失败**（含读字节序列/ACK/NACK/总线恢复） |
| `host_rf_frame_check` | **22 项 0 失败**（布局/字节序/CRC 未变） |
| 合计 | **338 项 0 失败** |

> 宿主机 mock 在 SCL 上升沿更新 SDA，采样点后移不改变其结果；因此上述通过只证明**未破坏既有原语/协议/格式**，不证明实板 CRC 失败率下降。

## 4. 交独立验证的 A/B 方法与判读（不预设结论）

1. **部署**：对当前工作树 `mdk_build` + `mdk_flash {}`；记录 `Verify OK` 与 axf md5 `08a4c266…`（必须与本文件一致）。
2. **采集**：COM43/9600 8N1，跨 **≥10 个采样周期**（建议 ≥30 分钟，覆盖 EV-018 的 33% 失败率量级），保存原始 `.cap`。
3. **统计**：逐周期解析 `S` 的 `q`；失败周期核对 `G raw`，独立复算温度/湿度 CRC，记录 `raw[2]` 是否仍等于 `raw[0]`、`a44/a45/rd/at`。
4. **判读**：
   - 若 `q=0` 率显著下降（尤其 0/≥10 且签名消失）⇒ 支持 H2/等待余量有贡献；需继续累积样本，不能以少量周期宣告最终修复。
   - 若仍保持 `G s=4`、`raw[2]==raw[0]`（约 30% 或 8/8 签名）⇒ 加固 H1/H5（器件侧输出或型号/焊接）；下一步按 EV-021 D3 做器件核对/换件对照，固件侧不再改 CRC 或伪造数据。
   - 若出现新的失败形态（湿度 CRC、读 NACK、地址丢失）⇒ 记录原始帧并交回，不按旧签名套用结论。
5. **禁止**：不得用宿主 mock、旧捕获或“已改时序”推断实板通过；不得把 `q=1` 的少量周期当作长期稳定。

## 5. 未完成项

- T4 的实板 `q=1` 稳定率与 CRC 失败率：**待本轮新镜像实测**；本文件不声明修复。
- T5 全暗标定、>35℃/无光升温上报、跨链路：仍未执行（无遮光/加温/手机观测条件）。
- 上电段 `BOOT/VDD/IOTEST/IOSIG` 的证据缺口属采集工具串行限制，固件未改动上电顺序；需由验证方在能并发复位/采集的会话中重取。

---

# 本轮 run10：T1–T3 实现与自检（T4/T5 待实板）

## 1. 实际改动（逐项对应任务验收）

| 文件 | 改动 | 对应验收 |
|---|---|---|
| `USER/src/fw_core.c` | `sensor_decide_report` 改为：`cur>350` → true；否则要求 `have_prev && dark` 且 `int32(cur)-int32(prev) > 9`。删除 `cur==350` 的整体排除与降温方向；差值 int32。 | T1 全部向量（+0.8/+0.9/+1.0、降温、35.0℃ 边界、无前值、int16 极值） |
| `USER/inc/sensor_config.h` | `SENSOR_REPORT_DROP_X10` → `SENSOR_REPORT_RISE_X10`（=9）；注释同步为升温语义；新增调试发送等待上限 `SENSOR_DEBUG_TX_GUARD=200000`。 | T1 单一配置点；T3 上限单一来源 |
| `USER/src/measure.c` | 新增冻结快照 `s_report_temp_x10`/`s_report_hum_x10`：仅当 `rep!=0 && report_req==0` 时写入；`send_data_to_gateway()` 组帧只读快照；新增本周期发送尝试标志与 P 行取数（`p_rq/p_temp/p_hum`）。失败路径仍不推进前值、不构造 0 值、不新建/覆盖快照。 | T2 冻结/不覆盖/仅成功清除/用尽放弃 |
| `USER/inc/debug_trace.h` | `debug_trace_sample_t` 增加 `send_attempted`、`p_rq`、`p_temp_x10`、`p_hum_x10`（只读观测）。 | T2 P 行 |
| `USER/src/debug_trace.c` | 新增 `P rq=/s=/y=/t=/h=` 行（`report!=0` 或 `send_attempted!=0` 时紧跟 S/G 打印，≤96 B）；`trace_putc` 的 TXE 等待与 `flush_close` 的 TXE/TXBUSY 等待改为有界（`trace_wait_flag` + 超时抑制标志），超时仍完成 UART1 关闭，不改变业务状态。既有 BOOT/VDD/IOTEST/IOSIG/BUS/S/G 字节格式未改。 | T3 正常字节不变/卡死有界/业务隔离 |
| `test/host_sensor_core_test.c`、`test/host_fw_core_pure_check.c` | 判定向量改为升温/严格边界，新增 35.0℃ 升温、降温、int16 极值。 | T1 实现侧自检 |
| `test/host_measure_flow_check.c` | 流程序列改为升温；新增“待上报期间新采样不覆盖冻结样本”“重试三次帧入参均为同一快照”“放弃后重新冻结”“P 行取数”断言；mock 编码器记录逐次 t/h。 | T2 实现侧自检 |
| `test/host_debug_trace_check.c` | 更新 S 行最坏预算（含 P 行分段检查）；新增 P 行格式/条件测试；新增 TXE 永不置位与 TXBUSY 永不清零的 mock 卡死测试（有限返回、完成关闭、保护 GPIOA、恢复后可再打印）。 | T3 实现侧自检 |

未改动：`gxht40.c`、`sf_i2c.c`、`light.c`、`encrytogate.c`、`main.c`、UM2005C/COMMON/厂商库、网关、Android；空口/BLE 布局与采样节拍不变。

## 2. 构建与自检结果（本轮实际执行）

| 项 | 结果 |
|---|---|
| Keil `mdk_build {"action":"rebuild"}` | **0 Error / 1 Warning**（既有 `main.c(245) while(k--);` 空循环体）；`Code=25912 RO-data=656 RW-data=116 ZI-data=1676`；axf md5 `c7ed03b796f2ae438b325c7b9306d836`，hex md5 `79090d64d8e5487ee896983401f828ec` |
| `test/build_test.sh` | **214 项 0 失败**（fw_core 46、measure flow 57、UART trace 54、light 未标定 28、light 已标定 29） |
| `host_gxht40_check`（真实驱动 + mock 总线） | **60 项 0 失败**（驱动未改动，回归） |
| `host_sf_i2c_bus_check` | **42 项 0 失败**（软 I²C 原语/总线恢复回归） |
| `host_rf_frame_check`（真实传感器编码 + 真实网关解码/组块） | **22 项 0 失败**（10 B 布局/字节序/CRC 未变） |
| 合计 | **338 项 0 失败** |

关键行为断言（原始输出见 `evidence/driver_test.md`）：
- T1：30.0→30.9（暗）不报、30.0→31.0（暗）报、30.0→31.0（明）不报、30.0→29.0 不报、34.0→35.0（暗）报、34.0→35.0（明）不报、无前值 35.0 不报、无前值 35.1 报；int16 极值不溢出。
- T2：触发后连续三次发送失败期间，`encode_frame10` 的 t/h 逐次等于冻结样本，即使中间插入了新采样（373/601）也不变；用尽后 `report_req=0` 且不出现假成功；放弃后的新触发重新冻结新样本；P 行 t/h 与组帧值一致。
- T3：TXE 永不置位时打印在有限循环后返回且不输出半行；TXBUSY 永不清零时关闭仍有界完成；两条超时路径都不复位 GPIOA/不关 GPIOA 时钟，并把 PA05/PA06 置输入；恢复正常后下一次打印重新初始化并输出完整行。

## 3. 未完成项与原因（不得视为通过）

| 项 | 状态 | 原因与本轮已做的准备 |
|---|---|---|
| T4 GXHT40 实板采集恢复 | **未完成** | 本调用授权工具仅 `mdk_build`，无 `mdk_flash`/串口采集；未下载、未取得当前构建的 `BUS/VDD/IOTEST/IOSIG/G/S` 一手数据。历史证据（EV-017：7/7 周期 `q=0`、`BUS ack=none`、器件对任何地址无 ACK、`VDD≈3289 mV`）只作背景；竞争假设（U9 焊点/器件、总线电气、软 I2C 速率/时序、供电瞬态）全部保留，未在无新证据时盲目改驱动/速率。既有失败语义（不上报/不推进前值/不构造 0 值/失败后重探+总线恢复）保持不变。 |
| T5 全暗基准标定 | **未完成** | 无完全遮光窗口与串口采集，未取得全暗 `o/n/x/a`，因此未回填 `LIGHT_DARK_REF_CODE`，`LIGHT_DARK_CALIBRATED` 仍为 0（未标定门禁有效：升温分支不触发）。未用亮态/满量程/估算值冒充。判据与标定机制已由宿主机未标定/已标定两种变体回归（28+29 项）。 |
| 实板功能通过 | **未验证** | 本轮只有编译与宿主机自检；编译/自检成功不等于实板 `q=1`、无光判据或跨链路通过。 |

## 4. 交独立验证的刺激与读回方法

1. **部署**：用工程保存的调试器/Flash 算法对当前工作树执行 `mdk_build` + `mdk_flash {}`；记录 axf/hex md5 与 `Verify OK`，确认板上横幅 `fw=FD-002r4` 与 `uid`。
2. **串口**：**COM43/9600 8N1**（不得用历史 COM42/COM26）。复位后先收 `BOOT/VDD/IOTEST/IOSIG/BUS`，再跨 ≥3 个 3 分钟采样机会收 `S`，失败周期应有紧随的 `G`；上报周期应有紧随的 `P`。
3. **T1 边界**：用真实温度刺激（明/暗两种）覆盖 30.0→30.9、30.0→31.0、34.0→35.0、35.1；对照 `S` 的 `r/s` 与手机读数。无完全遮光时 `V=0`，升温分支不应触发，超温分支仍应独立生效。
4. **T2 冻结**：在 `P` 行核对 `t/h` 与当次触发样本一致；若首次发送失败，`P` 显示 `rq=1/s=2` 仅表示本轮未结束（同唤醒后续重试不再打印），不得据此判最终失败；最终以 `report_req` 清除、后续周期行为与手机实收判断。
5. **T3**：正常路径核对既有各行字节未变；UART 卡死无法在实板主动制造时，以宿主机 mock 证据为准，实板只确认打印不再悬挂（观察复位/唤醒周期是否继续）。
6. **T5**：完全遮光窗口内取得 ≥1 笔有效 `o/n/x/a`，记录 UID/VDD/遮光方式/时间/样本分布；把原始字节交回实现回填 `C_dark` 并重建，再做有光/遮光/恢复三态复测。
7. **T4**：若 `q=0` 仍出现，回传原始 `BOOT/VDD/IOTEST/IOSIG/BUS/S/G` 字节与构建 md5；若出现 `ack=44/45` 但读失败/CRC/量程，按结果码分别定位。不得只写“仍失败”。

## 5. 版本与历史

本节替代 rev 5.8 的“无代码改动”状态；rev 5.8 及更早的实板分析、现场动作建议与 EV-012..017 记录继续保留在下方，仅作历史背景，不冒充本轮结果。

---

# 本轮 run8：ITEM-002 复验修复七（用既有证据收窄现场定位；无代码改动）

**本轮状态**：RESUME 检查后确认**无新上游证据、无现场动作记录**（最新验证仍为 EV-017：`VDD ok=1 mv=3289`、`gc=3`、`BUS ack=none`、7/7 周期 `q=0`），`git status` 干净。因此不重复上一轮结论，也不盲目改码；本轮唯一可新增的专业内容是：在**不新增任何测量**的前提下，用 EV-017 已取得的 `IOTEST`/`IOSIG`/`VDD` 读数，把“器件侧”的物理可能性进一步收窄，以减少现场返工次数。

## 1. 由既有读数可排除的装配缺陷（含“若成立应出现什么读数”）

| 假想装配缺陷 | 若成立本应出现的读数 | 实板实测（EV-017） | 结论 |
|---|---|---|---|
| SDA↔SCL 短接 | `IOSIG` 的 `sda` 会跟随 `scl`（不再等于 `0x30303`） | `sda=30303`、`scl=95555`（互不相同且 sda=数据位） | **排除** |
| SDA↔GND 或 SCL↔GND | `IOTEST idle` 对应位为 0 | `idle=3` | **排除** |
| SDA 或 SCL 短到 VDD 轨 | 对应 `sda_lo`/`scl_lo` 回读为 1（拉不低） | 均为 0 | **排除** |
| 器件 VDD↔VSS 硬短路 | 供电轨塌陷 | `VDD mv=3289`（MCU 正常运行） | **排除（显著短路）** |
| 装配横向错一位（pin1→pin2 焊盘 …，致器件 VDD 落到 GND 焊盘、器件 SDA 落到 SCL 网） | 器件 SDA 引脚的 ESD 二极体会把被上拉的 SCL 线钳到 ~0.6 V ⇒ `idle` bit0=0 | `idle=3` | **排除** |

⇒ 现场 **不需**重点排查“短路/连锡/错位”；应重点查**开路/虚焊/焊盘未润湿或抬起**。

## 2. 仍无法由固件区分、必须现场核对的项（不得写成已排除）

1. **四个焊点中的开路（含 `U9.3 VDD` 或 `U9.4 GND`）**：总线侧与供电网侧都无法区分“器件未得电”与“器件得电但不应答”。
2. **装配方位错（如 180° 旋转）**：该情形下器件 SDA/SCL 引脚分别接到板级 GND/BAT 网，两条总线仍由 MCU+上拉正常工作 ⇒ `idle=3`、`sda_lo=0/scl_lo=0`、`IOSIG=95555/30303` 全部“正常”，**不被现有读数排除** ⇒ 必须**目视核对器件方向/丝印标记**（仅测通断不够）。
3. **器件本体损坏 / 异常闩锁**：需 E-a（真断电）或 E-c（换件）区分。
4. 备注（推论，非测量结论）：若 **VDD 引脚开路而 SDA/SCL 已连接**，理论上过器件的 ESD 路径会经浮动 VDD 节点对总线产生一定钳位；本轮 `sda=30303`（未出现异常钳位）属**弱证据**不支持该情形，但依赖 ESD/阈值细节，**不作硬排除**。

## 3. 现场最小动作（相对上轮的范围修正）

**E-b（首选，需万用表；未提供不作为门槛）**：
1. **目视核对 U9 方位**（器件第 1 脚/丝印标记与焊盘一一对应；DFN4 不能只靠通断判；反向装配未必造成短路）。
2. 通断/阻值核对四焊点：`1=SDA↔PA04`、`2=SCL↔PA03`、`3=VDD↔BAT`、`4=VSS↔GND`；尤其看**虚焊/焊盘未润湿/抬起**。
然后 **E-a**（真断电 ≥5 s 再上电）→ **E-c**（换一只已知良好 GXHT40，含 `0x45` 变体）。
**E-d**：任一步后**不改任何代码**，重新上电 + COM42 连续采集 ≥ 10 分钟；恢复判据：`BUS ack` 含 `44`/`45`、或 `IOSIG sda=30300`（ACK 时隙被拉低）、或 `gc≠3`、或直接出现 `S … q=1`；同时读 `VDD` 行。

## 4. 本轮实际改动与交接

- **无产品代码改动**（`git diff --stat 8ae4a33 HEAD -- <FW>/USER` 为空）；交付件身份不变（上轮全量 rebuild 已复现 `5fc16235ccd043f4edb48b16ee44da0b`）。本轮未重新构建/未新增测试，故 build/driver_test 证据仅作版本注记。
- **设计/硬件能力（建议，属其职责）**：本项的物理限制有两处值得后续版本改进——① 传感器 VCC 与 MCU 共用 `BAT` 且无门控/测试点，**真断电复位难以施加**（本项 H3 因此无法由固件清除）；② DFN4 手工返修容易产生开路/方位错，且板上无第二个 I²C 器件可供“ACK 正对照”。建议后续版本加测试点/可断供电链路，并在板上预留一个已知良好的 I²C 器件或测试盘。
- **tester**：TD-002/FD-002 上电行集合与顺序（`BOOT→VDD→IOTEST→IOSIG→BUS`）与 `gc`/`VDD` 字段同步；`test/host_light_verify_ev.c` 历史遗留不可编译建议清理。
- **未满足项**：实板 `q=1` 仍未达成；器件对任何 I²C 地址（含 `0x00`）不应答且供电网电位正常 ⇒ 剩余路径为上述现场动作，**无需固件改动**。

---

# 历史：ITEM-002 复验修复五/六（供电轨 VDD 实测；固件侧手段穷尽性复核）

**本轮输入**：EV-017（当前交付件 `8ae4a33`，axf md5 `5fc16235…`）新增实测 **`VDD ok=1 code=1480 bgrmv=1189 mv=3289`**（供电轨 ≈3.289 V；本能力独立复算 `4095*1189/1480=3289` 相符），同时 7/7 周期 `q=0`、`BUS ack=none`、`gc=3`、`IOSIG scl=95555 sda=30303`、`IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none` 与前几轮**逐字节一致**。该能力据此**排除供电分支**并明确要求实现能力判断是否仍有未列出的、不违反冻结约束的驱动/总线层手段。

**本轮结论：固件侧可观测/可行动分支已穷尽；本轮无产品代码改动（无新可交付手段，不盲目改码）。**

## 1. 穷尽性复核（逐项给出“为何不能再做点什么”）

| # | 手段 | 现状 / 为何不能帮助 |
|---|---|---|
| 1 | 地址探测 | 112 个合法 7bit 地址 + general call `0x00` 全覆盖，全无 ACK；手册确认全系列仅 `0x44`/`0x45` ⇒ 无扩展面 |
| 2 | 位时序/速率 | 半位 ~50–100 us（≈8–10 kHz）远低于手册上限，手册无最低频率；`IOSIG` 已在引脚级证明 START/8 数据位/ACK 时隙形态正确（`95555/30303`）⇒ 再改只会降低裕量 |
| 3 | 等待与重试下限 | `tMEAS` 10 ms ≥ 8.3 ms、`tPU` 2 ms ≥ 1 ms；失败发生在**地址字节**（`rd=0`），任何等待都到不了读阶段 |
| 4 | 有界总线恢复 | 每次重探前已执行（≤9 个 SCL 脉冲 + STOP）；负对照证明“SDA 被持续拉低”必然报满表 ACK，而实板为 `none` ⇒ 非卡死 |
| 5 | general call 复位 | 已实现一次尝试；`gc=3` 证明 `0x00` 也不被 ACK ⇒ 复位命令物理上传不进器件 |
| 6 | 软复位 `0x94` / 加热命令 | 均需地址 ACK 才能送达；且周期路径禁用（冻结约束）⇒ 不可能成为修复 |
| 7 | 读/CRC/量程/重试策略 | 未进入（`rd=0`：连读地址都未被 ACK）⇒ 与该层无关，且不得放宽/绕过 |
| 8 | 引脚映射/方向/开漏/回读 | `IOTEST`（拉低/释放双向回读）与 `IOSIG`（事务内逐位）证明主机侧正确；网表 `PA04=SDA/PA03=SCL` 一致 |
| 9 | 供电轨 | 本轮实测 3.289 V，在器件域 1.6–5.5 V（典型 3.3 V）内；网表证明与 `U9.3` 同网、无开关/串联 ⇒ 排除电池与轨塌陷 |
| 10 | 总线内其它从机争用 | 网表：`N05845`/`N06024` 上只有 U9 + R1/R2 ⇒ 无争用源 |
| 11 | 推挽/反向/其它电气驱动变体 | a) 器件连地址都不应答，驱动方式改变无法使其应答；b) 推挽/反向违反 I²C，且器件真的 ACK 时存在**驱动冲撞损坏风险** ⇒ 明确不做 |
| 12 | 直流电气量判“器件未得电但 SDA/SCL 已连接” | 4.7 k 上拉使该情形与“器件未连接”的直流电平**同为高**（器件 ESD 钳位只会把浮动 VDD 节点充到 ≈VDD-0.6，线仍被拉高）；本项目无电流/AC 手段区分 ⇒ **不可由固件闭合**（与 EV-017 §6.4 一致） |
| 13 | 现场动作（E-b/E-a/E-c） | 属硬件/现场；固件不能替代，也不需要改代码 |

⇒ **不存在仍在实现能力边界内、且不违反冻结约束（不放宽/绕过 CRC、有效域与校验；周期路径不用 `0x94`/加热；失败周期不上报/不推进前值/不构造 0 值）的驱动或总线层手段。** 本判断已按该能力要求“如实登记并交由现场/用户执行 E-b（首选）→ E-a → E-c”。

## 2. 本轮实际改动与可复现证据

- **无产品代码改动**：`git diff --stat 8ae4a33 HEAD -- <FW>/USER` 为空；`USER/**` 与最后交付提交逐字节相同。
- **交付件身份可重建**：`mdk_build {"action":"rebuild"}` 全量重建 → `0 Error / 1 既有告警`（`main.c(245)`），`Code=20100 RO-data=644 RW-data=116 ZI-data=1676`，axf md5 **仍为 `5fc16235ccd043f4edb48b16ee44da0b`** ⇒ 与 tester 下载/受测的 axf 同一字节。
- **自检复跑**（本工作树）：`test/build_test.sh` 40+43+44+28+29、`host_gxht40_check` 60、`host_sf_i2c_bus_check` **42 合计 286 项 0 失败**；tester 自有 `host_diag_probe_verify_ev` **104/104**。

## 3. 未满足条件与下一步（如实登记）

- **未满足**：本项验收的核心可观察（真实目标每 3 分钟周期 `q=1`、`t/h` 有效域、连续多周期稳定）在当前板上**未达成**；固件**不能**使一个对任何 I²C 地址（含 `0x00`）都不应答的器件应答。这是本能力现阶段无法通过自身手段完成的部分。
- **下一步（现场/用户，最小必要动作，按优先序）**：
  1. **E-b（首选；需万用表，未提供不作为门槛）**：目视 + 通断/阻值核对 U9 四焊点（`1=SDA↔PA04`、`2=SCL↔PA03`、`3=VDD↔BAT`、`4=VSS↔GND`）——供电网电位已正常，故焊点连通性是剩余最可能的器件侧解释。
  2. **E-a**：真正断电 ≥ 5 s 后再上电（CR2032 焊在板上，需拆/抬电池或断开正极焊点）——清除可能的闩锁态。
  3. **E-c**：换一只已知良好的 GXHT40（含 `0x45` 变体）。
- **E-d（无需改代码）**：任一步后重新上电 + COM42 连续采集 ≥ 10 分钟（4 个 3 分钟周期）；恢复判据：`BUS ack` 含 `44`/`45`，或 `IOSIG sda=30300`（ACK 时隙被拉低），或 `gc≠3`，或直接出现 `S … q=1`；同时读 `VDD` 行确认供电仍正常。
- **本能力保留**：全部诊断行（`VDD`/`IOTEST`/`IOSIG`/`BUS`/`S`/`G`）与全部校验/重试/恢复逻辑；器件一旦恢复应答，**无需任何代码改动**即可产出 `q=1`。

## 4. 交接

- **tester**：TD-002/FD-002 文档同步（上电行集合与顺序 `BOOT→VDD→IOTEST→IOSIG→BUS`；`gc` 与 `VDD` 字段）；`test/host_light_verify_ev.c` 为早期遗留（引用的 `mockinc/` 已不在仓库、API 已随 `light_result_t` 变更）目前不可编译，建议清理或重写（属 tester 自有资产）。
- **设计能力**：把本项失败链与“固件侧可观测/可行动分支已穷尽”结论登记进 FD-002 的异常策略/未决项，避免后续轮次重复同一路径。

---

# 历史：ITEM-002 复验修复五（供电轨 VDD 实测）

**触发**：EV-016（当前交付件 rev 5.5）给出新的判别事实 **`gc=3`**：连 I²C general call 地址 `0x00` 也不应答，因此**未发出复位命令**（`wr_len=0`）；叠加 112 个合法 7bit 地址 `ack=none`，受测器件对**任何** I²C 地址都不响应。该能力用自有 harness（新增至 104 项，含 `gc=0/1/2/3` 四分支正/负对照）确认 `gc=3` 是器件静默的真实读数而非固件误报，并重申不代实现者判定不可修复。

**本轮选择的下一步（依据）**：主机侧可观测的分支已经穷尽（H4/H5/H12 已排除，general call/H12 分支已实测），而**唯一从未被测量的物理量是供电轨**（项目无万用表；MCU/BAT/传感器 VDD 同网）。该量在本项目内**可由固件第一手测得**：厂商提供了内部 BGR1.2V ADC 通道（`ADC_InputVref1P2`）+ 出厂修调值（`0x001007D2`，mV），示例公式 `VDD = 4.095 × BGR_mV / code`。GXHT40 要求 VDD ≥ 1.6 V，而 CR2032 衰老/负载压降可能使器件无法完成上电复位而闩锁——这恰好是 H3 的成因之一且**无需任何新仪器**即可判别。

## 1. 本轮实现

| # | 内容 | 位置 |
|---|---|---|
| 1 | 新增 `light_read_vdd_mv(&mv,&code,&bgrmv)`：配置 ADC 为内部 `ADC_InputVref1P2`，使能 BGR 并等 1 ms（厂商：启动≈30 us），有界 EOC 轮询（guard）读一次转换，`mv = 4095*bgrmv/code`；读完 **关 ADC 并清 BGREN**（低功耗）；超时/除零→返回 0 且三个输出均为 0（**不伪造读数**）；**必须在 `light_init()` 之前调用**（随后 light_init 会重配 AIN11 通道） | `USER/inc/light.h`、`USER/src/light.c`（ADC 拥有者） |
| 2 | 新增上电行 `VDD ok=<0\|1> code=<0..4095> bgrmv=<0..4095> mv=<0..6000>`（最坏 39 B ≤ 96 B）；同时上报 `code/bgrmv` 以便独立复核 `mv = 4095*bgrmv/code` | `USER/inc/debug_trace.h`、`USER/src/debug_trace.c` |
| 3 | 上电顺序变为 `BOOT` → **`VDD`** → `IOTEST` → `IOSIG` → `BUS`（VDD 测量无 I²C 事务，仍在所有 I²C 事务之前） | `USER/src/main.c` |

**未改动**：BUS/S/G/IOTEST/IOSIG 行字段与字面、采样/判定/上报/冻结、GXHT40 校验与重试、general call 复位尝试、433/BLE 布局；PB04/PB05 引脚归属仍在 light.c。

## 2. 判读表（交 tester；本能力不代替采集）

| `VDD` 实板读数 | 含义 | 后续 |
|---|---|---|
| `ok=1` 且 `mv` ≈ 2700–3100 | 新鲜 CR2032 范围，**供电正常** | 供电假设被排除 ⇒ 剩余解释在器件/接线侧（焊点开路/芯片损坏/闩锁），走现场 E-a（真正断电 ≥5 s）/E-b（通断）/E-c（换件） |
| `ok=1` 且 `mv` 明显偏低（≲ 1800） | 供电接近/低于 GXHT40 的 VDD 下限（1.6 V），器件可能无法完成上电复位 | **最小现场动作：换新 CR2032**（无仪器）后重上电复测 |
| `ok=0` | 测量无效（转换超时） | 不当作电压结论；记录后按上两项之一的实测重试 |

> 精度边界：依赖出厂修调值与 ADC，仅供“供电是否在可用量级”判断，**不作精密测量**；`ok=0` 时 `mv=0` 不代表 0 V。

## 3. 验证（本轮实际执行）

- **宿主自检合计 286 项 0 失败**：`test/build_test.sh` 40+43+**44**+**28**+**29**（light 两个变体各 +9：正常转换期望值 `mv=4095*1200/1632`、超时→`ok=0/mv=0` 不伪造、测后 `BGREN`/ADC 均关闭、**VDD 测量后 light_sample 仍 8/8 成功**；debug-trace 新增 [T13] VDD 行有效/无效逐字节与预算）；`host_gxht40_check` 60/60；`host_sf_i2c_bus_check` 42/42。
- **tester 自有 harness 只读复跑**（未修改其文件）：`host_diag_probe_verify_ev` **104/104**、`host_gxht40_verify_ev` **93/93**。
- **构建**：GNU 0 错误，FLASH **34,572 B** / RAM **1,808 B**（较上轮 34,120/1,808 为 +452/0 B）；Keil `mdk_build rebuild` **0 Error / 1 既有告警**（`main.c(245) while loop has empty body`），`Code=20100 RO-data=644 RW-data=116 ZI-data=1676`，axf md5 **`5fc16235ccd043f4edb48b16ee44da0b`**；镜像内含 `VDD ok=`/`bgrmv=` 字符串。
- **`SENSOR_DEBUG_UART=0` 复核**：`light.c`/`debug_trace.c` 0 告警 0 错误。
- 原始命令与 stdout：`evidence/build.md`、`evidence/driver_test.md`（本轮节）。

## 4. 未取得与交接

- **实板 `VDD` 读数与 `q=1` 均未取得**：本调用无 `mdk_flash`/串口。本轮交付的是**最后一个可由固件第一手观测、且尚未测量过的物理量**。
- **tester 需同步**：解析/断言接受新行 `VDD`（上电顺序 `BOOT→VDD→IOTEST→IOSIG→BUS`），可用 `mv == 4095*bgrmv/code` 独立复核 `ok=1` 的值；复判时把 `VDD mv` 与 `BUS ack`/`gc`/`q=` 关联记录。
- **设计能力需同步**：FD-002 §5.2/§6.7/§10 收录 `VDD` 行、`IOTEST`/`IOSIG` 行与真实上电顺序；若采纳，`low_power` 章节可记一笔「上电一次 BGR 测量，测后关 ADC/BGR」。
- **现场/用户（按 §2 分支）**：`mv` 正常 ⇒ E-a/E-b/E-c；`mv` 偏低 ⇒ 先换 CR2032。任一步后**无需再改代码**，重新上电即可自动重观测（新 `VDD` 行 + `gc` + `BUS` + `S q=`）。

---

# 历史：ITEM-002 复验修复四（general call 复位尝试 + gc）

**触发**：operator_update（2026-10-09T01:13Z）告知用户**已重新焊接 U9**并授权重做，且旧无 ACK/`q=0` 记录属焊接前条件；随后 EV-015 以当前交付件重测：重焊后失败形态**逐字节相同**（5/5 周期 `q=0`、`BUS ack=none`），但主机侧已被实板证据证明健康（`IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none`、`IOSIG scl=95555 sda=30303` ⇒ H4/H5/H12 均排除），剩余假设为 H1（贴装/焊接）、H2（损坏）、**H3（异常闩锁，需真正断电；权重最高）**，且该能力**明确登记 general call 地址 `0x00` 从未被探测**，并把“是否/如何改动”交回实现。

## 1. 手册依据（第一手，并如实标注提取限制）

- `gxht40.pdf`（GXHT4x Family Datasheet V2.6, Aug-2026）**§7.7（复位）** 列出多种复位方式，其中第 2 项可提取到拉丁字符：`I²C general call … I²C … 0x00 … 0x06`，即 **I²C general call 复位 = 地址 `0x00` + 命令 `0x06`**（I²C 规范中用于复位处于未定义/异常状态的从机的标准机制）。
- **提取限制（如实记录）**：该 PDF 的中文正文使用无 ToUnicode 映射的嵌入字体，`pdftotext` 只能取到拉丁字符/数字/公式；§7.7 的完整句子**未能被本能力读全**。因此本条依据为**手册 §7.7 “复位”小节的 token 级证据**（`general call`/`0x00`/`0x06` 同时出现于同一小节），不是整句翻译。本节实证价值在于：**该命令在此之前从未在本项目上尝试过**，且其效果可由现有 `BUS` 行直接观察。
- 与冻结约束的关系：任务禁止的是**周期路径**使用软复位 `0x94`/加热命令；本轮**未使用 `0x94`**，新增的是**上电一次性、仅当器件不应答时**才尝试的 general call 复位（不在采样周期内、不重试、不反复复位）。

## 2. 本轮实现

| # | 内容 | 位置 |
|---|---|---|
| 1 | `sensor_io_diag_scan()` 末尾新增**上电一次性器件恢复尝试**：先探 `0x44`/`0x45`；**任一应答则 `gc=0` 且不任何复位**（正常器件路径不变）；否则探 general call `0x00`：不应答 ⇒ `gc=3`（有界，不再重试）；应答但 `0x06` 未被 ACK ⇒ `gc=1`；`0x06` 被 ACK ⇒ `gc=2` 并等 `GXHT40_GCALL_RESET_WAIT_MS`（2 ms）后返回，随后的 `BUS` 扫描即反映复位后是否恢复应答 | `USER/src/measure.c` |
| 2 | 单一配置点新增：`GXHT40_ADDR_GENERAL_CALL=0x00`、`GXHT40_CMD_GENERAL_CALL_RESET=0x06`、`GXHT40_GCALL_RESET_WAIT_MS=2` | `USER/inc/sensor_config.h` |
| 3 | `IOTEST` 行**末尾追加** `gc=<0..3>`（不改既有字段/顺序/字面）；最坏行长 49 B ≤ 96 B | `USER/inc/debug_trace.h`、`USER/src/debug_trace.c` |

**未改动**：`BUS`/`S`/`G`/`IOSIG` 行字段、采样/判定/上报/冻结、GXHT40 校验与重试、地址缓存与总线恢复策略、433/BLE 布局。

## 3. 判读表（交 tester；本能力不代替采集）

| 新 `IOTEST gc=` | 含义 | 后续 |
|---|---|---|
| `gc=0` | 器件已应答 `0x44`/`0x45`，未做复位（正常路径） | 看 `BUS ack` 与 `S q=` |
| `gc=2` | `0x00` 与复位命令 `0x06` 均被 ACK ⇒ **复位命令已被器件接受** | **关键看紧随的 `BUS` 行**：`ack` 含 `44`/`45` 且后续 `S` 行 `q=1` ⇒ 固件侧恢复成功（无需再改代码）；仍 `ack=none` ⇒ 复位未奏效，转现场 |
| `gc=1` | `0x00` 被 ACK 但 `0x06` 未被 ACK（器件部分响应） | 记录；若重复出现，属器件异常行为，转现场 |
| `gc=3` | **连 general call 地址 `0x00` 也不应答** ⇒ 器件对**任何** I²C 地址均无响应 | 固件侧已无可为：唯一前行路径为现场 E-a（**真正断电 ≥ 5 s**）、E-b（四处焊点通断/阻值）、E-c（换件） |

> 按 EV-015 实测（`BUS ack=none`、`IOSIG` ACK 时隙 `11`）预测本轮实板最可能得 `gc=3`；若如此，则该项的剩余解释**全部**在器件/接线侧，且已有一手证据支撑——但这是**由 tester 采集判定**，本能力不预先宣告。

## 4. 验证（本轮实际执行）

- **宿主自检合计 265 项 0 失败**：`test/build_test.sh` 40+**43**+41+19+20；`host_gxht40_check` 60/60；`host_sf_i2c_bus_check` **42/42**（新增 general call 线序用例：`0x00`+`0x06` 逐字节、1 START/1 STOP、ACK 与不应答两条分支）。measure-flow 新增断言：无器件时 `gc=3`（不发出复位命令、不反复复位）。
- **tester 自有 harness 只读复跑**（未修改其文件）：`host_diag_probe_verify_ev` **85/85**、`host_gxht40_verify_ev` **93/93**。
- **构建**：GNU 0 错误，FLASH **34,120 B** / RAM **1,808 B**（较上轮 34,004/1,808 为 +116/0 B）；Keil `mdk_build rebuild` **0 Error / 1 既有告警**（`main.c(238) while loop has empty body`），`Code=19144 RO-data=644 RW-data=116 ZI-data=1676`，axf md5 **`ff49029608f3315ee76964c131d06821`**；镜像内含 ` gc=` 字符串。
- **`SENSOR_DEBUG_UART=0` 复核**：`measure.c`/`debug_trace.c` 0 告警 0 错误。
- 原始命令与 stdout：`evidence/build.md`、`evidence/driver_test.md`（本轮节）。

## 5. 未取得与交接

- **实板 `gc` 读数与 `q=1` 均未取得**：本调用无 `mdk_flash`/串口。本轮交付的是**最后一个固件侧可行动分支**的尝试与可观测字段。
- **tester 需同步**：判定脚本/用例接受 `IOTEST` 新尾部字段 `gc=<0..3>`（既有字段/顺序不变）；复判时把 `gc` 与紧随的 `BUS ack`、后续 `S q=` 关联；建议的恢复判据（EV-015 §7 已给出）：`BUS ack` 含 `44`/`45` 或 `IOSIG` ACK 时隙变为 `00`，或直接出现 `q=1`。
- **设计能力需同步**：FD-002 §5.2/§6.7 收录 `IOTEST`/`IOSIG`/`gc` 与 `BOOT→IOTEST→IOSIG→BUS` 顺序；§10 异常策略建议补一行「上电一次性 general call 复位尝试（仅器件不应答时，有界一次）」。
- **现场/用户（若 `gc=3`）**：E-a 真正断电 ≥ 5 s 后复上电；E-b 四处焊点通断/阻值；E-c 换一只已知良好的 GXHT40（含 `GXHT40-BD` 0x45）。任一步后**无需**再改代码，只需重新上电 + COM42 采集复判。

---

# 历史：ITEM-002 复验修复三（E1b IOSIG 事务内逐位回读签名）

**触发**：EV-012 判 TEST_FAIL，但取到实质进展：实板 `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none`（5/5 一致）**排除 H5（主机拉不低）与 H4（装配接反）**，并新登记 **H12**（静态驱动可回读，但**事务期间**的位/时钟形态是否真实送达未被覆盖）；该能力按规则交回实现，要求做 **E1b**。

## 1. 本轮实现（E1b）

| # | 内容 | 位置 |
|---|---|---|
| 1 | **新增上电 `IOSIG` 行**：`IOSIG scl=<5 位大写 hex> sda=<5 位大写 hex>`（各 20 bit）。用与软 I2C **相同的引脚原语与半位延时**（`speed=100` 等价）手工发出地址字节 `0x88` 的完整事务，在 **20 个半位采样点**回读 SCL/SDA 并 MSB 先入：S0 START 建立；S1 SCL 拉低；S2..S17 八位数据（每位 SCL 低/高各一点）；S18 ACK 时隙 SCL 低（主机释放 SDA）；S19 ACK 时隙 SCL 高 | `USER/src/measure.c` (`sensor_io_sig_scan`)、`USER/src/debug_trace.c` (`debug_trace_iosig`)、`USER/inc/{measure,debug_trace}.h` |
| 2 | 只发地址字节（**不写 0xFD、不发第二个字节**），结束补一个合法 STOP 并释放两线；不改变采样/判定/上报/冻结 | 同上 |
| 3 | 上电顺序：`BOOT` → `IOTEST` → `IOSIG` → `BUS`；仅 `SENSOR_DEBUG_UART=1`，`=0` 时两个自检均为编译期空实现 | `USER/src/main.c` |

**未改动**：`BUS`/`S`/`G`/`IOTEST` 行字段与字面、采样/判定/上报/冻结、GXHT40 校验与重试、总线恢复、433/BLE 布局。

## 2. 判读表（交 tester；本能力不代替采集）

无器件应答时，**正确主机**应得 `scl=95555 sda=30303`（数据位回读等于被驱动位、每个数据位 SCL 低/高各一次、ACK 时隙释放后为高）。

| IOSIG 实板读数 | 结论 |
|---|---|
| `scl=95555 sda=30303` | 位序列与时钟在引脚上**真实成立** ⇒ **H12 排除** ⇒ 剩余解释全部落在**器件/接线侧**（H1 贴装/焊接、H2 损坏、H3 异常闩锁） |
| `scl≠95555` | SCL 未按时序翻转/未被真实送达 ⇒ **H12**（主机侧） |
| `sda` 数据位部分不符 | SDA 位未被真实送达/回读不符 ⇒ **H12**（主机侧） |
| `sda` 末两位（ACK 时隙）为 `0`（即 `sda=30300`） | 有器件在该地址上 **ACK**——若与 `BUS ack=none`/`G a44=0` 同轮不符，是需单独追查的异常 |

> 边界：IOSIG 用与生产路径相同的引脚原语与半位延时构造序列，验证的是**引脚级序列与回读**；`sf_i2c` 源级位时序已由 tester 自有 harness 逐位验证，本轮不改 `sf_i2c`（保持其机器码不变验证）。

## 3. 验证（本轮实际执行）

- **宿主自检合计 162 项 0 失败**：`test/build_test.sh` 40+**42**+**41**+19+20（measure-flow 新增 [13]：签名扫描完成 + **无器件时 scl=95555/sda=30303** + **H12 负对照**（SDA 回读恒高时 `sda=FFFFF≠30303`）；debug-trace 新增 [T12]：IO SIG 期望签名逐字节匹配 / 全 1 最坏 27 B / 仅整数与大写 hex）；`host_gxht40_check` 60/60；`host_sf_i2c_bus_check` 35/35。
- **tester 自有 harness 只读复跑**（未修改其文件）：`host_diag_probe_verify_ev` **85/85**、`host_gxht40_verify_ev` **93/93**。
- **构建**：GNU 0 错误，FLASH **34,004 B** / RAM **1,808 B**（较上轮 33,564/1,808 为 +440/0 B；27 条告警均为既有类别）；Keil `mdk_build rebuild` **0 Error / 1 既有告警**（`main.c(238) while loop has empty body`），`Code=18888 RO-data=644 RW-data=116 ZI-data=1676`，axf md5 **`5c2fdb58d0fc4ef7dac8a21e343082a3`**；镜像内含 `IOSIG scl=` 与 `IOTEST sda_lo=` 字符串。
- **`SENSOR_DEBUG_UART=0` 复核**：`measure.c`/`debug_trace.c` 0 告警 0 错误（两个自检均空实现）。
- **修正上一轮证据的行号笔误**：Tester 在 EV-012 §1 指出上轮 `evidence/build.md` 写 `main.c(224)` 而实际为 231；本轮如实报 `main.c(238)`（行号随新增代码位移）。
- 原始命令与 stdout：`evidence/build.md`、`evidence/driver_test.md`（本轮节）。

## 4. 未取得与交接

- **实板 `IOSIG` 读数与 `q=1` 均未取得**：本调用无 `mdk_flash`/串口。本轮交付的是 E1b 的**可复跑判别观测量**；读数出来后才能把 H12 与器件侧一刀切开。
- **tester 需同步**：TD-002 §4.4 与解析脚本接受新行 `IOSIG`（上电顺序 `BOOT→IOTEST→IOSIG→BUS`；`BUS`/`IOTEST` 字面不变）；并在复判中记录 IOSIG 与 `BUS`/`G` 的同轮自洽性。
- **设计能力需同步**（EV-012 §7.3 亦已登记该文档不同步）：FD-002 §5.2/§6.7 需收录 `IOTEST`、`IOSIG` 两行与真实上电顺序；FWR/TD 三份基线目前均未出现 `IOTEST`。
- **用户/现场**：若 IOSIG 为 `scl=95555 sda=30303`（H12 排除），则只剩 E2（真实断电复上电）与 E3（目视/通断/换件）；两项均不需再改代码。

---

# 历史：ITEM-002 复验修复二（E1 拉低/回读 + 互换角色探测）

**触发**：EV-011（受测 md5 `e309bac2…` = 上一轮交付件）判 **TEST_FAIL**：核心可观察 0/10 周期 `q=1` 且稳定复现；该能力按规则“无 ACK/功能不符首先是 TEST_FAIL、未知根因不得据以证明硬件损坏或软件无可修”将本项交回实现，并列出竞争假设 H1–H11 与最小区分实验 E1–E4。其中 **E1 明文属实现能力**：在**任何 I²C 事务之前**增加一次「把 SDA/SCL 拉低→回读；释放→回读」的自检并打印，用于切分「主机拉不低」（H5）与「器件不应答」（H1/H2/H3）。

## 1. 本轮实现

| # | 内容 | 位置 |
|---|---|---|
| 1 | **新增上电 `IOTEST` 行**：`IOTEST sda_lo=<0\|1> scl_lo=<0\|1> idle=<0..3> swap=<addr7[,addr7]\|none>`。`sda_lo`/`scl_lo` = 把 PA04/PA03 按开漏拉低后回读（**0 = 主机确实拉低成功**，1 = 拉不低）；`idle` = 释放后回读位掩码（bit0=SCL、bit1=SDA）；`swap` = 把 SDA/SCL **角色对调**后候选 0x44/0x45 中收到 ACK 的地址（无则 `none`） | `USER/src/measure.c` (`sensor_io_diag_scan`)、`USER/src/debug_trace.c` (`debug_trace_iotest`)、`USER/inc/{measure,debug_trace}.h` |
| 2 | 电平驱动顺序保证**不产生 START/STOP**（先 SCL 低再 SDA 低；释放先 SDA 高再 SCL 高），且整个自检在任何 I²C 事务之前完成（`main.c` 上电顺序：`BOOT` → `IOTEST` → `BUS`） | `USER/src/measure.c`、`USER/src/main.c` |
| 3 | 互换角色影子设备（仅自检用，不注册进 `i2c_obj_find` 列表）；自检结束后恢复本工程引脚角色并释放 | `USER/src/measure.c` |
| 4 | 仅 `SENSOR_DEBUG_UART=1` 编译；`=0` 时两个扫描均为编译期空实现 | 同上 |

**未改动**：`BUS`/`S`/`G` 行的字面与字段、采样/判定/上报/冻结逻辑、GXHT40 校验与重试、总线恢复原语、433/BLE 布局。

## 2. 判读表（交 tester/用户；本能力不代替采集）

| IOTEST 实板读数 | 结论 | 后续 |
|---|---|---|
| `sda_lo=0 scl_lo=0` | 主机能真正拉低**并**释放两条线 ⇒ 地址位/时钟位真实出现在总线上 ⇒ 故障在**器件/接线侧**（H1 贴装/焊接、H2 损坏、H3 异常闩锁） | E2 真真实掉电复上电；E3 目视/通断/换件（用户动作） |
| `sda_lo=1` 或 `scl_lo=1` | 主机**拉不低**该线（引脚驱动器/走线/配置层）⇒ **H5**；此时**不应把传感器判为故障件** | 板级/硬件定位（需位级手段） |
| `swap=44`（或 `45`） | 板级装配把 SDA/SCL **接反**（与网表不符）⇒ **H4** 被证实 | 交硬件/装配核对；**不**用固件适配掩盖 |
| `swap=none` | 未发现接反（与网表一致） | 按 `sda_lo`/`scl_lo` 分支继续 |
| `idle≠3` | 释放后线未回高（缺上拉/短路） | 硬件排查（与 `BUS idle` 一致） |

> `swap` 是本能力在 E1 请求之外的**同一思路扩展**（成本 ≈ 2 ms / 2 个地址），目的是一次构建就切掉原本需要万用表的装配接反假设；它**只报告事实**，不自动适配接反的板子。

## 3. 验证（本轮实际执行）

- **宿主自检合计 155 项 0 失败**：`test/build_test.sh` 40+**39**+**37**+19+20（measure-flow 新增 [12] 拉低/回读/释放/对调 4 项 + 负对照 1 项；debug-trace 新增 [T11] IOTEST 典型/最坏/预算/仅整数 4 项）；`test/host_gxht40_check.c` 60/60；`test/host_sf_i2c_bus_check.c` 35/35。负对照：把 PA03 模拟为“回读恒高”时 `scl_lo=1` 而 `sda_lo=0`，证明该字段能区分主机拉不低。
- **tester 自有 harness 只读复跑**（未修改其文件）：`test/host_gxht40_verify_ev.c` **93/93**（含其已更新的 OBS-2 新语义）、`test/host_diag_probe_verify_ev.c` **85/85**。
- **构建**：GNU 0 错误，FLASH **33,564 B** / RAM **1,808 B**（较上轮 33,076/1,768 为 +488/+40 B；27 条告警均为既有类别）；Keil `mdk_build rebuild` **0 Error / 1 既有告警**，`Code=17808 RO-data=636 RW-data=116 ZI-data=1676`，axf md5 **`bcfc14dc27ee9afeb02df2e0e7b4b586`**；镜像内含 `IOTEST sda_lo=` 等字符串。
- **自检发现并修复的缺陷（本轮）**：新增的 `sensor_io_diag_scan` 首版误置于 `#if SENSOR_DEBUG_UART` 之外，导致 `SENSOR_DEBUG_UART=0` 时重定义/引用被条件编译剔除的符号——在 debug-off 编译复核中暴露并修正；修后 `measure.c`/`debug_trace.c`（`=0`）0 告警 0 错误。
- 原始命令与 stdout：`evidence/build.md`、`evidence/driver_test.md`。

## 4. 未取得与交接

- **实板 `q=1` 仍未取得**：本调用无 `mdk_flash`/串口。本轮交付的是**可复跑的判别观测量**；IOTEST 实板读数与 E2/E3（用户现场动作）齐备后才能分支处置（H5 → 硬件侧；H1/H2/H3 → 现场/换件；H4 → 装配）。
- **tester 需同步**：① TD-002 §4.4 与解析脚本需接受新行 `IOTEST`（`BUS` 字面不变，但位置由“横幅后第一行”变为第二行）；② `evidence/embedded_verify_gxht40_item002.py` 可添 `IOTEST` 字段域与自洽规则（`sda_lo/scl_lo/idle` 取值域、`swap` 为 `none` 或合法 7bit 地址）。
- **设计能力需同步**：FD-002 §5.2 上电顺序与 §6.7 诊断行集合需补 `IOTEST`（本次为验证能力交回的 E1 需求所追加，设计正文尚未收录）。
- **不属本能力**：E2（真实断电复上电）、E3（目视/通断/换件）、E4（示波器/逻辑分析仪）均为现场或仪器动作。

---

# 历史：ITEM-002 复验修复一（FWR-118 重探义务 + OBS-2 结果码一致性）

**触发**：独立验证 EV-009 判 **TEST_FAIL**，两条事实中的**事实 2（可实现缺陷）**：FWR-118 / FD-002 §6.6.3(4) / TD-002 B18 要求在**任何**测量失败（含 CRC 错、量程无效）后，下一周期必须重探 0x44/0x45 **两个**候选地址并先做有界总线恢复；实测在**缓存地址 CRC 错**与**量程无效**后缓存地址仍被保留、下一周期只探 `0x88`（`0x8A=0`）且未调用恢复。同时 EV-009 §4.3 将 **OBS-2**（“恰好一个候选 ACK 而读不通”时 `s=2` 与 `a44=1` 互相矛盾）交本能力裁决。（事实 1：器件地址级不应答，属现场条件，非固件可修。）

## 1. 本轮修复（均在 `USER/src/gxht40.c`）

| # | 修复 | 内容 |
|---|---|---|
| 1 | **FWR-118 字面符合** | `gxht40_measure()` 在 **CRC 错**与**量程无效**分支也清 `s_addr7=0`（原来只有读事务失败才清）；循环以失败退出时再清一次（覆盖所有失败路径）。`gxht40_acquire()` 的完整重探分支**依次探完 0x44 与 0x45 两个候选**（即使 0x44 已成功也把 0x45 探完），并在重探前调用 `i2c_bus_recover()`；0x45 的帧先读入局部缓冲，仅当 0x44 未成功时才拷回，不破坏 0x44 的结果。稳态快路径仍仅在“有缓存地址且上轮未失败”时使用 |
| 2 | **OBS-2 结果码一致性（本能力裁决）** | 两个候选都失败后，若本轮**有任一候选地址 ACK 过**（器件在总线上），则把 `GXHT40_ERR_NO_DEVICE` 收紧为 `GXHT40_ERR_IO`（“器件在但读不通”），使 `G` 行的 `s=` 与 `a44/a45` 不再矛盾；确实无任何 ACK 时仍报 `2` |

OBS-2 裁决依据：① EV-009 §4.3 明确将“结果码语义 vs 脚本规则哪一侧修正”交设计/实现；② 测试方**自己**在 ITEM-001 的脚本里有规则「`s=2` 时两地址 ACK 均应为 0」；③ 测试方 T1 轮 harness `host_diag_probe_verify_ev.c` 的 D6 本就期望「地址 ACK 但读用尽 ⇒ `ERR_IO`」——该期望在本轮修复后恢复通过。

**未改动**：双字 CRC-8、整数 x10 换算与有效域、读重读(5)/整帧重测(3)、命令白名单（仅 `0xFD`，无 `0x94`/加热器）、失败不写输出、`i2c_bus_recover` 原语本身、433/BLE 布局、引脚归属；`sensor_config.h` 无改动。

## 2. 预期行为（可供独立复验）

1. 上轮**任一**失败（`NO_DEVICE`/`IO`/`CRC`/`RANGE`）后，下一周期先有界恢复、再**完整重探 0x44→0x45**；稳态周期仍只走缓存快路径（每周期 1 次命令+读取，无额外开销）。
2. 失败周期仍不上报/不推进前值/不构造 0 值；输出不被修改。
3. 若器件 ACK 地址但读不通，`G` 行呈现 `s=3 a44=1 …`（不再是 `s=2 a44=1`）；`s=2` 仅在两地址均无 ACK 时出现。
4. 有界性不变：单轮测量最多 `GXHT40_MEAS_RETRY` 轮 × 2 候选探测；恢复仅在重探分支调用一次（总线空闲时不产时钟）。

## 3. 验证（本轮实际执行）

- **实现侧自检全部通过**：`test/build_test.sh` **146/146**（40+34+33+19+20）；`test/host_sf_i2c_bus_check.c` **35/35**；`test/host_gxht40_check.c` **60/60**：新增 [13] CRC 错后缓存失效 + 下一轮完整重探两候选、[14] 量程无效同样处理；[1] 期望更新为「首次访问完整重探：`0x88`+`0x89` 后仍探 `0x8A`」。
- **独立资产复跑**（仅编译运行，未修改 tester 任何文件；结论权归 tester）：
  - `test/host_gxht40_verify_ev.c`：**82/83**，唯一失败为 T15 的 OBS-2 *观察*断言（其目的就是复现旧语义 `status==2 && ack44==1`），按本轮 OBS-2 裁决需 tester 更新为新语义 `status==3 && ack44==1`；**T11/T12/T13/T14/T17/T17b 全部通过**（T17/T17b 现为 `0x88=1 0x8A=1 recovery_calls=1`，即 FWR-118 字面符合）。
  - `test/host_diag_probe_verify_ev.c`：**85/85**（D6「读 NACK 用尽 ⇒ `ERR_IO`」恢复通过）。
  - `test/host_sf_i2c_verify_ev.c`：51/51；`test/host_diag_line_verify_ev.c`：36/36。
  - `test/host_measure_flow_verify_ev.c` 仍无法编译（期望旧 `bool light_sample()`，自 run7 T2 起即如此，tester 已用 `host_diag_probe_verify_ev.c` 取代）——**与本轮改动无关**，登记供 tester 清理。
- **构建**：GNU 交叉编译 0 错误，FLASH **33,076 B** / RAM **1,768 B**（较上轮 33,016 为 +60 B；27 条告警均为既有类别）；Keil `mdk_build rebuild` **0 Error / 1 既有告警**，`Code=16572 RO-data=624 RW-data=76 ZI-data=1676`，axf md5 **`e309bac2a34fd210ee966d6da9001625`**；`SENSOR_DEBUG_UART=0` 下 `gxht40.c` 0 告警。
- 原始命令与 stdout：`evidence/build.md`、`evidence/driver_test.md`（本轮修复节）。

## 4. 交接（不当作通过）

- **tester 需更新**：`test/host_gxht40_verify_ev.c` 的 T15 观察断言（旧 `s=2` → 新 `s=3`）；`evidence/test.md` EV-009 §4.3 的文字与（若有关）`evidence/embedded_verify_gxht40_item002.py` 中 `s=2` 相关规则现在与实现**一致**，无需放宽。
- **实板 `q=1` 仍未取得**：器件在地址级完全不应答（EV-009 §3.1），本修复不改变也无法改变该事实；EV-009 §5 的现场动作（目视/万用表核对 U9 贴装与连通、一次真实掉电重启、必要时换件）是唯一前行路径，现场动作前后**不需要**再改代码。
- **测试资产清理（可选）**：`test/host_measure_flow_verify_ev.c` 旧签名（tester 自有资产）。

---

# 本轮 run8：ITEM-002（T2：GXHT40 采集失败定位与按证据修复；已实现，复验后修复见上节）

**任务**：依据上一增量在实板上取得的 `BUS`/`G` 原始数据定位失败环节（器件不应答/地址变体/读 NACK/CRC/量程/总线卡死）并在驱动/总线层按证据修复；保留双字 CRC-8、整数 x10 换算与有效域、有界读重读与有界整帧重测；周期路径不得用软复位 0x94/加热命令；任一失败周期仍不上报/不推进前一有效温度/不构造 0 值；**任何失败（含缓存地址读失败）后下一周期必须重新探测 0x44/0x45，并在重探前做有界总线恢复（9 个 SCL 脉冲 + STOP）**；等待与重试不低于手册；可观察：实板每周期 `q=1`、`t/h` 在有效域且与环境相符。

## 1. 一手输入与定位（不采信推测）

| 输入 | 原始事实 | 来源 |
|---|---|---|
| 上电总线身份 ×3 | `BUS idle=1 scl=1 sda=0 ack=none`（boot#1）；`BUS idle=3 scl=1 sda=1 ack=none`（boot#2/#3） | `evidence/test.md` EV-008 §2.2 |
| 失败周期 ×6 | **全部** `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`（= `GXHT40_ERR_NO_DEVICE`，两候选地址均无 ACK，未发起读） | 同上 §2.3 |
| 负对照 | 若 SDA 被持续拉低，113 地址扫描必然报出满 8 地址 + `+`；实得 `ack=none` ⇒ **排除“SDA 卡低”解释** | 同上 §3.3（宿主模型实测） |
| 引脚/拓扑（本能力一手复核 netlist） | N05845: `U9.SDA`(pin1)—`U7.PA04` + R1 4.7K→BAT；N06024: `U9.SCL`(pin2)—`U7.PA03` + R2 4.7K→BAT；`U9.3`=BAT(VDD)、`U9.4`=GND、`U9.5`=NC | `sensor_hardware/pstxnet.dat`、`MAIN_BOARD.BOM`（U9=`gxht40`） |
| 波形与驱动逻辑（实现侧自检回归） | 地址字节/START/STOP/ACK 采样位级正确；112 地址扫描只发地址字节；`gxht40` 失败不改输出 | `test/host_sf_i2c_bus_check.c`、`test/host_gxht40_check.c` |

**定位结论**：失败发生在**「器件是否在总线上应答」这一层（地址级）**，而非协议/CRC/量程层，也非上拉/供电缺失（`idle=3`）或观测通路。地址扫描已覆盖 0x44 与 0x45，故也不是“地址变体未覆盖”。

**因此驱动/总线层可做且被任务/需求明文要求的修复**（不超出设计要求，不降低任何校验）：

1. 失败后的**有界总线恢复**（≤9 个 SCL 脉冲 + STOP）——针对 `boot#1 idle=1` 与“从机字节中途被复位后握住 SDA”的异常保持态；
2. 任一失败后**强制失效缓存地址并重探 0x44/0x45**（原来缓存地址读失败会直接返回，不重探另一地址）；
3. 首次访问前的 **tPU 上电余量**（手册 ≤1 ms，配置点常量 `GXHT40_POWER_ON_WAIT_MS` 原先已定义但未被使用）。

**明确不能由固件解决、不得伪造的部分**：若器件在**任何时刻对任何地址都不应答**，则固件无法使其应答；报告必须停在“无应答”这一事实上，并把候选交给持有万用表/目视条件的角色（U9 贴装/焊接、VDD 与 PA03/PA04 连通性、器件异常保持态）。**本轮不声称 `q=1` 已取得。**

## 2. 实际改动（本轮）

| 文件 | 动作 | 内容 |
|---|---|---|
| `USER/inc/sf_i2c.h` / `USER/src/sf_i2c.c` | 新增（加法） | `i2c_bus_recover(dev)`：释放两线→若 SDA 仍低则补 ≤9 个 SCL 脉冲→补一个合法 STOP（避免误发 START）→返回 SDA 是否释放；不写任何器件命令、不无限等待；**既有函数语义未改** |
| `USER/src/gxht40.c` | 修改 | ① `gxht40_acquire()`：缓存地址**任一失败**即 `s_addr7=0`，重探前调用 `i2c_bus_recover()`；0x44 读失败不再提前返回，继续重探 0x45（FWR-118）；② 首次访问前 `delay_ms(GXHT40_POWER_ON_WAIT_MS)` 一次（`gxht40_init` 重置）；③ CRC/有效域/`GXHT40_READ_RETRY(5)`/`GXHT40_MEAS_RETRY(3)`/命令白名单/失败不写输出**均不变** |
| `test/host_sf_i2c_bus_check.c` | 扩展 | [7] 空闲总线恢复不产时钟；[8] SDA 持续被拉低→11 个上升沿（释放1+9脉冲+STOP1）、返回 TIMEOUT、不发事务；[9] 从机第 4 个上升沿前释放→5 个上升沿提前结束、返回 SUCCESS（共 35 项） |
| `test/host_gxht40_check.c` | 扩展 | [11] 缓存地址 0x45 读失败→缓存清空、**仍重探了 0x44**、下一轮重新探测后成功且缓存更新；[12] 首次访问恰好一次 `GXHT40_POWER_ON_WAIT_MS`、`tMEAS` 仍 1 次 10 ms、后续测量不再重复（共 48 项） |
| `.gitignore` | 修改 | `/build.log` → `/build*.log`（`mdk_build` 实测生成 `build.log` / `build_<timestamp>.log` 变体，避免共享仓库出现未跟踪文件） |

**未改动**：`fw_core.c`（升温方向/边界属 T4）、光照标定（T3）、发送冻结（T5）、433/BLE 布局、网关与 Android、引脚归属。

## 3. 预期行为（真实目标；本轮未执行实板复测）

1. **器件恢复应答时**：下一采样周期先做有界恢复、重新探测 0x44→0x45，成功后 `S` 行 `q=1` 且 `t/h` 在有效域并与环境相符；`G` 行不再出现。
2. **器件仍不应答时**：行为与 ITEM-001 一致（`q=0`、`S`+`G` 行、`G s=2 a44=0 a45=0`），不上报/不推进前值/不构造 0 值；不得伪造成功。
3. **有界性**：单周期内最多 `GXHT40_MEAS_RETRY` 轮 ×（1 次恢复 + 2 个候选地址）探测，每轮 ≤10 ms tMEAS + 读重读，最坏活动时间仍在百 ms 量级（远小于 3 分钟节拍）；恢复仅在需要时产生 ≤9 个 SCL 脉冲。
4. **成功路径无额外开销**：稳态下走缓存地址快路径，`i2c_bus_recover` 在总线空闲时立即返回且不产时钟；tPU 余量仅首次访问一次。

## 4. 验证（本轮实际执行）

- **宿主自检合计 229 项 0 失败**：`test/build_test.sh` 146 项（40+34+33+19+20）+ `host_sf_i2c_bus_check.c` 35 项 + `host_gxht40_check.c` 48 项。
- **GNU 交叉编译**：0 错误；FLASH **33,016 B** / RAM **1,768 B**（较 ITEM-001 的 32,840 为 +176 B；27 条告警均为既有类别）。
- **Keil MDK `mdk_build rebuild`（授权工具）**：0 Error / 1 Warning（既有 `main.c(224) while(k--);` 空体）；`Code=16488 RO-data=624 RW-data=76 ZI-data=1676`；`Project.axf` md5 `196b0040d5378eaf2edcd6a3d5fdf9a5`。
- **`SENSOR_DEBUG_UART=0` 复核**：`gxht40.c`/`measure.c` 0 告警 0 错误；`sf_i2c.c` 仅两处**改动前既有**的 `err may be used uninitialized`（属未触碰的旧函数）。
- 原始命令与 stdout：`evidence/build.md`（本轮 T2 节）、`evidence/driver_test.md`（本轮 T2 节）。

## 5. 未完成与交接（如实记录，不当作通过）

- **实板 `q=1` 未取得**：本调用工具列表仅含 `mdk_build`；部署与 COM42 复测由嵌入式测试能力执行。若复测仍 `q=0`，按 EV-008 §3 的“地址级无应答”结论交现场硬件核对（U9 贴装/焊接、VDD 与 PA03/PA04 连通、器件异常保持态）；**不得由固件继续猜测或放宽校验**。
- **测试方自有资产同步（属执行能力，本能力只登记）**：`host_gxht40_verify_ev.c` / `host_sf_i2c_verify_ev.c` 需补 `i2c_bus_recover` 与失败后重探的独立验证（TD-002 §3.2 已登记）。
- **设计文档数值更新（属设计能力，本能力只登记）**：`RTA-002` §3.2 的 T_MEAS 最坏时间语句（“失败重试时最多约 3 倍”）已不覆盖含双地址重探的最坏情形（最多 `GXHT40_MEAS_RETRY` 轮 × 2 个候选地址 ≈ 6 次 10 ms 级命令等待）；实际最坏值见本文件 §3.3，供设计能力下轮更新。
- **后续增量（非本项）**：T3 全暗基准回填；T4 判定升方向与严格边界；T5 待上报冻结快照。

---

# 本轮 run8：ITEM-001（T1：GXHT40 采集失败可诊断；已 IMPLEMENTED + TEST_PASS）

**任务**：上电横幅之后、任何 I²C 事务之前，读 PA03(SCL)/PA04(SDA) 空闲电平并对 0x08..0x77 逐地址做有界探测
（START + 地址写字节 + ACK 判定 + STOP，只发地址、不写命令、不读数据、NACK 后释放总线），打印一条 `BUS` 行
（空闲电平位掩码 + ACK 地址集合，无则 `none`，最多 8 个，超出加 `+`）；温湿度失败的周期在该周期 `S` 行之后追加一条 `G` 行
（结果码/0x44·0x45 地址 ACK/读重读次数/整帧重测次数/最近一次成功读回的 6 字节或 12 个 `-`）；每行 ≤96 B、仅整数与大写十六进制；
成功周期输出与既有 `S` 行逐字节相同；新增探测原语不改既有软 I²C 语义；诊断不改采样/判定/上报/冻结；`SENSOR_DEBUG_UART=0` 全部为空实现。

设计映射：FD-002 rev 5.0 §5.2（上电一次性扫描时序）/§6.6.1（诊断数据与判别）/§6.7（行格式与预算）/§11.4–§11.5/§11.9；
FWR-116；RTA-002 rev 5.0 §1.2（T_DIAG/T_TRACE）与 §4 不变量；TD-002 rev 5.0 §4.1 T-L0-11、§4.3 T-L1-14、§4.4 T-L2-11/12。

## 实际改动（本轮）

| 文件 | 动作 | 内容 |
|---|---|---|
| `USER/inc/sensor_config.h` | 修改 | 新增单一配置点常量：`SENSOR_BUS_DIAG_FIRST_ADDR7=0x08`、`SENSOR_BUS_DIAG_LAST_ADDR7=0x77`、`SENSOR_BUS_DIAG_MAX_ACK=8`、`SENSOR_BUS_DIAG_IDLE_SETTLE_MS=1`；诊断行共用既有 `SENSOR_DEBUG_UART_MAXLINE=96` |
| `USER/inc/sf_i2c.h` / `USER/src/sf_i2c.c` | 新增（加法） | `i2c_probe_addr(dev, slave8)`：START → 地址写字节 → ACK 判定 → STOP；无器件时也发 STOP 释放总线；**未修改既有函数** |
| `USER/inc/gxht40.h` / `USER/src/gxht40.c` | 修改 | `gxht40_diag_t {status, ack44, ack45, read_retry, attempt, raw_valid, raw[6]}` + 只读 getter `gxht40_diag_fetch()`；在既有探测/读循环内记录（地址 ACK 在命令被 ACK 时记录；读重读次数与原始 6 字节在读成功时记录；用尽时 `read_retry=GXHT40_READ_RETRY`）；`gxht40_init()` 清快照；**失败语义、重试上限、输出写入规则、命令白名单均不变** |
| `USER/inc/debug_trace.h` / `USER/src/debug_trace.c` | 修改 | `debug_trace_bus_t` + `debug_trace_bus()`（`BUS idle= scl= sda= ack=`）；`debug_trace_sample_t` 增加 `diag_*` 字段，`diag_valid=1` 时在 `S` 行之后追加 `G` 行；`S` 行字面与字段顺序**未改**；关闭态提供空内联 |
| `USER/inc/measure.h` / `USER/src/measure.c` | 修改 | `bsp_i2c_init()` 幂等化（一次性物理层/对象注册，供上电诊断与采样共用）；新增 `sensor_bus_diag_scan()`（读空闲电平 + 有界地址扫描，`SENSOR_DEBUG_UART=0` 时为空实现，不读引脚/不发探测）；`temperature_process()` 在失败分支取 `gxht40_diag_fetch()` 到本周期快照，`sensor_trace_fetch()` 传递 `diag_*`（成功周期 `diag_valid=0`） |
| `USER/src/main.c` | 修改 | 横幅之后调用 `sensor_bus_diag_scan()` 并打印一条 `BUS` 行（仅 `SENSOR_DEBUG_UART=1`；一次） |
| `test/host_debug_trace_check.c` | 扩展 | T8 `BUS` 行（`none`/地址列表/截断 `+`/最坏 53 B/仅大写十六进制）；T9 `G` 行（最宽 46 B、`raw` 12 位 hex、`raw` 占位 12 个 `-`）；T10 成功周期无 `G` 行；既有 `S` 行逐字节断言保持不变 |
| `test/host_sf_i2c_bus_check.c` | 扩展 | `i2c_probe_addr` 线上字节（仅 1 个地址字节，无命令）、ACK/NACK 返回值、NACK 后 STOP 释放 |
| `test/host_gxht40_check.c` | 扩展 | 成功/读 NACK/CRC 错/无器件/量程无效五种情形的诊断快照断言 |
| `test/host_measure_flow_check.c` | 扩展 | 失败周期 `diag_valid=1` 且字段逐项传递、成功周期 `diag_valid=0`；总线扫描（`idle=3`/`ack_count=0`）与 `bsp_i2c_init` 幂等 |
| `test/build_test.sh` | 修改 | 第 2 阶段补 `-DSENSOR_DEBUG_UART=1`（扫描为真实实现）；`_hostbin/` 产物改名以绕过宿主 EDR 按文件名拦截新建 exe（原因见头部注释，编译参数与用例不变） |
| `.gitignore` | 修改 | 新增 `/build.log`（`mdk_build` 会在仓库根生成该工具日志） |

**未改动**：`USER/src/fw_core.c`（判定方向/边界属后续增量 T4）、光线标定（T3）、发送冻结（T5）、433/BLE 布局、网关与 Android、MDK/IAR 工程文件列表（未新增源文件）。

## 预期行为（真实目标可观察；本轮未执行，交嵌入式测试）

1. COM42@9600 上电（捕获窗口内复位）：横幅之后恰出现**一条** `BUS` 行，`idle` 与 `scl`/`sda` 位一致，`ack` 为 `none` 或合法 7bit 地址集合（最多 8 个，含 `+` 即被截断），且与随后周期的 `G a44/a45` 不矛盾；后续周期不再出现 `BUS`。
2. 温湿度失败周期（`S` 行 `q=0`）：紧随一条 `G` 行；结果码取驱动结果码（1=入参/2=无器件/3=读失败/4=CRC 错/5=量程无效），`a44`/`a45`、`rd∈0..5`、`at∈1..3`、`raw` 为 12 位大写 hex 或 12 个 `-`，可直接用独立 CRC-8 复算 `raw` 的两个字。
3. 成功且未上报周期：输出与既有 15 字段 `S` 行逐字节相同（仅一行），采样间期仍无任何字节。
4. 诊断不改变采样/判定/上报/冻结状态；关闭串口只碰 UART1/PA05/PA06，不复位 GPIOA。

## 验证（本轮实际执行）

- **宿主自检 `test/build_test.sh`：146 项 0 失败**（40 + 34 + 33 + 19 + 20）；新增用例覆盖 `BUS`/`G` 格式与预算、条件出现、探测原语语义、诊断快照传递与幂等初始化。
- **自检发现并修复 1 处实现缺陷**：`G raw=` 占位原为 6 个 `-`（与 6 字节 hex 的 12 位宽不一致），改为 12 个 `-` 后复测通过（见 `evidence/driver_test.md` §6）。
- **GNU 交叉编译**：0 错误；FLASH **32,840 B** / RAM **1,768 B**（较基线 31,952/1,736 为 +888/+32 B）；27 条告警均为既有类别，无一条指向本轮新增代码。
- **Keil MDK `mdk_build rebuild`（授权工具）**：0 Error / 1 Warning（既有 `main.c(224) while(k--);` 空体）；`Code=16316 RO-data=624 RW-data=76 ZI-data=1676`；`Project.axf` md5 `c39bf314851a0988a5f347c73604d105`；镜像内含 `BOOT fw=FD-002r4 uid=`、`BUS idle=`、` ack=`、` raw=` 字符串（证明诊断行已编入交付件）。
- **`SENSOR_DEBUG_UART=0` 复核**：`measure.c`/`debug_trace.c`/`gxht40.c` 0 告警 0 错误（扫描为空实现、无 UART/I2C 探测依赖）。
- 原始命令与 stdout：`evidence/build.md`（本轮 T1 节）、`evidence/driver_test.md`（本轮 T1 节）。

## 未执行与交接（如实记录，不当作通过）

- **真实目标观测未执行**：本调用工具列表仅含 `mdk_build`；`mdk_flash` 部署与 COM42 采集、`BUS`/`G` 行实板核对与 OBS-1 定位由嵌入式测试能力执行（TD-002 §4.4 T-L2-11/12；`evidence/test.md`）。本项只交付**可供独立验证**的诊断接口与构建。
- **测试方自有资产需同步**（属嵌入式测试执行能力，本能力只登记、未修改）：`evidence/embedded_verify_uart_trace_item001.py` 增加 `BUS`/`G` 行解析（`S` 行不变）；测试方可新增地址扫描原语/失败后重探/`G raw` 独立 CRC 复算与 `bsp_i2c_init` 幂等的独立 harness（TD-002 §3.2 已登记）。
- **后续增量（非本项）**：T2 依实板 `BUS`/`G` 数据修复采集（含失败后重探与有界总线恢复）；T3 全暗基准回填；T4 判定升方向与严格边界；T5 待上报冻结快照。本轮**未**改动 `fw_core.c` 判定与发送路径。

---

# 历史：本轮 run7 T2（光照采集：完全无光基准 1/3 判据与有效性）

**任务**：每次采样在 PB05 输出供电、稳定等待后用 PB04/AIN11 多次转换，采样结束把 PB05 置低；输出结构化结果 `{valid, samples_ok, mean_adc_code, code_min, code_max, dark}`（均值只取成功样本，全部转换超时为 valid=false）；无光判定 `dark = valid 且 3*mean_adc_code >= C_dark`（uint32 乘法、边界相等为暗），每笔独立、不使用旧 350/250 滞回或历史暗态；ADC 全部转换超时不得用满量程合成暗态；`C_dark` 在实板上完全遮光实测后回填 `LIGHT_DARK_REF_CODE` 并置 `LIGHT_DARK_CALIBRATED=1`，标定记录含板件/供电/遮光方式/原始样本分布/取样次数/配置版本。

设计映射：FD-002 rev 4.0 §2.3（光敏分压）、§6.3（无光判据与标定方法）、§10（异常策略）、§11.7；FWR-102/110/113；TD-002 §4.5 T-L3-01..08、§4.1 T-L0-04。

## 实际改动（本轮已完成）

| 文件 | 动作 | 内容 |
|---|---|---|
| `USER/inc/sensor_config.h` | 修改 | 删除 `LIGHT_DARK_ENTER/EXIT/HYSTERESIS_STEP` 与 `LIGHT_ADC_FULL_SCALE`；新增 `LIGHT_DARK_REF_CODE`（未标定占位 0，允许 `-D` 覆盖供 harness 验证）与 `LIGHT_DARK_CALIBRATED`（本轮 = 0，未标定/未验收）；注释冻结 `dark = valid && 3*mean >= C_dark` 与标定记录要求 |
| `USER/inc/fw_core.h` / `USER/src/fw_core.c` | 修改 | 删除 `light_code_is_dark`（滞回）；新增纯函数 `light_is_dark(mean_adc_code, valid, c_dark)`（`uint32` 乘法、`valid=false` → false、边界相等为暗） |
| `USER/inc/light.h` / `USER/src/light.c` | 修改 | 新增 `light_result_t {valid, adc_ok, samples_ok, mean_adc_code, code_min, code_max, dark}`；`light_sample()` 返回该结构；**删除滞回状态与满量程回退**（全部超时 → `adc_ok=false`、`mean/min/max=0`、`valid=false`、`dark=false`）；`valid = adc_ok && LIGHT_DARK_CALIBRATED`；删除 `light_reset_state`/`light_last_code`/T1 期间的统计 getter |
| `USER/src/measure.c` / `USER/inc/measure.h` | 修改 | 本周期光照结果存为 `s_light`（判定与调试轨迹共用一份数据）；判定改传 `s_light.dark`；`sensor_trace_fetch()` 从 `s_light` 填 `light_*` 字段 |
| `USER/src/main.c` / `USER/inc/debug_trace.h` | 修改 | 移除 main.c 对 light 统计 getter 的调用与 `light.h` 包含（光照字段由 measure.c 快照提供）；更新 `light_valid`/`light_dark` 字段注释为新语义 |
| `test/host_light_check.c`、`test/mock_light/`、`test/build_test.sh`、`test/host_sensor_core_test.c`、`test/host_fw_core_pure_check.c`、`test/host_measure_flow_check.c` | 新增/修改 | 实现侧自检改为新语义：新增 `mock_light/` 影子 MCU 层驱动真实 `light.c`；`build_test.sh` 扩为 4 阶段（光照阶段跑未标定与标定后两个变体） |

未改动：`sensor_decide_report`（升温方向/严格边界属 T3）、433/BLE 帧布局、网关与 Android。

## 已验证行为（宿主/构建）

1. `light_is_dark`：`valid=false` 恒 false；`3*mean == C_dark` 为暗、少 1 不为暗；`C_dark=1..4095` 全量 `mean=ceil(C/3)` 边界与整数参考一致；无历史状态。
2. `light.c`：PB05 供电（每次转换期间均为高）→ 8 次 AIN11 转换 → PB05 置低、ADC 关闭；均值/极值只取成功样本；部分超时按成功数记录；**全部超时 → `samples_ok=0`、`adc_ok=false`、`mean/min/max=0`、`valid=false`、`dark=false`（无 4095 合成）**。
3. 有效性门禁：`LIGHT_DARK_CALIBRATED=0` 时即使 ADC 成功且读数为满量程，`valid=false`、`dark=false`；标定变体（`-DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=3000`）下 `valid=true` 且 `dark = 3*mean>=3000`。
4. 构建：GNU 交叉编译 FLASH **31,952 B** / RAM **1,736 B**（0 错误，无新增告警指向本院）；Keil `mdk_build rebuild` **0 Error / 1 既有告警**，`Program Size: Code=13976`，产物 md5 `3ee3830814ec61384785998556a4461e`。
5. 宿主自检（`test/build_test.sh`，4 阶段）：40/40 + 24/24 + 24/24 + 19/19（未标定）+ 20/20（标定变体）。

原始结果见 `evidence/driver_test.md`（T2 节）与 `evidence/build.md`（T2 节）。

## T2 未完成部分与阻塞（本轮返回 BLOCKED 的原因）

任务要求的**全暗基准 `C_dark` 实板标定未完成**，因此本轮**不满足 T2 的完整交付与观测验收项**：

| 未完成项 | 判定依据 | 阻塞原因 |
|---|---|---|
| `C_dark` 完全遮光实测 + 回填 `LIGHT_DARK_REF_CODE`/`LIGHT_DARK_CALIBRATED=1` + 标定记录（板件/供电/遮光方式/原始样本分布/取样次数/配置版本） | 任务文本；FD-002 §6.3/§12；TD-002 T-L3-05/07/08 | 本调用工具列表仅 `mdk_build`，**无 `mdk_flash`（部署）与串口采集（COM42）**，无法在真实目标上取得完全遮光下的原始样本分布 |
| 三态观测（有光 `dark=0`、完全遮光 `dark=1` 且 `3*mean>=C_dark`、移开恢复 `dark=0`） | 任务文本「可观测」；TD-002 T-L3-01/02/03 | 同上（需标定后的镜像 + 实板串口） |

**未标定状态已如实记录**：`LIGHT_DARK_CALIBRATED=0`、`LIGHT_DARK_REF_CODE=0`（占位），不使用旧默认 350/250、不使用满量程、不使用亮态读数冒充 `C_dark`；未标定时 `valid` 恒为 false，属设计允许的中间状态（FD-002 §6.3 规则 6；TD-002 T-L3-07①），但**不得**宣称「无光判定已验收」。

**完成后即可闭环所需步骤（交接给具备部署+串口工具的执行）**：

1. `mdk_flash {}` 部署本提交的镜像（`SENSOR_DEBUG_UART=1`，S 行会打印 `o/n/x/a` 原始样本）；
2. 用完全不透光遮蔽物完全包住 J4 光敏器件，采集 ≥30 笔 S 行（可用多个 3 分钟周期），记录 `n/x/a/o` 的分布；
3. `C_dark` 取全暗实测均值（或稳健中位），回填 `LIGHT_DARK_REF_CODE` 并置 `LIGHT_DARK_CALIBRATED=1`，附标定记录（板件标识、供电、遮光方式、原始样本分布、取样次数、配置版本）；
4. 复跑：有光 `dark=0` 且 mean 明显低于 `ceil(C_dark/3)`；完全遮光 `dark=1` 且 `3*mean>=C_dark`；移开恢复 `dark=0`；并核对 S 行 `V=1`。

## 交接（不属本能力执行）

- **测试资产（嵌入式测试）**：本次 API 变更使 4 个 tester 自有独立 harness 需按 TD-002 §3.2 重写/扩展（本次未修改它们）：`host_light_verify_ev.c`（`light_sample` 返回类型、`light_code_is_dark`/`light_last_code` 已删）、`host_fw_core_verify_ev.c`（光照滞回用例 → `light_is_dark`）、`host_measure_flow_verify_ev.c`、`host_rf_report_verify_ev.c`（`bool light_sample()` 桩 → `light_result_t`）；`host_rtc_cadence_verify_ev.c` 不受影响。
- **S 行语义变更**：自本项起 `light.valid = adc_ok && LIGHT_DARK_CALIBRATED`（未标定恒 0），不再是 T1 期间的 `V == (o>0)`；`o/n/x/a` 仍为原始样本观测值。
- **T3/T4**：`sensor_decide_report` 仍为 as-built 降温方向（D-01），待现金等待上报冻结（D-03）未动。

---

# 本轮 run7：T1（UART1 调试串口初始化与可观测打印）

**任务**：固件在上电后初始化 UART1（PA06=`UART1_TXD`、PA05=`UART1_RXD`，对应网表 J3.1/J3.2，9600 8N1、PCLK 8 MHz、轮询发送）并打印启动横幅（固件标识、芯片 UID 前 4 字节、复位来源、串口参数），在每个采样周期于判定与发送之后打印一行不超过 96 字节的整数轨迹；打印后等 TC 再关闭串口、唤醒后重新初始化；关闭只复位/关闭 UART1 与改写 PA05/PA06，不得复位 GPIOA、不得关闭 GPIOA 时钟、不得改动 PA03/PA04 与 PA07/PA08；打印不得改变采样/判定/发送结果与状态快照；Flash 读保护保持注释。

设计映射：FD-002 rev 4.0 §2.4（调试串口）、§4（数据流）、§6.5（轨迹格式）、§11.15–§11.18（约束）；FWR-115；TD-002 §4.4 T-L2-01..10。

### 实际改动（run7 ITEM-001）

| 文件 | 动作 | 内容 |
|---|---|---|
| `USER/inc/debug_trace.h` | 新增 | UART1 调试通道接口与 `debug_trace_sample_t {tick,prev_temp_x10,have_prev,light_valid,light_ok,light_min,light_max,light_mean,light_dark,temp_x10,hum_x10,sample_ok,report,send,retry}`；`SENSOR_DEBUG_UART=0` 时提供空操作内联定义 |
| `USER/src/debug_trace.c` | 新增 | UART1 初始化/关闭/排空与有界整数格式化；`BOOT` 横幅与 `S` 轨迹；UART1 与 PA05/PA06 的唯一所有者 |
| `USER/inc/sensor_config.h` | 修改 | 新增 `SENSOR_DEBUG_UART`（单一配置点：目标编译器默认 1、宿主机默认 0）与 `SENSOR_DEBUG_UART_MAXLINE=96`；新增 `DEBUG_UART*` 引脚/波特率宏 |
| `USER/src/main.c` | 修改 | 上电打印启动横幅（`SYSCTRL_GetAllRstFlag()`）；主循环在判定+发送之后发布 S 轨迹并排空/关闭 UART1；新增 `rtc_tick_total` 累计分钟节拍（只读调试计数）；移除已失效的 printf 重定向与旧 UART 时钟使能；Flash 读保护保持注释 |
| `USER/src/measure.c` | 修改 | 移除 `UART1_Configure`/`UartGPIO_Configuration`/`DebugUART_Close`（旧 `DebugUART_Close` 会复位 GPIOA，损坏 PA03/PA04 软 I²C 与 PA07/PA08 SWD）；新增 `sensor_trace_fetch()`（只读快照，不改变业务状态）；光照结果记录点提前到光采样之后（不影响判定输入） |
| `USER/src/light.c` / `USER/inc/light.h` | 修改 | 新增原始样本统计（成功数/均值/最小/最大）与只读 getter，供轨迹使用；滞回判定与满量程回退行为**未**改变（属 T2） |
| `MDK/Project.uvprojx` / `EWARM/project.ewp` | 修改 | User 组加入 `debug_trace.c` |
| `test/host_debug_trace_check.c`、`test/mock_trace/`、`test/build_test.sh` | 新增/修改 | 实现侧自检：宿主机影子 MCU 层 + 真实 `debug_trace.c`；`build_test.sh` 扩为 3 阶段 |

### 冻结的 S 行格式（实现字面；字段集合/顺序/单位不变，标签压缩以满足 96 B）

```text
BOOT fw=FD-002r4 uid=<UID前4B hex> rst=<复位标志 hex4> uart=9600
S k=<tick> p=<prev> H=<have> V=<lvalid> o=<ok> n=<min> x=<max> a=<mean> D=<dark> t=<temp> h=<hum> q=<sample_ok> r=<report> s=<send> y=<retry>
```

单位：`k`=RTC 累计 1 分钟节拍；`p`/`t`=0.1℃ x10；`H/V/D/q/r`=0/1；`o`=成功样本数；`n/x/a`=ADC 原始码 0..4095；`h`=0.1%RH；`s`=0 未发送/1 成功/2 失败；`y`=重试计数；`q`（本周期温湿度是否有效）是 TD-002 T-L2-05/T-L5-13 辨识失败周期所需的显式标记。

设计偏离登记（不改变字段集合与语义）：FD-002 rev 4.0 §6.5 的示例字面格式在均值/极值为 3–4 位数时已超 96 B；TD-002 §4.4 T-L2-03 要求实现压缩标签且验证方不得放宽上限，因此采用单字符键。另：`tick` 取 RTC 累计分钟计数（任务文本「RTC 累计节拍」），而非恒定的每周期 3。

### 预期行为

1. 上电初始化 UART1 并输出**一条**启动横幅（固件标识/UID 前 4 字节/复位来源/串口参数）；除横幅与 S 行外无其它调试输出。
2. 每个采样周期在**判定与发送之后**输出**一条** S 行；字段固定、仅整数、含 CRLF、单行 ≤ 96 B（实测最坏 92 B）。
3. 两次采样之间串口**已关闭**（无任何字节）：S 行打印后等 `TC` 再关闭。
4. 关闭只碰 UART1 与 PA05/PA06：不复位 GPIOA、不关 GPIOA 时钟；因此后续周期软 I²C 与 SWD 仍可用。
5. 唤醒后重新初始化 UART1，每个周期都能打印（非仅首条）。
6. `debug_trace_*` 只读传入快照，不修改 `report_req`/前值/冻结样本/发送结果。
7. `SENSOR_DEBUG_UART=0` 时全部调用编译为空，采样/判定/上报行为不变；`main.c` 的 Flash 读保护两行仍为注释。

### 验证（本轮实际执行）

- `gcc/build.sh` 交叉编译：0 错误，**FLASH 32,020 B / RAM 1,736 B**；`debug_trace.c` 无浮点引用（`arm-none-eabi-nm` 0 命中）。
- `mdk_build {action:rebuild}`（Keil ARMCLANG V6.24）：**0 Error / 1 Warning**（既有 `while(k--);` 空循环告警，非新增），`debug_trace.c` 已入构建，产物 `MDK/output/exe/Project.axf` 含 `BOOT fw=FD-002r4 uid=`（证明 Keil 目标构建下 `SENSOR_DEBUG_UART=1`）。
- `test/build_test.sh`（3 阶段）：38/38 + 24/24 + **24/24**；新增 `host_debug_trace_check.c` 覆盖横幅/S 行最坏 92 B 预算/字段顺序/仅整数/无截断/关闭语义（含排空）/幂等/唤醒重初始化/无副作用。
- 既有独立验证 harness 回归（未修改任何 `*_verify_ev.c`）：fw_core 53/53、light 45/45、gxht40 42/42、sf_i2c 35/35、measure 流程 79/79、rf_report 30/30、gw_compat 11/11、rf_frame 10/10、rtc 节拍 17/17。

原始命令与 stdout 见 `evidence/build.md`（T1 节）与 `evidence/driver_test.md`（T1 节）。

### EV-006 缺陷修复（本轮，针对 `evidence/test.md` TEST_FAIL）

| 缺陷 | 根因 | 修复 |
|---|---|---|
| **D-ITEM001-1** S 行末尾 CR+LF 未发出（行粘连/截断） | `debug_trace_flush_close()` 用 `UART_FLAG_TC` 排空：TC 是可清除标志，上一帧完成后可能保持置位 → 等待成为空操作，紧接着的 UART1 复位截断了仍在移位的尾字节；横幅因后面有较长间隔而幸免 | `debug_trace.c`：改为**先等 `TXE`**（确认最后一字节已进入移位器）**再等 `TXBUSY` 清零**（厂商 `UART_SendString()` 同款排空）后才复位/关闭 |
| **D-ITEM001-2** 横幅 `rst=` 恒为 `0000` | `main.c` 在 `RTC_Configuration()` **之后**才 `SYSCTRL_GetAllRstFlag()`，而 vendor `RTC_Init()` 会 `SYSCTRL_ClearRstFlag(SYSCTRL_RESETFLAG_ALL)` | `main.c`：在 `main()` 首条语句（任何初始化之前）锁存 `rst_flags`，横幅改用锁存值；锁存与使用均在 `#if SENSOR_DEBUG_UART` 内（不破坏宿主机 harness 链接） |

修复附带加强了实现侧自检（防止回归）：

- `test/mock_trace/` 新增 `UART_FLAG_TXBUSY` 并建模“移位中字节数”（`UART_SendData_8bit` 入队、TXBUSY 轮询逐字节移出）。
- `test/host_debug_trace_check.c` 新增两项检查：`drains with TXBUSY ... not TC` 与 `D-ITEM001-1 guard: UART1 is NOT reset while bytes are still shifting`（共 24 项）。
- **负对照**：把排空改回旧 `TC` 写法的副本编译运行 → `22 passed, 2 failed`、退出码 1（证明该回归拦截有效，且不是空验证）。

**真实目标复验归属**：本调用工具列表仍只有 `mdk_build`，无 `mdk_flash`/串口 → 修复后的 COM42 复验（横幅 `rst` 非零、每条 S 行以 CRLF 结束、字节数与期望一致）由嵌入式验证能力按 `evidence/test.md` §4/§5 方法复跑（脚本 `evidence/embedded_verify_uart_trace_item001.py` 可直接使用）。修复后的 Keil 产物 md5 `956dd6790bf11ff8cb584f584bc006d2`，已在反汇编中确认 `debug_trace_flush_close` 含 TXE + TXBUSY 两次轮询后再复位 UART1。

### 未执行 / 交接（如实记录，不当作通过）

- **真实目标观测未执行**：本调用工具列表仅含 `mdk_build`，无 `mdk_flash`/串口采集。TD-002 §4.4 T-L2-01..09 的下载-复位-COM42 轨迹采集（横幅、周期 S 行、采样间期 0 字节、二次下载验证 SWD 未被破坏）由嵌入式测试能力执行。
- **待实现能力（T2/T3/T4）**：`light.c` 仍为旧滞回判定且 ADC 全超时仍回退满量程（D-02）；`fw_core.c` 的 `sensor_decide_report` 仍为旧降温方向与 35.0℃ 特例排除（D-01）。因此本轮 S 行的 `D`（dark）与 `r`（report）反映的是**当前 as-built 判定**，而非 FD-002 rev 4.0 §6.3/§6.4 的目标语义；T-L2-04 的按新公式重算需在 T2/T3 落地后成立。
- **跨项观察 OBS-1（EV-006 登记，不在本项范围）**：真实目标上 GXHT40 温湿度测量每个周期均失败（轨迹 `q=0`/`t=0`/`h=0`，含上电 k=0 且发生在首次 UART 关闭之前）。本轮未修改 `gxht40.c`/`sf_i2c.c`，且 UART1 AF 配置仅写 `AFRL_f.AFR5/AFR6`（不碰 PA03/PA04）——属温湿度采集通路（驱动/器件/供电/上拉）问题，交接给该项负责能力与独立验证；本轮不据它改变本项边界。
- **测试资产依赖**：`host_debug_trace_check.c` 需显式 `-DSENSOR_DEBUG_UART=1`（宿主机默认 0）；TD-002 拟新增的 debug_trace harness 应采用同样方式，否则宿主机编译为空操作。

---

# 历史：上一轮工作流（FWI-002 rev 3.0，T1–T7 / 旧 12 项编号）

## 任务项 ITEM-001（T1：GXHT40 温湿度采集通路）

**任务**：每次采样触发后，驱动能探测 0x44/0x45 地址、发送 0xFD 高重复率测量命令、按 tMEAS 上限等待并读回 6 字节，温度字与湿度字分别通过 CRC-8（poly 0x31、init 0xFF）校验，输出带符号温度 x10（0.1℃，-400..1250）与湿度 x10（0.1%RH，截断 0..1000）；读请求 NACK、CRC 错或无器件时按上限有界重试，整次失败返回失败码且不修改传入的输出。

设计映射：FD-002 §2.2（GXHT40 电气/协议）、§3.1（模块划分）、§6.1/§6.2（CRC-8/换算）、§10（异常策略）、§11（实现约束）。FD-002 rev 3.0 §1.1 的偏差表（上报方向、光照滞回/满量程回退、待上报冻结）**仅涉及 T2/T4/T5**，T1 与 rev 2.0 行为一致，无变更项。

### 实际改动（本轮）

本轮核对确认：共享仓库当前提交（`85ba302`）已包含 T1 所需的全部实现（上一轮工作流实现，且 FD-002 rev 3.0 未对 T1 提出变更），故本轮**无需修改产品源码**；本轮完成的是按 rev 3.0 依据的核验与证据复现（含新授权的 Keil MDK 构建）。T1 实现由以下既有文件构成：

| 文件 | 角色 | 关键点 |
|---|---|---|
| `USER/inc/sensor_config.h` | 唯一配置点 | GXHT40 地址 0x44/0x45→0x88/0x89/0x8A/0x8B、命令 0xFD、等待 10 ms、读重试 5、重测 3、CRC-8 poly 0x31/init 0xFF、换算常量与量程 |
| `USER/inc/sf_i2c.h` / `USER/src/sf_i2c.c` | 总线原语 | `i2c_write_cmd`（START→addr+W→cmd→STOP，无寄存器地址）、`i2c_read_bytes`（START→addr+R→N 字节，末字节 NACK→STOP）；ACK 失败返回 `SF_I2C_TIMEOUT` |
| `USER/inc/gxht40.h` / `USER/src/gxht40.c` | 驱动 | 地址探测+缓存、0xFD、等待 tMEAS、读 6B、温度字/湿度字分别 CRC-8、整数 x10 换算与量程判定、读 NACK 有界重读、CRC/IO 有界重测、失败不写输出 |
| `USER/inc/fw_core.h` / `USER/src/fw_core.c` | 纯逻辑（宿主机可测） | `fw_crc8_gxht`、`gxht40_temp_raw_to_x10`、`gxht40_hum_raw_to_x10`（截断 0..1000）、`gxht40_temp_x10_valid`、`gxht40_raw_to_x10` |

### 预期行为

1. 地址探测：先试缓存 7bit 地址；未缓存或地址失配时依次试 0x44、0x45，成功后缓存，避免每周期重复扫描。
2. 命令：周期路径只发 0xFD（高重复率）；不发软复位 0x94 与加热器命令。
3. 时序：0xFD STOP 后等待 `GXHT40_MEASURE_WAIT_MS`=10 ms（覆盖 tMEAS.H max 8.3 ms）。
4. 读取：一次连续读 6 字节，顺序 `T_MSB,T_LSB,T_CRC,RH_MSB,RH_LSB,RH_CRC`；前 5 字节主机 ACK、末字节 NACK。
5. 校验：`buf[0..1]` 与 `buf[3..4]` 分别以 CRC-8(0x31, init 0xFF) 与 `buf[2]`/`buf[5]` 比对；任一不符即丢整帧重测。
6. 换算：`temp_x10 = -450 + round(1750*S_T/65536)`；`hum_x10 = -60 + round(1250*S_RH/65536)` 截断到 0..1000；温度超出 -40.0..125.0 ℃ 判无效。
7. 有界失败：读 NACK ≤ `GXHT40_READ_RETRY`(5) 次、间隔 1 ms；整帧重测 ≤ `GXHT40_MEAS_RETRY`(3) 次；用尽返回 `GXHT40_ERR_NO_DEVICE` / `_IO` / `_CRC` / `_RANGE` / `_PARAM`。
8. 失败不写输出：仅完全成功时写 `*temp_x10`/`*hum_x10`。

### 验证（本轮实际执行）

命令与原始结果见 `evidence/driver_test.md`（T1 节）与 `evidence/build.md`（T1 节）：

- `test/host_sf_i2c_bus_check.c`：**15 passed, 0 failed**（exit 0）
- `test/host_gxht40_check.c`：**27 passed, 0 failed**（exit 0）
- `test/build_test.sh`（L0 综合 + 采样流程）：**38 + 24 passed, 0 failed**（exit 0）
- `gcc/build.sh` 交叉编译：0 错误，**FLASH 32,208 B / RAM 1,712 B**，生成 elf/hex/bin；T1 翻译单元（gxht40/sf_i2c/fw_core）无浮点引用
- `mdk_build {action:build}`（Keil MDK / ARMCLANG V6.24）：**0 Error / 0 Warning**，产物 `MDK/output/exe/Project.axf`

**未执行（如实记录，不视为通过）**：TD-002 §2.2 要求的 `[实板]` 真实 I²C 抓包（T-L2-01/02/03）、参考温湿度计对照（T-L2-04/05）、故障注入（T-L2-06/07/08/09）——项目设备工具已禁用，属嵌入式测试能力在设备解禁后执行。

---

# 任务项状态（本轮）

| 队列项 | 任务清单 | 状态 |
|---|---|---|
| ITEM-001 | T1 GXHT40 温湿度采集通路 | 本轮完成（宿主机 + 交叉编译证据；实板观测待设备解禁） |
| ITEM-002 | T2 光照采集与无光判据 | 未在本轮处理 |
| ITEM-003 | T3 3 分钟采样节拍与采样顺序 | 未在本轮处理 |
| ITEM-004 | T4 条件上报判定 | 未在本轮处理 |
| ITEM-005 | T5 条件上报发送与空口兼容 | 未在本轮处理 |
| ITEM-006 | T6 旧通路退役与引脚所有权 | 未在本轮处理 |
| ITEM-007 | T7 网关与手机链路兼容核对 | 未在本轮处理 |

---

# 历史记录（上一轮工作流，旧 12 项编号；仅供追溯，勿与本轮 ITEM 编号混用）

# 任务项 ITEM-001（已完成，独立验证 TEST_PASS）

**ITEM-001**：新增 `sensor_config.h` 作为唯一配置点：定义 PB04/PB05 引脚与 ADC 通道、GXHT40 地址/命令/重试/等待参数、光照明暗阈值与滞回、3 分钟采样计数值、0.9℃ 下降与 35.0℃ 超温上报常量；加入该头文件后工程仍可交叉编译。

设计映射：FD-002 §3.1（新增文件）、§11.1/§11.4（单一配置点、引脚所有权）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/sensor_config.h` | 新增 | 传感器固件唯一配置点（见下表） |
| `.../USER/inc/main.h` | 修改 | 增加 `#include "sensor_config.h"`，使工程翻译单元实际解析该头文件（证明「加入后仍可交叉编译」非空验证） |

### `sensor_config.h` 定义的配置（只定义，未接入业务逻辑）

| 组 | 宏 | 值 | 来源 |
|---|---|---|---|
| 节拍 | `SENSOR_RTC_TICK_PERIOD_MIN` / `SENSOR_SAMPLE_TICKS` | 1 / 3（=3 分钟） | readme 修改点 3；FD-002 §5.2 |
| GXHT40 地址 | `GXHT40_ADDR_7BIT_A/B`、`GXHT40_ADDR_WRITE_A/READ_A/WRITE_B/READ_B` | 0x44/0x45 → 0x88/0x89/0x8A/0x8B | gxht40.pdf；FD-002 §2.2 |
| GXHT40 命令 | `GXHT40_CMD_MEASURE_HIGH_REP` = 0xFD（另备 0xF6/0xE0；软复位 0x94 标注周期路径禁用） | — | 手册表 9；FD-002 §11.10 |
| GXHT40 时序/重试 | `GXHT40_MEASURE_WAIT_MS`=10、`GXHT40_READ_RETRY`=5（间隔 1 ms）、`GXHT40_MEAS_RETRY`=3、`GXHT40_POWER_ON_WAIT_MS`=2 | — | 手册 tMEAS.H max 8.3 ms；FD-002 §10 |
| GXHT40 校验/换算 | `GXHT40_CRC8_POLY`=0x31、`GXHT40_CRC8_INIT`=0xFF、换算分子/偏移/取整/量程常量 | — | 手册 §7.3/§7.5；FD-002 §6.1/§6.2 |
| 光照引脚/ADC | `LIGHT_POWER_PORT/PIN`=PB05、`LIGHT_ADC_PORT/PIN`=PB04、`LIGHT_ADC_INPUT_CHANNEL`=`ADC_InputCH11`、`LIGHT_ADC_FULL_SCALE`=4095、`LIGHT_ADC_CLK_DIV`=`ADC_Clk_Div8`、`LIGHT_ADC_SAMPLE_TIME`=`ADC_SampTime390Clk`、`LIGHT_ADC_SAMPLES`=8 | — | 网表；`cw32l010_adc.h`（PB04=AIN11）；FD-002 §2.3 |
| 光照阈值/滞回 | `LIGHT_DARK_ENTER`=350、`LIGHT_DARK_EXIT`=250、`LIGHT_DARK_HYSTERESIS_STEP`=100、`LIGHT_DARK_CALIBRATED`=0 | — | FD-002 §6.3；FWR-OPEN-1（默认值待实板标定回填） |
| 上报判定 | `SENSOR_REPORT_DROP_X10`=9（0.9℃）、`SENSOR_REPORT_HIGH_X10`=350（35.0℃） | — | readme 修改点 4；IC-002 §2；FD-002 §6.4 |
| 433 链路 | `SENSOR_RF_FRAME_LEN`=10、`SENSOR_RF_TX_TIMEOUT_MS`=200、`SENSOR_RF_TX_RETRY`=3 | — | FD-002 §8.1/§10 |
| 调试钩子 | `SENSOR_TEST_TRACE` 默认 0（可 `-D` 覆盖） | — | test_design.md §2 观测钩子请求 |

### 预期行为

1. 上述常量只在 `sensor_config.h` 定义一次；其它文件只引用宏（FD-002 §11.1）。
2. 头文件自包含，可被后续 `gxht40.c`/`light.c`/`measure.c` 直接包含。
3. 加入头文件后，工程交叉编译与链接结果与改动前**逐字节一致**（宏不产生代码）。
4. `SENSOR_TEST_TRACE` 默认关闭，量产构建行为不变。

### 验证

交叉编译 0 错误、FLASH/RAM 与基线逐字节一致（33,896 B / 1,960 B）；配置头 43 条编译期断言通过；宿主机 L0 回归 56/56。详见 `evidence/build.md`，独立验证结论见 `evidence/test.md`（embedded_verification，TEST_PASS）。

---

# 任务项 ITEM-002（已完成，独立验证 TEST_PASS）

**ITEM-002**：扩展 sf_i2c 总线原语：提供不带寄存器地址的命令写（START→地址+W→命令→STOP）与连续读（START→地址+R→N 字节→NACK→STOP），并把从机 ACK 失败作为可判断的返回值；既有 i2c 函数语义与调用方式不变。

设计映射：FD-002 §3.2（`sf_i2c` 增加两个总线原语）、§2.2（GXHT40 无 clock stretching、数据未就绪返回 NACK）、§10（异常策略）。测试映射：TD-002 T-L2-01/T-L2-02/T-L2-03/T-L2-07 所需的软件侧时序。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/inc/sf_i2c.h` | 修改（+8 行） | 声明 `i2c_write_cmd()` 与 `i2c_read_bytes()`，注释说明序列与返回值语义 |
| `.../USER/src/sf_i2c.c` | 修改（+62 行，**0 行删除**） | 实现两个原语；既有函数逐字节未改动 |

### `i2c_write_cmd(dev, slave_addr, cmd)` → `sf_i2c_err`

- 序列：`START → I2C_WRITE(slave_addr) → cmd → STOP`（**不发送寄存器地址**，适配 GXHT40 命令 0xFD）。
- 地址字节或命令字节未获 ACK → 返回 `SF_I2C_TIMEOUT`；`i2c_wait_ack()` 超时路径已发 STOP 释放总线，故不再重复 STOP。
- 成功返回 `SF_I2C_SUCCESS`。

### `i2c_read_bytes(dev, slave_addr, buf, length)` → `sf_i2c_err`

- 序列：`START → I2C_READ(slave_addr) → length 字节（前 length-1 字节主机 ACK，末字节主机 NACK）→ STOP`。
- 读地址字节未获 ACK（GXHT40 转换未完成时返回 NACK）→ 返回 `SF_I2C_TIMEOUT`，总线已释放。
- `length == 0` 或空指针 → 直接返回 `SF_I2C_SUCCESS`，**不产生任何总线动作**。
- 成功返回 `SF_I2C_SUCCESS`。

### 预期行为（可检验）

1. 两个原语都不发送寄存器地址，字节序为标准 I²C 地址头 + 数据。
2. 从机 ACK 结果作为返回值可判断：`SF_I2C_SUCCESS` / `SF_I2C_TIMEOUT`。
3. 连续读末字节发 NACK 后 STOP（与 TD-002 T-L2-03 一致）。
4. 既有 `i2c_*` 函数源码 0 行删除，语义与调用方式不变。
5. 工程交叉编译通过，新增符号进入镜像，`-Wall -Wextra` 无新增告警。

**本项不包含**：GXHT40 协议层（地址探测、0xFD 命令调用、CRC、换算）属 ITEM-003；光照采样属 ITEM-004；板上总线波形验证属嵌入式测试（TD-002 T-L2）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误**；`FLASH 34,060 B (51.97%)`（较 ITEM-001 +164 B）、`RAM 1,960 B`（不变）。无新增告警指向新代码（既有 `i2c_write_multi_byte`/`_16bit` 的 `err may be used uninitialized` 告警为改动前既有，未触碰）。
- 符号：`arm-none-eabi-nm` 显示 `i2c_write_cmd`、`i2c_read_bytes` 均为全局符号并已进入 `sensor_fw.elf`；既有符号地址集合未消失。
- 变更范围：`git diff --numstat` 为 `62 insertions / 0 deletions`（sf_i2c.c）、`8 insertions`（sf_i2c.h），既有函数未被改动。
- 宿主机 mock 总线自检：15/15 通过，覆盖命令写序列、连续读 6 字节（5×ACK + 末字节 NACK）、从机不响应返回 `SF_I2C_TIMEOUT` 且释放总线、`length==0` 无总线动作。详见 `evidence/driver_test.md`。
- 宿主机 L0 既有回归：`test/build_test.sh` → 56 passed / 0 failed（未触碰被测算例）。

## 交接与依赖

- ITEM-003（GXHT40 驱动）将直接调用本原语；`i2c_read_bytes` 的 NACK 返回即"转换未完成需等待后重读"的判据（FD-002 §10）。
- 板上 I²C 电气/时序（地址字节、tMEAS 间隔、6 字节顺序、总线卡死恢复）仍须按 TD-002 T-L2-01/02/03/07 由嵌入式测试在真实硬件上验证；本项只提供软件侧序列与返回值。

---

# 任务项 ITEM-003（已完成，独立验证 TEST_PASS）

**ITEM-003**：实现 GXHT40 驱动 `gxht40.c/.h`：上电探测 0x44/0x45 地址（8 位写 0x88/0x8A、读 0x89/0x8B），发送 0xFD 高重复率测量命令，按 tMEAS 上限等待后读回 6 字节，用 CRC-8（poly 0x31、init 0xFF、无反转、xorout 0x00）分别校验温度字与湿度字，输出 `temp_x10`（int16，−400..1250）与 `hum_x10`（uint16，0..1000，越界截断）；失败返回失败码且不修改输出。

设计映射：FD-002 §3.1（新增 `gxht40.c/.h`）、§6.1/§6.2（CRC-8 与整数换算）、§10（异常策略）、§11.3/§11.9/§11.10（无 clock stretching、不用加热器、周期路径不软复位）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/inc/gxht40.h` | 新增 | `gxht40_status_t` 结果码、`gxht40_init()`、`gxht40_measure()`、`gxht40_detected_addr7()` |
| `.../USER/src/gxht40.c` | 新增 | 地址探测与缓存、`0xFD` 命令、tMEAS 等待、6 字节读取（读 NACK 有界重读）、双字 CRC-8、整数换算与量程判定 |
| `.../test/host_gxht40_check.c` | 新增（测试载体） | 宿主机 mock 总线自检，27 项 |

无既有文件被修改。

### 行为要点

1. **地址探测与缓存**：首次测量依次试 `0x88`→`0x8A`（地址字节未被 ACK 即认为该地址无器件）；命中后缓存 7bit 地址，后续直接使用；缓存地址失效时重新探测。
2. **时序**：命令写（START→地址+W→`0xFD`→STOP）→ `delay_ms(GXHT40_MEASURE_WAIT_MS=10ms)`（≥ tMEAS.H max 8.3 ms）→ 连续读 6 字节（START→地址+R→前 5 字节主机 ACK + 末字节 NACK→STOP）。
3. **读 NACK = 转换未完成**：等待 1 ms 后重读，最多 `GXHT40_READ_RETRY=5` 次；不重发命令。
4. **双字 CRC**：`crc8(buf[0..1])==buf[2]` 且 `crc8(buf[3..4])==buf[5]`（poly 0x31、init 0xFF、无反转、xorout 0x00；手册参考 `CRC(0xBEEF)=0x92`）。CRC 错丢弃整帧并按 `GXHT40_MEAS_RETRY=3` 重测。
5. **换算（整数，无浮点）**：`temp_x10 = -450 + round(1750*S_T/65536)`；`hum_x10 = -60 + round(1250*S_RH/65536)` 后截断 0..1000。
6. **量程**：温度结果超出 −400..1250（即 −40.0..125.0 ℃）视为无效测量。
7. **失败不改输出**：仅在完全成功时写入 `*temp_x10`/`*hum_x10`；否则返回 `GXHT40_ERR_PARAM/NO_DEVICE/IO/CRC/RANGE` 之一，输出保持调用前值。
8. **命令白名单**：周期路径只发 `0xFD`；不发 `0x94` 软复位、不发加热器命令。

**本项不包含**：把驱动接入采样流程（ITEM-006）、光照采样（ITEM-004）、纯逻辑抽取到 `fw_core.c`（ITEM-005）、MDK/IAR 工程源文件列表注册（ITEM-011）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误，无告警指向 `gxht40.c`**；工程 FLASH/RAM 与 ITEM-002 相同（34,060 / 1,960 B），因为 `gxht40.o` 尚未被引用（链接器未拉入，接线属 ITEM-006）；`obj/gxht40.o` = text 460 B / bss 5 B。
- 驱动级自检：`test/host_gxht40_check.c` → **27 passed / 0 failed**，覆盖手册 CRC 参考向量、常温/负温换算、地址探测与缓存、读 NACK 重读、CRC 错重测上限、无器件、超范围、失败不改输出、命令白名单。详见 `evidence/driver_test.md`。
- 宿主机 L0 回归：`test/build_test.sh` → 56 passed / 0 failed。

## 交接与依赖

- ITEM-004（光照）与 ITEM-006（采样流程）将调用 `gxht40_measure()`；接线时需先 `bsp_i2c_init()` 并把 `i2c_obj_find("i2c0")` 结果交给 `gxht40_init()`（与现有 `measure.c` 用法一致）。
- ITEM-005 将把 CRC-8 与 x10 换算抽为 `fw_core.c` 的纯函数供宿主机直测；届时 `gxht40.c` 应改为调用该纯函数（避免两份实现）。
- 真实 tMEAS 下界、地址变体实物确认、电气与 CRC 实读仍属 TD-002 T-L2-01/02/03/06/07，由嵌入式测试在真实硬件验证。

---

# 任务项 ITEM-004（已完成，独立验证 TEST_PASS）

**ITEM-004**：实现光照通路 `light.c/.h`：采样时 PB05 输出高、稳定延时后用 PB04/AIN11 多次取样求均值，按配置阈值与滞回输出 DARK/LIT（无光=读数≥进入阈值），采样结束 PB05 置低。

设计映射：FD-002 §2.3（分压拓扑与极性）、§3.1（新增 `light.c/.h`）、§6.3（滞回判定）、§10（光敏失效处理）、§11.4/§11.5（引脚所有权、空闲电平）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/inc/light.h` | 新增 | `light_init()`、`light_reset_state()`、`light_code_is_dark(code, prev_dark)`、`light_sample()`、`light_last_code()` |
| `.../USER/src/light.c` | 新增 | PB05 推挽输出(初始低) + PB04 模拟输入(AIN11) + ADC 单次转换配置；采样序列与滞回判定 |
| `.../USER/inc/sensor_config.h` | 修改（+2 行） | 新增 `LIGHT_ADC_EOC_GUARD`（ADC EOC 轮询上限，防转换挂死）；其余配置不变 |
| `.../test/host_light_check.c` | 新增（测试载体） | 宿主机滞回判定自检，17 项 |

### 行为要点

1. **引脚所有权**（FD-002 §11.4）：PB04 仅在本模块被配置为模拟输入（`GPIO_Init(..., GPIO_MODE_ANALOG)` 置 ANALOG 位，等价于 `PB04_ANALOG_ENABLE()`）；PB05 仅在本模块被配置为推挽输出。
2. **采样序列**：`PB05 = 高` → `delay_ms(LIGHT_SETTLE_MS=100ms)`（分压 RC + 光敏器件响应）→ `ADC_Enable()` → `LIGHT_ADC_SAMPLES=8` 次单次转换（`ADC_SoftwareStartConvCmd` + 轮询 EOC）→ `ADC_Disable()` → `PB05 = 低`（`LIGHT_IDLE_POWER_OFF`）。
3. **ADC 配置**：`ADC_Clk_Div8`（ADCCLK = PCLK/8 = 1 MHz）、`ADC_SampTime390Clk`（390 µs 采样保持，适配最高 5 MΩ 源阻抗）、通道 `ADC_InputCH11`（PB04）。
4. **均值**：对成功的转换取算术平均（`sum / ok`）；`light_last_code()` 保留最近均值供调试跟踪。
5. **滞回判定**（FD-002 §6.3）：已明时 `code >= LIGHT_DARK_ENTER(350)` 转 DARK；已暗时 `code <= LIGHT_DARK_EXIT(250)` 转 LIT；滞回带内保持原状态。语义：**无光 = code ≥ 进入阈值**（暗时节点电压高）。
6. **异常不阻塞**：单次 ADC 转换超时（`LIGHT_ADC_EOC_GUARD` 用尽）被丢弃；全部超时按满量程（器件开路 = 无光）处理，倾向上报且不死等。
7. **低功耗**：非采样期 PB05 = 低（分压无电流）、ADC 使能位关闭。

**本项不包含**：把光照接入采样流程（ITEM-006）、纯逻辑抽取到 `fw_core.c`（ITEM-005）、旧 hall/OPTCFG 对 PB04/PB05 的退役（ITEM-009）、MDK/IAR 工程源文件列表注册（ITEM-011）、光照阈值实板标定（FWR-OPEN-1 / TD-002 T-L3-03）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误**；总告警 29 条（与基线同数，**无一条指向 `light.c`**）；`obj/light.o` = text 364 B / bss 4 B。工程 FLASH/RAM 与 ITEM-003 相同（34,060 / 1,960 B），因 `light.o` 尚未被引用（链接器未拉入，接线属 ITEM-006）。
- 纯逻辑自检：`test/host_light_check.c` → **17 passed / 0 failed**，覆盖阈值/滞回方向与端点（0/349/350/351/4095 与 0/249/250/251/351/4095）与带内不抖动。详见 `evidence/driver_test.md`。
- 宿主机 L0 回归：`test/build_test.sh` → 56 passed / 0 failed。

## 交接与依赖

- ITEM-005 将把 `light_code_is_dark` 抽为 `fw_core.c` 纯函数供宿主机直测；届时 `light.c` 应改为调用该纯函数（避免两份实现），并从 `light.c` 移除同名定义。
- ITEM-006 接线时先 `light_init()`，采样周期内调用 `light_sample()`；需与 ITEM-009 配合：旧 `hall.c`/`optcfg.c` 目前仍会配置 PB04/PB05，必须在接线前退役，否则引脚所有权冲突。
- 光照阈值 `LIGHT_DARK_ENTER/EXIT` 仍为未标定默认值（`LIGHT_DARK_CALIBRATED=0`），按 TD-002 T-L3-03 实板标定后回填。

---

# 任务项 ITEM-005（已完成，独立验证 TEST_PASS）

**ITEM-005**：在 `fw_core.c` 中实现并保持不依赖 MCU 寄存器的纯逻辑：`fw_crc8_gxht`（参考向量 CRC(0xBEEF)=0x92）、GXHT40 原始字到 x10 的整数换算（含负温与 0-100%RH 截断）、`light_code_is_dark` 滞回判定、`sensor_decide_report` 判定 `((prev-cur)>9 且 DARK) 或 (cur>350)`（无前值时下降分支恒假）。

设计映射：FD-002 §3.2（fw_core 纯逻辑）、§6.1/§6.2/§6.3/§6.4（算法）、§11.1（单一配置点）。测试映射：TD-002 T-L0-01…T-L0-05。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/inc/fw_core.h` | 修改 | 新增 7 个纯函数声明（CRC-8、温度/湿度换算、量程判定、组合换算、光照滞回、上报判定） |
| `.../USER/src/fw_core.c` | 修改 | 实现上述纯函数；仅依赖 `sensor_config.h` 的纯数值段（定义 `SENSOR_CONFIG_NO_MCU`，不引 MCU 头） |
| `.../USER/inc/sensor_config.h` | 修改 | 拆为「纯数值段」+ `#ifndef SENSOR_CONFIG_NO_MCU` 的「MCU 引脚/外设段」；数值不变，仅重排 |
| `.../USER/src/gxht40.c` | 修改 | 删除内置 `gxht40_crc8`/`gxht40_conv_*`，改用 `fw_crc8_gxht`/`gxht40_raw_to_x10`（消除双份实现，落实 ITEM-003 观察项 F1） |
| `.../USER/inc/light.h`、`.../USER/src/light.c` | 修改 | `light_code_is_dark` 移入 `fw_core.c`；`light.c` 改为包含 `fw_core.h` 调用（消除双份实现） |
| `.../test/host_fw_core_pure_check.c` | 新增（测试载体） | 宿主机纯逻辑自检 36 项 |
| `.../test/host_gxht40_check.c`、`host_light_check.c` | 修改 | 链接新增 `fw_core.c`（驱动/光照已改为调用纯函数） |

### 接口（fw_core.h）

```c
uint8_t  fw_crc8_gxht(const uint8_t *p, uint16_t n);          /* poly 0x31, init 0xFF, 空长度 0xFF */
int16_t  gxht40_temp_raw_to_x10(uint16_t raw);                 /* -450 + round(1750*S/65536) */
uint16_t gxht40_hum_raw_to_x10(uint16_t raw);                  /* -60 + round(1250*S/65536), 截断 0..1000 */
bool     gxht40_temp_x10_valid(int16_t temp_x10);              /* -400..1250 */
bool     gxht40_raw_to_x10(uint16_t rt, uint16_t rh, int16_t *t, uint16_t *h);  /* 有效才写输出 */
bool     light_code_is_dark(uint16_t code, bool prev_dark);    /* ≥ENTER 转暗, ≤EXIT 转明 */
bool     sensor_decide_report(int16_t prev, bool have_prev, int16_t cur, bool dark);
```

### 行为要点

1. **无 MCU 依赖**：`fw_core.c` 只包含 `fw_core.h`/`params.h` 与 `sensor_config.h`（纯数值段），因此 `test/build_test.sh` 的 `gcc -I../USER/inc` 无需追加 MCU 头路径仍可编译（已回归 56/56）。
2. **上报判定**：`cur > 350` 优先返回 true；`cur == 350` 返回 false（IC-002 §2 的 `T<35` 与 `T>35` 在恰好 35.0 ℃ 均不成立）；无前值或非 DARK 返回 false；否则 `(prev-cur) > 9`。
3. **温度有效域**：`gxht40_raw_to_x10` 在温度超 -40.0..125.0 ℃ 时返回 false 且不写输出；`gxht40.c` 据此返回 `GXHT40_ERR_RANGE`（行为与 ITEM-003 一致，重测逻辑不变）。
4. **单一配置点保持**：阈值仍只在 `sensor_config.h` 定义；新增的 `SENSOR_CONFIG_NO_MCU` 只控制是否引入 MCU 头。

**本项不包含**：退役 OPTCFG/params/history 纯逻辑与其测试用例（ITEM-009/ITEM-010）、采样流程接线（ITEM-006）、工程文件注册（ITEM-011）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误、29 条告警（与基线同数）**；FLASH 34,352 B（较 ITEM-004 **+292 B**：新增纯函数已随 `fw_core.o` 入镜像）、RAM 1,960 B（不变）。
- 纯逻辑自检：`test/host_fw_core_pure_check.c` → **36 passed / 0 failed**，逐条覆盖 TD-002 T-L0-01…T-L0-05（含 T-L0-02 的 S=0/0xFFFF 无效域、T-L0-03 的 0..1000 截断、T-L0-05 的 35.0 ℃ 边界）。详见 `evidence/driver_test.md`。
- 回归自检：`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15（后两者已改为链接 `fw_core.c`）。
- 既有宿主机 L0 测试：`test/build_test.sh` → 56 passed / 0 failed（未改动被测算例；退役与新增用例属 ITEM-010）。

## 交接与依赖

- **T-L0-05 用例数值与规则不一致（需测试侧确认）**：TD-002 第②行写作 `prev=300,cur=290`（下降 10 = 1.0 ℃）期望 **false**，但同一行又要求「与 FD-002 §6.4 逐条一致」；按任务书/FD-002/IC-002 的 `(prev-cur)>9`，下降 10 应触发。实现按 `>9`（下降 9 不触发、下降 10 触发），即「恰好 0.9 ℃」对应 `cur=291`；已在自检中同时固定 9/10/11 三个边界。请测试侧在 T-L0-05 中校正该行数值。
- ITEM-006 采样流程将调用 `sensor_decide_report()`；`prev_temp_x10` 由测量成功后更新，失败不更新（ITEM-006 实现）。
- ITEM-009/010 将移除 `fw_core.c` 中的 OPTCFG/params 纯逻辑与 `host_sensor_core_test.c` 对应用例，并把本项纯逻辑用例并入 `host_sensor_core_test.c`/`build_test.sh`。
- **跨角色交接（embedded_verification）**：本项把 `light_code_is_dark` 移入 `fw_core.c`、并让 `gxht40.c`/`light.c` 依赖 `fw_core.c`。因此 `test/host_light_verify_ev.c` 与 `host_gxht40_verify_ev.c`（嵌入式测试的 ITEM-004/ITEM-003 独立 harness）需在链接目标中补上 `USER/src/fw_core.c`（`host_light_verify_ev.c` 还需包含 `fw_core.h`）；本能力未修改这两个跨角色验证文件，仅登记交接。

---

# 任务项 ITEM-006（已完成，独立验证 TEST_PASS）

**ITEM-006**：改造 `measure.c` 采样流程：每个采样周期先取光照再取温湿度；GXHT40 读 NACK 与 CRC 错按配置上限有界重试；整周期失败时不上报、不更新前一有效温度、不构造 0 值；成功后更新前一有效温度与最近有效湿度。

设计映射：FD-002 §4（数据流）、§6.4（上报判定）、§10（异常策略）、§3.2（纯逻辑在 fw_core）。测试映射：TD-002 T-L5/T-L6（实板/集成）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/src/measure.c` | 重写采样部分 | 删除 AHT21 步进状态机与 `alarm_triggered()`；新增 `measure_sample()`（`light_sample()` → `gxht40_measure()`）与新的 `temperature_process()`；保留软 I2C 端口与 `send_data_to_gateway()`/`go_to_sleep()` 框架 |
| `.../USER/inc/measure.h` | 修改 | 移除已废弃的 `samples_since_report`/`first_sample_reported` 声明 |
| `.../USER/src/fw_core.c` | 微调 | `SENSOR_CONFIG_NO_MCU` 定义加 `#ifndef` 守卫（避免命令行 `-D` 时重定义告警；不改变行为） |
| `.../test/host_measure_flow_check.c`、`test/mock_measure_mcu/` | 新增（测试载体） | 宿主机 mock MCU 影子头 + 可控 GXHT40/光照桩，编译**真实 measure.c**，15 项 |

### 行为要点

1. **顺序**：`measure_sample()` 先 `light_sample()`（PB05 供电→稳定→ADC 均值→PB05 低），再 `gxht40_measure()`。
2. **有界重试**：读 NACK（`GXHT40_READ_RETRY`×`GXHT40_READ_RETRY_DELAY_MS`）与 CRC 错（`GXHT40_MEAS_RETRY`）均在 `gxht40_measure()` 内按 `sensor_config.h` 上限完成；`measure.c` 只调用一次并按返回码分支。
3. **失败（`GXHT40_ERR_*`）**：`measure_sample()` 在写 `tempvalue`/`huminityvalue` **之前** 返回 false；`temperature_process()` 失败分支不置 `report_req`、不推进 `s_prev_temp_x10`/`s_have_prev`/`s_last_hum_x10`，也不写 0 值（`tempvalue`/`huminityvalue` 保持上次有效值）。
4. **成功**：先用**更新前**的 `prev` 调 `sensor_decide_report()` 置 `report_req`，再推进 `s_prev_temp_x10 = tempvalue`、`s_have_prev = true`、`s_last_hum_x10 = huminityvalue`。
5. **待上报不丢**：无上报条件的成功周期不清除已挂起的 `report_req`（发送失败重试语义保留）。
6. **初始化**：首次采样惰性执行 `bsp_i2c_init()` + `gxht40_init(temp_ptr)` + `light_init()`（`s_sensor_ready` 一次性）。
7. **发送路径**：帧布局/加密不变；仅把硬编码的 200 ms/3 次改为引用 `SENSOR_RF_TX_TIMEOUT_MS`/`SENSOR_RF_TX_RETRY`，并移除已废弃的 `samples_since_report` 归零。完整对齐属 ITEM-008。

**本项不包含**：3 分钟节拍（ITEM-007）、发送路径完整对齐（ITEM-008）、旧 `hall/OPTCFG/params/history` 退役（ITEM-009）、宿主机测试合并（ITEM-010）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误、28 条告警**（基线 29；旧 AHT21 序列移除后少 1 条，无新增告警）；FLASH **35,884 B**（较 ITEM-005 +1,532 B，新驱动/光照模块被引用入镜像；`history.o` 不再被引用）、RAM **1,896 B**（-64 B）。`arm-none-eabi-nm` 确认 `gxht40_measure`/`light_sample`/`sensor_decide_report` 已入镜像。
- 采样流程自检：`test/host_measure_flow_check.c` → **15 passed / 0 failed**，覆盖顺序、9/10 下降边界、需 DARK、待上报保持、失败不改前值/不置位/不写 0、失败后前值判定、超温分支、`sample_flag==0` 不采样。详见 `evidence/driver_test.md`。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15；既有 `test/build_test.sh` 56/56。

## 交接与依赖

- **报告判定位置**：本项已在采样成功分支调用 `sensor_decide_report()` 并置 `report_req`；ITEM-008 据此完成/校对发送路径（`encode_frame10` + `app_um2005C_send_data_timeout` + 成功清除/失败重试）。
- **引脚所有权冲突仍未消除**：`main.c` 仍调用 `optcfg_init()`/`hall_init()`（PB05 数字输入/PB04 EXTI/PB06），可能在运行中重新配置光照引脚；`light_init()` 在首次采样时会重新配置，但完整退役属 ITEM-009。在 ITEM-009 完成前，TD-002 的 T-L3/T-L5/T-L6 实板结论不具备有效前提。
- 宿主机 mock（`test/mock_measure_mcu/`）只影子 `cw32l010_gpio/sysctrl/uart`；`gxht40_measure`/`light_sample` 以桩替换，未在本 harness 中执行其真实实现（其行为已由 ITEM-003/004 harness 单独验证）。

---

# 任务项 ITEM-007（已完成，独立验证 TEST_PASS）

**ITEM-007**：将采样节拍改为 3 分钟：RTC 1 分钟中断累计到配置的 3 次才置采样标志，一个采样周期内只执行一次测量，并移除原小时上报与首样本强制上报逻辑。

设计映射：readme 修改点 3；FD-002 §5.1/§5.2（采样周期与 RTC 节拍）；RTA-002 §1.2（T_TICK）。测试映射：TD-002 T-L4-01（板级长时基）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/src/main.c` | 修改 | `RTC_IRQHandlerCallBack()` 阈值 `rtc_set_cnt >= 1` → `>= SENSOR_SAMPLE_TICKS`（=3）；删除未使用的 `temp_cnt`/`work_period_flag`；更新陈旧注释（原“每拍一次采样/小时上报由 samples_since_report 计数”）与 `RTC_SetInterval` 的“1s”注释 |

`measure.c` 的小时/首样本上报逻辑已在 ITEM-006 移除（`first_sample_reported`/`samples_since_report` 均无引用）。本项未再改 `measure.c`。

### 行为要点

1. **3 分钟节拍**：RTC 仍为 `RTC_INTERVAL_EVERY_1M`（1 分钟/拍）；`RTC_IRQHandlerCallBack()` 每拍 `rtc_set_cnt++`，达到 `SENSOR_SAMPLE_TICKS`（`sensor_config.h` = 3）时置 `sample_flag = 1` 并清零计数。
2. **一个周期一次测量**：`temperature_process()` 在 `sample_flag != 0` 时执行一次 `measure_sample()`，随后 `sample_flag = 0`；下一次置位需再累计 3 拍。
3. **无强制上报**：上报完全由 `sensor_decide_report` 条件门控；无小时上报、无首样本强制上报。
4. **上电即采一次**：`sample_flag` 初值为 1，上电后立即执行一个采样周期；因无前值，首样本不会触发下降分支（与 FWR-108 一致），超温分支仍可上报。

**本项不包含**：发送路径对齐（ITEM-008）、旧 `hall/OPTCFG/params/history` 退役（ITEM-009）、板级 3 分钟长时基测量（TD-002 T-L4-01，需嵌入式测试）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误、28 条告警**（与 ITEM-006 同数；`main.c` 仅有改动前既有的 4 条：未用参数 `handle`/`file`/`line`、`main` 返回类型）；FLASH 35,884 B、RAM 1,896 B（不变，节拍常量不产生代码）。
- 源码结构确定性检查（命令与输出见 `evidence/driver_test.md`）：`main.c` 阈值 = `SENSOR_SAMPLE_TICKS`；全工程无 `rtc_set_cnt >= 1`/`first_sample_reported`/`samples_since_report` 残留；`measure.c` 消费标志（`sample_flag = 0`）；配置值 1 min × 3 = 3 min。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_measure_flow_check.c` 15/15、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`test/build_test.sh` 56/56。

## 交接与依赖

- **板级节拍**：3 分钟实际间隔（含 LSI 容差与离散度）需 TD-002 T-L4-01 用长时基/电流波形在真实板记录；本能力无硬件，未做实测。
- **引脚所有权冲突仍未消除**：`main.c` 仍调用 `optcfg_init()`/`hall_init()`（PB05/PB04/PB06），完整退役属 ITEM-009。
- 观测钩子 `SENSOR_TEST_TRACE` 仍未接入打印点（TD-002 §2 交接项）；可随后续采样/上报任务或测试需求接入。

---

# 任务项 ITEM-008（已完成，独立验证 TEST_PASS）

**ITEM-008**：实现条件上报：满足 `sensor_decide_report` 时调用 `encode_frame10(uid, temp_x10, hum_x10)` 组 10 字节帧并经 `app_um2005C_send_data_timeout` 有界发送，帧布局、字节序与 Feistel 加密保持不变，仅在发送成功后清除待上报状态，发送失败按上限重试后放弃本轮。

设计映射：readme 修改点 4；IC-002 §2/§3；FD-002 §6.4/§8.1/§10。测试映射：TD-002 T-L0i-01（空口往返）、T-L5/T-L6-03（板级上报/发送失败）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/src/measure.c` | 修改 `send_data_to_gateway()` | 删除被 `encode_frame10()` 覆盖的冗余 `send_data[0..7]` 手写组帧；改用**最近一次有效样本** `s_prev_temp_x10`/`s_last_hum_x10`（同时消除 ITEM-006 遗留的 `s_last_hum_x10` 只写不读）；长度改用 `SENSOR_RF_FRAME_LEN` |
| `.../USER/src/main.c` | 注释修正 | 主循环调用处陈旧注释（“1 min 采样”“立即/小时/首样本上报”）改为“3 分钟一拍”“条件上报: sensor_decide_report 门控” |
| `.../test/host_rf_frame_check.c` | 新增（测试载体） | 传感器 `encrytogate.c` ↔ 网关 `feistel_al.c` 往返互操作 14 项 |
| `.../test/host_measure_flow_check.c` | 扩展 | 新增发送路径 9 项（门控/样本选择/长度/成功清除/失败重试/放弃） |

### 行为要点

1. **门控**：`report_req == 0` 直接返回（不上报）；`report_req` 由 `temperature_process()` 按 `sensor_decide_report()` 置位。
2. **组帧**：完全由 `encode_frame10()` 完成：`uid_pick(4) | temp_x10_LE(2) | hum_x10_LE(2) | crc16_LE(2)` 后 8 轮 Feistel；本次未修改 `encrytogate.c`，帧布局/字节序/加密不变。
3. **发送**：`app_um2005C_send_data_timeout(send_data, SENSOR_RF_FRAME_LEN, SENSOR_RF_TX_TIMEOUT_MS)`（有界）。
4. **成功**：`report_req = 0` 并复位重试计数；**失败**：`report_retry++`，达到 `SENSOR_RF_TX_RETRY`（3）后清除 `report_req` 放弃本轮（下个 3 分钟周期自然重试）。
5. **样本一致性**：发送最近一次有效样本（与触发判定的样本一致）；失败周期不会用 0/脏值覆盖它。

**本项不包含**：旧 `hall/OPTCFG/params/history` 退役（ITEM-009，包括 `send_data_to_gateway()` 中遗留的 `optcfg_window_active()` 延后门与 `go_to_sleep()` 的旧门控）、宿主机测试合并（ITEM-010）、工程文件注册（ITEM-011）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误、28 条告警**（与 ITEM-007 同数；`measure.c` 0 告警，`main.c` 仅既有 4 条）；FLASH **35,856 B**（较 ITEM-007 −28 B，删除冗余组帧代码）、RAM 1,896 B（不变）。
- 空口往返（协议级）：`test/host_rf_frame_check.c`（传感器 encode ↔ 网关 decode）→ **14 passed / 0 failed**：T-L0i-01 向量、128 组随机、负温/极值、字段位置与小端序、密文篡改必拒、编码确定性。详见 `evidence/protocol_test.md`。
- 发送路径（流程级）：`test/host_measure_flow_check.c` → **24 passed / 0 failed**（含 9 项发送路径）。详见 `evidence/driver_test.md`。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`test/build_test.sh` 56/56。

## 交接与依赖

- **板级上报链路**：真实 433 发射/网关接收、发送失败重试上限（TD-002 T-L6-03）与端到端时间戳需嵌入式测试在真实硬件完成；且 ITEM-009 退役 `hall/OPTCFG` 前，PB04/PB05 引脚所有权冲突使实板采样/上报验证不具备有效前提。
- **`optcfg_window_active()` 遗留门**：`send_data_to_gateway()` 仍会在配置窗口内延后上报；因硬件已无 OPTCFG，该门恒为 false，不影响行为；完整移除属 ITEM-009。
- `SENSOR_TEST_TRACE` 观测钩子仍未接入（TD-002 §2 交接项）。

---

# 任务项 ITEM-009（已完成，独立验证 TEST_PASS）

**ITEM-009**：退役与新硬件冲突的旧通路：从工程与构建中移除 hall/OPTCFG/params/history 模块及其对 PB04/PB05/PB06 的初始化，移除 GPIOB 霍尔 EXTI 分支与 LPTIM 中的 OPTCFG 分支；PB04 仅作 AIN11 模拟输入、PB05 仅作光照供电输出、PB06 不外驱动。

设计映射：FWR-111；FD-002 §1.2/§3.3/§11.4；网表（PB04=LIGHT_ADC、PB05=LIGHTPOWER、PB06=NC、无霍尔器件）。测试映射：TD-002 T-L0-06（退役引用为 0）、T-L1-07（工程源文件列表）。

## 实际改动

**删除文件（8 个）**：`USER/src/hall.c`、`USER/inc/hall.h`、`USER/src/optcfg.c`、`USER/inc/optcfg.h`、`USER/src/params.c`、`USER/inc/params.h`、`USER/src/history.c`、`USER/inc/history.h`。

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/inc/main.h` | 修改 | 移除 `params.h`/`history.h`/`optcfg.h`/`hall.h` 包含 |
| `.../USER/src/main.c` | 修改 | 移除 `params_init/history_init/optcfg_init/hall_init` 与主循环内 Hall 去抖/开窗、`optcfg_process()` 分支 |
| `.../USER/src/interrupts_cw32l010.c` | 修改 | 移除 `hall.h`/`optcfg.h` 包含；`GPIOB_IRQHandler` 删除霍尔 EXTI 分支（PB04 已为模拟输入）；`LPTIM_IRQHandler` 删除 OPTCFG 采样分支，仅保留 433 位时钟 |
| `.../USER/src/measure.c` | 修改 | 移除 `optcfg.h`/`hall.h` 包含；`send_data_to_gateway()` 删除配置窗口延后门；`go_to_sleep()` 仅按 `report_req`/`sample_flag` 门控 |
| `.../USER/inc/fw_core.h`、`.../USER/src/fw_core.c` | 修改 | 移除参数(NVM 阈值)纯函数与 OPTCFG/1 解码器/帧解析；**保留** CRC16-CCITT-FALSE（通用纯工具，仍由测试覆盖）与本次新增的 CRC-8/换算/滞回/上报纯逻辑；不再包含 `params.h`/`string.h`/`math.h`（改 `<stddef.h>`） |
| `.../test/host_sensor_core_test.c`、`test/build_test.sh` | 修改 | 移除已退役的 params/OPTCFG/history 用例与 `history.c` 链接，仅保留 CRC16 回归（新用例属 ITEM-010） |
| `.../test/host_measure_flow_check.c` | 修改 | 移除不再被引用的 `optcfg_window_active`/`hall_event_pending` 桩 |

### 行为要点

1. **引脚所有权唯一**：PB04 仅由 `light.c` 配为模拟输入（AIN11）；PB05 仅由 `light.c` 配为推挽输出；PB06 无任何代码引用（DEBUG UART 用的是 PA05/PA06）。
2. **无遗留引用**：全工程源码中已无 hall/OPTCFG/params/history 的代码引用（仅保留说明性注释）；固件镜像中无相关符号。
3. **构建自动收敛**：`gcc/build.sh` 以 `USER/src/*.c` 通配编译，删除后自动不再编译；MDK/IAR 工程源文件列表本就未登记这些模块（也尚未登记 gxht40.c/light.c/fw_core.c），列表一致性属 ITEM-011。
4. **功能不变**：采样→判定→上报→深睡主流程不受影响（已由既有 harness 回归）。

**本项不包含**：把 gxht40.c/light.c/fw_core.c 加入 MDK/IAR 源列表（ITEM-011）、新增纯逻辑测试用例（ITEM-010）、网关固件核对（ITEM-012）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误、28 条告警**（与 ITEM-008 同数，均为既有模板/库告警）；FLASH 35,856 → **32,208 B**（−3,648 B）、RAM 1,896 → **1,712 B**（−184 B）。`arm-none-eabi-nm` 确认镜像中**无** hall/optcfg/params/history 符号，`gxht40_measure`/`light_sample`/`sensor_decide_report`/`fw_crc8_gxht` 均在。
- 退役引用检查：`grep -rn "hall|optcfg|params_|history|OPTCFG_|PARAMS_" USER/` → 仅 3 处说明性注释；MDK/IAR 工程文件本就无这些模块条目。
- 引脚所有权：PB04/PB05 仅出现在 `light.c`/`sensor_config.h`（owner）；PB06 无引用。
- 回归：`test/build_test.sh`（CRC16）2/2、`host_fw_core_pure_check.c` 36/36、`host_measure_flow_check.c` 24/24、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`host_rf_frame_check.c` 14/14。

## 交接与依赖

- **跨角色交接（embedded_verification）**：`test/host_measure_flow_verify_ev.c`、`host_rf_report_verify_ev.c`、`host_rtc_cadence_verify_ev.c` 仍引用已退役的 `optcfg.h`/`hall.h`/`params.h`/`history.h` 或其符号；本能力未修改这些跨角色验证文件，需测试侧适配（移除相应桩/包含）。
- **MDK/IAR 源列表（ITEM-011）**：删除的 8 个文件未在这些工程中登记（无需移除）；但 `gxht40.c`/`light.c`/`fw_core.c` 也未登记，ITEM-011 必须补上，否则 MDK/IAR 链接会缺少驱动/纯逻辑。
- **新纯逻辑用例（ITEM-010）**：本项已将 `host_sensor_core_test.c` 收敛为 CRC16 回归，ITEM-010 需补充 CRC-8/换算/光照滞回/上报判定用例。
- 板级引脚/功耗验证（TD-002 T-L3/T-L4）仍需真实硬件；本项完成后 PB04/PB05 引脚冲突已消除，实板验证前提具备。

---

# 任务项 ITEM-010（已完成，独立验证 TEST_PASS）

**ITEM-010**：更新宿主机纯逻辑测试 `test/host_sensor_core_test.c` 与 `test/build_test.sh`：覆盖 CRC-8 参考向量、换算边界（负温、0%/100% 截断）、光照阈值与滞回、上报判定边界（恰好 0.9℃、恰好 35.0℃、无前值、非暗、失败不更新），移除已退役的 OPTCFG/params 用例，脚本在本机运行通过。

设计映射：TD-002 T-L0-01…T-L0-06、T-L6-01；FD-002 §6。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../test/host_sensor_core_test.c` | 重写为综合纯逻辑套件 | 保留 CRC16 回归，新增 T-L0-01 CRC-8、T-L0-02 温度换算/合法域、T-L0-03 湿度换算/截断、组合换算、T-L0-04 光照滞回、T-L0-05 上报判定共 38 项 |
| `.../test/build_test.sh` | 扩展为两阶段 | 阶段 1：fw_core 纯逻辑（`host_sensor_core_test.c`）；阶段 2：采样/上报流程（`host_measure_flow_check.c` + mock MCU），覆盖“测量失败不更新前值” |

已退役的 OPTCFG/params/history 用例已在 ITEM-009 移除；本项不再涉及旧模块。

### 覆盖对应（TD-002）

| 用例 | 断言要点 | 结果 |
|---|---|---|
| T-L0-06 | CRC16 `123456789`→0x29B1、参考帧 33B→0xB540 | PASS ×2 |
| T-L0-01 | CRC-8 `{BE,EF}`→0x92、`{00}`→0xAC、空长度→0xFF | PASS ×3 |
| T-L0-02 | 温度 S=1872→−400 / 16855→0 / 26214→250 / 63664→1250；S=0→−450、0xFFFF→1300；有效域 -400/1250 合法、−450/1251/1300 无效；组合换算有效写输出/无效不写 | PASS ×11 |
| T-L0-03 | 湿度 S=0→0 / 2247→0 / 29360→500 / 55575→1000 / 0xFFFF→1000 | PASS ×5 |
| T-L0-04 | 已明 349→LIT、350→DARK；已暗 251→DARK、250→LIT；0→LIT、4095→DARK；带内保持 | PASS ×7 |
| T-L0-05 | 无前值 200→false / 351→true；降 9→false、10→true、11→true；非 DARK→false；cur=350→false；prev==cur→false | PASS ×9 |
| T-L6-01（流程） | 失败周期：不置 report_req、不更新前值、不写 0；恢复后按失败前前值判定 | PASS（阶段 2） |

### 行为要点

1. `build_test.sh` 成为单一宿主入口：**阶段 1** 纯逻辑（38/38）、**阶段 2** 流程（24/24），退出码 0。
2. “失败不更新前值”属 `measure.c` 流程行为，放在阶段 2（`host_measure_flow_check.c`，mock MCU 影子头）验证；文件头注释已交叉引用。
3. 保留的 `host_fw_core_pure_check.c`（ITEM-005 引入）与本套件用例重叠，作为独立 harness 保留。

**本项不包含**：MDK/IAR 源列表一致性（ITEM-011）、网关固件核对（ITEM-012）。

## 验证（本轮实际执行）

- `test/build_test.sh`（本机 MinGW-w64 gcc 12.2.0）→ 阶段 1 **38 passed / 0 failed**、阶段 2 **24 passed / 0 failed**，**退出码 0**。
- 交叉编译：`gcc/build.sh` → **0 错误、28 条告警**；FLASH 32,208 B / RAM 1,712 B（不变，测试不进入固件）。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_measure_flow_check.c` 24/24、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`host_rf_frame_check.c` 14/14。

## 交接与依赖

- **T-L0-05 用例数值（TD-002 需校正）**：任务文本/FD-002/IC-002 均为 `(prev-cur)>9`（降 9 不触发、降 10 触发）；TD-002 第②行写作 `prev=300,cur=290` 期望 false 与规则不符（“恰好 0.9℃”应为 cur=291）。实现与测试均按 `>9`，并同时固定 9/10/11 三个边界。
- MDK/IAR 源列表一致性（含 fw_core.c/gxht40.c/light.c）属 ITEM-011。
- 板级长时基/功耗/射频验证仍属嵌入式测试（TD-002 T-L4/T-L6/T-L8）。

---

# 任务项 ITEM-011（已完成，独立验证 TEST_PASS）

**ITEM-011**：交叉编译验证传感器工程：执行 `gcc/build.sh` 完成编译与链接并生成 elf/hex/bin，RAM 与 Flash 占用不超过 4KB/64KB，且 MDK/IAR 工程源文件列表与新增、移除的源文件保持一致。

设计映射：FD-002 §3/§11.12；TD-002 T-L1-02（交叉编译与产物）、T-L1-07（工程文件一致性）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../MDK/Project.uvprojx` | 修改 | User 组补 `fw_core.c`/`gxht40.c`/`light.c`；Driver 组补 `cw32l010_adc.c`（light.c 依赖 ADC） |
| `.../EWARM/project.ewp` | 修改 | User 组补齐 8 个 `USER/src/*.c`；Driver 组补 `cw32l010_adc/digitalsign/lptim/rtc/uart.c`；新增 `UM2005C` 与 `COMMON` 组；Release/Debug 两套 include 路径补 `..\UM2005C`/`..\COMMON` |
| `.gitignore` | 修改 | 新增 `*.exe`（宿主机测试产物，避免污染 `git status`；ITEM-010 验证提出的卫生项） |

### 一致性结果

- MDK/IAR 的 `USER/src` 列表与磁盘实际 `USER/src/*.c`（8 个）**逐一相等**；已删的 `hall/optcfg/params/history` 在两个工程中均不存在。
- 两个工程均含 `cw32l010_adc.c`、`UM2005C`（含 `app_um2005C.c`）、`COMMON`（`delay.c`）。
- 两个工程文件均为合法 XML（`ElementTree` 解析通过）。

### 本项不包含

网关固件（CH592）构建与核对（ITEM-012）、板级实机验证（嵌入式测试）。

## 验证（本轮实际执行）

- **交叉编译**：`gcc/build.sh` → **0 错误、28 条告警**；生成 `gcc/obj/sensor_fw.elf`、`.hex`、`.bin`。
  - `arm-none-eabi-size`：text 32,208 / data 84 / bss 1,628。
  - **FLASH 32,208 B / 65,536 B（49.15% ≤ 64 KB）**；**RAM 1,712 B / 4,096 B（41.80% ≤ 4 KB）**。
  - `.bin` MD5 `47abc6dfc798c837b2b68916f29db543`；`.hex` MD5 `ddf2e2e7bac47a80ea00177cb104afc9`。
- **生产编译器 AC5**（`armcc --cpu=Cortex-M0+ --c99`，CMSIS 5.9.0）：`USER/src` 8 个源文件均 **0 error / 0 warning**（`encrytogate.c` 3 条既有未用静态量告警）。
- **工程列表一致性**：脚本化比对 MDK/IAR 与 `ls USER/src/*.c` → 完全相等（见 `evidence/driver_test.md`）。
- **宿主机测试**：`test/build_test.sh` → 38/38 + 24/24，退出码 0。

## 交接与依赖

- **MDK/IAR 实机构建未执行**：本环境 Keil 受 CMSIS 6.3.0 与 AC5 不兼容影响（ITEM-001 已登记），IAR 工具链不可用；已用 GNU 交叉编译 + AC5 逐文件编译 + XML 合法性 + 列表一致性代替。建议在具备正确 CMSIS 5.9.0 包的 Keil/IAR 环境复编译。
- 网关固件（CH592 beiwov2）构建与核对属 ITEM-012。
- 板级实机验证（TD-002 T-L2/T-L3/T-L4/T-L6/T-L8）仍属嵌入式测试。

---

# 任务项 ITEM-012（本轮，末项）

**ITEM-012**：核查网关固件（CH592 beiwov2）兼容性：确认 `decode_frame10` 取 `temp=p[4..5]`、`hum=p[6..7]`，BLE 每设备 8 字节记录为 `id|hum_be|temp_be` 且与 Android `parsePlainFrame` 的 `u16be@4`/`s16be@6` 一致；一致则不改代码并记录核对结论，不一致才按接口契约对齐。

设计映射：IC-002 §3/§4；FD-002 §8/§9。

## 核对结论：**一致，未修改任何网关代码**

（`git status -- CH592EVT/` 为空）

| 环节 | 源码位置 | 实际行为 | 与契约 |
|---|---|---|---|
| 433 解密 | `APP/feistel_al.c: decode_frame10()` | `temp = p[4] \| p[5]<<8`（小端）；`hum = p[6] \| p[7]<<8`（小端）；CRC16 校验 `p[8..9]` 小端 | ✅ 与 IC-002 §3 及传感器 `encode_frame10` 一致 |
| 记录组装 | `APP/app_um2006A.c` | `resbf[4..5]=temp_be`、`resbf[6..7]=hum_be`；`sensorres[i*8+4..5]=resbf[6..7]`（湿度大端）、`[6..7]=resbf[4..5]`（温度大端） | ✅ 每设备 8 B = `id(4) \| hum_be(2) \| temp_be(2)` |
| 帧构造 | `APP/bleencrypt.c: build_device_block()` | `memcpy(out16, payload8, 8)`：设备块前 8 B 直拷，`[8..15]` 为 pad；`build_padded_frame_blocks()` = Header(16) + DeviceBlock(16)×N | ✅ 记录原样上 BLE |
| BLE 下发 | `APP/app_um2006A.c` → `encrypt_frame_ecb_inplace()` → `SimpleProfile_SetParameter(CHAR2)` | AES-128-ECB 加密后写 FFE2 | ✅ 不改变记录布局 |
| Android 解析 | `dengbei_care/.../MqtttService.kt: parsePlainFrame()` | `p8 = block[0..8)`；`humi = u16be(p8,4)/10`；`temp = s16be(p8,6)/10` | ✅ 与网关记录逐位一致（含负温符号） |

结论：传感器→433→网关→BLE→Android 的字段位置与字节序**已一致**，本次改动（GXHT40 不影响帧布局/单位）无需网关固件变更；网关也不应再按自身温度历史二次过滤（IC-002 §3，源码中未发现此类过滤）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../test/host_rf_frame_check.c` | 扩展（仅测试） | 新增第 [6] 组：真实传感器编码器 → 真实网关 `decode_frame10` → 真实 `build_device_block`/`build_padded_frame_blocks` → 按 Android `u16be@4`/`s16be@6` 解析；22 项 |

**未修改**：`CH592EVT/EVT/EXAM/BLE/beiwov2/**`（网关固件）、`dengbei_care/**`（Android）。

## 验证（本轮实际执行）

- 空口→BLE→Android 映射自检：`test/host_rf_frame_check.c` → **22 passed / 0 failed**（含 1 设备整帧 32 B、Header `gwid6+devCount@6`、offset 16 起解析）。详见 `evidence/protocol_test.md`。
- 回归：`gcc/build.sh` 0 错误/28 告警（FLASH 32,208 B / RAM 1,712 B 不变）；`test/build_test.sh` 38/38 + 24/24。
- 变更范围：`git status` 仅 `host_rf_frame_check.c` 一个测试文件；网关与 Android 源码 0 改动。

## 交接与依赖

- **网关实机构建未执行**：CH592 beiwov2 需 WCH 工具链（MounRiver，`config.h` 由 IDE 生成），本环境不具备；本轮以真实网关解码器/帧构造器的宿主机编译与逐位映射验证代替（网关源码未改）。
- **板级端到端**（真实 433 发射/接收 + BLE + App 消费）属嵌入式测试（TD-002 T-L2/T-L7）。
- Android 侧是否需改动由 Android 能力判定（readme 修改点 6）；固件侧已保证空口/BLE 布局不变（FWR-OPEN-5）。

---

# 后续任务项状态

`artifacts/firmware_tasks.yaml` 共 12 项；**ITEM-001…ITEM-012 全部完成**（前十一项独立验证 TEST_PASS，ITEM-012 本轮）。任务队列已到末项，后续路由由 Runtime 决定。

# 交接与依赖（累计）

- 光照阈值数值（`LIGHT_DARK_ENTER`/`EXIT`）当前为**未标定工程默认值**（`LIGHT_DARK_CALIBRATED=0`）；按 test_design T-L3-03 实板标定后回填配置头（FWR-OPEN-1）。
- 调试钩子 `SENSOR_TEST_TRACE` 已在配置头预留，实际打印点在后续采样/上报任务项中接入（test_design §2 交接项）。
- `sf_i2c.c` 既有函数 `i2c_write_multi_byte`/`i2c_write_multi_byte_16bit` 存在"`length==0` 时 `err` 未初始化"的既有缺陷；本项按「既有函数语义不变」要求未修改，登记为后续可选清理项。
