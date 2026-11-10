# 嵌入式验证证据（EV-032 / LIGHT-DEF-2 交付固件实板复核）

状态：`embedded_tester.embedded_verification` 独立验证证据（本轮独立 rebuild、下载、3 个 COM43 原始窗口、1 次位级 I²C 被动采集/离线解码、宿主与静态复核）
输入：`runtime.artifacts.firmware_current` = `artifacts/firmware_implementation.md`（FWI-003 **rev 8**，LIGHT-DEF-2）；测试方案 = `artifacts/test_design.md`（TD-002 rev 8.0，6 项任务）
负责角色：嵌入式测试

**结论：`TEST_FAIL`（完整目标；`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）**

本轮未发现固件功能缺陷。当前交付增量（LIGHT-DEF-2 形制、`T=0` 未启用等价性）已在真实板上按可执行范围成立；失败的原因是完整目标中仍有必需项未执行且无已测缺陷：上电五行/`uid` 的采集编排不可用（`FW-DIAG-BOOT`）、判据启用缺少接受记录与双态被动证据（`FW-LIGHT-DEF2-ENABLE`）、真实升温/超温触发与跨链路按用户安排属后续阶段、功耗项按已批准范围延期（E2E-04）。

1. **身份闭合**：验证时 HEAD `abaa706`；产品源码与实现提交 `82a7e74` 逐字节一致（`git diff 82a7e74 -- USER/工程文件` 为空）。独立 `mdk_build {"action":"rebuild"}` **0 Error / 1 既有 warning**，`Code=25964 RO=656 RW=116 ZI=1676`；axf md5 **`b6a15fed2ed5e3d0c4adc51ac87a4830`**、hex **`4d728dec9b0d28a352c46436b7586c72`**（与 FWI-003 rev 8 一致）。`mdk_flash {}` 于 14:31:30Z 返回 **`Erase Done. Programming Done. Verify OK. Application running`**；下载后两产物 md5 未变。`Verify OK` 只证明下载校验。
2. **`FW-LIGHT-DEF2-MECH` 通过（可行范围）**：`light_is_dark` 独立穷举 **1,253,376 例 / 153 个阈值 / 0 不一致**；真实 `light.c` 接线在 `T=0`（40/40）与 `T=3000` 启用变体（41/41）下均通过；ADC 全超时不合成满量程、无滞回/状态；旧 `LIGHT_DARK_REF_CODE`/`LIGHT_DARK_CALIBRATED` 在 `USER/` 与全部测试资产中零残留；三通道构建 0 Error。实板 `T=0` 等价性成立：**5 个周期全部 `V=0 D=0 r=0 s=0 y=0`**。
3. **`FW-OBS-PASSIVE` 通过**：COM43/9600 8N1 取得 **k=3、6、9（连续 3 个周期）** 以及 k=15、18，共 5 个周期；全部 `q=1`、`o=8`、`t/h` 在有效域、`a∈[n,x]`、行长 78–79 B（含 CRLF）≤96、无 NUL/杂散、采样间期无输出；相邻观测 `k` 差值恒为 3。`p(k)=t(k-3)` 链在 k=6/9/18 全部成立且 `p≠t`（F-1 保持）。
4. **位级 I²C 补充**：`fx2lafw:conn=2.32` 被动采集（8,000,000 样点 @4 MS/s，2.0 s，D0=SCL、D1=SDA）解码为 `0x44` 写 `0xFD` → `0x44` 读 `69 15 A1 51 8C 95` → NACK/STOP；独立复算双字 CRC-8 **均匹配**（26.8 ℃ / 33.8 %RH）。命令 STOP→读 START 32.531 ms、SCL 位周期 614.25 µs（名义值），与既有基线一致。该事务的换算值等于 `p(k=15)=268`，故对应未在串口窗口内捕获的 k=12 周期（**推断配对，不是同窗直接配对**）。
5. **宿主独立证据**：测试侧 13 个 harness **609 项 0 失败**（含本轮同步的 LIGHT-DEF-2 穷举、真实 `light.c` 双变体、升温方向与冻结样本发送路径）；实现侧 `build_test.sh` **222 项 0 失败**；GNU 交叉构建 0 error（FLASH 34840 B / RAM 1816 B）。静态核查 **21/21 PASS**。此前两个“陈旧不可编译”的测试资产已修复并纳入本轮证据，无排除项。
6. **未满足的完整目标项**（详见 §7）：
   - **`FW-DIAG-BOOT`（BOT-01/02/03）未执行**：本运行时 `serial_capture` 与 `mdk_flash` 严格串行，端口在 `Application running` 后 3.4 s 才建立，仅取得周期 `S` 行；`BOOT/VDD/IOTEST/IOSIG/BUS` 与 `uid` 未取得。pyocd 0.36.0 可见 CW-DAPLINK 探针，但 **`Target=n/a`、无 CW32 目标定义**，且 pyocd 不是 `runtime/environment.yaml` 登记入口，未执行复位、未安装 DFP。
   - **`FW-LIGHT-DEF2-ENABLE`（EN-01/02）未执行**：无 LIGHT-DEF-2 形制/数值接受记录（IC-002 §4.2 未同步），无同板同构建双态被动证据；用户明确当前不能改变光强，本轮不安排遮光/照射。`T` 保持 0。
   - **真实升温 >0.9 ℃ 且无光 / 稳定 >35 ℃ 触发与跨链路**：用户约束禁止调温；跨链路按用户安排进入后续联调。本轮全部周期 `r=0 s=0 y=0`、无 `P`、无发送。
   - **E2E-04 功耗**：按已批准范围 **DEFERABLE** 延期（缺少电流表/SMU/示波器），本轮未测，报告中列明。

---

## 1. 被测身份、构建与部署

| 项 | 原始结果 |
|---|---|
| 验证时 HEAD | `abaa706e6515494b64f9136e81f62cea107bfa3a`（接手时工作树仅测试侧资产改动） |
| 产品源码身份 | `git diff --stat 82a7e74 -- USER/ Project.uvprojx project.ewp` **为空**；即当前交付源码 = FWI-003 rev 8 |
| 独立构建 | `mdk_build {"action":"rebuild"}`（14:31:00.141–14:31:03.315Z）：**0 Error / 1 Warning**（既有 `main.c(254) while loop has empty body`）；`Program Size: Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 产物身份 | axf **`b6a15fed2ed5e3d0c4adc51ac87a4830`**、hex **`4d728dec9b0d28a352c46436b7586c72`**（本能力独立复算；与 FWI-003 rev 8 一致） |
| 部署 | `mdk_flash {}`（14:31:18.045–14:31:30.625Z）：`Build target 'Project'` 0E/0W → `Erase Done. Programming Done. Verify OK. Application running ...`；下载后 axf/hex md5 不变 |
| GNU 交叉构建 | `gcc/build.sh` exit 0；FLASH **34840 B** / RAM **1816 B**（与实现记录一致） |
| 调试串口 | `serial_list_ports`：`COM43` = Prolific `067B:2303`、`LOCATION=1-9.4.2`，9600 8N1；`COM26` = `C251:F001`（调试器 CDC，未使用） |
| USB 在位 | CW-DAPLink `C251:F001` SN `87094109484987710672FF50`；手机 `18D1:4EE7` SN `12F046690506`；FX2 `04B4:8613`（`fx2lafw`）；**未见**电流表/SMU/示波器 |
| 逻辑分析仪 | `sigrok_probe` conn=2.32 exit 0；`sigrok_capture`/`sigrok_decode` exit 0（参数与原始文件见 §3） |

原始：`evidence/ev032/mdk_rebuild_build.log`、`image_identity.txt`、`gcc_build.log`、`git_identity_static.txt`、`tool_results.txt`。

## 2. 真实目标 COM43 周期（9600 8N1，无刺激）

原始：`cap01_k3.cap`、`cap02_k6_k9.cap`、`cap03_k15_k18.cap`、`capture_windows.txt`；独立解析：`analyze_ev032.py` → `capture_analysis.txt`（不引用实现常量）。

| 窗口（UTC） | 原始行 | 判读 |
|---|---|---|
| 14:31:34–14:36:34 | `S k=3 p=283 H=1 V=0 o=8 n=3985 x=3992 a=3988 D=0 t=269 h=339 q=1 r=0 s=0 y=0` | `q=1`；`V=0 D=0`；无 `G` |
| 14:36:43–14:41:43 | `S k=6 p=269 … t=266 h=343 q=1 …`；`S k=9 p=266 … t=258 h=354 q=1 …` | **连续 k=6→9**；`p(k=6)=269=t(k=3)`、`p(k=9)=266=t(k=6)` |
| （串口间隙） | 位级 I²C：`0x44 W FD` → `0x44 R 69 15 A1 51 8C 95` | 双 CRC 匹配，26.8 ℃ / 33.8 %RH；对应 k=12（推断：`p(k=15)=268`） |
| 14:44:17–14:49:18 | `S k=15 p=268 … t=253 h=354 q=1 …`；`S k=18 p=253 … t=252 h=360 q=1 …` | `p(k=18)=253=t(k=15)`；`p≠t` |

判读：
1. **连续性与节拍**：k = 3、6、9 为连续 3 个采样机会，另取得 15、18；相邻 `k` 差恒为 3，同一 `k` 无重复行，采样间期无字节输出。
2. **格式/预算**：全部行 78–79 B（含 CRLF）≤96；15 字段顺序与冻结字面一致；CRLF 完整、无 NUL、无杂散。
3. **判据未启用等价性**：全部 5 个周期 `V=0 D=0`，`r=0 s=0 y=0`；`a=3984..3988`、`o=8` 仅记录，不作为“已识别有光”或光路好坏结论。
4. **F-1 语义**：`p` 恒等于上一周期 `t` 且 `p≠t`（k=6/9/18 全部成立）。
5. **失败路径**：本轮无 `q=0`、无 `G`；失败语义与失败后重探未在板上触发（宿主证据只作背景，不写成实板通过）。
6. **节拍精度**：readme 未给出 LSI 容差；墙钟间隔约 58.7 s/分钟拍（k 内部证据为准），本报告不声称 180.0 s 精度。

## 3. 位级 I²C（可选补充，非替代任何实板行证据）

- 采集：`fx2lafw:conn=2.32`、D0=SCL、D1=SDA、D1 下降沿触发、8,000,000 样点 @4 MS/s（2.0 s），14:42:08–14:43:13Z。原始 `.sr` md5 `ec5d22730670f56d4ee2e556a694974f`；`decoded_i2c.txt`。
- 解码：`Start / Address write 44 / ACK / Data write FD / ACK / Stop`；`Start / Address read 44 / ACK / Data read 69 15 A1 51 8C 95 / NACK / Stop`。
- 独立复算：温度字 `0x6915` CRC `0xA1` **MATCH**；湿度字 `0x518C` CRC `0x95` **MATCH**；换算 268 x10（26.8 ℃）/ 338 x10（33.8 %RH）。
- 名义时序：写事务 12.051 ms；命令 STOP→读 START **32.531 ms**；读事务 40.165 ms；SCL 位间隔 **614.25 µs**（≈1.628 kHz）。
- 边界：数字采样只证明逻辑电平与采样分辨率内时序，不证明模拟幅度/上升沿/上拉质量；配对为推断（见 §2 表）。

## 4. 宿主与静态证据（同一源码）

| 入口 | 结果 |
|---|---|
| 实现侧 `test/build_test.sh` | 48+61+54+28+31 = **222 / 0**，exit 0 |
| 测试侧 13 个独立 harness | **609 / 0**（明细见 `host_harness_summary.txt`） |
| LIGHT-DEF-2 独立穷举 | 1,253,376 例、153 个阈值、**0 不一致**；`valid=false` 恒 false；`T=0` 恒 false；`mean=T-1/T` 为 false/true；`T=4095` 仅满量程 |
| 真实 `light.c` 双变体 | `T=0` 40/0（`valid=false/dark=false`、全超时无序合成、PB05/ADC 时序）；`T=3000` 41/0（边界相等为暗、无状态） |
| T1/T2/T3 回归 | 59/0、79/0 + 30/0、38/0；升温方向、严格 >35.0 ℃、冻结快照/重试/不覆盖、P 行、TXE/TXBUSY 有界 |
| 静态核查 | `static_check_ev032.py` **21/21 PASS**：常量 0、旧宏零残留、无浮点/动态内存、无旧通路、S 行顺序、上电顺序、有界等待 |
| 验证资产边界 | 实现提交 `82a7e74` 未修改任何 `*_verify_ev.c`/`mock_*_ev/`；本轮测试侧资产同步见 §6 |

## 5. 验收目标 → 本轮结果

| 任务 | 结果 | 依据 |
|---|---|---|
| FW-REG-ASBUILT | **局部通过（附身份说明）** | 当前源码为 LIGHT-DEF-2，独立重建摘要必为 `b6a15fed…`（与实现记录一致）；as-built 历史摘要 `a3b08774…` 由实现侧改前基线记录，本轮未回退源码重演。宿主回归/静态不变量全部通过 |
| FW-REG-REPORT | **局部通过** | T1/T2/T3 独立 harness 全部通过；升温方向与冻结快照在板外可复现 |
| FW-LIGHT-DEF2-MECH | **通过（本任务范围）** | MCH-01..10：形制/接线/超时/旧宏/三构建/资产边界全部满足；`T=0` 实板等价性成立 |
| FW-OBS-PASSIVE | **通过** | ≥3 连续周期（k=3/6/9）+ k=15/18 全部 `q=1`，字段/节拍/行长/无杂散全部满足；无失败周期 |
| FW-LIGHT-DEF2-ENABLE | **未执行（前置缺失）** | EN-01 无接受记录；EN-02 无双态被动证据且用户禁止改变光强；`T=0` 保持 |
| FW-DIAG-BOOT | **未执行（编排缺口）** | 端口晚于上电段 3.4 s；pyocd 无 CW32 目标且未登记；不以后续 `S` 替代 |
| 真实升温/超温触发、跨链路 | **未执行（用户阶段安排）** | 禁止调温；跨链路属后续联调；本轮无 `P`/发送 |
| E2E-04 功耗 | **延期（已批准）** | 无电流表/SMU/示波器；`human_test_deferral_allowed_items=["E2E-04"]` |

## 6. 本轮测试侧资产同步（只改验证角色自有资产，未改产品源码）

| 文件 | 改动 | 目的 |
|---|---|---|
| `test/host_fw_core_verify_ev.c` | T7/T7b 由 1/3 形制改为 LIGHT-DEF-2 穷举与边界（`T=0/1/3590/4095`、`valid=false`） | 独立验证新判据，防止旧断言污染结论 |
| `test/host_light_verify_ev.c` | 按 `light_result_t`/LIGHT-DEF-2 重写；`mock_mcu` 影子层；`T=0` 与 `-DLIGHT_DARK_THRESHOLD_CODE=3000u` 双变体 | 独立验证真实 `light.c` 接线、全超时、部分成功、无状态 |
| `test/host_vdd_verify_ev.c` | 旧 `LIGHT_DARK_CALIBRATED` 断言改为 `LIGHT_DARK_THRESHOLD_CODE == 0` 门禁 | 编译恢复并贴合新语义 |
| `test/host_rf_report_verify_ev.c` | `light_sample` 桩改为结构体；触发向量改为升温方向（320→400）且引用冻结触发样本 | 使 T2 发送路径在当前语义下可独立复验 |
| `test/mock_mcu/mock_cw32.h` | 补 `ADC_InputVref1P2` 与 `CR_f.BGREN` | 供真实 `light.c` 编译 |

`git status` 确认产品源码、工程文件、网关与 Android 零改动；实现侧的 4 个自检文件未由本能力修改。

## 7. 失败/未执行报告（逐项关联目标、用例、身份与原始证据）

| # | 目标/用例 | 预期 | 实际 | 原始证据 | 已尝试措施 | 尚未排除的假设 | 最小下一步 |
|---|---|---|---|---|---|---|---|
| F-1 | FW-DIAG-BOOT / BOT-01/02/03 | 同一 COM43 窗口取得 `BOOT→VDD→IOTEST→IOSIG→BUS` 与 `fw/uid/rst` | 仅取得周期 `S`；上电段在端口打开前已输出（flash 结束 14:31:30.6，端口 14:31:34.0 建立） | `cap01_k3.cap`、`tool_results.txt` §9–10 | 下载后立即采集；只读评估 pyocd 0.36.0 | 是否存在未登记、可“先开端口后复位”的并发编排入口（当前无） | 由编排/环境所有者提供预打开端口 + 窗口内复位；或登记并授权带 CW32L010 DFP 的调试入口。不得改固件、不得用后续 `S` 替代 |
| F-2 | FW-LIGHT-DEF2-ENABLE / EN-01/02 | 存在判据形制/数值接受记录 + 同板同构建双态被动证据后才可启用 | 无接受记录；无受光/无光双态（用户禁止改变光强）；`T=0` | `static_check_ev032.txt`（常量 0）、`capture_analysis.txt` | 记录 `a=3984..3988` 事实；未回填、未遮光/照射 | 分离度与阈值数值仍待需求/架构接受 | 上游给出接受记录与被动双态证据后，由固件实现回填 `T` 并重建；测试侧再复验 `V=1 D=0/1` |
| F-3 | 真实升温/超温触发与跨链路 | 暗 + 升温 >0.9 ℃ 或 >35.0 ℃ 时 `r=1`、`P` 冻结、`s=1`，手机同笔一致 | 全部周期 `r=0 s=0 y=0`，无 `P`、无发送 | 同上 | 未施加刺激（用户禁止）；本能力无 `android_test` 入口 | 网关/手机链路是否可用（未观测） | 按用户安排的后续联调阶段执行；不伪造发送成功/失败 |
| F-4 | T4-03 失败路径 | `q=0` 后紧跟 `G`，失败零副作用、下周期重探/恢复 | 本轮 5/5 `q=1`、0 条 `G`；未触发 | `capture_analysis.txt` | 增加采集窗口至 k=18 | 更长运行/不同供电条件下的失败率（未观测） | 出现真实 `q=0` 时按 `G` 原始行取证；不人为制造失败或改判据 |
| F-5 | E2E-04 功耗 | CR2032 采样峰值/深睡电流实测值与条件 | 未执行（已批准延期） | `tool_results.txt` §4（无仪器枚举） | 枚举 USB 设备 | 是否存在未登记测量入口（无） | 仪器具备时由硬件测试测量并记录量程/接线/电池电压 |

## 8. 证据局限（如实登记）

1. 串口窗口之间存在未采集间隙（cap01/cap02 间 9.2 s，cap02/cap03 间 2 min 34 s）；k=12 的 `S` 行未取得，位级事务与之的对应为**推断**（`p(k=15)=268` 与换算值相等）。
2. 上电五行与 `uid` 未取得；镜像身份由构建摘要 + `Verify OK` 闭合，未由 `BOOT` 行交叉核对。
3. LIGHT-DEF-2 **启用后**行为只在宿主 `T=3000` 变体验证；板上保持 `T=0`，未取得 `V=1`/`D=1` 实板证据。
4. 无升温/超温、无网关/手机同窗、无功耗仪器；相应项未执行，不写通过。
5. `a=3984..3988` 近满量程与“当前有光”陈述的极性解释仍属现场/硬件未决项；本报告只记录事实，不据此判器件好坏。
6. 位级 I²C 为数字采样（4 MS/s、250 ns 分辨率），不证明模拟幅度/上升沿；名义时间不能跨设备同步。
7. as-built 摘要 `a3b08774…` 未在本轮回退源码重演；当前交付摘要 `b6a15fed…` 已独立重建确认。

## 9. 本轮实际改动（只增测试侧资产与证据）

- 新增：`evidence/ev032/`（原始 `.cap`×3、原始 `.sr`、解码、分析/静态脚本与输出、工具结果、宿主日志汇总）
- 更新：测试侧 harness 5 个文件（见 §6）；`evidence/test.md`（本段置顶；EV-030 及更早保留）、`evidence/programmed_prototype.md`（EV-032 下载身份置顶）、`file_manifest.txt`
- **未修改**：`USER/**` 产品源码、Keil/IAR 工程、网关、Android、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载软件或驱动

---
