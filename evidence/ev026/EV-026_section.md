# 嵌入式验证证据（EV-026 / E1 `delay_ms()` 修复轮复验）

状态：`embedded_tester.embedded_verification` 独立验证证据（本轮**独立全量 rebuild、两次下载当前交付提交、在真实目标上重新连续采集约 57 分钟原始字节流、并做一次位级 I²C 被动采集**；EV-018/021/022/023/024/025 只作对照，不作本轮结果）
输入：`runtime.artifacts.delivered.g003.implementation` = `artifacts/firmware_implementation.md`（FWI-002 **rev 6.3** / E1 修复轮）；测试方案 = `artifacts/test_design.md`（TD-002 rev 6.0，`verification_scope=complete_delivery`）
负责角色：嵌入式测试

**结论：`TEST_FAIL`（整体完整交付仍未通过；E1 修复目标已在实板上成立）**

1. **E1 修复在真实目标上成立（本轮核心新增事实）**：交付镜像中 `delay()` 不再是 `bx lr`；位级实测命令 STOP→读 START = **32.525 ms**（EV-025 修前为 **0.622 ms**，配置 20 ms），SCL 位周期 **614.5 µs**（≈1.627 kHz，D2 慢时钟保留）；一次完整事务 `0x44` 写 `0xFD` + 读 6 字节 `65 BF 32 55 22 F5` 的双字 CRC-8 独立复算**均匹配**。
2. **修复后 CRC 无回归**：第二个下载会话内取得 **15 个 `S` 行（k=0…57，全部 3 的倍数）**，加逻辑分析仪解出的 1 个周期，共 **16 个采样周期全部 `q=1`、0 条 `G`**；`t=203..241`（20.3–24.1 ℃）、`h=375..477`（37.5–47.7 %RH），均在有效域。T4-05（连续 ≥3 周期 `q=1`）成立。
3. **T4-02 上电五行仍不完整（证据缺口）**：本运行时 `serial_capture` 与 `mdk_flash` 严格串行，串口会话在 `Application running` 后约 **68 ms** 才建立；本轮只捕获到 `BUS idle=3 scl=1 sda=1 ack=44` 与随后 `S k=0`，`BOOT/VDD/IOTEST/IOSIG` 四行在端口打开前已输出完毕。**不得用后续周期行替代**；板件 `uid` 未取得。
4. **T5 仍未执行**：无完全遮光窗口；`LIGHT_DARK_CALIBRATED=0`，全部周期 `V=0 D=0 o=8`（未标定门禁成立）。本轮 `delay` 修复使光敏稳定等待从 0 恢复到约 160 ms，**EV-024 的光照数值不得用于标定**。
5. **INT 上报/跨链路仍未执行**：无遮光/加温刺激，全部周期 `r=0 s=0 y=0`、无 `P`、无发送；网关/手机同笔读数未观测。
6. **新登记一项既有观测缺陷（非本轮引入，不影响业务判定）**：`sensor_trace_fetch()` 在 `temperature_process()` 推进前值**之后**读取 `s_prev_temp_x10/s_have_prev`，因此 `S` 行的 `p/H` 是**本周期更新后**的值；上电首条 `S k=0` 实测 `H=1 p=241`，无法从轨迹观察 TD-002 INT-01 要求的“首样本 `H=0`”。宿主 harness 已证明判定本身使用更新前的前值。
7. **宿主证据全部通过**：实现侧 `test/build_test.sh` **214 项 0 失败**；测试侧独立 harness **493 项 0 失败**（§5）。两个陈旧测试资产（`host_rf_report_verify_ev.c`、`host_light_verify_ev.c`）仍为旧 API，未计入证据。

局部通过项（不改变整体结论）：T4-05 实板成功判据、T4-07 位级判别、T4-03/08、T5-01 未标定门禁、INT-01 的 `k` 节拍、INT-02/10、T1/T2/T3 宿主回归、BLD-01/02/03（下载身份）。

---

## 1. 被测身份、构建与部署（本轮实际执行）

| 项 | 原始结果 |
|---|---|
| 受测提交 | `d871957`（`[role:firmware_engineer]`；接手时 `git status` 干净）。`git diff --stat HEAD -- <FW>/USER` = 空 ⇒ 产品源码逐字节等于 HEAD |
| 本轮产品源码增量（相对上一受测 `3113a96`） | `git diff --stat 3113a96 HEAD -- <FW>/USER` = **仅 `USER/src/main.c`（+15/−6）**：`delay()` 计数变量加 `volatile`、参数改 `uint32_t`；**未改** `sensor_config.h`（`SENSOR_I2C_DELAY_LOOPS=300`、`GXHT40_MEASURE_WAIT_MS=20`、`LIGHT_DARK_CALIBRATED=0`、`LIGHT_DARK_REF_CODE=0` 均保持）、CRC/换算/量程/重试/命令/帧格式 |
| 独立构建 | `mdk_build {"action":"rebuild"}`（09:43:37–09:43:39Z）→ **0 Error / 1 Warning**（既有 `main.c(254) while(k--);` 空循环体）；`Program Size: Code=25940 RO-data=656 RW-data=116 ZI-data=1676` |
| 产物身份 | `Project.axf` md5 **`917bffe63ba0a4945a20a9a51f4a60df`**、`Project.hex` md5 `5b017c84d693ce17318fca633b27ad07` —— 与实现 FWI-002 rev 6.3 / BUILD-002 rev 6.3 声明逐字符一致，并由本能力 rebuild **确定性重建** |
| 部署 | `mdk_flash {}` ×2：**09:46:29–09:46:41Z** 与 **09:47:13–09:47:25Z**，两次日志均为 `Erase Done.Programming Done.Verify OK.Application running ...`。所有实板串口证据取自第二次下载后的同一会话。**`Verify OK` 只证明下载校验，不表示功能合格** |
| 调试串口 | `serial_list_ports`（09:43:30Z）：`COM43 = Prolific USB-to-Serial 067B:2303` 在位（9600 8N1）；`COM26 = C251:F001`（调试器 CDC，未使用） |
| 下载探针 / 逻辑分析仪 / 手机 | `usb_list_devices`（10:46:10Z）：CW-DAPLink `C251:F001`、FX2 `04B4:8613`（`fx2lafw` 已绑定、Status OK）、手机 `18D1:4EE7` SN `12F046690506` |

---

## 2. E1 修复的位级复核（本轮核心）

原始：`evidence/ev026/decoded_i2c.txt`、`capture_metadata.json`、`decode_metadata.json`、`delay_disasm.txt`、`nm_defined.txt`；独立复算：`evidence/ev026/analyze_ev026.py` → `capture_analysis.txt`。

采集：`sigrok_capture`（09:49:05–09:49:56Z，exit 0），`fx2lafw:conn=2.27`，D0=SCL/D1=SDA，触发 `D1=f`，**8,000,000 样点 @ 4 MS/s（2.0 s）**；原始 `.sr` md5 `a450e3969392cf75a854749b90a2e345`（`evidence/ev026/raw_i2c_capture.sr`），离线 `sigrok_decode`（`i2c:scl=D0:sda=D1`）。

| 测量项 | 本轮（修复镜像 `917bffe6`） | EV-025（修前镜像 `e56d8d4b`）对照 |
|---|---|---|
| 交付 axf 中 `delay` | `0x4788`：`SUB sp / CMP r0,#0 / MOVS r1,#0xC7 / LSLS #3 / STR/LDR [sp] / SUBS / STR / CMP / BNE / SUBS r0 / BNE / ADD sp / BX lr` | 单条 `bx lr`（空实现） |
| 命令 STOP → 读 START | **32.525 ms**（130,100 样点 × 0.25 µs） | **0.622 ms** |
| 写事务 START→STOP | 12.049 ms | 12.046 ms |
| 读事务 START→STOP | 40.162 ms | 40.151 ms |
| SCL 全位周期 | **614.5 µs**（2458 样点为众数，38/71；次众数 2457）→ **≈1.627 kHz** | 614.3 µs |
| 事务内容 | `0x44` 写 `0xFD` → ACK → STOP；`0x44` 读 6 字节 `65 BF 32 55 22 F5` → NACK → STOP | 同 |

独立 CRC/换算（`analyze_ev026.py`，poly 0x31/init 0xFF，不引用实现常量）：

- 温度字 `0x65BF`：CRC-8 计算 **`0x32` = 收到 `0x32` ✅ → 24.6 ℃**；
- 湿度字 `0x5522`：CRC-8 计算 **`0xF5` = 收到 `0xF5` ✅ → 35.6 %RH**。

判读：
1. E1 目标（“等待不可被优化消除、按配置施加”）**已在实板上成立**：STOP→读 START 从 0.622 ms 恢复到 32.5 ms 量级，且读事务仍完整、双 CRC 有效。
2. **D2 慢时钟保留**：SCL 位周期与 EV-025 在 ±0.2 µs 内一致（614.5 vs 614.3 µs），说明本轮只恢复了等待，没有改变读时钟速率。
3. **观察（不判失败）**：实测 32.5 ms 约为配置 20 ms 的 **1.63×**。`delay()` 加 `volatile` 后每毫秒循环变慢，原 `796*2` 标定不再等于 1 ms。当前效果是“等待更长”，仍远大于 GXHT40 `tMEAS.H max 8.3 ms`、`tPU max 1 ms`，**不构成功能失败**；若后续需要精确毫秒，应由实现侧重新标定并在实板复核（不在本能力修改产品源码）。

**本轮未执行**：第二次独立位级采集（单事务样本，数字采样，无模拟幅度/上升沿形状；不能据此排除总线电气因素）。

---

## 3. 长时串口观测（COM43 @ 9600 8N1）

原始：`evidence/ev026/cap00_preflash_old_image.cap` … `cap10_k54_k57.cap`；窗口、间隙与字节数见 `capture_windows.txt`；逐行解析、行长与统计见 `capture_analysis.txt`。

| 项 | 结果 |
|---|---|
| 下载前旧镜像（对照） | `cap00`（09:43:58–09:46:29Z）1 行：`S k=117 … q=1` |
| 修复镜像会话 | 第二次下载（09:47:25Z）后 `k` 从 0 重新开始；观测到 `k=0, 6, 9, 12, 18, 21, 24, 27, 30, 33, 45, 48, 51, 54, 57`，共 **15 条 `S`** |
| 位级周期 | 逻辑分析仪解出 1 个完整采样事务（09:49:54Z，对应 `k=3`）⇒ 合计 **16 个有证据的采样周期** |
| `q` / `G` | **`q=1` 16/16；`G` 行 0 条**（温度 CRC 历史失败签名未复现） |
| `t` | **203..241**（20.3–24.1 ℃），全部在 −400..1250 有效域 |
| `h` | **375..477**（37.5–47.7 %RH），全部在 0..1000 有效域 |
| `V / D / o` | 全部 `V=0 D=0 o=8`（未标定门禁 + 8/8 光照采样成功） |
| `r / s / y` | 全部 `r=0 s=0 y=0`；无 `P`、无发送（无触发条件） |
| `k` 增量 | 同一窗口内相邻 `S` 均为 **+3**；观测到的较大间隔都对应捕获窗口空隙（`k=3/15/36/39/42` 未落入窗口），不是跳拍 |
| 行长 / 间隙 | `S` 78–79 B、`BUS` 31 B（含 CRLF，均 ≤96 B）；每个捕获文件都是完整 CRLF 行序列，**无杂散字节**；采样间期无输出 |
| 帧完整性 | `p` 等于本周期更新后的温度（见 §6 F-1），`H` 恒为 1 |

**节拍（INT-01）**：由窗口边界可推得，第二个下载会话首样本在 09:47:26.3 ± 0.1 s；`k=15` 只能落在 `10:01:33.89–10:01:58.34` 的 24.4 s 窗口空隙内 ⇒ 前 5 个周期的平均周期 **169.5–174.4 s**；逻辑分析仪把 `k=3` 定在 09:49:54.3（首个间隔 **148.0 s**）；其后各窗口与 **≈176–178 s** 的稳态周期一致。`k` 增量恒为 3，采样周期即 3 个 RTC 分钟拍。**判定**：节拍量级（≈3 分钟）成立、`k` 节拍正确；但首个“调试器复位→首采样后”的间隔明显短于稳态，属 RTC 相位/复位方式造成的观测现象，如实登记（见 §6 F-2）。未用 180 s 名义值改写实测。

---

## 4. 上电诊断行（T4-02）与身份

| 项 | 结果 |
|---|---|
| 捕获窗口与复位 | 第二次 `mdk_flash` 于 09:47:25.96Z 结束（`Application running`）；`serial_capture` 于 09:47:26.03Z 开始（**+68 ms**），持续 60 s |
| 实得行 | `BUS idle=3 scl=1 sda=1 ack=44`（31 B）→ `S k=0 … q=1`（78 B） |
| 未得行 | `BOOT` / `VDD` / `IOTEST` / `IOSIG` —— 在端口打开前已输出（复位到端口打开仅 68 ms；`BUS` 的 112 地址扫描在端口打开后才完成） |
| 判读 | `BUS ack=44` 证明固件确实执行了上电诊断且 GXHT40 在 `0x44` 应答；`S k=0 q=1` 证明新镜像在目标上运行并立即完成首采样。**五行序列不完整，T4-02 记为未满足**；不得用 `BUS` 或后续 `S` 替代 `BOOT/VDD/IOTEST/IOSIG` |
| 横幅 `fw/uid` | **未取得**（同上）；构建/下载身份由 commit + axf/hex md5 + `Verify OK` 闭合，缺横幅交叉核对 |
| 已尝试措施 | ① 先开捕获再下载：因后端严格串行，捕获在下载前结束；② 先下载再捕获：捕获在 `Application running` 后 68 ms 建立，仅够看到 `BUS`。未修改配置、未换用其他串口或下载入口 |

---

## 5. 宿主证据（L0/L1）

原始日志：`evidence/ev026/host_*.log`、`host_harness_summary.txt`、`host_build_test.log`。

| 资产 | 结果 |
|---|---|
| 实现侧 `test/build_test.sh` | **46 + 57 + 54 + 28 + 29 = 214 项 0 失败**，退出码 0 |
| `host_fw_core_verify_ev` | **59/59**（升升温/严格边界/1-3 判据穷举/参考哈希） |
| `host_gxht40_verify_ev` | **93/93**（位级 mock 总线/从机、重探、总线恢复、**命令白名单 47 次全部 `0xFD`、无 `0x94`**） |
| `host_sf_i2c_verify_ev` | **51/51**（位时序、ACK/NACK、有界恢复无地址字节） |
| `host_diag_line_verify_ev` | **38/38**（`BUS/S/G/P` 逐字节、逐行 ≤96 B、关闭语义） |
| `host_diag_probe_verify_ev` | **104/104**（地址扫描/general call/诊断快照/失败语义） |
| `host_measure_flow_verify_ev` | **79/79**（采样顺序、失败不推进、冻结快照、重试/用尽、`P` 取数） |
| `host_rf_frame_verify_ev` | **10/10**（10 B 布局/字节序/Feistel/篡改拒绝） |
| `host_gw_compat_verify_ev` | **11/11**（传感器→网关→BLE→Android 映射） |
| `host_rtc_cadence_verify_ev` | **17/17**（3 拍一次机会、1 min × 3） |
| `host_vdd_verify_ev` | **31/31**（`4095*bgrmv/code`、超时不伪造、测后清理） |
| 合计（测试侧独立 harness） | **493 项 0 失败** |
| 陈旧资产（不计入） | `host_rf_report_verify_ev.c`（仍为旧 `bool light_sample()` API，编译失败）；`host_light_verify_ev.c`（旧 mock 头，历史已登记不可编译） |

> 宿主结论只证明逻辑/流程与格式未回归；**不证明**实板时序、失败率或跨链路。

---

## 6. 本轮新登记观测（交实现调查，非本轮修复引入）

**F-1（诊断字段语义偏差，既有）**：`USER/src/measure.c::sensor_trace_fetch()` 在 `temperature_process()` 已执行 `s_prev_temp_x10 = tempvalue; s_have_prev = true;` **之后**读取前值，导致 `S` 行的 `p/H` 是“本周期更新后”的值。实板首条 `S k=0` 为 `H=1 p=241`，无法满足 TD-002 INT-01 的“首样本 `H=0`”观察要求。判定逻辑本身使用更新前前值（`host_measure_flow_verify_ev` 79/79 覆盖），故**不影响上报业务**；但外部无法由 `S` 行复核判定所用前值。最小下一步（实现）：在推进前值前缓存 `prev/have` 供 `sensor_trace_fetch()` 使用，或由测试设计明确改字段语义；不得用放宽判据解决。

**F-2（节拍观测）**：首个“复位后→首采样之后”的采样间隔实测 148.0 s，而后续窗口与 ≈176–178 s 稳态一致、`k` 增量恒为 3。与 RTC/LSI 相位及调试器复位方式有关；本轮未改 RTC 配置。最小下一步：若需精确 180 s 或首拍行为，由实现/设计确认 RTC 复位语义与 LSI 容差；本能力不以名义值替代实测。

**F-3（E1 附带标定）**：`delay()` 恢复后实测 ≈1.63× 名义毫秒（32.5 ms vs 20 ms），方向安全（等待更长）；若需要精确延时，由实现重新标定 `796*2` 并在实板复核。

---

## 7. 验收追踪（TD-002 rev 6.0；逐项判定）

| 用例 | 结果 | 依据/说明 |
|---|---|---|
| T1-01..03、T1-06、T1-07 | **通过（宿主）** | `build_test.sh` 46/57、`host_fw_core_verify_ev` 59/59；本轮仅改 `delay()`，未触碰判定 |
| T1-04 常量/源码语义 | **通过（静态）** | `SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350`；无 DROP/降温/`cur==350` |
| T1-05 验证资产边界 | **通过** | 本轮未修改产品源码；测试资产仅新增 `evidence/ev026/*` |
| T2-01..06、T2-09、T2-10 | **通过（宿主）** | `host_measure_flow_verify_ev` 79/79（冻结/重试/用尽/P 行取数） |
| T2-07 失败周期零副作用 | **通过（宿主）** | 同上（真实实板本轮无 `q=0`，失败路径未在板上触发） |
| T2-08 `SENSOR_DEBUG_UART=0` | **通过（构建）** | `main.c` 修改不涉及调试宏；既有构建路径未改 |
| T3-01..07 | **通过（宿主）** | `host_debug_trace_check` 54/54（实现侧）+ `host_diag_line_verify_ev` 38/38 |
| **T4-01 构建/下载身份** | **部分通过** | rebuild 0 Error、axf md5 一致、两次 `Verify OK`；**横幅 `fw/uid` 未取得** |
| **T4-02 上电五行** | **未满足** | 仅 `BUS` + `S k=0`；缺 `BOOT/VDD/IOTEST/IOSIG`（工具串行，§4） |
| T4-03 窗口/S-G 配对/行长 | **通过** | 15 条 `S`、0 条 `G`、78–79 B、无杂散字节；窗口与间隙见 `capture_windows.txt` |
| T4-04 失败环节判别 | **不适用（本轮无失败）** | 16/16 `q=1`；历史温度 CRC 签名未复现；无 `G` 可判别 |
| **T4-05 连续 ≥3 周期成功** | **通过** | 15 条实板 `S` 全部 `q=1`，`t/h` 有效且与室温一致，`k` 增量 3 |
| T4-06 失败语义/下周期重探 | **未在板上触发** | 无 `q=0`；宿主 `host_gxht40_verify_ev` 93/93、`host_diag_probe_verify_ev` 104/104 覆盖 |
| **T4-07 位级补充判别** | **通过（本轮实测）** | `fx2lafw:2.27` probe exit 0、capture/decode 成功；SCL 614.5 µs、双 CRC 匹配 |
| T4-08 修复边界 | **通过（静态+回归）** | 相对 `3113a96` 仅 `main.c delay()`；CRC/换算/量程/重试/速率/帧格式未变，93/93 + 79/79 回归 |
| T4-09 硬件分支证据 | **本轮不涉及** | 无 `q=0`，无硬件分支证据需求 |
| **T4-10 触发周期跨链路** | **未执行** | 全部 `r=0`，无上报 |
| T4-11 宿主/构建证据 | **通过** | §5 + `mdk_build` 0 Error |
| **T5-01 未标定门禁** | **通过** | 全部 `V=0 D=0 o=8` |
| **T5-02/03/04/07/08/10 全暗标定与三态** | **未执行** | 无完全遮光窗口；未回填 `C_dark`；EV-024 旧光照值不得复用 |
| T5-05/06/09（回填/判据/ADC 异常） | **判据与异常路径通过（宿主）；回填未执行** | `build_test` 28/29、`host_vdd` 31/31；`LIGHT_DARK_CALIBRATED=0` 未变 |
| **INT-01 节拍/启动采样** | **部分通过** | `k` 增量 3、量级 ≈3 min；**首样本 `H=0` 因 F-1 不可观察**；首间隔 148 s 见 F-2 |
| INT-02 间期零输出/关闭/可再次下载 | **通过** | 300 s 窗口内采样间期无字节；两次 `mdk_flash` 均 `Verify OK`；后续周期仍 `q=1 o=8` |
| **INT-03/04/05/06 上报判定** | **未执行** | 无遮光/加温；全部 `r=0` |
| **INT-07 P 行** | **未执行** | 无触发/发送 |
| **INT-08 失败周期不伪造** | **未在板上触发** | 无 `q=0`；宿主覆盖 |
| **INT-09 跨链路** | **未执行** | 无上报；网关/手机未同窗观测 |
| INT-10 S 行兼容/预算 | **通过** | 78–79 B、字段顺序未变、无 P |
| INT-11 当前构建身份 | **部分通过** | commit + md5 + 下载校验闭合；横幅缺失 |
| BLD-01/02/03 | **通过** | 宿主 214 项；rebuild 0 Error；两次 `Verify OK` |
| BLD-04/05/06 | **通过（静态/回归）** | 命令白名单 47×`0xFD`、无 `0x94`；引脚/无 RTOS/UART 不用中断未改；产品源码零改动 |
| REG-01/02/03/05/07 | **通过（宿主）** | 59/79/38/10/11 等回归；无旧语义 |

---

## 8. 未测项、已尝试措施与最小下一步

| # | 未满足项 | 性质 | 已尝试 / 现状 | 最小下一步（责任） |
|---|---|---|---|---|
| G1 | T4-02 上电五行 + 横幅 UID | **采集编排缺口（工具串行）** | 两种调用顺序均已实测；最短在 `Application running` 后 68 ms 才建立串口会话 | 需要能并发/预开串口的采集入口，或在下载前保持串口会话（测试运行编排）；不得用历史 `uid` 或后续行替代 |
| G2 | T5-02/03/04/07/08/10 全暗标定与三态 | **物理条件未满足** | 当前 `LIGHT_DARK_CALIBRATED=0`；15 周期 `V=0 D=0 o=8` 门禁正常 | 用户提供完全遮光窗口（≥2 个采样周期）；先记录 `o/n/x/a`，由实现回填 `C_dark` 并置 1、重建；再用本修复镜像复测三态。**不得复用 EV-024 光照值** |
| G3 | INT-03/04/05/06/07/09 上报与跨链路 | **刺激/联测条件未满足** | 全部 `r=0 s=0`、无 `P`、无发送；网关/手机在线状态未业务验证 | 遮光 + ≥1.0 ℃ 升温（或稳定 >35 ℃）+ 网关/手机同窗观测；按 TD-002 INT 判据执行 |
| G4 | T4-06/INT-08 实板失败路径 | **未触发** | 16/16 `q=1`，无 `q=0` | 若自然出现 `q=0`，按 `S/G` 记录并复算；不得人为制造失败 |
| G5 | F-1 轨迹前值语义 | **既有观测缺陷（低风险）** | 首条 `S k=0 H=1 p=241`；判定逻辑已由宿主证明使用更新前前值 | 实现侧缓存更新前 `prev/have` 供轨迹使用，或设计侧明确语义；不影响当前业务 |
| G6 | F-2 首间隔 148 s | **观测现象** | `k` 增量恒为 3；稳态 ≈176–178 s；首个间隔 148 s | 设计/实现确认 RTC 复位语义与 LSI 容差；不影响 `q=1` |
| G7 | E1 毫秒标定 1.63× | **安全方向偏差** | 32.5 ms vs 配置 20 ms；tMEAS/tPU/稳定等待均更长 | 如需精确毫秒，实现重新标定 `796*2` 并实板复核 |

**尚未排除的假设**：H3 总线电气（数字采样无法排除模拟因素）；H5 器件型号/装配（当前数据不支持，无目视/换件证据）；`delay()` 恢复后长时间稳定性（本轮 57 分钟、16 周期无失败，未覆盖电池老化/温度循环）。**已排除/削弱**：当前 `TEST_FAIL` 由 GXHT40 温度 CRC 缺陷导致（16/16 `q=1` + 位级双 CRC）；`delay_ms()` 仍为空实现（反汇编 + 32.5 ms 实测）；逻辑分析仪入口不可达（`conn=2.27` probe exit 0）。

---

## 9. 本轮实际改动（只增测试侧证据）

- 新增：`evidence/ev026/` —— `cap00_preflash_old_image.cap` … `cap10_k54_k57.cap`、`raw_i2c_capture.sr`、`capture_metadata.json`、`decode_metadata.json`、`decoded_i2c.txt`、`Project.axf`、`flash_1.log`、`flash_2.log`、`md5_list.txt`
- 新增：`evidence/ev026/analyze_ev026.py`、`capture_analysis.txt`、`capture_windows.txt`、`tool_results.txt`
- 新增：`evidence/ev026/disasm_text.txt`、`delay_disasm.txt`、`nm_defined.txt`（交付 axf 反汇编与符号）
- 新增：`evidence/ev026/run_host_harnesses.sh`、`host_harness_summary.txt`、`host_build_test.log`、`host_*_verify_ev.log`、`host_*_build.log`
- 更新：`evidence/test.md`（本段置顶）、`evidence/programmed_prototype.md`（EV-026 下载身份置顶）、`file_manifest.txt`
- **未修改**：`USER/**` 产品源码、Keil/IAR 工程、网关、Android、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载软件或驱动

---

## 10. 结论

- **E1 修复已在实板验证**：`delay()` 不再是空实现；命令 STOP→读 START 32.525 ms（修前 0.622 ms）；SCL 614.5 µs（D2 保留）；位级双 CRC 匹配。
- **修复未引入 CRC 回归**：第二个下载会话 15 条 `S`（k=0…57）+ 位级 1 周期，共 16 个周期全部 `q=1`、0 条 `G`；T4-05 成立。
- **整体仍 `TEST_FAIL`**：T4-02 上电五行/横幅（工具串行缺口）、T5 全暗标定（无遮光窗口）、INT-03..07/09（无刺激/联测）未执行或未满足；`p/H` 轨迹语义与首间隔节拍作为观测项交实现/设计。
- **不宣告**器件损坏、固件不可修复或外部阻塞；上述缺口均有明确的原始证据、已尝试措施与最小下一步。
