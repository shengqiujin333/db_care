# 嵌入式验证证据（EV-033 / LIGHT-DEF-2 交付固件再次实板复核）

状态：`embedded_tester.embedded_verification` 独立验证证据（本轮独立 rebuild、下载、4 个 COM43 原始窗口、1 次位级 I²C 被动采集/离线解码、宿主与静态复核）
输入：`runtime.artifacts.firmware_current` = `artifacts/firmware_implementation.md`（FWI-003 **rev 8**，LIGHT-DEF-2）；测试方案 = `artifacts/test_design.md`（TD-002 rev 8.0，6 项任务）
负责角色：嵌入式测试

**结论：`TEST_FAIL`（完整目标；`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）**

上电五行/`uid` 项（`FW-DIAG-BOOT`，BOT-01/02/03）已由用户明确回复“无法准备，先跳过”，按 `SKIP_CONFIRMED` 记录为**人工确认未执行**；本流程后续不再重测该项，也绝不写成通过。其余强制项无实测缺陷，故按契约返回 `TEST_FAIL`（`EXTERNAL_CONDITION_UNEXECUTED`），不以未执行项放行。

本轮未发现固件功能缺陷。当前交付增量（LIGHT-DEF-2 形制、`T=0` 未启用等价性）已在真实板上按可执行范围再次成立；失败的原因是完整目标中仍有必需项未执行且无已测缺陷：上电五行/`uid` 的采集编排不可用（`FW-DIAG-BOOT`）、判据启用缺少接受记录与双态被动证据（`FW-LIGHT-DEF2-ENABLE`）、真实升温/超温触发与跨链路按用户安排属后续阶段、功耗项按已批准范围延期（E2E-04）。

1. **身份闭合**：验证时 HEAD `0008dde`；`git diff 82a7e74..HEAD -- USER/ MDK/ EWARM/` 为空（产品源码与工程文件 = FWI-003 rev 8 实现提交）；差异仅为本角色上轮同步的 `test/*_verify_ev.c`/`test/mock_mcu/`。独立 `mdk_build {"action":"rebuild"}` **0 Error / 1 既有 warning**（`main.c(254)` 空循环体），`Code=25964 RO=656 RW=116 ZI=1676`；axf md5 **`b6a15fed2ed5e3d0c4adc51ac87a4830`**、hex **`4d728dec9b0d28a352c46436b7586c72`**（与 FWI-003 rev 8 逐字符一致）。`mdk_flash {}` 于 23:14:32.82–23:14:45.41Z 返回 **`Erase Done. Programming Done. Verify OK. Application running`**；下载后两产物 md5 未变。`Verify OK` 只证明下载校验。
2. **`FW-LIGHT-DEF2-MECH` 通过（可行范围）**：独立 `light_is_dark` 穷举 **1,253,376 例 / 153 个阈值 / 0 不一致**（本轮重新执行）；真实 `light.c` 接线在 `T=0`（40/40）与 `T=3000` 启用变体（41/41）下均通过；ADC 全超时不合成满量程、无滞回/状态；旧 `LIGHT_DARK_REF_CODE`/`LIGHT_DARK_CALIBRATED` 在 `USER/` 与全部测试资产中零残留；三通道构建（Keil rebuild、GNU `gcc/build.sh`、`test/build_test.sh`）0 Error。实板 `T=0` 等价性成立：**5 个周期全部 `V=0 D=0 r=0 s=0 y=0`**。
3. **`FW-OBS-PASSIVE` 通过**：COM43/9600 8N1 取得 **k=3、9、12、15、21**，其中 **k=9 → 12 → 15 为连续 3 个采样机会**（差值恒 3）；全部 `q=1`、`o=8`、`t/h` 在有效域、`a∈[n,x]`、行长 78–79 B（含 CRLF）≤96、无 NUL/杂散、采样间期无输出；`p(k=12)=241=t(k=9)`、`p(k=15)=235=t(k=12)`（F-1 保持）。`p(k=9)=241` 与 `t(k=9)=241` 相等是**相邻两周期温度相同的巧合**，由 `p(k=12)=241` 可反证 `p` 取的是上一周期值，不是本周期值；`p(k=21)=233` 同理指向未采集的 k=18。
4. **位级 I²C 补充**：`fx2lafw:conn=2.32` 被动采集（8,000,000 样点 @4 MS/s，2.0 s，D0=SCL、D1=SDA）解码为 `0x44` 写 `0xFD` → `0x44` 读 `63 DF D3 63 EA E3` → NACK/STOP；独立复算双字 CRC-8 **均匹配**（23.3 ℃ / 42.8 %RH）。命令 STOP→读 START 32.515 ms、SCL 位周期 614.25 µs（名义值），与既有基线一致。该事务换算值 233 等于 `p(k=21)=233`，故对应未在串口窗口内捕获的 k=18 周期（**推断配对，不是同窗直接配对**）。
5. **宿主独立证据**：测试侧 13 个 harness **609 项 0 失败**（含 LIGHT-DEF-2 穷举、真实 `light.c` 双变体、升温方向与冻结样本发送路径）；实现侧 `test/build_test.sh` **222 项 0 失败**；GNU 交叉构建 0 error（FLASH 34840 B / RAM 1816 B）。静态核查 **21/21 PASS**。无陈旧不可编译资产被排除。
6. **未满足的完整目标项**（详见 §7）：
   - **`FW-DIAG-BOOT`（BOT-01/02/03）经人工确认未执行（`SKIP_CONFIRMED`）**：本运行时 `serial_capture` 与 `mdk_flash` 严格串行，端口在 `Application running` 后 **12.9 s** 才建立，仅取得周期 `S` 行；`BOOT/VDD/IOTEST/IOSIG/BUS` 与 `uid` 未取得。pyocd 0.36.0 可见 CW-DAPLINK 探针，但 **`Target=n/a`、`pyocd list --targets --name CW32` 为空**，且 pyocd 不是 `runtime/environment.yaml` 登记入口，未执行复位、未安装 DFP。用户已回复“无法准备，先跳过”，按未执行记录，不再重测、不写成通过。
   - **`FW-LIGHT-DEF2-ENABLE`（EN-01/02）未执行**：`artifacts/interface_contract.md` 无 LIGHT-DEF-2 形制/数值接受记录，无同板同构建双态被动证据；用户明确当前不能改变光强，本轮不安排遮光/照射。`T` 保持 0。
   - **真实升温 >0.9 ℃ 且无光 / 稳定 >35 ℃ 触发与跨链路**：用户约束禁止调温；跨链路按用户安排进入后续联调。本轮全部周期 `r=0 s=0 y=0`、无 `P`、无发送。
   - **E2E-04 功耗**：按已批准范围 **DEFERABLE** 延期（缺少电流表/SMU/示波器），本轮未测，报告中列明。

---

## 1. 被测身份、构建与部署

| 项 | 原始结果 |
|---|---|
| 验证时 HEAD | `0008dde600a3d53bc47ac9f8e4c2ace91cf665f0`（接手时工作树干净） |
| 产品源码身份 | `git diff 82a7e74..HEAD -- USER/ MDK/ EWARM/` **为空**；即当前交付源码 = FWI-003 rev 8；`82a7e74` 为 `[role:firmware_engineer]` 实现提交 |
| 独立构建 | `mdk_build {"action":"rebuild"}`（23:14:22.24–23:14:24.31Z）：**0 Error / 1 Warning**（既有 `main.c(254) while loop has empty body`）；`Program Size: Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 产物身份 | axf **`b6a15fed2ed5e3d0c4adc51ac87a4830`**、hex **`4d728dec9b0d28a352c46436b7586c72`**（本能力独立复算；与 FWI-003 rev 8 一致） |
| 部署 | `mdk_flash {}`（23:14:32.82–23:14:45.41Z）：`Build target 'Project'` 0E/0W → `Erase Done.Programming Done.Verify OK.Application running ...`；下载后 axf/hex md5 不变 |
| GNU 交叉构建 | `gcc/build.sh` exit 0；FLASH **34840 B** / RAM **1816 B**（与实现记录一致） |
| 调试串口 | `serial_list_ports`：`COM43` = Prolific `067B:2303`、`LOCATION=1-9.4.2`，9600 8N1；`COM26` = `C251:F001`（调试器 CDC，未使用） |
| USB 在位 | CW-DAPLink `C251:F001` SN `87094109484987710672FF50`；手机 `18D1:4EE7` SN `12F046690506`；FX2 `04B4:8613`（`fx2lafw`）；**未见**电流表/SMU/示波器 |
| 逻辑分析仪 | `sigrok_probe` conn=2.32 exit 0；`sigrok_capture`/`sigrok_decode` exit 0（参数与原始文件见 §3） |

原始：`evidence/ev033/mdk_rebuild_build.log`、`image_identity.txt`、`mdk_flash.log`、`gcc_build.log`、`git_identity_static.txt`、`tool_results.txt`。

## 2. 真实目标 COM43 周期（9600 8N1，无刺激）

原始：`cap01_k3.bin`、`cap02_k9.bin`、`cap03_k12_k15.bin`、`cap04_k21.bin`、`capture_windows.txt`；独立解析：`analyze_ev033.py` → `capture_analysis.txt`（不引用实现常量）。

| 窗口（UTC） | 原始行 | 判读 |
|---|---|---|
| 23:14:58–23:19:58 | `S k=3 p=242 H=1 V=0 o=8 n=3800 x=3810 a=3806 D=0 t=242 h=378 q=1 r=0 s=0 y=0` | `q=1`；`V=0 D=0`；无 `G` |
| 23:20:10–23:25:10 | `S k=9 p=241 H=1 V=0 o=8 n=3771 x=3779 a=3774 D=0 t=241 h=386 q=1 r=0 s=0 y=0` | `p(k=9)=241`；与 `t(k=9)=241` 相同（相邻周期同温巧合） |
| 23:25:19–23:30:19 | `S k=12 p=241 … t=235 h=394 q=1 …`；`S k=15 p=235 … t=233 h=414 q=1 …` | **连续 k=12→15**；`p(k=12)=241=t(k=9)`、`p(k=15)=235=t(k=12)`；`p≠t` |
| （串口间隙） | 位级 I²C：`0x44 W FD` → `0x44 R 63 DF D3 63 EA E3` | 双 CRC 匹配，23.3 ℃ / 42.8 %RH；对应 k=18（推断：`p(k=21)=233`） |
| 23:32:28–23:37:28 | `S k=21 p=233 H=1 V=0 o=8 n=3715 x=3726 a=3720 D=0 t=233 h=429 q=1 r=0 s=0 y=0` | `p(k=21)=233`；与位级事务换算值一致（推断配对） |

判读：
1. **连续性与节拍**：k = 9、12、15 为连续 3 个采样机会，另取得 3、21；相邻 `k` 差恒为 3，同一 `k` 无重复行，采样间期无字节输出。k=6、k=18 落在未采集间隙，其 S 行未取得（不是漏采样）。
2. **格式/预算**：全部行 78–79 B（含 CRLF）≤96；15 字段顺序与冻结字面一致；CRLF 完整、无 NUL、无杂散。
3. **判据未启用等价性**：全部 5 个周期 `V=0 D=0`，`r=0 s=0 y=0`；`a=3720..3806`、`o=8` 仅记录，不作为“已识别有光”或光路好坏结论。
4. **F-1 语义**：连续周期 `p` 等于上一周期 `t`；k=9 的 `p==t` 由 k=12 的 `p=241` 反证为同温巧合，不是“`p` 取本周期值”。
5. **失败路径**：本轮无 `q=0`、无 `G`；失败语义与失败后重探未在板上触发（宿主证据只作背景，不写成实板通过）。
6. **节拍精度**：readme 未给出 LSI 容差；墙钟间隔约 56–58 s/分钟拍（以 `k` 内部证据为准），本报告不声称 180.0 s 精度。

## 3. 位级 I²C（可选补充，非替代任何实板行证据）

- 采集：`fx2lafw:conn=2.32`、D0=SCL、D1=SDA、D1 下降沿触发、8,000,000 样点 @4 MS/s（2.0 s），数据窗约 23:32:00.8–23:32:02.8Z。原始 `.sr` md5 `69cdd160451cadbeccf43a06c9ae6f8c`；`decoded_i2c.txt`。
- 解码：`Start / Address write 44 / ACK / Data write FD / ACK / Stop`；`Start / Address read 44 / ACK / Data read 63 DF D3 63 EA E3 / NACK / Stop`。
- 独立复算：温度字 `0x63DF` CRC `0xD3` **MATCH**；湿度字 `0x63EA` CRC `0xE3` **MATCH**；换算 233 x10（23.3 ℃）/ 428 x10（42.8 %RH）。
- 名义时序：写事务 12.046 ms；命令 STOP→读 START **32.515 ms**；读事务 40.150 ms；SCL 位间隔 **614.25 µs**（≈1.628 kHz）。
- 边界：数字采样只证明逻辑电平与采样分辨率内时序，不证明模拟幅度/上升沿/上拉质量；配对为推断（见 §2 表）。

## 4. 宿主与静态证据（同一源码）

| 入口 | 结果 |
|---|---|
| 实现侧 `test/build_test.sh` | 48+61+54+28+31 = **222 / 0**，exit 0 |
| 测试侧 13 个独立 harness | **609 / 0**（明细见 `host_harness_summary.txt`） |
| LIGHT-DEF-2 独立穷举 | 1,253,376 例、153 个阈值、**0 不一致**；`valid=false` 恒 false；`T=0` 恒 false；`mean=T-1/T` 为 false/true；`T=4095` 仅满量程 |
| 真实 `light.c` 双变体 | `T=0` 40/0（`valid=false/dark=false`、全超时无合成、PB05/ADC 时序）；`T=3000` 41/0（边界相等为暗、无状态） |
| T1/T2/T3 回归 | 59/0、79/0 + 30/0、38/0；升温方向、严格 >35.0 ℃、冻结快照/重试/不覆盖、P 行、TXE/TXBUSY 有界 |
| 静态核查 | `static_check_ev033.py` **21/21 PASS**：常量 0、旧宏零残留、无浮点/动态内存、无旧通路、S 行顺序、上电顺序、有界等待 |
| 验证资产边界 | 实现提交 `82a7e74` 未修改任何 `*_verify_ev.c`/`mock_*_ev/`；本轮测试侧资产沿用上轮同步版本，未改产品源码 |

## 5. 验收目标 → 本轮结果

| 任务 | 结果 | 依据 |
|---|---|---|
| FW-REG-ASBUILT | **局部通过（附身份说明）** | 当前源码为 LIGHT-DEF-2，独立重建摘要必为 `b6a15fed…`（与实现记录一致）；as-built 历史摘要 `a3b08774…` 由实现侧改前基线记录，本轮未回退源码重演。宿主回归/静态不变量全部通过 |
| FW-REG-REPORT | **局部通过** | T1/T2/T3 独立 harness 全部通过；升温方向与冻结快照在板外可复现 |
| FW-LIGHT-DEF2-MECH | **通过（本任务范围）** | MCH-01..10：形制/接线/超时/旧宏/三构建/资产边界全部满足；`T=0` 实板等价性成立 |
| FW-OBS-PASSIVE | **通过** | ≥3 连续周期（k=9/12/15）+ k=3/21 全部 `q=1`，字段/节拍/行长/无杂散全部满足；无失败周期 |
| FW-LIGHT-DEF2-ENABLE | **未执行（前置缺失）** | EN-01 无接受记录；EN-02 无双态被动证据且用户禁止改变光强；`T=0` 保持 |
| FW-DIAG-BOOT | **未执行（经人工确认跳过，`SKIP_CONFIRMED`）** | 端口晚于上电段 12.9 s；pyocd 无 CW32 目标且未登记；用户回复“无法准备，先跳过”；不以后续 `S` 替代、不再重测 |
| 真实升温/超温触发、跨链路 | **未执行（用户阶段安排）** | 禁止调温；跨链路属后续联调；本轮无 `P`/发送 |
| E2E-04 功耗 | **延期（已批准）** | 无电流表/SMU/示波器；`human_test_deferral_allowed_items=["E2E-04"]` |

**人工确认与批准延期（明示）**：`FW-DIAG-BOOT`（BOT-01/02/03）经用户回复“无法准备，先跳过”确认为**未执行**（`SKIP_CONFIRMED`，非延期批准，仍为强制阻断项；原因与所需前置保留在本证据，不再重测/不重复询问）。`E2E-04`（CR2032 采样峰值/深睡电流）属已批准 `DEFERABLE` 项，本轮缺少功耗测量仪器，**未测**并在此列明。

## 6. 本轮测试侧资产状态（未改产品源码）

| 文件 | 状态 | 说明 |
|---|---|---|
| `test/host_fw_core_verify_ev.c` | 上轮已同步（本次仅运行） | LIGHT-DEF-2 穷举与边界 |
| `test/host_light_verify_ev.c` | 上轮已同步（本次仅运行） | 真实 `light.c`、`T=0` 与 `T=3000` 双变体 |
| `test/host_vdd_verify_ev.c` | 上轮已同步（本次仅运行） | `LIGHT_DARK_THRESHOLD_CODE == 0` 门禁 |
| `test/host_rf_report_verify_ev.c` | 上轮已同步（本次仅运行） | 结构体 `light_sample` 桩与升温触发向量 |
| `test/mock_mcu/mock_cw32.h` | 上轮已同步（本次仅运行） | `ADC_InputVref1P2`/`CR_f.BGREN` |

`git status` 确认产品源码、工程文件、网关与 Android 零改动；`evidence/ev033/` 为本轮新增证据。

## 7. 失败/未执行报告（逐项关联目标、用例、身份与原始证据）

| # | 目标/用例 | 预期 | 实际 | 原始证据 | 已尝试措施 | 尚未排除的假设 | 最小下一步 |
|---|---|---|---|---|---|---|---|
| F-1 | FW-DIAG-BOOT / BOT-01/02/03 | 同一 COM43 窗口取得 `BOOT→VDD→IOTEST→IOSIG→BUS` 与 `fw/uid/rst` | 仅取得周期 `S`；上电段在端口打开前已输出（flash 结束 23:14:45.4，端口 23:14:58.3 建立） | `cap01_k3.bin`、`tool_results.txt` §3–4 | 下载后立即采集；只读评估 pyocd 0.36.0（`Target=n/a`、CW32 目标表为空）；已向用户请求该采集编排前置 | 是否存在未登记、可“先开端口后复位”的并发编排入口（当前无） | **用户已明确回复“无法准备，先跳过”（`SKIP_CONFIRMED`）**：本项按人工确认未执行记录，本轮及后续同一批准流程不再重测、不写成通过；后续若提供预打开端口 + 窗口内复位或登记的 CW32L010 调试入口，再另行验证。不得改固件、不得用后续 `S` 替代 |
| F-2 | FW-LIGHT-DEF2-ENABLE / EN-01/02 | 存在判据形制/数值接受记录 + 同板同构建双态被动证据后才可启用 | 无接受记录（`interface_contract.md` 无 LIGHT-DEF-2 匹配）；无受光/无光双态（用户禁止改变光强）；`T=0` | `static_check_ev033.txt`（常量 0）、`capture_analysis.txt` | 记录 `a=3720..3806` 事实；未回填、未遮光/照射 | 分离度与阈值数值仍待需求/架构接受 | 上游给出接受记录与被动双态证据后，由固件实现回填 `T` 并重建；测试侧再复验 `V=1 D=0/1` |
| F-3 | 真实升温/超温触发与跨链路 | 暗 + 升温 >0.9 ℃ 或 >35.0 ℃ 时 `r=1`、`P` 冻结、`s=1`，手机同笔一致 | 全部周期 `r=0 s=0 y=0`，无 `P`、无发送 | 同上 | 未施加刺激（用户禁止）；本能力无 `android_test` 入口 | 网关/手机链路是否可用（未观测） | 按用户安排的后续联调阶段执行；不伪造发送成功/失败 |
| F-4 | T4-03 失败路径 | `q=0` 后紧跟 `G`，失败零副作用、下周期重探/恢复 | 本轮 5/5 `q=1`、0 条 `G`；未触发 | `capture_analysis.txt` | 连续采集至 k=21 | 更长运行/不同供电条件下的失败率（未观测） | 出现真实 `q=0` 时按 `G` 原始行取证；不人为制造失败或改判据 |
| F-5 | E2E-04 功耗 | CR2032 采样峰值/深睡电流实测值与条件 | 未执行（已批准延期） | `tool_results.txt` §5（无仪器枚举） | 枚举 USB 设备 | 是否存在未登记测量入口（无） | 仪器具备时由硬件测试测量并记录量程/接线/电池电压 |

## 8. 证据局限（如实登记）

1. 串口窗口之间存在未采集间隙（cap01/cap02 间 11.8 s、cap02/cap03 间 9.2 s、cap03/cap04 间 2 min 8.5 s）；k=6、k=18 的 `S` 行未取得，位级事务与 k=18 的对应为**推断**（换算值 233 与 `p(k=21)=233` 相等）。
2. 上电五行与 `uid` 未取得（用户已明确确认“无法准备，先跳过”，按 `SKIP_CONFIRMED` 记为未执行，不再重测）；镜像身份由构建摘要 + `Verify OK` 闭合，未由 `BOOT` 行交叉核对。
3. LIGHT-DEF-2 **启用后**行为只在宿主 `T=3000` 变体验证；板上保持 `T=0`，未取得 `V=1`/`D=1` 实板证据。
4. 无升温/超温、无网关/手机同窗、无功耗仪器；相应项未执行，不写通过。
5. `a=3720..3806` 与“当前有光”陈述的极性解释仍属现场/硬件未决项；本报告只记录事实，不据此判器件好坏。
6. 位级 I²C 为数字采样（4 MS/s、250 ns 分辨率），不证明模拟幅度/上升沿；名义时间不能跨设备同步。
7. as-built 摘要 `a3b08774…` 未在本轮回退源码重演；当前交付摘要 `b6a15fed…` 已独立重建确认。

## 9. 本轮实际改动（只增测试侧证据）

- 新增：`evidence/ev033/`（原始 `.bin`×4、原始 `.sr`、解码、分析/静态脚本与输出、工具结果、宿主日志汇总）
- 更新：`evidence/test.md`（本段置顶；EV-032 及更早保留）、`evidence/programmed_prototype.md`（EV-033 下载身份置顶）、`file_manifest.txt`
- **未修改**：`USER/**` 产品源码、Keil/IAR 工程、网关、Android、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载软件或驱动

---
