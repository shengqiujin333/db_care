# 嵌入式验证证据（EV-039 / 当前固件 FD-002r8 独立实板验证）

状态：`embedded_tester.embedded_verification`（action `firmware_check`）本轮独立验证。
被测对象：`artifacts/firmware_implementation.md`（FWI-004 rev 9，产品源提交 `82a7e74`，本轮无代码改动）。
测试方案：`artifacts/test_design.md`（TD-002 rev 9.0）。
负责角色：嵌入式测试。
原始证据：`evidence/ev039/`（`tool_results.txt`、`capture_windows.txt`、`capture_analysis.txt`、`bot01_run.txt`、`static_check_ev039.txt`、`host_build_test.log`、`host_harness_run.stdout`、`mdk_build_flash.log`、原始 `.cap`）。

## 0. 结论

**`TEST_PASS`（仅限 TD-002 rev 9.0 当前适用范围：ITEM-001/002/003/004/006）**。

- 当前构建独立重建、下载并重新取得实板证据，身份与本轮源码/发布包逐字节一致；未发现实测固件功能缺陷。
- 适用任务的全部 acceptance 均执行且期望一致：构建/宿主/静态 → 通过；COM43 被动观测（4 个连续周期）→ 通过；上电五行自动编排 → 通过。
- **明示未执行（不写成通过）**：`FW-LIGHT-DEF2-ENABLE` 为人工 `SKIP_CONFIRMED` / `APPROVED_DEFERRED`（NOT EXECUTED，本轮不重测）；`E2E-04` 功耗为已批准 `DEFERABLE`（NOT EXECUTED，无仪器）；真实升温/超温、改变光强、跨链路、平台写入/回读为阶段外项（未执行）。
- **可选 L3 位级 I²C 未取得**：配置入口 `fx2lafw:conn=2.32` 本轮 `No devices found`；未猜测新地址、未做全局扫描、未改配置。该项不是必需用例，也不替代任何必需证据。

阶段通过 ≠ 完整产品通过：本结论不覆盖上述未执行项。

## 1. 范围复核（operator：重评原检查者范围错误）

| # | 原报告范围错误 | 本轮处理 |
|---|---|---|
| E-1 | 把人工已确认延期的 `FW-LIGHT-DEF2-ENABLE` 与“实现未满足”混同 | 上报公式实现按宿主/静态证据判定；无光判据启用单列 NOT EXECUTED（人工延期）；真实升温/超温单列 NOT EXECUTED（外部条件） |
| E-2 | 以“需求 4 无光分支仍禁用”作为整体 TEST_FAIL/返工依据 | 该人工延期项不作为本轮返工依据，也不计入本阶段退出门槛；本轮不据此改固件 |
| E-3 | 未区分“后续入口”与“本轮不返工” | 保留为需求/接受侧未决事项；固件本轮不返工 |
| E-4 | R4 标签与 N-1 披露互相矛盾 | 以 N-1 的 `SKIP_CONFIRMED` 披露为准；`T=0`、`V=0 D=0` 只作为“判据未启用等价性”，不解释为“已识别有光” |

BOT-01 的 `REASSESS` 已落实为测试侧自有脚本的自动方式（见 §4），未要求固件新增测试接口；正式发布包经复核只含 3 个生产成员，无测试代码。

## 2. 身份与部署（本轮实际执行）

| 项 | 原始结果 |
|---|---|
| 验证时 HEAD | `9ce380cfb7fc5779d85df1f41db5f59a673e622d`；接手时 `git status` 干净 |
| 产品固件源 | `git diff --stat 82a7e74..HEAD -- <sensor>/{USER,MDK,EWARM,UM2005C,COMMON}` **为空** |
| 发布包 | `fw_release/fw_sensor_FD-002r8.zip`：134,791 B、md5 `5df0c6d4…`；恰 3 成员（axf `b6a15fed…` 297,380 B；hex `4d728dec…` 75,275 B；`PACKAGE.txt` `a58f67fc…` 3,202 B），无 harness/mock/测试开关 |
| 独立重建 | 06:27:00.469–06:27:03.205Z，ARMCLANG V6.24；**0 Error / 1 Warning**（既有 `main.c(254)` 空循环体）；`Code=25964 RO=656 RW=116 ZI=1676` |
| 产物身份 | axf **`b6a15fed2ed5e3d0c4adc51ac87a4830`**、hex **`4d728dec9b0d28a352c46436b7586c72`**；与发布包成员逐字节一致 |
| 下载 | `mdk_flash {}` 06:28:51.227–06:29:04.466Z：`Erase Done.Programming Done.Verify OK.Application running ...`；下载镜像即上述 axf |
| 串口/探针 | COM43 = Prolific `067B:2303` LOCATION=1-9.4.2（9600 8N1）；CW-DAPLink `C251:F001` SN `87094109484987710672FF50`；FX2 `04B4:8613` 在位但连接地址未解析（§6） |
| 禁止项 | 未调温、未改变光强、未遮光/照射、未测功耗、未操作网关/手机；未改产品源码、工程文件、配置或验收要求 |

原始：`evidence/ev039/tool_results.txt`、`mdk_build_flash.log`。

## 3. 构建、宿主与静态（ASB / RPT / MCH）

| 用例组 | 结果 | 证据 |
|---|---|---|
| ASB-01 实现侧宿主套件 | **222/0**（48+61+54+28+31），exit 0 | `host_build_test.log` |
| ASB-03/RPT-01..15 测试侧独立 harness | **609/0**，`RESULT: FAILED=[]`（fw_core 59、gxht40 93、sf_i2c 51、diag_line 38、diag_probe 109、measure_flow 79、rf_frame 10、rf_report 30、rtc 17、gw_compat 11、vdd 31、light T=0 40、light T=3000 41） | `host_harness_run.stdout` 与各组日志 |
| ASB-02/MCH-07 构建 | Keil rebuild 0E/1W；摘要与包成员一致 | `mdk_build_flash.log` |
| ASB-08/MCH-06/09/10/RPT-15 静态 | **21/21**：`LIGHT_DARK_THRESHOLD_CODE=0`、旧 1/3 宏无残留、判据无状态、升温方向与严格 >35.0℃、3 tick 采样、10 B 空口帧、上电顺序 `BOOT<VDD<IOTEST<IOSIG<BUS>`、UART 关闭、S 行字段顺序、引脚单点、无浮点/动态内存、无 hall/optcfg/params/history 活动引用、单点 TX 守卫 | `static_check_ev039.txt` |
| RPT-13 `SENSOR_DEBUG_UART=0` | 8 个 USER 翻译单元全部 exit 0；`debug_trace.o` 无定义符号；`measure.o`/`main.o`/`debug_trace.o` 不引用 UART 符号 | `build_debug_off.log` |
| 工程纯净性 | `MDK/Project.uvprojx` 源列表无 `test`/`mock` 路径 | 本轮 Grep 原文（见 `tool_results.txt` §2） |

判定：T1 升温/严格边界、T2 冻结/重试/不覆盖/P 行、T3 有界等待、T4/F-1 驱动与 `p/H` 语义、LIGHT-DEF-2 形制与 `T=0` 等价性均在上述宿主/静态证据范围内通过。宿主结论标 `[L0]`，不冒充实板结论。

## 4. 实板上电五行（BOT-01..06，自动编排）

端口先开（06:29:22.377Z），于窗口内执行 `pyocd 0.36.0 reset -t cortex_m -M attach -m hw`（06:29:23.386Z，rc=0，仅复位、不写 Flash/内存），同一窗口取得 271 B：

| 顺序 | 原始行（逐字节见 `bot01_hw.cap`） |
|---|---|
| 1 | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` |
| 2 | `VDD ok=1 code=1480 bgrmv=1189 mv=3289` |
| 3 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=0` |
| 4 | `IOSIG scl=95555 sda=30300` |
| 5 | `BUS idle=3 scl=1 sda=1 ack=44` |
| — | `S k=0 p=0 H=0 V=0 o=8 n=3280 x=3293 a=3284 D=0 t=319 h=321 q=1 r=0 s=0 y=0` |

判读：五行各一次且相邻有序；`4095*1189//1480 = 3289` MATCH；`gc=0`（0x44/0x45 应答，未做 general call 复位）；`BUS ack=44` 与 `IOSIG` ACK 时隙（`sda=30300`）一致；`BOOT uid=6A002C00` 与本板历史同一；首样本 `H=0 p=0` 符合“无前值”。

`pyocd` 仍未登记在 `runtime/environment.yaml`；本轮仅按用户补充条件 2 把它作为外部复位源使用，未安装软件、未改配置，建议配置所有者补登记 reset 入口。正式包只含 3 个成员，故 BOT-07 的“测试接口”需求为**无**。

## 5. COM43 被动观测（OBS-01..09）

窗口与间隙见 `capture_windows.txt`；独立解析 `analyze_ev039.py` → `capture_analysis.txt` **104 项断言 0 失败**。

| k | p | H | V | o | n | x | a | D | t (0.1℃) | h (0.1%RH) | q | r | s | y | 行长 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | 0 | 0 | 0 | 8 | 3280 | 3293 | 3284 | 0 | 319 | 321 | 1 | 0 | 0 | 0 | 76 B |
| 3 | 319 | 1 | 0 | 8 | 3426 | 3440 | 3432 | 0 | 312 | 328 | 1 | 0 | 0 | 0 | 78 B |
| 9 | 308 | 1 | 0 | 8 | 3280 | 3294 | 3284 | 0 | 303 | 341 | 1 | 0 | 0 | 0 | 78 B |
| 12 | 303 | 1 | 0 | 8 | 3175 | 3191 | 3182 | 0 | 303 | 342 | 1 | 0 | 0 | 0 | 79 B |
| 15 | 303 | 1 | 0 | 8 | 3199 | 3210 | 3203 | 0 | 297 | 345 | 1 | 0 | 0 | 0 | 79 B |
| 18 | 297 | 1 | 0 | 8 | 3191 | 3205 | 3196 | 0 | 294 | 351 | 1 | 0 | 0 | 0 | 79 B |

- **连续周期**：k=9/12/15/18 四个连续机会，步长 3；另有 k=0/3 连续。窗口间工具周转间隙 2 min 14.5 s 使 k=6 未被观测，已如实记录，不是节拍异常。
- **字段**：全部 `q=1`、`o=8`、`t∈[-400,1250]`、`h∈[0,1000]`、`n<=a<=x`、`r=s=y=0`、`V=0 D=0`；无 `G`（无失败周期）、无 `P`（未触发上报）。
- **格式/节拍**：每行含 CRLF 27..79 B ≤ 96 B；完整 CRLF；无 NUL/杂散；同一 k 不重复；采样间期无输出。
- **前值链**：`p(k=3)=t(k=0)=319`、`p(k=12)=t(k=9)=303`、`p(k=15)=t(k=12)=303`、`p(k=18)=t(k=15)=297`；首样本 `H=0 p=0`，后续 `H=1`。
- **判据状态**：交付默认 `T=0` ⇒ `V=0 D=0`，这是判据未启用的等价性证据，**不是“已识别有光”**。本轮如实记录 `a=3182..3432`（`n/x` 跨度 ≤16，无饱和）；不据此判定光敏器件或光路好坏。

## 6. 可选 L3 位级 I²C（未执行，非必需）

`sigrok_probe`（06:48:13.087–06:48:14.451Z，exit 1）对配置入口 `fx2lafw:conn=2.32` 返回 `No devices found.`。USB 枚举仍见 Cypress FX2 身份 `04B4:8613`（“Unknown Device #1”），但其当前总线地址未被核实。按项目约束未做全局扫描、未猜测新地址、未修改环境配置。该项是 TD-002 §4.1 的**可选补充**，不替代任何必需证据；本轮无需据此判定器件或功能失败。建议配置所有者重新核验 FX2 实际连接地址。

## 7. 明示未执行与延期（不得写成通过）

| 项 | 状态 | 依据/原因 |
|---|---|---|
| `FW-LIGHT-DEF2-ENABLE`（EN-01..07，判据数值接受后启用） | **NOT EXECUTED（人工 `SKIP_CONFIRMED` / `APPROVED_DEFERRED`，2026-11-11T03:50:42Z）** | 人工已确认“无法准备，先跳过”；本轮不重测、不重排、不自行回填阈值；`T` 保持 0；不阻止本阶段退出，但必须披露 |
| `E2E-04` CR2032 采样峰值/深睡电流 | **NOT EXECUTED（已批准 `DEFERABLE`）** | 本次缺少功耗测量仪器；交付报告列明未测 |
| 真实升温 >0.9℃ 触发、真实 >35.0℃ 触发 | NOT EXECUTED（阶段外） | 用户明确当前不能调节温度；不安排相关硬件测试 |
| 改变光强/遮光/照射双态 | NOT EXECUTED（阶段外） | 用户明确当前不能改变光强；与 `FW-LIGHT-DEF2-ENABLE` 的人工延期一致 |
| 网关→手机→平台跨链路、433 实收、平台 `/upload_data` 写入/回读 | NOT EXECUTED（后续联调阶段） | 用户补充条件 4；本阶段不安排 |
| 可选 L3 位级 I²C | NOT EXECUTED | §6，配置入口未解析；非必需 |

## 8. 失败与交接

本轮未发现实测固件功能缺陷（`failure_cause` 不适用）。若后续重新验证，须重新构建并重新取得实板证据，不得沿用本报告的摘要或捕获。剩余未闭合项均属人工延期、外部条件或后续阶段，不是本阶段固件返工依据。

## 9. 证据索引

| 文件 | 内容 |
|---|---|
| `evidence/ev039/tool_results.txt` | 端口/USB/sigrok 枚举、构建/下载身份、宿主与静态计数、BOT 与 COM43 工具原始返回、pyocd 参数 |
| `evidence/ev039/capture_windows.txt` | 4 个捕获窗口起止、字节数、md5、观测 k、窗口间隙 |
| `evidence/ev039/capture_analysis.txt` | 独立解析 104 项断言（行格式/字段/节拍/p 链/无杂散） |
| `evidence/ev039/bot01_hw.cap` | BOT-01 原始 271 B（五行 + S k=0） |
| `evidence/ev039/cap01_k3.cap` / `cap02_k9_k12.cap` / `cap03_k15_k18.cap` | COM43 被动原始字节 |
| `evidence/ev039/bot01_run.txt` | 端口先开/复位时间/逐块接收原始日志 |
| `evidence/ev039/mdk_build_flash.log` | 重建与下载原始日志（Verify OK） |
| `evidence/ev039/host_build_test.log` / `host_harness_run.stdout` | 实现侧 222/0、测试侧 609/0 |
| `evidence/ev039/static_check_ev039.txt` / `build_debug_off.log` | 静态 21/21、`SENSOR_DEBUG_UART=0` 构建与符号检查 |
| `evidence/ev039/analyze_ev039.py` / `capture_with_reset.py` / `run_host_harnesses.sh` / `build_debug_off.sh` / `static_check_ev039.py` | 可复跑测试资产（测试侧，不入发布包） |

---

