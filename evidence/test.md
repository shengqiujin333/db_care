# 嵌入式实板验证证据（EV-040 / 当前发布候选 FW-REL-001 rev 3 独立复核）

负责角色：嵌入式测试
被测对象：`fw_release/fw_sensor_FD-002r8.zip`（zip 134,791 B / md5 `5df0c6d4…`；发布记录 `artifacts/firmware_release.md` FW-REL-001 rev 3）
本轮方式：独立重建 → 用工程保存配置下载 → 在真实传感器板上采集 UART1 调试输出与采样节拍
原始证据：`evidence/ev040/`（`tool_results.txt`、`ev040_boot_passive.cap/.log`、`ev040_reboot_check.cap/.log`、两份分析输出）

## 0. 结论

**`HIL_PASS`（仅限本次有效范围：发布包身份 + 当前板上固件的上电诊断、UART1 调试输出、采样节拍与交付默认上报状态）。**

- 发布包身份、独立重建产物、实际下载镜像三者逐字节一致（axf `b6a15fed…`、hex `4d728dec…`）。
- `mdk_flash {}` 下载日志含 `Erase Done.Programming Done.Verify OK.Application running ...`；下载后在同一串口窗口取得完整上电五行与 `S k=0`。
- 连续观测到 4 个采样机会（`k=0/3/6/9`，步长 3），稳定节拍 179.66 s / 179.59 s；全部 `q=1`、`V=0 D=0`、`r=s=y=0`，无 `G`/`P` 行、无杂散输出。
- 第二次复位复现同一上电五行与首样本 `H=0 p=0`，表现为可重复的复位恢复路径。
- **明示未执行（不写成通过）**：`FW-LIGHT-DEF2-ENABLE` 为人工 `SKIP_CONFIRMED` / `APPROVED_DEFERRED`；真实升温/超温触发、改变光强、网关/手机/平台跨链路、功耗测量均为未执行项（见 §6）。
- 本结论不等于产品整体验收通过。产品级门禁按既有记录（`FR-006`）仍为未对齐，不属本次复核范围。

## 1. 范围复核（按操作员恢复指令）

| # | 原检查者口径 | 本轮核对与处理 |
|---|---|---|
| E-1 | 将“无光判据启用（`T`）”与“上报判定公式实现”混同 | 公式实现按既有宿主/静态证据归为“已实现且一致”；判据启用单列 NOT EXECUTED（人工延期），两者不互相代替 |
| E-2 | 把人工已确认跳过的 `FW-LIGHT-DEF2-ENABLE` 作为整体失败与返工依据 | 该项为人工 `APPROVED_DEFERRED` / `SKIP_CONFIRMED`（2026-11-11T03:50:42Z，「无法准备，先跳过」）。**本轮不作为返工依据、不重测、不重复询问**，仅按原状披露 |
| E-3 | 未区分“后续阶段项”与“本阶段未执行” | 网关/手机/平台链路属后续联调阶段；真实升温/改变光强属用户明确不能提供的条件；功耗属已批准 `DEFERABLE`。均单列未执行，不写成通过 |
| E-4 | 上电诊断缺少可重复的自动测试方式 | 按操作员 `REASSESS`，由测试侧自有脚本编排“先开串口再 HW 复位”，本轮两次执行成功，**未要求固件新增测试接口** |

已确认跳过的测项仅从本次交付的必需测试中排除；其余有效目标继续实测，结果见下。

## 2. 身份、构建与下载（本轮实际执行）

| 项 | 结果 | 证据 |
|---|---|---|
| 仓库状态 | HEAD `4657076`；开工时 `git status --porcelain` 为空 | `tool_results.txt` §1 |
| 产品固件源 | `git diff 82a7e74..HEAD` 对 `<sensor>/{USER,MDK,EWARM,UM2005C,COMMON}` **为空** | 同上 |
| 发布包 | 134,791 B、md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`；zip CRC OK；恰 3 个成员 | §2 |
| 包成员 | axf `b6a15fed2ed5e3d0c4adc51ac87a4830`（297,380 B）；hex `4d728dec9b0d28a352c46436b7586c72`（75,275 B）；`PACKAGE.txt` `a58f67fc…`（3,202 B） | §2 |
| 独立重建 | 07:08:38.268–07:08:40.599Z，ARMCLANG V6.24，**0 Error / 1 Warning**（既有 `main.c(254)` 空循环体）；`Code=25964 RO=656 RW=116 ZI=1676` | §3 |
| 重建产物身份 | axf/hex 摘要与包内成员**逐字节一致** | §3 |
| 下载 | `mdk_flash {}` 07:08:49.267–07:09:02.194Z：`Erase Done.Programming Done.Verify OK.Application running ...` | §4 |
| 端口/探针 | COM43 = Prolific `067B:2303` LOCATION=1-9.4.2（9600 8N1）；CW-DAPLink `C251:F001` SN `87094109484987710672FF50` | §5 |
| 禁止项 | 未调温、未改变光强、未遮光/照射、未测功耗；未操作网关/手机/平台；未改产品源码、工程文件、配置或验收要求 | — |

## 3. 上电诊断与 UART1 调试输出（自动编排，两次）

测试侧脚本先打开 COM43，再通过 CW-DAPLink 执行 `pyocd reset -t cortex_m -M attach -m hw`（仅复位，不写 Flash/内存），同一窗口连续采集。

| 顺序 | 原始行（上电五行两次复位逐字节一致；S 行取 run 1 首样本） |
|---|---|
| 1 | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` |
| 2 | `VDD ok=1 code=1480 bgrmv=1189 mv=3289` |
| 3 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=0` |
| 4 | `IOSIG scl=95555 sda=30300` |
| 5 | `BUS idle=3 scl=1 sda=1 ack=44` |
| — | `S k=0 p=0 H=0 V=0 o=8 n=3258 x=3270 a=3263 D=0 t=316 h=322 q=1 r=0 s=0 y=0`（run 1） |

判定要点（脚本断言，run 1 38/38、run 2 33/33 通过）：

- 五行各一次且顺序为 `BOOT < VDD < IOTEST < IOSIG < BUS`；`BOOT uart=9600`。
- `4095*1189//1480 = 3289` 与 `VDD mv=3289` 一致；`ok=1`。
- `gc=0`（未用 general call 复位器件）；`BUS ack=44` 与 GXHT40 地址一致。
- 首样本 `H=0 p=0`；此后 `H=1`，`p` 链与上一条 `t` 一致。
- 未发现调试串口初始化失败、乱码、NUL 或超长行（最长 78 B ≤ 96 B）。

## 4. COM43 被动观测（节拍与上报状态）

窗口：07:09:34.040Z 开串口 → 07:09:36.233Z 复位 → 07:19:19.261Z 结束（505 B，13 个接收块）。原始字节 `ev040_boot_passive.cap`（md5 `581c2792…`）。

| k | p | H | V | o | n | x | a | D | t (0.1℃) | h (0.1%RH) | q | r | s | y |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | 0 | 0 | 0 | 8 | 3258 | 3270 | 3263 | 0 | 316 | 322 | 1 | 0 | 0 | 0 |
| 3 | 316 | 1 | 0 | 8 | 3268 | 3278 | 3272 | 0 | 310 | 331 | 1 | 0 | 0 | 0 |
| 6 | 310 | 1 | 0 | 8 | 3305 | 3316 | 3309 | 0 | 309 | 333 | 1 | 0 | 0 | 0 |
| 9 | 309 | 1 | 0 | 8 | 3330 | 3340 | 3335 | 0 | 304 | 339 | 1 | 0 | 0 | 0 |

- **连续机会**：`k=0/3/6/9` 四个，步长均为 3；同一次开窗内无缺口。
- **节拍**（脚本按接收时间戳计算）：上电首样本 → 首个调度样本 **165.515 s**，随后 **179.656 s / 179.594 s**。稳态间隔为 3 分钟量级且无漏采；首间隔短于 180 s 的现象在同一窗口内只出现一次，后续保持稳定。本轮未对 RTC 计时相位做额外注入验证，故只报告观测值，不把解释写成已验证结论；该项不作缺陷判定。
- **字段约束**：全部 `q=1`、`o=8`、`t∈[-400,1250]`、`h∈[0,1000]`、`n<=a<=x`、`r=s=y=0`。
- **上报状态**：交付默认 `T=0` ⇒ `V=0 D=0`；这是“判据未启用”的等价性证据，**不是“已识别有光”**，也不构成对光敏器件或光路好坏的判定。
- **异常/恢复**：第二次 HW 复位（07:20:52.905Z）后再次取得完整五行与 `S k=0 p=0`，证明复位恢复路径可重复。
- **静默性**：采样间期无输出，无 `G`（周期失败）与 `P`（上报）行，无重复 `k`。

## 5. 可选 L3 位级 I²C

`sigrok_probe`（07:21:43.035Z）对配置入口 `fx2lafw:conn=2.32` 返回 `No devices found.`；USB 枚举仍见 Cypress FX2 `04B4:8613`（“Unknown Device #1”），但当前总线地址未核实。按项目约束未做全局扫描、未猜测地址、未改配置。该项是可选补充证据，**NOT EXECUTED**，不影响本轮必需判定；建议配置所有者重新核验 FX2 实际连接地址。

## 6. 明示未执行与延期（不得写成通过）

| 项 | 状态 | 依据/原因 |
|---|---|---|
| `FW-LIGHT-DEF2-ENABLE`（无光判据数值接受后启用） | **NOT EXECUTED（人工 `SKIP_CONFIRMED` / `APPROVED_DEFERRED`，2026-11-11T03:50:42Z）** | 人工已确认“无法准备，先跳过”；本轮不重测、不重排、不自行回填阈值；`T` 保持 0，仅披露 |
| `E2E-04` CR2032 峰值/深睡电流 | **NOT EXECUTED（已批准 `DEFERABLE`）** | 无电流表/SMU/示波器；用户明确本轮不测功耗 |
| 真实升温 >0.9℃ 触发、真实 >35.0℃ 触发 | NOT EXECUTED | 用户明确当前不能调节温度 |
| 改变光强/遮光/照射双态 | NOT EXECUTED | 用户明确当前不能改变光强 |
| 网关 → 手机 → 平台跨链路、433 实收、平台 `/upload_data` 写入/回读 | NOT EXECUTED（后续联调阶段） | 用户补充条件 4；本阶段不安排 |
| 可选 L3 位级 I²C | NOT EXECUTED | §5，配置入口未解析；非必需 |
| `SENSOR_DEBUG_UART=0` 正式量产变体 | NOT EXECUTED（本轮） | 交付包为工程默认调试开构建；该变体的编译/符号检查属另一项，未在本轮重跑 |

## 7. 失败与交接

本轮未发现实测固件功能缺陷。未闭合项均属人工延期、用户不可提供的条件、已批准延期或后续阶段，不是本阶段固件返工依据。

环境建议（非本能力修改）：`pyocd 0.36.0` 已被测试侧用于 HW 复位，但未登记在 `runtime/environment.yaml`；建议配置所有者评估登记为受控复位入口。上电诊断不需要固件新增测试接口。

## 8. 证据索引

| 文件 | 内容 |
|---|---|
| `evidence/ev040/tool_results.txt` | 端口/USB、包身份、构建/下载日志、pyocd 参数、捕获窗口与摘要、可选 sigrok 结果 |
| `evidence/ev040/ev040_boot_passive.cap` / `.log` | 本轮 505 B 原始串口字节与逐块时间戳 |
| `evidence/ev040/ev040_reboot_check.cap` / `.log` | 第二次复位 271 B 原始串口字节与时间戳 |
| `evidence/ev040/capture_analysis.txt` | 独立解析 38 项断言（0 失败） |
| `evidence/ev040/reboot_check_analysis.txt` | 复位复现 33 项断言（0 失败） |
| `evidence/ev040/boot_and_passive_capture.py` / `analyze_ev040.py` | 可复跑测试资产（测试侧，不入发布包） |

---

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

# 嵌入式联调验证证据（EV-038 / 当前发布包 FD-002r8 rev 2 独立重建、下载与实板 HIL）

状态：`embedded_tester.embedded_integration_verification`（action `firmware_package_check`）本轮独立 HIL。被测对象为 `artifacts/firmware_release.md`（FW-REL-001 rev 2）与 `fw_release/fw_sensor_FD-002r8.zip`。rev 2 为同一包的整包自查与证据重绑（包字节未变）；本轮仍重新核对包身份、独立重建、重新下载到真实目标，并在板上重新采集启动、周期与位级 I²C，不以旧下载记录作为本轮判据。
输入：`runtime.artifacts.firmware_package` = `artifacts/firmware_release.md`（rev 2）+ `fw_release/fw_sensor_FD-002r8.zip`；测试方案 = `artifacts/test_design.md`（TD-002 rev 8.0）。
负责角色：嵌入式测试
原始证据：`evidence/ev038/`（`capture_windows.txt`、`tool_results.txt`、`capture_analysis.txt`、`pyocd_check.txt`、`pyocd_reset_boot.txt`、`static_check_ev038.txt`、原始 `.cap`/`.sr`/`decoded_i2c.txt`）。

**结论：`HIL_PASS`（仅限本报告已执行且通过的范围；未发现已测固件功能缺陷）**

- **包身份（独立复算）**：zip 134,791 B、md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`、`testzip=None`；恰 3 个成员，MD5 与 rev 2 声明一致；无 harness/mock/测试开关。
- **重建身份**：本轮 `mdk_build {"action":"rebuild"}` 0 Error / 1 既有 warning，Code=25964 RO=656 RW=116 ZI=1676；axf `b6a15fed2ed5e3d0c4adc51ac87a4830`、hex `4d728dec9b0d28a352c46436b7586c72`，与包内成员逐字节一致。
- **下载**：`mdk_flash {}` 日志含 `Erase Done.Programming Done.Verify OK.Application running ...`（`Verify OK` 仅证明回读校验）；下载前后 axf/hex 摘要不变。
- **上电五行**：端口先开后以 CW-DAPLink + `pyocd` HW reset（rc=0），同一窗口取得 `BOOT→VDD→IOTEST→IOSIG→BUS` 各一次且有序，随后 `S k=0`；`VDD` 独立复算 MATCH；`BUS ack=44` 与 0x44 应答自洽。
- **连续运行**：同一镜像、同一启动会话取得 `k=0,3,6,9,12,15,21` 共 **7 条 S 行**；全部 `q=1`、`V=0`、`D=0`、`o=8`、字段在有效域、行 ≤96 B、CRLF 完整、间期无杂散、无 `G`/`P`；`p` 链逐周期等于上一周期 `t`（首样本 `H=0 p=0`）。
- **板级通信**：fx2lafw 被动 I²C 解码 `0x44` 写 `0xFD`、读 `68 43 8D 59 81 EE`；温度/湿度双 CRC-8 独立复算均 MATCH；换算 263（26.3 ℃）/ 377（37.7 %RH）；`p(k=21)=263` 与该温度相等，故该事务对应 k=18 采样（k=18 未出现在 COM43 窗口，不作为串口行声明）。
- **宿主与静态**：实现侧 `test/build_test.sh` 222/0；测试侧 13 组独立 harness 609/0；静态核查 21/21。
- **未执行/延期（明示，不计为通过）**：`FW-LIGHT-DEF2-ENABLE` 经人工确认 **SKIP_CONFIRMED（2026-11-11T03:50:42Z，「无法准备，先跳过」）**，本轮不重测；`E2E-04` 功耗为已批准 DEFERABLE 且无仪器；真实升温/超温与改变光强受用户约束不安排；网关/手机跨链路与 433 空口接收按用户补充条件 4 属后续联调阶段；未施加物理断线/断总线的故障注入。

---

## 1. 身份、包与部署（本轮实际执行）

| 项 | 原始结果 |
|---|---|
| 验证时 HEAD | `15f70a5d4b9570817ce4e71320730d2728be6fdf`（接手时工作树干净） |
| 产品固件源 | 最近触及 `<sensor>/USER MDK EWARM gcc` 的提交 = `82a7e74`；`git diff --stat 82a7e74 HEAD --` 同路径 **为空** |
| 发布包 | `fw_release/fw_sensor_FD-002r8.zip`：134,791 B、md5 `5df0c6d4…`；`testzip=None`；恰 3 成员（axf `b6a15fed…` 297,380 B；hex `4d728dec…` 75,275 B；`PACKAGE.txt` `a58f67fc…` 3,202 B） |
| 独立重建 | 11:59:40.689–11:59:42.640Z，ARMCLANG V6.24，**0 Error / 1 Warning**（既有 `main.c(254)` 空循环体）；`Code=25964 RO=656 RW=116 ZI=1676` |
| 产物身份 | axf **`b6a15fed2ed5e3d0c4adc51ac87a4830`**、hex **`4d728dec9b0d28a352c46436b7586c72`**；与包成员逐字节一致 |
| 下载 | `mdk_flash {}` 11:59:59.554–12:00:11.910Z：`Verify OK`、`Application running`；下载后摘要不变 |
| 调试串口 | COM43 Prolific `067B:2303` LOCATION=1-9.4.2，9600 8N1；COM26 为调试器 CDC，未使用 |
| 探针 | CW-DAPLink `C251:F001` SN `87094109484987710672FF50`；FX2 `04B4:8613` 在位 |
| 复位编排 | 测试侧脚本先开 COM43，2 s 后 `pyocd 0.36.0 reset -t cortex_m -M attach -m hw`，12:00:22.932–12:00:23.645Z rc=0；pyocd 仅作外部复位源，未写 Flash/内存 |
| 范围声明 | `Verify OK` 只证明回读校验；本轮未调温、未改变光强、未测功耗、未烧录网关 |

原始：`evidence/ev038/tool_results.txt`（含 HEAD/源码零差异/包与成员 md5/构建与下载身份）、`capture_windows.txt`、`pyocd_check.txt`、`pyocd_reset_boot.txt`。

## 2. 上电五行与首样本（BOT-01/02/03/04）

原始：`bot01_hw.cap`（271 B，md5 `a5cc0252…`）。复位发生在端口已打开的窗口内（12:00:22.932Z），首行于 t+2.765 s 到达。

| 顺序 | 原始行 |
|---|---|
| 1 | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` |
| 2 | `VDD ok=1 code=1481 bgrmv=1189 mv=3287` |
| 3 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=0` |
| 4 | `IOSIG scl=95555 sda=30300` |
| 5 | `BUS idle=3 scl=1 sda=1 ack=44` |
| — | `S k=0 p=0 H=0 V=0 o=8 n=3920 x=3926 a=3923 D=0 t=269 h=364 q=1 r=0 s=0 y=0` |

判读：五行各一次且有序；`4095*1189//1481 = 3287` MATCH；`ack=44` 为 0x44 地址应答；`gc=0`；首样本 `p=0 H=0` 符合“无前值”。

**BOT-04 复核**：`pyocd --version` = 0.36.0；`pyocd list --targets --name CW32` 为空表；`pyocd list` 显示 CW-DAPLINK 探针但 `Target=n/a`。因此复位使用内置 `cortex_m` 目标、attach 模式，仅作复位源；pyocd 仍未登记在 `runtime/environment.yaml`，本轮未安装软件、未改配置（建议配置所有者登记 reset 入口）。

**BOT-01 自动测试方式（按重新评估要求）**：由测试侧自有脚本完成“端口先开 + 后台 pyocd HW reset”，未新增或修改固件测试接口/测试宏；正式发布包仅含生产产物，不需要实现补充测试接口。

## 3. COM43 被动观测（OBS）

原始窗口与字节见 `capture_windows.txt`；独立解析 `analyze_ev038.py` → `capture_analysis.txt`（129 项断言 0 失败）。

| tick | 原始 S 行（字段顺序冻结） |
|---:|---|
| 0 | `S k=0 p=0 H=0 V=0 o=8 n=3920 x=3926 a=3923 D=0 t=269 h=364 q=1 r=0 s=0 y=0` |
| 3 | `S k=3 p=269 H=1 V=0 o=8 n=3919 x=3925 a=3922 D=0 t=264 h=371 q=1 r=0 s=0 y=0` |
| 6 | `S k=6 p=264 H=1 V=0 o=8 n=3919 x=3923 a=3920 D=0 t=267 h=367 q=1 r=0 s=0 y=0` |
| 9 | `S k=9 p=267 H=1 V=0 o=8 n=3919 x=3923 a=3921 D=0 t=263 h=374 q=1 r=0 s=0 y=0` |
| 12 | `S k=12 p=263 H=1 V=0 o=8 n=3918 x=3924 a=3920 D=0 t=262 h=375 q=1 r=0 s=0 y=0` |
| 15 | `S k=15 p=262 H=1 V=0 o=8 n=3917 x=3922 a=3919 D=0 t=262 h=376 q=1 r=0 s=0 y=0` |
| 21 | `S k=21 p=263 H=1 V=0 o=8 n=3917 x=3923 a=3919 D=0 t=265 h=375 q=1 r=0 s=0 y=0` |

判读：
1. 7 条不重复 S 行，`k` 递增且相邻差为 3 的倍数（k=15→21 的间隔内 k=18 未落入 COM43 窗口）。
2. 全部 S 行 76–79 B（含 CRLF）≤96；15 字段顺序冻结；CRLF 完整、无 NUL、无杂散；无 `G`/`P`。
3. 全部 `q=1 o=8`；`t∈[-400,1250]`、`h∈[0,1000]`、`a∈[n,x]`；`r=s=y=0`。
4. `V=0 D=0` 为 `LIGHT_DARK_THRESHOLD_CODE=0` 的等价性证据；**不据此判“有光/无光”或光路好坏**。
5. `p` 链：k=3←269、6←264、9←267、12←263、15←262；k=21 的 `p=263` 与 §4 位级温度配对（对应 k=18）。

## 4. 位级 I²C（补充，不替代串口行）

- 采集：`fx2lafw:conn=2.32`、D0=SCL/D1=SDA、D1 下降沿触发、8,000,000 样点 @4 MS/s（2.0 s），会话 12:16:16.536–12:17:58.983Z，触发约 12:17:56.9Z；`.sr` md5 `d4c712ad…`。
- 解码：`Start / Address write 44 / ACK / Data write FD / ACK / Stop / Start / Address read 44 / ACK / Data read 68 43 8D 59 81 EE / NACK / Stop`。
- 独立复算：温度字 `0x6843` CRC `0x8D` MATCH；湿度字 `0x5981` CRC `0xEE` MATCH；按项目声明的 x10 换算得 **263（26.3 ℃）/ 377（37.7 %RH）**。
- 配对：k=15 与 k=21 之间唯一采样机会为 k=18；`p(k=21)=263` 与解码温度相等，故该事务对应 k=18。
- 边界：数字采样只证明逻辑电平与采样分辨率内时序，不证明模拟幅度/上升沿/上拉质量；`.sr` 与串口非跨设备同步。

## 5. 宿主、静态与发布包边界

| 入口 | 结果 |
|---|---|
| 实现侧 `test/build_test.sh` | 5 阶段 48+61+54+28+31 = **222 通过 / 0 失败** |
| 测试侧独立 harness（13 组） | **609 通过 / 0 失败**（含 light.c T=0 40 与 T=3000 启用变体 41） |
| `static_check_ev037.py` | **21/21 PASS**（T=0 默认、无旧 1/3 宏、判据形制、升温方向与严格 >35.0、3 ticks、10 B 帧、上电顺序、S 字段顺序、有界 TX、无浮点/动态内存、无旧霍尔/光通讯） |
| 发布包纯净性 | 仅 axf/hex/PACKAGE.txt 三成员；无 harness/mock/测试开关 |
| 产品源码改动 | 本轮未修改产品源码、工程、网关、Android、权限/环境配置 |

## 6. 验收目标 → 本轮结果

| 目标 | 结果 | 依据 |
|---|---|---|
| FW-REG-ASBUILT | **通过（本任务范围）** | 独立 rebuild = 发布包成员；源码零差异；0E/1W、资源摘要一致 |
| FW-REG-REPORT | **通过（本任务范围）** | fw_core/measure_flow/rf_report/rtc 宿主向量 609/0；实板 `p/H` 与 `q` 语义一致 |
| FW-LIGHT-DEF2-MECH | **通过（本任务范围）** | 静态 21/21；实板 T=0 下 7 周期 `V=0 D=0` 等价性 |
| FW-OBS-PASSIVE | **通过** | 7 周期 `q=1`；节拍/字段/行长/无杂散/无 G/P 全部满足 |
| FW-DIAG-BOOT | **通过** | 端口先开 + HW reset；五行各一次且有序；VDD 可复算；BUS/IOSIG/S 自洽 |
| 发布包 ↔ 实际烧录 | **通过** | 包成员 = 独立重建 = 下载镜像；`Verify OK` |
| FW-LIGHT-DEF2-ENABLE | **未执行（人工 `SKIP_CONFIRMED`）** | EN-01/EN-02 前置未具备；2026-11-11T03:50:42Z「无法准备，先跳过」；T 保持 0；不是通过 |
| 真实升温/超温、光强变化 | **未执行（用户约束）** | 用户明确不能调温、不能改变光强；未施加任何刺激 |
| E2E-04 功耗 | **延期（已批准）** | 无电流表/SMU/示波器；未测并在此列明 |
| 网关/手机跨链路、433 空口接收 | **不在本阶段** | 用户补充条件 4：后续联调阶段 |
| 物理断线/断总线故障注入 | **未执行（无授权刺激）** | 重探/有界总线恢复仅由宿主 harness 覆盖（gxht40 93/0、sf_i2c 51/0），不声明为实板故障注入 |

**人工确认与批准延期（明示）**：`FW-LIGHT-DEF2-ENABLE` = SKIP_CONFIRMED（未执行，不重测）；`E2E-04` = APPROVED_DEFERRED（未测）。上列未执行项不得删除、不得解释为通过。

## 7. 边界与限制

1. 本结论只覆盖本轮实际执行的实板范围；`V=0 D=0` 是判据未启用的等价性证据，不是“有光/无光”判定，也不能证明光路分离度可用。
2. 未执行任何调温、改变光强、遮光/照射、拆装外壳或功耗测量；未操作网关与手机。
3. 数字串口行与逻辑采样分别证明协议字段与逻辑电平；不能据此推断模拟幅度、上升沿形状或供电余量。
4. `pyocd` 未在统一环境登记，本轮按 Project Goal 补充条件 2 复用既有安装与探针；建议配置所有者登记 reset 入口。
5. 本轮未消费 Android 候选包与服务上下文：按用户补充条件 4，网关/手机跨链路属后续联调阶段。板端串口/逻辑数据**不代表应用已收到数据**，跨端观察尚未执行，也不作为本阶段通过依据。

---

# 嵌入式验证证据（EV-037 / 当前固件独立实板验证：身份、上电五行、7 周期被动观测、位级 I²C）

状态：`embedded_tester.embedded_verification`（action `firmware_check`）本轮独立验证。产品固件源自 `82a7e74` 起未变；本轮仍重新构建、下载并在真实板重新采集，不复用 EV-036 的实板结论。
输入：`runtime.artifacts.firmware_current` = `artifacts/firmware_implementation.md`（FWI-003 rev 8）；测试方案 = `artifacts/test_design.md`（TD-002 rev 8.0）。
负责角色：嵌入式测试
原始证据：`evidence/ev037/`（`capture_windows.txt`、`tool_results.txt`、`capture_analysis.txt`、`static_check_ev037.txt`、`package_purity.txt`、原始 `.bin`/`.sr`/`decoded_i2c.txt`）。

**结论：`TEST_PASS`（仅限本报告已执行且通过的范围；未发现已测固件功能缺陷）**

- 构建/下载身份：本轮独立 `mdk_build {"action":"rebuild"}` 0 Error / 1 既有 warning，Code=25964 RO=656 RW=116 ZI=1676；axf `b6a15fed2ed5e3d0c4adc51ac87a4830`、hex `4d728dec9b0d28a352c46436b7586c72`；`mdk_flash {}` 日志 `Erase Done.Programming Done.Verify OK.Application running ...`。发布包 `fw_sensor_FD-002r8.zip`（md5 `5df0c6d4…`，3 成员）与重建逐字节一致，无测试资产。
- 上电五行（BOT-01/02/03/04）：端口先开后 `pyocd` HW reset，同一窗口取得 `BOOT→VDD→IOTEST→IOSIG→BUS` 各一次且有序，随后 `S k=0`；`VDD` 独立复算 MATCH；`BUS ack=44` 与 `IOSIG`/`q=1` 自洽。
- 长窗口被动观测（OBS）：同一镜像、同一启动会话取得 k=0、3、6、9、12、15、21 共 **7 个周期**；全部 `q=1`、`V=0`、`D=0`、`o=8`、字段在有效域、行 ≤96 B、无杂散、无 `G`/`P`；`p` 链逐周期等于上一周期 `t`（首样本 `H=0 p=0`）。
- 位级 I²C（可选补充）：`0x44` 写 `0xFD`、读 `68 09 6B 59 5C A6`；温/湿度双字 CRC-8 独立复算均 MATCH，换算 26.1 ℃ / 34.9 %RH；`p(k=21)=261` 与该温度相等，故该事务对应 k=18 采样（跨窗口按值配对）。
- 宿主与静态：实现侧 `test/build_test.sh` 222/0；测试侧 13 组独立 harness 609/0；静态核查 21/21。
- **未执行/延期（明示，不计为通过）**：`FW-LIGHT-DEF2-ENABLE` 经人工确认 **SKIP_CONFIRMED（2026-11-11T03:50:42Z）**，本轮不重测；`E2E-04` 功耗为已批准 DEFERABLE 且无仪器；真实升温/超温与改变光强受用户约束不安排；网关/手机跨链路按用户条件 4 属后续联调阶段。

---

## 1. 身份与部署（本轮实际执行）

| 项 | 原始结果 |
|---|---|
| 验证时 HEAD | `d5701f55b456e5fe07440c8dc4ec4ed206fa626e`（工作树干净） |
| 产品固件源 | 最近触及 `<sensor>/USER MDK EWARM` = `82a7e74`；`git diff 82a7e74..HEAD` 对该路径为空（仅 `dengbei_care` 有 Android 变更，不属本能力范围） |
| 独立重建 | 11:27:06.523–11:27:08.526Z，ARMCLANG V6.24，0E/1W（既有 `main.c(254)`） |
| 产物 | axf `b6a15fed…`（297,380 B）、hex `4d728dec…`（75,275 B）；与发布包成员逐字节一致 |
| 下载 | `mdk_flash {}` 11:27:45.931–11:27:58.287Z：`Verify OK`、`Application running`；下载后 axf/hex md5 未变 |
| 调试串口 | COM43 Prolific `067B:2303` LOCATION=1-9.4.2，9600 8N1；COM26 为调试器 CDC，未使用 |
| 探针 | CW-DAPLink `C251:F001` SN `87094109484987710672FF50`；FX2 `04B4:8613` 在位 |
| 复位编排 | 测试侧脚本先开 COM43，延时 2 s 后 `pyocd 0.36.0 reset -M attach -m hw`，11:28:12.015–11:28:12.757Z rc=0；未经 pyocd 写 Flash/内存 |
| 范围声明 | `Verify OK` 只证明回读校验；本轮未调温、未改变光强、未测功耗 |

原始：`evidence/ev037/image_identity.txt`、`mdk_flash.log`、`pyocd_reset_boot.txt`、`tool_results.txt`。

## 2. 上电五行与首样本（BOT-01/02/03/04）

原始：`bot01_hw.cap`（271 B，md5 `83fc268c…`）。

| 顺序 | 原始行 |
|---|---|
| 1 | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` |
| 2 | `VDD ok=1 code=1481 bgrmv=1189 mv=3287` |
| 3 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=0` |
| 4 | `IOSIG scl=95555 sda=30300` |
| 5 | `BUS idle=3 scl=1 sda=1 ack=44` |
| — | `S k=0 p=0 H=0 V=0 o=8 n=3914 x=3922 a=3918 D=0 t=271 h=360 q=1 r=0 s=0 y=0` |

判读：五行各一次且有序；`4095*1189//1481 = 3287` MATCH；`ack=44` 为 0x44 地址应答；`gc=0`；首样本 `p=0 H=0` 符合“无前值”。

**BOT-01 自动测试方式（按重新评估要求）**：由测试侧自有脚本完成“端口先开 + 后台 pyocd HW reset”，未新增或修改固件测试接口/测试宏；正式发布包仅含生产产物（见 §5），不需要实现补充测试接口。

## 3. COM43 被动观测（OBS）

原始窗口与字节见 `capture_windows.txt`；独立解析 `analyze_ev037.py` → `capture_analysis.txt`（166 项断言 0 失败）。

| tick | 原始 S 行（字段顺序冻结） |
|---:|---|
| 0 | `S k=0 p=0 H=0 V=0 o=8 n=3914 x=3922 a=3918 D=0 t=271 h=360 q=1 r=0 s=0 y=0` |
| 3 | `S k=3 p=271 H=1 V=0 o=8 n=3915 x=3921 a=3918 D=0 t=268 h=364 q=1 r=0 s=0 y=0` |
| 6 | `S k=6 p=268 H=1 V=0 o=8 n=3920 x=3924 a=3921 D=0 t=263 h=374 q=1 r=0 s=0 y=0` |
| 9 | `S k=9 p=263 H=1 V=0 o=8 n=3920 x=3925 a=3922 D=0 t=265 h=372 q=1 r=0 s=0 y=0` |
| 12 | `S k=12 p=265 H=1 V=0 o=8 n=3919 x=3925 a=3922 D=0 t=266 h=370 q=1 r=0 s=0 y=0` |
| 15 | `S k=15 p=266 H=1 V=0 o=8 n=3918 x=3925 a=3922 D=0 t=262 h=378 q=1 r=0 s=0 y=0` |
| 21 | `S k=21 p=261 H=1 V=0 o=8 n=3921 x=3925 a=3922 D=0 t=262 h=376 q=1 r=0 s=0 y=0` |

判读：
1. 7 周期连续或按 3 的倍数递进（k=0/3/6/9/12/15 连续，k=18 未直接抓到，k=21 接续）；同一 k 无重复。
2. 全部行 76–79 B（含 CRLF）≤96；15 字段顺序冻结；CRLF 完整、无 NUL、无杂散；无 `G`/`P`。
3. 全部 `q=1 o=8`；`t∈[-400,1250]`、`h∈[0,1000]`、`a∈[n,x]`；`r=s=y=0`。
4. `V=0 D=0` 为 `LIGHT_DEF_THRESHOLD_CODE=0` 的等价性证据；**不据此判“有光/无光”或光路好坏**。
5. `p` 链：k=3←271、6←268、9←263、12←265、15←266、21←261（k=18 的 t=261，由 §4 位级温度配对）。

## 4. 位级 I²C（可选补充，不替代串口行）

- 采集：`fx2lafw:conn=2.32`、D0=SCL/D1=SDA、D1 下降沿触发、8,000,000 样点 @4 MS/s（2.0 s），11:44:04.535–11:45:53.658Z；`.sr` md5 `0f5603dd…`。
- 解码：`Start / Address write 44 / ACK / Data write FD / ACK / Stop / Start / Address read 44 / ACK / Data read 68 09 6B 59 5C A6 / NACK / Stop`。
- 独立复算：温度字 `0x6809` CRC `0x6B` MATCH；湿度字 `0x595C` CRC `0xA6` MATCH；换算 **261 x10（26.1 ℃）/ 349 x10（34.9 %RH）**。
- 名义时序：命令 STOP→读 START **32.533 ms**（4 MS/s 样点差；非跨设备同步）。
- 配对：k=15 与 k=21 之间唯一采样机会为 k=18；`p(k=21)=261` 与解码温度相等，故该事务对应 k=18。
- 边界：数字采样只证明逻辑电平与采样分辨率内时序，不证明模拟幅度/上升沿/上拉质量。

## 5. 宿主、静态与发布包边界

| 入口 | 结果 |
|---|---|
| 实现侧 `test/build_test.sh` | 5 阶段 48+61+54+28+31 = **222 通过 / 0 失败** |
| 测试侧独立 harness（13 组） | **609 通过 / 0 失败**（含 light.c T=0 40 与 T=3000 启用变体 41） |
| `static_check_ev037.py` | **21/21 PASS**（T=0 默认、无旧 1/3 宏、判据形制、升温方向与严格 >35.0、3 ticks、10 B 帧、上电顺序、S 字段顺序、有界 TX、无浮点/动态内存、无旧霍尔/光通讯） |
| 发布包纯净性 | zip md5 `5df0c6d4…`、CRC OK；仅 axf/hex/PACKAGE.txt 三成员；无 harness/mock/测试开关 |
| 产品源码改动 | 本轮未修改产品源码、工程、网关、Android、权限/环境配置 |

## 6. 验收目标 → 本轮结果

| 目标 | 结果 | 依据 |
|---|---|---|
| FW-REG-ASBUILT | **通过（本任务范围）** | 独立 rebuild = 发布包成员；源码零差异；0E/1W、资源摘要一致 |
| FW-REG-REPORT | **通过（本任务范围）** | fw_core/measure_flow/rf_report/rtc 宿主向量 609/0；实板 `p/H` 与 `q` 语义一致 |
| FW-LIGHT-DEF2-MECH | **通过（本任务范围）** | 静态 21/21；实板 T=0 下 7 周期 `V=0 D=0` 等价性 |
| FW-OBS-PASSIVE | **通过** | 7 周期 `q=1`；节拍/字段/行长/无杂散/无 G/P 全部满足 |
| FW-DIAG-BOOT | **通过** | 端口先开 + HW reset；五行各一次且有序；VDD 可复算；BUS/IOSIG/S 自洽 |
| 发布包 ↔ 实际烧录 | **通过** | 包成员 = 独立重建 = 下载镜像；`Verify OK` |
| FW-LIGHT-DEF2-ENABLE | **未执行（人工 `SKIP_CONFIRMED`）** | EN-01/EN-02 前置未具备；2026-11-11T03:50:42Z「无法准备，先跳过」；T 保持 0；不是通过 |
| 真实升温/超温、光强变化 | **未执行（用户约束）** | 用户明确不能调温、不能改变光强；未施加任何刺激 |
| E2E-04 功耗 | **延期（已批准）** | 无电流表/SMU/示波器；`human_test_deferral_allowed_items=["E2E-04"]`，未测并在此列明 |
| 网关/手机跨链路 | **不在本阶段** | 用户补充条件 4：后续联调阶段 |

**人工确认与批准延期（明示）**：`FW-LIGHT-DEF2-ENABLE` = SKIP_CONFIRMED（未执行，不重测）；`E2E-04` = APPROVED_DEFERRED（未测）。上列未执行项不得删除、不得解释为通过。

---

# 嵌入式验证证据（EV-036 / 发布包 FD-002r8 独立核验：包身份 + 重建下载 + 实板启动/周期/I²C）

状态：`embedded_tester.embedded_integration_verification`（action `firmware_package_check`）独立 HIL 证据。本轮不复用 EV-035 的下载记录：先核对发布包，再独立重建、重新下载到真实目标，并在板上重新采集启动诊断、周期 `S` 行与位级 I²C。
输入：`runtime.artifacts.firmware_package` = `artifacts/firmware_release.md`（FW-REL-001 rev 1）+ `fw_release/fw_sensor_FD-002r8.zip`；测试方案 = `artifacts/test_design.md`（TD-002 rev 8.0）。
负责角色：嵌入式测试

**结论：`HIL_PASS`（本交付已执行范围；无已测固件功能缺陷）**

- **发布包身份**：zip md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`（134,791 B），CRC 测试无坏成员；三成员 MD5 与 `PACKAGE.txt`/`firmware_release.md` 声明一致。
- **重建身份**：本能力独立 `mdk_build {"action":"rebuild"}`（Keil V6.24）0 Error / 1 既有 warning，Code=25964 RO=656 RW=116 ZI=1676；axf `b6a15fed2ed5e3d0c4adc51ac87a4830`、hex `4d728dec9b0d28a352c46436b7586c72`，与发布包成员逐字节一致。
- **下载**：`mdk_flash {}` 见 `Erase Done.Programming Done.Verify OK.Application running ...`（`Verify OK` 仅证明回读校验）。
- **实板启动**：端口先开 + `pyocd` HW reset，同一窗口取得 `BOOT→VDD→IOTEST→IOSIG→BUS` 各一次且有序，`VDD` 可复算，随后 `S k=0`。
- **实板周期**：取得 k=0、3、6、9、15、18 共 6 行；全部 `q=1`、`V=0`、`D=0`、`o=8`、字段在有效域、行 ≤96 B；`p` 链与上一周期 `t` 匹配；采样间期无杂散输出。
- **位级通信**：fx2lafw 被动 I²C 解码为 `0x44` 写 `0xFD`、读 `69 71 DE 54 19 2E`；温度/湿度 CRC 独立复算均 MATCH，换算 27.1 ℃ / 35.1 %RH；`p(k=15)=271` 与该温度相等，说明该事务对应 k=12 采样。
- **静态/交付包边界**：测试侧静态核查 21/21 PASS（`LIGHT_DARK_THRESHOLD_CODE=0`、无旧光通讯/霍尔残留、无浮点/动态内存、上电顺序与 `S` 行字段顺序冻结）；发布包仅含 axf/hex/PACKAGE.txt，不含测试 harness 或测试开关。
- **未执行项（明示，不计为通过）**：`FW-LIGHT-DEF2-ENABLE` 经人工确认 **SKIP_CONFIRMED（2026-11-11T03:50:42Z，「无法准备，先跳过」）**，本轮不重测；`E2E-04` 功耗为已批准 DEFERABLE 且无仪器；真实升温/超温与光强变化受用户约束不安排；网关/手机跨链路按用户条件 4 属后续联调阶段。

本结论只覆盖上列已执行范围，未执行/延期项不得删除或解释为通过。

---

## 1. 被测身份、构建与部署（本轮实际执行）

| 项 | 原始结果 |
|---|---|
| 验证时 HEAD | `5b9b7aa63309c82de1b90e46b533d4559d2f2efa`（接手时工作树干净） |
| 产品源码身份 | 最近触及 `<sensor>/USER MDK EWARM` 的提交 = `82a7e74e5d6bae855a573e1f23f83936ba8db049`；`git diff --stat 82a7e74 HEAD --` 同路径 **为空** |
| 发布包 | `fw_release/fw_sensor_FD-002r8.zip`：134,791 B，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`；CRC 测试通过；成员 MD5/CRC 见 `evidence/ev036/release_identity.txt` |
| 独立重建 | `mdk_build {"action":"rebuild"}`（05:15:06.830–05:15:09.136Z）：**0 Error / 1 Warning**（既有 `main.c(254)`）；`Code=25964 RO=656 RW=116 ZI=1676` |
| 产物身份 | axf **`b6a15fed2ed5e3d0c4adc51ac87a4830`**（297,380 B）、hex **`4d728dec9b0d28a352c46436b7586c72`**（75,275 B），与包内成员逐字节一致 |
| 部署 | `mdk_flash {}`（05:15:19.665–05:15:32.356Z）：`Erase Done.Programming Done.Verify OK.Application running ...` |
| 调试串口 | `serial_list_ports`：`COM43` = Prolific `067B:2303`、`LOCATION=1-9.4.2`，9600 8N1；`COM26` = `C251:F001`（调试器 CDC，未使用） |
| USB 在位 | CW-DAPLink `C251:F001` SN `87094109484987710672FF50`；FX2 `04B4:8613`；手机 `18D1:4EE7` SN `12F046690506`；**未见**电流表/SMU/示波器 |
| 上电编排 | MCP `serial_capture` 先打开 COM43；测试侧后台命令延时 4 s 执行 `pyocd reset -t cortex_m -u <probe> -M attach -m hw`；pyocd `0.36.0` 识别 CW-DAPLINK，reset 窗口 05:15:52.036–05:15:52.806Z，exit 0 |

原始：`evidence/ev036/release_identity.txt`、`tool_results.txt`、`pyocd_reset_boot.txt`。

## 2. 发布包核对（包 → 源 → 重建 → 下载）

| 检查 | 结果 |
|---|---|
| zip 完整性 | `zipfile.testzip()` 无坏成员；包 md5 可复现（确定性成员时间戳） |
| 成员与声明一致 | axf/hex/PACKAGE.txt 三成员 MD5 与 `PACKAGE.txt` 及 `artifacts/firmware_release.md` 完全一致 |
| 包 ↔ 当前源码 | 独立重建 axf/hex 与包内成员 **逐字节一致**；产品源自 `82a7e74` 起无变化 |
| 包 ↔ 实际烧录 | `mdk_flash {}` 使用工程保存配置下载同一 axf，日志 `Verify OK`，下载窗口内产物 md5 未变 |
| 生产包纯净性 | 包内仅 3 个成员，无测试 harness、mock、验证专用源码或测试开关；证据资产全部在 `evidence/ev036/` |

## 3. 上电五行（BOT-01/02/03/04）

原始字节：`evidence/ev036/bot01_hw.cap`（271 B，md5 `b88020173b013491b91d8d3728370b4e`）。

| 顺序 | 原始行（含 CRLF 长度均 ≤96 B） |
|---|---|
| 1 | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` |
| 2 | `VDD ok=1 code=1480 bgrmv=1189 mv=3289` |
| 3 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=0` |
| 4 | `IOSIG scl=95555 sda=30300` |
| 5 | `BUS idle=3 scl=1 sda=1 ack=44` |
| — | `S k=0 p=0 H=0 V=0 o=8 n=2856 x=2864 a=2860 D=0 t=273 h=349 q=1 r=0 s=0 y=0` |

判读（`analyze_ev036.py` → `capture_analysis.txt`）：
1. **顺序与次数**：五行各一次且相邻有序 `BOOT→VDD→IOTEST→IOSIG→BUS`（PASS）。
2. **身份闭合**：`fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600`；镜像身份由本轮独立重建摘要 + `Verify OK` + 包内成员一致闭合；`uid` 与同板历史一致。`BOOT fw=` 是字节冻结的既有横幅，不代表版本号。
3. **VDD 自洽**：独立复算 `4095*1189//1480 = 3289` **MATCH**。
4. **总线自洽**：`BUS ack=44` 与 `IOSIG sda=30300`（0x88 写事务 ACK 时隙拉低）一致，随后 `S k=0 q=1`；`gc=0`。
5. **首样本语义**：复位后首条 `S k=0 p=0 H=0`，符合“无前值”语义。

**BOT-01 重新评估要求的落实（不新增固件测试接口）**：自动编排由测试侧自有脚本完成（MCP 端口先开 + 后台 `pyocd` HW reset），未修改固件、未新增测试宏/接口；正式发布包由独立测试核验，仅含生产产物。`pyocd 0.36.0` 仍**不是** `runtime/environment.yaml` 登记入口，本轮按 Project Goal 补充条件 2 使用现有安装与探针，未安装软件、未改配置；建议配置所有者登记 reset 入口。

## 4. 真实目标 COM43 周期（OBS-01..09，9600 8N1，无刺激）

原始：`bot01_hw.cap`、`cap01_k3.cap`、`cap02_k6_k9.cap`、`cap03_k15_k18.bin`；窗口/字节/时间见 `tool_results.txt`；独立解析见 `capture_analysis.txt`。

| 窗口（UTC） | 原始 `S` 行 | 判读 |
|---|---|---|
| 05:15:47–05:16:18（端口先开 + HW reset） | `S k=0 p=0 H=0 V=0 … D=0 t=273 h=349 q=1 …` | 首样本；`q=1`；`V=0 D=0` |
| 05:16:29–05:21:29 | `S k=3 p=273 H=1 … D=0 t=269 h=355 q=1 …` | `p` 取上一周期 `t` |
| 05:21:34–05:26:34 | `S k=6 p=269 H=1 … D=0 t=267 h=364 q=1 …`；`S k=9 p=267 H=1 … D=0 t=266 h=361 q=1 …` | 连续 k=6→9 |
| 05:28:37–05:33:37 | `S k=15 p=271 H=1 … D=0 t=267 h=358 q=1 …`；`S k=18 p=267 H=1 … D=0 t=266 h=362 q=1 …` | 连续 k=15→18；`p(k=15)=271` 与 I²C 温度换算值配对 |

判读：
1. **连续性与节拍**：观测 k=0、3、6、9、15、18；k=0/3/6/9 与 k=15/18 为连续采样机会（差值恒 3）；同一 `k` 无重复行；采样间期无字节输出。
2. **格式/预算**：全部行 76–79 B（含 CRLF）≤96；15 字段顺序与冻结字面一致；CRLF 完整、无 NUL、无杂散。
3. **字段**：全部周期 `q=1`、`o=8`、`t∈[-400,1250]`、`h∈[0,1000]`、`a∈[n,x]`、`r=0 s=0 y=0`；无 `G`、无 `P`。
4. **判据未启用等价性**：全部周期 `V=0 D=0`；`a=2860..2886` 仅作事实记录，**不据此判“有光”或光路好坏**；`T` 保持 0。
5. **前值语义**：`p(k=3)=t(k=0)=273`、`p(k=6)=t(k=3)=269`、`p(k=9)=t(k=6)=267`、`p(k=18)=t(k=15)=267`；`H=1` 表示有前值，首样本 `H=0`。
6. **节拍精度**：未给出 LSI 容差；以 `k` 增量 3 为内部证据，不声称 180.0 s 精度。

## 5. 位级 I²C（可选补充，非替代任何实板行证据）

- 采集：`fx2lafw:conn=2.32`、D0=SCL、D1=SDA、D1 下降沿触发、8,000,000 样点 @4 MS/s（2.0 s 会话），窗口 05:26:53.563–05:27:38.455Z。原始 `.sr` md5 `79b9edbbebeb6a68ac714795ab0fe04d`；`decoded_i2c.txt`。
- 解码：`Start / Address write 44 / ACK / Data write FD / ACK / Stop`；`Start / Address read 44 / ACK / Data read 69 71 DE 54 19 2E / NACK / Stop`。
- 独立复算：温度字 `0x6971` CRC `0xDE` **MATCH**；湿度字 `0x5419` CRC `0x2E` **MATCH**；换算 **271 x10（27.1 ℃）/ 351 x10（35.1 %RH）**。
- 名义时序：命令 STOP→读 START **32.535 ms**；写/读事务位间隔落在与历史一致量级（数字采样分辨率 250 ns）。
- 配对：`p(k=15)=271` 与本次温度换算值相等，故该事务对应 k=12（跨窗口推断，不是同窗直接配对）。
- 边界：数字采样只证明逻辑电平与采样分辨率内时序，不证明模拟幅度/上升沿/上拉质量。

## 6. 静态与交付包边界（同一源码）

| 入口 | 结果 |
|---|---|
| `static_check_ev036.py`（沿用测试侧只读核查） | **21/21 PASS**：`LIGHT_DARK_THRESHOLD_CODE=0` 交付默认、旧 1/3 宏零残留、`dark = valid && T != 0 && mean >= T`、升温方向与严格 `>35.0`、3 ticks/样本、10 B RF 帧、上电顺序、`S` 行字段顺序冻结、UART TX 有界保护、无浮点/动态内存、无旧霍尔/光通讯引用 |
| 发布包内容 | 仅 axf/hex/PACKAGE.txt；无测试 harness、mock 或测试开关 |
| 产品源码改动 | 本轮**未修改**任何产品源码、Keil/IAR 工程、网关、Android、`runtime/permissions.yaml`、`runtime/environment.yaml` |

## 7. 验收目标 → 本轮结果

| 任务/项 | 结果 | 依据 |
|---|---|---|
| FW-REG-ASBUILT | **通过（本任务范围）** | 包身份/CRC/成员 MD5 与声明一致；独立重建摘要逐字符一致；源码零差异 |
| FW-REG-REPORT | **通过（本任务范围）** | 判定/冻结/有界等待的宿主与静态回归沿用 EV-035 同源码结论；本轮实板周期与 `p/H` 语义一致 |
| FW-LIGHT-DEF2-MECH | **通过（本任务范围）** | 形制/接线/超时/旧宏/资产边界；实板 `T=0` 等价性（6 周期 `V=0 D=0`） |
| FW-OBS-PASSIVE | **通过** | k=0/3/6/9/15/18；全部 `q=1`、字段/节拍/行长/无杂散满足；无失败周期 |
| FW-DIAG-BOOT | **通过** | 端口先开 + HW reset；同一窗口取得五行各一次且有序；`VDD` 可复算；`BUS ack=44` 与 `IOSIG`/`S q=1` 自洽 |
| 发布包身份 ↔ 实际烧录 | **通过** | 包成员 = 独立重建 = 下载镜像；`Verify OK`；上电/周期行为与包说明一致 |
| FW-LIGHT-DEF2-ENABLE | **未执行（人工确认 `SKIP_CONFIRMED`）** | EN-01/EN-02 前置未具备；用户 2026-11-11T03:50:42Z 回复「无法准备，先跳过」；`T=0` 保持；该项**不是通过**，本轮不重测 |
| 真实升温/超温触发、跨链路 | **未执行（用户阶段安排）** | 用户明确禁止调温/调光；跨链路属后续联调；本轮无 `P`/发送 |
| E2E-04 功耗 | **延期（已批准）** | 无电流表/SMU/示波器；`human_test_deferral_allowed_items=["E2E-04"]`，**未测并在此列明** |

**人工确认与批准延期（明示）**：
1. `FW-LIGHT-DEF2-ENABLE` 经人工确认**未执行**（`SKIP_CONFIRMED`，回复「无法准备，先跳过」，2026-11-11T03:50:42Z）；该项本身**不是 PASS**，不重测、不重复询问；`T=0` 保持。
2. `BOT-01` 按重新评估要求由测试侧脚本实际执行并通过（端口先开 + pyocd HW reset），无需新增固件测试接口。
3. `E2E-04` 属已批准 `DEFERABLE`，本轮缺少功耗测量仪器，**未测**并在本报告列明。

## 8. 未执行/延期项与失败报告（逐项关联目标、用例、身份与原始证据）

| # | 目标/用例 | 预期 | 实际 | 原始证据 | 已尝试措施 | 尚未排除的假设 | 最小下一步 |
|---|---|---|---|---|---|---|---|
| N-1 | FW-LIGHT-DEF2-ENABLE / EN-01/02 | 判据形制/数值接受记录 + 同板同构建双态被动证据后才可启用 | **经人工确认未执行（`SKIP_CONFIRMED`）**；`T=0` | 人工确认记录、`static_check_ev036.txt`（常量 0）、`capture_analysis.txt`（`V=0 D=0`、`a=2860..2886`） | 只记录被动 `a` 事实；未回填、未遮光/照射；按指令不再重测 | 分离度与阈值数值仍待需求/架构接受 | 上游给出接受记录与被动双态证据后，由固件实现回填 `T` 并重建；测试侧再复验 `V=1 D=0/1` |
| N-2 | 真实升温/超温触发与跨链路 | 暗 + 升温 >0.9 ℃ 或 >35.0 ℃ 时 `r=1`、`P` 冻结、`s=1`，手机同笔一致 | 全部周期 `r=0 s=0 y=0`，无 `P`、无发送 | `capture_analysis.txt` | 未施加刺激（用户禁止）；本能力本轮未使用手机/网关入口 | 网关/手机链路是否可用（未观测） | 按用户安排的后续联调阶段执行；不伪造发送成功/失败，不据此退回固件实现 |
| N-3 | E2E-04 功耗 | CR2032 采样峰值/深睡电流实测值与条件 | 未执行（已批准延期） | `tool_results.txt` §8（无仪器枚举） | 枚举 USB 设备 | 是否存在未登记测量入口（无） | 仪器具备时由硬件测试测量并记录量程/接线/电池电压 |
| N-4 | T4-03 失败路径 | `q=0` 后紧跟 `G`，失败零副作用、下周期重探/恢复 | 本轮 6/6 周期 `q=1`、0 条 `G`；未触发 | `capture_analysis.txt` | 连续采集至 k=18 | 更长运行/不同供电条件下的失败率（未观测） | 出现真实 `q=0` 时按 `G` 原始行取证；不人为制造失败或改判据 |

**无已测固件功能缺陷**：以上均为未执行/延期项，不是实板测得的实现缺陷；本轮不触发失败回流。

## 9. 证据局限（如实登记）

1. `k=12` 的 `S` 行未落在采集窗口；其 `t` 值由 `p(k=15)=271` 与位级 I²C 温度换算值相等作跨窗口推断。连续证据为 k=0/3/6/9 与 k=15/18。
2. 窗口之间存在未采集间隙（BOT→C1 11.2 s、C1→C2 4.9 s、C2→C3 2 min 2.9 s）；未把跨窗口拼接冒充无间隙连续。
3. `pyocd` 非 `runtime/environment.yaml` 登记入口；本轮按 Project Goal 补充条件 2 使用现有安装/探针，未安装、未改配置；建议配置所有者补登记。
4. 上电五行 `BOOT` 不含 axf md5；镜像身份由独立重建摘要 + `Verify OK` + 包成员一致闭合。
5. LIGHT-DEF-2 **启用后**行为只在宿主变体验证；板上保持 `T=0`，未取得 `V=1`/`D=1` 实板证据。
6. 无升温/超温、无网关/手机同窗、无功耗仪器；相应项未执行，不写通过。
7. 位级 I²C 为数字采样（4 MS/s、250 ns 分辨率），不证明模拟幅度/上升沿；名义时间不能跨设备同步；k=12 配对为跨窗口推断。

## 10. 本轮实际改动（只增测试侧证据）

- 新增：`evidence/ev036/`（4 份 COM43 原始 `.bin`、原始 `.sr` 与采集元数据、I²C 解码、分析/静态/包身份脚本与输出、工具原始结果、发布包身份记录）
- 更新：`evidence/test.md`（本段置顶；EV-035 及更早保留）、`evidence/programmed_prototype.md`（EV-036 下载身份置顶）、`file_manifest.txt`
- **未修改**：`USER/**` 产品源码、Keil/IAR 工程、网关、Android、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载软件或驱动

---

# 嵌入式验证证据（EV-035 / 当前交付固件独立复核，实板周期 + 上电五行）

状态：`embedded_tester.embedded_verification` 独立验证证据（本轮独立 rebuild、`mdk_flash` 下载校验、端口先开后 pyocd HW reset 的上电捕获、COM43 共 4 个窗口约 19 分钟、1 次 fx2lafw 位级 I²C 被动采集/离线解码、宿主与静态复核）
输入：`runtime.artifacts.firmware_current` = `artifacts/firmware_implementation.md`（FWI-003 **rev 8**，LIGHT-DEF-2）；测试方案 = `artifacts/test_design.md`（TD-002 rev 8.0，6 项任务）
负责角色：嵌入式测试

**结论：`TEST_PASS`（本交付范围内已执行的必需用例全部通过，无已测固件缺陷）**

本轮是对当前交付镜像的**新一轮独立复核**（未复用 EV-034 结论）：重新构建、重新下载、重新复位采集。结果：

- **FW-REG-ASBUILT / FW-REG-REPORT / FW-LIGHT-DEF2-MECH**：独立重建摘要与 FWI-003 rev 8 逐字符一致（axf `b6a15fed…`、hex `4d728dec…`，0 Error / 1 既有 warning）；实现侧宿主 222/0、测试侧独立 harness 609/0、静态 21/21。
- **FW-OBS-PASSIVE**：真实目标取得 k=0、3、6、12、15、18 共 6 个采样行；全部 `q=1`、`V=0 D=0`、`o=8`、字段在有效域、每行 ≤96 B 且 CRLF、采样间期无输出；连续集 k=0/3/6 与 k=12/15/18 均成立；`p` 链保持上一周期 `t`。
- **FW-DIAG-BOOT**：端口先开 + `pyocd` HW reset，同一窗口取得 `BOOT→VDD→IOTEST→IOSIG→BUS` 五行各一次且有序，`VDD` 可复算，`BUS ack=44` 与 `IOSIG`/`S q=1` 自洽。
- **FW-LIGHT-DEF2-ENABLE**：**经人工确认未执行（`SKIP_CONFIRMED`，2026-11-11T03:50:42Z，回复「无法准备，先跳过」）**。按 Runtime 指令，该项不纳入本交付的必需测试，且**不写成通过**；`T` 保持 0，不重测、不重复询问。
- **E2E-04（CR2032 采样峰值/深睡电流）**：属已批准 **DEFERABLE**，本轮未见电流表/SMU/示波器，**未测**并在此列明。
- 真实升温 >0.9 ℃ 且无光 / 稳定 >35 ℃ 触发：用户明确当前不能调节温度，不安排；跨链路属用户安排的后续联调阶段。二者均不属本轮固件阶段任务。

本结论仅覆盖上述已执行范围；未执行/延期项不得从汇总中删除，也不得解释为已通过。

---

## 1. 被测身份、构建与部署

| 项 | 原始结果 |
|---|---|
| 验证时 HEAD | `cfe98a18c7932840195f80ed7d587433e0a0701f`（接手时工作树干净） |
| 产品源码身份 | `git log -1 -- <sensor>/USER <sensor>/MDK <sensor>/EWARM` = `82a7e74e5d6bae855a573e1f23f83936ba8db049`（FWI-003 rev 8）；`git diff 82a7e74 HEAD --` 同路径 **为空** |
| 独立构建 | `mdk_build {"action":"rebuild"}`（04:38:11.281–04:38:13.319Z）：**0 Error / 1 Warning**（既有 `main.c(254)` 空循环体）；`Code=25964 RO=656 RW=116 ZI=1676` |
| 产物身份 | axf **`b6a15fed2ed5e3d0c4adc51ac87a4830`**、hex **`4d728dec9b0d28a352c46436b7586c72`**（本能力独立复算，与 FWI-003 rev 8 一致） |
| 部署 | `mdk_flash {}`（04:38:30.078–04:38:42.466Z）：`Erase Done.Programming Done.Verify OK.Application running ...`；下载后 axf/hex md5 不变 |
| GNU 交叉构建 | `gcc/build.sh` exit 0；0 error（27 项既有 warning）；FLASH **34840 B** / RAM **1816 B** |
| 调试串口 | `serial_list_ports`：`COM43` = Prolific `067B:2303`、`LOCATION=1-9.4.2`，9600 8N1；`COM26` = `C251:F001`（调试器 CDC，未使用） |
| USB 在位 | CW-DAPLink `C251:F001` SN `87094109484987710672FF50`；FX2 `04B4:8613`；手机 `18D1:4EE7` SN `12F046690506`；**未见**电流表/SMU/示波器 |
| 上电编排 | MCP `serial_capture` 先打开 COM43；测试侧后台命令延时 4 s 执行 `pyocd reset -t cortex_m -u <probe> -M attach -m hw`；pyocd `0.36.0` 识别 CW-DAPLINK，reset exit 0 |

原始：`evidence/ev035/image_identity.txt`、`tool_results.txt`、`mdk_rebuild.log`、`mdk_flash.log`、`gcc_build.log`、`pyocd_reset_boot.txt`。

## 2. 上电五行（BOT-01/02/03/04）

原始字节：`evidence/ev035/bot01_hw.cap`（271 B，md5 `6bfa1012869228692f722488ce773cc4`）。

| 顺序 | 原始行（含 CRLF 共 50/39/48/27/31 B，均 ≤96） |
|---|---|
| 1 | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` |
| 2 | `VDD ok=1 code=1481 bgrmv=1189 mv=3287` |
| 3 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=0` |
| 4 | `IOSIG scl=95555 sda=30300` |
| 5 | `BUS idle=3 scl=1 sda=1 ack=44` |
| — | `S k=0 p=0 H=0 V=0 o=8 n=2829 x=2833 a=2831 D=0 t=273 h=350 q=1 r=0 s=0 y=0`（76 B） |

判读（`analyze_ev035.py` → `capture_analysis.txt`）：
1. **顺序与次数**：五行各出现一次且相邻有序 `BOOT→VDD→IOTEST→IOSIG→BUS`（PASS）。
2. **身份闭合**：`fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` 与当前交付及同板历史 UID 一致；构建摘要 `b6a15fed…` + `Verify OK` 绑定当前镜像。`Verify OK` 只证明下载校验。
3. **VDD 自洽**：独立复算 `4095*1189//1481 = 3287` **MATCH**。
4. **总线自洽**：`BUS ack=44` 与 `IOSIG sda=30300`（地址 0x88 写事务 ACK 时隙被拉低）一致，随后 `S k=0 q=1`；`gc=0`（0x44/0x45 已应答，未做 general-call 复位）。
5. **首样本语义**：复位后首条 `S k=0 p=0 H=0`，符合“无前值”语义。

**BOT-04（pyocd 只读评估）**：`pyocd 0.36.0` 已安装并可识别 CW-DAPLINK；`pyocd` 仍**不是** `runtime/environment.yaml` 登记入口。本轮按 Project Goal 补充条件 2 使用现有安装与探针，未安装软件、未改配置；建议配置所有者登记 reset 入口。原始输出：`evidence/ev035/pyocd_reset_boot.txt`、`tool_results.txt` §3。

## 3. 真实目标 COM43 周期（OBS-01..09，9600 8N1，无刺激）

原始：`bot01_hw.cap`、`cap01_k3_k6.bin`、`cap02_k12.bin`、`cap03_k15_k18.bin`；窗口/字节/时间见 `capture_windows.txt`；独立解析见 `capture_analysis.txt`。

| 窗口（UTC） | 原始 `S` 行 | 判读 |
|---|---|---|
| 04:38:53–04:39:23（端口先开 + HW reset） | `S k=0 p=0 H=0 V=0 o=8 n=2829 x=2833 a=2831 D=0 t=273 h=350 q=1 …` | 首样本；`q=1`；`V=0 D=0` |
| 04:39:35–04:44:35 | `S k=3 p=273 … t=272 h=346 q=1 …`；`S k=6 p=272 … t=265 h=360 q=1 …` | 连续 k=3→6 |
| 04:47:46–04:52:46 | `S k=12 p=268 … t=267 h=360 q=1 …` | `p(k=12)=268` 与位级 I²C 的 k=9 换算值配对（跨窗口推断） |
| 04:52:55–04:57:56 | `S k=15 p=267 … t=266 h=362 q=1 …`；`S k=18 p=266 … t=271 h=351 q=1 …` | 连续 k=15→18 |

判读：
1. **连续性与节拍**：观测 k=0、3、6、12、15、18；k=0/3/6 与 k=12/15/18 均为连续采样机会（差值恒 3）；同一 `k` 无重复行；采样间期无字节输出。窗口间隙记录于 `capture_windows.txt`，未把跨窗口拼接冒充无间隙连续。
2. **格式/预算**：全部行 76–79 B（含 CRLF）≤96；15 字段顺序与冻结字面一致；CRLF 完整、无 NUL、无杂散。
3. **字段**：全部周期 `q=1`、`o=8`、`t∈[-400,1250]`、`h∈[0,1000]`、`a∈[n,x]`、`r=0 s=0 y=0`；无 `G`、无 `P`。
4. **判据未启用等价性**：全部周期 `V=0 D=0`；`a=2830..2840` 仅作事实记录，**不据此判“有光”或光路好坏**；`T` 保持 0。
5. **F-1 语义**：`p(k=3)=t(k=0)=273`、`p(k=6)=t(k=3)=272`、`p(k=15)=t(k=12)=267`、`p(k=18)=t(k=15)=266`；`p` 取上一周期值。
6. **节拍精度**：readme 未给出 LSI 容差；以 `k` 增量 3 为内部证据，不声称 180.0 s 精度。

## 4. 位级 I²C（可选补充，非替代任何实板行证据）

- 采集：`fx2lafw:conn=2.32`、D0=SCL、D1=SDA、D1 下降沿触发、8,000,000 样点 @4 MS/s（2.0 s），窗口约 04:44:36–04:47:34Z。原始 `.sr` md5 `602e49e48dc4829d35a4e4d1bc3d155f`；`decoded_i2c.txt`。
- 解码：`Start / Address write 44 / ACK / Data write FD / ACK / Stop`；`Start / Address read 44 / ACK / Data read 68 FC 1C 55 AB 07 / NACK / Stop`。
- 独立复算：温度字 `0x68FC` CRC `0x1C` **MATCH**；湿度字 `0x55AB` CRC `0x07` **MATCH**；换算 **268 x10（26.8 ℃）/ 358 x10（35.8 %RH）**。
- 名义时序：命令 STOP→读 START **32.533 ms**；写/读事务与 SCL 位间隔落在与历史一致的量级（数字采样分辨率 250 ns）。
- 配对：`p(k=12)=268` 与本次温度换算值相等，故该事务对应 k=9（跨窗口推断，不是同窗直接配对）。
- 边界：数字采样只证明逻辑电平与采样分辨率内时序，不证明模拟幅度/上升沿/上拉质量。

## 5. 宿主与静态证据（同一源码）

| 入口 | 结果 |
|---|---|
| 实现侧 `test/build_test.sh` | 48+61+54+28+31 = **222 / 0**，exit 0 |
| 测试侧 13 个独立 harness | **609 / 0**（明细见 `host_harness_summary.txt`） |
| LIGHT-DEF-2 形制/接线 | `host_fw_core_verify_ev` 59/0（`T=0`、`mean=T-1/T`、`T=4095`、`valid=false`）；`host_light_verify_ev` `T=0` 40/0、`T=3000` 41/0（真实 `light.c`）；`host_vdd_verify_ev` 31/0 |
| T1/T2/T3/T4 回归 | `host_measure_flow` 79/0、`host_rf_report` 30/0、`host_diag_line` 38/0、`host_diag_probe` 109/0、`host_gxht40` 93/0、`host_sf_i2c` 51/0、`host_rtc` 17/0、`host_rf_frame` 10/0、`host_gw_compat` 11/0 |
| 静态核查 | `static_check_ev035.py` **21/21 PASS**：`T=0`、旧宏零残留、无浮点/动态内存、无旧通路、`S` 行顺序、上电顺序、有界等待 |
| 验证资产边界 | 实现提交 `82a7e74` 未修改任何 `*_verify_ev.c`/`mock_*_ev/`；本轮未改产品源码 |

## 6. 验收目标 → 本轮结果

| 任务 | 结果 | 依据 |
|---|---|---|
| FW-REG-ASBUILT | **通过（本任务范围）** | 独立重建 `b6a15fed…`/`4d728dec…`（与 FWI-003 rev 8 一致）；宿主/静态回归全部通过；产品源码零差异 |
| FW-REG-REPORT | **通过（本任务范围）** | T1/T2/T3 独立 harness 全部通过；升温方向/冻结快照/有界等待在宿主可复现 |
| FW-LIGHT-DEF2-MECH | **通过（本任务范围）** | 形制/接线/超时/旧宏/三构建/资产边界全部满足；实板 `T=0` 等价性成立（6 个周期 `V=0 D=0`） |
| FW-OBS-PASSIVE | **通过** | k=0/3/6/12/15/18；全部 `q=1`、字段/节拍/行长/无杂散满足；无失败周期 |
| FW-DIAG-BOOT | **通过** | 端口先开 + HW reset；同一窗口取得 `BOOT→VDD→IOTEST→IOSIG→BUS` 五行各一次且有序；`VDD` 可复算；`BUS ack=44` 与 `IOSIG`/`S q=1` 自洽 |
| FW-LIGHT-DEF2-ENABLE | **未执行（人工确认 `SKIP_CONFIRMED`）** | EN-01 无接受记录；EN-02 无双态被动证据；用户明确禁止改变光强并于 2026-11-11T03:50:42Z 回复「无法准备，先跳过」；`T=0` 保持。该项不纳入本交付必需测试，但**不是通过** |
| 真实升温/超温触发、跨链路 | **未执行（用户阶段安排）** | 用户明确禁止调温；跨链路属后续联调；本轮无 `P`/发送 |
| E2E-04 功耗 | **延期（已批准）** | 无电流表/SMU/示波器；`human_test_deferral_allowed_items=["E2E-04"]`，**未测并在此列明** |

**人工确认与批准延期（明示）**：
1. `FW-LIGHT-DEF2-ENABLE` 经人工确认**未执行**（`SKIP_CONFIRMED`，回复「无法准备，先跳过」，2026-11-11T03:50:42Z）；该项本身**不是 PASS**，不重测、不重复询问；`T=0` 保持。
2. `BOT-01` 的自动编排已由测试侧脚本实际执行并通过（无需新增固件测试接口）。
3. `E2E-04` 属已批准 `DEFERABLE`，本轮缺少功耗测量仪器，**未测**并在本报告列明。

## 7. 未执行/延期项与失败报告（逐项关联目标、用例、身份与原始证据）

| # | 目标/用例 | 预期 | 实际 | 原始证据 | 已尝试措施 | 尚未排除的假设 | 最小下一步 |
|---|---|---|---|---|---|---|---|
| N-1 | FW-LIGHT-DEF2-ENABLE / EN-01/02 | 存在判据形制/数值接受记录 + 同板同构建双态被动证据后才可启用 | **经人工确认未执行（`SKIP_CONFIRMED`）**：无接受记录；无受光/无光双态；用户于 2026-11-11T03:50:42Z 回复「无法准备，先跳过」；`T=0` | 人工确认记录、`static_check_ev035.txt`（常量 0）、`capture_analysis.txt`（`V=0 D=0`、`a=2830..2840`） | 记录被动 `a` 事实；未回填、未遮光/照射；按指令不再重测该项 | 分离度与阈值数值仍待需求/架构接受 | 上游给出接受记录与被动双态证据后，由固件实现回填 `T` 并重建；测试侧再复验 `V=1 D=0/1` |
| N-2 | 真实升温/超温触发与跨链路 | 暗 + 升温 >0.9 ℃ 或 >35.0 ℃ 时 `r=1`、`P` 冻结、`s=1`，手机同笔一致 | 全部周期 `r=0 s=0 y=0`，无 `P`、无发送 | `capture_analysis.txt` | 未施加刺激（用户禁止）；本能力无 `android_test` 入口 | 网关/手机链路是否可用（未观测） | 按用户安排的后续联调阶段执行；不伪造发送成功/失败，不据此退回固件实现 |
| N-3 | E2E-04 功耗 | CR2032 采样峰值/深睡电流实测值与条件 | 未执行（已批准延期） | `tool_results.txt` §8（无仪器枚举） | 枚举 USB 设备 | 是否存在未登记测量入口（无） | 仪器具备时由硬件测试测量并记录量程/接线/电池电压 |
| N-4 | T4-03 失败路径 | `q=0` 后紧跟 `G`，失败零副作用、下周期重探/恢复 | 本轮 6/6 周期 `q=1`、0 条 `G`；未触发 | `capture_analysis.txt` | 连续采集至 k=18 | 更长运行/不同供电条件下的失败率（未观测） | 出现真实 `q=0` 时按 `G` 原始行取证；不人为制造失败或改判据 |

**无已测固件功能缺陷**：以上均为未执行/延期项，不是实板测得的实现缺陷；本轮不触发失败回流。

## 8. 证据局限（如实登记）

1. 串口窗口之间存在未采集间隙（W1 结束至 W2 开始 3 min 10 s）；k=9 的 `S` 行未直接取得，其温度由位级 I²C 换算值与 `p(k=12)` 相等作跨窗口推断。连续证据为 k=0/3/6 与 k=12/15/18。
2. `pyocd` 非 `runtime/environment.yaml` 登记入口；本轮按 Project Goal 补充条件 2 使用现有安装/探针，未安装、未改配置；建议配置所有者补登记。
3. 上电五行的 `BOOT` 不含 axf md5；镜像身份由独立 rebuild 摘要 + `Verify OK` + `fw=FD-002r4` 闭合，`uid=6A002C00` 与同板历史一致。
4. LIGHT-DEF-2 **启用后**行为只在宿主 `T=3000` 变体验证；板上保持 `T=0`，未取得 `V=1`/`D=1` 实板证据。
5. 无升温/超温、无网关/手机同窗、无功耗仪器；相应项未执行，不写通过。
6. 位级 I²C 为数字采样（4 MS/s、250 ns 分辨率），不证明模拟幅度/上升沿；名义时间不能跨设备同步；k=9 配对为跨窗口推断。

## 9. 本轮实际改动（只增测试侧证据）

- 新增：`evidence/ev035/`（原始 `.bin`×4、原始 `.sr`、解码、分析/静态脚本与输出、pyocd/工具结果、宿主日志汇总）
- 更新：`evidence/test.md`（本段置顶；EV-034 及更早保留）、`evidence/programmed_prototype.md`（EV-035 下载身份置顶）、`file_manifest.txt`
- **未修改**：`USER/**` 产品源码、Keil/IAR 工程、网关、Android、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载软件或驱动

---

# 嵌入式验证证据（EV-034 / LIGHT-DEF-2 交付固件实板复核 + BOT-01 自动编排成功）

状态：`embedded_tester.embedded_verification` 独立验证证据（本轮独立 rebuild、下载、端口先开后复位、COM43 原始捕获（上电窗口 + 3 个周期窗口 + 1 份补充）、1 次位级 I²C 被动采集/离线解码、宿主与静态复核）
输入：`runtime.artifacts.firmware_current` = `artifacts/firmware_implementation.md`（FWI-003 **rev 8**，LIGHT-DEF-2）；测试方案 = `artifacts/test_design.md`（TD-002 rev 8.0，6 项任务）
负责角色：嵌入式测试

**结论：`TEST_FAIL`（完整目标；`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）**

本轮**无已测固件缺陷**。上一轮阻塞的 `FW-DIAG-BOOT`（BOT-01/02/03/04）本轮已通过：测试侧脚本先打开 COM43，再用 `pyocd` 经 CW-DAPLink 执行 **HW reset**，在同一窗口取得完整的 `BOOT→VDD→IOTEST→IOSIG→BUS` 五行与首条 `S k=0`。当前交付增量（LIGHT-DEF-2 形制、`T=0` 未启用等价性）与被动采样行为继续在真实板上成立；完整目标仍未通过，原因是其余必需项由用户约束/上游前置**未执行且无已测缺陷**：

- `FW-LIGHT-DEF2-ENABLE`（EN-01/02）：**经人工确认未执行（`SKIP_CONFIRMED`，2026-11-11T03:50:42Z）**。原因：无判据形制/数值接受记录，无同板同构建双态被动证据；用户明确当前不能改变光强，并已回复「无法准备，先跳过」。该项按 `NOT_EXECUTED` 记录、不重测、不写成通过；`T` 保持 0。
- 真实升温 >0.9 ℃ 且无光 / 稳定 >35 ℃ 触发：用户明确当前不能调节温度，不安排。
- 网关/手机跨链路：按用户补充条件 4 属后续联调阶段，不在本固件阶段执行，也不据此退回固件实现。
- `E2E-04`（CR2032 采样峰值/深睡电流）：属已批准 **DEFERABLE** 延期项（无电流表/SMU/示波器），本轮未测并在此列明。

按能力契约，未执行/条件未满足的必需项不得从汇总删除后判通过，故本轮返回 `TEST_FAIL`；`failure_cause` 为外部条件未执行，而非实板测得的固件功能缺陷。

---

## 1. 被测身份、构建与部署

| 项 | 原始结果 |
|---|---|
| 验证时 HEAD | `d7a645cc9aa732fd752d34933dd672d30d4932b3`（接手时工作树干净） |
| 产品源码身份 | `git log -1 -- USER` = `82a7e74e5d6bae855a573e1f23f83936ba8db049`；`git diff --stat 82a7e74 HEAD -- USER/ MDK/ EWARM/` **为空**（产品源码/工程 = FWI-003 rev 8） |
| 独立构建 | `mdk_build {"action":"rebuild"}`（02:59:51.125–02:59:53.060Z）：**0 Error / 1 Warning**（既有 `main.c(254)` 空循环体）；`Code=25964 RO=656 RW=116 ZI=1676` |
| 产物身份 | axf **`b6a15fed2ed5e3d0c4adc51ac87a4830`**、hex **`4d728dec9b0d28a352c46436b7586c72`**（本能力独立复算；与 FWI-003 rev 8 一致） |
| 部署 | `mdk_flash {}`（03:00:06.302–03:00:18.715Z）：`Build target 'Project'` 0E/0W → `Erase Done.Programming Done.Verify OK.Application running ...`；下载后 axf/hex md5 不变 |
| GNU 交叉构建 | `gcc/build.sh` exit 0；0 error（27 项既有 warning）；FLASH **34840 B** / RAM **1816 B**（与实现记录一致） |
| 调试串口 | `serial_list_ports`：`COM43` = Prolific `067B:2303`、`LOCATION=1-9.4.2`，9600 8N1；`COM26` = `C251:F001`（调试器 CDC，未使用） |
| USB 在位 | CW-DAPLink `C251:F001` SN `87094109484987710672FF50`；手机 `18D1:4EE7` SN `12F046690506`；FX2 `04B4:8613`；**未见**电流表/SMU/示波器 |
| 逻辑分析仪 | `sigrok_probe` conn=2.32 exit 0；`sigrok_capture`/`sigrok_decode` exit 0（参数与原始文件见 §3） |

原始：`evidence/ev034/image_identity.txt`、`tool_results.txt`、`gcc_build.log`。

## 2. 上电五行（BOT-01/02/03/04）——本轮已通过

**编排方法（BOT-01，测试侧脚本，未改固件）**：`capture_with_reset.py` 用 pyserial 先打开 COM43 9600 8N1 并确认端口就绪，等待 2 s 后调用 `pyocd reset -t cortex_m -u <probe> -M attach -m hw`（nRESET），继续采集 25 s。原始字节 `bot01_hw.cap`（271 B，md5 `d6121c06834b6abc23bc4f0c74348746`）。

| 顺序 | 原始行（含 CRLF 共 50/39/48/27/31 B，均 ≤96） |
|---|---|
| 1 | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` |
| 2 | `VDD ok=1 code=1481 bgrmv=1189 mv=3287` |
| 3 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=0` |
| 4 | `IOSIG scl=95555 sda=30300` |
| 5 | `BUS idle=3 scl=1 sda=1 ack=44` |
| — | `S k=0 p=0 H=0 V=0 o=8 n=3045 x=3054 a=3049 D=0 t=256 h=374 q=1 r=0 s=0 y=0`（76 B） |

判读（`analyze_ev034.py` / `capture_analysis.txt`）：
1. **顺序与次数**：五行各出现一次且相邻有序 `BOOT→VDD→IOTEST→IOSIG→BUS`（PASS）；不是只取得 `BUS`/后续 `S`。
2. **身份闭合**：`BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` 与当前交付（FWI-003 rev 8）和同板历史 UID 一致；构建摘要 `b6a15fed…` + `Verify OK` 绑定当前镜像。`Verify OK` 只证明下载校验。
3. **VDD 自洽**：`ok=1 code=1481 bgrmv=1189 mv=3287`；独立复算 `4095*1189//1481 = 3287` **MATCH**（非伪造）。
4. **总线自洽**：`BUS ack=44` 与 `IOSIG sda=30300`（地址 0x88 写事务 ACK 时隙被拉低）一致，且随后 `S k=0 q=1`；`gc=0`（0x44/0x45 已应答，未做 general-call 复位）。
5. **`S` 首样本语义**：复位后首条 `S k=0 p=0 H=0`，符合“无前值”语义；下一周期 `p` 链见 §3。

**BOT-04（pyocd 只读评估）**：`pyocd 0.36.0`；探针 `P&S CW-DAPLINK(CMSIS-DAP) 87094109484987710672FF50` 被识别；`pyocd list --targets --name CW32` **为空**，仅有 builtin `cortex_m`（Cortex-M0+ r0p1 可发现）；`sysresetreq`/`hw` 均报告 Done，`hw`（nRESET）产生了本轮的完整上电捕获。**pyocd 不是 `runtime/environment.yaml` 登记入口**；本轮按 Project Goal 补充条件 2 的明确要求仅使用现有安装与探针，未安装软件、未改配置。建议配置所有者登记 pyocd reset 入口（或加载 `WHXY.CW32L010_DFP 1.0.2`）。原始输出：`pyocd_check.txt`。

**编排观察（不写成固件缺陷）**：先用 MCP `serial_capture` 开窗、再在 3–8 s 后外部复位时，4 个窗口均为 0 字节；同一复位确实生效（后续 `k` 从 0 重新计），说明是采集工具的端口就绪时刻与一次性上电输出之间的编排问题。改用“pyserial 先开端口确认 + 2 s 后 HW reset”后一次成功。该差异属测试侧编排，不是固件失败。

**按 Runtime 的 BOT-01 重新评估要求**：自动测试已用测试侧脚本实现，**不需要新增固件测试接口**，因此不提出固件改动请求。最小验收方法（可复跑）：以 9600 8N1 打开 COM43 并确认端口就绪 → 调用 `pyocd reset -t cortex_m -M attach -m hw`（CW-DAPLink，nRESET）→ 同窗口采集 ≥10 s → 断言五行各一次且有序、`BOOT fw/uid/rst/uart` 可读、`VDD` 可复算、`BUS ack` 与 `IOSIG`/`S q=1` 自洽。脚本 `capture_with_reset.py` 与原始 `bot01_hw.cap` 已入库。

## 3. 真实目标 COM43 周期（OBS-01..09，9600 8N1，无刺激）

原始：`bot01_hw.cap`、`cap01_k3_k6.bin`、`cap02_k9_k12.bin`、`cap03_k18.bin`（补充：`cap00_k3_prior_reset.bin`）；窗口/字节/时间见 `capture_windows.txt`；独立解析 `analyze_ev034.py` → `capture_analysis.txt`（不引用实现常量）。

| 窗口（UTC） | 原始 `S` 行 | 判读 |
|---|---|---|
| 03:10:38–03:11:06（端口先开 + HW reset） | `S k=0 p=0 H=0 V=0 o=8 n=3045 x=3054 a=3049 D=0 t=256 h=374 q=1 r=0 s=0 y=0` | 首样本；`q=1`；`V=0 D=0` |
| 03:12:10–03:17:10 | `S k=3 p=256 … t=255 h=375 q=1 …`；`S k=6 p=255 … t=253 h=380 q=1 …` | 连续 k=3→6 |
| 03:17:38–03:22:38 | `S k=9 p=253 … t=253 h=381 q=1 …`；`S k=12 p=253 … t=252 h=383 q=1 …` | 连续 k=9→12 |
| 03:26:38–03:30:39 | `S k=18 p=254 … t=253 h=383 q=1 …` | `p(k=18)=254` 与位级 I²C 的 k=15 换算值配对（跨窗口推断） |

判读：
1. **连续性与节拍**：k=0、3、6、9、12 为**连续 5 个采样机会**（差值恒 3），另取得 k=18；同一 `k` 无重复行，采样间期无字节输出。
2. **格式/预算**：全部行 76–79 B（含 CRLF）≤96；15 字段顺序与冻结字面一致；CRLF 完整、无 NUL、无杂散。
3. **字段**：全部周期 `q=1`、`o=8`、`t∈[-400,1250]`、`h∈[0,1000]`、`a∈[n,x]`、`r=0 s=0 y=0`；无 `G`、无 `P`。
4. **判据未启用等价性**：全部周期 `V=0 D=0`；`a=2959..3049` 仅作事实记录，不据此判“有光”或光路好坏；`T` 保持 0。
5. **F-1 语义**：`p(k=3)=t(k=0)=256`、`p(k=6)=t(k=3)=255`、`p(k=9)=t(k=6)=253`、`p(k=12)=t(k=9)=253`（`p(k=9)==t(k=9)` 为同温巧合，由 `p(k=12)=253` 反证 `p` 取上一周期值）。
6. **节拍精度**：readme 未给出 LSI 容差；以 `k` 增量 3 为内部证据，不声称 180.0 s 精度。

## 4. 位级 I²C（可选补充，非替代任何实板行证据）

- 采集：`fx2lafw:conn=2.32`、D0=SCL、D1=SDA、D1 下降沿触发、8,000,000 样点 @4 MS/s（2.0 s），窗口约 03:25:34–03:25:36Z。原始 `.sr` md5 `6de76678f0476d6b89a9b8df8a62d5b5`；`decoded_i2c.txt`。
- 解码：`Start / Address write 44 / ACK / Data write FD / ACK / Stop`；`Start / Address read 44 / ACK / Data read 67 01 4B 59 D6 07 / NACK / Stop`。
- 独立复算：温度字 `0x6701` CRC `0x4B` **MATCH**；湿度字 `0x59D6` CRC `0x07` **MATCH**；换算 **254 x10（25.4 ℃）/ 379 x10（37.9 %RH）**。
- 名义时序：写事务 12.051 ms；命令 STOP→读 START **32.528 ms**；读事务 40.166 ms；SCL 位间隔 **614.25 µs**（≈1.628 kHz）。
- 配对：`p(k=18)=254` 与本次温度换算值相等，故该事务对应 k=15（跨窗口推断，不是同窗直接配对）。
- 边界：数字采样只证明逻辑电平与采样分辨率内时序，不证明模拟幅度/上升沿/上拉质量。

## 5. 宿主与静态证据（同一源码）

| 入口 | 结果 |
|---|---|
| 实现侧 `test/build_test.sh` | 48+61+54+28+31 = **222 / 0**，exit 0 |
| 测试侧 13 个独立 harness | **609 / 0**（明细见 `host_harness_summary.txt`） |
| LIGHT-DEF-2 形制/接线 | `host_fw_core_verify_ev` 59/0（含 `T=0`、`mean=T-1/T`、`T=4095`、`valid=false`）；`host_light_verify_ev` `T=0` 40/0、`T=3000` 41/0（真实 `light.c`）；`host_vdd_verify_ev` 31/0 |
| T1/T2/T3/T4 回归 | `host_measure_flow` 79/0、`host_rf_report` 30/0、`host_diag_line` 38/0、`host_diag_probe` 109/0、`host_gxht40` 93/0、`host_sf_i2c` 51/0、`host_rtc` 17/0、`host_rf_frame` 10/0、`host_gw_compat` 11/0 |
| 静态核查 | `static_check_ev034.py` **21/21 PASS**：`T=0`、旧宏零残留、无浮点/动态内存、无旧通路、`S` 行顺序、上电顺序、有界等待 |
| 验证资产边界 | 实现提交 `82a7e74` 未修改任何 `*_verify_ev.c`/`mock_*_ev/`；本轮未改产品源码 |

## 6. 验收目标 → 本轮结果

| 任务 | 结果 | 依据 |
|---|---|---|
| FW-REG-ASBUILT | **通过（本任务范围）** | 独立重建 `b6a15fed…`/`4d728dec…`（与 FWI-003 rev 8 一致）；宿主/静态回归全部通过；`git diff 82a7e74..HEAD` 产品源码为空 |
| FW-REG-REPORT | **通过（本任务范围）** | T1/T2/T3 独立 harness 全部通过；升温方向/冻结快照/有界等待在宿主可复现 |
| FW-LIGHT-DEF2-MECH | **通过（本任务范围）** | 形制/接线/超时/旧宏/三构建/资产边界全部满足；实板 `T=0` 等价性成立（5 个连续周期 `V=0 D=0`） |
| FW-OBS-PASSIVE | **通过** | k=0,3,6,9,12 连续 5 个采样机会 + k=18；全部 `q=1`、字段/节拍/行长/无杂散满足；无失败周期 |
| FW-DIAG-BOOT | **通过（本轮新增）** | 端口先开 + HW reset；同一窗口取得 `BOOT→VDD→IOTEST→IOSIG→BUS` 五行各一次且有序；`uid/rst/uart` 可读；`VDD` 可复算；`BUS ack=44` 与 `IOSIG`/`S q=1` 自洽 |
| FW-LIGHT-DEF2-ENABLE | **未执行（人工确认 `SKIP_CONFIRMED`）** | EN-01 无接受记录；EN-02 无双态被动证据；用户明确禁止改变光强并于 2026-11-11T03:50:42Z 回复「无法准备，先跳过」；`T=0` 保持。该未执行项不从汇总删除，阻断整体通过 |
| 真实升温/超温触发、跨链路 | **未执行（用户阶段安排）** | 用户明确禁止调温；跨链路属后续联调；本轮无 `P`/发送 |
| E2E-04 功耗 | **延期（已批准）** | 无电流表/SMU/示波器；`human_test_deferral_allowed_items=["E2E-04"]`，**未测并在此列明** |

**人工确认与批准延期（明示）**：
1. `FW-LIGHT-DEF2-ENABLE` 经人工确认**未执行**（`SKIP_CONFIRMED`，回复「无法准备，先跳过」，2026-11-11T03:50:42Z）；该项为**强制项**，不从交付范围删除、不写成通过，本报告与交付中明确列明。
2. `BOT-01` 按 Runtime 重新评估要求已由测试侧脚本实际执行并通过（无需新增固件测试接口）。
3. `E2E-04` 属已批准 `DEFERABLE`，本轮缺少功耗测量仪器，**未测**并在本报告列明。

## 7. 失败/未执行报告（逐项关联目标、用例、身份与原始证据）

| # | 目标/用例 | 预期 | 实际 | 原始证据 | 已尝试措施 | 尚未排除的假设 | 最小下一步 |
|---|---|---|---|---|---|---|---|
| F-1 | FW-LIGHT-DEF2-ENABLE / EN-01/02 | 存在判据形制/数值接受记录 + 同板同构建双态被动证据后才可启用 | **经人工确认未执行（`SKIP_CONFIRMED`）**：无接受记录（`interface_contract.md` 无 LIGHT-DEF-2 匹配）；无受光/无光双态；用户于 2026-11-11T03:50:42Z 回复「无法准备，先跳过」；`T=0` | 人工确认记录（2026-11-11T03:50:42Z，回复「无法准备，先跳过」）、`static_check_ev034.txt`（常量 0）、`capture_analysis.txt`（`V=0 D=0`、`a=2959..3049`） | 记录被动 `a` 事实；未回填、未遮光/照射；按指令不再重测该项 | 分离度与阈值数值仍待需求/架构接受 | 上游给出接受记录与被动双态证据后，由固件实现回填 `T` 并重建；测试侧再复验 `V=1 D=0/1`（本轮不再重复询问同一项） |
| F-2 | 真实升温/超温触发与跨链路 | 暗 + 升温 >0.9 ℃ 或 >35.0 ℃ 时 `r=1`、`P` 冻结、`s=1`，手机同笔一致 | 全部周期 `r=0 s=0 y=0`，无 `P`、无发送 | `capture_analysis.txt` | 未施加刺激（用户禁止）；本能力无 `android_test` 入口 | 网关/手机链路是否可用（未观测） | 按用户安排的后续联调阶段执行；不伪造发送成功/失败，不据此退回固件实现 |
| F-3 | T4-03 失败路径 | `q=0` 后紧跟 `G`，失败零副作用、下周期重探/恢复 | 本轮 5/5 连续周期 `q=1`、0 条 `G`；未触发 | `capture_analysis.txt` | 连续采集至 k=18 | 更长运行/不同供电条件下的失败率（未观测） | 出现真实 `q=0` 时按 `G` 原始行取证；不人为制造失败或改判据 |
| F-4 | E2E-04 功耗 | CR2032 采样峰值/深睡电流实测值与条件 | 未执行（已批准延期） | `tool_results.txt` §2（无仪器枚举） | 枚举 USB 设备 | 是否存在未登记测量入口（无） | 仪器具备时由硬件测试测量并记录量程/接线/电池电压 |

## 8. 证据局限（如实登记）

1. MCP `serial_capture` 在“外部复位发生在窗口早期”时出现 0 字节（4 个窗口）；同一复位经后续 `k` 从 0 重置证明生效。改用 pyserial 先开端口 + 2 s 后 HW reset 后取得完整上电段。该差异属采集编排，不是固件缺陷，但说明“仅用 MCP 串口工具 + 外部复位”在本运行时不可靠。
2. `pyocd` 非 `runtime/environment.yaml` 登记入口；本轮按 Project Goal 补充条件 2 使用现有安装/探针，未安装、未改配置；建议配置所有者补登记。
3. 上电五行的 `BOOT` 不含 axf md5；镜像身份由独立 rebuild 摘要 + `Verify OK` + `fw=FD-002r4` 闭合，`uid=6A002C00` 与同板历史一致。
4. LIGHT-DEF-2 **启用后**行为只在宿主 `T=3000` 变体验证；板上保持 `T=0`，未取得 `V=1`/`D=1` 实板证据。
5. 无升温/超温、无网关/手机同窗、无功耗仪器；相应项未执行，不写通过。
6. 位级 I²C 为数字采样（4 MS/s、250 ns 分辨率），不证明模拟幅度/上升沿；名义时间不能跨设备同步；k=15 配对为跨窗口推断（`p(k=18)=254` 与换算值相等）。
7. 串口窗口之间存在未采集间隙（k=15 的 `S` 行未直接取得）；连续证据为 k=0,3,6,9,12。

## 9. 本轮实际改动（只增测试侧证据）

- 新增：`evidence/ev034/`（原始 `.bin`×5、原始 `.sr`、解码、分析/静态脚本与输出、pyocd 评估、工具结果、宿主日志汇总）
- 更新：`evidence/test.md`（本段置顶；EV-033 及更早保留）、`evidence/programmed_prototype.md`（EV-034 下载身份置顶）、`file_manifest.txt`
- **未修改**：`USER/**` 产品源码、Keil/IAR 工程、网关、Android、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载软件或驱动

---

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

原始：`cap01_k3.cap`、`cap02_k9.bin`、`cap03_k12_k15.cap`、`cap04_k21.cap`、`capture_windows.txt`；独立解析：`analyze_ev033.py` → `capture_analysis.txt`（不引用实现常量）。

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
| F-1 | FW-DIAG-BOOT / BOT-01/02/03 | 同一 COM43 窗口取得 `BOOT→VDD→IOTEST→IOSIG→BUS` 与 `fw/uid/rst` | 仅取得周期 `S`；上电段在端口打开前已输出（flash 结束 23:14:45.4，端口 23:14:58.3 建立） | `cap01_k3.cap`、`tool_results.txt` §3–4 | 下载后立即采集；只读评估 pyocd 0.36.0（`Target=n/a`、CW32 目标表为空）；已向用户请求该采集编排前置 | 是否存在未登记、可“先开端口后复位”的并发编排入口（当前无） | **用户已明确回复“无法准备，先跳过”（`SKIP_CONFIRMED`）**：本项按人工确认未执行记录，本轮及后续同一批准流程不再重测、不写成通过；后续若提供预打开端口 + 窗口内复位或登记的 CW32L010 调试入口，再另行验证。不得改固件、不得用后续 `S` 替代 |
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

# 嵌入式验证证据（EV-030 / 当前交付镜像完整交付复核）

状态：`embedded_tester.embedded_verification` 独立验证证据（本轮独立全量 rebuild、下载当前交付提交、在真实目标上取得 4 个 COM43 原始字节窗口与 1 次位级 I²C 被动采集/离线解码）
输入：`runtime.artifacts.delivered.g006.implementation` = `artifacts/firmware_implementation.md`（FWI-002 **rev 7**）；测试方案 = `artifacts/test_design.md`（TD-002 rev 7.0，`verification_scope=complete_delivery`）
负责角色：嵌入式测试

**结论：`TEST_FAIL`（完整交付）**

1. **受测身份闭合**：HEAD `f978a01`；`git diff --stat ef5caa6 HEAD -- USER test` 为空（产品源码与测试资产零变化）；独立 rebuild **0 Error / 1 既有 warning**，`Code=25964 RO=656 RW=116 ZI=1676`；`Project.axf` md5 **`a3b08774e764f174eac318f6771d0c42`**、`Project.hex` md5 `546ce20a718fa69b40177cce0145f4b5`；`mdk_flash {}` **`Verify OK / Application running`**。`Verify OK` 只证明下载校验。
2. **FW-REG-GXHT40 正常路径成立**：复位后取得 `S k=0`，随后 `k=6/9/12` 为**连续 3 个 3 分钟周期 `q=1`**，另有 `k=18`；共 5 个有原始行证据的周期全部 `q=1`、**0 条 `G`**；`t=243..247`（24.3–24.7 ℃）、`h=321..328`（32.1–32.8 %RH）均在有效域；行长 76–79 B（含 CRLF）≤96、CRLF 完整、无 NUL/杂散字节、采样间期无输出。
3. **F-1 语义在本次下载的新启动会话上继续成立**：首样本 `S k=0` 为 **`H=0 p=0`**；`p(k=9)=247=t(k=6)`、`p(k=12)=247=t(k=9)`，且 `p(k=12)=247 ≠ t(k=12)=246`。
4. **位级 I²C 证据（T4-04）本轮取得**：`fx2lafw:conn=2.32` 探测 exit 0，被动采集 8,000,000 样点 @4 MS/s（2.0 s），离线解码为 `0x44` 写 `0xFD` → `0x44` 读 6 字节 `65 7B B1 4E 1B CC` → NACK/STOP；独立复算双字 CRC-8 **均匹配**（温度 244 = 24.4 ℃、湿度 321 = 32.1 %RH）；命令 STOP→读 START **32.528 ms**、SCL 位周期 **614.25 µs（≈1.628 kHz）**。与 `BUS ack=44` 不矛盾。
5. **FW-REG-ASBUILT / FW-REG-REPORT 局部通过**：实现侧 `build_test.sh` **218 项 0 失败**；测试侧独立 harness **498 项 0 失败**（fw_core 59、gxht40 93、sf_i2c 51、diag_line 38、diag_probe 109、measure_flow 79、rf_frame 10、rtc 17、gw_compat 11、vdd 31）。静态不变量（T1 常量/升温方向、无旧通路、引脚所有权、有界等待、无浮点/动态内存、433 10 B / BLE 8 B 布局）保持。
6. **未满足的完整交付项**：
   - **FW-DIAG-BOOT / T4-01 横幅**：本运行时 `serial_capture` 与 `mdk_flash` 严格串行——串口会话在 `Application running` 后 **0.208 s** 才建立，仅捕获 `BUS` 与 `S k=0`；`BOOT/VDD/IOTEST/IOSIG` 四行与横幅 `uid` **未取得**。pyocd 未安装（`No module named pyocd`），未执行复位编排。
   - **FW-T5-LIGHT**：`LIGHT_DARK_CALIBRATED=0`、`REF_CODE=0`，全部周期 `V=0 D=0 o=8`（未标定门禁按设计工作）；无受光 + 受控全遮光**双态**窗口，未回填 `C_dark`。
   - **FW-E2E-POWER / T4-06 / E2E-03**：无遮光 + ≥1.0 ℃ 升温或稳定 >35 ℃ 刺激，无网关/手机同窗观测（本能力无 `android_test` 入口），全部周期 `r=0 s=0 y=0`、无 `P`、无发送。
   - **T4-03 失败路径**：本轮无 `q=0` 周期，失败语义与失败后重探未在板上触发（宿主证据只作背景）。
   - **E2E-04 功耗**：无电流表/SMU/示波器入口，未执行。
7. **证据缺口与功能失败分开记录**：本轮未发现新的固件功能缺陷签名（5/5 周期 `q=1`、双字 CRC 匹配、I²C 事务完整）；上述未满足项为采集编排、物理刺激/标定、跨链路与功耗仪器条件。按 TD-002 §6.7/§8，完整交付不能据此判通过。

---

## 1. 被测身份、构建与部署（本轮实际执行）

| 项 | 原始结果 |
|---|---|
| 受测提交 | **`f978a01`**（`[role:firmware_engineer]`；接手时 `git status` 干净） |
| 产品源码增量 | `git diff --stat ef5caa6 HEAD -- .../USER .../test` **为空**；实现记录声明本轮产品源码/工程/测试断言零改动 |
| 独立构建 | `mdk_build {"action":"rebuild"}`（08:22:12.896–08:22:14.880Z）→ **0 Error / 1 Warning**（既有 `main.c(254) while(k--);` 空循环体）；`Program Size: Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 产物身份（本能力独立复算） | `Project.axf` md5 **`a3b08774e764f174eac318f6771d0c42`**、`Project.hex` md5 `546ce20a718fa69b40177cce0145f4b5`——与 FWI-002 rev 7 / TD-002 rev 7 期望逐字符一致 |
| 部署 | `mdk_flash {}`（08:22:23.679–08:22:36.147Z）：`Erase Done.Programming Done.Verify OK.Application running ...`。**只证明下载校验** |
| 调试串口 | `serial_list_ports`（08:21:57Z）：`COM43` = Prolific `067B:2303` 在位（9600 8N1）；`COM26` = `C251:F001`（调试器 CDC，未使用） |
| 探针/手机/仪器 | `usb_list_devices`（08:21:58–08:22:00Z）：CW-DAPLink `C251:F001`、手机 `18D1:4EE7` SN `12F046690506`、FX2 `04B4:8613`（`fx2lafw`）在位；未枚举到示波器/电流表 |
| 逻辑分析仪 | `sigrok_probe`（08:22:08Z）`--driver fx2lafw:conn=2.32 --show` **exit 0**；随后 `sigrok_capture`/`sigrok_decode` 均 exit 0 |

## 2. 真实目标串口周期（COM43 @ 9600 8N1）

原始：`evidence/ev030/cap01_boot_bus_k0.cap` … `cap04_k18.cap`、`capture_windows.txt`、`tool_results.txt`；独立解析：`analyze_ev030.py` → `capture_analysis.txt`（不引用实现常量，全部检查通过）。

| 窗口（UTC） | 原始行（逐字节见 capture_analysis.txt） | 判读 |
|---|---|---|
| 08:22:36–08:24:36 | `BUS idle=3 scl=1 sda=1 ack=44`；`S k=0 p=0 H=0 V=0 o=8 n=3987 x=3995 a=3990 D=0 t=247 h=328 q=1 r=0 s=0 y=0` | 复位后首样本 `H=0 p=0`；`q=1`、无 `G` |
| 08:25:02–08:30:02 | `S k=6 p=248 H=1 V=0 o=8 n=3991 x=3996 a=3993 D=0 t=247 h=325 q=1 r=0 s=0 y=0` | `q=1`；`p` 指向未捕获的 k=3 |
| 08:30:17–08:35:18 | `S k=9 p=247 … t=247 h=322 q=1 …`；`S k=12 p=247 … t=246 h=324 q=1 …` | **`p(k=9)=247=t(k=6)`**；**`p(k=12)=247=t(k=9)`** 且 `p≠t` |
| 08:37:28–08:42:28 | `S k=18 p=244 H=1 … t=243 h=321 q=1 r=0 s=0 y=0` | `q=1`；`p` 指向未捕获的 k=15 |

判读：
1. **正常路径**：`k=6 → 9 → 12` 为连续 3 个采样周期（相邻 `k` 差 3），全部 `q=1`；另有 `k=0`、`k=18`，共 5/5 成功、0 条 `G`。
2. **有效域/格式**：`t=243..247`、`h=321..328` 在有效域；`S` 15 字段顺序与冻结格式一致；行长 76–79 B（含 CRLF）≤96；CRLF 完整、无 NUL、采样间期无输出。
3. **未标定门禁**：全部周期 `V=0 D=0`，`LIGHT_DARK_CALIBRATED=0` 门禁按设计工作；**不构成 T5 通过**。
4. **无触发**：全部周期 `r=0 s=0 y=0`，无 `P` 行；本轮未观测到上报/发送。

## 3. 位级 I²C（T4-04）

采集参数：`fx2lafw:conn=2.32`、D0=SCL、D1=SDA、触发 D1 下降沿、8,000,000 样点 @4 MS/s（2.0 s），工具时间 08:35:43.570–08:36:53.397Z；原始 `.sr` 为 `evidence/ev030/raw_i2c_capture.sr`（md5 `365feb35fb722225f4aeb5f79147fe7a`），离线 `i2c` 解码输出 `decoded_i2c.txt`。

```text
Start / Address write: 44 / ACK / Data write: FD / ACK / Stop
Start / Address read : 44 / ACK
Data read: 65 7B B1 4E 1B CC / NACK / Stop
```

| 测量 | 本轮值 | 说明 |
|---|---|---|
| 温度字 / CRC | `0x657B` / 计算 `0xB1` = 收到 `0xB1` ✅ | 换算 244 = 24.4 ℃ |
| 湿度字 / CRC | `0x4E1B` / 计算 `0xCC` = 收到 `0xCC` ✅ | 换算 321 = 32.1 %RH |
| 写事务 START→STOP | 12.050 ms | 0x44 地址 + 0xFD |
| 命令 STOP→读 START | **32.528 ms** | 与 F-3 已登记基线一致（配置 20 ms 的下限等待，方向安全） |
| 读事务 START→STOP | 40.162 ms | 6 字节 + NACK + STOP |
| SCL 位周期 | **614.25 µs（≈1.628 kHz）** | D2 慢时钟设定保持 |

该事务墙钟落在 cap03 结束与 cap04 开始之间（对应未采集的 k=15 周期）；解码值 244/321 介于 k=12（246/324）与 k=18（243/321）之间，属一致性推断，**不是同一窗口直接配对**。数字采样只证明逻辑电平与采样分辨率内时序，不证明模拟幅度/上升沿。

## 4. 宿主证据（同一源码）

| 入口 | 结果 |
|---|---|
| `test/build_test.sh`（实现侧，4 阶段） | 46+61+54+28+29 = **218 passed / 0 failed**，exit 0 |
| 测试侧独立 harness | fw_core 59、gxht40 93、sf_i2c 51、diag_line 38、diag_probe 109、measure_flow 79、rf_frame 10、rtc 17、gw_compat 11、vdd 31 = **498 passed / 0 failed** |
| 陈旧资产（历史登记，不计入证据） | `host_rf_report_verify_ev.c`（旧 `light_sample` API）编译失败；`host_light_verify_ev.c`（旧 mock 头 `ADC_InputCH11`/`CR_f`）编译失败 |

覆盖：T1 九组向量/严格边界/int32 极值、T2 冻结/重试/跨采样/不覆盖/P 行、T3 TXE/TXBUSY 卡死有界、T4 GXHT40 地址/CRC/换算/重探/恢复、1/3 判据与未标定门禁、433→网关→BLE→Android 字段映射、RTC 3 分钟节拍、VDD 反推公式。

## 5. 静态不变量（ASB-04 / T1-04 / T4-05 / BLD-04/05 / REG-07）

| 检查 | 结果 |
|---|---|
| 上报常量 | 仅 `sensor_config.h`：`SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350`；无 `SENSOR_REPORT_DROP_X10`、无 `cur==350` 整体排除 |
| 判定方向 | `fw_core.c`：`cur > 350` 超温分支 + `(cur-prev) > 9` 升温分支；int32 差值 |
| 旧通路 | `hall/optcfg/params/history` 仅出现在退役说明注释，无活动引用 |
| 引脚所有权 | PA03/PA04 仅软 I²C（`measure.c`）；PB05 光照供电、PB04/AIN11 光照 ADC（`light.c`）；PA05/PA06 仅 UART1（`debug_trace.c`）；PB06 未使用 |
| 有界等待 | `debug_trace.c::trace_wait_flag` 使用 `SENSOR_DEBUG_TX_GUARD=200000`；无无界 `while(TXE/TXBUSY)` |
| 命令边界 | 周期路径只用 `0xFD`；`0x94` 软复位仅定义未使用；general call `0x00/0x06` 仅上电一次性恢复（`measure.c:227-230`） |
| 浮点/动态内存 | `USER/src` 无 `float/double`、无 `malloc/calloc/realloc` |
| 接口/节拍 | `SENSOR_RF_FRAME_LEN=10`、`SENSOR_SAMPLE_TICKS=3`；433/BLE 布局由宿主 harness 逐字节验证 |
| 验证资产边界 | `git diff ef5caa6..HEAD` 对 `test/*_verify_ev.c`、`test/mock_*_ev/*`、`evidence/embedded_verify_*.py` 为空 |

## 6. 验收目标 → 本轮结果

| 任务 | 本轮结果 | 依据 |
|---|---|---|
| FW-REG-ASBUILT | **局部通过** | ASB-01..06：构建身份、宿主 218/0、独立 498/0、静态不变量、源码零功能改动、验证资产未改 |
| FW-REG-REPORT | **局部通过** | T1/T2/T3 宿主向量与卡死路径全部通过；T1-04/05、T3-05 静态通过 |
| FW-REG-GXHT40 | **失败（完整交付必需项未齐）** | T4-02 连续 3 周期 `q=1` ✅；T4-04 位级 ✅；T4-05 ✅；T4-01 横幅 `uid` 未取得；T4-03 无 `q=0` 未执行；T4-06 跨链路未执行 |
| FW-T5-LIGHT | **未执行（条件未满足）** | `V=0 D=0`；无双态/遮光窗口；`CALIBRATED=0` 未回填 |
| FW-DIAG-BOOT | **失败/未执行** | 仅取得 `BUS`；`BOOT/VDD/IOTEST/IOSIG` 与 `uid` 缺失；pyocd 未安装 |
| FW-E2E-POWER | **未执行** | 无遮光+升温/超温刺激、无网关/手机同窗、无功耗仪器 |

## 7. 失败报告（逐项关联目标、用例、身份与原始证据）

| # | 目标/用例 | 预期 | 实际 | 原始证据 | 已尝试措施 | 尚未排除的假设 | 最小下一步 |
|---|---|---|---|---|---|---|---|
| F-1 | FW-DIAG-BOOT / T4-01、BOOT-01/02 | 同一 COM43 窗口取得 `BOOT→VDD→IOTEST→IOSIG→BUS` 五行及 `fw/uid` | 仅 `BUS idle=3 scl=1 sda=1 ack=44` 与 `S k=0`；前四行在端口打开前已输出 | `cap01_boot_bus_k0.cap`、`tool_results.txt` §7–8 | 本轮 `mdk_flash` 已授权并与 `serial_capture` 同批发起；后端仍严格串行，端口在 `Application running` 后 0.208 s 建立；检查 pyocd → 未安装 | 是否存在本能力未登记、可“先开端口后复位”的编排入口（当前无） | 运行编排提供并发/预打开端口 + 窗口内复位；不得用后续 `S` 或历史 `uid` 替代 |
| F-2 | FW-T5-LIGHT / T5-01..09 | 受光 + 受控全遮光双态原始 `o/n/x/a`，满足 `受光 mean < ceil(C_dark/3)` 且 `全遮光 3*mean >= C_dark` 后由实现回填 | 全部周期 `V=0 D=0 o=8 n=3987..3992 x=3995..3998 a=3990..3994`；`CALIBRATED=0`、`REF_CODE=0` | `capture_analysis.txt`、`capture_windows.txt` | 记录当前原始光敏读数与门禁行为；未伪造暗态、未回填 | 现场当前光照状态无法由未标定固件分类；受光/全遮光窗口未提供 | 用户约定遮光开始/结束与恢复窗口（≥2 个采样周期）→ 记录 `o/n/x/a` 与遮光方式 → 实现回填 `C_dark` 并重建 → 三态复测 |
| F-3 | FW-E2E-POWER / E2E-01..06、T4-06 | 暗 + 升温 >0.9 ℃ 或 >35.0 ℃ 时 `r=1`、`P` 冻结、`s=1`，手机同笔一致 | 全部周期 `r=0 s=0 y=0`，无 `P`、无发送 | 同上 | 未构造刺激；本能力无 `android_test` 入口 | 网关/手机链路是否可用（未观测） | 遮光 + 局部升温/稳定 >35 ℃ + 网关/手机同窗；实板不伪造发送成功/失败 |
| F-4 | FW-REG-GXHT40 / T4-03 | `q=0` 周期后紧跟 `G`，失败零副作用，下一周期重探 0x44/0x45 并有界恢复 | 本轮 5/5 `q=1`、0 条 `G`，未触发失败路径 | `capture_analysis.txt` | 持续捕获 4 个窗口；无失败事件 | 失败路径是否在更长运行/不同供电条件下出现（未观测） | 出现真实 `q=0` 时按 `G` 原文取证；不得人为制造失败或改判据 |
| F-5 | E2E-04 | CR2032 采样峰值/深睡电流实测值与测量条件 | 未执行 | `tool_results.txt` §2 | 枚举未发现电流表/SMU/示波器 | 是否存在未登记测量入口（无） | 硬件测试在仪器具备时测量并记录量程/接线/电池电压 |

## 8. 证据局限（如实登记）

1. 串口窗口为分区 120/300 s，窗口之间存在未采集间隙；`k=3`、`k=15` 的 `S` 行未取得，节拍仅按已捕获 `k` 的步长与连续性判读。
2. 位级 I²C 仅 1 个事务（对应未直接配对的 k=15 周期），且为数字采样（4 MS/s、250 ns 分辨率），不证明模拟幅度/上升沿/上拉强度。
3. 上电五行与 `uid` 未取得；`BOOT` 横幅无法与产物摘要交叉核对（下载日志与 md5 已闭合）。
4. 无受光/全遮光双态、无升温/超温、无网关/手机同窗、无功耗仪器；相应任务记未执行，不写通过。
5. 陈旧测试资产 `host_rf_report_verify_ev.c`、`host_light_verify_ev.c` 编译失败，不计入本轮通过证据；相关独立回归覆盖为零。
6. 所有时间为 Windows 主机 UTC；串口行无设备时间戳，`k` 与墙钟的对应为推断；位级时间为“样点号 / 名义采样率”，不能跨设备同步。

## 9. 本轮实际改动（只增测试侧证据）

- 新增：`evidence/ev030/cap01_boot_bus_k0.cap`、`cap02_k6.cap`、`cap03_k9_k12.cap`、`cap04_k18.cap`
- 新增：`evidence/ev030/raw_i2c_capture.sr`、`decoded_i2c.txt`、`capture_metadata.json`、`decode_metadata.json`
- 新增：`evidence/ev030/analyze_ev030.py`、`capture_analysis.txt`、`capture_windows.txt`、`tool_results.txt`、`run_host_harnesses.sh` 及宿主日志
- 更新：`evidence/test.md`（本段置顶；EV-028 及更早原文保留）、`evidence/programmed_prototype.md`、`file_manifest.txt`
- **未修改**：`USER/**` 产品源码、Keil/IAR 工程、网关与 Android、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载软件或驱动

---

# 嵌入式验证证据（EV-028 / F-1 `S` 行前值语义修复轮复验）

状态：`embedded_tester.embedded_verification` 独立验证证据（本轮独立全量 rebuild、两次下载当前交付提交、在真实目标上连续 5 个 COM43 原始字节窗口，并尝试位级 I²C 被动采集）
输入：`runtime.artifacts.delivered.g003.implementation` = `artifacts/firmware_implementation.md`（FWI-002 **rev 6.4** / F-1 修复轮）；测试方案 = `artifacts/test_design.md`（TD-002 rev 6.0，`verification_scope=complete_delivery`）
负责角色：嵌入式测试

**结论：`TEST_FAIL`（F-1 修复已在实板成立；完整交付仍因 T4-02/T5/INT 未满足而失败）**

1. **F-1 修复在真实目标上成立（本轮核心新增事实）**
   - 复位后首个 `S k=0` 实测 **`H=0 p=0`**（修复前实测为 `H=1 p=t`，见 EV-026/EV-027）。
   - 后续周期 `p` 恒等于**上一周期**的 `t` 且 `p ≠ t`：A 会话 `p(k=6)=213=t(k=3)`、`p(k=9)=243=t(k=6)`；B 会话 `p(k=3)=263=t(k=0)`；跨越未捕获窗口的 `p(k=9)=260`、`p(k=18)=245` 均不等于本周期 `t`。
2. **回归**：7 条有原始行证据的 `S` 全部 `q=1`、**0 条 `G`**；`t=213..263`（21.3–26.3 ℃）、`h=369..451`（36.9–45.1 %RH）均在有效域；`k` 全为 3 的倍数；行长 76–79 B（含 CRLF）≤96，CRLF 完整、无杂散字节；采样间期无输出。
3. **T4-02 上电五行仍不完整（证据缺口）**：本运行 `serial_capture` 在 `mdk_flash` 返回后约 **73 ms** 才能打开端口，本轮仅捕获 `BUS idle=3 scl=1 sda=1 ack=44`；`BOOT/VDD/IOTEST/IOSIG` 在端口打开前已输出完毕，板件 `uid` 未取得。**不得用后续周期行替代**。
4. **T5 仍未执行**：无完全遮光窗口；全部周期 `V=0 D=0 o=8`（未标定门禁成立），未回填 `C_dark`，不得用亮态/估算值冒充。
5. **INT 上报/跨链路仍未执行**：全部 `r=0 s=0 y=0`，无 `P`、无发送、无手机同笔读数。
6. **位级补充（T4-07，可选）本轮不可用**：配置入口 `fx2lafw:conn=2.27` 返回 `No devices found.`；`usb_list_devices` 仍可见 FX2 身份 `04B4:8613`（`fx2lafw`）。USB 地址已变化，需由配置所有者更新；本轮不猜地址、不换用其他通道。**位级未覆盖**，不据此判器件或功能失败。
7. **宿主回归**：实现侧 `build_test.sh` **218 项 0 失败**；测试侧独立 harness **498 项 0 失败**（`diag_probe` 由 104 增至 109：按 F-1 语义更新 1 项旧断言并新增 5 项）。两个陈旧测试资产（`host_rf_report_verify_ev.c`、`host_light_verify_ev.c`）仍无法编译，属历史登记，不计入证据。

局部通过：F-1、T4-05（A 会话 k=3/6/9 连续 `q=1`；B 会话 k=0/3/9/18 全部 `q=1`）、T4-03（≥3 个采样机会/`S`-`G` 配对；本轮无失败周期）、T4-08（源码边界审计）、T1/T2/T3 宿主回归、BLD-01/02/03。T4-04/T4-06（失败周期判别与失败后重探）本轮因**无 `q=0` 周期**未重新取证，宿主机与上轮实板证据只作背景。
未满足（完整交付必需）：T4-02（上电五行缺失）、T5（全暗标定与三态）、INT-03..09（触发与跨链路）；T4-07 为可选补充且本轮不可用。

---

## 1. 被测身份、构建与部署（本轮实际执行）

| 项 | 原始结果 |
|---|---|
| 受测提交 | **`bee3d8f`**（`[role:firmware_engineer]`；接手时 `git status` 干净） |
| 本轮产品源码增量（相对上一受测 `d871957`） | `USER/src/measure.c` **+8/−2**：在 `temperature_process()` 推进前值前缓存判定所用 `prev/have`，`sensor_trace_fetch()` 的 `p/H` 改读该快照；判定逻辑仍用原变量。`test/host_measure_flow_check.c` +12（实现侧自检）。其余模块未变 |
| 独立构建 | `mdk_build {"action":"rebuild"}`（11:28:29–11:28:31Z）→ **0 Error / 1 Warning**（既有 `main.c(254) while(k--);` 空循环体）；`Program Size: Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 产物身份（本能力独立复算） | `Project.axf` md5 **`a3b08774e764f174eac318f6771d0c42`**、`Project.hex` md5 `546ce20a718fa69b40177cce0145f4b5`——与 FWI-002 rev 6.4 / BUILD-002 rev 6.4 声明逐字符一致，确定性重建 |
| 部署 | `mdk_flash {}` ×2：**11:28:38–11:28:51Z** 与 **11:39:34–11:39:47Z**，两次均为 `Erase Done.Programming Done.Verify OK.Application running ...`（日志 `evidence/ev028/flash_1.log`、`flash_2.log`）。**`Verify OK` 只证明下载校验，不表示功能合格** |
| 调试串口 | `serial_list_ports`（11:28:19Z）：`COM43` = Prolific `067B:2303` 在位（9600 8N1）；`COM26` = `C251:F001`（调试器 CDC，未使用） |
| 手机/探针 | `usb_list_devices`（11:53:40Z）：CW-DAPLink `C251:F001`、手机 `18D1:4EE7` SN `12F046690506` 在位；FX2 `04B4:8613` 在位但配置地址 `2.27` 不解析 |

## 2. F-1 实板复核（本轮核心）

原始：`evidence/ev028/cap01…cap05` 的 5 个 `.bin`、`tool_results.txt`；独立解析：`analyze_ev028.py` → `capture_analysis.txt`（不引用实现常量）。

| 会话 | 窗口（UTC） | 原始 `S` 行（逐字节见 `capture_analysis.txt`） | F-1 判读 |
|---|---|---|---|
| A（flash#1 后） | 11:28:56–11:33:56 | `S k=3 p=236 H=1 … t=213 h=451 q=1` | 首条捕获行：`p` 为上一周期 `t`（k=0，未捕获），`p≠t` |
| A | 11:34:22–11:39:22 | `S k=6 p=213 … t=243`；`S k=9 p=243 … t=250` | `p(k=6)=213=t(k=3)`；`p(k=9)=243=t(k=6)` ✅ |
| B（flash#2 后） | 11:39:47–11:44:47 | `BUS idle=3 scl=1 sda=1 ack=44`；`S k=0 p=0 H=0 … t=263`；`S k=3 p=263 H=1 … t=262` | **首样本 `H=0 p=0`** ✅；`p(k=3)=263=t(k=0)` ✅ |
| B | 11:45:16–11:50:17 | `S k=9 p=260 … t=241` | k=6 行落在窗口外；`p≠t`，且 `p=260` 即 k=6 的 `t` |
| B | 11:54:11–11:59:12 | `S k=18 p=245 … t=242` | k=15 行落在窗口外；`p≠t` |

判读：
1. **首样本语义修复成立**：复位后首条 `S` 的 `H=0 p=0`，与“判定无前值”一致；旧版同位置实测 `H=1 p=t`（EV-026/027）。
2. **前值语义修复成立**：连续周期 `p` 逐周期等于上一周期 `t`，且 `p≠t`；旧版 `p==t` 的签名未再出现。
3. **未引入业务回归**：7 条 `S` 全 `q=1`、0 `G`；`t/h` 在有效域；`k` 步长 3；行格式/行长不变（76–79 B）。
4. **`V=0 D=0 o=8`**：未标定门禁按设计工作；不构成 T5 通过。

## 3. 未执行项、证据缺口与最小下一步

| 项 | 状态 | 事实与原因 | 最小下一步 / 归属 |
|---|---|---|---|
| T4-02 上电五行 | **未满足** | 本运行时串口在 `Application running` 后约 73 ms 才打开；`BOOT/VDD/IOTEST/IOSIG` 已输出完毕，仅得 `BUS`；`uid` 未取得 | 需要“先开采集、窗口内复位”的编排能力（当前后端 `serial_capture`/`mdk_flash` 严格串行，无法并发）；不得用后续行替代 |
| T5 全暗标定 + 三态 | **未执行** | 无完全遮光窗口；`LIGHT_DARK_CALIBRATED=0`、`REF_CODE=0`，全周期 `V=0 D=0` | 用户配合遮光窗口 → 采 `o/n/x/a` → 实现回填 `C_dark` → 重建 → 三态复测（ext 责任见 TD-002 §10） |
| INT-03..09 上报/跨链路 | **未执行** | 无遮光/加温刺激，`r=0 s=0 y=0`，无 `P`；网关/手机同笔读数未观测 | 遮光 + 局部加温 + 网关/手机同窗；实板不伪造发送成功/失败 |
| T4-07 位级补充 | **不可用（可选）** | `fx2lafw:conn=2.27` → `No devices found.`；FX2 `04B4:8613` 仍在位 ⇒ USB 地址变化 | 配置所有者更新登记地址后重试；本轮位级未覆盖，不作失败或不通过判据 |
| 陈旧测试资产 | 历史登记 | `host_rf_report_verify_ev.c`（T2 前 API）、`host_light_verify_ev.c`（`light_result_t` 前 API）编译失败，未计入本轮证据 | 测试角色后续更新；不影响本轮结论 |

## 4. 宿主证据（同一源码）

| 入口 | 结果 |
|---|---|
| `test/build_test.sh`（实现侧，4 阶段） | 46+61+54+28+29 = **218 passed / 0 failed**，exit 0 |
| 测试侧独立 harness | fw_core 59、gxht40 93、sf_i2c 51、diag_line 38、diag_probe **109**、measure_flow 79、rf_frame 10、rtc 17、gw_compat 11、vdd 31 = **498 passed / 0 failed** |
| 本轮测试资产更新 | `test/host_diag_probe_verify_ev.c`：把“`p` 等于本周期测量值”的旧断言改为 F-1 语义（首有效周期 `H=0 p=0`），并新增“下一周期 `p=上一周期 t` 且 `p≠t`”等 5 项断言；**仅测试资产，产品源码未改** |

## 5. 结论

F-1 修复目标已在实板成立，且无 `q=1`/CRC/行长回归。但 `verification_scope=complete_delivery` 的必需项 **T4-02、T5、INT** 仍未取得所需实板证据（上电五行/标定/触发与跨链路），按退出准则 **`TEST_FAIL`**。本报告交当前实现者与条件提供方；修复后须以新构建重新取得实板证据，不得沿用本 PASS 子项作为整体通过。

---

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

---

# 嵌入式验证证据（EV-024 / D2 修复轮复验：GXHT40 读时钟速率单变量 A/B）

状态：`embedded_tester.embedded_verification` 独立验证证据（本轮**重新全量构建、重新下载当前交付提交、在真实目标上重新连续采集 81 分钟原始字节流**；EV-018/EV-021/EV-022/EV-023 只作对照，不作本轮结果）
输入：`runtime.artifacts.delivered.g003.implementation` = `artifacts/firmware_implementation.md`（FWI-002 **rev 6.2** / D2 修复轮）；测试方案 = `artifacts/test_design.md`（TD-002 rev 6.0，`verification_scope=complete_delivery`）
负责角色：嵌入式测试

**结论：`TEST_FAIL`（整体完整交付仍未通过；但本轮修复目标已在实板上成立）**

1. **D2 修复有效（本轮核心新增事实）**：新镜像在真实目标上连续 **27 个 3 分钟采样周期全部 `q=1`，`q=0` 为 0/27（0.0%）**，`k=3..81` 无一缺失；对照 rev 6.1 修复镜像为 **16/25（64.0%）失败**，旧镜像为 7/18（38.9%）失败。Fisher 精确检验双侧 **p = 1.97×10⁻⁷**。27 个周期内 **没有任何 `G` 行**（无读失败/CRC 错）。⇒ 支持 **H2′（主机读时钟速率兼容性）**，D2 单变量（软 I²C 半位延时 100→300）改变了失败形态。
2. **T4-05 成功判据在实板上成立**：连续 27 个周期 `q=1`（要求 ≥3），`t=197..245`（19.7–24.5 ℃）、`h=368..454`（36.8–45.4 %RH），均在有效域且与环境相符；相邻 `k` 差值恒为 3。
3. **T4-02 上电诊断行仍未取得（证据缺口，非产品判定）**：`serial_capture` 与 `mdk_flash` 在本运行时严格串行，复位发生在串口会话建立前约 5 s，`BOOT/VDD/IOTEST/IOSIG/BUS` 未落入窗口；板件 `uid` 未取得。不得用后续周期行替代。
4. **T5（全暗基准标定与 1/3 判据）仍未执行**：无完全遮光窗口；当前 `LIGHT_DARK_CALIBRATED=0`，全部周期 `V=0 D=0`（未标定门禁成立），无光升温分支在产品上仍不可用。
5. **INT 上报/跨链路仍未执行**：无遮光/加温刺激，27 个周期全部 `r=0 s=0 y=0`、无 `P` 行、无发送；手机/网关同笔读数未观测。
6. **逻辑分析仪入口仍不可达**：`fx2lafw:conn=2.31` → `No devices found`（09:09:52Z），位级 I²C 波形未覆盖；未改配置、未猜新地址。
7. **宿主证据全部通过**：实现侧 214 项 0 失败；独立 harness 59+51+93+38+104+79 = **424 项 0 失败**（本轮修复一项陈旧验证资产，见 §5.1）。

局部通过项（不改变整体结论）：T1 判定、T2 冻结/`P` 行、T3 有界等待的宿主证据；T4-01/03/05/06/08 与 T4-05 实板成功；T5-01 未标定门禁；INT-01/02/10；BLD-01/02/04。

---

## 1. 被测身份、构建与部署（本轮实际执行）

| 项 | 原始结果 |
|---|---|
| 受测提交 | `3113a9680985dcaf564839ccbe57bae161a0bcda`（HEAD；接手时工作区干净）。`git diff --stat HEAD -- <FW>/USER` = 空 ⇒ 产品源码逐字节等于 HEAD |
| 本轮产品源码增量（相对 rev 6.1 `0f8a8fb`） | `USER/inc/sensor_config.h` 新增 `SENSOR_I2C_DELAY_LOOPS 300u`；`USER/src/measure.c` 的 `i2c0_dev.speed`、`i2c0_swap_dev.speed` 与 `i2c0_halfbit_delay()` 改用该常量（原硬编码 100）。**仅改读时钟速率**，命令/CRC/量程/帧格式/采样相位未变 |
| 独立构建 | `mdk_build {"action":"rebuild"}`（07:48:02–07:48:04Z）→ **0 Error / 1 Warning**（既有 `main.c(245) while(k--);` 空循环体）；`Code=25912 RO-data=656 RW-data=116 ZI-data=1676` |
| 产物身份 | `Project.axf` md5 **`e56d8d4b48c14ff29ae376781bbdc915`**、`Project.hex` md5 `d872d703e0ed8ec95bc3dee88f074b33` —— 与实现 FWI-002 rev 6.2 声明逐字符一致，且由当前工作树**确定性重建** |
| 部署 | `mdk_flash {}`（07:48:11–07:48:23Z）：`Erase Done.Programming Done.Verify OK.Application running ...`。**`Verify OK` 仅证明下载校验，不表示功能合格** |
| 调试串口 | `serial_list_ports`（07:47:58Z）：`COM43 = Prolific USB-to-Serial 067B:2303` 在位（9600 8N1）；`COM26 = C251:F001`（调试器 CDC，未使用） |
| 探针/手机/FX2 | `usb_list_devices`：`C251:F001`（CW-DAPLink）、`12F046690506`（手机）、`04B4:8613`（Windows 名 "Unknown Device #1"）均在枚举中 |
| 逻辑分析仪 | `sigrok_info` 配置在（`fx2lafw:conn=2.31`、D0=SCL/D1=SDA）；`sigrok_probe`（09:09:52Z）→ exit 1、`No devices found.` |
| 板件身份 | 未取得 `BOOT … uid=` 横幅（§2.1），因此不能以 UID 证明与历史运行同一块板；可确认同一 COM43/DAPLink 链路与同一失败签名历史 |

原始工具结果：`evidence/ev024/tool_results.txt`；构建/下载日志摘要：§1 表；捕获清单：`evidence/ev024/capture_windows.txt`。

### 1.1 下载后行为变化（同链路、同一目标）

- 下载前（EV-023 最后观测，`08a4c266`）：k=96/99 均 `q=0`、`G s=4` 温度 CRC 错。
- 下载后首周期：`k=3`（复位后重新累计）、`q=1`、`t=214 h=405`。
- `k` 从 3 起以 +3 连续到 81，**中间无缺失周期**；下载前后 k 归零说明镜像确实重启（不是旧会话延续）。

---

## 2. 真实目标观测（COM43 @ 9600 8N1，原始字节）

原始文件：`evidence/ev024/cap01.cap` … `cap16.cap`（16 个连续 300 s 窗口，总 2130 B）。逐窗口与逐行见 `evidence/ev024/capture_analysis.txt`。

### 2.1 捕获窗口（UTC）与周期

| 捕获 | 窗口（UTC） | 字节 | 周期与结果 |
|---|---|---|---|
| cap01 | 07:48:28–07:53:28 | 78 | k=3 `q=1` |
| cap02 | 07:53:34–07:58:34 | 156 | k=6,9 `q=1` |
| cap03 | 07:58:38–08:03:38 | 158 | k=12,15 `q=1` |
| cap04 | 08:03:42–08:08:42 | 79 | k=18 `q=1` |
| cap05 | 08:08:47–08:13:47 | 158 | k=21,24 `q=1` |
| cap06 | 08:13:51–08:18:51 | 158 | k=27,30 `q=1` |
| cap07 | 08:18:56–08:23:56 | 158 | k=33,36 `q=1` |
| cap08 | 08:24:01–08:29:01 | 79 | k=39 `q=1` |
| cap09 | 08:29:05–08:34:05 | 158 | k=42,45 `q=1` |
| cap10 | 08:34:10–08:39:10 | 158 | k=48,51 `q=1` |
| cap11 | 08:39:13–08:44:14 | 79 | k=54 `q=1` |
| cap12 | 08:44:18–08:49:18 | 158 | k=57,60 `q=1` |
| cap13 | 08:49:23–08:54:23 | 158 | k=63,66 `q=1` |
| cap14 | 08:54:27–08:59:27 | 79 | k=69 `q=1` |
| cap15 | 08:59:32–09:04:32 | 158 | k=72,75 `q=1` |
| cap16 | 09:04:36–09:09:36 | 158 | k=78,81 `q=1` |

- 相邻窗口间隙 4–6 s（远小于 180 s 采样间隔）；**k 序列 3,6,9,…,81 连续无缺**，即每个采样周期都被观测到。
- 每窗口字节数恰等于窗口内完整行之和；`stray bytes = 0`，无半行/无截断。
- 采样间期无任何字节（UART 关闭语义成立，INT-02）。

### 2.2 周期统计与对照

| 组 | 构建 | 周期数 | `q=1` | `q=0` | 失败率 |
|---|---|---|---|---|---|
| 本轮 D2 镜像 | `e56d8d4b` | **27** | **27** | **0** | **0.0%** |
| rev 6.1 修复镜像（EV-022+EV-023） | `08a4c266` | 25 | 9 | 16 | 64.0% |
| 旧镜像（EV-018+EV-021） | `c7ed03b` 系 | 18 | 11 | 7 | 38.9% |

- **本轮 27 个周期没有任何 `G` 行**：无读 NACK、无 CRC 错、无量程无效。
- 失败率对照：0/27 vs 16/25，Fisher 精确检验双侧 **p = 1.97×10⁻⁷**；与旧镜像 7/18 对照亦为极显著。⇒ **失败率显著下降，不是抽样波动**。
- 有效读数：`t=197..245`（19.7–24.5 ℃）、`h=368..454`（36.8–45.4 %RH），在有效域内且相邻周期稳定（无发散）。
- `p` 字段逐周期等于上一成功周期的 `t`（首周期 `H=1 p=t=214`），前值推进语义成立；`r=0 s=0 y=0`（无触发、无发送）。
- 光照 ADC `a` 在 3087..3855 间变化（k=57 附近由约 3850 降到约 3090，环境光变化）；因 `LIGHT_DARK_CALIBRATED=0`，全部周期 `V=0 D=0`（未标定门禁成立）。

### 2.3 逐行原文（首/中/末示例）

```text
cap01  S k=3  p=214 H=1 V=0 o=8 n=3656 x=3664 a=3660 D=0 t=214 h=405 q=1 r=0 s=0 y=0
cap07  S k=36 p=198 H=1 V=0 o=8 n=3832 x=3843 a=3838 D=0 t=198 h=438 q=1 r=0 s=0 y=0
cap16  S k=81 p=221 H=1 V=0 o=8 n=3185 x=3199 a=3192 D=0 t=221 h=419 q=1 r=0 s=0 y=0
```

（对照：同一 `k=36`/`k=39`/`k=42`/`k=45`/`k=48`/`k=51`/`k=54`/`k=60` 在 rev 6.1 镜像上均为 `q=0`、`G s=4`；本轮同位置全部 `q=1`。）

### 2.4 行格式与预算

16 份捕获逐行计长（含 CRLF）：`S` 行 78–79 B；**0 行超过 96 B**；`S` 字段集合/顺序与 TD-002 §1.2 冻结格式完全一致（15 字段 `k p H V o n x a D t h q r s y`）；无未知行、无乱码、无空行。

---

## 3. 独立 CRC-8 复算与签名

`evidence/ev024/analyze_ev024.py`（独立实现 poly 0x31、init 0xFF、MSB-first，不引用固件常量）输出见 `evidence/ev024/capture_analysis.txt`：

| 项 | 结果 |
|---|---|
| `G` 行数量 | **0** |
| 温度 CRC 错 | 0 |
| 湿度 CRC 错 | 0 |
| `raw[2] == raw[0]` 签名 | 本轮**未出现**（无失败帧） |
| 行结构/字段/预算问题 | 0 |
| `k` 增量异常 | 0 |
| `q=0` 后缺 `G` / `q=1` 后多 `G` | 0 |

> 独立复算器与 EV-022/EV-023 使用的 CRC 约定一致；本轮无需对失败帧执行签名比对，因为不存在失败帧。只有出现 `G` 时才会打印逐帧 CRC 复算表。

---

## 4. D2 A/B 判读（关键结论）

FWI-002 rev 6.2 按 EV-023 归因 D2 执行**读时钟速率单变量**改动：软 I²C 半位延时 100→300 循环（约放慢 3 倍），采样相位保持 rev 6.1 的“SCL 高电平保持段末尾”。

| A/B 项 | 预期若有效 | 实测 |
|---|---|---|
| H2′ 主机读时钟速率（D2） | 失败率显著下降或签名消失 | **有效性成立**：25 周期 16 失败（64.0%）→ **27 周期 0 失败（0.0%）**，p=1.97×10⁻⁷；同编号 k 位置由 `G s=4` 全部转为 `q=1` |
| H1′ 器件第 3 字节输出内容 | 与主机速率无关 | **相对削弱**：主机降速后签名完全消失，说明“温度 MSB 覆盖第 3 字节”可由读时钟兼容性产生，无需假设器件内容本身错误 |
| H2 采样相位（rev 6.1） / H1 等待不足（rev 6.1） | — | 维持 EV-022 结论：两项均无效，不得重复 |

**判读边界**：本轮证明“当前板 + 当前器件 + 该软件降速”组合下失败不再出现；逻辑分析仪仍不在线，未取得位级 SDA 波形，因此不能外推为器件时序余量的定量结论，也不能证明所有温度/供电/批次条件下永久有效。但就 TD-002 T4-05 的实板成功判据，本轮已满足（连续 27 周期）。

---

## 5. 局部（宿主/构建/独立 harness）结果

本轮由测试能力实际重新编译并运行（输出原文见 `evidence/ev024/*.log`）：

| 项 | 方法 | 结果 |
|---|---|---|
| 实现侧宿主自检 | `test/build_test.sh` | **214 项 0 失败**（46+57+54+28+29），退出码 0 |
| T1 独立 harness | `test/host_fw_core_verify_ev.c` + 真实 `fw_core.c` | **59/59** |
| 软 I²C 独立 harness | `test/host_sf_i2c_verify_ev.c` + 真实 `sf_i2c.c` | **51/51** |
| GXHT40 独立 harness | `test/host_gxht40_verify_ev.c` + 真实 `gxht40.c`/`sf_i2c.c` | **93/93** |
| 诊断行独立 harness | `test/host_diag_line_verify_ev.c` + 真实 `debug_trace.c` | **38/38** |
| 总线诊断独立 harness | `test/host_diag_probe_verify_ev.c` + 真实 `measure.c`/`gxht40.c`/`sf_i2c.c` | **104/104** |
| 采样流程独立 harness | `test/host_measure_flow_verify_ev.c` + 真实 `measure.c`/`sf_i2c.c`/`fw_core.c` | **79/79**（本轮先修复陈旧资产，见 §5.1） |
| 合计独立 harness | — | **424/424** |

> 宿主证据只证明**原语/判定/格式**未被破坏，**不证明**实板 CRC 失败率；实板结论见 §2–§4。

### 5.1 本轮测试资产修复（test 范围环境自修，未改产品源码、未放宽断言）

`test/host_measure_flow_verify_ev.c` 仍按 T1 之前的旧“降温方向”语义编写：引用已删除的 `SENSOR_REPORT_DROP_X10`（编译失败），且两个断言期望“降温 + 暗 → 上报”。按 IC-002 v3.0 / TD-002 T1（升温 `cur-prev > 9` 且暗，或严格 `cur > 350`）修复：

1. 参考模型改为 `cur > HIGH` 或 `(have_prev && dark && cur-prev > RISE)`；删除 `cur < HIGH` 旧包裹与 DROP 项。
2. 脚本温度向量由 `210`→`230`（cycle 4）、`300`→`340`（cycle 11），使“失败周期不污染前值”和“当前周期光照态生效”两条断言在**升温**语义下仍为严格等式；断言数量与强度不变。
3. 复跑 **79/79 0 失败**。

未修改 `USER/**` 产品源码、Keil/IAR 工程、网关与 Android、`runtime/permissions.yaml`、`runtime/environment.yaml`。

---

## 6. 逐项验收状态（TD-002 rev 6.0；本轮为完整交付复验）

| 项 | 状态 | 依据 |
|---|---|---|
| T1 条件上报判定 | **局部通过**（L0/L1） | 独立 harness 59/59；实现侧 214 项；构建 0 Error |
| T2 冻结快照 + `P` 行 | **局部通过**（L0/L1）；实板 `P` 未覆盖 | 宿主流程/调试行通过；本轮无 `r=1`，无 `P` |
| T3 有界发送等待 | **局部通过**（L0/L1） | 宿主含 TXE/TXBUSY 卡死用例；实板采样间期 0 字节 |
| T4-01 构建/下载身份 | 通过 | commit + axf md5 + `Verify OK/Application running` 闭合 |
| T4-02 上电诊断完整性 | **未取得**（证据缺口） | `BOOT/VDD/IOTEST/IOSIG/BUS` 未落入窗口（§1/§2.1） |
| T4-03 窗口覆盖与 S/G 配对 | 通过 | 27 个周期连续无缺；`q=1` 周期无 `G` |
| T4-04 失败环节判别 | 本轮无失败可判别；历史判别表保留 | 27/27 无 `G` |
| T4-05 成功判据 | **通过（实板）** | 连续 27 周期 `q=1`，`t/h` 有效稳定，`k` 差值 3 |
| T4-06 失败语义/下周期重探 | 本轮无失败周期；宿主失败语义通过 | 历史 EV-022/EV-023 + 宿主 harness |
| T4-07 逻辑分析仪补充判别 | **NOT EXECUTED** | `No devices found` |
| T4-08 修复边界审计 | 通过 | 仅 `speed` 常量变化；CRC/换算/有效域/命令/布局未改 |
| T5-01 未标定门禁 | 通过 | 全 27 周期 `V=0 D=0`，`o=8` |
| T5-02/T5-04 全暗标定 | **NOT EXECUTED** | 无完全遮光窗口 |
| T5-07 三态复测 / T5-08 分离度 | **NOT EXECUTED** | 同上 |
| INT-01 节拍/单周期单次 | 通过 | k 序列 3..81 步长恒 3；每周期一条 `S` |
| INT-02 采样间期零输出 | 通过 | 窗口内行间字节 0 |
| INT-03/04/05 上报触发 | **NOT EXECUTED** | `V=0`（升温分支禁用）、无加温、`r=0` |
| INT-06 稳态不重复上报 | **NOT EXECUTED** | 无上报事件 |
| INT-07 `P` 行条件出现 | **NOT EXECUTED** | 无触发/发送尝试 |
| INT-08 失败周期不伪造 | 本轮无失败周期；历史通过 | EV-022/EV-023 |
| INT-09 跨链路一致性 | **NOT EXECUTED** | 无上报事件 |
| INT-10 `S` 行兼容/行预算 | 通过 | 逐行计长 78–79 B；字段核对一致 |
| INT-11 当前构建身份 | 部分 | commit/md5/下载校验闭合；横幅未捕获 |
| BLD-01/02/04 | 通过 | 214 项 0 失败；0 Error；静态审计 |
| BLD-03 部署身份 | 部分 | `Verify OK` + md5 一致；横幅缺失 |
| REG-02/03/07 | 通过（静态 + 捕获） | 布局/行格式未变；无旧语义常量 |

**判定**：本轮 **T4 的核心功能（每 3 分钟周期可靠取得有效温湿度）已恢复**，这是对上一轮 `TEST_FAIL` 缺陷 A 的直接实板反驳证据。但 T4-02 证据缺口仍在，且 T5、INT-03..07/09 未执行。按 TD-002 §8.3/§8.4，**整体仍为 `TEST_FAIL`**。

---

## 7. 失败/缺口报告（交回实现能力与运行时编排）

**缺口 A：上电诊断行未取得（证据问题，非产品功能失败）**
- 目标来源：TD-002 T4-02、BLD-03、INT-11。
- 身份：commit `3113a968`，axf `e56d8d4b48c14ff29ae376781bbdc915`，COM43@9600 8N1。
- 预期：`BOOT → VDD → IOTEST → IOSIG → BUS` 各一次且相邻，含板件 `uid`。
- 实际：本运行时 `serial_capture` 与 `mdk_flash` 严格串行；下载于 07:48:23Z 完成，首个捕获窗口 07:48:28Z 才开始，五行已在此前数秒内输出完毕。
- 已通过子项：新镜像运行与采样节拍、行格式、总线可用性（27/27 `q=1` 间接证明 I²C 地址/命令/CRC 通路正常）。
- 最小下一步：在可并发“串口先开 + 下载/复位”的编排下重取；不属固件缺陷，不得用后续周期行替代。

**缺口 B：T5 全暗标定与三态（条件未满足）**
- 需要：用户配合的完全遮光窗口（不透光遮蔽 J4 与受光路径，跨 ≥2 个采样周期），取得 ≥1 笔有效全暗 `o/n/x/a` 样本；随后回填 `C_dark` 并重建再复测。
- 现状：`LIGHT_DARK_CALIBRATED=0`，`V=0 D=0`；无光升温分支不可用。

**缺口 C：INT 上报与跨链路（条件未满足）**
- 需要：遮光 + 可控局部升温 ≥1.0 ℃（或稳定 >35.0 ℃）、网关供电/配对、手机 App 同窗观测。
- 现状：27 周期全部 `r=0`，无 `P`、无发送。

**缺口 D：位级 I²C 波形（工具不在线）**
- 需要：`fx2lafw` 入口在线（当前 `conn=2.31` → No devices found）并确认 D0=SCL/D1=SDA/共地。
- 影响：D2 有效性的定量时序解释未取得；不改变本轮实板功能结论。

**缺陷（上一轮）：GXHT40 温度 CRC 间歇失败** —— **本轮在当前构建上未复现（0/27）**，建议实现能力按“已修复候选”处理但仍保留历史失败证据；不得据此放宽 CRC 或删除诊断。

### 7.1 假设状态（本轮更新，未宣告永久结论）

| 假设 | 状态 | 依据 |
|---|---|---|
| H2′ 主机读时钟速率兼容性 | **支持（本轮实证）** | 仅把半位延时 ×3（100→300），失败 16/25→0/27；`dev->speed` 经 `i2c_delay()` 作用于每个半位（`sf_i2c.c:79`） |
| H1′ 器件第 3 字节输出内容缺陷 | **削弱（未完全排除）** | 主机降速后签名消失；但无位级 SDA 波形，不能证明器件在任何速率下输出都正确 |
| H1 等待不足 / H2 采样相位 | 已排除为充分原因 | EV-022 两项 A/B 均无效，不得重复 |
| H3 总线电气/上拉/接触 | 低概率保留 | 无位级/电气测量；但速率改变即改变边沿裕量，电气因素不能仅凭串口硬排除 |
| H4 MCU/编译器（`buf[0]`→`buf[2]`） | 低概率保留 | 源码无索引相关路径；本轮降速后不再出现，无需构建级对照 |
| H5 器件型号/装配 | 未决（本轮无目视/备件） | 与速率敏感性可并存；不影响本轮实板功能结论 |

**已尝试措施（累计）**：读采样点后移（rev 6.1，无效）→ 转换等待 10→20 ms（rev 6.1，无效）→ **读时钟速率 ×3（rev 6.2，本轮有效）**。前两项不得再作为修复重复。

---

## 8. 未执行项与所需条件

| 项 | 所需条件（当前未具备） |
|---|---|
| T5-02/T5-04/T5-07/T5-08 | 完全遮光窗口；回填 `C_dark` 后由实现重建再复测 |
| INT-03（无光+升温上报） | 遮光 + 可控局部加温 ≥1.0 ℃，且网关/手机在位观测 |
| INT-05（严格 >35.0 ℃ 上报） | 可控加温到 >35.0 ℃ 并保持 |
| INT-06/07/09 | 需先出现真实 `r=1` 上报事件（依赖上述条件） |
| T4-07 位级判别 | FX2 逻辑分析仪入口在线 |
| T4-02/BLD-03 横幅与 UID | 可并发“串口采集 + 下载/复位”的运行时编排 |

---

## 9. 本轮实际改动（测试侧）

- 新增：`evidence/ev024/cap01.cap` … `cap16.cap`（16 份 COM43 原始字节）
- 新增：`evidence/ev024/analyze_ev024.py`（独立 CRC-8/字段/节拍/配对复算）、`capture_analysis.txt`（其输出）、`capture_windows.txt`（窗口与字节数）、`tool_results.txt`（工具原始结果与时间戳）
- 新增：`evidence/ev024/host_build_test.log`、`host_fw_core_verify_ev.log`、`host_sf_i2c_verify_ev.log`、`host_gxht40_verify_ev.log`、`host_diag_line_verify_ev.log`、`host_diag_probe_verify_ev.log`、`host_measure_flow_verify_ev.log`
- 修改：`test/host_measure_flow_verify_ev.c`（验证侧陈旧语义修复，见 §5.1）
- 更新：`evidence/test.md`（本段置顶；EV-022 及更早原文保留）、`evidence/programmed_prototype.md`（EV-024 段置顶）
- **未修改**：`USER/**` 产品源码、Keil/IAR 工程、网关与 Android、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载软件或驱动

---

---

# 嵌入式验证证据（EV-022 / 修复轮复验：GXHT40 温度 CRC 间歇错误的两项时序 A/B）

状态：`embedded_tester.embedded_verification` 独立验证证据（本轮**重新全量构建、重新下载当前交付提交、在真实目标上重新采集原始字节流**；EV-018/EV-021 只作对照基线，不作本轮结果）
输入：`runtime.artifacts.delivered.g003.implementation` = `artifacts/firmware_implementation.md`（FWI-002 **rev 6.1** / 修复轮）；测试方案 = `artifacts/test_design.md`（TD-002 rev 6.0，`verification_scope=complete_delivery`）

**结论：`TEST_FAIL`**
1. **修复轮的两项时序 A/B 未产生任何可测改善**：同一块板、同一 COM43 链路，修复镜像实测 **19 个采样周期中 13 次 `q=0`（68.4%）**；对照（旧镜像 EV-018+EV-021）为 **18 周期 7 次失败（38.9%）**。失败率没有下降（Fisher 双侧 p=0.103，不能声称改善），且出现 **k=36→60 连续 9 个周期全部失败**。
2. **失败签名保持**：13/13 失败帧均为 **`G s=4`（温度字 CRC 错）**；湿度字与其 CRC **13/13 正确**；第 3 字节 11/13 精确等于温度 MSB，另 2 帧等于温度 MSB ±1 LSB。独立 CRC-8 复算与实现无关。
3. **T5（全暗基准标定与 1/3 判据）仍未执行**：无完全遮光窗口；`LIGHT_DARK_CALIBRATED=0` 使 `V=0/D=0`（未标定门禁成立），无光升温分支在产品上仍不可用。
4. **INT 上报/跨链路未执行**：无光照/加温刺激，`r=0`、无 `P` 行、无发送。
5. **上电诊断证据仍不完整**：本运行时 `serial_capture` 与 `mdk_flash` 串行执行（实测两调用时间戳不重叠），`BOOT/VDD/IOTEST/IOSIG/BUS` 未落入捕获窗口。
6. **逻辑分析仪入口仍不可达**：`fx2lafw:conn=2.31` → `No devices found`；位级时序/电气未覆盖（未改配置、未猜新地址）。

局部通过项（不改变整体结论）：T1 判定（独立 1764 向量 0 不一致）、T2 冻结/`P` 行与 T3 有界等待的宿主证据；T4 的失败语义（不推进前值/不构造 0/不冻结）、S/G 配对、行预算；T5-01 未标定门禁；INT-01 节拍、INT-02 采样间期零输出。

---

## 1. 被测身份、构建与部署（本轮实际执行）

| 项 | 原始结果 |
|---|---|
| 受测提交 | `0f8a8fbaa8e20f4aaaeb067a74af8084e0c0034c`（HEAD；接手时工作区干净）。`git diff --stat 7bfd248 HEAD -- USER` = **2 文件 +9/−3**：`USER/src/sf_i2c.c`（读字节 SDA 采样点移到 SCL 高电平保持段末尾）、`USER/inc/sensor_config.h`（`GXHT40_MEASURE_WAIT_MS` 10→20 ms），与 FWI-002 rev 6.1 声明一致 |
| 独立构建 | `mdk_build {"action":"rebuild"}` → **0 Error / 1 Warning**（既有 `main.c(245) while(k--);` 空循环体）；`Code=25912 RO=656 RW=116 ZI=1676` |
| 产物身份 | `Project.axf` md5 **`08a4c266af6a0f058821ec809d38f78d`**、`Project.hex` md5 `8fcfa1da9b7b204d83d36faf97b267e4` —— 与实现声明逐字符一致，且由当前工作树**确定性重建** |
| 部署 | `mdk_flash {}`（本地 13:57:34）：`Erase Done.Programming Done.Verify OK.Application running ...`。**`Verify OK` 仅证明下载校验，不表示功能合格** |
| 调试串口 | `serial_list_ports`：`COM43 = Prolific 067B:2303` 在位（9600 8N1）；未使用历史 COM42；`COM26` 为调试器 CDC，未使用 |
| 探针/手机/FX2 | `usb_list_devices`：`C251:F001`（CW-DAPLink）、`12F046690506`、`04B4:8613`（Windows 名 "Unknown Device #1"）均在枚举中 |
| 逻辑分析仪 | `sigrok_info` 配置在（`2.31`、D0=SCL/D1=SDA）；`sigrok_probe` → exit 1、`No devices found.` |

**板件身份**：本轮未取得 `BOOT … uid=` 横幅（见 §1.1），因此**不能**用 UID 证明与 EV-012..EV-021 为同一块板；可确认的是同一 COM43/DAPLink 链路、同一失败签名、且下载前最后一个周期（k=63）的 `G raw` 与下载后 k=3.. 的签名同族。此为证据局限，不影响"当前镜像在本目标上的失败"这一事实。

### 1.1 上电段证据缺口（本轮仍然存在）

本运行时两次 MCP 调用严格串行：`serial_capture` 05:55:21–05:57:22Z 结束后，`mdk_flash` 才于 05:57:22–05:57:34Z 执行。复位发生在串口已释放之后，`BOOT/VDD/IOTEST/IOSIG/BUS`（合计约数百 ms）全部落在无会话期间；随后启动的采集窗口从 05:57:45Z 开始，已晚于复位 11 s。**未取得上电五行**（T4-02 缺口），不以任何后续周期行替代。

---

## 2. 真实目标观测（COM43 @ 9600 8N1，原始字节）

原始文件：`evidence/ev022/cap01.cap` … `cap13.cap`（cap01 为下载前窗口，其余为下载后）。逐窗口与逐行见 `evidence/ev022/capture_analysis.txt`。

### 2.1 捕获窗口（UTC）

| 捕获 | 窗口（UTC） | 字节 | 周期与结果 |
|---|---|---|---|
| cap01 | 05:55:21–05:57:22（**下载前**） | 125 | k=63 `q=0` |
| cap02 | 05:57:45–06:02:46（**下载后**） | 118 | k=3 `q=0` |
| cap03 | 06:02:53–06:07:53 | 196 | k=6 `q=0`；k=9 `q=1` |
| cap04 | 06:07:59–06:12:59 | 204 | k=12 `q=1`；k=15 `q=0` |
| cap05 | 06:13:04–06:18:04 | 125 | k=18 `q=0` |
| cap06 | 06:18:09–06:23:09 | 79 | k=24 `q=1`（k=21 未见） |
| cap07 | 06:23:30–06:28:30 | 158 | k=27 `q=1`；k=30 `q=1` |
| cap08 | 06:28:36–06:33:36 | 204 | k=33 `q=1`；k=36 `q=0` |
| cap09 | 06:33:43–06:38:44 | 125 | k=39 `q=0` |
| cap10 | 06:38:49–06:43:49 | 250 | k=42 `q=0`；k=45 `q=0` |
| cap11 | 06:43:55–06:48:55 | 250 | k=48 `q=0`；k=51 `q=0` |
| cap12 | 06:49:12–06:54:12 | 250 | k=54 `q=0`；k=57 `q=0` |
| cap13 | 06:54:19–06:59:19 | 125 | k=60 `q=0` |

- 相邻窗口间隙 5–21 s（远小于 180 s 采样间隔）；每窗口字节数恰等于窗口内完整行之和，**无半行/无截断**。
- **k=21 未观测**：该周期处于 cap05 结束（06:18:04）与 cap06 开始（06:18:09）之间的 5 s 间隔附近；若 RTC 累计漂移使该行落入此间隙则不可见，也可能是该行未发出。**本项无法区分，如实登记为观测缺口**，不据此推断固件丢行。

### 2.2 周期统计

| 组 | 周期数 | `q=1` | `q=0` | 失败率 |
|---|---|---|---|---|
| 本轮修复镜像（08a4c266） | **19** | 6 | **13** | **68.4%** |
| 旧镜像对照（EV-018 15 周期 5 失败 + EV-021 3 周期 2 失败） | 18 | 11 | 7 | 38.9% |

- 连续 4 个周期 `q=1`：k=24,27,30,33（T4-05 的“连续 ≥3 周期”在该段成立）。
- **连续 9 个周期 `q=0`：k=36,39,42,45,48,51,54,57,60**——采集可靠性不成立。
- `k` 每周期 +3（3 分钟节拍成立）；每周期恰一条 `S`；`q=0` 周期均紧跟一条 `G`，`q=1` 周期无 `G`（S/G 配对成立）。
- 有效周期读数：`t=219..253`（21.9–25.3 ℃）、`h=371..432`（37.1–43.2 %RH），在有效域内。
- 失败周期 `p/t/h` 保持上一次有效值，`r=0 s=0 y=0`：**不上报、不推进前值、不构造 0、不冻结**（失败语义成立）。

### 2.3 逐行原文（失败周期示例）

```text
cap02  S k=3  p=0   H=0 V=0 o=8 n=3748 x=3755 a=3750 D=0 t=0   h=0   q=0 r=0 s=0 y=0
       G s=4 a44=1 a45=0 rd=0 at=3 raw=682568579EEE
cap03  S k=6  p=0   H=0 V=0 o=8 n=3705 x=3715 a=3710 D=0 t=0   h=0   q=0 r=0 s=0 y=0
       G s=4 a44=1 a45=0 rd=0 at=3 raw=64D1645CAF00
cap04  S k=15 p=219 H=1 V=0 o=8 n=3670 x=3684 a=3675 D=0 t=219 h=432 q=0 r=0 s=0 y=0
       G s=4 a44=1 a45=0 rd=0 at=3 raw=6433645F5E9E
cap13  S k=60 p=249 H=1 V=0 o=8 n=3704 x=3711 a=3706 D=0 t=249 h=372 q=0 r=0 s=0 y=0
       G s=4 a44=1 a45=0 rd=0 at=3 raw=6B5C6B503556
```

### 2.4 行格式与预算

13 份捕获逐行计长（含 CRLF）：`S` 行 72/78/79 B，`G` 行 46 B；**0 行超过 96 B**；`S` 字段集合/顺序与 TD-002 §1.2 冻结格式完全一致；无未知行、无乱码。采样间期无任何字节（UART 关闭语义成立）。

---

## 3. 独立 CRC-8 复算与失败签名（不引用实现常量）

`evidence/ev022/analyze_ev022.py`（poly 0x31、init 0xFF、MSB-first）与 `evidence/ev022/ev022_stats.py`：

| k | `raw`（6 B） | 温度字 | CRC 计算 | 收到 `raw[2]` | `raw[2]−raw[0]` | 湿度字 | 湿度 CRC | 湿度 CRC 正确 |
|---|---|---|---|---|---|---|---|---|
| 3 | `682568579EEE` | 0x6825 | 0x90 | **0x68** | 0 | 0x579E | 0xEE | 是 |
| 6 | `64D1645CAF00` | 0x64D1 | 0x62 | **0x64** | 0 | 0x5CAF | 0x00 | 是 |
| 15 | `6433645F5E9E` | 0x6433 | 0xC1 | **0x64** | 0 | 0x5F5E | 0x9E | 是 |
| 18 | `6504655E8213` | 0x6504 | 0x67 | **0x65** | 0 | 0x5E82 | 0x13 | 是 |
| 36 | `6B046A53D7D8` | 0x6B04 | 0x0A | **0x6A** | **−1** | 0x53D7 | 0xD8 | 是 |
| 39 | `6A4F6A590461` | 0x6A4F | 0x29 | **0x6A** | 0 | 0x5904 | 0x61 | 是 |
| 42 | `6B556B5727D9` | 0x6B55 | 0x45 | **0x6B** | 0 | 0x5727 | 0xD9 | 是 |
| 45 | `6B096B51091A` | 0x6B09 | 0x46 | **0x6B** | 0 | 0x5109 | 0x1A | 是 |
| 48 | `6A376A550D5D` | 0x6A37 | 0x68 | **0x6A** | 0 | 0x550D | 0x5D | 是 |
| 51 | `6AF86B5482FD` | 0x6AF8 | 0x01 | **0x6B** | **+1** | 0x5482 | 0xFD | 是 |
| 54 | `6B7E6B5062BF` | 0x6B7E | 0x29 | **0x6B** | 0 | 0x5062 | 0xBF | 是 |
| 57 | `6BCA6B5139DF` | 0x6BCA | 0x52 | **0x6B** | 0 | 0x5139 | 0xDF | 是 |
| 60 | `6B5C6B503556` | 0x6B5C | 0xCD | **0x6B** | 0 | 0x5035 | 0x56 | 是 |

**签名**：温度 CRC 错 13/13；湿度 CRC 错 0/13；第 3 字节 11/13 精确等于温度 MSB，2/13 与温度 MSB 相差 1 个 LSB（0x6A↔0x6B，单比特差）。温度字数据本身按手册公式换算为 23.5–29.0 ℃（k=36→60 的 9 帧为 27.6–28.7 ℃），**数据字不呈现随机损坏形态**；失败集中在第 3 字节。

> 与 EV-021 的差别：EV-021 的 8 个失败帧 8/8 精确等于温度 MSB；本轮新增 2 个“MSB±1”样本，说明该字节不是随机的，而是与温度 MSB 同源（当前或相邻转换的 MSB）。

---

## 4. 修复轮 A/B 判读（关键结论）

FWI-002 rev 6.1 对 H2 与 H1 各做了一项有界改动：

| A/B 项 | 改动 | 预期若有效 | 实测 |
|---|---|---|---|
| H2 主机读时序 | `i2c_read_byte()` 的 SDA 采样点由 SCL 上升沿后立即采样 → 高电平保持段末尾（半位总长不变） | 位采样裕量改善 ⇒ 失败率下降、签名消失 | **未改善**：19 周期 13 失败（68.4%），签名不变 |
| H1 器件输出/CRC 加载竞争 | `GXHT40_MEASURE_WAIT_MS` 10 → 20 ms（仍远大于手册 tMEAS.H max 8.3 ms） | 器件有时间装好 CRC ⇒ 失败率下降 | **未改善**：失败率高于对照 |

- 对照与实测量级：旧镜像 7/18（38.9%）→ 修复镜像 13/19（68.4%）。Fisher 精确检验双侧 **p = 0.1031**：没有证据表明失败率发生变化（更没有下降）。
- 由于两项 A/B 分别针对 **主机采样点** 与 **等待余量**，而失败形态逐字节保持，二者对“第 3 字节被温度 MSB 覆盖”的签名**均无影响**：
  - H2（主机位采样过早）**不被支持**：把采样点移到最后仍逐帧复现。
  - 器件侧解释（H1：输出寄存器/CRC 加载竞争；H5：器件型号/装配状态）**相对增强**，但仅凭串口仍不能单独定罪。
- 结论：**修复轮无效**。不得把“已改时序”当作修复，也不得据此放宽 CRC/量程/帧格式。

---

## 5. 局部（宿主/构建/独立 harness）结果

| 项 | 方法 | 结果 |
|---|---|---|
| 实现侧宿主自检 | `test/build_test.sh`（本能力实际执行） | **214 项 0 失败**，退出码 0（46+57+54+28+29，`evidence/ev022/host_build_test.log`） |
| T1 独立复核 | `evidence/ev018/t1_independent_check.py`（真实 `fw_core.c` × 独立 Python 参考） | **1764 向量 0 不一致** |
| T1 独立 harness | `test/host_fw_core_verify_ev.c`（真实 `fw_core.c`；含 572 组判定网格哈希） | **59/59**（本轮先修复测试资产，见 §5.1） |
| 软 I²C 独立 harness | `test/host_sf_i2c_verify_ev.c` + 真实 `sf_i2c.c` | **51/51**（采样点改动未破坏位级语义） |
| GXHT40 独立 harness | `test/host_gxht40_verify_ev.c` + 真实 `gxht40.c`/`sf_i2c.c` | **93/93**（命令白名单/CRC/重读重测/失败语义） |
| 诊断行独立 harness | `test/host_diag_line_verify_ev.c` + 真实 `debug_trace.c` | **38/38**（本轮修复测试资产，见 §5.1） |
| 总线诊断独立 harness | `test/host_diag_probe_verify_ev.c` + 真实 `measure.c`/`gxht40.c`/`sf_i2c.c` | **104/104** |
| 行预算/字段 | 13 份实板捕获逐行 | 0 行超 96 B；字段顺序/集合符合冻结格式 |
| 逻辑分析仪 | `sigrok_probe` | 入口不在线，位级未覆盖 |

> 宿主证据只证明**原语/判定/格式**未被破坏，**不证明**实板 CRC 失败率、也不替代 `q=1` 的实板要求。

### 5.1 本轮测试资产修复（test 范围环境自修，未改产品源码）

两项都是**验证 harness 自身陈旧**，不是产品缺陷；未放宽任何断言：

1. `test/host_diag_line_verify_ev.c` [S2]：该 harness 早于 T2 `P` 行。它把 `report=1` 的最坏样本的**整次输出**（现为 `S`+`P` 两行）按单行 96 B 预算判断，并断言“恰好一个 CRLF”，因此报 2 项失败。已改为**逐行**判断：每行 ≤96 B 且以 CRLF 结束（`evidence/ev022/patch_diag_line_harness.py`），复跑 38/38。
2. `test/host_fw_core_verify_ev.c` [T8/T8b]：期望仍为旧“降温方向”（D-01）。用独立参考模型（`evidence/ev022/ref_report_hash.py`，572 向量 FNV-1a = `0xA524C2E9`，与实现的观测哈希一致）重算并更新真值表；新增升温/降温/严格边界向量（`evidence/ev022/patch_fw_core_harness.py`），复跑 59/59。

两项修复均保留原有的严格等式，未删除、未弱化任何用例。

---

## 6. 逐项验收状态（TD-002 rev 6.0；本轮为完整交付复验）

| 项 | 状态 | 依据 |
|---|---|---|
| T1 条件上报判定 | **局部通过**（L0/L1） | 1764 独立向量 0 不一致；572 组网格哈希一致；构建 0 Error |
| T2 冻结快照 + `P` 行 | **局部通过**（L0/L1）；实板 `P` 未覆盖 | 宿主流程 57 项、调试行 54 项通过；本轮无 `r=1`，无 `P` |
| T3 有界发送等待 | **局部通过**（L0/L1） | 宿主含 TXE/TXBUSY 卡死用例；实板采样间期 0 字节 |
| T4-01 构建/下载身份 | 通过 | commit + axf md5 + `Verify OK/Application running` 闭合 |
| T4-02 上电诊断完整性 | **未取得**（证据缺口） | `BOOT/VDD/IOTEST/IOSIG/BUS` 未落入窗口（§1.1） |
| T4-03 窗口覆盖与 S/G 配对 | 通过（有 1 处观测缺口） | 19 个周期；`q=0` 后均紧跟 `G`，`q=1` 无 `G`；k=21 未观测 |
| T4-04 失败环节判别 | 完成（判别表见 §3） | `a44=1/a45=0/rd=0/at=3`；第 3 字节签名；独立 CRC 复算 |
| T4-05 成功判据 | **不通过（整体）**：连续 4 周期 `q=1` 成立，但 19 周期 13 失败 | §2.2 |
| T4-06 失败语义/下周期重探 | 通过 | `p/t/h` 不变、`r=0`、`at=3`、无 0 值 |
| T4-07 逻辑分析仪补充判别 | **NOT EXECUTED** | `No devices found` |
| T5-01 未标定门禁 | 通过 | 全 19 周期 `V=0 D=0`，`o=8` |
| T5-02/T5-04 全暗标定 | **NOT EXECUTED** | 无完全遮光窗口 |
| T5-07 三态复测 / T5-08 分离度 | **NOT EXECUTED** | 同上 |
| INT-01 节拍/单周期单次 | 通过 | k 增量恒 3；每周期一条 `S` |
| INT-02 采样间期零输出 | 通过 | 窗口内无行间字节 |
| INT-03/04/05 上报触发 | **NOT EXECUTED** | `V=0`（升温分支禁用）、无加温、`r=0` |
| INT-06 稳态不重复上报 | **NOT EXECUTED** | 无上报事件 |
| INT-07 `P` 行条件出现 | **NOT EXECUTED** | 无触发/发送尝试 |
| INT-08 失败周期不伪造 | 通过 | §2.2 |
| INT-09 跨链路一致性 | **NOT EXECUTED** | 无上报事件 |
| INT-10 `S` 行兼容/行预算 | 通过 | 逐行计长与字段核对 |
| INT-11 当前构建身份 | 部分 | commit/md5/下载校验闭合；横幅未捕获 |
| BLD-01/02/04 | 通过 | 214 项 0 失败；0 Error；静态审计 |
| BLD-03 部署身份 | 部分 | `Verify OK` + md5 一致；横幅缺失 |
| REG-02/03/07 | 通过（静态 + 捕获） | 布局/行格式未变；无旧语义常量 |

**判定**：T4 的核心功能（每 3 分钟周期可靠取得有效温湿度）仍不成立；T5、INT-03..07/09 未执行或缺必要证据。按 TD-002 §8.2/§8.4，**整体 `TEST_FAIL`**。

---

## 7. 失败报告（交回实现能力调查）

**缺陷 A：GXHT40 温度字 CRC 间歇失败在修复轮后仍存在（功能性，优先级最高）**

- 目标来源：`readme.txt` 修改点 1（GXHT40 采集）、TD-002 T4-04/T4-05/T4-06、INT-08；本轮修复输入 FWI-002 rev 6.1。
- 身份：commit `0f8a8fba`，axf md5 `08a4c266af6a0f058821ec809d38f78d`，COM43@9600 8N1，板 UID 未取得（横幅缺失），与 EV-018/EV-021 同链路同签名。
- 预期：每个 3 分钟周期 `q=1`，`t∈-400..1250`、`h∈0..1000` 且与环境相符。
- 实际：19 周期中 13 次 `q=0`（68.4%），全部 `G s=4 a44=1 a45=0 rd=0 at=3`；温度 CRC 13/13 错，湿度 CRC 0/13 错；第 3 字节 11/13 = 温度 MSB、2/13 = MSB±1；k=36→60 连续 9 个周期失败。
- 原始证据：`evidence/ev022/cap01..cap13.cap`、`capture_analysis.txt`、`stats.txt`、§2/§3。
- 已通过的子项：地址应答（`a44=1`）、读事务建立（`rd=0`）、湿度字与 CRC、失败语义（不推进/不构造 0/不上报/不冻结）、3 分钟节拍、行格式与预算、T1/T2/T3 宿主证据。
- 已尝试措施（本轮之前与本轮）：重新构建/下载、连续多窗口复测；**读采样点后移**（H2 A/B）；**转换等待 10→20 ms**（H1 A/B）。两项均未改变失败率与签名。
- 尚未排除的假设：H1 器件输出/CRC 加载竞争（**相对增强**）；H5 器件型号/真伪/装配状态（未决）；H3 总线电气（低，但无位级波形）；H4 MCU/编译器（低）。
- **最小下一步（建议实现能力，按信息量排序）**：
  1. 在不改协议/不放宽 CRC 的前提下，**记录同一采样周期内 3 次 attempt 的完整 6 字节**（当前只保留最后一次），用于判断 3 次是否读出**同一**错误帧（器件状态稳定）还是**不同**错误帧（随机竞争）；不改判定。
  2. 做一次**对照构建**：`GXHT40_MEASURE_WAIT_MS` 取更大值（如 50 ms）作为单变量，观察失败率是否变化——若仍不变，H1 的“等待不足”解释被进一步削弱（剩余为器件输出寄存器内容本身错误）。
  3. 若上述仍无效，按 EV-021 D3 交现场：核对 U9 型号丝印/装配方位、四焊点通断、换一只已知良好的 GXHT40（含 0x45 变体）后**不改代码**复测。
  4. 位级对照（需逻辑分析仪在线）：抓取一次失败读事务，确认第 3 字节时隙线上确为温度 MSB。

**缺陷 B：上电诊断行证据缺口（证据问题，非产品功能判定）**
- 预期：`BOOT → VDD → IOTEST → IOSIG → BUS` 各一次且相邻。
- 实际：本运行时的 `serial_capture`/`mdk_flash` 严格串行，复位时无串口会话，五行均未取得。
- 影响：T4-02/BLD-03 与板件 UID 绑定无法闭合；**不得**用后续周期行替代。
- 最小下一步：在能并发“串口采集 + 下载/复位”的会话中重取；属运行时工具编排限制，不是固件缺陷。

---

## 8. 未执行项与所需条件

| 项 | 所需条件（当前未具备） |
|---|---|
| T5-02/T5-04/T5-07/T5-08 | 完全遮光窗口（不透光遮蔽 J4 与受光路径，保持 ≥2 个采样周期）并提供有光/遮光/恢复三态；回填 `C_dark` 后由实现重建再复测 |
| INT-03（无光+升温上报） | 遮光 + 可控局部加温 ≥1.0 ℃，且网关/手机在位观测 |
| INT-05（严格 >35.0 ℃ 上报） | 可控加温到 >35.0 ℃ 并保持 |
| INT-06/07/09 | 需先出现真实 `r=1` 上报事件（依赖上述条件） |
| T4-07 位级判别 | FX2 逻辑分析仪入口在线（当前 `fx2lafw:conn=2.31` → No devices found） |

上述条件未提供时，按 TD-002 §8.3/§8.4 记录 `NOT EXECUTED`，**不是通过，也不作为豁免**。

---

## 9. 本轮改动与证据索引

**本轮新增（仅测试侧证据与测试资产，未改产品源码）**
- `evidence/ev022/cap01.cap` … `cap13.cap`（COM43 原始字节，cap01 为下载前窗口）
- `evidence/ev022/analyze_ev022.py` / `capture_analysis.txt`（逐行解析、独立 CRC-8、签名统计）
- `evidence/ev022/ev022_stats.py` / `stats.txt`（失败率对照/Fisher、失败帧换算）
- `evidence/ev022/ref_report_hash.py` / `ref_report_hash.txt`（T8 独立参考哈希）
- `evidence/ev022/patch_diag_line_harness.py`、`patch_fw_core_harness.py`（测试资产修复脚本）
- `evidence/ev022/host_build_test.log`、`host_sf_i2c_verify_ev.log`、`host_gxht40_verify_ev.log`、`host_diag_line_verify_ev.log`、`host_diag_probe_verify_ev.log`、`host_fw_core_verify_ev.log`、`t1_independent_check.txt`
- `evidence/ev022/tool_results.txt`（本轮全部设备/构建工具原始结果）
- **修改**：`test/host_diag_line_verify_ev.c`、`test/host_fw_core_verify_ev.c`（验证侧测试资产，见 §5.1）；`.gitignore`（忽略 `**/MDK/build*.log` 工具日志）
- **未修改**：`USER/**` 产品源码、Keil/IAR 工程、网关、Android、`runtime/permissions.yaml`、`runtime/environment.yaml`

**证据索引**：原始捕获 `cap01..cap13.cap`；分析 `capture_analysis.txt`/`stats.txt`；工具结果 `tool_results.txt`；构建/下载身份见 §1；历史对照见 `evidence/test.md` EV-018/EV-021。

---

# 嵌入式验证证据（EV-018 / 当前交付构建 T1–T5 全目标复验：GXHT40 采集、光照、节拍、调试通道）

状态：embedded_tester.embedded_verification 独立验证证据（本轮**重新构建、重新下载当前交付提交、重新采集真实目标原始字节流**；历史 EV-012..EV-017 只作背景，不作本轮结果）
本轮输入：`runtime.artifacts.delivered.g003.implementation` = `artifacts/firmware_implementation.md`（FWI-002 rev 6.0 / run10）；测试方案 = `artifacts/test_design.md`（TD-002 rev 6.0，`verification_scope=complete_delivery`）

**结论：`TEST_FAIL`**
1. **T4/GXHT40 采集存在可复现的间歇性缺陷**：当前交付构建 15 个采样周期中 **10 次 `q=1`、5 次 `q=0`（≈33% 失败）**。全部失败均为 **`G s=4`（温度字 CRC 错）**，且 6/6 失败帧（含下载前旧镜像 1 帧）呈现同一签名：**湿度字与湿度 CRC 全部正确，仅温度 CRC 字节 = 温度 MSB**（独立 CRC-8 复算，§2.4）。
2. **T5（全暗基准标定与 1/3 判据）未执行**：本轮未取得完全遮光窗口，`V=0/D=0` 恒成立；`LIGHT_DARK_CALIBRATED=0` 门禁使“无光 + 升温”上报分支在产品上尚不可用。
3. **INT 上报/跨链路未执行**：无无光/加温刺激，`r=0`、无 `P` 行；本轮未做网关/手机业务观测。
4. **上电诊断证据不完整**：本运行工具串行执行，`serial_capture` 与 `mdk_flash` 无法真正并发，仅捕到上电段尾部 `BUS idle=3 scl=1 sda=1 ack=44`；`BOOT/VDD/IOTEST/IOSIG` 未落入窗口（T4-02 证据缺口）。
5. **逻辑分析仪入口不可用**：配置的 `fx2lafw:conn=2.31` 实测 `No devices found`（`sigrok_probe`/`sigrok_capture` 均失败）；位级电气/时序未覆盖（可选 T4-07 未执行）。

局部通过项：T1 判定语义、T2 冻结快照、T3 有界等待的宿主证据通过（§3）；T4 的“连续 ≥3 周期 `q=1`”在 run2 的 `k=6..18`（5 个连续周期）成立，但**不能覆盖上述间歇 CRC 缺陷与证据缺口**。

- 受测提交：`7bfd248`（HEAD，`[role:firmware_engineer]`，传感器源码最后改动 = `7bfd248`；接手时 `git status` 干净；本轮**未改产品源码**）。
- 受测镜像：`mdk_build rebuild` → `0 Error / 1 Warning`（既有 `main.c(245) while(k--);`），`Code=25912 RO-data=656 RW-data=116 ZI-data=1676`；**axf md5 `c7ed03b796f2ae438b325c7b9306d836`**、hex md5 `79090d64d8e5487ee896983401f828ec`（与实现 `evidence/build.md` 声明逐字符一致）。
- 部署：`mdk_flash {}` ×2（本地 12:24:48、12:53:41）均 `Erase Done.Programming Done.Verify OK.Application running ...`；两次下载的是同一 axf（md5 未变）。`Verify OK` 仅证明下载校验，不表示业务功能合格。
- 验证基线：TD-002 rev 6.0、FWR-002 rev 6.0、FD-002 rev 6.0、IC-002 v3.0、`readme.txt`、`gxht40.pdf`。

| # | 本轮实测事实 | 性质 |
|---|---|---|
| 1 | 当前构建 **15 个采样周期：`q=1` 10 次 / `q=0` 5 次**；`k` 增量恒为 3（3 分钟节拍成立） | 采集不可靠（间歇失败） |
| 2 | 5 条失败 `S` 后均紧跟 `G s=4 a44=1 a45=0 rd=0 at=3`；失败周期 `p/t/h` 保持上一次有效值，`r=0 s=0 y=0`，未构造 0、未推进前值 | 失败语义正确；失败发生在 CRC 校验层（地址已 ACK） |
| 3 | **6/6 失败帧（含下载前旧镜像 1 帧）温度 CRC 不匹配、湿度 CRC 全部匹配、`raw[2]==raw[0]`**（§2.4 独立复算） | 高重复性缺陷签名，指向第 3 字节读取/器件温度 CRC 输出 |
| 4 | 上电段尾部 `BUS idle=3 scl=1 sda=1 ack=44`（当前构建） | 器件在 0x44 上应答；主机侧总线空闲电平正常 |
| 5 | 光照通路每周期 `o=8`、`n≤a≤x`、`a≈3099..3761`；`V=0 D=0` | ADC/GPIO 通路工作；全暗标定未做，1/3 判据未生效 |
| 6 | 无 `P` 行、`r=0` | 无上报触发；T2 实板冻结/重试路径未覆盖（宿主证据见 §3） |
| 7 | 所有捕获行 ≤96 B（含 CRLF），`S` 字段集合/顺序与 TD-002 冻结格式一致；采样间期无字节（UART 关闭语义成立） | 调试通道格式/功耗语义符合 |
| 8 | `sigrok_probe`/`sigrok_capture` → `No devices found`（`fx2lafw:conn=2.31`） | 位级未覆盖；不据此判器件好坏，也不扩权猜新地址 |

---

## 1. 环境、构建与部署（本轮实际调用，原始结果）

| 项 | 原始结果 | 结论 |
|---|---|---|
| `serial_list_ports` | `COM43 = Prolific USB-to-Serial (VID:PID 067B:2303)`；`COM26 = USB 串行设备 (C251:F001)` | **COM43 在位**，与 `runtime/environment.yaml` 登记一致；未使用历史 COM42 |
| `usb_list_devices` | CW-DAPLink `C251:F001`、手机 `18D1:4EE7` 实例含 `12F046690506`、FX2 `04B4:8613` | 探针/手机/FX2 USB 身份在位（不等于业务可用） |
| `mdk_build {action:rebuild}` | `0 Error(s), 1 Warning(s)`；`Code=25912 RO=656 RW=116 ZI=1676`；axf md5 `c7ed03b796f2ae438b325c7b9306d836` | 当前工作树可确定性重建受测镜像 |
| `mdk_flash {}` ×2 | 均 `Erase Done.Programming Done.Verify OK.Application running ...`（12:24:48 / 12:53:41） | 下载/回读校验成功；功能另判 |
| `serial_capture` ×10（COM43@9600 8N1） | 原始 `.cap` 见 `evidence/ev018/`；窗口与解析见 `evidence/ev018/capture_analysis.txt` | 原始字节已留档 |
| `sigrok_probe` / `sigrok_capture` | 均 `stderr: No devices found.`（`fx2lafw:conn=2.31`） | 配置地址不在线；未自行改配置 |
| `sigrok_info` | `enabled=true`、`connection=2.31`、`D0=scl/D1=sda`、`online=not_checked` | 仅配置读取，不代表在线 |

## 2. 真实目标观测（COM43 @9600 8N1，原始字节）

### 2.1 上电段

- 本轮**只捕到上电段尾部一条**：`BUS idle=3 scl=1 sda=1 ack=44`（31 B，`run2_boot.cap`，下载复位后）。`BOOT/VDD/IOTEST/IOSIG` 因工具串行执行（`serial_capture` 结束后才开始 `mdk_flash`）未落入捕获窗口。
- `ack=44` 表明 **GXHT40 在 0x44 地址上应答**（与 EV-012..EV-017 的 `ack=none` 不同 ⇒ 受测板硬件状态已变化，本轮结论只基于本状态）。

### 2.2 采样周期统计（当前构建）

| 运行 | 复位锚点（本地） | 观察到 `k` | `q=1` | `q=0` |
|---|---|---|---|---|
| run1 | 12:24:48（flash#1） | 3, 6, 9, 12, 15, 21, 24, 27（k=18 落在窗口间隙） | 3, 9, 21, 24, 27 = **5** | 6, 12, 15 = **3** |
| run2 | 12:53:41（flash#2） | 0, 3, 6, 9, 12, 15, 18 | 6, 9, 12, 15, 18 = **5**（**连续 5 个周期**） | 0, 3 = **2** |

- 合计：**15 个周期，`q=1` 10 次、`q=0` 5 次**；相邻 `k` 差恒为 3。
- run2 的 `k=6..18` 满足 TD-002 T4-05 的“连续 ≥3 个 3 分钟周期 `q=1`、`t/h` 在有效域、读数稳定”判据；但 run1 的 `k=6/12/15` 与 run2 的 `k=0/3` 失败表明采集**不可靠**。
- 有效读数示例：`t=230..259`（23.0–25.9 ℃）、`h=361..401`（36.1–40.1 %RH），均在有效域且与环境相符。
- 失败周期：`S … q=0 r=0 s=0 y=0` + 紧随 `G`；`t/h` 保持上次有效值（未构造 0、未推进前值）。

### 2.3 失败周期逐行原文

| 捕获 | `S` 关键字段 | `G` 行 |
|---|---|---|
| preflash（旧镜像，下载前） | `k=8673 p=256 … q=0` | `G s=4 a44=1 a45=0 rd=0 at=3 raw=67A667543C5D` |
| run1_a | `k=6 p=243 … q=0` | `G s=4 a44=1 a45=0 rd=0 at=3 raw=67176753FA12` |
| run1_c | `k=12 p=227 … q=0` | `G s=4 a44=1 a45=0 rd=0 at=3 raw=65356558B51B` |
| run1_c | `k=15 p=227 … q=0` | `G s=4 a44=1 a45=0 rd=0 at=3 raw=6310635E31FF` |
| run2_boot | `k=0 p=0 H=0 … q=0` | `G s=4 a44=1 a45=0 rd=0 at=3 raw=6454645B5002` |
| run2_a | `k=3 p=0 H=0 … q=0` | `G s=4 a44=1 a45=0 rd=0 at=3 raw=636B635DE85E` |

### 2.4 失败帧的独立 CRC-8 复算（本能力自有脚本）

`evidence/ev018/parse_captures.py` 以 `poly=0x31, init=0xFF` 对 `raw[0..1]` 与 `raw[3..4]` 分别复算：

| raw | 温度字 | 温度 CRC 计算/收到 | 湿度字 | 湿度 CRC 计算/收到 | `raw[2]==raw[0]` |
|---|---|---|---|---|---|
| `67A667543C5D` | `0x67A6` | `0x20` / **`0x67`** | `0x543C` | `0x5D` / `0x5D` | **是** |
| `67176753FA12` | `0x6717` | `0xAE` / **`0x67`** | `0x53FA` | `0x12` / `0x12` | **是** |
| `65356558B51B` | `0x6535` | `0x93` / **`0x65`** | `0x58B5` | `0x1B` / `0x1B` | **是** |
| `6310635E31FF` | `0x6310` | `0xBA` / **`0x63`** | `0x5E31` | `0xFF` / `0xFF` | **是** |
| `6454645B5002` | `0x6454` | 不匹配 / **`0x64`** | `0x5B50` | 匹配 / `0x02` | **是** |
| `636B635DE85E` | `0x636B` | 不匹配 / **`0x63`** | `0x5DE8` | 匹配 / `0x5E` | **是** |

⇒ 失败**仅**发生在温度 CRC 字节；湿度两字节与其 CRC 始终正确。该签名在下载前后的两个镜像上一致，而 `gxht40.c`/`sf_i2c.c` 自上一交付以来未改动 ⇒ 与本轮 T1–T3 改动无关，属采集驱动/器件/总线层的既有或硬件状态相关问题。

### 2.5 未取得/受限的观测

- `BOOT/VDD/IOTEST/IOSIG` 上电行（T4-02 缺口）；`P` 行（无上报触发）；`r=1` 周期；全暗三态；>35 ℃ 响应；手机端到端读数；I²C 位级波形（逻辑分析仪不在线）。

## 3. 局部（宿主/构建/静态）验证

| 项 | 方法 | 结果 |
|---|---|---|
| T1 判定语义 | **本能力独立 harness** `evidence/ev018/t1_independent_check.c/.py`：真实 `fw_core.c` × 21 组温度 × `dark/have_prev` 全组合，与独立参考公式（`cur>350` 或 `have&&dark&&cur-prev>9`，无界整数）逐条比对 | **1764 向量，0 不一致** |
| T1 常量/旧语义 | 源码审计 | `SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350`；无 `SENSOR_REPORT_DROP_X10`/降温语义 |
| T2 冻结快照 | 源码审计 + 实现侧宿主测试 | `s_report_temp_x10/hum_x10` 仅在 `rep!=0 && report_req==0` 写入；组帧只读快照（`measure.c:504/595`） |
| T3 有界等待 | 源码审计 + 实现侧宿主测试 | `trace_wait_flag()` 以 `SENSOR_DEBUG_TX_GUARD=200000` 计数有界；TXE/TXBUSY 均经该函数 |
| 实现侧宿主自检 | `test/build_test.sh`（本能力实际执行） | **46+57+54+28+29 = 214 项，0 失败**，退出码 0（`evidence/ev018/host_build_test.log`） |
| 构建 | `mdk_build rebuild` | 0 Error / 1 既有告警；镜像身份见文首 |
| 行预算/格式 | 10 份原始捕获逐行计长与字段校验 | 0 行超 96 B；`S` 字段集合/顺序符合 TD-002 §1.2 |
| 逻辑分析仪 | `sigrok_probe`/`sigrok_capture` | 入口不在线，位级未覆盖 |

## 4. 逐项验收状态（TD-002 rev 6.0 / readme）

| 项 | 状态 | 依据 |
|---|---|---|
| T1 条件上报判定 | **局部通过**（L0/L1） | 独立 1764 向量 0 不一致；构建通过 |
| T2 冻结快照 + P 行 | **局部通过**（L0/L1）；实板 P 未覆盖 | 宿主 57 项流程 + 54 项调试行通过；无上报触发 |
| T3 有界发送等待 | **局部通过**（L0/L1）；实板排空正常但卡死路径不可在板制造 | 宿主 54 项含 TXE/TXBUSY 卡死；采样间期无字节 |
| T4 GXHT40 实板采集 | **不完全通过**：连续 5 周期 `q=1` 达成，但存在 33% 间歇 CRC 失败；上电诊断证据不完整 | §2.1–2.4 |
| T5 全暗标定/1/3 判据 | **NOT EXECUTED** | 无完全遮光窗口；`V=0 D=0` |
| INT-01 节拍/单周期单次 | 通过 | `k` 增量恒 3；每周期一条 `S` |
| INT-02 采样间期零输出/关闭语义 | 通过 | 捕获中行间无字节 |
| INT-03/04/05 上报触发 | **NOT EXECUTED** | `V=0`（升温分支禁用）、温度 <35 ℃、无加温；`r=0`、无 `P` |
| INT-07 P 行条件出现 | **NOT EXECUTED** | 无触发/发送尝试 |
| INT-08 失败周期不伪造 | 通过 | `q=0` 时 `t/h` 保持上次有效值、`r=0`、无 0 值 |
| INT-09 跨链路一致性 | **NOT EXECUTED** | 无上报事件；未做网关/手机业务观测 |
| BLD-01/02/04 构建/配置/引脚 | 通过（静态+构建） | 见 §3 |
| BLD-03 部署身份 | 部分 | `Verify OK` + axf md5 一致；横幅未捕获 |
| REG-03/07 行兼容/旧语义清零 | 通过（静态+捕获） | 行格式与预算符合；无旧常量 |

## 5. 失败报告（交回实现能力调查）

**缺陷 A：GXHT40 温度字 CRC 间歇失败（功能性，待调查）**
- 目标来源：`readme.txt` 修改点 1（GXHT40 采集）、TD-002 T4-04/T4-05/T4-06、INT-08。
- 对应用例与身份：T4-03/04/05/06；受测 commit `7bfd248`，axf md5 `c7ed03b796f2ae438b325c7b9306d836`，目标板 MCU `uid` 未在本轮捕获（横幅缺失），同一 COM43/DAPLink 目标。
- 预期：每个 3 分钟采样周期 `q=1`，`t∈-400..1250`、`h∈0..1000` 且与环境相符。
- 实际：15 周期中 5 次 `q=0`，全部 `G s=4`；温度 CRC 不匹配；湿度 CRC 全部正确；`raw[2]==raw[0]` 6/6。
- 原始证据：`evidence/ev018/*.cap`、`capture_analysis.txt`、§2.3/§2.4。
- 已通过子项：地址 ACK（`a44=1`）、湿度字/CRC、失败语义（不推进前值/不构造 0/不上报）、下一周期重试（`at=3`）、总线空闲电平。
- 未测项：位级 I²C 波形（分析仪不在线）；`VDD` 上电读数（横幅缺失）；器件断电重启后的对照。
- 已尝试措施：重新构建/下载当前交付件；连续 2 次运行、16 个周期复测；独立 CRC 复算定位到第 3 字节。
- 尚未排除假设：① 器件温度 CRC 输出异常（器件/批次行为）；② 第 3 字节的 ACK/采样时序或 clock-stretching 交互；③ SDA 建立/保持裕量不足（当前软 I²C 半位约 50–100 µs，需位级确认）；④ 供电瞬态/接触间歇（本项无示波器/电流表入口）。
- 最小下一步（建议实现能力）：在**不放宽 CRC/有效域**的前提下，用位级 mock/分析仪核对第 3 字节的 ACK 与采样点；按 TD-002 H3 允许范围做一次有界 A/B（半位延时/读取等待）并比较失败率；若 A/B 无效，保持现状并把器件侧作为候选交现场（换件/断电复位对照）。不得把本缺陷写成“已修复”或以宿主通过替代实板证据。

**缺陷 B：上电诊断行证据不完整（证据缺口）**
- 目标来源：TD-002 T4-02、BLD-03。
- 预期：`BOOT → VDD → IOTEST → IOSIG → BUS` 各一次且相邻。
- 实际：仅捕到 `BUS idle=3 scl=1 sda=1 ack=44`；其余四行因工具串行执行未落入窗口。
- 最小下一步：在能真正并发“采集 + 复位/下载”的工具会话中重取；不得用后续周期行替代。

**未执行项及所需条件**
- T5：完全遮光窗口（不透光遮蔽 J4/受光路径，保持 ≥2 个采样周期），取 ≥1 笔有效 `o/n/x/a`，由实现回填 `C_dark`/`CALIBRATED=1` 后重建，再做有光/遮光/恢复三态复测。
- INT-03/05：无光 + ≥1.0 ℃ 升温、或稳定 >35.0 ℃ 的可控加温窗口；以及网关/手机在位观测。
- INT-09：一次真实上报事件的手机端读数与 `S/P` 对照（App 业务判定不属本能力）。

## 6. 证据索引

- 原始捕获（10 份）：`evidence/ev018/{preflash_old_image,run1_a,run1_b,run1_c,run1_d,run1_e,run2_boot,run2_a,run2_b,run2_c}.cap`
- 下载日志：`evidence/ev018/flash_1.log`、`flash_2.log`
- 宿主自检：`evidence/ev018/host_build_test.log`
- 捕获解析与 CRC 复算：`evidence/ev018/parse_captures.py` → `evidence/ev018/capture_analysis.txt`
- T1 独立复核：`evidence/ev018/t1_independent_check.c` / `.py`
- 窗口时间表：见 `evidence/ev018/capture_windows.txt`

---

# 嵌入式验证证据（EV-017 / run8 ITEM-002 ↔ GXHT40 温湿度采集在真实目标上恢复有效）

状态：embedded_tester.embedded_verification 独立验证证据（本轮**重新构建下载、重新采集真实目标原始字节流**；历史结论只作背景，不作本轮结果）
本轮触发：Runtime 当前调用（`n005_embedded_tester_embedded_verification`，ITEM-002 复验）。承接实现能力对 EV-016 的交付件（rev 5.6：新增**上电供电轨测量** `light_read_vdd_mv()`（ADC 内部 BGR1.2V 反推 VDD）与上电行 `VDD ok= code= bgrmv= mv=`），用于把 EV-016 §6 登记的「唯一从未被测量的物理量 = 供电轨」实测掉。

**结论：`TEST_FAIL`** —— 本项核心可观察「真实目标每个 3 分钟采样周期 `S` 行 `q=1`、`t/h` 落在有效域（温度 −40.0..125.0 ℃、湿度 0..100.0 %RH）、连续多个周期稳定」在**当前构建、当前受测板**上**仍未达成**：1 次成功下载、4 个首尾相接的采集窗口跨 20 分钟、**7 个连续采样周期全部 `q=0`**。

**本轮新增的判别事实（供电轨实测）**：下载后的镜像实测 **`VDD ok=1 code=1480 bgrmv=1189 mv=3289`** —— 供电轨 **≈3.289 V**，落在 GXHT40 的 `VDD 1.6..5.5 V`（典型 3.3 V，手册电气表，本轮亲自复核）内、且为**新鲜 CR2032 的量级**。网表（本轮亲自重解析）证明 `U9.3(VCC)` 与 `U7.9(MCU VDD)` **同在 `BAT` 网、无开关、无串联元件**，因此该电位就是**传感器供电网**的电位。⇒ 「电池枯竭 / 供电轨塌陷 / 固件可控供电门」这一支**被实板数据排除**；解释面收缩到 **U9 自身的焊点连通性 / 器件本体**。该结论**不**等于「传感器得电正常」：测点在 MCU 侧，`U9.3` 焊点开路时该网仍会读到 3.289 V。本能力**不**代替实现者判定「不可修复」，**不**以仪器缺失作为门槛，**不**放宽或绕过任何判据，**不**虚构数据。

- 受测提交：`8ae4a33`（HEAD；接手时 `git status` 干净；本轮**未改产品源码**）
- **被测件 = 当前交付件本身**：`git log -1 -- <FW>/USER` = `8ae4a33`；本轮 `mdk_flash {}` 所下 axf md5 = **`5fc16235ccd043f4edb48b16ee44da0b`**（hex `9a32cf3bcdbca2c1346da9c3bb3581da`），与实现交付声明**逐字符一致**；且本能力**独立执行 `mdk_build {"action":"rebuild"}`** 后 md5 **不变**（同一字节）⇒ 所下镜像可由当前工作树确定性重建（见 `evidence/ev017/image_identity.txt`）。
- 本轮交付的实际源码增量（`git diff --stat 55ed979 HEAD -- <FW>/USER`）= **5 个文件、+136/−0 行**：`USER/src/light.c`（+67：`light_read_vdd_mv()`）、`USER/inc/light.h`（+10）、`USER/src/debug_trace.c`（+32：`debug_trace_vdd()` 渲染）、`USER/inc/debug_trace.h`（+20）、`USER/src/main.c`（+7：上电顺序 `BOOT→VDD→IOTEST→IOSIG→BUS`）。**`measure.c`/`gxht40.c`/`sf_i2c.c`/`fw_core.c` 本轮零改动**。
- 验证基线：TD-002 rev 5.0；FWR-002 v5.0（FWR-116/117/118）；FD-002 rev 5.0；`readme.txt`；`gxht40.pdf`（GXHT4x Family Datasheet，本轮重新提取电气表与 §7.7）；`sensor_hardware/{pstxnet.dat,MAIN_BOARD.BOM}`（本轮重新解析网表）。

| # | 本轮事实（全部为本轮新采集） | 性质 | 依据 |
|---|---|---|---|
| 1 | 实板 **0/7 个采样周期 `q=1`**；核心可观察（`q=1`、`t/h` 有效域、连续多周期稳定）**未达成** | 功能不符（本项核心验收不通过）⇒ `TEST_FAIL` | §2.2 |
| 2 | **`VDD ok=1 code=1480 bgrmv=1189 mv=3289`**：供电轨 ≈3.289 V；`mv` 由本能力**独立复算** `4095*1189/1480 = 3289`（整数除法）逐位相符 | EV-016 登记的「唯一从未被测量的物理量」**已被实测覆盖**；**排除**电池枯竭/轨塌陷 | §2.5、§4 |
| 3 | 网表（本轮亲自重解析）：`BAT` = {J1.1(CR2032+), **U7.9(MCU VDD)**, **U9.3(GXHT40 VCC)**, R1.1, R2.2, L2.2, C7.2, C8.1, C9.1, C13.1, C14.1, C15.1, C16.1, U4.6}；`N05845`={R1.2, U7.14/PA04, U9.1/SDA}；`N06024`={R2.1, U9.2/SCL, U7.13/PA03}；`GND` 含 U9.4；`NC` 含 U9.5 | 测得的电位即**传感器供电网**电位（测点在 MCU 侧，不能证明 U9 焊点连通） | §2.5 |
| 4 | 手册电气表（本轮重新提取）：`VDD 1.6..5.5 V`（典型 **3.3 V**）、`VPOR ≤ 1.0 V`、`tPU ≤ 1 ms`、`tMEAS.H ≤ 8.3 ms`；全系列仅 `0x44`/`0x45` 两个 I²C 地址；§7.7 条目 2 = I²C general call `0x00` + `0x06` | 3.289 V 在器件供电域内且 ≈典型值；固件等待（10 ms/2 ms）≥ 手册上限 | §2.5、§3.2 |
| 5 | 7 条失败周期 `G` 行**逐字节相同**：`G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`；`BUS … ack=none`；`gc=3`；`IOSIG scl=95555 sda=30303`（与前几轮同名行逐字符一致） | 失败形态稳定可复现；器件对**任何** I²C 地址无响应 | §2.1、§2.2 |
| 6 | `r1→s1→s2→s3` 四窗口首尾相接（05:29:11→05:49:13，间隙 0.40–0.43 s），构成 **`k=0,3,6,9,12,15,18` 七个连续周期（≈21 分钟）**，器件全程无应答、无自愈 | 失败稳定、非偶发；排除「需等待自愈」 | §2.2 |
| 7 | **下载生效的实板自证**：窗口起点（下载前）那段是 rev 5.5 镜像（`IOTEST` 5 字段带 `gc=3`、**无 `VDD` 行**），下载复位后的段为 rev 5.6（**`BOOT→VDD→IOTEST→IOSIG→BUS`**、含 `VDD` 行） | 板上镜像确实由 rev 5.5 换为 rev 5.6，且受测件就是本轮交付件；上电顺序与交付声明一致 | §2.1 |
| 8 | 上电行 `VDD …` ×1、`IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=3` ×2、`IOSIG scl=95555 sda=30303` ×1、`BUS … ack=none` ×1、横幅 `uid=6A002C00`（与 EV-012..EV-016 同一块受测板） | 主机侧行为可复现、同一受测板（无换板混淆） | §2.1 |
| 9 | 光照通路持续工作：`o=8` 恒成立、每周期 `n≤a≤x`、**周期内 min/max 离散度 ≤3 码**，跨周期 `a` = 33..35（与 EV-016 同期 30–31、EV-015 的 32–35 一致） | 交叉对照：MCU/时钟/ADC/GPIO/PWR/UART1 通路正常；环境偏暗但有光 | §2.4 |
| 10 | 失败周期**不上报、不推进前一有效温度、不构造 0 值**：`p=0 H=0 V=0 D=0 q=0 r=0 s=0 y=0`、`raw=------------` | 冻结不变量在实板成立 | §2.2 |
| 11 | 本能力自有宿主 harness 对当前交付源码复跑 **529 项 0 失败**（104/93/51/36/56/79/79/**31**；31 = 本轮新增 `host_vdd_verify_ev.c`）；判定脚本 `--selftest` **32/32**；加载实板 4 份捕获判定 **86 通过 / 1 失败（唯一失败 = 核心验收 `H ACCEPT`）** | 实现符合性**未被推翻**；判据非空验（含新增 `VDD` 的公式/域/顺序正负对照） | §3 |

---

## 1. 环境、构建与部署（本轮实际调用，原始结果）

| 工具 / 命令 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `COM42 = Prolific USB-to-Serial Comm Port (VID:PID 067B:2303)`；`COM26 = USB 串行设备 (C251:F001, SN 87094109484987710672FF50)` | 传感器调试口 **COM42 在位**，与配置声明的设备身份一致 |
| `md5sum …/Project.axf` / `.hex`（下载前） | `5fc16235ccd043f4edb48b16ee44da0b` / `9a32cf3bcdbca2c1346da9c3bb3581da` | 与实现交付声明**完全一致** ⇒ 受测件身份闭合 |
| `mdk_flash {}` ×1（本地 13:29:15→13:29:25） | `Erase Done.Programming Done.Verify OK.Application running ...`；下载前 build 为 `0 Error(s), 0 Warning(s)`（无可重建项 ⇒ 所下即上面那个 axf） | 当前交付镜像已下载并**回读校验成功**。`Verify OK` **仅**证明下载校验，业务功能仍须实测（§2） |
| `mdk_build {"action":"rebuild"}` ×1（本能力独立执行，本地 13:52:58→13:53:00） | `0 Error(s), 1 Warning(s)`（唯一告警 = 既有模板 `main.c(245) while(k--);` 空循环体）；`Program Size: Code=20100 RO-data=644 RW-data=116 ZI-data=1676` | 全量重编译后 axf/hex md5 **不变** ⇒ 所下镜像可由当前工作树**确定性重建**；RAM 1,792 B ≤ 4 KB、Flash 20,744 B ≤ 64 KB（与实现 `evidence/build.md` 声明逐项一致） |
| `serial_capture` ×4（COM42@9600，300 s/次；`r1` 与 `mdk_flash` 串接于同一 shell 脚本，使复位横幅落在窗口内；四个窗口首尾相接） | `evidence/ev017/{r1,s1,s2,s3}.cap`（528/224/113/226 B，逐字节账目闭合） | 原始字节已入库；窗口与 md5 见 §2.6 |
| `git status --short`（提交前） | 仅本能力声明的输出：新增 `evidence/ev017/`、`test/host_vdd_verify_ev.c`、`test/mock_vdd_ev/`，修改 `evidence/embedded_verify_gxht40_item002.py`、`evidence/test.md`、`evidence/programmed_prototype.md`、`file_manifest.txt` | 产品源码（`USER/**`）与 Keil 工程**零改动**；无无关改动被卷入提交 |

授权与入口：本轮 context 的 `当前能力有效工具` 与 `本次允许调用的工具及参数` 均非空（9 个工具），实际调用 4 个（`serial_list_ports`、`mdk_flash`、`serial_capture`、`mdk_build`），未用 `usb_list_devices`（探针身份已由 `serial_list_ports` 的 hwid `C251:F001` 给出）、未用 `serial_exchange`（本项无「发送已有协议指令」的需要：失败发生在地址探测层，器件不应答，无命令可发）、未用 `install_*`/`android_test`（理由见 §6）。`mdk_flash`/`mdk_build`/`serial_capture` 经 hardware-verification 工具入口（脚本 `evidence/ev017/seq.sh`，使用**本轮** context 路径 `CALL-ae8aa78a-…`）。

## 2. 真实目标 UART1 观测（COM42 @9600 8N1，原始字节）

### 2.1 上电行（BOOT / VDD / IOTEST / IOSIG / BUS）

| 捕获 | 复位段 | 上电行（逐字节原文） |
|---|---|---|
| r1 | reset#1（窗口起点，`rst=0040` = PIN 复位；**下载前镜像**） | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` + `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=3`（**rev 5.5 形态：无 `VDD` 行**）+ `IOSIG scl`（**在字段中途被下载复位切断**，其后紧接下一条 `BOOT`；该段无 BUS） |
| r1 | reset#2（`rst=0240` = SYSRESETREQ\|PIN，下载复位 05:29:25） | `BOOT fw=FD-002r4 uid=6A002C00 rst=0240 uart=9600` + **`VDD ok=1 code=1480 bgrmv=1189 mv=3289`** + **`IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=3`** + **`IOSIG scl=95555 sda=30303`** + `BUS idle=3 scl=1 sda=1 ack=none` |

- 上电顺序实测 `BOOT → VDD → IOTEST → IOSIG → BUS`，五行紧邻（判据脚本按相邻性硬门禁）。
- **下载生效的实板自证（本轮关键）**：reset#1 段**没有** `VDD` 行，reset#2 段**有** `VDD` 行。`VDD` 行是本轮新增，**只能由 rev 5.6 打印**；下载前板上是 EV-016 所下的 rev 5.5（其打印序列为 `BOOT→IOTEST→IOSIG→BUS`）。两者同板（`uid=6A002C00`）⇒ 板上镜像确实由 rev 5.5 换成了 rev 5.6，且受测件就是本轮交付件。
- **`VDD` 行的独立复核（§2.5）**：`4095*1189/1480 = 3289`（整数除法），与行内 `mv=3289` 逐位相符；脚本对 `ok=1` 行强制该等式（本能力独立复算，不引用实现常量），并另以 double 参考扫描 32 组 `(code,trim)` 全部一致（§3.3）。
- `IOTEST`/`IOSIG`/`BUS` 各字段与前几轮逐字节相同：主机确实能把 PA04(SDA)/PA03(SCL) 拉低并回读为低、释放后两线回高（`idle=3`）、未发现角色接反（`swap=none`）；`gc=3` 与同段 `BUS ack=none` 一致（脚本硬门禁）。
- **受测板身份核对**：横幅 MCU 唯一编号 `uid=6A002C00`，与 EV-012（3 份）、EV-013（2 份）、EV-014（6 份）、EV-015（多份）、EV-016（多份）全部捕获**相同** ⇒ 本轮复测的**就是同一块传感器板**。

### 2.2 采样周期（`S` 行与紧随的 `G` 行）

| 捕获 | 窗口（UTC） | 字节 | 周期 `k` | `q` | `S` 行关键字段 | 紧随 `G` 行 |
|---|---|---|---|---|---|---|
| r1 | 05:29:11.41 → 05:34:11.60（内含下载复位 05:29:25） | 528 | 0, 3 | 0, 0 | `o=8 n=32 x=35 a=33` ×2 | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` ×2 |
| s1 | 05:34:12.03 → 05:39:12.23（无复位，与 r1 仅隔 0.43 s） | 224 | 6, 9 | 0, 0 | `o=8 n=32 x=34 a=33` / `o=8 n=33 x=36 a=34` | 同上 ×2 |
| s2 | 05:39:12.63 → 05:44:12.85（无复位，与 s1 仅隔 0.40 s） | 113 | 12 | 0 | `o=8 n=33 x=36 a=34` | 同上 ×1 |
| s3 | 05:44:13.26 → 05:49:13.47（无复位，与 s2 仅隔 0.41 s） | 226 | 15, 18 | 0, 0 | `o=8 n=34 x=36 a=34` / `o=8 n=35 x=37 a=35` | 同上 ×2 |

- **7/7 周期 `q=0`**；`t=0 h=0 p=0 H=0 V=0 D=0 r=0 s=0 y=0`（无有效温湿度、不推进前值、不置上报）。
- 以 05:29:25 的下载复位为锚，`r1+s1+s2+s3` 四窗口首尾相接给出 `k=0,3,6,9,12,15,18` **七个连续周期（≈21 分钟）**；期间器件无任何成功周期、无自愈。
- 7 条 `G` 行**逐字节相同**，与 `BUS ack=none`、`gc=3`、`a44=a45=0`、`rd=0`、`at=3` 逐项自洽；失败发生在**地址探测阶段**（`rd=0`，重试/CRC/量程分支尚未进入）。

### 2.3 `IOSIG` 事务内逐位回读签名：本能力独立重建期望值后判读

期望值由脚本内 `iosig_expected()` **独立推导**（20 个半位采样点：S0 START 建立、S1 SCL 拉低、S2..S17 地址字节 `0x88` 的 8 位各「低+高」、S18/S19 ACK 时隙），不引用实现常量：

- 无器件应答 ⇒ 期望 **`scl=95555`、`sda=30303`**；器件在 `0x44` 应答 ⇒ `sda=30300`。
- 实板完整读数（r1 reset#2）**为 `scl=95555 sda=30303`**；reset#1 的那条在**字段中途**被烧录复位切断（本轮切断点落在 `IOSIG scl` 之后，比 EV-016 更早一个字节），判据脚本按「复位切断」登记 NOTE（A 组 `CUT`：字节是同窗口同类型完整行的前缀且其后紧接 `BOOT`），并另设负对照证明「无完整行可参照的中途截断」仍判 `A TERM` 失败（§3.2）。
- 判读：SCL 的 START/8 位时钟/ACK 时隙形态**逐位**与主机驱动形态相符、SDA 的 8 个数据位等于被驱动的地址位 ⇒ **事务内的位与时钟确实送达引脚并被如实回读**；ACK 时隙两位=`11` ⇒ 地址 `0x44` 上无器件拉低 SDA，并与**同一次复位段**的 `BUS ack=none`、`gc=3` 三方交叉一致（脚本硬门禁）。

### 2.4 交叉对照（同一批观测内的其它通路）

`o=8` 恒成立（每周期 8 次 AIN11 转换全部成功）、`n≤a≤x` 逐周期成立、**周期内 min/max 离散度 ≤3 码**（最稳的 `32/34`，最散的 `34/37`），跨周期 `a` 在 33–35 之间。对照：EV-016（同日稍早）为 30–31、EV-015 为 32–35、EV-014 为 80–117、EV-013（傍晚）为 ~3400–3624 ⇒ 码值随环境光照单调变化，本轮为**偏暗但有光环境**（与 readme 点 7 一致）。⇒ MCU、RTC 节拍、ADC、GPIO 供电窗口（PB05）、UART1 通路均正常，故障被进一步限定在 **U9 及其 I²C 连接**上。

### 2.5 `VDD` 上电供电轨测量：独立复核、量级判读与适用边界（本轮新增）

**(a) 原始读数（逐字节）**：`VDD ok=1 code=1480 bgrmv=1189 mv=3289`（上电一次，reset#2 段，出现在任何 I²C 事务之前）。

**(b) 独立复算（不引用实现常量）**：

| 复核 | 方法 | 结果 |
|---|---|---|
| 行内自洽 | 脚本内整数复算 `4095*bgrmv/code`（上限 6000 截断） | `4095*1189/1480 = 3289` == 行内 `mv` ⇒ **相符** |
| 公式出处 | 厂商示例 `Examples/ADC/adc_sgl_sw_vdd/USER/src/main.c`：`MCU_VDD = (4095 * BGR_1_2V_value * 0.001f)/valueAdc`（伏，等价 `mv = 4095*BGR_mV/code`）；中断版 `(4.095f * BGR_1_2V_value)/valueAdc` 同式 | 固件公式与厂商示例**同式** |
| 采样条件 | 厂商注释：采 TS/BGR 需采样时间 ≥40 µs；本固件 `ADC_Clk_Div8`（PCLK 8 MHz ⇒ ADCCLK 1 MHz）+ `ADC_SampTime390Clk` = **390 µs** | 满足（余量 ≈10×） |
| 修调值 | 厂商在 `0x001007D2` 存 BGR1.2V 实际 mV；本固件同址读取，`bgrmv=1189`（1.189 V，1.2 V 基准的正常个体偏差） | 量级合理 |
| 调用顺序 | 交付声明「必须在 `light_init()` 之前调用」；`main.c:217` 的 `light_read_vdd_mv()` 在 `measure.c:460` 的 `light_init()` 之前 | 约束被遵守 |
| 独立 harness | `test/host_vdd_verify_ev.c`（本能力自有影子层与断言）**31 项 0 失败**：公式扫描 32 组、除零保护、超时（有界）不伪造读数、`BGREN`/ADC 清理、空指针保护、输出清零、测量后 `light_sample()` 仍 8/8 且 PB05 只在采样期供电 | 见 §3.3 |

**(c) 该电位在板上的含义（本轮亲自重解析 `sensor_hardware/pstxnet.dat`）**：

| 网 | 节点 | 与固件/手册对照 |
|---|---|---|
| `BAT` | `J1.1`(CR2032+) · **`U7.9`(MCU VDD)** · **`U9.3`(GXHT40 VCC)** · `R1.1` · `R2.2` · `L2.2` · `C7.2` · `C8.1` · `C9.1` · `C13.1` · `C14.1` · `C15.1` · `C16.1` · `U4.6`(无线 VDD) | MCU 供电与传感器 VCC **同网、无开关、无串联元件**（R1/R2 是 I²C 上拉，L2 是射频支路电感）⇒ 测得的 ≈3.289 V 即**传感器供电网**电位 |
| `N05845` | `R1.2` · `U7.14`(PA04) · `U9.1`(SDA) | 与固件 PA04=SDA 一致 |
| `N06024` | `R2.1` · `U9.2`(SCL) · `U7.13`(PA03) | 与固件 PA03=SCL 一致 |
| `GND` / `NC` | `U9.4` / `U9.5`(=PAD，未连接) | 与手册 DFN4 引脚定义一致 |

**(d) 手册电气边界（本轮重新提取 `gxht40.pdf`）**：`VDD 1.6..5.5 V`（典型 **3.3 V**）、`VPOR ≤ 1.0 V`、`tPU ≤ 1 ms`、`tMEAS.H ≤ 8.3 ms`（≤ 固件 `GXHT40_MEASURE_WAIT_MS=10`）、全系列仅 `0x44`/`0x45` 两个 I²C 地址、§7.7 条目 2 = I²C general call `0x00` + `0x06`。

**(e) 判读与适用边界（如实登记）**：

- 实测 3.289 V **在器件供电域内**、且为**新鲜 CR2032 量级** ⇒ 「电池枯竭 / 供电轨塌陷 / 存在固件可控供电门」这一支被**实板数据排除**。
- **边界**：测点在 MCU 侧（内部 BGR 反推的是 MCU 自己的 VDD）。`U9.3` 焊点开路 / 虚焊 / 焊盘裂纹时，该网仍读到 3.289 V，而**器件引脚可能未真正得电** ⇒ 本读数**不能**排除「器件未得电（H1 的焊接支）」。
- **精度边界**：依赖出厂修调值与 ADC 本身，仅供「供电是否在可用量级」判断，**不作精密测量**（`ok=0` 时 `mv=0` 也不代表 0 V）。本轮 `ok=1`，且行内自洽与独立复算相符。

### 2.6 原始证据清单（入库）

| 文件 | 字节 | md5 | 窗口（UTC） |
|---|---|---|---|
| `evidence/ev017/r1.cap` | 528 | `beecc3e5315e064430d4a5da8c44bd85` | 05:29:11.41 → 05:34:11.60（内含下载复位 05:29:25） |
| `evidence/ev017/s1.cap` | 224 | `8270bf855f7b05dbe81980075b17eed5` | 05:34:12.03 → 05:39:12.23 |
| `evidence/ev017/s2.cap` | 113 | `4c618a64037d635502dfe56a6c533c9b` | 05:39:12.63 → 05:44:12.85 |
| `evidence/ev017/s3.cap` | 226 | `ba016b0b907256721db5a42d4185e71b` | 05:44:13.26 → 05:49:13.47 |

下载日志与镜像身份：`evidence/ev017/mdk_flash_logs.txt`（1 次，`Verify OK`）、`evidence/ev017/image_identity.txt`（下载前/后/独立 rebuild 后三处 md5 一致 + 两次构建原始日志）；窗口清单：`evidence/ev017/capture_windows.txt`；表格化摘要：`evidence/ev017/summaries.txt`（`summarize.py`）。扩展名刻意用 `.cap`：仓库 `.gitignore` 含 `*.bin`，`collect.py` 逐份按字节与 md5 复核。

## 3. 独立判定（本能力自建资产，不引用实现自述）

| 运行 | 对象 | 结果 |
|---|---|---|
| 实板判定（本轮） | `evidence/ev017/{r1,s1,s2,s3}.cap`（4 份合并 7 周期） | **86 项通过 / 1 项失败**（唯一失败 = 核心验收 `H ACCEPT`：最长连续 `q=1` = 0）→ 原文 `evidence/ev017/verify_run.txt`，退出码 1 |
| **定向变异自检** | 合成捕获（正确签名、SCL 错、数据位错、ACK 时隙混合、ACK/无 ACK 与 BUS 双向矛盾、`IOSIG` 位置错、缺失、复位切断行、**含字段中途截断且无完整行可参照的负对照**、`gc` 一致/矛盾/越域/缺字段、跨下载窗口的旧镜像 4 字段行、以及**本轮新增 10 例 `VDD`**：正确/6000 截断/`ok=0&mv=0` 通过，`mv` 与公式不符、`ok=1&code=0`、`ok=0&mv≠0`、越域、字段序错、缺行、行序错 失败） | **32/32 与预期一致**（exit 0）→ `evidence/ev017/verify_selftest.txt` ⇒ 判据**既能通过也能失败**，非恒过的空验 |
| 宿主 harness 复跑（8 份，本轮对**当前工作树**重跑） | `USER/src/*.c` | **104 / 93 / 51 / 36 / 56 / 79 / 79 / 31 通过，0 失败（合计 529）** → 原文 `evidence/ev017/harness_results.txt` |

### 3.1 本轮判据脚本的更新（仅测试资产，未改产品源码）

1. **新增 `N VDD` 组**：逐条解析 `VDD ok= code= bgrmv= mv=`，按**独立复算**的 `4095*bgrmv/code`（6000 上限）硬门禁 `ok=1` 行，并硬门禁 `ok=0 ⇒ mv==0`（不得伪造读数）、域检查（`ok∈{0,1}`、`code/bgrmv 0..4095`、`mv 0..6000`）、冻结字段序 `(ok,code,bgrmv,mv)` 与「完整上电段内恰 1 条、位于 `BOOT` 之后 `IOTEST` 之前且相邻」。
2. **上电顺序门禁更新**：`L IOSIG` 的相邻性由 `BOOT<IOTEST<IOSIG<BUS` 改为 **`BOOT<VDD<IOTEST<IOSIG<BUS`**（与交付声明及本轮实板一致）。
3. **截断行处理**：对「字节是同窗口同类型完整行前缀、且其后紧接 `BOOT`」的行（本轮 `IOSIG scl` 落在字段中途）登记为 `B FIELDS NOTE`，不据此判字段集；同时新增负对照证明「无完整行可参照的中途截断」仍判 `A TERM` 失败。
4. **`mv` 提取修正**：聚合登记原先按 `split("mv=")` 取值会被 `bgrmv=` 抢先匹配，改为词边界正则（`\bmv=`/`\bcode=`/`\bbgrmv=`），并把 `VDD` 读数量级以 **note** 形式登记（**不作门槛**：本项验收未定义供电电压阈值）。

### 3.2 判据非空验的关键负对照（本轮实跑）

| 组 | 负对照（期望 FAIL，实跑一致） |
|---|---|
| `N VDD` | `mv` 与自身 `code/bgrmv` 不符；`ok=1` 而 `code=0`（无效转换被报为有效）；`ok=0` 而 `mv≠0`（伪造读数）；`mv=6001` 越域；字段序错；完整段缺行；`VDD` 排在 `IOTEST` 之后 |
| `A`/`B` | 中途未终止且**非任何完整行前缀**仍判 `A TERM`；完整段内 4 字段 `IOTEST` 仍判冻结字段集失败 |
| `J`/`L` | `gc=3` 与 `BUS ack=44` 矛盾；`IOSIG` 设备应答签名与 `BUS ack=none` 矛盾；`IOSIG` 排在 `BUS` 之后 |

### 3.3 本轮新增的独立 harness（`host_vdd_verify_ev.c`，31 项 0 失败）

编译**真实** `USER/src/light.c`（+`fw_core.c`），影子 MCU 层为**本能力自有**的 `test/mock_vdd_ev/`（不复用实现能力的 `mock_light/`），期望值双重独立推导（厂商示例公式 + double 参考）：

| 组 | 断言要点 | 结果 |
|---|---|---|
| V1 | BGR 通道 `ADC_InputVref1P2`、采样时间 390 clk（≥40 µs）、单次转换、返回 1 | PASS ×5 |
| V2 | 转换开始时 `BGREN==1`、测量后 `BGREN==0`、ADC 关闭（低功耗/清理） | PASS ×3 |
| V3 | 值路径（含**与实板同一点** `code=1480,trim=1189 → mv=3289`）与 6000 截断 | PASS ×6 |
| V4 | **公式扫描** 32 组 `(code,trim)` 与 double 参考逐例一致 | PASS ×1 |
| V5 | `code==0` 除零保护：返回 0、`mv` 保持 0、清理照做 | PASS ×3 |
| V6 | **超时**（EOC 永不置位）：有界返回、不伪造读数、`BGREN`/ADC 清理；guard 常量有界 | PASS ×5 |
| V7/V8 | 空指针四种组合返回 0 且不发转换；失败时覆盖调用者旧值 | PASS ×6 |
| V9 | 测量后 `light_init()` 重配 AIN11、`light_sample()` 仍 8/8、均值正确、PB05 仅采样期供电、未标定时 `valid=false` | PASS ×6 |

⇒ 本轮新增的供电测量 API 在**宿主机层面**的公式、边界、清理与不伪造读数均**独立成立**；实板读数（3.289 V）与该 API 的实际行为相符。

## 4. 失败环节定位（本轮一手证据闭合到哪一层）

| 层 | 本轮实测 | 判定 |
|---|---|---|
| 主机引脚驱动/回读 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none` | 能拉低、能释放、未接反 |
| 事务内位/时钟送达 | `IOSIG scl=95555 sda=30303` | 位与时钟形态真实成立（H12 维持排除） |
| 器件地址应答（候选） | `BUS … ack=none`（112 个合法 7bit 地址）、`IOSIG` ACK 时隙 `11` | **无任何地址被应答** |
| 器件地址应答（general call `0x00`） | `gc=3`（EV-016 起覆盖） | **连 `0x00` 也不应答** ⇒ 器件对**任何** I²C 地址均无响应 |
| **供电轨（本轮新增）** | **`VDD ok=1 … mv=3289`（≈3.289 V，网表证明与 `U9.3` 同网）** | **供电网电位正常**（器件域 1.6–5.5 V、典型 3.3 V）⇒ 电池/轨塌陷支被排除；**不能**排除 U9 焊点未连通 |
| 协议层（命令/读/CRC/量程/重试） | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` ×7 | **未进入**（`rd=0`：连读事务都未发起） |
| 采样/判定/冻结不变量 | `q=0 p=0 H=0 V=0 r=0 s=0`、`raw` 全 `-` | 按设计生效（不上报/不推进前值/不构造 0 值） |
| 其它通路（ADC/GPIO/UART/RTC） | `o=8` 恒成立、离散度 ≤3 码、节拍 Δk=3 | 正常 |

⇒ 本轮失败环节与前几轮一致：**「器件是否在总线上应答」这一层（地址级）**。EV-016 登记的唯一未测物理量（供电轨）现已实测为**正常量级**，主机侧与供电网侧可改变的因素已被实板数据逐条覆盖，**未发现**新的主机侧或供电网侧可修项；剩余解释在 **U9 自身的焊点连通性 / 器件本体**。

## 5. 竞争假设状态（本轮更新）

| 假设 | 状态 | 依据 |
|---|---|---|
| **供电异常（电池枯竭 / 轨塌陷 / 供电门）** | **排除（本轮新增）** | 实板 `VDD ok=1 mv=3289`；网表证明测点与 `U9.3` 同网、无开关无串联；手册域 1.6–5.5 V（典型 3.3 V） |
| H1 器件贴装/焊接不良（含 U9.3 焊点未连通） | **开放（现场）；仍为最可能的「器件侧」解释** | 用户已重焊，现象逐字节未变；任何地址（含 `0x00`）都不应答；**测点在 MCU 侧，不能排除 U9 引脚未真正得电/未连通**；本能力无目视/通断手段 |
| H2 器件损坏 / 非本系列件 | **开放（现场）** | 112 个合法地址 + `0x00` 全域不应答；手册确认全系列仅 `0x44`/`0x45` ⇒ 型号/地址变体假设已排除 |
| H3 器件异常闩锁（需真正断电） | **开放（现场）** | 手册确认器件只能靠**断电**复位（无复位脚；`U9` 与 MCU 同 `BAT` 网、无开关、CR2032 焊接 ⇒ MCU 复位不给 U9 断电）；固件侧的 general call 复位尝试**不被器件接受**（`gc=3`） |
| H4 SDA/SCL 接反 | **排除** | `IOTEST swap=none` + 网表复核 |
| H5 主机拉不低/拉不高 | **排除** | `IOTEST sda_lo=0 scl_lo=0 idle=3` |
| H12 事务内位/时钟未送达 | **排除** | `IOSIG scl=95555 sda=30303` + 与 `BUS ack=none`、`gc=3` 三方交叉一致 |
| 上拉缺失/总线卡低 | **排除** | `idle=3`；恢复原语有界；宿主负对照：SDA 被持续拉低必然报满 8 地址 + `+`，而实板为 `none` |
| 地址变体（含 general call `0x00`） | **排除** | 手册确认全系列仅 `0x44`/`0x45`；`BUS` 扫 112 地址 `ack=none`；`gc=3`：`0x00` 亦无应答 |
| 读未就绪 / CRC / 量程 / 重试策略 | **排除** | 失败在地址探测阶段（`rd=0 at=3`）；手册时序/换算逐条对齐；529 项 harness 全绿 |
| 光照/ADC/RTC/UART 通路异常干扰 | **排除** | §2.4 交叉对照 |

## 6. 本轮未覆盖项与证据缺口（如实登记）

1. **「主机能否正确判定 ACK」在本板上仍缺正向对照**：本板 I²C 总线上除 U9 外无第二个从机（§2.5 网表），没有「真器件应答、主机读到 0」的实板样本。覆盖靠三层间接证据：bit 级 harness（mock 从机在正确时隙被采到 ACK，含 `gc=0/1/2` 三种应答形态）、`IOSIG` 在真实总线上回读到 ACK 时隙的实际电平、§2.3 的采样点时序算术。
2. **实板 `raw` 十六进制形态**仍只有宿主覆盖（实板 `rd=0`，从未读回帧）——器件不响应的直接后果。
3. **供电测量的绝对精度未标定**：本轮的 3.289 V 只作**量级**判断（依赖出厂修调值与 ADC）；无万用表/参考源可做绝对比对。该量级判断足以排除「电池枯竭/轨塌陷」，但**不**用于任何精度结论。
4. **测点在 MCU 侧**：`U9.3` 焊点是否真正连通**不能**由本读数判定（同网电位在开路时仍正常）。这是 H1 仍开放的直接原因。
5. **无逻辑分析仪/示波器/万用表/可控电源**：这些**不是**本项阻塞理由，亦**未**以任何未提供仪器作为门槛。
6. **`LIGHT_DARK_CALIBRATED=0`**：全暗基准仍未标定。本轮光照读数（`a` 33–35，有光）**不作、也不能作**全暗标定值；本项 `V=0 D=0` 说明未标定门禁按设计生效。
7. **手机端读数一致性未实测**：`r=0`（永不满足上报条件）⇒「满足上报条件时手机端与该周期 `S` 行 `t/h` 为同一笔读数」这一子句**在本轮构造上无法被触发**，故未调用 `android_test`（不虚构未发生的上报）。
8. **登记偏差 D-01**（上报方向仍为降温 `(prev-cur)>9`，readme 要求升温）在本轮构建上**仍然存在**（只读复核 `fw_core.c`），属固件任务 T4 范围，**不属本项**验收。
9. **文档同步**（非本能力职责）：`VDD` 行（含本轮新增字段集与判读）与实际上电顺序 `BOOT→VDD→IOTEST→IOSIG→BUS` 尚未收录进 TD-002/FD-002 正文（EV-012..EV-016 已登记 `IOTEST`/`IOSIG` 项，本轮再次确认并新增 `VDD`）。
10. **测试资产卫生**：`test/host_light_verify_ev.c` 为早期（ITEM-004 时期）harness，其引用的 `mockinc/` 影子层已不在仓库、且使用的 `light_last_code()` API 已随 `light_result_t` 重构消失 ⇒ 该文件**当前不可编译**（历史遗留）。本轮未删除（不属本项范围），改由新增的 `test/host_vdd_verify_ev.c` 覆盖 `light.c` 的供电测量与采样序列路径；建议后续任务清理或重写该文件。

## 7. 判定与交接

**判定：`TEST_FAIL`。** 本项验收要求的**实际板上行为**（每 3 分钟周期 `S` 行 `q=1`、`t/h` 落有效域、连续多周期稳定）在当前构建与当前受测板上**未观察到**，本轮 7/7 周期 `q=0`，且失败形态逐字节稳定可复现。按能力契约，实际行为不符合当前验收要求即返回 `TEST_FAIL`，并如实交付复现、预期/实际、竞争假设与最小区分实验；未知根因**不**被本能力用来宣告外部阻塞，**不**用来宣告器件损坏，也**不**代替实现者判定「不可修复」。

**预期 / 实际（一句话对照）**：期望每个 3 分钟周期 `S` 行 `q=1` 且 `t/h` 在 −40.0..125.0 ℃ / 0..100.0 %RH 且连续 ≥3 周期稳定；实际连续 7 个周期 `q=0`、`t/h=0`、各紧随 `G s=2 a44=0 a45=0 rd=0 at=3`，器件在 112 个合法地址与 general call `0x00` 上均不应答（`gc=3`），而**供电轨实测 ≈3.289 V（正常量级）**。

**复现步骤（可原样重跑）**

1. `serial_list_ports` 核对 COM42（Prolific `067B:2303`）与 CW-DAPLink（`C251:F001`）。
2. 运行 `sh evidence/ev017/seq.sh r1 s1 s2 s3`（先启 300 s `serial_capture`，4 s 后 `mdk_flash {}`，再背靠背续采三个 300 s 窗口）。
3. `python evidence/ev017/collect.py <work-dir> evidence/ev017 r1 s1 s2 s3` 收纳原件；`python evidence/embedded_verify_gxht40_item002.py evidence/ev017/{r1,s1,s2,s3}.cap` 判定；`… --selftest` 复核判据非空验。
4. 期望的恢复判据（一眼可判，任一成立即说明器件重新上线）：`VDD` 行出现且 `ok=1`（本轮已成立，用于供电分支判断）；`IOTEST` 的 `gc` 由 `3` 变为 `0`/`1`/`2`；或 `IOSIG` 的 ACK 时隙由 `11` 变为 `00`（即 `sda=30300`）；或 `BUS ack` 由 `none` 变为含 `44`/`45`；其后应出现 `S … q=1`。

**最小区分实验（供实现能力与现场选择；本能力不替其决定）**

| 编号 | 动作 | 区分什么 | 谁执行 |
|---|---|---|---|
| E-b | 目视 + 通断/阻值核对 U9 四个焊点（`1=SDA`/`2=SCL`/`3=VDD`/`4=VSS`），并核对 `U9.3` 与 `BAT` 网、`U9.4` 与 `GND` 网 | **H1（U9 引脚未真正连通）**——本轮供电网电位已正常，焊点连通性是剩余最可能的器件侧解释；与「器件得电但不应答」区分 | 现场/用户（需万用表；**未提供不作为通过门槛**，仅作为前行手段） |
| E-a | **真正断开电源 ≥ 5 s 后再上电**（CR2032 焊在板上，需拆/抬电池或断开电池正极焊点） | H3（异常闩锁，只能靠断电复位） | 现场/用户（本能力无 `power_cycle` 工具） |
| E-c | 换一只**已知良好**的 GXHT40（含 `GXHT40-BD`/`GXHT40C-AD` 变体 `0x45`） | H2（器件损坏/错件） | 现场/用户 |
| E-d | 同上任一步后**不改任何代码**，仅重新上电 + COM42 采集 ≥ 10 分钟 | 用本轮同一判据脚本复判「是否出现 `q=1` 与连续周期」；同时读新的 `VDD` 行确认供电 | 本能力（拿回同板复测） |

**验收恢复判据（供现场一眼判断）**：见上「复现步骤」第 4 条——`gc≠3`、或 `IOSIG sda=30300`、或 `BUS ack` 含 `44`/`45`、或直接出现 `S … q=1`。

**交实现能力**：本轮把 EV-016 登记的最后一个主机侧可测物理量（供电轨）实测为**正常量级**，因此**固件侧可观测的判别分支已全部穷尽**：器件对任何 I²C 地址（含 `0x00`）都不应答，且其供电网电位正常。固件不可使一个对任何地址都不应答的器件应答；请据此判断是否仍有本能力未列出的、不违反冻结约束（不得放宽/绕过 CRC、有效域与校验；周期路径不得用 `0x94`/加热命令；失败周期不上报/不推进前值/不构造 0 值）的驱动或总线层手段；若无，请如实登记并交由现场/用户执行 E-b（首选）→ E-a → E-c。**本能力不替实现者判定不可修复，也不终止重试。**

**交设计能力**：TD-002/FD-002 的上电行集合与顺序需同步为 `BOOT→VDD→IOTEST→IOSIG→BUS`，并收录 `IOTEST` 尾字段 `gc`（0..3）与新增 `VDD` 行（`ok/code/bgrmv/mv`，`ok=1` 时 `mv==4095*bgrmv/code`；该行仅作供电量级判读，不构成功能通过门槛）。

**交现场/用户（最小必要动作与时间窗口）**：按 **E-b → E-a → E-c** 顺序，任一步后即回到 E-d。E-b 只需一次目视+通断核对（需万用表，未提供不作为门槛）；E-a 只需一次「断电 ≥ 5 s 再上电」；E-c 需替换件。E-d 的采集时间窗口为 **≥ 10 分钟（4 个 3 分钟周期）**；完成后**无需**任何固件改动。

**已交付的端口**：`evidence/test.md`（本文件，TestEvidence，`TEST_FAIL`）；`evidence/programmed_prototype.md`（本轮下载身份与回读校验：1 次 `Verify OK`、axf md5 `5fc16235…`、独立 rebuild 复现同一 md5）。

---

# 历史记录（EV-016 / run8 ITEM-002；当轮判 TEST_FAIL：7/7 周期 `q=1` 数 = 0、`gc=3`（连 general call `0x00` 也不应答），主机侧判健康，交回实现新增「上电供电轨 VDD 测量」以覆盖最后一个未测物理量；已提交 e769e56）

# 嵌入式验证证据（EV-016 / run8 ITEM-002 ↔ GXHT40 温湿度采集在真实目标上恢复有效）

状态：embedded_tester.embedded_verification 独立验证证据（本轮**重新构建下载、重新采集真实目标原始字节流**；历史结论只作背景，不作本轮结果）
本轮触发：Runtime 当前调用（`n005_embedded_tester_embedded_verification`，ITEM-002 复验）。承接实现能力对 EV-015 的交付件（rev 5.5：按手册 §7.7 新增**上电一次性 I²C general call 复位尝试 `0x00`+`0x06`**，并在 `IOTEST` 行尾新增 `gc=<0..3>`），用于把 EV-015 §6.3 明确登记的未测项「general call 地址 `0x00` 从未被探测」实测掉。

**结论：`TEST_FAIL`** —— 本项核心可观察「真实目标每个 3 分钟采样周期 `S` 行 `q=1`、`t/h` 落在有效域（温度 −40.0..125.0 ℃、湿度 0..100.0 %RH）、连续多个周期稳定」在**当前构建、当前受测板**上**仍未达成**：1 次成功下载、4 个首尾相接的采集窗口跨 21 分钟、**7 个连续采样周期全部 `q=0`**。

**本轮新增的判别事实**：下载后的镜像实测 **`gc=3`** —— 连 I²C general call 地址 `0x00` 也是**无应答**。结合 `BUS ack=none`（0x08..0x77 共 112 个合法 7bit 地址无一应答），受测器件对**任何** I²C 地址都不应答。本能力同时用**Tester 自有宿主 harness（新增 19 项）**把该字段的四个分支（`gc=0/1/2/3`）逐条钉死，证明 `gc=3` 是器件静默的真实读数，而不是固件侧误报。本能力**不**代替实现者判定「不可修复」，**不**以仪器缺失作为门槛，**不**放宽或绕过任何判据，**不**虚构数据。

- 受测提交：`55ed979`（HEAD；接手时 `git status` 干净；本轮**未改产品源码**）
- **被测件 = 当前交付件本身**：`git log -1 -- <FW>/USER` = `55ed979`；本轮 `mdk_flash {}` 所下 axf md5 = **`ff49029608f3315ee76964c131d06821`**（hex `d18c0d623812f96351d7b8ebb3d9d3a9`），与实现交付声明**逐字符一致**。
- 本轮交付的实际源码增量（`git diff --stat 5b07982 HEAD -- <FW>/USER`）= **4 个文件、+35/−3 行**：`USER/inc/sensor_config.h`（+6：`GXHT40_ADDR_GENERAL_CALL`/`GXHT40_CMD_GENERAL_CALL_RESET`/`GXHT40_GCALL_RESET_WAIT_MS`）、`USER/src/measure.c`（+27/−3：`sensor_io_diag_scan()` 末尾的上电一次性 general call 尝试）、`USER/inc/debug_trace.h`（+1）与 `USER/src/debug_trace.c`（+4/−1：`IOTEST` 行尾 `gc=`）。`gxht40.c`/`sf_i2c.c`/`fw_core.c` **本轮零改动**。
- 验证基线：TD-002 rev 5.0；FWR-002 v5.0（FWR-116/117/118）；FD-002 rev 5.0；`readme.txt`；`gxht40.pdf`（GXHT4x Family Datasheet **V2.6, Aug-2026**）§7.7；`sensor_hardware/{pstxnet.dat,MAIN_BOARD.BOM}`。

| # | 本轮事实（全部为本轮新采集） | 性质 | 依据 |
|---|---|---|---|
| 1 | 实板 **0/7 个采样周期 `q=1`**；核心可观察（`q=1`、`t/h` 有效域、连续多周期稳定）**未达成** | 功能不符（本项核心验收不通过）⇒ `TEST_FAIL` | §2.2 |
| 2 | **`gc=3`**：`0x00` 既不应答、也未发出复位命令（`wr_len=0`）；`BUS … ack=none`（112 个合法 7bit 地址）⇒ 器件对**任何** I²C 地址无响应 | EV-015 §6.3 登记的未测项**已被实测覆盖**；解释面收缩到器件/供电/连线侧 | §2.1、§4 |
| 3 | 7 条失败周期 `G` 行**逐字节相同**：`G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`；`BUS` 行 `BUS idle=3 scl=1 sda=1 ack=none`（与前几轮同名行逐字符一致） | 失败形态稳定可复现 | §2.1、§2.2 |
| 4 | `r1→s1→s2→s3` 四窗口首尾相接（04:32:37→04:52:39，间隙 0.41–0.42 s），构成 **`k=0,3,6,9,12,15,18` 七个连续周期（≈21 分钟）**，器件全程无应答、无自愈 | 失败稳定、非偶发；排除「需等待自愈」 | §2.2 |
| 5 | **下载生效的实板自证**：窗口起点（下载前）那一段的 `IOTEST` 为 **4 字段**（无 `gc`），下载复位后的 `IOTEST` 为 **5 字段 + `gc=3`** | 证明本轮所下镜像与新镜像均在板上真实运行，且受测件就是 rev 5.5 | §2.1 |
| 6 | 上电行 `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=3` ×1、`IOSIG scl=95555 sda=30303` ×1、`BUS … ack=none` ×1、横幅 `uid=6A002C00`（与 EV-012/013/014/015 同一块受测板） | 主机侧行为可复现、同一受测板（无换板混淆） | §2.1 |
| 7 | `IOSIG scl=95555 sda=30303`：SCL 的 START/8 位时钟/ACK 时隙形态逐位与本能力**独立重建**的期望相符，SDA 的 8 个地址位等于被驱动的 `0x88`；ACK 时隙两位 = `11` ⇒ 地址 `0x44` 上无器件拉低 SDA（与同复位段 `BUS ack=none`、`gc=3` 三方交叉一致） | 主机在事务内确实把位与时钟送到引脚并如实回读（**H12 维持排除**） | §2.3 |
| 8 | 光照通路持续工作：`o=8` 恒成立、每周期 `n≤a≤x`、**周期内 min/max 离散度 ≤3 码**，跨周期 `a` 在 30–31 之间（与 EV-015 同期 32–35 一致） | 交叉对照：MCU/时钟/ADC/GPIO/PWR/UART1 通路正常；本轮环境偏暗但仍属「有光」 | §2.4 |
| 9 | 失败周期**不上报、不推进前一有效温度、不构造 0 值**：`p=0 H=0 V=0 D=0 q=0 r=0 s=0 y=0`、`raw=------------` | 冻结不变量在实板成立 | §2.2 |
| 10 | 本能力自有宿主 harness 对当前交付源码复跑 **498 项 0 失败**（104/93/51/36/56/79/79）；判定脚本 `--selftest` **20/20**；加载实板 4 份捕获判定 **79 通过 / 1 失败（唯一失败 = 核心验收 `H ACCEPT`）** | 实现符合性**未被推翻**；判据非空验（含新增 `gc` 分支的正/负对照） | §3.3、§3.4 |

---

## 1. 环境、构建与部署（本轮实际调用，原始结果）

| 工具 / 命令 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `COM42 = Prolific USB-to-Serial Comm Port (VID:PID 067B:2303)`；`COM26 = USB 串行设备 (C251:F001, SN 87094109484987710672FF50)` | 传感器调试口 **COM42 在位**，与配置声明的设备身份一致 |
| `usb_list_devices`（经 `serial_list_ports` 的 hwid 佐证） | `USB VID:PID=C251:F001 SER=87094109484987710672FF50` | **CW-DAPLink 下载探针在位** |
| `md5sum …/Project.axf` / `.hex`（下载前） | `ff49029608f3315ee76964c131d06821` / `d18c0d623812f96351d7b8ebb3d9d3a9` | 与实现交付声明**完全一致** ⇒ 受测件身份闭合 |
| `mdk_flash {}` ×1（本地 12:32:41→12:32:51） | `Erase Done.Programming Done.Verify OK.Application running ...`；下载前 build 为 `0 Error(s), 0 Warning(s)`（无可重建项 ⇒ 所下即上面那个 axf） | 当前交付镜像已下载并**回读校验成功**。`Verify OK` **仅**证明下载校验，业务功能仍须实测（§2） |
| `serial_capture` ×4（COM42@9600，300 s/次；`r1` 与 `mdk_flash` 串接于同一 shell 脚本，使复位横幅落在窗口内；四个窗口首尾相接） | `evidence/ev016/{r1,s1,s2,s3}.cap`（499/224/226/113 B，逐字节账目闭合） | 原始字节已入库；窗口与 md5 见 §2.5 |
| `git status --short`（提交前） | 仅本能力声明的输出：新增 `evidence/ev016/`，修改 `evidence/test.md`、`evidence/programmed_prototype.md`、`evidence/embedded_verify_gxht40_item002.py`、`test/host_diag_probe_verify_ev.c`、`file_manifest.txt` | 产品源码（`USER/**`）与 Keil 工程**零改动**；无无关改动被卷入提交 |

授权与入口：本轮 context 的 `当前能力有效工具` 与 `本次允许调用的工具及参数` 均非空（9 个工具），实际调用 3 个（`serial_list_ports`、`mdk_flash`、`serial_capture`），未用 `mdk_build`（`mdk_flash` 内含编译且实测「无可重建项」）、未用 `usb_list_devices`（其身份信息已由 `serial_list_ports` 的 hwid 给出）、未用 `install_*`/`android_test`（理由见 §6）。`mdk_flash`/`serial_capture` 经 `tool_cli.py` CLI 入口（`evidence/ev016/seq.sh`，使用**本轮** context 路径 `CALL-e71e0e66-…`）。

## 2. 真实目标 UART1 观测（COM42 @9600 8N1，原始字节）

### 2.1 上电行（BOOT / IOTEST / IOSIG / BUS）

| 捕获 | 复位段 | 上电行（逐字节原文） |
|---|---|---|
| r1 | reset#1（窗口起点，`rst=0040` = PIN 复位；下载前镜像） | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` + `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none`（**4 字段，无 `gc`**）+ `IOSIG scl=95555 sda=3030`（**行尾被下载复位切断**，其后紧接下一条 `BOOT`；该段无 BUS） |
| r1 | reset#2（`rst=0240` = SYSRESETREQ\|PIN，下载复位 04:32:51） | `BOOT fw=FD-002r4 uid=6A002C00 rst=0240 uart=9600` + **`IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=3`** + **`IOSIG scl=95555 sda=30303`** + `BUS idle=3 scl=1 sda=1 ack=none` |

- 上电顺序实测 `BOOT → IOTEST → IOSIG → BUS`，四行紧邻（脚本按相邻性硬门禁）。
- **下载生效的实板自证（本轮关键）**：reset#1 的 `IOTEST` 只有 4 个字段，reset#2 的 `IOTEST` 有 5 个字段且带 `gc=3`。`IOTEST` 的 `gc` 字段是本轮新增，**只能由 rev 5.5 打印**；下载前板上是 EV-015 所下的 rev 5.4（其 `IOTEST` 恒为 4 字段）。两者同板（`uid=6A002C00`）。⇒ 板上镜像确实由 rev 5.4 换成了 rev 5.5，且受测件就是本轮交付件。
- **`gc=3` 的含义（按实现交付的判读表，逐条对上）**：先探 `0x44`/`0x45` 均无 ACK ⇒ 再探 general call `0x00`，**`0x00` 也不应答** ⇒ `gc=3`，且**未发出复位命令**（宿主侧同一分支实测 `wr_len==0`，§3.3 [I1]）。这与同复位段的 `BUS ack=none` 完全自洽（判据脚本按硬门禁交叉核对：`gc=3 ⇒ ack=none`）。
- `IOTEST` 其它字段与前几轮逐字节相同：主机确实能把 PA04(SDA)/PA03(SCL) 拉低并回读为低、释放后两线回高（`idle=3`）、未发现角色接反（`swap=none`）。
- **受测板身份核对**：横幅 MCU 唯一编号 `uid=6A002C00`，与 EV-012（3 份）、EV-013（2 份）、EV-014（6 份）、EV-015（多份）全部捕获**相同** ⇒ 本轮复测的**就是同一块传感器板**。

### 2.2 采样周期（`S` 行与紧随的 `G` 行）

| 捕获 | 窗口（UTC） | 字节 | 周期 `k` | `q` | `S` 行关键字段 | 紧随 `G` 行 |
|---|---|---|---|---|---|---|
| r1 | 04:32:37.25 → 04:37:37.47（内含下载复位 04:32:51） | 499 | 0, 3 | 0, 0 | `o=8 n=30 x=32 a=30` / `o=8 n=29 x=32 a=30` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` ×2 |
| s1 | 04:37:37.89 → 04:42:38.11（无复位，与 r1 仅隔 0.41 s） | 224 | 6, 9 | 0, 0 | `o=8 n=30 x=32 a=30` / `o=8 n=30 x=32 a=31` | 同上 ×2 |
| s2 | 04:42:38.54 → 04:47:38.73（无复位，与 s1 仅隔 0.43 s） | 226 | 12, 15 | 0, 0 | `o=8 n=30 x=32 a=30` / `o=8 n=30 x=32 a=30` | 同上 ×2 |
| s3 | 04:47:39.15 → 04:52:39.35（无复位，与 s2 仅隔 0.42 s） | 113 | 18 | 0 | `o=8 n=30 x=33 a=31` | 同上 ×1 |

- **7/7 周期 `q=0`**；`t=0 h=0 p=0 H=0 V=0 D=0 r=0 s=0 y=0`（无有效温湿度、不推进前值、不置上报）。
- 以 04:32:51 的下载复位为锚，`r1+s1+s2+s3` 四窗口首尾相接给出 `k=0,3,6,9,12,15,18` **七个连续周期（≈21 分钟）**；期间器件无任何成功周期、无自愈。
- 7 条 `G` 行**逐字节相同**，与 `BUS ack=none`、`gc=3`、`a44=a45=0`、`rd=0`、`at=3` 逐项自洽；失败发生在**地址探测阶段**（`rd=0`，重试/CRC/量程分支尚未进入）。

### 2.3 `IOSIG` 事务内逐位回读签名：本能力独立重建期望值后判读

期望值由脚本内 `iosig_expected()` **独立推导**（20 个半位采样点：S0 START 建立、S1 SCL 拉低、S2..S17 地址字节 `0x88` 的 8 位各「低+高」、S18/S19 ACK 时隙），不引用实现常量：

- 无器件应答 ⇒ 期望 **`scl=95555`、`sda=30303`**；器件在 `0x44` 应答 ⇒ `sda=30300`。
- 实板完整读数（r1 reset#2）**为 `scl=95555 sda=30303`**；reset#1 的那条被烧录复位从中途切断，其字节恰为完整行的前缀（按「复位切断」登记 NOTE，且被判据脚本按段scope 出「非完整上电段」而不作字段集判据）。
- 判读：SCL 的 START/8 位时钟/ACK 时隙形态**逐位**与主机驱动形态相符、SDA 的 8 个数据位等于被驱动的地址位 ⇒ **事务内的位与时钟确实送达引脚并被如实回读**；ACK 时隙两位=`11` ⇒ 地址 `0x44` 上无器件拉低 SDA，并与**同一次复位段**的 `BUS ack=none`、`gc=3` 三方交叉一致（脚本硬门禁）。

### 2.4 交叉对照（同一批观测内的其它通路）

`o=8` 恒成立（每周期 8 次 AIN11 转换全部成功）、`n≤a≤x` 逐周期成立、**周期内 min/max 离散度 ≤3 码**（最稳的 `30/32`，最散的 `31/33`），跨周期 `a` 在 30–31 之间。对照：EV-015（同日稍早）为 32–35、EV-014 为 80–117、EV-013（傍晚）为 ~3400–3624、EV-011（下午）为 130–388 ⇒ 码值随环境光照单调变化，本轮为**偏暗但有光环境**（与 readme 点 7 一致）。⇒ MCU、RTC 节拍、ADC、GPIO 供电窗口（PB05）、UART1 通路均正常，故障被进一步限定在 **U9 及其 I²C 连接/供电**上。

### 2.5 原始证据清单（入库）

| 文件 | 字节 | md5 | 窗口（UTC） |
|---|---|---|---|
| `evidence/ev016/r1.cap` | 499 | `54e675fbb8d12652a05e121a37c5e2bc` | 04:32:37.25 → 04:37:37.47（内含下载复位 04:32:51） |
| `evidence/ev016/s1.cap` | 224 | `fd5ef5a02cb3e20bd40084837c5a3428` | 04:37:37.89 → 04:42:38.11 |
| `evidence/ev016/s2.cap` | 226 | `9f366c16dd4b461f330da8e2486f09f3` | 04:42:38.54 → 04:47:38.73 |
| `evidence/ev016/s3.cap` | 113 | `8a2ce5826f81ee5dd27a244078c32c2b` | 04:47:39.15 → 04:52:39.35 |

下载日志：`evidence/ev016/mdk_flash_logs.txt`（1 次，`Verify OK`）；窗口清单：`evidence/ev016/capture_windows.txt`；表格化摘要：`evidence/ev016/summaries.txt`（`summarize.py`）。扩展名刻意用 `.cap`：仓库 `.gitignore` 含 `*.bin`，`collect.py` 逐份按字节与 md5 复核。

## 3. 独立判定（本能力自建资产，不引用实现自述）

| 运行 | 对象 | 结果 |
|---|---|---|
| 实板判定（本轮） | `evidence/ev016/{r1,s1,s2,s3}.cap`（4 份合并 7 周期） | **79 项通过 / 1 项失败**（唯一失败 = 核心验收 `H ACCEPT`：最长连续 `q=1` = 0）→ 原文 `evidence/ev016/verify_run.txt`，退出码 1 |
| **定向变异自检** | 合成捕获（正确签名、SCL 错、数据位错、ACK 时隙混合、ACK/无 ACK 与 BUS 双向矛盾、`IOSIG` 位置错、缺失、复位切断行、**`gc` 一致/矛盾/越域/缺字段、以及「跨下载窗口的旧镜像 4 字段行」的正反两例**） | **20/20 与预期一致**（exit 0）→ `evidence/ev016/verify_selftest.txt` ⇒ 判据**既能通过也能失败**，非恒过的空验 |
| 宿主 harness 复跑（7 份，本轮对**当前工作树**重跑） | `USER/src/*.c` | **104 / 93 / 51 / 36 / 56 / 79 / 79 通过，0 失败（合计 498）** → 原文 `evidence/ev016/harness_results.txt` |

### 3.1 本轮新增的 `gc` 分支独立覆盖（Tester 自有 harness，19 项）

EV-015 §6.3 把「general call 地址 `0x00` 从未被探测」登记为未测项；实现据此新增了该分支。为把**实板 `gc=3` 是器件静默的真实读数**这一结论与「固件是否恒报 3」区分开，本能力在**自有** `test/host_diag_probe_verify_ev.c` 中直接驱动真实 `measure.c::sensor_io_diag_scan()` 与真实 `sf_i2c.c`（bit 级 mock 总线），用**四种不同的 mock 从机**分别得到四个分支：

| 用例 | mock 条件 | 断言（全部 PASS） |
|---|---|---|
| [I1] | 总线上无任何器件 | `gc=3`；`0x00` **恰好被探 1 次**（有界、不重试）；**`wr_len=0`（未发出任何命令字节）**；无读事务；最后一次等待是 `SENSOR_BUS_DIAG_IDLE_SETTLE_MS`（**未取复位后等待**） |
| [I2] | 器件应答 `0x44` | `gc=0`；`0x00` **一次也不探**；无命令字节 |
| [I3] | 仅 `0x00` 应答且 `0x06` 被 ACK | `gc=2`；**恰好 1 个命令字节且为 `0x06`**；`0x06 != 0x94`（周期路径软复位从未使用）；随后取 `GXHT40_GCALL_RESET_WAIT_MS` 一次 |
| [I4] | `0x00` 应答但 `0x06` 被 NACK | `gc=1`；`0x06` 确实上线 |
| [I5] | 无器件时的跨观测一致性 | `gc=3` ⟺ 同轮 `sensor_bus_diag_scan()` 的 `ack_count==0` |

⇒ 该字段**可区分**四态、且负面对照（[I1] 与 [I2]/[I3]/[I4] 的取值不同、[I1] 要求「不发命令」）使「恒报 3」的实现无法通过。判据脚本侧同样以 `gc=4`（越域）、缺 `gc`（旧 4 字段行）与 `gc` 与 `BUS ack` 互相矛盾三类变异验证其能失败。

### 3.2 主机侧与协议/换算侧复核（结论：未被推翻）

- 采样路径唯一命令为 `0xFD`；`GXHT40_CMD_SOFT_RESET`(0x94) 宏**零调用**；无加热命令（只读静态复核 + harness 断言）。
- 总线恢复 `i2c_bus_recover()`：先释放两线，仅当 SDA 仍被拉低才补 **≤9 个 SCL 脉冲**并补一个**合法 STOP**，有界、不发地址、不发读、不写命令。
- 失败后 `s_addr7 = 0`（缓存地址失效）、下一周期**先做总线恢复再依次重探 `0x44`/`0x45`**（FWR-118）；本轮实板 `G` 行 `a44=a45=0 at=3 rd=0` 与该路径一致（每周期都重探 3 轮）。
- 手册对齐（本能力亲自读 `gxht40.pdf` §7.7 与电气表）：DFN4 `1=SDA/2=SCL/3=VDD/4=VSS` 且**无复位脚**，器件**只能靠断电复位**；全系列仅 `0x44`/`0x45` 两个 I²C 地址；`t_MEAS.H` 最大 8.3 ms ≤ 固件 `GXHT40_MEASURE_WAIT_MS=10`；`t_PU` 最大 1 ms ≤ 固件 `GXHT40_POWER_ON_WAIT_MS=2`；板级 4.7 KΩ 上拉 + 开漏驱动满足 `R_P ≥ 390 Ω`/`V_IL`/`V_IH`。
- 网表复核：`N05845 = R1(4.7K) + U7.PA04 + U9.1(SDA)`、`N06024 = R2(4.7K) + U9.2(SCL) + U7.PA03`、`BAT = U7.VDD + U9.VCC`（同网、**无开关、无固件可控供电门**）、`U9.5 = PAD → NC`；与固件 `measure.c` 的 PA04=SDA / PA03=SCL 逐项一致。

## 4. 失败环节定位（本轮一手证据闭合到哪一层）

| 层 | 本轮实测 | 判定 |
|---|---|---|
| 主机引脚驱动/回读 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none` | 能拉低、能释放、未接反 |
| 事务内位/时钟送达 | `IOSIG scl=95555 sda=30303` | 位与时钟形态真实成立（H12 维持排除） |
| 器件地址应答（候选） | `BUS … ack=none`（112 个合法 7bit 地址）、`IOSIG` ACK 时隙 `11` | **无任何地址被应答** |
| 器件地址应答（general call `0x00`） | **`gc=3`**（本轮新增覆盖） | **连 `0x00` 也不应答** ⇒ 器件对**任何** I²C 地址均无响应 |
| 协议层（命令/读/CRC/量程/重试） | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` ×7 | **未进入**（`rd=0`：连读事务都未发起） |
| 采样/判定/冻结不变量 | `q=0 p=0 H=0 V=0 r=0 s=0`、`raw` 全 `-` | 按设计生效（不上报/不推进前值/不构造 0 值） |
| 其它通路（ADC/GPIO/UART/RTC） | `o=8` 恒成立、离散度 ≤3 码、节拍 Δk=3 | 正常 |

⇒ 本轮失败环节与前几轮一致：**「器件是否在总线上应答」这一层（地址级）**。**EV-015 登记的唯一未测地址空间（general call `0x00`）现已实测为不应答**；主机侧可改变的因素已被实板数据逐条覆盖，**未发现**新的主机侧可修项。

## 5. 竞争假设状态（本轮更新）

| 假设 | 状态 | 依据 |
|---|---|---|
| H1 器件贴装/焊接不良（含 VDD 未连通） | **开放（现场）；权重升高** | 用户已重焊，现象逐字节未变；`gc=3` 把「仅候选地址不响应」扩展为「任何地址都不响应」，与「器件 VDD/GND/SDA/SCL 任一脚未真正连通」或「器件未得电」同形；本能力无目视/通断手段，不能据字节流排除 |
| H2 器件损坏 / 非本系列件 | **开放（现场）** | 112 个合法地址 + `0x00` 全域不应答；手册确认全系列仅 `0x44`/`0x45` ⇒ 型号/地址变体假设已排除 |
| H3 器件异常闩锁（需真正断电） | **开放（现场）；权重较高** | 手册 §7.7 确认器件只能靠**断电**复位；U9 无复位脚、与 MCU 同 `BAT` 网、无开关、CR2032 焊接 ⇒ MCU 复位不给 U9 断电；本轮已确认固件侧能做的「上电一次性 general call 复位」**不被器件接受**（`gc=3`） |
| H4 SDA/SCL 接反 | **排除** | `IOTEST swap=none` + 网表复核 |
| H5 主机拉不低/拉不高 | **排除** | `IOTEST sda_lo=0 scl_lo=0 idle=3` |
| H12 事务内位/时钟未送达 | **排除** | `IOSIG scl=95555 sda=30303` + 与 `BUS ack=none`、`gc=3` 三方交叉一致 |
| 上拉缺失/总线卡低 | **排除** | `idle=3`；恢复原语有界；宿主负对照：SDA 被持续拉低必然报满 8 地址 + `+`，而实板为 `none` |
| 地址变体（含 general call `0x00`） | **排除（本轮扩充）** | 手册确认全系列仅 `0x44`/`0x45`；`BUS` 扫 112 地址 `ack=none`；**`gc=3`：`0x00` 亦无应答** |
| 读未就绪 / CRC / 量程 / 重试策略 | **排除（本轮）** | 失败在地址探测阶段（`rd=0 at=3`）；手册时序/换算逐条对齐；498 项 harness 全绿 |
| 光照/ADC/RTC/UART 通路异常干扰 | **排除** | §2.4 交叉对照 |

## 6. 本轮未覆盖项与证据缺口（如实登记）

1. **「主机能否正确判定 ACK」在本板上仍缺正向对照**：本板 I²C 总线上除 U9 外无第二个从机（§3.2 网表），没有「真器件应答、主机读到 0」的实板样本。覆盖靠三层间接证据：bit 级 harness（mock 从机在正确时隙被采到 ACK，含本轮新增的 `gc=0/1/2` 三种应答形态）、`IOSIG` 在真实总线上回读到 ACK 时隙的实际电平、§2.3 的采样点时序算术。
2. **实板 `raw` 十六进制形态**仍只有宿主覆盖（实板 `rd=0`，从未读回帧）——器件不响应的直接后果。
3. **无逻辑分析仪/示波器/万用表/可控电源**：这些**不是**本项阻塞理由，亦**未**以任何未提供仪器作为门槛。
4. **`LIGHT_DARK_CALIBRATED=0`**：全暗基准仍未标定。本轮光照读数（`a` 30–31，有光）**不作、也不能作**全暗标定值；本项 `V=0 D=0` 说明未标定门禁按设计生效。
5. **手机端读数一致性未实测**：`r=0`（永不满足上报条件）⇒「满足上报条件时手机端与该周期 `S` 行 `t/h` 为同一笔读数」这一子句**在本轮构造上无法被触发**，故未调用 `android_test`（不虚构未发生的上报）。
6. **登记偏差 D-01**（上报方向仍为降温 `(prev-cur)>9`，readme 要求升温）在本轮构建上**仍然存在**（只读复核 `fw_core.c`），属固件任务 T4 范围，**不属本项**验收。
7. **文档同步**（非本能力职责）：`IOTEST`/`IOSIG` 两行（含本轮新增的 `gc` 字段）与实际上电顺序 `BOOT→IOTEST→IOSIG→BUS` 仍未收录进 TD-002/FD-002 正文（EV-012..EV-015 已登记，本轮再次确认）。
8. **判据脚本对本轮「跨下载窗口」做了明确取舍**（非放松，已登记）：`IOTEST` 的冻结字段集只对**段内有 BUS 行的完整上电段**强制；采集窗口跨越下载时，段内可能是**下载前镜像**打印的行，该行只登记为显式 `NOTE`，不据此评判被测构建。自检中同时固定了正例（不在完整段 ⇒ NOTE）与反例（**在完整段 ⇒ 仍判 `B FIELDS` 失败**），确保该取舍不能被用来掩盖被测构建的字段缺失。

## 7. 判定与交接

**判定：`TEST_FAIL`。** 本项验收要求的**实际板上行为**（每 3 分钟周期 `S` 行 `q=1`、`t/h` 落有效域、连续多周期稳定）在当前构建与当前受测板上**未观察到**，本轮 7/7 周期 `q=0`，且失败形态逐字节稳定可复现。按能力契约，实际行为不符合当前验收要求即返回 `TEST_FAIL`，并如实交付复现、预期/实际、竞争假设与最小区分实验；未知根因**不**被本能力用来宣告外部阻塞，**不**用来宣告器件损坏，也**不**代替实现者判定「不可修复」。

**预期 / 实际（一句话对照）**：期望每个 3 分钟周期 `S` 行 `q=1` 且 `t/h` 在 −40.0..125.0 ℃ / 0..100.0 %RH 且连续 ≥3 周期稳定；实际连续 7 个周期 `q=0`、`t/h=0`、各紧随 `G s=2 a44=0 a45=0 rd=0 at=3`，器件在 112 个合法地址与 general call `0x00` 上均不应答（`gc=3`）。

**复现步骤（可原样重跑）**

1. `serial_list_ports` 核对 COM42（Prolific `067B:2303`）与 CW-DAPLink（`C251:F001`）。
2. 运行 `sh evidence/ev016/seq.sh r1 s1 s2 s3`（先启 300 s `serial_capture`，4 s 后 `mdk_flash {}`，再背靠背续采三个 300 s 窗口）。
3. `python evidence/ev016/collect.py <work-dir> evidence/ev016` 收纳原件；`python evidence/embedded_verify_gxht40_item002.py evidence/ev016/{r1,s1,s2,s3}.cap` 判定。
4. 期望的恢复判据（一眼可判，任一成立即说明器件重新上线）：`IOTEST` 的 `gc` 由 `3` 变为 `0`（`0x44`/`0x45` 应答，正常路径）或 `1`/`2`（`0x00` 应答）；或 `IOSIG` 的 ACK 时隙由 `11` 变为 `00`（即 `sda=30300`）；或 `BUS ack` 由 `none` 变为含 `44`/`45`；其后应出现 `S … q=1`。

**最小区分实验（供实现能力与现场选择；本能力不替其决定）**

| 编号 | 动作 | 区分什么 | 谁执行 |
|---|---|---|---|
| E-a | **真正断开电源 ≥ 5 s 后再上电**（CR2032 焊在板上，需拆/抬电池或断开电池正极焊点） | H3（异常闩锁，只能靠断电复位）——这是固件侧已无法触及的最后一个「非破坏性」实验 | 现场/用户（本能力无 power_cycle 工具） |
| E-b | 目视 + 通断/阻值核对 U9 四个焊点（`1=SDA`/`2=SCL`/`3=VDD`/`4=VSS`），并核对 `U9.3` 与 `BAT` 网、`U9.4` 与 `GND` 网 | H1（贴装/焊接/VDD 未连通）；与「器件得电但不应答」区分 | 现场/用户（需万用表；**未提供不作为通过门槛**，仅作为前行手段） |
| E-c | 换一只**已知良好**的 GXHT40（含 `GXHT40-BD` 变体 `0x45`） | H2（器件损坏/错件） | 现场/用户 |
| E-d | 同上任一步后**不改任何代码**，仅重新上电 + COM42 采集 ≥ 10 分钟 | 用本轮同一判据脚本复判「是否出现 `q=1` 与连续周期」 | 本能力（拿回同板复测） |

**验收恢复判据（供现场一眼判断）**：见上「复现步骤」第 4 条——`gc≠3`、或 `IOSIG sda=30300`、或 `BUS ack` 含 `44`/`45`、或直接出现 `S … q=1`。

**交实现能力**：本轮确认 `gc=3` 后，**固件侧按手册可做的最后一个动作已实测穷尽**（上电一次性 general call 复位未被器件接受）。固件不可使一个对任何地址都不应答的器件应答；请据此判断是否仍有本能力未列出的、不违反冻结约束（不得放宽/绕过 CRC、有效域与校验；周期路径不得用 `0x94`/加热命令；失败周期不上报/不推进前值/不构造 0 值）的驱动或总线层手段；若无，请如实登记并交由现场/用户执行 E-a..E-c。**本能力不替实现者判定不可修复，也不终止重试。**

**交设计能力**：TD-002/FD-002 的上电行集合与顺序需同步为 `BOOT→IOTEST→IOSIG→BUS`，并收录本轮新增的 `IOTEST` 尾字段 `gc`（取值域 0..3 与判读表）。

**交现场/用户（最小必要动作与时间窗口）**：按上表 **E-a → E-b → E-c** 顺序，任一步后即回到 E-d。E-a 只需一次「断电 ≥ 5 s 再上电」；E-b/E-c 需要万用表/替换件（未提供不作为门槛，仅作为可选前行手段）。E-d 的采集时间窗口为 **≥ 10 分钟（4 个 3 分钟周期）**；完成后**无需**任何固件改动。

**已交付的端口**：`evidence/test.md`（本文件，TestEvidence，`TEST_FAIL`）；`evidence/programmed_prototype.md`（本轮下载身份与回读校验：1 次 `Verify OK`、axf md5 `ff490296…` 与交付件一致）。

---

# 历史记录（EV-015 / run8 ITEM-002；当轮判 TEST_FAIL：重焊后 0/5 周期 `q=1`、失败形态逐字节相同、主机侧判健康，交回实现新增上电 general call 复位尝试与 `gc` 观测量；已提交 5b07982）

# 嵌入式验证证据（EV-015 / run8 ITEM-002 ↔ GXHT40 温湿度采集在真实目标上恢复有效）

状态：embedded_tester.embedded_verification 独立验证证据（本轮**重新下载、重新采集真实目标原始字节流**；历史结论只作背景，不作本轮结果）
本轮触发：`operator_update`（2026-10-09T03:45:56Z）——「更新通用能力提示词和 BLOCKED 规则后继续第二个任务」；叠加 01:13:57Z 的重做授权（用户已重新焊接 U9，旧无 ACK/`q=0` 记录属焊接前条件，须用最新固件实板复测并保存新的 `BUS`/`G`/`S` 原始日志）。

**结论：`TEST_FAIL`** —— 本项核心可观察「真实目标每个 3 分钟采样周期 `S` 行 `q=1`、`t/h` 落在有效域（温度 −40.0..125.0 ℃、湿度 0..100.0 %RH）、连续多个周期稳定」在**当前构建、当前受测板**上**仍未达成**：1 次成功下载、3 个首尾相接的采集窗口跨 15 分钟、**5 个采样周期全部 `q=0`**，失败形态与前几轮**逐字节相同**。

本轮同时在同一批实板观测内把**主机侧链路判为健康**（上电自检 `IOTEST`/`IOSIG` 自洽、光照通路随环境工作、7 份本能力自有宿主 harness 479 项全绿、手册已知答案 KAT 通过），并如实交付**复现步骤、预期/实际、竞争假设与最小区分实验**。本能力**不**代替实现者判定「不可修复」，**不**以仪器缺失作为门槛，**不**放宽或绕过任何判据，**不**虚构数据。

- 受测提交：`59ca659`（HEAD；接手时 `git status` 干净；本轮**未改产品源码**）
- **被测件 = 当前交付件本身**：传感器固件源码自 `ef03c13`（实现能力交付提交）以来**零改动** —— `git diff --stat ef03c13 HEAD -- <FW>/USER` 为空、`git log -1 -- <FW>/USER` = `ef03c13`；本轮 `mdk_flash {}` 所下 axf md5 = **`5c2fdb58d0fc4ef7dac8a21e343082a3`**（hex `db3dbc72195d346815500b8ef158b90f`），与实现交付声明、EV-013/EV-014 受测件**逐字符一致** ⇒ 本轮验的就是当前交付件。
- 验证基线：TD-002 rev 5.0；FWR-002 v5.0（FWR-116/117/118）；FD-002 rev 5.0；`readme.txt`；`gxht40.pdf`（GXHT4x Family Datasheet **V2.6, Aug-2026**）；`sensor_hardware/{pstxnet.dat,MAIN_BOARD.BOM}`。

| # | 本轮事实（全部为本轮新采集） | 性质 | 依据 |
|---|---|---|---|
| 1 | 实板 **0/5 个采样周期 `q=1`**；核心可观察（`q=1`、`t/h` 有效域、连续多周期稳定）**未达成** | 功能不符（本项核心验收不通过）⇒ `TEST_FAIL` | §2.2 |
| 2 | 5 条失败周期 `G` 行**逐字节相同**：`G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`；`BUS` 行 `BUS idle=3 scl=1 sda=1 ack=none`（与 EV-013/EV-014 同名行**逐字符一致**） | 失败形态稳定可复现 | §2.1、§2.2 |
| 3 | `r1→s1→s2` 三窗口首尾相接（03:47:10→04:02:11，间隙 0.42 s / 0.46 s），构成 **`k=0,3,6,9,12` 五个连续周期**，器件全程无应答、无自愈 | 失败稳定、非偶发；排除「需等待自愈」 | §2.2 |
| 4 | 上电行 `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none` ×2、`IOSIG scl=95555 sda=30303` ×1（另 1 条被下载复位从行中切断，其字节为完整行的前缀）、`BUS … ack=none` ×1、横幅 `uid=6A002C00`（与 EV-012/013/014 同一块受测板） | 主机侧行为可复现、同一受测板（无换板混淆） | §2.1 |
| 5 | `IOSIG scl=95555 sda=30303`：SCL 的 START/8 位时钟/ACK 时隙形态逐位与本能力**独立重建**的期望相符，SDA 的 8 个地址位等于被驱动的 `0x88`；ACK 时隙两位 = `11` ⇒ 地址 `0x44` 上无器件拉低 SDA（与同复位段 `BUS ack=none` 交叉一致） | 主机在事务内确实把位与时钟送到引脚并如实回读（**H12 维持排除**） | §2.3 |
| 6 | 光照通路持续工作：`o=8` 恒成立、每周期 `n≤a≤x`、**周期内 min/max 离散度 ≤4 码**，跨周期 `a` 在 32–35 之间（对照 EV-014 的 80–117、EV-013 傍晚 ~3600） | 交叉对照：MCU/时钟/ADC/GPIO/PWR/UART1 通路正常，读数随真实光环境变化；本轮环境偏暗但仍属「有光」 | §2.4 |
| 7 | 失败周期**不上报、不推进前一有效温度、不构造 0 值**：`p=0 H=0 V=0 D=0 q=0 r=0 s=0 y=0`、`raw=------------` | 冻结不变量在实板成立 | §2.2 |
| 8 | 本能力自有宿主 harness 对当前交付源码复跑 **479 项 0 失败**（85/93/51/36/56/79/79）；判定脚本 `--selftest` **12/12**；手册已知答案 KAT `CRC(0xBEEF)=0x92` MATCH、换算与 §7.5 逐点一致、越域判无效 | 实现符合性**未被推翻**；判据非空验 | §3.3、§3.4 |

---

## 1. 环境、构建与部署（本轮实际调用，原始结果）

| 工具 / 命令 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `COM42 = Prolific USB-to-Serial Comm Port (VID:PID 067B:2303)`；`COM26 = USB 串行设备 (C251:F001, SN 87094109484987710672FF50)` | 传感器调试口 **COM42 在位**，与配置声明的设备身份一致 |
| `usb_list_devices`（经 `serial_list_ports` 的 hwid 佐证） | `USB VID:PID=C251:F001 SER=87094109484987710672FF50` | **CW-DAPLink 下载探针在位** |
| `git diff --stat ef03c13 HEAD -- <FW>/USER` | 空 | 受测源码 = 上一轮已交付并已实测的源码 |
| `md5sum …/Project.axf` / `.hex` | `5c2fdb58d0fc4ef7dac8a21e343082a3` / `db3dbc72195d346815500b8ef158b90f` | 与实现交付声明、EV-013/EV-014 受测件**完全一致** ⇒ 受测件身份闭合 |
| `mdk_flash {}` ×1（本地 11:47:24） | `Erase Done.Programming Done.Verify OK.Application running ...`；下载前 build 为 `0 Error(s), 0 Warning(s)`（无可重建项 ⇒ 所下即上面那个 axf） | 当前交付镜像已下载并**回读校验成功**。`Verify OK` **仅**证明下载校验，业务功能仍须实测（§2） |
| `serial_capture` ×3（COM42@9600，300 s/次；`r1` 与 `mdk_flash` 串接于同一 shell 脚本，使复位横幅落在窗口内；`r1→s1→s2` 首尾相接） | `evidence/ev015/{r1,s1,s2}.cap`（494/224/113 B，逐字节账目闭合） | 原始字节已入库；窗口与 md5 见 §2.5 |
| `git status --short`（提交前） | 仅本能力声明的输出：新增 `evidence/ev015/`，修改 `evidence/test.md`、`evidence/programmed_prototype.md`、`file_manifest.txt` | 产品源码与 Keil 工程**零改动**；无无关改动被卷入提交 |

授权与入口：本轮 context 的 `当前能力有效工具` 与 `本次允许调用的工具及参数` 均非空（9 个工具），实际调用 3 个（`serial_list_ports`、`mdk_flash`、`serial_capture`），未用 `mdk_build`（`mdk_flash` 内含编译且实测「无可重建项」）、未用 `usb_list_devices`（其身份信息已由 `serial_list_ports` 的 hwid 给出）、未用 `install_*`/`android_test`（理由见 §6）。`mdk_flash`/`serial_capture` 经 `tool_cli.py` CLI 入口（`evidence/ev015/seq.sh`，使用**本轮** context 路径 `CALL-58732d70-…`）。

## 2. 真实目标 UART1 观测（COM42 @9600 8N1，原始字节）

### 2.1 上电行（BOOT / IOTEST / IOSIG / BUS）

| 捕获 | 复位段 | 上电行（逐字节原文） |
|---|---|---|
| r1 | reset#1（窗口起点，`rst=0040`） | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` + `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none` + `IOSIG scl=95555 sda=3030`（**行尾被下载复位切断**，其后紧接下一条 `BOOT`；该段无 BUS，其 BUS 行落在窗口之前） |
| r1 | reset#2（`rst=0240`，下载复位 03:47:24） | `IOTEST …` + **`IOSIG scl=95555 sda=30303`** + `BUS idle=3 scl=1 sda=1 ack=none` |

- 上电顺序实测 `BOOT → IOTEST → IOSIG → BUS`，四行紧邻（脚本按相邻性硬门禁）。
- `IOTEST` 本轮 2 次读数**逐字节相同**：主机确实能把 PA04(SDA)/PA03(SCL) 拉低并回读为低、释放后两线回高（`idle=3`）、未发现角色接反（`swap=none`）。
- `BUS` 本轮 1 次读数 `ack=none`：0x08..0x77 共 112 个合法 7bit 地址**无一应答**。
- **与历史轮次逐字节对照**：`G` 行取值集合、`BUS` 取值集合、`IOTEST` 取值集合均与 EV-013/EV-014 相同；`IOSIG` 完整读数亦相同（两条被复位切断的前缀行都只是**采集窗口边界的伪影**，不是器件侧差异）。
- **受测板身份核对**：横幅 MCU 唯一编号 `uid=6A002C00`，与 EV-012（3 份）、EV-013（2 份）、EV-014（6 份）全部捕获**相同** ⇒ 本轮复测的**就是同一块传感器板**。

### 2.2 采样周期（`S` 行与紧随的 `G` 行）

| 捕获 | 窗口（UTC） | 字节 | 周期 `k` | `q` | `S` 行关键字段 | 紧随 `G` 行 |
|---|---|---|---|---|---|---|
| r1 | 03:47:10.242 → 03:52:10.449（内含下载复位 03:47:24） | 494 | 0, 3 | 0, 0 | `o=8 n=35 x=37 a=35` / `o=8 n=34 x=37 a=35` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` ×2 |
| s1 | 03:52:10.872 → 03:57:11.079（无复位，与 r1 仅隔 0.42 s） | 224 | 6, 9 | 0, 0 | `o=8 n=33 x=36 a=34` / `o=8 n=32 x=35 a=33` | 同上 ×2 |
| s2 | 03:57:11.538 → 04:02:11.764（无复位，与 s1 仅隔 0.46 s） | 113 | 12 | 0 | `o=8 n=32 x=34 a=32` | 同上 ×1 |

- **5/5 周期 `q=0`**；`t=0 h=0 p=0 H=0 V=0 D=0 r=0 s=0 y=0`（无有效温湿度、不推进前值、不置上报）。
- 以 03:47:24 的下载复位为锚，`r1+s1+s2` 三窗口首尾相接给出 `k=0,3,6,9,12` **五个连续周期（≈13 分钟）**；期间器件无任何成功周期、无自愈。
- 5 条 `G` 行**逐字节相同**，与 `BUS ack=none`、`a44=a45=0`、`rd=0`、`at=3` 逐项自洽；失败发生在**地址探测阶段**（`rd=0`，重试/CRC/量程分支尚未进入）。

### 2.3 `IOSIG` 事务内逐位回读签名：本能力独立重建期望值后判读

期望值由脚本内 `iosig_expected()` **独立推导**（20 个半位采样点：S0 START 建立、S1 SCL 拉低、S2..S17 地址字节 `0x88` 的 8 位各「低+高」、S18/S19 ACK 时隙），不引用实现常量：

- 无器件应答 ⇒ 期望 **`scl=95555`、`sda=30303`**；器件在 `0x44` 应答 ⇒ `sda=30300`。
- 实板完整读数（r1 reset#2）**为 `scl=95555 sda=30303`**；另 1 条被烧录复位从中途切断，其字节恰为完整行的前缀（按「复位切断」记 note，非缺陷）。
- 判读：SCL 的 START/8 位时钟/ACK 时隙形态**逐位**与主机驱动形态相符，SDA 的 8 个数据位等于被驱动的地址位 ⇒ **事务内的位与时钟确实送达引脚并被如实回读**；ACK 时隙两位=`11` ⇒ 地址 `0x44` 上无器件拉低 SDA，并与**同一次复位段**的 `BUS … ack=none` 交叉一致（脚本硬门禁）。

### 2.4 交叉对照（同一批观测内的其它通路）

`o=8` 恒成立（每周期 8 次 AIN11 转换全部成功）、`n≤a≤x` 逐周期成立、**周期内 min/max 离散度 ≤4 码**（最稳的 `32/34`，最散的 `35/37`），跨周期 `a` 在 32–35 之间。对照：EV-014（同日上午）为 80–117、EV-013（傍晚）为 ~3400–3624、EV-011（下午）为 130–388 ⇒ 码值随环境光照单调变化，本轮为**偏暗但有光环境**（与 readme 点 7 一致）。⇒ MCU、RTC 节拍、ADC、GPIO 供电窗口（PB05）、UART1 通路均正常，故障被进一步限定在 **U9 及其 I²C 连接**上。

### 2.5 原始证据清单（入库）

| 文件 | 字节 | md5 | 窗口（UTC） |
|---|---|---|---|
| `evidence/ev015/r1.cap` | 494 | `d6e407035995a157f2e3e12999880e35` | 03:47:10.242 → 03:52:10.449（内含下载复位 03:47:24） |
| `evidence/ev015/s1.cap` | 224 | `69af31dd288b0e3954dcff0cde26ee57` | 03:52:10.872 → 03:57:11.079 |
| `evidence/ev015/s2.cap` | 113 | `d5263dc8fed9afe68d4f1e40651d18ef` | 03:57:11.538 → 04:02:11.764 |

下载日志：`evidence/ev015/mdk_flash_logs.txt`（1 次，`Verify OK`）；窗口清单：`evidence/ev015/capture_windows.txt`；表格化摘要：`evidence/ev015/summaries.txt`（`summarize.py`）。扩展名刻意用 `.cap`：仓库 `.gitignore` 含 `*.bin`，`collect.py` 逐份按字节与 md5 复核。

## 3. 独立判定（本能力自建资产，不引用实现自述）

| 运行 | 对象 | 结果 |
|---|---|---|
| 实板判定（本轮） | `evidence/ev015/{r1,s1,s2}.cap` | **60 项通过 / 1 项失败**（唯一失败 = 核心验收 `H ACCEPT`：最长连续 `q=1` = 0）→ 原文 `evidence/ev015/verify_run.txt`，退出码 1 |
| **定向变异自检** | 合成捕获（正确签名、SCL 错、数据位错、ACK 时隙混合、ACK/无 ACK 与 BUS 双向矛盾、`IOSIG` 位置错、缺失、复位切断行、缺 CRLF 类缺陷、非前缀乱码切断） | **12/12 与预期一致**（exit 0）→ `evidence/ev015/verify_selftest.txt` ⇒ 判据**既能通过也能失败**，非恒过的空验 |
| 宿主 harness 复跑（7 份，本轮对**当前工作树**重跑） | `USER/src/*.c` | **85 / 93 / 51 / 36 / 56 / 79 / 79 通过，0 失败（合计 479）** → 原文 `evidence/ev015/harness_results.txt` |
| 手册已知答案 KAT | 交付 `fw_core.c` | `CRC(0xBEEF)=0x92` **MATCH**；`raw_t=0x6666,raw_h=0x8000 → 25.0 ℃ / 56.5 %RH`；越域样本 `0x0000`/`0xFFFF` 判无效 → `evidence/ev015/datasheet_kat.txt` |

### 3.1 主机侧与协议/换算侧复核（结论：未被推翻）

- 采样路径唯一命令为 `0xFD`；`GXHT40_CMD_SOFT_RESET`(0x94) 宏**零调用**；无加热命令（只读静态复核）。
- 总线恢复 `i2c_bus_recover()`：先释放两线，仅当 SDA 仍被拉低才补 **≤9 个 SCL 脉冲**并补一个**合法 STOP**，有界、不发地址、不发读、不写命令。
- 失败后 `s_addr7 = 0`（缓存地址失效）、下一周期**先做总线恢复再依次重探 `0x44`/`0x45`**（FWR-118）。
- 手册对齐（本能力亲自读 `gxht40.pdf`）：DFN4 `1=SDA/2=SCL/3=VDD/4=VSS` 且**无复位脚**；全系列仅 `0x44`/`0x45` 两个 I²C 地址；`t_MEAS.H` 最大 8.3 ms ≤ 固件 `GXHT40_MEASURE_WAIT_MS=10`；`t_PU` 最大 1 ms ≤ 固件 `GXHT40_POWER_ON_WAIT_MS=2`；`R_P ≥ 390 Ω`、`V_IL ≤ 0.3·V_DD`、`V_IH ≥ 0.7·V_DD` 板级 4.7 KΩ 上拉 + 开漏驱动均满足；CRC poly `0x31`/init `0xFF`/xorout `0x00` 且**手册示例在交付代码上 MATCH**；换算与 §7.5 式(1)(2) 逐点一致。
- 网表复核：`N05845 = R1(4.7K) + U7.PA04 + U9.1(SDA)`、`N06024 = R2(4.7K) + U9.2(SCL) + U7.PA03`、`BAT = U7.VDD + U9.VCC`（同网、无开关、无固件可控供电门）、`U9.5 = PAD → NC`；与固件 `measure.c` 的 PA04=SDA / PA03=SCL **逐项一致**。

## 4. 失败环节定位（本轮一手证据闭合到哪一层）

| 层 | 本轮实测 | 判定 |
|---|---|---|
| 主机引脚驱动/回读 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none` ×2 | 能拉低、能释放、未接反 |
| 事务内位/时钟送达 | `IOSIG scl=95555 sda=30303` | 位与时钟形态真实成立（H12 维持排除） |
| 器件地址应答 | `BUS … ack=none`（112 个合法 7bit 地址）、`IOSIG` ACK 时隙 `11` | **无任何地址被应答** |
| 协议层（命令/读/CRC/量程/重试） | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` ×5 | **未进入**（`rd=0`：连读事务都未发起） |
| 采样/判定/冻结不变量 | `q=0 p=0 H=0 V=0 r=0 s=0`、`raw` 全 `-` | 按设计生效（不上报/不推进前值/不构造 0 值） |
| 其它通路（ADC/GPIO/UART/RTC） | `o=8` 恒成立、离散度 ≤4 码、节拍 Δk=3 | 正常 |

⇒ 本轮失败环节与前几轮一致：**「器件是否在总线上应答」这一层（地址级）**。主机侧可改变的因素已由实板数据逐条覆盖，**未发现**新的主机侧可修项。

## 5. 竞争假设状态（本轮更新）

| 假设 | 状态 | 依据 |
|---|---|---|
| H1 器件贴装/焊接不良 | **开放（现场）** | 用户已重焊，现象逐字节未变；本能力无目视/通断手段，不能据字节流排除 |
| H2 器件损坏 / 非本系列件 | **开放（现场）** | 112 地址全域不应答；手册确认全系列仅 0x44/0x45 ⇒ 型号/地址变体假设已排除 |
| H3 器件异常闩锁（需真正断电） | **开放（现场）；权重较高** | 手册 §7.7 确认器件只能靠**断电**复位；U9 无复位脚、与 MCU 同 `BAT` 网、无开关、CR2032 焊接 ⇒ MCU 复位不给 U9 断电 |
| H4 SDA/SCL 接反 | **排除** | `IOTEST swap=none` ×2 + 网表复核 |
| H5 主机拉不低/拉不高 | **排除** | `IOTEST sda_lo=0 scl_lo=0 idle=3` ×2 |
| H12 事务内位/时钟未送达 | **排除** | `IOSIG scl=95555 sda=30303` + 与 `BUS ack=none` 交叉一致 |
| 上拉缺失/总线卡低 | **排除** | `idle=3`；恢复原语有界 |
| 地址变体 | **排除** | 手册确认全系列仅 0x44/0x45；`BUS` 扫 112 地址 `ack=none` |
| 读未就绪 / CRC / 量程 / 重试策略 | **排除（本轮）** | 失败在地址探测阶段（`rd=0 at=3`）；手册时序/换算逐条对齐；479 项 harness 全绿；KAT 通过 |
| 光照/ADC/RTC/UART 通路异常干扰 | **排除** | §2.4 交叉对照 |

## 6. 本轮未覆盖项与证据缺口（如实登记）

1. **「主机能否正确判定 ACK」在本板上仍缺正向对照**：本板 I²C 总线上除 U9 外无第二个从机（§3.1 网表），没有「真器件应答、主机读到 0」的实板样本。覆盖靠三层间接证据：bit 级 harness（mock 从机在正确时隙被采到 ACK）、`IOSIG` 在真实总线上回读到 ACK 时隙的实际电平、§2.3 的采样点时序算术。
2. **实板 `raw` 十六进制形态**仍只有宿主覆盖（实板 `rd=0`，从未读回帧）——器件不响应的直接后果。
3. **general call 地址 `0x00` 未被探测**（`BUS` 只扫 0x08..0x77 合法 7bit 地址）。登记为未测项；本项验收不涉及 general call。
4. **无逻辑分析仪/示波器/万用表/可控电源**：这些**不是**本项阻塞理由，亦**未**以任何未提供仪器作为门槛。
5. **`LIGHT_DARK_CALIBRATED=0`**：全暗基准仍未标定。本轮光照读数（`a` 32–35，有光）**不作、也不能作**全暗标定值；本项 `V=0 D=0` 说明未标定门禁按设计生效。
6. **手机端读数一致性未实测**：`r=0`（永不满足上报条件）⇒「满足上报条件时手机端与该周期 `S` 行 `t/h` 为同一笔读数」这一子句**在本轮构造上无法被触发**，故未调用 `android_test`（不虚构未发生的上报）。
7. **登记偏差 D-01**（上报方向仍为降温 `(prev-cur)>9`，readme 要求升温）在本轮构建上**仍然存在**（只读复核 `fw_core.c`），属固件任务 T4 范围，**不属本项**验收。
8. **文档同步**（非本能力职责）：`IOTEST`/`IOSIG` 两行与实际上电顺序 `BOOT→IOTEST→IOSIG→BUS` 仍未收录进 TD-002/FD-002 正文（EV-012/EV-013/EV-014 已登记，本轮再次确认）。

## 7. 判定与交接

**判定：`TEST_FAIL`。** 本项验收要求的**实际板上行为**（每 3 分钟周期 `S` 行 `q=1`、`t/h` 落有效域、连续多周期稳定）在当前构建与当前受测板上**未观察到**，本轮 5/5 周期 `q=0`，且失败形态逐字节稳定可复现。按能力契约，实际行为不符合当前验收要求即返回 `TEST_FAIL`，并如实交付复现、预期/实际、竞争假设与最小区分实验；未知根因**不**被本能力用来宣告外部阻塞，**不**用来宣告器件损坏，也**不**代替实现者判定「不可修复」。

**预期 / 实际（一句话对照）**：期望每个 3 分钟周期 `S` 行 `q=1` 且 `t/h` 在 −40.0..125.0 ℃ / 0..100.0 %RH 且连续 ≥3 周期稳定；实际连续 5 个周期 `q=0`、`t/h=0`、各紧随 `G s=2 a44=0 a45=0 rd=0 at=3`，器件在 112 个合法地址上均不应答。

**复现步骤（可原样重跑）**

1. `serial_list_ports` 核对 COM42（Prolific `067B:2303`）与 CW-DAPLink（`C251:F001`）。
2. 运行 `sh evidence/ev015/seq.sh r1 s1 s2`（先启 300 s `serial_capture`，4 s 后 `mdk_flash {}`，再背靠背续采两个 300 s 窗口）。
3. `python evidence/ev015/collect.py <work-dir> evidence/ev015` 收纳原件；`python evidence/embedded_verify_gxht40_item002.py evidence/ev015/r1.cap evidence/ev015/s1.cap evidence/ev015/s2.cap` 判定。
4. 期望的恢复判据（一眼可判）：`IOSIG` 的 ACK 时隙由 `11` 变为 `00`（即 `sda=30300`），或 `BUS ack` 由 `none` 变为含 `44`/`45`。

**最小区分实验（供实现能力与现场选择；本能力不替其决定）**

- E-a（现场，本能力无法替代）：**让 U9 真正失去供电 ≥5 s 再恢复**。注意本板 CR2032 焊接、`BAT` 网无开关，`mdk_flash` 的复位只复位 MCU、**不给 U9 断电** ⇒ 需断开 `BAT` 网本身。若重焊时 U9 只是被拖焊而从未真正断电，此步可清除闩锁态。
- E-b（现场）：电气核查 U9 四处焊点 `U9.1→PA04`、`U9.2→PA03`、`U9.3→BAT`、`U9.4→GND`（通断/阻值）。
- E-c（现场）：换一只已知良好的 GXHT40（或 `GXHT40-BD`，地址 `0x45` 同样被扫描）作对照。
- E-d（实现/测试，无需新仪器）：若实现侧能给出**在地址不应答条件下仍可观察的新的主机侧判别量**（例如对 ACK 采样点更细的时序/回读证据），本能力可在同一脚本上复跑；本轮判定脚本已接受新行，扩展成本低。

**交实现能力**：本能力**未发现**新的、可由当前证据定位到固件源码的缺陷；本轮以 7 份宿主 harness（479 项全绿）+ 手册已知答案 KAT + 静态复核**重新确认**交付件符合性未被推翻。是否、以及如何改动，由实现能力依自身职责与本节证据决定；本能力不代其判定不可修复，也不终止其重试路径。

**交现场/用户（最小必要动作）**：上表 E-a → E-b → E-c，任一步即可分支处置；恢复后**无需**任何固件改动，只需重新上电 + COM42 采集（约 16 分钟给出「`q=1` 是否出现 + 连续周期」判定）。

**交设计能力**：TD-002/FD-002 的上电行集合与顺序（`BOOT→IOTEST→IOSIG→BUS`）同步。

**已交付的端口**：`evidence/test.md`（本文件，TestEvidence，`TEST_FAIL`）；`evidence/programmed_prototype.md`（本轮下载身份与回读校验：1 次 `Verify OK`、axf md5 与交付件一致）。

---

# 嵌入式验证证据（EV-014 / run8 ITEM-002 ↔ GXHT40 温湿度采集在真实目标上恢复有效）

状态：embedded_tester.embedded_verification 独立验证证据（本轮**重新下载、重新采集真实目标原始字节流**；历史结论只作背景，不作本轮结果）
本轮触发：`operator_update`（2026-10-09T01:13:57Z）—— **用户已重新焊接 GXHT40（U9）温湿度芯片**，授权重做 ITEM-002；并明确「旧无 ACK 与 q=0 记录属焊接前条件，不可作为本轮结论」「先用最新固件实板复测并保存新的 BUS/G/S 原始日志，必要时通过现有 DAP 重新下载」「若现有固件恢复有效，不为重做而盲改代码」。

- 受测提交：`58978b8`（HEAD；接手时 `git status` 干净；本轮**未改产品源码**）
- **被测件 = 当前交付件本身**：传感器固件源码自 `ef03c13`（上一轮 `firmware_implementation` 交付提交）以来**零改动** —— `git diff --stat ef03c13 HEAD -- <FW>/USER` 为空、`git log -1 -- <FW>/USER` = `ef03c13`；本轮两次 `mdk_flash` 所下的 axf md5 = **`5c2fdb58d0fc4ef7dac8a21e343082a3`**（hex `db3dbc72195d346815500b8ef158b90f`），与实现交付声明、EV-013 受测件**逐字符一致**。⇒ 本轮验的就是当前交付件，**无需也不应改代码**。
- 验证基线：TD-002 rev 5.0；FWR-002 v5.0（FWR-116/117/118）；FD-002 rev 5.0；`readme.txt`；**本轮新增的第一手独立基线**：`gxht40.pdf`（GXHT4x Family Datasheet V2.6, Aug-2026，用户已放入仓库，本轮本能力**亲自逐页读取**）与 `sensor_hardware/{pstxnet.dat,MAIN_BOARD.BOM}`（本能力**独立重解析**，见 §3.1）。

结论：**BLOCKED** —— 用户已执行 EV-013 §7 第 1 项所指的现场动作（对 U9 重新焊接），但**重焊后实板失败形态与重焊前逐字节相同**：2 次成功下载、6 个采集窗口跨 34 分钟、**11 个采样周期全部 `q=0`**、2 次独立复位，全地址扫描仍 `ack=none`，`IOSIG` ACK 时隙仍读为 `11`。本轮把「主机侧」剩余因素全部**在实板上再次独立复现排除**，并用**用户新提供的手册与网表**把固件侧的协议/换算/电气余量逐条对齐（含手册自带 CRC 已知答案 `CRC(0xBEEF)=0x92` 在交付代码上**实测通过**）；同时从手册/网表**新证实两条事实**（§3.3）：① U9 **无复位引脚**（DFN4 仅 SDA/SCL/VDD/VSS），其状态只能靠**断电**改变；② 器件**不 ACK 地址即收不到任何命令**，故连软复位 `0x94` 与 general-call 复位在物理上都到不了器件 —— 即**任何固件改动都不可能让本器件恢复应答**。剩余路径只有现场动作（真断电复上电 / 电气核查 / 换件），现有资源无法替代。

| # | 本轮事实（全部为本轮新采集） | 性质 | 依据 |
|---|---|---|---|
| 1 | 重焊后实板 **0/11 个采样周期 `q=1`**；核心可观察（`q=1`、`t/h` 有效域、连续多周期稳定）**未达成** | 功能不符（本项核心验收不通过） | §2.2 |
| 2 | 失败形态与重焊前**逐字节相同**：11 条 `G` 行全为 `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`；2 条 `BUS` 行全为 `BUS idle=3 scl=1 sda=1 ack=none`（对照 EV-013 §2 的同名行，逐字符一致） | 重焊未改变可观察现象；排除「偶发/焊接瞬态」 | §2.1 |
| 3 | `h1→h2→h3` 三窗口首尾相接（01:35:11→01:50:13，无间隙 15 分钟），构成 **`k=0,3,6,9,12,15` 六个连续周期**，器件全程无应答、无自愈 | 失败稳定、非偶发；排除「需等待自愈」 | §2.2 |
| 4 | 2 次独立下载（01:16:18 / 01:35:25）后上电行逐字节相同：`IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none` ×4、`IOSIG scl=95555 sda=30303` ×2、`BUS … ack=none` ×2；两条横幅的 MCU 编号 `uid=6A002C00` 与 EV-012/EV-013 全部捕获**完全相同** | 主机侧行为可复现、与重焊前一致；**同一块受测板**（无换板混淆） | §2.1、§2.3 |
| 5 | `IOSIG` 实板读数 `scl=95555 sda=30303`：SCL 的 START/8 位时钟/ACK 时隙形态**逐位**与本能力独立推导的期望相符，SDA 的 8 个地址位等于被驱动的 `0x88`；ACK 时隙两位=`11` ⇒ 地址 `0x44` 上无器件拉低 SDA（与同复位段 `BUS ack=none` 交叉一致） | 主机在事务内**确实把位与时钟送到引脚并如实回读** | §2.3 |
| 6 | 光照通路持续工作且随环境变化：`o=8` 恒成立、每周期 `n≤a≤x`、**周期内 min/max 离散度 ≤4 码**，跨周期 `a` 在 80–117 之间浮动（对照 EV-013 傍晚 ~3600、EV-011 下午 130–388） | 交叉对照：MCU/时钟/ADC/GPIO/UART1 通路正常，且读数随真实光环境变化；本轮环境为**有光**（与 readme 点 7 一致） | §2.4 |
| 7 | 失败周期**不上报、不推进前一有效温度、不构造 0 值**：`p=0 H=0 V=0 D=0 q=0 r=0 s=0 y=0`、`raw=------------` | 冻结不变量在实板成立 | §2.2 |
| 8 | 交付代码对**手册自带已知答案** `CRC(0xBEEF)=0x92` 实测结果为 `0x92`（MATCH）；换算与手册 §7.5 式(1)(2) 逐点一致（`0x6666/0x8000→25.0℃/56.5%RH`；越域样本 `0x0000`/`0xFFFF` 判为无效） | 交付件的 CRC 与换算**与器件手册精确对齐**（本轮新增的独立校核） | §3.4 |
| 9 | 6 份本能力自有宿主 harness 在**当前工作树**上全部复跑通过（85/93/51/36/79×2/56，0 失败）；定向变异自检 12/12；实板字节流判定 123 项通过 / 1 项失败（唯一失败=核心 `H ACCEPT`） | 判据非空验、实现符合性未被推翻 | §4 |

---

## 1. 环境、构建与部署（本轮实际调用，原始结果）

| 工具 / 命令 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `COM42 = Prolific USB-to-Serial Comm Port (VID:PID 067B:2303)`；`COM26 = USB 串行设备 (C251:F001, SN 87094109484987710672FF50)` | 传感器调试口 **COM42 在位**，与配置声明的设备身份一致 |
| `usb_list_devices` | `USB\VID_C251&PID_F001`（HID MI_02 + Ports MI_00 = COM26，SN `87094109484987710672FF50`，Status OK） | **CW-DAPLink 下载探针在位** |
| `git diff --stat ef03c13 HEAD -- <FW>/USER` | 空 | 受测源码 = 上一轮已交付并已实测的源码 |
| `md5sum …/Project.axf` | `5c2fdb58d0fc4ef7dac8a21e343082a3`（`Project.hex` = `db3dbc72195d346815500b8ef158b90f`） | 与实现交付声明、EV-013 受测件**完全一致** ⇒ 受测件身份闭合 |
| `mdk_flash {}` ×2（本地 09:16:18 / 09:35:25） | 每次均 `Erase Done.Programming Done.Verify OK.Application running ...`；下载前 build 为 `0 Error(s), 0 Warning(s)`（无可重建项 ⇒ 所下即上面那个 axf） | 当前交付镜像已下载并**回读校验成功**。`Verify OK` **仅**证明下载校验，业务功能仍须实测（§2） |
| `serial_capture` ×6（COM42@9600，300 s/次；`f1`/`h1` 与 `mdk_flash` 串接于同一 shell 脚本，使复位横幅落在窗口内；`h1→h2→h3` 首尾相接无间隙） | `evidence/ev014/{f1,g1,g2,h1,h2,h3}.cap`（495/226/113/494/230/226 B，逐字节账目闭合） | 原始字节已入库；采集窗口与 md5 见 §2.5 |
| `git status --short`（提交前） | 仅本能力声明的输出：新增 `evidence/ev014/`，修改 `evidence/test.md`、`evidence/programmed_prototype.md`、`file_manifest.txt` | 产品源码与 Keil 工程**零改动**；无无关改动被卷入提交 |

授权与入口：本轮 context 的 `当前能力有效工具` 与 `本次允许调用的工具及参数` 均非空（9 个工具），实际调用 4 个（`serial_list_ports`、`usb_list_devices`、`mdk_flash`、`serial_capture`），未用 `mdk_build`（`mdk_flash` 内含编译，且实测「无可重建项」）、未用 `install_*`/`android_test`（理由见 §6 第 6 条）。`serial_capture`/`mdk_flash` 经 `tool_cli.py` CLI 入口（`evidence/ev014/{run,seq}.sh`，使用**本轮** context 路径 `CALL-4aaa5528-…`）。

## 2. 真实目标 UART1 观测（COM42 @9600 8N1，原始字节）

### 2.1 上电行（BOOT / IOTEST / IOSIG / BUS）

| 捕获 | 复位段 | 上电行（逐字节原文） |
|---|---|---|
| f1 | reset#1（窗口起点，`rst=0040`） | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` + `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none` + `IOSIG scl=95555 sda=30303`（值完整，但**行尾 CRLF 被下载复位吃掉**而直接粘连下一条 `BOOT`，即 `…sda=30303BOOT fw=…`；该段无 BUS） |
| f1 | reset#2（`rst=0240`，下载复位 01:16:18） | `IOTEST …` + **`IOSIG scl=95555 sda=30303`** + `BUS idle=3 scl=1 sda=1 ack=none` |
| h1 | reset#1（窗口起点，`rst=0040`） | `BOOT …` + `IOTEST …` + `IOSIG scl=95555 sda=3030`（**被下载复位从行中切断**，其后紧接下一条 `BOOT`） |
| h1 | reset#2（`rst=0240`，下载复位 01:35:25） | `IOTEST …` + **`IOSIG scl=95555 sda=30303`** + `BUS idle=3 scl=1 sda=1 ack=none` |

- 上电顺序实测 `BOOT → IOTEST → IOSIG → BUS`，四行紧邻（脚本 L 组按相邻性硬门禁）。
- `IOTEST` 本轮 4 次读数**逐字节相同**：主机确实能把 PA04(SDA)/PA03(SCL) 拉低并回读为低、释放后两线回高（`idle=3`）、未发现角色接反（`swap=none`）。
- `BUS` 本轮 2 次读数逐字节相同，`ack=none`：0x08..0x77 共 112 个合法 7bit 地址**无一应答**。
- **重焊前后逐字节对照（本轮新做的机械核对）**：把 EV-013 四份入库捕获与本轮六份捕获按行首标记分组后取**取值集合**比对 —— `G` 行：EV-013 7 条 / 本轮 11 条，两侧取值集合**完全相同**（唯一值 `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`）；`BUS`：两侧各 2 条、唯一值 `BUS idle=3 scl=1 sda=1 ack=none`，集合相同；`IOTEST`：两侧各 4 条、唯一值 `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none`，集合相同；`IOSIG`：两侧完整读数均 `scl=95555 sda=30303`（EV-013 另有 1 条被 halt 切断的前缀、本轮另有 1 条 CRLF 被复位吃掉而粘连下一条 `BOOT` 的同值行——两者都只是**采集窗口边界的伪影**，不是器件侧差异）。⇒ 用户重焊 U9 后，上电自检/总线诊断/失败周期**全部可观察量没有任何变化**。
- **受测板身份核对**：本轮两条横幅的 MCU 唯一编号均为 `uid=6A002C00`，与 EV-012（3 份捕获）、EV-013（2 份捕获）**完全相同**（逐份正则提取比对）⇒ 本轮复测的**就是同一块传感器板**，重焊前后/多轮之间不存在「换了一块板」的混淆。

### 2.2 采样周期（`S` 行与紧随的 `G` 行）

| 捕获 | 窗口（UTC） | 字节 | 周期 `k` | `q` | `S` 行关键字段 | 紧随 `G` 行 |
|---|---|---|---|---|---|---|
| f1 | 01:16:03.059 → 01:21:03.266（内含下载复位 01:16:18） | 495 | 0, 3 | 0, 0 | `o=8 n=96 x=98 a=96` / `o=8 n=94 x=97 a=95` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` ×2 |
| g1 | 01:22:36.364 → 01:27:36.561（无复位） | 226 | 9, 12 | 0, 0 | `o=8 n=97 x=100 a=98` / `o=8 n=91 x=93 a=92` | 同上 ×2 |
| g2 | 01:27:37.101 → 01:32:37.311（无复位，与 g1 仅隔 0.5 s） | 113 | 15 | 0 | `o=8 n=94 x=97 a=95` | 同上 ×1 |
| h1 | 01:35:11.602 → 01:40:11.845（内含下载复位 01:35:25） | 494 | 0, 3 | 0, 0 | `o=8 n=86 x=89 a=87` / `o=8 n=84 x=87 a=85` | 同上 ×2 |
| h2 | 01:40:12.372 → 01:45:12.568（无复位，与 h1 仅隔 0.5 s） | 230 | 6, 9 | 0, 0 | `o=8 n=116 x=120 a=117` / `o=8 n=104 x=107 a=105` | 同上 ×2 |
| h3 | 01:45:13.023 → 01:50:13.228（无复位，与 h2 仅隔 0.5 s） | 226 | 12, 15 | 0, 0 | `o=8 n=80 x=82 a=80` / `o=8 n=82 x=84 a=82` | 同上 ×2 |

- **11/11 周期 `q=0`**；`t=0 h=0 p=0 H=0 V=0 D=0 r=0 s=0 y=0`（无有效温湿度、不推进前值、不置上报）。
- 以 01:35:25 的下载复位为锚，`h1+h2+h3` 两个相邻窗口链（01:35:11→01:50:13，**无间隙 15 分钟**）给出 `k=0,3,6,9,12,15` **六个连续周期**；期间器件无任何成功周期、无自愈。EV-012 曾出现的 `k=6` 缺口本轮**已被填补**。
- 11 条 `G` 行**逐字节相同**，与 `BUS ack=none`、`a44=a45=0`、`rd=0`、`at=3` 逐项自洽；失败发生在**地址探测阶段之前**（`rd=0`），重试/CRC/量程分支尚未进入 —— 与 EV-013 结论一致。

### 2.3 `IOSIG` 事务内逐位回读签名（E1b）：本能力独立重建期望值后判读

期望值由脚本内 `iosig_expected()` **独立推导**（20 个半位采样点：S0 START 建立、S1 SCL 拉低、S2..S17 地址字节 `0x88` 的 8 位各「低+高」、S18/S19 ACK 时隙），不引用实现常量：

- 无器件应答 ⇒ 期望 **`scl=95555`、`sda=30303`**；器件在 `0x44` 应答 ⇒ `sda=30300`。
- 实板 2 次完整读数（f1 reset#2、h1 reset#2）**均为 `scl=95555 sda=30303`**，另有 1 条被烧录 halt 从中途切断（其字节恰为完整行的前缀，按「复位切断」记 note，非缺陷）。
- 判读：SCL 的 START/8 位时钟/ACK 时隙形态**逐位**与主机驱动形态相符，SDA 的 8 个数据位等于被驱动的地址位 ⇒ **事务内的位与时钟确实送达引脚并被如实回读**；ACK 时隙两位=`11` ⇒ 地址 `0x44` 上无器件拉低 SDA，并与**同一次复位段**的 `BUS … ack=none` 交叉一致（脚本 L 组硬门禁）。

### 2.4 交叉对照（同一批观测内的其它通路）

`o=8` 恒成立（每周期 8 次 AIN11 转换全部成功）、`n≤a≤x` 逐周期成立、**周期内 min/max 离散度 ≤4 码**（最稳的一周期 `80/82`，最散的一周期 `116/120`），跨周期 `a` 在 80–117 之间浮动。对照：EV-011 下午（16:20–16:47）为 130–388、EV-012/EV-013 傍晚为 ~3400–3624 ⇒ 码值随环境光照单调变化，本轮为**有光环境**（与 readme 点 7「当前的情况是有光的」一致）。⇒ MCU、RTC 节拍、ADC、GPIO 供电窗口（PB05）、UART1 通路均正常，故障被进一步限定在 **U9 及其 I²C 连接**上。

### 2.5 原始证据清单（入库）

| 文件 | 字节 | md5 | 窗口（UTC） |
|---|---|---|---|
| `evidence/ev014/f1.cap` | 495 | `0d5bdbe62527613ccdffe4b18cdfa9af` | 01:16:03.059 → 01:21:03.266 |
| `evidence/ev014/g1.cap` | 226 | `0bf54dd895e349c682428ea6e6a3bda1` | 01:22:36.364 → 01:27:36.561 |
| `evidence/ev014/g2.cap` | 113 | `f99bfe782498a00b0ddf84c2137d68ec` | 01:27:37.101 → 01:32:37.311 |
| `evidence/ev014/h1.cap` | 494 | `8cf82d5dc5cba27b3d71eed4df2e4f7b` | 01:35:11.602 → 01:40:11.845 |
| `evidence/ev014/h2.cap` | 230 | `b95dc1f80f9ba3c064cbc4b4f2baa7fd` | 01:40:12.372 → 01:45:12.568 |
| `evidence/ev014/h3.cap` | 226 | `cd6158c62b1787572e6f51cc0915565b` | 01:45:13.023 → 01:50:13.228 |

下载日志：`evidence/ev014/mdk_flash_logs.txt`（2 次，均 `Verify OK`）；窗口清单：`evidence/ev014/capture_windows.txt`；表格化摘要：`evidence/ev014/summaries.txt`（`summarize.py`）。扩展名刻意用 `.cap`：仓库 `.gitignore` 含 `*.bin`（EV-012 的 `.bin` 原始捕获因此从未进入 Git，EV-013 已改用 `.cap`），`collect.py` 逐份按字节与 md5 复核。

## 3. 失败环节定位（本能力独立复核）

### 3.1 网表独立重解析（本轮亲自做，不引用他人结论）

本能力用自写解析器**重新解析** `sensor_hardware/pstxnet.dat`，得到完整引脚归属（不是只看 U9）：

| 网络 | 节点 | 说明 |
|---|---|---|
| `N05845` | `R1.2`（4.7K 上拉）+ **`U7.14 = PA04`** + **`U9.1 = SDA`** | I²C 数据线，**除 U9 外无第二个从机** |
| `N06024` | `R2.1`（4.7K 上拉）+ **`U9.2 = SCL`** + **`U7.13 = PA03`** | I²C 时钟线，同样只有 U9 |
| `BAT` | 电池 `J1.1` + `U7.9 = VDD` + **`U9.3 = VCC`** + R1.1/R2.2 + C7/C8/C9/C13–C16 + `U4.6` + `J2.1` | **U9 与 MCU 同网**；全网**无开关、无固件可控门、无稳压器** |
| `GND` | `U9.4 = VSS` + `J4.2` + 其余地脚 | U9 地脚成网 |
| `NC` | **`U9.5 = PAD`**、（另含 U7 的 PB06/PA00/PA01/PA02/PB00/PB01 等未用脚） | U9 的**焊盘脚悬空**（手册 DFN4 仅 4 个功能脚，见 §3.2） |
| `UART1_TXD` | `U7.16 = PA06` → `J3.1` | 调试串口**发送**（本轮实测 9600 8N1 逐字节可解 ⇒ PA06 的实际引脚映射被证实） |
| `UART1_RXD` | `U7.15 = PA05` → `J3.2` | 调试串口**接收**（固件全 `USER/src` 无任何 RX 读取/命令解析代码 ⇒ 本项无 UART 命令接口，故本轮未用 `serial_exchange`） |
| `LIGHT_ADC` | `U7.1 = PB04` + `R3.1`（5M）+ `J4.1` | 光照分压中点 |
| `LIGTHT_POWER` | `U7.2 = PB05` + `R3.2` | 分压供电脚 |

- 与交付源码逐条比对：`measure.c` 的软 I²C 引脚为 **CW_GPIOA `GPIO_PIN_4` = PA04（SDA）**、**`GPIO_PIN_3` = PA03（SCL）** ⇒ **固件驱动的引脚与网表中 U9 的 SDA/SCL 引脚是同一对引脚**，不存在「固件驱动了别的脚」或「U9 挂在别的脚」。
- 光照分压拓扑（`LIGTHT_POWER/PB05 —R3(5M)— LIGHT_ADC/PB04 与 J4.1 —(外接光敏元件)— J4.2/GND`）与 §2.4 的读数形态**相符**：低码（80–117）且周期内离散度 ≤4 码，说明该节点被一条**真实的低阻分压支路**拉低，而不是完全悬空引脚的大幅漂移噪声（登记为**相符性推断**，非电气测量结论 —— 本轮无万用表/示波器）。

### 3.2 器件手册独立读取（本轮新增的一手基线）

本能力直接读取仓库内 `gxht40.pdf`（GXHT4x Family Datasheet **V2.6, Aug. 2026**），逐条核对：

| 手册条目 | 手册内容 | 交付件/板级实际情况 | 判定 |
|---|---|---|---|
| §5 表 1（引脚） | DFN4：**1=SDA、2=SCL、3=VDD、4=VSS**；**无复位脚** | 网表 U9.1/2/3/4 与之一一对应（§3.1） | **一致** |
| §1 基本性能 + 封装信息表（地址） | 全系列仅 **2 个 I²C 地址**：`0x44`（GXHT40-AD / GXHT41-AD / GXHT45-AD）与 `0x45`（GXHT40-BD） | 固件候选即 `0x44`/`0x45`；`BUS` 另扫 0x08..0x77 **全部 112 个合法地址**无一应答 | **地址变体假设对全系列排除** |
| §6.4 表 5（时序） | 高重复率测量 `t_MEAS.H` 典型 **6.9 ms** / 最大 **8.3 ms**（VDD=1.6 V 时测得） | `GXHT40_MEASURE_WAIT_MS = 10` ⇒ **≥ 手册上限**，满足本项「等待下限不低于高重复率测量时间」 | **满足** |
| §6.4 表 5（上电） | `t_PU` 典型 0.3 ms / 最大 1 ms | `GXHT40_POWER_ON_WAIT_MS = 2` ⇒ **≥ 手册上限** | **满足** |
| §6.3 表 4（电气） | `V_IL ≤ 0.3·V_DD`、`V_IH ≥ 0.7·V_DD`；上拉电阻 `R_P 最小 390 Ω`；总线电容 `C_b ≤ 400 pF` | 板级 4.7 KΩ 上拉（≥390 Ω，表 4 未给上限）；开漏驱动 + 3 V 上拉下 `V_IL≈0.1–0.2 V`、`V_IH≈3 V`，两侧均有余量 | **满足** |
| §7.3 表 8（CRC） | CRC-8、poly **0x31**、init **0xFF**、不反射、末异或 0x00；**示例 `CRC(0xBEEF)=0x92`** | 交付代码实测 `CRC(0xBEEF)=0x92`（§3.4 KAT） | **一致（已知答案通过）** |
| §7.4 表 9（命令） | 测量 `0xFD/0xF6/0xE0`、序列号 `0x89`、**软复位 `0x94`**、加热 `0x39/0x32/0x2F/0x24/0x1E/0x15` | 周期路径**只用 `0xFD`**；`0x94` 宏零调用；无加热命令 | **符合本项约束** |
| §7.5（换算） | `RH=(-6+125·S_RH/2^16) %RH`；`T=(-45+175·S_T/2^16) ℃` | 固件常量 `-60/1250`（湿度 x10）、`-450/1750`（温度 x10）、分母 `65536`、四舍五入 `32768` | **精确一致** |
| §7.1（读未就绪） | 转换未完成时对**读地址返回 NACK**；数据被读走即清除 | 固件对读地址 NACK 做**有界重读**（`GXHT40_READ_RETRY=5`，间隔 1 ms）后整帧重测（`MEAS_RETRY=3`） | **符合** |
| §7.1（时钟拉伸） | **不支持 clock-stretching** | 主机不依赖拉伸，纯开漏 + 固定半位延时 | **符合** |
| §7.7（复位方式） | ① 软复位命令 ② **general call（地址 0x00 + 命令 0x06）** ③ **断电** | 周期路径不用 ①；②③ 见下 | 见 §3.3 |
| §7.9 图 12（典型应用） | MCU + SDA/SCL **10 KΩ 上拉** + **VDD 100 nF 去耦** | 板级 4.7 KΩ 上拉 + `C7`(0.1 µF)/`C8`(470 pF)/`C9`(2.2 µF) 在 `BAT`↔`GND` 上 | **同类且满足要求** |

### 3.3 本能力从手册+网表**新证实**的两条事实（决定「固件已无路可走」）

1. **U9 没有复位引脚**（手册 §5 表 1 只有 SDA/SCL/VDD/VSS；网表 U9.5 = `PAD` 在 `NC` 网）。⇒ 器件状态**只能通过断电**改变；MCU 复位（`rst=0240`，PIN|SYSRESETREQ）**不会给 U9 断电**（同 `BAT` 网、无开关、且 readme 载明 CR2032 **焊接**在板上，`J1` 无取出式电池仓）。用户重焊时器件是否曾**真正断电**取决于当时是把芯片**取下重贴**还是**仅重新拖焊引脚**——本能力无法从字节流判别，故**在两种情形下都不能排除「器件的异常状态尚未被一次真正的冷启动清除」**。
2. **器件不 ACK 地址 ⇒ 任何命令都到不了器件**。手册 §7.1：所有通信以 START + **地址字节**开始，从机必须 ACK 才能继续。实板 `BUS ack=none`（112 地址）与 `IOSIG` ACK 时隙 `11` 都表明 U9 连自己的地址都不应答。⇒ 连**软复位 `0x94`**、**general-call 复位（0x00+0x06）**、加热等命令**在物理上都无法送达**；「用固件命令把器件救回来」在原理上不可能实现，因此**不存在任何能改变本现象的固件改动**。

### 3.4 已知答案校核（本轮新增的独立验证手段）

用**未改动的交付源码** `USER/src/fw_core.c` 编译最小 KAT，与手册自带示例比对：`CRC(0xBEEF)=0x92` **MATCH**；换算点 `raw_t=0x6666,raw_h=0x8000 → 25.0 ℃ / 56.5 %RH`（手算 `-45+175×26214/65536 = 25.0`、`-6+125×32768/65536 = 56.5`）一致；越域样本 `0x0000`（T=-45.0 < -40.0）、`0xFFFF`（T=129.99 > 125.0、RH=119 > 100）**均判为无效** ⇒ 有效域门禁真实生效。原始输出：`evidence/ev014/datasheet_kat.txt`。

### 3.5 只读静态复核（不属产品改动）

- 采样路径唯一命令为 `0xFD`；`GXHT40_CMD_SOFT_RESET`(0x94) 宏**零调用**（`grep` 全 `USER/src` 无引用）；无加热命令。
- 总线恢复 `i2c_bus_recover()`：先释放两线，仅当 SDA 仍被拉低才补 **≤9 个 SCL 脉冲**并补一个**合法 STOP**，有界、不发地址、不发读、不写命令。
- 失败后 `s_addr7 = 0`（缓存地址失效）、下一周期**先做总线恢复再依次重探 `0x44`/`0x45`**（FWR-118）。

## 4. 独立判定（本能力自建资产，不引用实现自述）

| 运行 | 对象 | 结果 |
|---|---|---|
| 实板判定（本轮） | `evidence/ev014/{f1,g1,g2,h1,h2,h3}.cap` | **123 项通过 / 1 项失败**（唯一失败 = 核心验收 `H ACCEPT`：最长连续 `q=1` = 0）→ 原文 `evidence/ev014/verify_run.txt`，退出码 1 |
| **定向变异自检** | 合成捕获（正确签名、SCL 错、数据位错、ACK 时隙混合、ACK/无 ACK 与 BUS 双向矛盾、`IOSIG` 位置错、缺失、复位切断行、缺 CRLF 类缺陷、非前缀乱码切断） | **12/12 与预期一致**（exit 0）→ `evidence/ev014/verify_selftest.txt` ⇒ L 组**既能通过也能失败**，非恒过的空验 |
| 宿主 harness 复跑（6 份，本轮全部对**当前工作树**重跑） | `USER/src/*.c` | **85 / 93 / 51 / 36 / 79 / 79 / 56 通过，0 失败** → 原文 `evidence/ev014/harness_results.txt` |
| 手册已知答案 KAT | 交付 `fw_core.c` | CRC 与换算**逐点一致**（§3.4） |

## 5. 竞争假设状态（本轮更新）

| 假设 | 状态 | 依据 |
|---|---|---|
| H1 器件贴装/焊接不良 | **剩余候选（现场）** | 用户已重焊，现象**逐字节未变** ⇒ 若重焊到位则本项权重下降，但本能力无法目视验证焊点，故仍开放 |
| H2 器件损坏 / 装错件/错型号 | **剩余候选（现场）** | 112 地址全域不应答；手册确认全系列仅 0x44/0x45 ⇒ 型号/地址变体已排除，剩「损坏/非本系列件」 |
| H3 器件异常闩锁（需真冷启动） | **剩余候选（现场）；本轮权重上升** | 手册 §7.7 复核后确认器件**只能靠断电**复位，且**同 BAT 网、无开关、电池焊接** ⇒ 若重焊时器件未真正断电（仅拖焊），闩锁状态可一直保留 |
| H4 SDA/SCL 接反 | **排除** | `IOTEST swap=none` ×4 + 网表独立重解析（§3.1） |
| H5 主机拉不低 / 拉不高 | **排除** | `IOTEST sda_lo=0 scl_lo=0 idle=3` ×4 |
| H12 事务内位/时钟未送达 | **排除** | `IOSIG scl=95555 sda=30303` ×2 |
| 上拉缺失/总线卡低 | **排除** | `idle=3`；恢复原语有界（≤9 脉冲 + STOP） |
| 地址变体 | **排除（本轮扩到全系列）** | 手册确认全系列仅 0x44/0x45；`BUS` 扫 112 地址 `ack=none` |
| 读未就绪 / CRC / 量程 / 重试策略 | **排除** | 失败在地址探测阶段之前（`rd=0 at=3`）；手册时序/换算逐条对齐（§3.2）；宿主 harness 全绿 |
| 固件可修改即能恢复 | **排除（本轮新证）** | §3.3-2：器件不应答 ⇒ 任何命令（含 0x94 / general call）都无法送达 |

## 6. 本轮未覆盖项与证据缺口（如实登记）

1. **「主机能否正确判定 ACK」在本板上仍缺正向对照**：本板 I²C 总线上除 U9 外无第二个从机（§3.1 网表），没有「真器件应答、主机读到 0」的实板样本。覆盖仍靠三层间接证据：bit 级 harness（mock 从机在正确时隙被采到 ACK）、`IOSIG` 在真实总线上回读到 ACK 时隙的**实际电平**、§2.3 的采样点时序算术。
2. **实板 `raw` 十六进制形态**仍只有宿主覆盖（实板 `rd=0`，从未读回帧）——器件不响应的直接后果。
3. **general call 地址 `0x00` 未被探测**（`BUS` 只扫 0x08..0x77 合法 7bit 地址）。登记为未测项：它**无区分价值**——不应答自身地址的器件不可能被任何命令唤醒（§3.3-2），且本项验收不涉及 general call。
4. **无逻辑分析仪/示波器/万用表/可控电源**：这些**不是**本项阻塞理由（阻塞来自「器件不应答 + 必需现场动作」）；亦**未**以任何未提供仪器作为门槛。
5. **`LIGHT_DARK_CALIBRATED=0`**：全暗基准仍未标定。本轮光照读数（`a` 80–117，有光）**不作、也不能作**全暗标定值；本项 `V=0 D=0` 说明未标定门禁按设计生效。
6. **手机端读数一致性未实测**：`r=0`（永不满足上报条件）⇒「满足上报条件时手机端与该周期 `S` 行 `t/h` 为同一笔读数」这一子句**在本轮构造上无法被触发**，故未调用 `android_test`（不虚构未发生的上报）。
7. **登记偏差 D-01**（上报方向仍为降温 `(prev-cur)>9`，readme 要求升温）在本轮构建上**仍然存在**（只读复核 `fw_core.c:121`），属固件任务 T4 范围，**不属本项**验收。
8. **文档同步**（非本能力职责）：`IOTEST`/`IOSIG` 两行与实际上电顺序 `BOOT→IOTEST→IOSIG→BUS` 仍未收录进 TD-002/FD-002 正文（EV-012/EV-013 已登记，本轮再次确认 `artifacts/{test_design,firmware_design,firmware_requirements}.md` 中 `IOSIG` 出现次数为 0）。

## 7. 判定与交接

**结论：BLOCKED。** 用户已完成 EV-013 §7 第 1 项所指的现场动作（对 U9 重新焊接），但**重焊后的实板可观察量与重焊前逐字节相同**（§2.1 对照）；本轮另以用户新提供的手册与网表把「主机侧 + 协议/换算/电气余量」逐条对齐（§3），并**新证实**「器件无复位脚、只能断电复位」与「不应答即收不到任何命令」两条事实（§3.3），因此**任何固件改动都无法改变本现象**。剩余路径只有现场物理动作，现有资源无法替代。

判定为 **BLOCKED** 而非再判 TEST_FAIL 的依据：按能力规则「无 ACK …首先是 TEST_FAIL」的前提是**尚未定位失败环节**；本轮该前提已不成立——失败环节已用实板一手数据逐条收敛到**器件/接线侧**，且 operator 明确「软件缺陷才 TEST_FAIL 反馈实现能力、不为重做而盲改代码」。把本项再判 TEST_FAIL 会向实现能力发出「存在可修软件缺陷」的错误信号（§3.3-2 证明不存在），故改为如实登记外部条件与恢复条件。**未**以任何未提供仪器作为门槛（§6.4）。

**交用户（最小必要现场动作，按顺序；任一步即可分支处置）**

1. **真断电复上电（本能力认为现在最该先做、且是本轮结论的关键缺口）**：让 **U9 真正失去供电** ≥5 s 再恢复。注意本板 **CR2032 是焊接的、且 `BAT` 网无任何开关**，`mdk_flash` 的复位只复位 MCU、**不给 U9 断电**（§3.1、§3.3-1）⇒ 需**断开 `BAT` 网本身**（拆焊电池某一脚，或断开该网）。若重焊时 U9 只是被"拖焊"而**从未真正断电**，此步可清除闩锁态。
2. **电气核查 U9 四处焊点**：`U9.1→U7.14(PA04)`、`U9.2→U7.13(PA03)`、`U9.3→BAT`、`U9.4→GND`（通断/阻值）。若手头无万用表，可在第 1 步之后直接做第 3 步。
3. **换件对照**：换一只已知良好的 GXHT40（或 `GXHT40-BD`，地址 `0x45` 同样被扫描），复测。

**恢复条件（一眼可判）**：`IOSIG` 的 ACK 时隙由 `11` 变为 `00`（即 `sda=30300`），或 `BUS ack` 由 `none` 变为含 `44`/`45` —— 即表示器件已能应答。**恢复后无需任何固件改动**：`S` 行的 `q` 应在该周期内转为 `1` 且 `t/h` 落在 `-40.0..125.0 ℃`/`0..100.0 %RH`。复测只需**重新上电 + COM42 采集**，用本轮同一脚本 `evidence/ev014/seq.sh`（约 16 分钟给出「`q=1` 是否出现 + 连续 6 周期」的判定）。

**交实现能力**：本项**无待修项**。本轮以 6 份宿主 harness 在当前工作树复跑（全绿）+ 手册已知答案 KAT（CRC/换算精确一致）+ 静态复核（仅 `0xFD`、`0x94` 零调用）**重新确认**交付件符合性未被推翻；§3.3-2 从器件手册层面证明**不存在**能让本器件恢复应答的固件改动。请**不要**在没有新证据的情况下改代码。

**交设计能力**：§6.8 的 TD-002/FD-002 行集合与顺序同步（延续 EV-012/EV-013 的登记）。

**已交付的端口**：`evidence/programmed_prototype.md`（本轮下载身份与回读校验：2 次 `Verify OK`、axf md5 与交付件一致）；本文件 `evidence/test.md`（TestEvidence，结论 BLOCKED）。

---

# 历史记录（EV-013 / run8 ITEM-002；当轮判 BLOCKED：实板 0/7 周期 q=1、新增 `IOSIG` 实板读数排除 H12、剩余候选全在器件/接线侧须用户现场动作；已提交 58978b8）

# 嵌入式验证证据（EV-013 / run8 ITEM-002 ↔ GXHT40 温湿度采集在真实目标上恢复有效）

状态：embedded_tester.embedded_verification 独立验证证据（本轮重新构建、重新下载、重新采集真实目标字节流；不沿用历史结论）
本轮验证对象：run8 队列项 **ITEM-002**（GXHT40 温湿度采集在真实目标上恢复有效：实板每个 3 分钟采样周期 `S` 行 `q=1`、`t/h` 落在有效域且与环境相符、连续多个周期稳定；满足上报条件时手机端读数与该周期 `S` 行 `t/h` 为同一笔）
本轮增量：实现能力新增上电 **`IOSIG`** 行（EV-012 §5 交回的 **E1b** 事务内逐位回读签名，用于判别 H12）

- 受测提交：`ef03c13`（接手时 `git status` 干净；`HEAD` 即实现能力本轮的 `firmware_implementation` 提交；本轮**未改产品源码**）
- 被测件：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf`，md5 **`5c2fdb58d0fc4ef7dac8a21e343082a3`**（由受测提交源码经 `mdk_build rebuild` 重建；与实现能力 `evidence/build.md` 声明的 axf md5 **逐字符一致**、`Code=18888 RO-data=644 RW-data=116 ZI-data=1676` **逐项一致** ⇒ 本轮实测的二进制就是本轮实现交付的那一个）
- 验证基线：TD-002 rev 5.0 `artifacts/test_design.md`；FWR-002 v5.0 `artifacts/firmware_requirements.md`（FWR-116/117/118）；FD-002 rev 5.0 `artifacts/firmware_design.md` §6.6；`sensor_hardware/pstxnet.dat` + `MAIN_BOARD.BOM`（本轮逐网复核供电与 I²C 网络）；`readme.txt`（字面阈值）

结论：**BLOCKED** —— 本项核心可观察在真实目标上**仍未达成**（2 次成功下载、4 个采集窗口跨 20 分钟、**7 个采样周期全部 `q=0`**、2 次独立复位、全地址扫描 `ack=none`），而本能力用本轮新交付的判别观测量把**主机侧最后一条残余假设 H12 也在实板上排除**：所有失败环节现在都落在**器件/接线侧**，且**必须用户现场动作**（目视/通断/换件；真断电复上电需断开 BAT 网）才能继续，现有资源**无法替代**（本板 I²C 总线上除 U9 外无第二个从机可作正向对照，且无万用表/示波器/可控电源开关）。

本轮**实质性进展**（新证据，非重复；EV-012 的 H12 不再存在）：

1. 实板 **`IOSIG scl=95555 sda=30303`**，2 次完整读数**逐字节相同**（另有 1 条被烧录 halt 从中途切断、其字节恰为该完整行的前缀，按「复位切断」记 note，非缺陷）。该签名由本能力**独立重建**（`evidence/embedded_verify_gxht40_item002.py` 内 `iosig_expected()`，不引用实现常量）：20 个半位采样点中 SCL 形态必须是 `95555`（START 建立 → SCL 低 → 8 位数据各「低+高」→ ACK 时隙低+高），SDA 的 16 个数据位采样点必须等于被驱动的地址字节 `0x88` 的位模式，其后 2 位为 ACK 时隙。实板读数**逐位相符** ⇒ **事务内的位与时钟形态确实送达引脚并被如实回读** ⇒ **H12 排除**。
2. ACK 时隙两位实板读为 `11`（高、高）⇒ 地址 `0x44` 上**无器件拉低 SDA**；并与**同一次复位段**的 `BUS … ack=none`（112 个地址全域）**交叉一致**（脚本 L 组硬门禁）。
3. 设备侧排除项已达本能力工具边界：主机能拉低两线并回读（EV-012 `IOTEST`，本轮 4 次读数逐字节相同）、未接反（`swap=none`）、上拉有效（`idle=3`）、地址字节与时钟形态真实成立（本轮 `IOSIG`）、112 个合法 7bit 地址全域无 ACK（`BUS ack=none`，沿用 EV-012/本轮一致结论）。

判定为 **BLOCKED** 而非再判 TEST_FAIL 的依据（同能力规则）：**已证实**必需现场动作是唯一剩余路径——本项验收要求「器件本身应答」，而一切可由固件/工具侧改变的因素都已被实板数据逐条排除；实现能力按现有证据**已无可改之处**（无任何一条剩余假设可通过改代码验证），故不再把本项交回实现，改为如实登记外部条件并给出恢复条件（§7）。**未**以任何未提供仪器作为门槛（无逻辑分析仪/示波器不构成阻塞理由，见 §6）。

| # | 事实 | 性质 | 依据 |
|---|---|---|---|
| 1 | 实板 **0/7 个采样周期 `q=1`**；核心可观察（`q=1`、`t/h` 有效域、连续多周期稳定）**未达成** | 功能不符（本项核心验收不通过） | §2.2 |
| 2 | reset 之后 `k=0,3,6,9,12` **五个周期无间隙连续**（≈15 分钟、3 个采集窗口首尾相接），器件**未自行恢复**、无任何成功周期 | 失败稳定、非偶发；排除「偶发/需等待自愈」 | §2.2 |
| 3 | 7 条失败周期 `G` 行**逐字节相同**：`G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`；2 条 `BUS` 行**逐字节相同**：`BUS idle=3 scl=1 sda=1 ack=none` | 失败形态稳定可复现（2 次独立复位、两种复位来源 `rst=0040`/`rst=0240`） | §2.1、§2.2 |
| 4 | **`IOSIG` 实板读数 2/2 逐字节相同：`IOSIG scl=95555 sda=30303`** ⇒ 事务内的 SCL 时钟形态与地址字节 8 个数据位**逐位**如实送达并被回读；ACK 时隙两位读高 ⇒ 该地址无器件应答 | 本轮新增的判别事实（**排除 H12**） | §2.3 |
| 5 | `IOSIG` 的 ACK 时隙结论与**同一次复位段**的 `BUS ack=none` 交叉一致（同一对引脚、同一器件、同一次上电内两次独立观测互不矛盾） | 自洽性硬门禁（脚本 L 组） | §4.1 |
| 6 | 同窗口内光照通路持续工作且随环境变化：`o=8` 恒成立；`c1` 内相邻两周期的原始码 `n/x/a` 由 `3628/3634/3631` 跳到 `143/145/143`（min≈max≈mean，8 次转换离散度 ≤6 码） | 交叉对照：MCU/时钟/ADC/GPIO/UART1 通路正常，且读数随真实光环境变化 | §2.4 |
| 7 | 失败周期**不上报、不推进前一有效温度、不构造 0 值上报**：`p=0 H=0 V=0 D=0 q=0 r=0 s=0 y=0`、`raw=------------`（无效帧） | 冻结不变量在实板成立 | §2.2 |
| 8 | 交付源码只写**唯一**命令 `0xFD`（高重复率测量）；`0x94` 软复位宏**零调用**；无加热命令 | 只读复核（不属产品改动） | §4.2 |

---

## 1. 环境、构建与部署（本轮实际调用，原始结果）

| 工具 / 命令 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `COM42 = Prolific USB-to-Serial Comm Port (VID:PID 067B:2303)`；`COM26 = USB 串行设备 (C251:F001, SN 87094109484987710672FF50)` | 传感器调试口 **COM42 在位**，与配置声明的设备身份一致 |
| `usb_list_devices` | `USB\VID_C251&PID_F001`（HID MI_02 + Ports MI_00 = COM26，SN `87094109484987710672FF50`，Status OK） | **CW-DAPLink 下载探针在位** |
| `mdk_build rebuild` | `*** Using Compiler 'V6.24'`；`../USER/src/main.c(238): warning: while loop has empty body`；`Program Size: Code=18888 RO-data=644 RW-data=116 ZI-data=1676`；`0 Error(s), 1 Warning(s)` | 0 错误；1 条为**基线既有**空体 `while(k--);`（EV-012 时在 `231`，行号随新增代码位移；实现 `evidence/build.md` 本轮报 `238`，**一致**） |
| `md5sum …/Project.axf` | `5c2fdb58d0fc4ef7dac8a21e343082a3`（`Project.hex` = `db3dbc72195d346815500b8ef158b90f`） | 与实现交付的 axf md5 **完全一致** ⇒ 受测件身份闭合 |
| `mdk_flash {}` ×2（本地 18:50:21 / 18:55:22） | 每次均 `Erase Done.Programming Done.Verify OK.Application running ...`；下载前 build 为 `0 Error(s), 0 Warning(s)`（无可重建项 ⇒ 所下即上面那个 axf） | 当前交付镜像已下载并**回读校验成功**。按能力约定：`Verify OK` **仅**证明下载校验，业务功能仍须实测（§2） |
| `serial_capture` ×4（COM42@9600，300 s/次，首 2 次与 `mdk_flash` 串接于同一 shell 脚本，使复位横幅落在窗口内） | `evidence/ev012` 之外新建 `evidence/ev013/{b1,b2,c1,c2}.cap`（482/506/233/116 B，逐字节账目闭合） | 原始字节已入库；采集窗口与 md5 见 §2.5 |
| `git status --short`（提交前） | 空（除本能力声明的证据/清单文件外） | 所编/所下的源码即受测提交 |

授权与入口：本轮 context 的 `当前能力有效工具` 与 `本次允许调用的工具及参数` 均非空（9 个工具），实际调用 5 个（`serial_list_ports`、`usb_list_devices`、`mdk_build`、`mdk_flash`、`serial_capture`），未用 `install_system_package`/`install_python_package`/`android_test`（本项不涉及 Android 侧，且无新增依赖需求）。`mdk_build` 走 MCP 入口；`serial_capture`/`mdk_flash` 走 `tool_cli.py` CLI 入口（`evidence/ev013/run.sh`、`chain.sh`，使用**本轮** context 路径），两者共用同一授权与证据记录。

## 2. 真实目标 UART1 观测（COM42 @9600 8N1，原始字节）

采集方式：`serial_capture`（单次上限 300 s，采样周期 ≈180 s）。为让复位横幅落在窗口内，把 `mdk_flash` 与采集**串接在同一个 shell 脚本**中（`evidence/ev013/run.sh`，flash 延迟 4 s）；随后用 `evidence/ev013/chain.sh` 续采两个窗口。原始捕获入库为 `evidence/ev013/*.cap`（`.cap` 而非 `.bin`：仓库 `.gitignore` 含 `*.bin`，EV-012 的 `.bin` 原始捕获因此**从未真正进入 Git**，本轮已改用 `.cap` 并把 EV-012 的 5 份原件按字节回填为 `evidence/ev012/*.cap`，md5 逐份核对一致——见 §6.3）。

### 2.1 上电行（BOOT / IOTEST / IOSIG / BUS）

| 捕获 | 复位段 | 上电行（逐字节原文） |
|---|---|---|
| b1 | reset#1（窗口起点，`rst=0040`） | `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` + `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none`（随后被下载复位切断，无 BUS） |
| b1 | reset#2（`rst=0240`，下载复位 10:50:21） | `IOTEST …` + **`IOSIG scl=95555 sda=30303`** + `BUS idle=3 scl=1 sda=1 ack=none` |
| b2 | reset#1（窗口起点，`rst=0040`） | `BOOT …` + `IOTEST …` + `IOSIG scl=95555 sda=3030`（**被下载复位从行中切断**，其后紧接下一条 `BOOT`） |
| b2 | reset#2（`rst=0240`，下载复位 10:55:22） | `IOTEST …` + **`IOSIG scl=95555 sda=30303`** + `BUS idle=3 scl=1 sda=1 ack=none` |

- 上电顺序实测 `BOOT → IOTEST → IOSIG → BUS`，四行紧邻（脚本 L 组按相邻性硬门禁）。
- `IOTEST` 本轮 4 次读数**逐字节相同**（EV-012 结论在**新构建**上复现）：主机确实能把 PA04(SDA)/PA03(SCL) 拉低并回读为低、释放后两线回高（`idle=3`）、未发现角色接反（`swap=none`）。
- `BUS` 本轮 2 次读数逐字节相同，`ack=none`：0x08..0x77 共 112 个合法 7bit 地址**无一应答**。

### 2.2 采样周期（`S` 行与紧随的 `G` 行）

| 捕获 | 窗口（UTC） | 字节 | 周期 | `q` | `S` 行关键字段 | 紧随 `G` 行 |
|---|---|---|---|---|---|---|
| b1 | 10:50:08.344 → 10:55:08.569（内含下载复位 10:50:21） | 482 | k=0, k=3 | 0, 0 | `o=8 n=3592 x=3599 a=3597` / `o=8 n=3586 x=3592 a=3589` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` ×2 |
| b2 | 10:55:09.007 → 11:00:09.235（内含下载复位 10:55:22） | 506 | k=0, k=3 | 0, 0 | `o=8 n=3583 x=3588 a=3586` / `o=8 n=3599 x=3606 a=3603` | 同上 ×2 |
| c1 | 11:00:09.655 → 11:05:09.875（无复位） | 233 | k=6, k=9 | 0, 0 | `o=8 n=3628 x=3634 a=3631` / `o=8 n=143 x=145 a=143` | 同上 ×2 |
| c2 | 11:05:10.290 → 11:10:10.504（无复位） | 116 | k=12 | 0 | `o=8 n=155 x=157 a=156` | 同上 ×1 |

- **7/7 周期 `q=0`**；`t=0 h=0 p=0 H=0 V=0 D=0 r=0 s=0 y=0`（无有效温湿度、不推进前值、不置上报）。
- 以 10:55:22 的下载复位为锚，`k=0,3,6,9,12` 构成**无间隙连续栅格**（≈15 分钟），期间器件无任何成功周期、无自愈。
- 7 条 `G` 行**逐字节相同**，与 `BUS ack=none`、`a44=a45=0`、`rd=0`、`at=3` 逐项自洽。

### 2.3 本轮新增：`IOSIG` 事务内逐位回读签名（E1b）与独立判读

格式（实现 `debug_trace.c`，行首标签 + 两个 5 位大写 hex，共 20 bit）：`IOSIG scl=<5 位大写 HEX> sda=<5 位大写 HEX>`

本能力**独立重建**期望值（`iosig_expected()`，不引用实现常量；采样点定义按 TD/实现文档化的 20 个半位）：

| 采样点 | 语义 | 期望 SCL | 期望 SDA（无器件） |
|---|---|---|---|
| S0 | START 建立（SCL 已高，SDA 刚被主机拉低） | 1 | 0 |
| S1 | SCL 拉低 | 0 | 0 |
| S2..S17 | 地址字节 `0x88` 的 8 位（MSB 先），每位「SCL 低」+「SCL 高」各一点 | `0101…01`（16 位） | 每一位重复两次 |
| S18 | ACK 时隙，主机已释放 SDA，SCL 仍低 | 0 | 1（无器件，被上拉）/ 0（器件 ACK） |
| S19 | ACK 时隙，SCL 抬高 | 1 | 同上 |

⇒ 推导值 **`scl=0x95555`**、**`sda=0x30303`**（无器件）/ **`sda=0x30300`**（器件在该地址 ACK）。

| 实板读数 | 判读 |
|---|---|
| `scl=95555`（2/2 一致） | SCL 的 START/8 位时钟/ACK 时隙**逐位**与主机驱动形态一致 ⇒ 时钟形态真实送达且被如实回读 ⇒ **排除 H12 的时钟分支** |
| `sda` 的数据位（bit17..bit2）= `0x0C0C0`，与 `0x88` 逐位相符（2/2 一致） | 地址字节的 8 个数据位真实送达 ⇒ **排除 H12 的数据位分支** |
| ACK 时隙两位 = `11`（2/2 一致） | 地址 `0x44` 上无器件拉低 SDA；与 `BUS ack=none`、`G a44=a45=0` **交叉一致** |
| b2 reset#1 的 `IOSIG scl=95555 sda=3030` | 被下载 halt **从行中切断**；其 24 字节恰为完整行的前缀（脚本判为「复位切断」note，非行终止缺陷；对照 EV-006 的 D-ITEM001-1 类缺陷不会被本规则放过，见 §4.1 自检用例） |

### 2.4 交叉对照（同一批观测内的其它通路）

`o=8` 恒成立（每周期 8 次 AIN11 转换全部成功）、`n≤a≤x`（min ≤ mean ≤ max）逐周期成立；`c1` 内相邻两周期的 `a` 由 `3631` 跳到 `143`（min/max 与该均值相差 ≤6 码），符合「环境光被打开」的真实物理变化 ⇒ MCU、RTC 节拍、ADC、GPIO 供电窗口（PB05）、UART1 通路均正常，故障被进一步限定在 **U9 及其 I²C 连接**上。

### 2.5 原始证据清单（入库）

| 文件 | 字节 | md5 | 窗口（UTC） |
|---|---|---|---|
| `evidence/ev013/b1.cap` | 482 | `e19e539b21a75737882960ed25fb1086` | 10:50:08.344 → 10:55:08.569 |
| `evidence/ev013/b2.cap` | 506 | `d33bb142da414e355091bf957f358f16` | 10:55:09.007 → 11:00:09.235 |
| `evidence/ev013/c1.cap` | 233 | `f5ced0e75cab83bd98bc394374f2a0af` | 11:00:09.655 → 11:05:09.875 |
| `evidence/ev013/c2.cap` | 116 | `6e7c59f935ab4a340fe7c8fb62686901` | 11:05:10.290 → 11:10:10.504 |

字节账目（含 CRLF，逐字节闭合、无未知字节）：b1 = 50+43+50+43+27+33+72+46+72+46 = **482**；b2 = 50+43+**24**（被切断的 IOSIG 前缀）+50+43+27+33+72+46+72+46 = **506**；c1 = 72+46+69+46 = **233**；c2 = 70+46 = **116**。下载日志：`evidence/ev013/mdk_flash_logs.txt`；窗口清单：`evidence/ev013/capture_windows.txt`。

## 3. 失败环节定位（本能力独立复核）

| 候选环节 | 实板证据 | 判定 |
|---|---|---|
| 主机拉不低 / 拉不高（H5） | `IOTEST sda_lo=0 scl_lo=0 idle=3`（4/4） | **排除** |
| 板级 SDA/SCL 接反（H4） | `IOTEST swap=none`；网表逐网复核 `SDA=U7.PA04`、`SCL=U7.PA03` | **排除** |
| 上拉缺失 / 总线被卡低 | `idle=3`（两线释放后回高）；总线恢复原语在最坏情形下也仅产生 9 个 SCL 脉冲 + 1 个 STOP（宿主 harness 51/51） | **排除** |
| 事务内位/时钟未真实送达（H12） | `IOSIG scl=95555 sda=30303`（2/2），与主机驱动形态逐位相符 | **排除（本轮新增）** |
| ACK 采样点过早导致漏判 ACK | `IOSIG` 在 ACK 时隙的**两个**半位点（S18 = 第 8 个 SCL 下降沿后 + 半位延时、S19 = 第 9 个时钟高相）各读一次；100 kHz 下半位延时 ≈5 µs，而从机器件在下降沿后 ≤0.9 µs 内驱动 SDA 并保持到第 9 个时钟高相结束 ⇒ 两个采样点都落在 ACK 有效窗内且有余量 | **排除**（时序算术；非仪器结论） |
| 地址变体（器件在别的地址） | `BUS` 扫描 0x08..0x77 共 **112** 个合法 7bit 地址（含 0x44/0x45/0x46 家族）`ack=none` | **排除** |
| 器件未被供电 | 网表：`BAT` 网 = `U7.9 (MCU VDD)` + **`U9.3 (GXHT40 VCC)`** + 上拉电阻 R1.1/R2.2 + `U4.6` + 电池座 `J1.1` + `J2.1`（SWD 排针电源脚）——**同一网、无稳压器、无固件可控门**；既然 MCU 在运行、上拉有效（`idle=3`），该网即为带电 | **不能排除**：若 `U9.3`/`U9.4` 焊点开路仍会呈现本现象（器件侧/装配侧） |
| 器件损坏 / 装错件 / 异常闩锁 | 112 地址全域不应答、跨 20 分钟与 2 次下载复位稳定复现 | **剩余候选（现场）** |
| 固件策略（重试/CRC/量程/重探/恢复） | 宿主独立 harness 全部通过（§4.2）；且失败发生在**地址探测阶段之前**（`rd=0`），策略分支尚未进入 | **排除本项** |

**关键的供电/复位事实**（本轮逐网复核，一手 `pstxnet.dat`）：U9 的 VCC 与 MCU 的 VDD 在**同一 BAT 网**且**无固件可控开关**。因此 `mdk_flash` 的复位（`rst=0240`，PIN|SYSRESETREQ）只复位 MCU，**不会给 U9 断电**；要给 U9 做真正的断电复上电，必须**断开 BAT 网（例如取下/拆焊电池或断开该网）**，属现场动作。器件若处于异常闩锁态，现有任何固件手段都无法解除（周期路径禁用软复位 `0x94`，且器件此刻根本不回应命令）。

**I²C 网络拓扑**（同文件）：`N05845` = R1.2 + **U9.1 (SDA)** + U7.PA04；`N06024` = R2.1 + **U9.2 (SCL)** + U7.PA03 ⇒ 这两条网上**除 U9 外无第二个从机**。

## 4. 独立判定（本能力自建资产，不引用实现自述）

### 4.1 实板字节流独立解析脚本 `evidence/embedded_verify_gxht40_item002.py`

本轮为其新增 **L/M 组**（接受并判定 `IOSIG` 行），并同步 A 组的行终止判据。判据全部由脚本**独立重建**（`iosig_expected()` 按采样点定义推导，不引用实现常量/函数）。

| 运行 | 对象 | 结果 |
|---|---|---|
| 实板判定（本轮） | `evidence/ev013/{b1,b2,c1,c2}.cap` | **90 项通过 / 1 项失败**（唯一失败 = 核心验收 `H ACCEPT`：最长连续 `q=1` = 0）→ 原文 `evidence/ev013/verify_run.txt`，退出码 1 |
| **定向变异自检** | 合成捕获（含正确签名、SCL 错、数据位错、ACK 时隙混合、ACK/无 ACK 与 BUS 双向矛盾、`IOSIG` 位置错、`IOSIG` 缺失、复位切断行、缺 CRLF 类缺陷、非前缀乱码切断） | **12/12 与预期一致**（exit 0）→ `evidence/ev013/verify_selftest.txt`。**证明 L 组既能通过也能失败**（非恒过的空验），且 EV-006 的 D-ITEM001-1（CRLF 不发）类缺陷仍会被判失败 |
| **负对照：上一构建** | `evidence/ev012/*.cap`（`09408c1` 镜像，**无 `IOSIG`**） | **100 项通过 / 5 项失败**：新增的 4 项失败恰为「本窗口无 `IOSIG`/无读数值」⇒ L/M 组非空验 → `evidence/ev013/verify_run_prev_build.txt` |

L 组本轮在实板上实际通过的硬门禁（2 次复位各一组）：`IOSIG` 在完整上电打印段内**恰 1 条**且 `BOOT<IOTEST<IOSIG<BUS` 四行**相邻**；字段域为 5 位大写 hex；`scl` 等于独立推导的时钟形态 `95555`；`sda` 的 16 个数据位等于被驱动地址位；ACK 时隙两位自洽（`00` 器件应答 / `11` 无器件，不接受混合）；`sda` 取值为 `30303` 或 `30300` 之一；**与同复位段 `BUS ack` 列表交叉一致**。

### 4.2 交付源码的独立宿主复跑（本能力自有 harness，本轮全部对当前交付源码重跑）

原始命令与末尾输出：`evidence/ev013/harness_results.txt`。**仅被测对象来自交付固件**（`USER/src/*.c`），harness 与阴影层均为本能力自有资产。

| harness | 覆盖 | 结果 |
|---|---|---|
| `host_diag_probe_verify_ev.c` | 真实 `measure.c`+`gxht40.c`+`sf_i2c.c`+`fw_core.c`：`i2c_probe_addr` 线上字节、112 地址扫描（无命令/无读/无业务态改动）、8 地址截断、诊断快照 5 类结果码、失败周期传播、**S6 SDA 卡低负对照** | **85/85** |
| `host_gxht40_verify_ev.c` | 位级 mock + 真实驱动：地址探测/缓存、`0xFD` 白名单、**`0x94` 全程从未出现**、6 字节帧顺序、双字 CRC-8、换算/有效域、有界重读重测、失败后重探两候选 + `--wrap` 证明恢复先于重探、失败不改输出 | **93/93** |
| `host_sf_i2c_verify_ev.c` | 位级总线：命令写/连续读线上字节、ACK/NACK 位置、`i2c_bus_recover` 最坏 9 脉冲 + 1 STOP、无地址字节/无读 | **51/51** |
| `host_diag_line_verify_ev.c` | 真实 `debug_trace.c`：BUS/G/S 最坏行长与 96 B 预算、CRLF、关闭语义 | **36/36** |
| `host_measure_flow_verify_ev.c` | 真实 `measure.c` 流程：逐周期状态、失败不推进前值/不构造 0 值/不清待上报、光照先于温湿度、休眠与节拍门控（`SENSOR_DEBUG_UART=0` 与 `=1` **各跑一次**，后者编译进本轮新增的 `sensor_io_sig_scan` 路径） | **79/79（两种编译）×2** |
| `host_fw_core_verify_ev.c` | 纯逻辑；**本轮修正其自 T2 起的陈旧断言**（原引用已删除的 `light_code_is_dark`，harness 自 T2 起无法编译）：T7 改为按 readme 点 7 的 1/3 判据、以**独立写出的 ceil 除法形式**（`mean >= (c_dark+2)/3`，与被测的 `3*mean >= c_dark` 属不同表述）逐例比对 `mean×valid×c_dark` 采样网格；T8/T8b 明确标注为 **as-built 降温方向（登记偏差 D-01）** | **56/56**，其中 1,253,376 个用例 0 不符 |

**未使用且已登记**：`test/host_light_verify_ev.c` 仍按 T2 之前的 `bool light_sample()` API 编写（现返回 `light_result_t`），自 T2 起**无法编译**（本次实测：`error: incompatible types when initializing type '_Bool' using type 'light_result_t'`）。本轮**未使用**它作任何结论；`light.c` 的判据部分已由上面 fw_core harness 的 T7（穷举）覆盖，驱动时序部分留待 ITEM-004/T4 相关轮次重写（§6.2）。

### 4.3 只读静态复核（不属产品改动）

- 采样路径唯一命令为 `0xFD`；`0x94` 软复位宏零调用；无加热命令（`host_gxht40_verify_ev` T16 全程断言）。
- 失败周期不上报/不推进前值/不构造 0 值由实板 `S`/`G` 行（§2.2）与 `host_measure_flow_verify_ev` 双路覆盖。

## 5. 竞争假设状态（本轮更新）

| 假设 | 状态 | 依据 |
|---|---|---|
| H1 器件贴装/焊接不良 | **剩余候选（现场）** | 一切主机侧因素已排除（§3） |
| H2 器件损坏 / 装错件 | **剩余候选（现场）** | 同上；112 地址全域不应答 |
| H3 器件异常闩锁 | **剩余候选（现场）** | 需真断电复上电；U9 与 MCU 同网、无固件可控门（§3） |
| H4 SDA/SCL 接反 | 排除 | `IOTEST swap=none` + 网表复核 |
| H5 主机拉不低 | 排除 | `IOTEST sda_lo=0 scl_lo=0` |
| H12 事务内位/时钟未送达 | **排除（本轮）** | `IOSIG scl=95555 sda=30303` |
| 地址变体 / 读未就绪 / CRC / 量程 / 总线卡死 / 缓存失联 | 排除 | `BUS ack=none`、`G rd=0 at=3`、宿主 harness |

## 6. 本轮未覆盖项与证据缺口（如实登记）

### 6.1 残余假设与限制
- **「主机能否正确判定 ACK」在本板上缺正向对照**：本板 I²C 总线上除 U9 外无第二个从机（§3 网表），因此没有「一个真器件应答、主机读到 0」的实板样本。该环节由三层间接证据覆盖：① bit 级 harness（mock 从机应答时主机在正确时隙采样 ACK）；② `IOSIG` 在真实总线上回读到 ACK 时隙的**实际电平**；③ §3 的采样点时序算术。若现场查出 U9 良好且焊接完好，此条应作为下一步复核项重新提出。
- 实板 `raw` 十六进制形态（`raw=ABCDEF012345` 类）仍只有宿主覆盖（实板 `rd=0`，从未读回帧）——与 EV-012 相同，属器件不响应的直接后果。
- 无逻辑分析仪/示波器/万用表/可控电源：这些**不是**本项阻塞理由（本项阻塞来自器件不响应 + 必需现场动作）；它们只影响「电气级证据」的粒度。
- `LIGHT_DARK_CALIBRATED=0`（全暗基准仍未标定）：与本项无因果；本轮 `V=0 D=0` 说明未标定门禁按设计生效（未把亮态读数当真值）。

### 6.2 测试资产维护（本能力自有）
- 已修：`test/host_fw_core_verify_ev.c`（陈旧断言 → 1/3 判据，见 §4.2）。
- 待重写：`test/host_light_verify_ev.c`（T2 前的 API/滞回语义；登记交 ITEM-004/T4 相关轮次重写，本轮未用其结论）。
- 已更新：`evidence/embedded_verify_gxht40_item002.py`（新增 L/M 组与 A 组切断判据 + `--selftest`）。

### 6.3 上一轮证据归档缺陷的更正（本能力自查）
EV-012 的原始捕获以 `.bin` 入库，而仓库 `.gitignore` 含 `*.bin` ⇒ 那 5 份原始字节**实际从未进入 Git**（`git check-ignore` 证实），清单中「已入库」的表述与事实不符。本轮已把 EV-012 的 5 份原件按字节复制为 `evidence/ev012/*.cap`（md5 逐份核对一致：`49378788…`/`d0dde1a3…`/`2e1409ce…`/`97bad38a…`/`c5848e1b…`），并把本轮的捕获直接以 `.cap` 入库。

### 6.4 文档同步交接（非本能力职责）
- `artifacts/test_design.md`（TD-002）与 `artifacts/firmware_design.md`（FD-002 §5.2/§6.7）尚未收录 `IOTEST`/`IOSIG` 两行与实际上电顺序 `BOOT→IOTEST→IOSIG→BUS`（实现能力本轮 §4 亦登记）。本能力已在**自己的**解析脚本中同步接受新行；TD/FD 正文同步属设计能力职责。
- 登记偏差 **D-01**（判定方向仍为降温 `(prev-cur)>9`，readme 要求升温）在本轮构建上**仍然存在**（只读复核 `fw_core.c:121`），属固件任务 T4 范围，**不属本项**验收；本能力的 fw_core harness 已把该 as-built 语义显式标注。
- `BOOT fw=FD-002r4`：固件标识字面仍为 `r4`。TD-002 明确「固件标识字符串随版本演进可能变化，判定按字段类别与取值自洽，不按固定字符串」，故不作为门禁；仅登记供设计文档自行统一。

## 7. 判定与交接

**结论：BLOCKED。** 核心验收（实板 `q=1`、连续多周期）未达成且稳定可复现；剩余全部候选原因都在**器件/接线侧**，且**必须用户现场动作**才能继续；现有资源无法替代（本板无第二个 I²C 从机作正向对照、无电气测量仪器、无可控电源开关）。按能力规则，本项**不以未提供的可选仪器阻塞**——阻塞条件是「器件在整个合法地址空间不应答」这一已证实事实，加上「必须现场动作」这一已证实条件。

**交用户（最小必要现场动作，按顺序，任一步即可分支处置）**
1. **目视 + 通断核查 U9**：确认 U9 为 GXHT40 且方向/位号正确；核查 `U9.3 (VCC)`、`U9.4 (GND)`、`U9.1 (SDA→U7.PA04)`、`U9.2 (SCL→U7.PA03)` 四处焊点（§3 网表给了逐网落点）。**不需要**为本步改动任何代码或固件。
2. **真断电复上电**：断开 `BAT` 网（取电池或断开该网）≥5 s 再恢复——注意 **MCU 复位不等于 U9 断电**（同网、无开关，§3）。若器件曾闩锁，此步可解除。
3. **换件对照**：若 1/2 无果，换一只已知良好的 GXHT40（或在确认 U9 拆下后测其 VCC-GND 阻抗）。
4. 上述任一动作后可**立即复测**：所需只是重新上电与 COM42 采集；本能力可在下一轮用同一脚本（`evidence/ev013/run.sh` + `embedded_verify_gxht40_item002.py`）在 ~10 分钟内给出「`q=1` 是否出现」的判定。**恢复条件**：`IOSIG` ACK 时隙由 `11` 变为 `00`，或 `BUS ack` 由 `none` 变为含 `44`/`45`，即表示器件已能应答。

**交实现能力**：本项**无待修项**（§5 已排除全部固件侧假设；宿主 harness 全绿）。若后续现场查出 U9 良好，请按 §6.1 的第一条重新提出「ACK 判定正向对照」的验证需求，而非在本轮证据上直接改代码。

**交设计能力**：§6.4 的 TD-002/FD-002 行集合与顺序同步。

**已交付的端口**：`evidence/programmed_prototype.md`（本轮下载身份与回读校验：2 次 `Verify OK`、axf md5 与交付件一致）；本文件 `evidence/test.md`（TestEvidence，结论 BLOCKED）。

---

# 历史记录（EV-012 / run8 ITEM-002；当轮判 TEST_FAIL：排除 H4/H5、新登记 H12，交回实现执行 §5 的 E1b；已提交 6dac824）

# 嵌入式验证证据（EV-012 / run8 ITEM-002 ↔ GXHT40 温湿度采集在真实目标上恢复有效）

状态：embedded_tester.embedded_verification 独立验证证据（本轮重新构建、重新部署、重新采集真实目标字节流；不沿用历史结论）
本轮验证对象：run8 队列项 **ITEM-002**（GXHT40 温湿度采集在真实目标上恢复有效：实板每个 3 分钟采样周期 `S` 行 `q=1`、`t/h` 落在有效域且与环境相符、连续多个周期稳定；满足上报条件时手机端读数与该周期 `S` 行 `t/h` 为同一笔）

- 受测提交：`09408c1`（接手时 `git status` 干净；`HEAD` 即实现能力本轮的 `firmware_implementation` 提交）
- 被测件：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf`，md5 **`bcfc14dc27ee9afeb02df2e0e7b4b586`**（由受测提交源码经 `mdk_build rebuild` 重建；与实现能力 `evidence/build.md` 声明的 `Code=17808 RO-data=636 RW-data=116 ZI-data=1676` 与 axf md5 **逐项/逐字符一致** ⇒ 本轮实测的二进制就是本轮实现交付的那一个）
- 验证基线：TD-002 rev 5.0 `artifacts/test_design.md`；FWR-002 v5.0 `artifacts/firmware_requirements.md`（FWR-116/117/118）；FD-002 rev 5.0 `artifacts/firmware_design.md` §6.6.2/§6.6.3；`sensor_hardware/pstxnet.dat` + `MAIN_BOARD.BOM`（本轮重新逐网复核）；`readme.txt`（字面阈值）
- 本轮新增的判别观测量：上电 `IOTEST` 行（EV-011 §5 的 **E1** 最小区分实验，由实现能力在本轮实现）

结论：**TEST_FAIL** —— 本项核心可观察在真实目标上**仍未达成**：3 次成功下载、5 个采集窗口、跨 ~28 分钟、**10 个采样周期全部 `q=0`**（其中 reset 后 `k=0,3,6,9,12,15` 六个周期为**无间隙连续观测**，~15 分钟内器件未自行恢复），3 次复位的全地址扫描**全部 `ack=none`**，10 条失败周期诊断行**逐字节相同**。

本轮**实质性进展**（新证据、非重复）：E1 自检已在**实板**取得读数，且 5 次读数**逐字节相同**：`IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none`。按冻结语义（`sda_lo/scl_lo = 0` 表示主机确实把该线拉低并回读为低；`idle=3` 表示释放后两线被上拉拉高；`swap=none` 表示 SDA/SCL 角色对调后两候选地址均无 ACK），该读数**直接排除**了原竞争假设中的 **H5（主机根本拉不低该线）** 与 **H4（板级装配把 SDA/SCL 接反）**，把故障面从「主机侧或器件侧不可分」收缩到**器件/接线侧**，并**新登记一条主机侧残余假设 H12**（见 §5）。判定仍为 TEST_FAIL：本项核心验收未达成且稳定可复现；根因**尚未证实**属外部条件缺失，且现有资源仍可再切掉 H12 这一分支（§5 E1b），故不宣告外部阻塞。

| # | 事实 | 性质 | 依据 |
|---|---|---|---|
| 1 | 实板 **0/10 个采样周期 `q=1`**；核心可观察（`q=1`、`t/h` 有效域、连续多周期稳定）**未达成** | 功能不符（本项核心验收不通过） | §2.2 |
| 2 | **reset#3 之后 `k=0,3,6,9,12,15` 六个周期无间隙连续**（~15 分钟，4 个采集窗口首尾相接），期间器件**未自行恢复**、也无任何成功周期 | 失败稳定、非偶发；排除「偶发/需等待自愈」 | §2.2 |
| 3 | 10 条失败周期 `G` 行**逐字节相同**：`G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`；3 条 `BUS` 行**逐字节相同**：`BUS idle=3 scl=1 sda=1 ack=none` | 失败形态稳定可复现（3 次独立复位） | §2.1、§2.2 |
| 4 | **E1 实板读数 5/5 逐字节相同**：`IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none` ⇒ 主机**确实**能把 PA04(SDA) 与 PA03(SCL) 拉低并回读为低、释放后两线回高；**未**发现 SDA/SCL 接反 | 本轮新增的判别事实 | §2.3 |
| 5 | `IOTEST idle=3` 与同一次复位的 `BUS idle=3` **逐次一致**（同一对引脚、两处独立读取不得互相矛盾） | 自洽性交叉校验 | §2.3、§3 |
| 6 | 同窗口内光照通路持续工作且读数随环境变化：`o=8` 恒成立，原始码均值 `a` 在 3422–3624 之间（同一会话内先升后稳），与 EV-011 下午（16:20–16:47）的 130–388 相比整体抬升约一个数量级，符合傍晚环境光下降 | 交叉对照：MCU/时钟/ADC/GPIO/UART1 通路均正常 | §2.4 |
| 7 | 失败周期**不上报、不推进前一有效温度、不构造 0 值上报**：`r=0 s=0 y=0 p=0 H=0 V=0 D=0 q=0`、`raw=------------`（无效帧） | 冻结不变量在实板成立 | §2.2 |
| 8 | 交付源码只写**唯一**命令 `0xFD`（高重复率测量）；`0x94` 软复位仅作为配置宏存在、**周期路径零调用**；无加热命令 | 只读复核（不属产品改动） | §3.3 |

---

## 1. 环境、构建与部署（本轮实际调用，原始结果）

| 工具 / 命令 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `COM42 = Prolific USB-to-Serial Comm Port (VID:PID 067B:2303)`；`COM26 = USB 串行设备 (C251:F001, SN 87094109484987710672FF50)` | 传感器调试口 **COM42 在位**，与配置声明的设备身份一致 |
| `usb_list_devices` | `USB\VID_C251&PID_F001`（HID MI_02 + Ports MI_00，Status OK，SN `87094109484987710672FF50`） | **CW-DAPLink 下载探针在位** |
| `mdk_build rebuild` | `*** Using Compiler 'V6.24'`；`../USER/src/main.c(231): warning: while loop has empty body`；`Program Size: Code=17808 RO-data=636 RW-data=116 ZI-data=1676`；`0 Error(s), 1 Warning(s)` | 0 错误；1 条为**基线既有**空体 `while(k--);`。`Code/RO/RW/ZI` 四项与实现证据声明**逐项一致**；唯一差异是告警行号：实现 `evidence/build.md` 写 `main.c(224)`，本轮重建为 `main.c(231)`（同一告警、同一性质；EV-011 的上一版镜像该告警确在 `224`），属实现证据中的**行号笔误**，不影响受测件身份（axf md5 一致）或本轮判定 |
| `md5sum …/Project.axf` | `bcfc14dc27ee9afeb02df2e0e7b4b586` | 与实现交付的产物 md5 **完全一致** ⇒ 受测件身份闭合 |
| `mdk_flash {}` ×3（本地 18:09:39 / 18:17:16 / 18:22:17） | 每次均 `Erase Done.Programming Done.Verify OK.Application running ...`；下载前 `Build target 'Project'` 为 **0 Error(s), 0 Warning(s)**（无可重建项，故所下即上面那个 axf） | 当前交付镜像已下载并**回读校验成功**；3 次成功亦再次证明**在目标已连续打印多条轨迹之后 SWD 仍可用**。按能力约定：`Verify OK` **仅**证明下载校验，业务功能仍须实测（§2） |
| `git status --short` | 空 | 所编/所下的源码即受测提交；本轮未改产品源码 |

授权与入口：本轮调用 context 的 `当前能力有效工具` 与 `本次允许调用的工具及参数` **均非空**（9 个工具），实际调用 5 个（`serial_list_ports`/`usb_list_devices`/`mdk_build`/`mdk_flash`/`serial_capture`）；MCP 工具入口与 `tool_cli.py` CLI 入口**均可用**（本轮采集与烧录走 CLI 入口，`mdk_build` 走 MCP 入口，两者结果一致、指向同一工程与同一日志）。

## 2. 真实目标 UART1 观测（COM42 @9600 8N1，原始字节）

采集方式：`serial_capture`（单次上限 300 s，采样周期 ≈180 s）。为让复位横幅落在窗口内，把 `mdk_flash` 与采集**串接在同一 shell 脚本**中（`evidence/ev012/run.sh`）。原始捕获已入库 `evidence/ev012/*.bin`（5 份，md5 见 §2.5），下载日志见 `evidence/ev012/mdk_flash_logs.txt`。

| 捕获 | 文件 | 窗口（UTC） | 字节 | 内容 |
|---|---|---|---|---|
| b1 | `evidence/ev012/b1.bin` | 10:09:26.922 → 10:14:27.124（内含 `mdk_flash`，复位 10:09:39） | 412 | `BOOT`×2 + `IOTEST` + `BUS` + `S k=0`+`G` + `S k=3`+`G` |
| b2 | `evidence/ev012/b2.bin` | 10:17:03.481 → 10:22:03.685（内含 `mdk_flash`，复位 10:17:16） | 455 | `BOOT`+`IOTEST`+`BOOT`+`IOTEST`+`BUS` + `S k=0`+`G` + `S k=3`+`G` |
| b3 | `evidence/ev012/b3.bin` | 10:22:04.125 → 10:27:04.337（内含 `mdk_flash`，复位 10:22:17） | 455 | 同 b2 结构 |
| c1 | `evidence/ev012/c1.bin` | 10:27:04.771 → 10:32:04.976 | 236 | `S k=6`+`G` + `S k=9`+`G` |
| c2 | `evidence/ev012/c2.bin` | 10:32:05.387 → 10:37:05.599 | 238 | `S k=12`+`G` + `S k=15`+`G` |

**字节账目（逐字节闭合，无未知字节）**——按 `\r\n` 切分后逐行核对（每项 = 行长含 CRLF）：

| 捕获 | 行构成 | 账目 |
|---|---|---|
| b1 | `BOOT:50`×2、`IOTEST:43`、`BUS:33`、`S:72`、`G:46`、`S:72`、`G:46` | = **412** ✓ |
| b2 / b3 | `BOOT:50`、`IOTEST:43`、`BOOT:50`、`IOTEST:43`、`BUS:33`、`S:72`、`G:46`、`S:72`、`G:46` | = **455** ✓ |
| c1 | `S:72`、`G:46` ×2 | = **236** ✓ |
| c2 | `S:73`、`G:46` ×2（`k` 为两位数） | = **238** ✓ |

全部行长 ≤ 73 B（远低于 96 B 预算）、全部以 CRLF 终止、无截断残行、无裸 LF、行首仅 `BOOT`/`IOTEST`/`BUS`/`S`/`G`。

### 2.1 上电诊断行（3 次独立复位，逐字段）

| 复位 | 捕获 | 原始 `IOTEST` 行 | 原始 `BUS` 行 |
|---|---|---|---|
| #1（`mdk_flash` 10:09:39） | b1 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none` | `BUS idle=3 scl=1 sda=1 ack=none` |
| #2（`mdk_flash` 10:17:16） | b2 | 同上（**2 条，逐字节相同**） | `BUS idle=3 scl=1 sda=1 ack=none` |
| #3（`mdk_flash` 10:22:17） | b3 | 同上（**2 条，逐字节相同**） | `BUS idle=3 scl=1 sda=1 ack=none` |

- `IOTEST` 共 **5 条**（b1 1 条 + b2 2 条 + b3 2 条），**5/5 逐字节相同**。
- 每条 `IOTEST` 都**紧跟**其复位段的 `BOOT` 横幅、且**紧随其后**就是该段的 `BUS` 行（`main.c` 上电顺序 `BOOT` → `IOTEST` → `BUS`）；`c1`/`c2`（无复位窗口）内 0 条。
- b1 的第 1 条 `BOOT`（`rst=0040`）**没有**随之的 `IOTEST`/`BUS`：该次复位很快被 `mdk_flash` 的擦除/编程流程中止（调试器 halt），boot 打印序列未走完。这是**烧录序列的正常副产物**（EV-008/EV-011 已登记「首条横幅属烧录产物」），我方脚本据此只把**走完 boot 打印的复位段（带 `BUS` 行者）**作为约束对象；被中止的段记 note、不记通过（见 §4 的 `J` 项）。

### 2.2 采样周期：`S` 行 → 紧随之 `G` 行（本轮全部 10 个周期）

| 捕获 | `S` 行（原文节选：`k/p/H/V/o/n/x/a/D/t/h/q/r/s/y`） | 紧跟的 `G` 行（原文） |
|---|---|---|
| b1 | `S k=0 p=0 H=0 V=0 o=8 n=3419 x=3425 a=3422 D=0 t=0 h=0 q=0 r=0 s=0 y=0` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` |
| b1 | `S k=3 … n=3504 x=3511 a=3508 …` | 同上（逐字节相同） |
| b2 | `S k=0 … n=3590 x=3596 a=3594 …` | 同上 |
| b2 | `S k=3 … n=3614 x=3620 a=3617 …` | 同上 |
| b3 | `S k=0 … n=3604 x=3610 a=3607 …` | 同上 |
| b3 | `S k=3 … n=3618 x=3627 a=3623 …` | 同上 |
| c1 | `S k=6 … n=3620 x=3628 a=3624 …` | 同上 |
| c1 | `S k=9 … n=3611 x=3618 a=3614 …` | 同上 |
| c2 | `S k=12 … n=3608 x=3615 a=3612 …` | 同上 |
| c2 | `S k=15 … n=3605 x=3615 a=3611 …` | 同上 |

- 每个 `q=0` 周期**恰好一条** `G` 行且**紧跟**其 `S` 行，中间无其它字节；10 条 `G` 行**逐字节相同**。
- **连续性（本轮加强项）**：reset#3 之后 `k=0,3`（b3）→ `k=6,9`（c1）→ `k=12,15`（c2）首尾相接，构成 **6 个无间隙连续周期（~15 分钟）**；期间无任何 `q=1`、无器件自愈迹象。EV-011 的 `k=6` 缺口（两次采集调用之间的 ~59 s 间隙）本轮**已被填补**。
- **失败周期语义**：`r=0`（未满足上报条件）、`s=0`（未发送）、`y=0`（无重试）、`p=0`（前一有效温度从未推进）、`H=0`、`V=0`（未标定 ⇒ 不足以证明「无光」）、`t=0 h=0 q=0` 且 `raw=------------`（帧无效）。⇒ **未出现**「不上报却上报」「失败却构造 0 值上报」「失败推进前值」。
- **节拍**：观测到的 `k` 序列在同一复位段内 `Δk≡3`（各段分别为 `0,3`；`0,3`；`0,3,6,9,12,15`）。按窗口边界反推，1 个 `k` 拍 ≈ 58–59 s，一个采样周期（3 拍）≈ 175 s，即在 RTC/LSI 容差内符合需求「3 分钟」；节拍未被本轮改动破坏（节拍项的完整判定不属本项）。

### 2.3 上电 I/O 自检（E1）——本轮的关键新读数

原始行（5/5 相同，任取一条）：

```
IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none
```

| 字段 | 实板读数 | 冻结语义（FD-002 上电诊断集合 / 实现能力判读表） | 本能力据此排除的假设 |
|---|---|---|---|
| `sda_lo` | **0** | 把 PA04(SDA) 按开漏拉低后回读：**0 = 主机确实拉低成功**（且回读通路能读出 0） | **H5**：主机拉不低 SDA |
| `scl_lo` | **0** | 把 PA03(SCL) 按开漏拉低后回读：**0 = 主机确实拉低成功** | **H5**：主机拉不低 SCL |
| `idle` | **3** | 释放两线后回读位掩码（bit0=SCL、bit1=SDA，1=高）⇒ 两线均被外部上拉拉高（与网表 R1/R2 4.7 K→`BAT` 一致） | 缺上拉/线间短路 |
| `swap` | **none** | SDA/SCL **角色对调**后候选 0x44/0x45 均无 ACK ⇒ 未发现接反 | **H4**：板级装配把 SDA/SCL 接反 |

三点自洽校验（本轮独立检查，§4 的 `J`/`K` 项）：
1. `sda_lo`/`scl_lo` 与 `idle` **来自同一引脚、同一读函数的两次不同取值**（先 0、后 1）⇒ 该回读**不是常量**，确实跟踪驱动状态；若回读恒 1 则 `sda_lo/scl_lo` 会是 1，若恒 0 则 `idle` 会是 0，二者**均未出现**。
2. 每条 `IOTEST` 的 `idle` 与其复位段 `BUS` 行的 `idle` **逐次相等（5/5 = 3）**——同一对引脚在两处独立读取，不得互相矛盾。
3. `swap=none` 与网表（§3.2）**一致**：网表里 `U7.PA04↔U9.1(SDA)`、`U7.PA03↔U9.2(SCL)` 正是本工程软 I²C 的引脚角色，故「对调后无 ACK」是预期结果，同时也**不能**用「接反」解释当前的 `ack=none`（H4 被排除，而非被证实）。

> **本能力对该读数的判读边界**：`sda_lo=0/scl_lo=0` 证明的是**静态直流驱动与回读能力**，即主机能把两条线拉到低、能放开、并能正确读回电平；它**不**证明「地址字节期间的位序列与时钟边沿一定被完整送到器件引脚上」（见 §5 的 H12，本轮**新登记的主机侧残余假设**）。同样，`swap=none` 只排除「SDA/SCL 两条线整体接反」，不排除 U9 单个引脚开路/虚焊。

### 2.4 交叉对照：同一窗口内光照通路工作且随环境变化

| 字段 | 10 个周期实测 | 含义 |
|---|---|---|
| `o` | **恒为 8** | 每次采样的 8 次 AIN11 转换全部成功 ⇒ ADC 与 PB04/PB05 通路工作 |
| `a`（ADC 原始码均值） | 3422 → 3508 → 3594 → 3617 → 3607 → 3623 → 3624 → 3614 → 3612 → 3611 | 会话内稳定在 ~3.6 K 码附近；与会话外对照（EV-011 下午 16:20–16:47 为 130→388）相比整体抬升约一个数量级，符合**傍晚环境光下降**；不是常量、不是伪造值 |
| `n / x` | 逐周期在均值附近小幅波动（如 `3608/3615`、`3605/3615`） | 每次采样内多转换之间自洽 |
| `V` / `D` | 恒为 0 / 恒为 0 | 未标定（`LIGHT_DARK_CALIBRATED=0`）⇒ 不足以证明「无光」，判据门禁未被绕过 |

⇒ MCU、时钟、ADC、GPIO 与 UART1 观测通路均正常，故障面被进一步限定在 **GXHT40(U9) 及其 I²C 连接**上，而非整板/时钟/供电总体。

> 登记性观察（不属本项）：本次会话的 `a` 读数是一次**真实物理量**且随环境变化；但**本轮不据此声明任何全暗标定**——`C_dark` 必须取自**完全遮光**条件，本项未做遮光，故 `LIGHT_DARK_CALIBRATED` 仍应为 0（实板 `V=0` 与之一致）。

### 2.5 原始捕获入库与校验

| 文件 | 字节 | md5 |
|---|---|---|
| `evidence/ev012/b1.bin` | 412 | `493787880ec114b36b76cfb6243683a9` |
| `evidence/ev012/b2.bin` | 455 | `d0dde1a36cee170835c55582febb5213` |
| `evidence/ev012/b3.bin` | 455 | `2e1409ceacc40d2effc8cf04c2d512b1` |
| `evidence/ev012/c1.bin` | 236 | `97bad38a21d840674d820cf9927e4710` |
| `evidence/ev012/c2.bin` | 238 | `c5848e1b1bfcae6811f6c6113158474c` |
| `evidence/ev012/mdk_flash_logs.txt` | — | 3 次 `mdk_flash` 的 build/flash 原始日志与时间戳 |
| `evidence/ev012/run.sh` | — | 本轮「采集 + 烧录串接」的可复跑脚本（含 context/workspace/工具参数） |

（捕获原件同时保存在 Runtime 角色工作目录 `work/verification_tools/serial-capture-*.bin`；两份内容一致。）

## 3. 失败环节定位（本能力独立复核，不引用实现自述）

把 10 条 `G` 行与当前驱动代码路径逐项对齐：

| 观测 | 闭合到的代码事实 | 排除的环节 |
|---|---|---|
| `a44=0 a45=0` | `gxht40_diag_note_ack()` 只在 `i2c_write_cmd()`（地址字节）成功后置位；两位皆 0 ⇒ 3 次重测中 `0x44`/`0x45` 的**地址字节处即 NACK** | 「地址变体」（两候选都探了，且 `0x08..0x77` 全域无应答） |
| `rd=0` + `raw=------------` | 读重试计数仅由成功读或读重试耗尽分支写入；`rd=0` ⇒ **从未进入读阶段** | 「读请求 NACK 未就绪」、「CRC 错」、「量程无效」——三者都发生在读回 6 字节**之后** |
| `at=3` | `GXHT40_MEAS_RETRY = 3`（`sensor_config.h:56`），三种尝试全部用尽 | 「单次偶发」；失败每周期稳定复现 |
| `s=2` 与 `a44=a45=0` 并存 | 修订后的裁决规则：仅当**无任何候选 ACK** 时才报 `NO_DEVICE` | EV-009 登记的 OBS-2 张力（旧形态 `s=2` + `a44=1`）**未出现** |
| `idle=3` 且每周期都重探两地址 | 稳态下缓存地址恒为 0（从未成功）⇒ 每个 `gxht40_acquire()` 都走「完整重探」分支：先 `i2c_bus_recover()`，再依次探 0x44/0x45 | 「总线被从机拉低卡死」与「缓存地址失效导致永久失联」（FWR-118） |

### 3.1 本轮新增的独立事实（相对 EV-011）

- **E1 实板读数**（§2.3）：主机能把两条线拉低并回读为低、释放后回高，且未发现接反 ⇒ **H5、H4 被排除**（EV-011 时二者仍开放）。
- **连续 6 周期无恢复**（§2.2）：把「偶发/需要等待自愈」也压掉了。

### 3.2 网表逐网复核（本轮完整重读 `sensor_hardware/pstxnet.dat`，非摘要）

- `N05845` = {`R1.2`, `U7.14(PA04)`, `U9.1(SDA)`}；`N06024` = {`R2.1`, `U9.2(SCL)`, `U7.13(PA03)`} ⇒ **SDA=PA04、SCL=PA03**，与软 I²C 的 `GPIO_PIN_4 | GPIO_PIN_3` 配置以及 `IOTEST swap=none` 三者一致。
- `BAT` 网含 `J1.1`(CR2032+)、`U7.9(VDD)`、`U7.8`(VCORE 另网)、`R1.1`、`R2.2`、`L2.2`、`U4.6`、`C7.2`、`C8.1`、`C9.1`、`C13.1`、`C14.1`、`C15.1`、`C16.1`、`J2.1`、**`U9.3(VCC)`** ⇒ U9 与 MCU **同接电池、直连**，**不存在任何固件可控的供电门/使能开关** ⇒「固件忘记给传感器上电」被网表排除；同时 ⇒ **MCU 复位（含调试器复位）并不切断 U9 供电**，真断电必须物理断开 J1.1/CR2032（§5 E2）。
- `GND` 网含 `U9.4`；`NC` 网含 `U9.5(PAD)`（未连接）。`MAIN_BOARD.BOM` 第 22 行 `gxht40 U9 1 GXHT40`（BOM 有该器件，数量 1）。
- GXHT40 的两条 I²C 网上**只有 U9 一个器件**（另有 R1/R2 上拉与 MCU 引脚）⇒ 板内**不存在第二个已知良好的 I²C 器件**可作对照（这是本项最小区分实验的客观限制，§5）。

### 3.3 周期性不变量只读复核（不属产品改动，仅阅读交付源码）

| 要求（ITEM-002 描述原文） | 交付源码事实 | 结论 |
|---|---|---|
| 周期路径**不得**使用软复位 `0x94` 或加热命令 | `grep` 全部 `USER/src`、`USER/inc`：唯一被写入总线的命令是 `GXHT40_CMD_MEASURE_HIGH_REP`(`0xFD`)，位于 `gxht40.c:93`；`GXHT40_CMD_SOFT_RESET 0x94` 仅作为宏定义出现在 `sensor_config.h:45`，**零调用**；无任何加热命令 | 成立 |
| 保留温度字与湿度字的**双字 CRC-8** 校验 | `gxht40.c:203..208`：`fw_crc8_gxht(&buf[0],2)!=buf[2] || fw_crc8_gxht(&buf[3],2)!=buf[5]` ⇒ 整帧丢弃、不写输出、失效缓存地址、重测 | 成立 |
| **整数 x10 换算与有效域** | `gxht40_raw_to_x10()` 返回假即视为无效（`GXHT40_ERR_RANGE`），无效**不写输出** | 成立 |
| **有界读重读**与**有界整帧重测** | `GXHT40_READ_RETRY=5`（每次间 1 ms）、`GXHT40_MEAS_RETRY=3` | 成立（实板 `at=3`、`rd=0` 与该上限一致） |
| 任一失败后下一周期**重新探测 0x44/0x45**，重探前做**有界总线恢复** | `gxht40_acquire()`：缓存地址失败即清 0 并转入完整重探；重探前 `i2c_bus_recover(s_dev)`；随后**依次**探 A、B（即使 A 成功也把 B 探完）；`gxht40_measure()` 失败出口再次 `s_addr7=0` | 成立（实板每周期 `a44=0 a45=0` 即两候选都被探过） |
| 失败周期**不上报/不推进前值/不构造 0 值** | 仅完全成功才写 `*temp_x10/*hum_x10`；诊断快照与上报调度分离 | 成立（实板 10 周期 `r=0 s=0 p=0`、`raw=------------`） |

⇒ 主机（MCU）侧在**静态与状态机语义层面**未发现能造成「112 个地址全域无 ACK」的缺陷；但**这不等于已证明器件或接线损坏**（见 §5 仍开放的假设）。

## 4. 独立判定（本能力自建脚本，不引用实现源码）

脚本：`evidence/embedded_verify_gxht40_item002.py`（按冻结契约逐字节解析；阈值取 readme 字面值，不从被测实现推导）。本轮按 EV-011 交接**新增 `IOTEST` 行的域与结构检查**（`J` 组）与**跨复位读数登记**（`K` 组）：

| 组 | 检查内容 |
|---|---|
| `J IOTEST` | 字段域（`sda_lo/scl_lo ∈ {0,1}`、`idle ∈ 0..3`、`swap ∈ {none, 合法 7bit 地址列表}）；**走完 boot 打印的复位段**内恰 1 条 `IOTEST`、顺序 `BOOT<IOTEST<BUS` 且 `IOTEST` **紧邻** `BUS`；`IOTEST idle` 必须等于该段 `BUS idle`；被烧录流程中止的段只记 note（`len(io)<=1`），不记通过 |
| `K IOTEST` | 跨全部捕获登记全部读数并断言：主机两线均可拉低、释放后均回高、无接反 |

```
$ python evidence/embedded_verify_gxht40_item002.py evidence/ev012/{b1,b2,b3,c1,c2}.bin
    periods observed: 10  (q=1: 0, q=0: 10)
    longest run of consecutive q=1 periods: 0
    distinct IOTEST readings: 1 of 5 occurrences
      x5  IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none
===== CHECKS =====
    FAIL  H ACCEPT >=3 consecutive q=1 periods with t/h in domain  [longest run = 0 (needed >= 3)]
    (100 checks passed, 1 failed)
===== RESULT: FAIL =====   (exit=1)
```

**唯一失败项就是本项的核心验收**（`H ACCEPT`）。其余 100 项全部通过，覆盖：行长 ≤96 B 与 CRLF 终止、行首集合、`S` 行 15 个冻结字段的顺序与取值域、`BUS`/`G`/`IOTEST` 行字段齐全、`G` 行的 `s∈1..5`/`a44,a45∈{0,1}`/`rd∈0..5`/`at∈1..3`/`raw` 形态、**`s=2 ⇔ 两候选均未 ACK`** 与 **`s∈{3,4,5} ⇒ 至少一个候选 ACK`** 两条硬门禁、`G` 行位置（`q=0` 恰一条且紧随、`q=1` 不得有）、节拍 `Δk==3`、失败不推进前值、以及上述 `J`/`K` 的 IOTEST 结构与读数。

**门禁有效性（负对照，本轮实做）**：把同一脚本对 **EV-011 的旧捕获**（无 `IOTEST` 行的 `serial-capture-1791449127894503400.bin`）运行，`J` 组**如实报 FAIL**（`reset#1/#2 exactly one IOTEST ... found=0`）⇒ 新增的 `IOTEST` 检查**不是空验**，它确实能区分「有/无该诊断行」。旧捕获在本轮仍报 `H ACCEPT` FAIL，与 EV-011 结论一致。

## 5. 竞争假设与最小区分实验（交回实现 / 交用户）

**预期 / 实际**：预期每个 3 分钟周期 `S` 行 `q=1`、`t/h` 落在 −40.0..125.0 ℃ / 0..100.0 %RH 且与环境相符、连续多周期稳定；**实际** 10/10 周期 `q=0`、`t=0`、`h=0`，每周期紧跟 `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`，每次复位 `BUS ack=none`，`IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none`。

**复现步骤（可直接复跑，无需仪器）**：① `mdk_build rebuild`（0 Error）；② `mdk_flash {}`（Verify OK）；③ 在 COM42@9600 连续 `serial_capture`（300 s），使复位落在窗口内；④（连续周期）紧接着不加复位地再采集 1–2 个窗口。复现率：**3/3 复位、10/10 周期**。

| # | 假设 | 本轮状态 |
|---|---|---|
| H1 | U9 未贴装 / 虚焊 / 引脚开路（SDA、SCL、VCC、GND 任一） | **未排除**（需目视/通断/换件） |
| H2 | U9 本体损坏 | **未排除**（需换件） |
| H3 | U9 被闩锁在异常态，需**真正断电**复位 | **未排除**；且本轮三次复位**均为调试器复位**，`U9.VCC` 与 MCU 同接 `BAT`（§3.2）⇒ **整个会话期间器件从未掉电** |
| H4 | 板级装配把 SDA/SCL 接反 | **本轮排除**：实板 `IOTEST swap=none`（角色对调后两候选仍无 ACK），且与网表一致 |
| H5 | 主机**根本拉不低** SDA/SCL（引脚驱动器/走线/配置层），总线上没有合法地址位 | **本轮排除**：实板 `IOTEST sda_lo=0 scl_lo=0`（拉低可回读为 0、释放可回读为 1） |
| H6 | 总线速率/时序超器件上限 | 已排除（位延时远低于 100 kHz） |
| H7 | 等待下限不足（tMEAS/tPU） | 已排除（`MEASURE_WAIT_MS=10 ms` ≥ 手册 tMEAS.H 上限 8.3 ms；`POWER_ON_WAIT_MS=2 ms` ≥ tPU ≤1 ms；且失败发生在**地址字节处**） |
| H8 | 地址变体 | 已排除（`0x08..0x77` 共 112 个 7bit 地址全域无 ACK） |
| H9 | 读请求 NACK / CRC 错 / 量程无效 | 已排除（`rd=0`，从未发起读事务） |
| H10 | 总线被从机持续拉低卡死 | 已排除（负对照证明持续拉低会得到满表 ACK，与实测 `ack=none` 相反） |
| H11 | 固件未给传感器上电 | 已排除（网表证明 `U9.VCC` 直连 `BAT`，无固件可控供电门） |
| **H12** | **主机静态能拉低/释放，但事务期间的位/时钟形态未被真实送达**（例如地址字节期间 SCL 未按时序翻转、或 SDA 未被驱动到目标位；`IOTEST` 只覆盖静态驱动能力，不覆盖动态位序列） | **本轮新登记，未排除**；现有字节流无法区分（`ack=none` 与「器件不存在」同形）。**这是当前唯一仍可由现有资源切掉的主机侧分支** |

**最小区分实验（按成本排序）**

1. **E1b（区分 H12 与器件侧；属实现能力的测试构建，本能力不修改产品源码）**：把上电自检从「静态拉低/回读」扩展为**事务内逐位回读签名**——在写地址字节（含 ACK 时隙）的**每个半位**读取 SCL/SDA 回读电平并累积成一个紧凑签名（例如每位的 2 bit 读数拼成十六进制串）打印，例如 `IOSIG scl=.. sda=..`。判读：
   - 回读**跟随**预期位序列（SCL 每位低/高各一次、SDA 在数据位等于被驱动的位、ACK 时隙释放后为高）⇒ 主机确实把合法位与时钟送到引脚上 ⇒ 故障在**器件侧**（H1/H2/H3），转 E2/E3；
   - 回读**不跟随**⇒ 落在 **H12**，主机侧仍有可修项（本能力随即可据此复判）。
   成本约十行、一次性、可复跑，**不需要任何仪器**，能一次切掉最后一个主机侧分支。
2. **E2（器件侧，需用户现场动作）——给传感器一次真实断电复上电**：`U9.VCC` 与 MCU 同接 `BAT`（§3.2），**调试器复位不等于器件掉电**；需断开/取下 CR2032（J1.1）数秒后再上电。这是唯一能让 GXHT40 真正经历一次上电复位的动作，用于区分 H3 与 H1/H2。执行后本能力在同一 COM42 上连续采集 ≥10 分钟（≥3 个周期）即可判定。
3. **E3（器件侧，需用户现场动作）——目视/通断检查 U9**：是否贴装、5 脚有无虚焊/连锡；量 `U9.1↔PA04`、`U9.2↔PA03`、`U9.3↔BAT`、`U9.4↔GND` 通断（网表基准见 §3.2）。若方便可直接换一颗 GXHT40 复测。
4. **E4（可选，需**未提供**的仪器）**——示波器/逻辑分析仪观察 PA03/PA04 在地址字节与第 9 个时钟上的波形，可同时区分 H12/H4/H5 与器件侧。用户本轮未提供该仪器，故**不作为通过门槛**，仅在前三项无法收敛时作为排除手段。

**本能力对上述假设不作结论**：现有字节流**不足以**判定是器件损坏还是主机侧动态位序列未被送达；`IOTEST` 已把主机侧的**静态**驱动能力证明清楚并排除了接反，但**不能**据未知根因宣告外部阻塞（ROLE/PROMPT：无 ACK 首先是 TEST_FAIL；未知根因不能证明硬件损坏或软件无可修）。

## 6. 本轮未覆盖项与证据缺口（如实登记）

| 项 | 状态 | 原因 / 说明 |
|---|---|---|
| 成功周期实板形态（`q=1`、无 `G` 行、`t/h` 有效域、连续 ≥3 周期） | **未覆盖** | 器件在 `0x08..0x77` 全域不应答（§2.1）⇒ 当前硬件上不可产生 |
| 手机端读数与该周期 `S` 行 `t/h` 为同一笔 | **未执行（前提不成立）** | 10 个周期 `r=0 s=0`，从未满足上报条件，没有任何上报可比较；该子句以「满足上报条件时」为前提。为避免与本项无关的**生产副作用**，本轮**未**安装/启动 Android App（`android_test` 已授权但未调用） |
| 实板 `raw` 十六进制形态 / `ack` 地址表 | 未覆盖（实板） | 同上（`rd=0` 从未读回帧、`ack=none` 无地址应答）；仅由宿主 harness 覆盖（历史结论仍有效，本轮未改其断言） |
| E1b 事务内逐位回读签名 | **未执行** | 需要变更产品镜像（新增测试打印），属实现能力；本能力不改产品源码 |
| E2 真实断电复上电 / E3 目视·通断·换件 | **未执行** | 必须用户现场动作（电池焊在板上），现有资源无法替代 |
| I²C 位级电气/时序波形、供电电流、U9 焊接与供电实测 | **未覆盖** | 用户未提供逻辑分析仪/示波器/万用表/电流表；**不作为本项通过门槛**，仅在 §5 作为排除手段（E4） |
| 节拍项的完整判定 | 不属本项 | 本轮仅登记观测：同段 `Δk≡3` 且 3 拍 ≈175 s（RTC/LSI 容差内符合「3 分钟」）；完整判定属节拍任务项 |
| 光照标定 / 全暗基准 | 不属本项 | 本轮 `V=0 D=0` 恒成立；`a≈3.6 K` 只是**有光环境下的真实读数**，未做完全遮光 ⇒ 不声明任何 `C_dark`，`LIGHT_DARK_CALIBRATED` 仍为 0（属光照判据/标定任务项） |

## 7. 判定与交接

| 维度 | 结论 |
|---|---|
| 部署与受测件身份 | 通过：`mdk_build rebuild` 0 Error/1 既有告警、`mdk_flash` ×3 均 Verify OK、axf md5 与实现交付**逐字符一致** |
| 实板可观察（本项核心验收） | **未达成**：0/10 周期 `q=1`（3 次独立复位、~28 分钟、10 个周期全域扫描 `ack=none`、reset 后连续 6 周期无恢复） |
| 失败稳定性 | 10 条 `G` 行、3 条 `BUS` 行**逐字节相同**；5 条 `IOTEST` 行**逐字节相同** ⇒ 稳定可复现，非偶发 |
| 结构 / 字段 / 自洽 | 通过：行长 ≤73 B、CRLF、`G`/`BUS`/`IOTEST` 字段域与位置、`IOTEST idle == BUS idle`、失败周期不上报/不推进前值/不构造 0 值（100 项独立检查全通过；`IOTEST` 检查经旧捕获负对照证非空验） |
| 主机侧复核 | 静态驱动能力**已由实板 E1 证明**（H5/H4 排除）；周期路径无软复位 `0x94`/加热命令、双字 CRC-8/有效域/有界重读重测/失败重探与总线恢复齐备；**残余 H12 未排除** |
| **Outcome** | **TEST_FAIL**（功能不符、稳定可复现；核心验收未达成；按能力规则交回实现并附 §5 复现、预期/实际、竞争假设与最小区分实验；不据未知根因宣告外部阻塞） |

**交接**

1. **【交实现能力】** 本轮把 **H5（主机拉不低）** 与 **H4（装配接反）** 从「未排除」变为**已排除**（实板 `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none`，5/5 一致），并**新登记 H12**：`IOTEST` 只证明静态驱动/回读能力，**不**证明地址字节期间的位序列与时钟边沿被真实送到引脚。请按 §5 **E1b** 增加**事务内逐位回读签名**（一次性、无需仪器、约十行）以切掉最后一个主机侧分支；若签名显示位序列与时钟形态正确，则本项的剩余解释全部落在**器件/接线侧**（H1/H2/H3），届时由 §5 的 E2/E3 现场动作决定。
2. **【交用户（最小必要现场动作，无需仪器）】** ① 给传感器一次**真实断电复上电**（`U9.VCC` 与 MCU 同接 `BAT`，调试器复位**不等于**器件掉电；整个会话期间器件从未断电）；② 目视/万用表检查 U9 贴装与 `U9.1↔PA04`、`U9.2↔PA03`、`U9.3↔BAT`、`U9.4↔GND` 通断（必要时换一颗 GXHT40）。两项完成后告知即可，本能力立即在同一 COM42 上连续采集 ≥10 分钟复判（不需要额外仪器，也不需要重新改代码）。
3. **【登记性观察，交设计/测试设计能力同步文档】** 交付镜像的复位横幅仍为 `BOOT fw=FD-002r4 …`（固件身份以 axf md5 为准，横幅字符串是设计版本号而非构建哈希）。本轮实测新增了上电 `IOTEST` 行，但设计/测试设计三份基线**均未收录**该行（本轮逐一核对：`artifacts/firmware_design.md`、`artifacts/test_design.md`、`artifacts/firmware_requirements.md` 中 `IOTEST` 出现次数均为 **0**）；且 FD-002 现写「`BUS` 每次上电一次，**紧跟 `BOOT` 行之后**」，而当前实现的实际顺序是 `BOOT` → `IOTEST` → `BUS`。⇒ 属**文档与实现不同步**（实现能力已在 `artifacts/firmware_implementation.md` §4 登记该交接；本能力已在 EV-012 同步更新自有判定脚本），**不影响功能与本次判定**，但会误导后续按文档消费该行集合的读者。
4. **【仓库卫生】** 本轮把 5 份原始捕获、3 次下载日志与复跑脚本一并入库 `evidence/ev012/`（便于在同一仓库内复算，不依赖角色私有目录）；`git status` 干净（`mdk_build`/`mdk_flash` 产生的 `build*.log` 被 `.gitignore` 的 `/build*.log` 覆盖）。本轮产品源码**零改动**。

---

# 历史记录（EV-011 / run8 ITEM-002；当轮判 TEST_FAIL：0/10 周期 q=1、全地址 ack=none，交回实现执行 §5 的 E1 主机拉低/回读自检；已提交 001cd52）

# 嵌入式验证证据（EV-011 / run8 ITEM-002 ↔ GXHT40 温湿度采集在真实目标上恢复有效）

状态：embedded_tester.embedded_verification 独立验证证据（本轮任务项执行轮；重新构建、重新部署、重新采集真实目标字节流，不沿用历史结论）
本轮验证对象：run8 队列项 **ITEM-002**（GXHT40 温湿度采集在真实目标上恢复有效：实板每个 3 分钟采样周期 `S` 行 `q=1`、`t/h` 落在有效域且与环境相符、连续多个周期稳定）
受测提交：`8929438`（HEAD；接手时 `git status` 干净。该提交是上一轮 BLOCKED 的**空提交**——无任何文件变更，故其源码与上一实现提交 `aa3bcc4` 逐字节相同）
受测件：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf`，md5 **`e309bac2a34fd210ee966d6da9001625`**（由 HEAD 源码经 `mdk_build rebuild` 重建；与 EV-010 §1 登记的受测件 md5 **逐字符一致** ⇒ 本轮实测的二进制就是实现能力交付的那一个）
验证基线：TD-002 rev 5.0 `artifacts/test_design.md`（B1/B18、T-L2-14、§13 当前构建再观测硬前提）；FWR-002 v5.0 `artifacts/firmware_requirements.md`（FWR-117/118）；FD-002 rev 5.0 `artifacts/firmware_design.md` §6.6.2/§6.6.3；`sensor_hardware/pstxnet.dat` + `MAIN_BOARD.BOM`（网表与装配，本轮重新逐网复核）

结论：**TEST_FAIL** —— 本项核心可观察在真实目标上**未达成**：3 次独立复位、跨 ~27 分钟、10 个采样周期**全部 `q=0`**，3 条上电总线扫描全部 `ack=none`，10 条失败周期诊断行**逐字节相同**。失败形态是「**GXHT40 在所有地址上完全不应答**」这一**功能不符**；按本能力专业规则（无 ACK/零值/功能不符**首先是 TEST_FAIL**；未知根因不得据以证明硬件损坏或软件无可修；不得以失败宣告外部阻塞），本项交回实现，并附复现、预期/实际、竞争假设与最小区分实验（§5）。

> **结论类别说明（相对 EV-010）**：EV-010 对同一可观察判 BLOCKED（理由是「驱动/总线层已无剩余可修项」）。本轮按 ROLE/PROMPT 明文规则重新裁决为 **TEST_FAIL**：`无 ACK …首先是 TEST_FAIL`、`未知根因不能证明硬件损坏或软件无可修`、`只有证实必需设备/访问缺失或必须用户现场动作、现有资源无法替代时才 BLOCKED`。本轮**测试本身已实际执行**（未缺设备、未缺授权，见 §1），得到的是一个**可复现的功能不符**，而不是无法执行；把「根因未定位的功能失败」判成 BLOCKED 会把现场条件当成结论、并跳过实现侧的待办。EV-010 的一手数据与排除项本轮**全部重新采集并独立复现**，未被推翻。

| # | 事实 | 性质 | 依据 |
|---|---|---|---|
| 1 | 实板 **0/10 个采样周期 `q=1`**（`k = 0,3,9,12,15,18,21,24` + 3 次复位后各自的 `k=0`），跨 **~27 分钟**（16:20–16:47 本地时间）、**3 次独立复位**；核心可观察（`q=1`、`t/h` 有效域、连续多周期稳定）**未达成** | 功能不符（本项核心验收不通过） | §2、§3、§4 |
| 2 | 10 个失败周期的诊断行**逐字节相同**：`G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`；3 次复位的上电总线扫描**均为** `BUS idle=3 scl=1 sda=1 ack=none` | 失败**稳定可复现**，非偶发 | §2.1、§2.2 |
| 3 | 失败环节：`a44=a45=0`（两候选地址均未 ACK）+ `rd=0`（从未进入读事务）+ `at=3`（3 次整帧重测用尽）+ 112 个 7bit 地址全域无 ACK ⇒ 排除「地址变体」「读请求 NACK 未就绪」「CRC 错」「量程无效」「总线被从机拉低卡死」 | 定位结论（见 §3、§5 的开放假设） | §3 |
| 4 | 本轮**新增排除项**（EV-010 未做）：① 逐网复核网表——U9 `VCC` **直连 `BAT`**（`BAT` 网同时含 U7.VDD/R1.1/R2.2/J1.1/C7.2，**无任何固件可控的供电门/开关**）；② GXHT40 的 I²C 两条网（`N05845`/`N06024`）上**只有 U9 一个器件**（另仅 R1/R2 4.7K 上拉）；③ 软 I²C 主机静态复核：`GPIO_MODE_OUTPUT_OD`→库内 `REGBITS_SET(GPIOx->OPENDRAIN,…)` 为**真开漏**，`i2c_wait_ack()` 在采样 ACK 前把 SDA 切为**输入**、超时 255 次后释放并 STOP ⇒ 主机侧**未发现**能造成「全域无 ACK」的静态缺陷；④ `USER/src` 内除 `measure.c` 外**无**任何对 PA03/PA04 的配置/复用/复位，无 GPIOA 时钟关闭或外设复位 | 缩小故障面（但**未**闭合到硬件损坏） | §3.1 |
| 5 | 同一采集窗口内**光照通路持续工作**且随环境变化：`o=8`（8 次 ADC 转换全成功）恒成立，原始码均值 `a` 从 130 单调升到 388（16:20→16:44，见 §2.3），`n/x/a` 逐周期小幅波动 ⇒ MCU、时钟、ADC、GPIO 与 UART1 通路均正常 | 交叉对照（缩小故障面） | §2.3、§3.2 |

---

## 1. 环境、构建与部署（本轮实际调用，原始结果）

| 工具 / 命令 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `COM42 = Prolific USB-to-Serial Comm Port (VID:PID 067B:2303)`；`COM26 = USB 串行设备 (C251:F001, SN 87094109484987710672FF50)` | 传感器调试口 **COM42 在位**，与配置声明的设备身份一致 |
| `usb_list_devices` | `USB\VID_C251&PID_F001`（HID MI_02 + Ports MI_00，Status OK，SN `87094109484987710672FF50`） | **CW-DAPLink 下载探针在位** |
| `mdk_build rebuild` | `*** Using Compiler 'V6.24'`；`../USER/src/main.c(224): warning: while loop has empty body`；`Program Size: Code=16572 RO-data=624 RW-data=76 ZI-data=1676`；`0 Error(s), 1 Warning(s)` | 0 错误；1 条为**基线既有**空体 `while(k--);`。与实现证据声明的 `Code=16572 RO=624 RW=76 ZI=1676` **逐项一致** |
| `md5sum …/MDK/output/exe/Project.axf` | `e309bac2a34fd210ee966d6da9001625` | 与实现交付的产物 md5 **完全一致** ⇒ 受测件身份闭合 |
| `mdk_flash {}` ×3（16:20:15 / 16:43:40 / 16:45:41） | 每次均 `Erase Done.Programming Done.Verify OK.Application running ...` | 当前交付镜像已下载并**回读校验成功**；3 次成功也再次证明**在已连续打印多条轨迹之后 SWD 仍可用**（调试通道未被诊断路径破坏）。按能力约定：Verify OK **仅**证明下载校验，业务功能仍须实测（§2） |
| `git status --short` | 空 | 所编/所下的源码即受测提交；本轮产品源码**零改动** |
| `.gitignore` | 已含 `/build*.log` | EV-008 §6 交接的仓库卫生项**已由负责能力修复**；本轮 `mdk_build`/`mdk_flash` 产生的 `build*.log` 被正确忽略（未出现在 `git status`），本轮产生的 3 个日志文件已读入本证据后删除 |

授权与入口：本轮调用 context 的 `当前能力有效工具` 与 `本次允许调用的工具及参数` **均非空**（9 个工具：`serial_list_ports` / `serial_capture` / `serial_exchange` / `mdk_build` / `mdk_flash` / `usb_list_devices` / `android_test` / `install_system_package` / `install_python_package`），并已实际调用其中 5 个；MCP 工具入口与 `tool_cli.py` 入口**均可用且结果一致**（§2 的采集脚本使用 CLI 入口）。上一轮（`CALL-f11b625d`）的「授权为空、只读、无工具」阻塞在本轮**不复存在**。

## 2. 真实目标 UART1 观测（COM42 @9600 8N1，原始字节）

采集方式：`serial_capture`（单次上限 300 s，采样周期 ≈180 s），把 `mdk_flash` 与采集**串接在同一 shell 脚本**中（`work/ev011/run.sh`、`chain.sh`），使复位横幅与 `BUS` 行落在窗口内。原始捕获文件在 Runtime 角色工作目录 `work/verification_tools/serial-capture-*.bin`；本轮 6 份（时间均 UTC，本地 = UTC+8）：

| 捕获 | 文件（后缀） | 窗口（UTC） | 字节 | 内容 |
|---|---|---|---|---|
| c1 | `…447901458388500.bin` | 08:20:01.231 → 08:25:01.459（内含 `mdk_flash`，复位 08:20:15） | 363 | 复位横幅 ×2 + `BUS` + `S k=0`+`G` + `S k=3`+`G` |
| c2 | `…448260352063300.bin` | 08:26:00.158 → 08:31:00.352 | 115 | `S k=9`+`G` |
| c3 | `…448560967559100.bin` | 08:31:00.769 → 08:36:00.968 | 232 | `S k=12`+`G`、`S k=15`+`G` |
| c4 | `…448861589212300.bin` | 08:36:01.401 → 08:41:01.590 | 232 | `S k=18`+`G`、`S k=21`+`G` |
| d1 | `…449127894503400.bin` | 08:43:27.696 → 08:45:27.895（内含 `mdk_flash`，复位 08:43:40） | 364 | `S k=24`+`G` + 复位横幅 ×2 + `BUS` + `S k=0`+`G` |
| d2 | `…449248897131100.bin` | 08:45:28.693 → 08:47:28.897（内含 `mdk_flash`，复位 08:45:41） | 248 | 复位横幅 ×2 + `BUS` + `S k=0`+`G` |

**字节账目（逐字节闭合，无未知字节）**——按 `\r\n` 切分后逐行核对（行长含 CRLF）：

| 捕获 | 行构成 | 账目 |
|---|---|---|
| c1 | `BOOT:48+2` ×2、`BUS:31+2`、`S:67+2`、`G:44+2`、`S:67+2`、`G:44+2` | = **363** ✓ |
| c2 | `S:67+2`、`G:44+2` | = **115** ✓ |
| c3 / c4 | `S:68+2`、`G:44+2` ×2 | = **232** ✓ |
| d1 | `S:68+2`、`G:44+2`、`BOOT:48+2` ×2、`BUS:31+2`、`S:67+2`、`G:44+2` | = **364** ✓ |
| d2 | `BOOT:48+2` ×2、`BUS:31+2`、`S:67+2`、`G:44+2` | = **248** ✓ |

全部行长 ≤ 68 B（远低于 96 B 预算）、全部以 CRLF 终止、无截断残行、无裸 LF、行首仅 `BOOT`/`BUS`/`S`/`G`。（`S` 行 67 B 与 68 B 的差异来自 `k` 为 1 位或 2 位十进制，逐项可解释。）

### 2.1 上电总线身份（3 次独立复位，逐字段）

| 复位 | 捕获 | 原始 `BUS` 行 | `idle` | `scl` | `sda` | `ack` |
|---|---|---|---|---|---|---|
| #1（`mdk_flash` 08:20:15） | c1 | `BUS idle=3 scl=1 sda=1 ack=none` | 3 | 1 | 1 | none |
| #2（`mdk_flash` 08:43:40） | d1 | `BUS idle=3 scl=1 sda=1 ack=none` | 3 | 1 | 1 | none |
| #3（`mdk_flash` 08:45:41） | d2 | `BUS idle=3 scl=1 sda=1 ack=none` | 3 | 1 | 1 | none |

- 三次**逐字节相同**：`idle=3` ⇒ 释放后 SCL/SDA 均被外部上拉拉高（与网表 R1/R2 4.7K→`BAT` 一致）；`ack=none` ⇒ `i2c_probe_addr` 对 **0x08..0x77 共 112 个 7bit 地址（含 0x44/0x45）一个 ACK 都没有**。
- 每条 `BUS` 行在其复位段内**恰出现 1 次**，位于横幅之后、任何 `S`/`G` 之前；无复位窗口（c2/c3/c4）内 0 条。
- 每个复位段的两条 `BOOT` 横幅分别为 `rst=0040`（PIN）与 `rst=0240`（PIN|SYSRESETREQ）：**非零且随复位来源变化**（D-ITEM001-2 修复在当前构建继续成立）。第一条横幅与其后的重启属 `mdk_flash` 下载序列（擦除/编程/校验）的**烧录产物**。

### 2.2 失败周期：`S` 行 → 紧随之 `G` 行（本轮全部 10 个周期）

| 捕获 | `S` 行（原文节选） | 紧跟的 `G` 行（原文） |
|---|---|---|
| c1 | `S k=0 … o=8 n=130 x=132 a=130 D=0 t=0 h=0 q=0 r=0 s=0 y=0` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` |
| c1 | `S k=3 … n=137 x=139 a=137 …` | 同上（逐字节相同） |
| c2 | `S k=9 … n=147 x=150 a=148 …` | 同上 |
| c3 | `S k=12 … n=150 x=152 a=151 …` | 同上 |
| c3 | `S k=15 … n=164 x=168 a=165 …` | 同上 |
| c4 | `S k=18 … n=184 x=188 a=185 …` | 同上 |
| c4 | `S k=21 … n=217 x=220 a=218 …` | 同上 |
| d1 | `S k=24 … n=384 x=386 a=385 …` | 同上 |
| d1 | `S k=0 … n=388 x=389 a=388 …` | 同上 |
| d2 | `S k=0 … n=367 x=368 a=367 …` | 同上 |

- 每个 `q=0` 周期**恰好一条** `G` 行且**紧跟**其 `S` 行，中间无其它字节。
- **失败周期语义**：`r=0`（未满足上报条件）、`s=0`（未发送）、`y=0`（无重试）、`p=0`（前一有效温度从未推进）、`H=0`、`V=0`（未标定 ⇒ 不足以证明「无光」）、`q=0`。10 个周期一致 ⇒ **未出现「不上报却上报」「失败却构造 0 值上报」「失败推进前值」**。
- **节拍**：观测到的 `k` 序列为 `0,3,9,12,15,18,21,24`（另有 3 次复位后各自的 `k=0`，属调试器复位重启计数，基线行为）。同一次采集内 `Δk≡3`。`k=6` 落在 c1 结束（08:25:01.459）与 c2 开始（08:26:00.158）之间约 **59 s** 的窗口空隙内——这是**采集工具两次调用之间的间隙**，不是固件跳过周期（该间隙两侧 `k=3`、`k=9` 均落在以复位为锚的 3 拍栅格上）。按 6 份窗口边界反推，1 个 `k` 拍约 **57–59 s**，一个采样周期（3 拍）约 **172–177 s**，即在 RTC/LSI 容差内符合需求「3 分钟」；`Δk≡3` 的**设计契约**逐周期成立，节拍未被本项改动破坏（节拍项的完整判定不属本项）。

### 2.3 交叉对照：同一窗口内光照通路工作且随环境变化

| 字段 | 10 个周期实测 | 含义 |
|---|---|---|
| `o` | **恒为 8** | 每次采样的 8 次 AIN11 转换全部成功 ⇒ ADC 与 PB04/PB05 通路工作 |
| `a`（ADC 原始码均值） | 130 → 137 → 147 → 150 → 164 → 184 → 217 → 384 → 388 → 367（时间序） | 单次会话内单调上升约 3 倍并小幅回落 ⇒ 该读数是**真实物理量**（环境光变化），不是常量或伪造值 |
| `V` / `D` | 恒为 0 / 恒为 0 | 未标定（`LIGHT_DARK_CALIBRATED=0`）⇒ 不足以证明「无光」，判据门禁未被绕过 |
| `n / x` | 逐周期在均值附近小幅波动（如 `384/386`、`367/368`） | 每次采样内多转换之间自洽 |

⇒ MCU、时钟、ADC、GPIO 与 UART1 观测通路均正常，故障面被进一步限定在 **GXHT40 及其 I²C 连接**上，而非整板/时钟/供电总体。

## 3. 失败环节定位（本能力独立复核，不引用实现自述）

把 10 条 `G` 行与当前驱动代码路径逐项对齐：

| 观测 | 闭合到的代码事实 | 排除的环节 |
|---|---|---|
| `a44=0 a45=0` | `gxht40_diag_note_ack()` 只在 `i2c_write_cmd()`（地址字节）成功后置位；两位皆 0 ⇒ 3 次重测中 `0x44`/`0x45` 的**地址字节处即 NACK** | 「地址变体」（两候选都探了，且 0x08..0x77 全域无应答） |
| `rd=0` + `raw=------------` | 读重试计数仅由成功读或读重试耗尽分支写入；`rd=0` ⇒ **从未进入读阶段** | 「读请求 NACK 未就绪（转换未完成）」、「CRC 错」、「量程无效」——三者都发生在读回 6 字节**之后** |
| `at=3` | `GXHT40_MEAS_RETRY = 3`，三种尝试全部用尽 | 「单次偶发」；失败每周期稳定复现 |
| `s=2` 与 `a44=a45=0` 并存 | 修订后的裁决规则：仅当**无任何候选 ACK** 时才报 `NO_DEVICE` ⇒ 合法且自洽 | EV-009 登记的 OBS-2 张力（旧形态 `s=2` + `a44=1`）本轮**未出现** |
| `idle=3` 且每周期先做有界恢复 | 稳态下缓存地址恒为 0（从未成功）⇒ 每个 `gxht40_acquire()` 都走「完整重探」分支：先 `i2c_bus_recover()`（≤9 个 SCL 脉冲 + STOP），再探两候选 | 「总线被从机拉低卡死」与「缓存地址失效导致永久失联」 |

### 3.1 本轮新增的独立排除（EV-010 未覆盖项）

1. **网表逐网复核**（`sensor_hardware/pstxnet.dat`，本轮完整读取，非摘要）：
   - `N05845` = {`R1.2`, `U7.14(PA04)`, `U9.1(SDA)`}；`N06024` = {`R2.1`, `U9.2(SCL)`, `U7.13(PA03)`} ⇒ **SDA=PA04、SCL=PA03**，与软 I²C 的 `GPIO_PIN_4 | GPIO_PIN_3` 配置一致。
   - `BAT` 网含 `J1.1`(CR2032+)、`U7.9(VDD)`、`R1.1`、`R2.2`、`C7.2`、`C13.1`、`C14.1`、`U4.VDD`、**`U9.3(VCC)`** ⇒ U9 与 MCU **同接电池、直连**，**不存在任何固件可控的供电门/使能开关**（因此「固件忘记给传感器上电」这一假设**被网表排除**）。
   - `GND` 网含 `U9.4`；`NC` 网含 `U9.5(PAD)`（未连接）。
   - GXHT40 的两条 I²C 网上**只有 U9 一个器件**（另有 R1/R2 上拉与 MCU 引脚）⇒ 板内**不存在第二个已知良好的 I²C 器件**可作对照（这是本项最小区分实验的客观限制，见 §5）。
2. **软 I²C 主机静态复核**（本轮读取 `USER/src/measure.c`、`sf_i2c.c` 与厂商库）：
   - `i2c0_phy_init()` 把 `GPIO_PIN_4|GPIO_PIN_3` 配为 `GPIO_MODE_OUTPUT_OD`；库内 `GPIO_Init()` 对 `GPIO_MODE_OUTPUT_OD` 执行 `REGBITS_CLR(DIR)` + `REGBITS_SET(OPENDRAIN)` ⇒ 是**真开漏**（不是被误配成推挽——推挽会让主机在每个 ACK 时隙强行驱动 SDA 高、从而**永远读不到 ACK**，这是本轮**特意核查**的一条能解释全域无 ACK 的假设）。
   - `i2c_wait_ack()`：`SCL` 拉低 → **把 SDA 切为 `GPIO_MODE_INPUT`** → `SCL` 拉高 → 读 SDA；持续为高 255 次后释放 SDA、切回 OD 输出并发 STOP 返回超时 ⇒ ACK 采样路径正确。
   - `GPIO_WritePin()` 经 `BSRR`/`BRR` 写输出锁存；开漏下置高=释放、置低=拉低 ⇒ 与协议语义一致。
3. **引脚所有权**：`grep` 全部 `USER/src`、`USER/inc`、`COMMON`——除 `measure.c` 外**无**任何对 PA03/PA04 的配置/复用/复位；`debug_trace.c` 仅以位域写 `AFR6`（PA06/PA05）；无 `GPIOA` 时钟关闭、无 `SYSCTRL_AHBPeriphReset` 命中。

⇒ **主机（MCU）侧在静态层面未发现任何能造成「112 个地址全域无 ACK」的缺陷**；但**这不等于已证明器件或接线损坏**——见 §5 仍开放的假设。

### 3.2 仍未闭合的环节

字节流层面的证据只能证明「**地址时隙上 SDA 恒为高**」。这在电气上同时兼容下列两种截然不同的原因，**二者在固件可观测的字节流上不可区分**：

- **A：器件侧**——U9 未贴装/损坏/虚焊/接反/被闩锁在异常态 ⇒ 无人应答；
- **B：主机侧**——PA03/PA04 虽然**能读**为高（`idle=3`），但**实际未把线拉低**（引脚驱动器/板级走线/配置层面问题）⇒ 总线上根本没有合法 START 与地址位，器件自然不应答。

EV-010 曾据「驱动/总线层已无剩余可修项」而判 BLOCKED；本轮按能力规则不据未知根因宣告外部阻塞，并把 A/B 的区分作为**交回实现的最小区分实验**（§5）。

## 4. 独立判定（本能力自建脚本，不引用实现源码）

脚本：`evidence/embedded_verify_gxht40_item002.py`（按冻结契约逐字节解析；阈值取 readme 字面值，不从被测实现推导）。本轮按 `c1,c2,c3,c4,d1,d2` 顺序传入：

```
$ python evidence/embedded_verify_gxht40_item002.py <c1> <c2> <c3> <c4> <d1> <d2>
    periods observed: 10  (q=1: 0, q=0: 10)
    longest run of consecutive q=1 periods: 0
===== CHECKS =====
    FAIL  H ACCEPT >=3 consecutive q=1 periods with t/h in domain  [longest run = 0 (needed >= 3)]
    (79 checks passed, 1 failed)
===== RESULT: FAIL =====   (exit=1)
```

**唯一失败项就是本项的核心验收**（`H ACCEPT`）；结构（行长 ≤96 B、CRLF 终止）、冻结字段与顺序、取值域、`G` 行位置与字段逐项自洽（含 `s=2 ⇔ 两候选均未 ACK`、`s∈{3,4,5} ⇒ 至少一个候选 ACK` 两条硬门禁）、节拍 `Δk==3`、失败不推进前值/不构造 0 值等 **79 项全部通过**。

## 5. 竞争假设与最小区分实验（交回实现）

**预期 / 实际**：预期每个 3 分钟周期 `S` 行 `q=1`、`t/h` 落在 −40.0..125.0 ℃ / 0..100.0 %RH 且与环境相符、连续多周期稳定；**实际** 10/10 周期 `q=0`、`t=0`、`h=0`，每周期紧跟 `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`，`BUS ack=none`。

**复现步骤（可直接复跑，无需仪器）**：① `mdk_build rebuild`（0 Error）；② `mdk_flash {}`（Verify OK）；③ 在 COM42@9600 连续 `serial_capture`（300 s），使复位落在窗口内。复现率：**3/3 复位、10/10 周期**。

**竞争假设（按现有证据的相容性列出，未按可能性排序）**

| # | 假设 | 与现有证据的相容性 | 现状 |
|---|---|---|---|
| H1 | U9 未贴装 / 虚焊 / 引脚开路（SDA、SCL、VCC、GND 任一） | 完全相容 | 未排除（需目视/通断） |
| H2 | U9 本体损坏 | 完全相容 | 未排除（需换件） |
| H3 | U9 被闩锁在异常态，需**真正断电**复位 | 完全相容；且本轮**三次都是调试器复位**，`U9.VCC` 与 MCU 同接 `BAT`（§3.1-1），**MCU 复位并不切断器件供电** ⇒ 整个会话期间 GXHT40 从未经历一次掉电上电 | 未排除（需现场断电） |
| H4 | 板上实际装配把 SDA/SCL 接反（装配 ≠ 网表） | 完全相容（器件会去拉它自认的 SDA 脚，而主机采样另一条线，ACK 恒读为高） | 未排除（需万用表/示波器） |
| H5 | **主机从未把 SDA/SCL 真正拉低**（引脚驱动器/板级走线/配置层），总线上的地址位并不成立 | 完全相容：`idle=3` 只证明两线**被读**为高、不证明 MCU **能拉低**；字节流无法区分 | **本轮新增的开放项**；静态核查（§3.1-2/3）未发现对应缺陷，但静态正确性**不能证明**硅片/板级实际行为 |
| H6 | 总线速率/时序超器件上限 | **已排除**：位延时为 `speed=100` 次 `volatile` 递减循环，位时钟远低于 100 kHz |
| H7 | 等待下限不足（tMEAS/tPU） | **已排除**：`GXHT40_MEASURE_WAIT_MS=10 ms` ≥ 手册 tMEAS.H 上限 8.3 ms；`GXHT40_POWER_ON_WAIT_MS=2 ms` ≥ tPU ≤1 ms；且失败发生在**地址字节处**，从未进入测量等待 |
| H8 | 地址变体 | **已排除**：0x08..0x77 共 112 个 7bit 地址全域无 ACK |
| H9 | 读请求 NACK / CRC 错 / 量程无效 | **已排除**：`rd=0`，从未发起读事务 |
| H10 | 总线被从机持续拉低卡死 | **已排除**：负对照（宿主 harness）证明持续拉低会得到**满表 ACK**，与实测 `ack=none` 相反 |
| H11 | 固件未给传感器上电 | **已排除**：网表证明 `U9.VCC` 直连 `BAT`，无固件可控供电门（§3.1-1） |

**最小区分实验（按成本排序；前两项无需任何仪器，也无需本能力未获授权的工具）**

1. **E1（区分 H5 与 A 类：器件侧）——主机拉低/回读自检，属实现能力的测试构建（本能力不修改产品源码）**：
   在上电诊断中，于任何 I²C 事务之前增加一次自检并打印，例如 `IOTEST sda_lo=<读回> scl_lo=<读回> idle=<读回>`：把 SDA 按 OD 拉低 → 立即读回 PA04；把 SCL 拉低 → 读回 PA03；再释放 → 读回两线。
   - 若**拉低时读回仍为 1** ⇒ 主机根本拉不低该线（引脚/走线/配置层）⇒ 落在 H5/H4 侧，**传感器不应被视为故障件**；
   - 若**拉低时读回 0、释放后读回 1** ⇒ 主机确实能拉低并能释放 ⇒ 地址位真实出现在线上 ⇒ 故障在**器件侧**（H1/H2/H3），再走 E2/E3。
   这是一次性、可复跑、约十行的改动，能一次性切掉最大的开放分支。**本能力不实施该改动**（会变更产品镜像，属实现能力）。
2. **E2（区分 H3 与 H1/H2）——现场给传感器一次真实断电复上电**：取下/断开 CR2032（或断开 U9 供电）后再上电，重新观测 `BUS`/`G`。这是唯一能让 GXHT40 真正经历一次上电复位的动作；**只需数秒**，无需仪器。执行后本能力在同一 COM42 上连续采集 ≥10 分钟（约 4 个周期）即可判定。
3. **E3（区分 H1 与 H4）——目视/通断检查 U9**：是否贴装、5 脚有无虚焊/连锡；用万用表量 `U9.1↔PA04`、`U9.2↔PA03`、`U9.3↔BAT`、`U9.4↔GND` 的通断（网表基准见 §3.1-1）。若方便，可直接换一颗 GXHT40 复测。
4. **E4（可选，需未提供的仪器）**——示波器/逻辑分析仪观察 PA03/PA04 在地址字节与第 9 个时钟（ACK 时隙）上的波形，可一次同时区分 H4 与 H5。**用户本轮未提供该仪器**，故不作为门槛；仅在前三项无法收敛时作为排除手段。

**本能力对上述假设不作结论**：现有字节流**不足以**判定是器件损坏还是主机侧拉低失效，也因此**不能**据此宣告外部阻塞（见 ROLE：未知根因的功能失败交回实现）。

## 6. 本轮未覆盖项与证据缺口（如实登记）

| 项 | 状态 | 原因 / 说明 |
|---|---|---|
| 成功周期实板形态（`q=1`、无 `G` 行、`t/h` 有效域、连续 ≥3 周期） | **未覆盖** | 器件在 0x08..0x77 全域不应答（§2.1/§3）⇒ 当前硬件上不可产生 |
| 手机端读数与该周期 `S` 行 `t/h` 为同一笔 | **未执行（前提不成立）** | 10 个周期 `r=0 s=0`，从未满足上报条件，没有任何上报可比较；该子句以「满足上报条件时」为前提。为避免与本项无关的**生产副作用**，本轮**未**安装/启动 Android App |
| 实板 `ack` 地址表 / `raw` 十六进制形态 | 未覆盖（实板） | 同上；仅由宿主 harness 覆盖（历史 harness 结果仍有效，本轮未改其断言） |
| `k=6` 这一周期 | **未采集** | 落在 c1 结束与 c2 开始之间约 59 s 的**采集工具间隙**内；两侧 `k=3`/`k=9` 均在栅格上。不影响判定：本项结论只依赖「是否存在 `q=1`」，而 10 个跨越 27 分钟、节拍自洽的观测点**全部** `q=0` |
| 硬件使能/供电门假设（H11） | **已排除** | 网表逐网复核（§3.1-1） |
| 拉低/回读自检（E1） | **未执行** | 需要变更产品镜像（新增测试打印），属实现能力；本能力不改产品源码 |
| I²C 位级电气/时序波形、供电电流、U9 焊接与供电实测 | **未覆盖** | 用户未提供逻辑分析仪/示波器/万用表/电流表；**不作为本项通过门槛**，仅在 §5 作为排除手段（E3/E4） |
| 节拍项的完整判定 | 不属本项 | 本轮仅登记观测：`Δk≡3` 且 3 拍 ≈172–177 s（RTC/LSI 容差内符合「3 分钟」）；完整判定属节拍任务项 |

## 7. 判定与交接

| 维度 | 结论 |
|---|---|
| 部署与受测件身份 | 通过：`mdk_build rebuild` 0 Error/1 既有告警、`mdk_flash` ×3 均 Verify OK、axf md5 与实现交付**逐字符一致** |
| 实板可观察（本项核心验收） | **未达成**：0/10 周期 `q=1`（3 次独立复位、~27 分钟、10 个周期的全域扫描均 `ack=none`） |
| 失败稳定性 | 失败形态在 10 个周期上**逐字节相同**、3 次复位 `BUS` 行**逐字节相同** ⇒ 稳定可复现，非偶发 |
| 结构 / 字段 / 自洽 | 通过：行长 ≤68 B、CRLF、`G` 行位置与字段逐项自洽、失败周期不上报/不推进前值/不构造 0 值（79 项独立检查全通过） |
| 主机侧静态复核 | 未发现能造成「全域无 ACK」的缺陷（真开漏配置、ACK 采样切输入、引脚所有权唯一、无固件可控供电门）；但**不构成**器件或接线无故障的证明 |
| **Outcome** | **TEST_FAIL**（功能不符、稳定可复现；按能力规则交回实现，附 §5 的 4 项最小区分实验；不据未知根因宣告外部阻塞） |

**交接**

1. **【交实现能力】** 本项的实板可观察**未达成**且**稳定复现**（§2）。请按 §5 的 **E1** 先做主机拉低/回读自检以切分「主机拉不低」与「器件不应答」；若 E1 证明主机可正常拉低/释放，则把 E1~E4 的现场动作清单交用户执行（E2 断电、E3 目视/通断/换件），本能力在条件具备后**在同一 COM42 上连续采集 ≥10 分钟**即可复判（不需要额外仪器，也不需要重新改代码）。
2. **【交用户（最小必要现场动作，无需仪器）】** ① 给传感器一次**真实断电复上电**（U9 与 MCU 同接 `BAT`，调试器复位**不等于**器件掉电；整个会话期间器件从未断电）；② 目视/万用表检查 U9 贴装与 `U9.1↔PA04`、`U9.2↔PA03`、`U9.3↔BAT`、`U9.4↔GND` 通断。两项完成后告知即可，本能力立即复采。
3. **【登记性观察，不属本项】** 会话语内光敏原始码 `a` 从 130 升到 388（§2.3）——该读数是真实物理量且通路工作正常；`LIGHT_DARK_CALIBRATED=0` 的全暗基准仍未取得，属光照判据/标定任务项。
4. **【仓库卫生】** `.gitignore` 的 `/build*.log` 已修复（EV-008 §6 的交接项已闭环）；本轮 `mdk_build`/`mdk_flash` 产生的日志被正确忽略并在读入证据后删除，`git status` 干净、产品源码零改动。

---

# 历史记录（EV-010 / run8 ITEM-002；当轮判 BLOCKED：实现缺陷已修复复验通过，实板 q=1 未达成；已提交 a668bd2）

# 嵌入式验证证据（EV-010 / run8 ITEM-002 ↔ GXHT40 温湿度采集在真实目标上恢复有效）

状态：embedded_tester.embedded_verification 独立验证证据（本轮任务项执行轮；重新构建、重新部署、重新采集真实目标字节流，不沿用历史结论）
本轮验证对象：run8 队列项 **ITEM-002**（GXHT40 温湿度采集在真实目标上恢复有效：实板每周期 `q=1`、`t/h` 有效域、连续多周期稳定）
受测提交：`aa3bcc4`（接手时工作树干净；中间提交为 `3305b68` 的 EV-009 TEST_FAIL → 本项实现轮）
受测件：`MDK/output/exe/Project.axf`，md5 **`e309bac2a34fd210ee966d6da9001625`**（由 HEAD 源码经 `mdk_build rebuild` 构建；与 `evidence/build.md` 声明的 md5 **逐字符一致** ⇒ 本轮实测的二进制就是实现交付的那一个）
验证基线：TD-002 rev 5.0 `artifacts/test_design.md`（B1/B18、T-L2-14、§13 当前构建再观测硬前提）；FWR-002 v5.0 `artifacts/firmware_requirements.md`（FWR-117/118）；FD-002 rev 5.0 `artifacts/firmware_design.md` §6.6.2/§6.6.3/§12；IC-002 v3.0 `artifacts/interface_contract.md`

结论：**BLOCKED**（上一增量的**实现缺陷已修复并独立复验通过**；但本项核心可观察在真实目标上仍**未达成**，且失败环节已用一手数据定位到**地址级完全无应答**——驱动/总线层已无剩余可修项，缺的是现场器件条件）

| # | 事实 | 性质 | 依据 |
|---|---|---|---|
| 1 | EV-009 登记的**实现缺陷已修复且独立复验通过**：FWR-118「任一失败（含 CRC 错、量程无效）后下一周期必须重探 `0x44`/`0x45` 两候选地址并在重探前做有界总线恢复」现已在**线上字节级**成立；OBS-2（`s=2` 与 `a44=1` 并存的自相矛盾）已由实现侧裁决为「任一地址 ACK ⇒ 报 `ERR_IO`」，并加了反面对照（两候选均不 ACK 仍报 `NO_DEVICE`） | 已修复，复验通过 | §4.1 |
| 2 | 实板 **0/9 个采样周期 `q=1`**（`k=0,3,6,9,12,15,18,21,24` 连续 24 分钟栅格全覆盖），9 个周期失败形态**逐字节相同**：`G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`；上电全地址扫描 `BUS idle=3 scl=1 sda=1 ack=none`。本项核心可观察（`q=1`、`t/h` 有效域、连续多周期稳定）**未达成** | **条件缺失**（需现场动作，非代码缺陷） | §2、§3 |
| 3 | 失败环节定位：`rd=0` + `at=3` + `a44=a45=0` 与「3 次整帧重测中，两个候选地址都**在地址字节处就 NACK**」逐项闭合 ⇒ 排除「读请求 NACK 未就绪」「CRC 错」「量程无效」「总线被拉低卡死」；而 0x08..0x77 共 **112** 个 7bit 地址**无一应答** ⇒ 亦排除「地址变体」 | 定位结论（指向器件/接线物理层） | §3.2、§3.3 |
| 4 | 同一采集窗口内**光照通路正常**：`o=8`（8 次 ADC 转换全部成功）、`a≈62..65`（亮态低码，与用户「当前的情况是有光的」一致）、`n/x/a` 随环境小幅变化 ⇒ MCU/ADC/PB04-PB05 物理通路工作正常，故障不属整板或时钟层 | 交叉对照（缩小故障面） | §3.4 |

> 说明：事实 1 与事实 2 是**先后两件不同的事**，不能互相掩盖。事实 1 是可由实现能力修复的实现缺陷，已在 `aa3bcc4` 修复并由本轮独立复验；事实 2 在事实 1 修复**之后**重新采集，形态与 EV-009 完全一致，说明其成因在固件之外。

---

## 1. 环境、构建与部署

| 工具 / 命令 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `COM42 = Prolific USB-to-Serial Comm Port (VID:PID 067B:2303)`；`COM26 = USB 串行设备 (C251:F001, SN 87094109484987710672FF50)` | 传感器调试口 **COM42 在位**；与配置声明的设备身份一致 |
| `usb_list_devices` | `USB\VID_C251&PID_F001`（HID MI_02 + Ports MI_00，Status OK） | **CW-DAPLink 下载探针在位** |
| `mdk_build rebuild` | `*** Using Compiler 'V6.24'`；`../USER/src/main.c(224): warning: while loop has empty body`；`Program Size: Code=16572 RO-data=624 RW-data=76 ZI-data=1676`；`0 Error(s), 1 Warning(s)` | 0 错误；1 条为**基线既有**空体 `while(k--);`。与 `evidence/build.md` 声明的 `Code=16572 RO=624 RW=76 ZI=1676` **逐项一致** |
| `md5sum MDK/output/exe/Project.axf` | `e309bac2a34fd210ee966d6da9001625` | 与实现证据声明的产物 md5 **完全一致** ⇒ 受测件身份闭合 |
| `mdk_flash {}` ×4 | `Erase Done.Programming Done.Verify OK.Application running ...` | 当前交付镜像已下载并**回读校验成功**（4 次均为 Verify OK；本轮判定不依赖其中任一次是否落在采集窗口内） |
| `sh gcc/build.sh`（GNU 交叉编译，独立构建路径） | `FLASH: 33076 B / 64 KB (50.47%)`、`RAM: 1768 B / 4 KB (43.16%)` | 与实现证据声明的 33,076 / 1,768 B 一致（较上一轮 +60 B） |
| `sh test/build_test.sh`（实现侧宿主机套件，只读复跑） | 40 + 34 + 33 + 19 + 20 = **146 passed, 0 failed** | 实现侧全套通过，与本项无回归 |

### 1.1 失败环节是否只在调试路径？（独立核查）

命令（可复跑）：

```bash
INC="-IUSER/inc -ICOMMON -IUM2005C -I../../../Libraries/inc -IMDK/RTE -I<CMSIS Core>"
for f in 0 1; do arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -O1 -Wall -Wextra \
  -DSENSOR_DEBUG_UART=$f -c $INC USER/src/gxht40.c -o /tmp/gxht40_dbg$f.o && md5sum /tmp/gxht40_dbg$f.o; done
```

| 对象 | `SENSOR_DEBUG_UART=0` | `SENSOR_DEBUG_UART=1` | 结论 |
|---|---|---|---|
| `gxht40.o` | `4599f09d97c1a274ba6f5f9f3f3919d5` | `4599f09d97c1a274ba6f5f9f3f3919d5` | **逐字节相同** ⇒ FWR-118 重探修复与 `i2c_bus_recover` 调用是**无条件产品代码**，与调试开关无关 |
| `sf_i2c.o` | `6abd895e7162942358ecfd58f71f420c` | `6abd895e7162942358ecfd58f71f420c` | **逐字节相同** ⇒ `i2c_probe_addr` / `i2c_bus_recover` 同样在常规路径中 |

`arm-none-eabi-nm gxht40.o` 的未定义符号包含 `i2c_bus_recover` ⇒ 有界总线恢复确实进入产品镜像（不是仅存在于宿主 harness）。

## 2. 真实目标 UART1 观测（COM42 @9600 8N1，原始字节）

采集方式：`serial_capture`（单次上限 300 s，采样周期 180 s），本能力把 `mdk_flash` 与采集**串接在同一 shell 脚本**中，使复位横幅与 `BUS` 行落在窗口内（`work/ev_item002r/run.sh`、`cap.sh`）。原始捕获文件在 Runtime 角色工作目录 `work/verification_tools/serial-capture-*.bin`，本轮副本 `work/ev_item002r/r1..r7_*.bin`。

| 捕获 | 文件 | 窗口（UTC） | 字节 | 内容 |
|---|---|---|---|---|
| r1 | `serial-capture-1791437866780288900.bin` | 05:32:46.576 → 05:37:46.781 | 224 | `S k=3`+`G`、`S k=6`+`G` |
| r2 | `serial-capture-1791438195996982100.bin` | 05:38:15.802 → 05:43:15.997 | 112 | `S k=3`+`G` |
| **r3** | `serial-capture-1791438627481835600.bin` | 05:45:27.282 → 05:50:27.482 | **357** | **复位横幅 ×2 + `BUS`** + `S k=0`+`G` + `S k=3`+`G` |
| r4 | `serial-capture-1791438944799540300.bin` | 05:50:44.601 → 05:55:44.800 | 224 | `S k=6`+`G`、`S k=9`+`G` |
| r5 | `serial-capture-1791439323456979400.bin` | 05:57:03.251 → 06:02:03.458 | 226 | `S k=12`+`G`、`S k=15`+`G` |
| r6 | `serial-capture-1791439623973428600.bin` | 06:02:03.761 → 06:07:03.973 | 226 | `S k=18`+`G`、`S k=21`+`G` |
| r7 | `serial-capture-1791439924472062400.bin` | 06:07:04.265 → 06:12:04.480 | 113 | `S k=24`+`G` |

> r3 的两次复位横幅：`rst=0040` 与 `rst=0240` 是非零且随复位来源变化的两个值（`mdk_flash` 的调试器复位落在同一窗口内）；`BUS` 行在横幅之后、任何 `S`/`G` 之前，恰 1 条。

**字节账目（逐字节闭合，无未知字节）**

| 捕获 | 行构成 | 账目 |
|---|---|---|
| r3 | 48+2 48+2 31+2 64+2 44+2 64+2 44+2 | = **357** ✓ |
| r1 / r4 | 64+2 44+2 64+2 44+2 | = **224** ✓ |
| r5 / r6 | 65+2 44+2 65+2 44+2 | = **226** ✓ |
| r2 / r7 | 64/65+2 44+2 | = **112 / 113** ✓ |

全部行以 CRLF 终止、行长 ≤ 96 B（最长 65 B）、行首仅 `BOOT`/`BUS`/`S`/`G`，无截断残行、无裸 LF。

`r3` 原始字节（完整）：

```
BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600\r\n
BOOT fw=FD-002r4 uid=6A002C00 rst=0240 uart=9600\r\n
BUS idle=3 scl=1 sda=1 ack=none\r\n
S k=0 p=0 H=0 V=0 o=8 n=61 x=64 a=62 D=0 t=0 h=0 q=0 r=0 s=0 y=0\r\n
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------\r\n
S k=3 p=0 H=0 V=0 o=8 n=62 x=65 a=63 D=0 t=0 h=0 q=0 r=0 s=0 y=0\r\n
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------\r\n
```

## 3. 独立判定（本能力自建脚本，不引用实现源码）

脚本：`evidence/embedded_verify_gxht40_item002.py`（按冻结契约逐字节解析；阈值取 readme 字面值，不从被测实现推导）。本轮按 `r1..r7` 顺序传入：

```
$ python evidence/embedded_verify_gxht40_item002.py work/ev_item002r/r1_133746.bin ... r7_141204.bin
    periods observed: 12  (q=1: 0, q=0: 12)
    longest run of consecutive q=1 periods: 0
===== CHECKS =====
    FAIL  H ACCEPT >=3 consecutive q=1 periods with t/h in domain  [longest run = 0 (needed >= 3)]
    (96 checks passed, 1 failed)
```

**唯一失败项就是本项的核心验收**（`H ACCEPT`）；结构、字段、取值域、`G` 行位置与自洽、节拍、失败不推进前值等 96 项全部通过。

### 3.1 逐周期观测（`k` 为 1 分钟 RTC 节拍，每 3 拍一次采样）

| 捕获 | `k` | `q` | `t` | `h` | `G` 行 |
|---|---|---|---|---|---|
| r1 | 3 | 0 | 0 | 0 | `s=2 a44=0 a45=0 rd=0 at=3 raw=------------` |
| r1 | 6 | 0 | 0 | 0 | 同上（逐字节相同） |
| r2 | 3 | 0 | 0 | 0 | 同上 |
| r3 | 0 | 0 | 0 | 0 | 同上 |
| r3 | 3 | 0 | 0 | 0 | 同上 |
| r4 | 6 | 0 | 0 | 0 | 同上 |
| r4 | 9 | 0 | 0 | 0 | 同上 |
| r5 | 12 | 0 | 0 | 0 | 同上 |
| r5 | 15 | 0 | 0 | 0 | 同上 |
| r6 | 18 | 0 | 0 | 0 | 同上 |
| r6 | 21 | 0 | 0 | 0 | 同上 |
| r7 | 24 | 0 | 0 | 0 | 同上 |

- **节拍**：`r3` 的 `k=0` 与 `k=3` 同一次采集内 Δk=3；`r3→r4→r5→r6→r7` 的 `k` 为 `0,3 | 6,9 | 12,15 | 18,21 | 24`，**连续无缺**，跨 4 个采集窗口、24 分钟真实时间，全部落在以复位为锚的 180 s 栅格上。
- **失败周期语义**：`r=0`（未满足上报条件）、`s=0`（未发送）、`y=0`（无重试）、`p=0`（前一有效温度从未推进）、`V=0`（未标定 ⇒ 光照不足以证明「无光」）、`q=0`。9 个周期一致，**未出现「不上报却上报」、「失败却构造 0 值并上报」或「失败推进前值」**。

### 3.2 上电总线身份（`BUS` 行）

`BUS idle=3 scl=1 sda=1 ack=none`（解码：`idle` 位掩码 bit0=SCL、bit1=SDA，`1`=高）：

- `idle=3` ⇒ **两条线在空闲时都被上拉为高**，PA04(SDA)/PA03(SCL) 的**上拉与引脚读回通路均正常**（与网表一致：U9 pin1=SDA→PA04、U9 pin2=SCL→PA03，各带一只上拉电阻）。
- `ack=none` ⇒ `i2c_probe_addr` 对 **0x08..0x77 全部 112 个 7bit 地址**（起始/结束地址见 `sensor_config.h`）**一个 ACK 都没有**。
- 独立对照（宿主端，本轮复跑）：`host_diag_probe_verify_ev.c` 的 **S6 负对照**证明「若 SDA 被持续拉低，扫描会得到满表 ACK（8 + `+`）」⇒ 实板的 `ack=none` **不是**由 SDA 卡低造成的，而是真的没有任何器件应答。

### 3.3 失败环节定位（本项要求「定位失败环节」）

把 `r3` 之后每个周期的 `G` 行与当前驱动代码路径逐项对齐：

| 观测 | 闭合到的代码事实 | 排除的环节 |
|---|---|---|
| `a44=0 a45=0` | `gxht40_diag_note_ack()` 只在 `i2c_write_cmd()`（地址字节）成功后才置位；两位皆 0 ⇒ 3 次重测中 `0x44`/`0x45` 的地址字节都是 NACK | 排除「地址变体」（两个候选都探了，且 0x08..0x77 全域无应答） |
| `rd=0` | `s_diag.read_retry` 仅由 `gxht40_diag_note_read()`（成功读）或读重试耗尽分支写入；`rd=0` + `raw_valid=0` ⇒ **从未进入读阶段**（地址字节即失败，`start_and_read` 在 `i2c_write_cmd` 处返回） | 排除「读请求 NACK 未就绪（转换未完成）」、「CRC 错」、「量程无效」 |
| `at=3` | `GXHT40_MEAS_RETRY = 3`，三种尝试全部用尽 | 排除「单次偶发」；失败是**每周期稳定复现** |
| `raw=------------` | `raw_valid=0` 时按契约输出 12 个 `-` 占位 | 与 `rd=0` 自洽（没有成功读，故无 6 字节可报） |
| `s=2` 与 `a44=a45=0` 并存 | 修复后的裁决规则：仅当**无任何候选 ACK** 时才报 `NO_DEVICE`；本轮 `ack=0/0` ⇒ 合法且自洽 | 排除 OBS-2 张力（旧形态 `s=2` + `a44=1` 本轮**未出现**） |
| `idle=3` 且每周期先做有界恢复 | 稳态下缓存地址恒为 0（从未成功），每个 `gxht40_acquire()` 都走「完整重探」分支：先 `i2c_bus_recover()`（≤9 个 SCL 脉冲 + STOP），再探两候选 | 排除「总线被从机拉低卡死」与「缓存地址失效导致永久失联」 |

**结论**：本次失败的唯一环节是「**两个候选地址（以及全域任何地址）在地址字节处完全不应答**」。该环节按本项正文的分类属「器件不应答」一类，**在驱动/总线层已无剩余可修项**：候选地址已两两重探、重探前已有界恢复、等待下限满足手册（`GXHT40_MEASURE_WAIT_MS=10 ms` ≥ 手册 tMEAS.H 上限 8.3 ms）、命令白名单仍只用 `0xFD`（软复位 `0x94`/加热命令从未出现）。

### 3.4 交叉对照：同一窗口内光照通路正常

| 字段 | 9 个周期实测 | 含义 |
|---|---|---|
| `o` | **恒为 8** | 每次采样的 8 次 AIN11 转换全部成功 ⇒ ADC 与 PB04/PB05 通路工作 |
| `n / x / a` | `61..67 / 64..67 / 62..65` | ADC 原始码均值约 62..65（亮态低码，符合 `code = 4095*R_photo/(R_photo+5M)` 的亮态特性），且随环境小幅起伏 |
| `V` | 恒为 0 | 未标定（`LIGHT_DARK_CALIBRATED=0`）⇒ 不足以证明「无光」，判据门禁未被绕过 |
| `D` | 恒为 0 | 与 `V=0` 自洽 |

用户本轮说明「当前的情况是有光的」，与「亮态低码」一致 ⇒ **该读数是真实物理量**，可佐证 MCU、时钟、ADC 与外设通路正常，故障被进一步限定在 GXHT40 及其 I2C 连接上（而非整板/时钟层）。

## 4. 实现符合性独立复验（事实 1 的证据）

### 4.1 FWR-118 重探义务 + OBS-2 结果码自洽（本能力自建 harness，链接真实驱动）

`test/host_gxht40_verify_ev.c`（本能力所有；位级 mock 总线 + mock GXHT40 器件 + `-Wl,--wrap=i2c_bus_recover` 链接期拦截，期望帧由独立参考脚本给出）：

```bash
gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast -Itest/mock_mcu -IUSER/inc -ICOMMON \
    test/host_gxht40_verify_ev.c USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c \
    -Wl,--wrap=i2c_bus_recover -o _evbin/gxht40_verify.exe && ./_evbin/gxht40_verify.exe
=> ==== result: 93 passed, 0 failed ====
```

| 用例 | 断言（本轮实测） | 结果 |
|---|---|---|
| **T17**（CRC 错） | CRC 错周期报 `ERR_CRC` 且**不修改输出**；失败后 `gxht40_detected_addr7()==0x00`（缓存被失效）；下一周期**同时**探 `0x88` 与 `0x8A`；恢复调用在**线上第一个重探字节之前**（`RECOV` 事件序号 < 首个 `TX(88)`）；`recovery_calls>=1` | **PASS**（EV-009 时该项为 2 项失败之一） |
| **T17b**（量程无效） | 同上（`ERR_RANGE` 分支） | **PASS** |
| **T13** | 恢复调用序号 4 < 首个 `0x88` 探针序号 6 < `0x8A` 序号 10；单次失败恰调用 1 次恢复 | PASS |
| **T15（已改判为硬门禁）** | 恰一个候选 ACK 而读不通 ⇒ `r == GXHT40_ERR_IO` 且 `diag.status == ERR_IO`（**不再是** `s=2` 与 `a44=1` 并存）；**反面对照**：两候选均不 ACK ⇒ 仍报 `GXHT40_ERR_NO_DEVICE`（证明规则不是「一律 `ERR_IO`」）；两情形输出均不被修改 | PASS（含新增反面对照） |
| **T16** | 全程命令白名单：47 次命令字节**全部为 `0xFD`**，软复位 `0x94` 从未出现 | PASS |
| **T14** | `tPU` 仅首次访问 1 次；`tMEAS` 仍 1 次 10 ms（≥ 手册上限 8.3 ms） | PASS |

> 说明：T15 在 EV-009 中作为「OBS-2 登记性观察」编码的是**旧形态**（断言 `s=2` 与 `a44=1` 并存）。`aa3bcc4` 依本能力 EV-009 的 OBS-2 裁决改变了该形态，故本轮把 T15 由「观察」升级为**双向硬门禁**（正向 + 反面对照），并非放宽或删除断言；T17/T17b 另增「失败后缓存地址必须为 0」「恢复调用先于重探」两条断言。

### 4.2 其余独立 harness（只读复跑，未改源码）

| harness | 覆盖 | 结果 |
|---|---|---|
| `host_diag_probe_verify_ev.c` | 真实 `measure.c` + 真实 `gxht40.c`/`sf_i2c.c`/`fw_core.c`：`i2c_probe_addr` 线上字节、0x08..0x77 共 112 次扫描（无命令/无读/无业务态改动）、8 地址截断、诊断快照 5 类结果码、失败周期快照传播、**S6 SDA 卡低负对照** | **85 passed, 0 failed** |
| `host_sf_i2c_verify_ev.c` | 位级原语回归 + `i2c_bus_recover` 独立模型（空闲不产时钟 / 持续拉低恰 9 脉冲 + 1 个合法 STOP / 提前释放则提前停） | **51 passed, 0 failed** |
| `host_diag_line_verify_ev.c` | `BUS`/`G`/`S` 行最坏宽度与 96 B 预算、`raw` 12 位 hex / 12 个 `-`、关闭语义 | **36 passed, 0 failed** |
| `host_measure_flow_verify_ev.c` | 真实 `measure.c` 流程参考模型（14 周期逐步比对、光照先于温湿度、失败不推进前值/不构造 0 值） | **79 passed, 0 failed**（本轮修复后，见 §5） |
| 实现侧 `host_gxht40_check.c` | 供对照（不属于本能力的判定依据） | 60 passed, 0 failed |

## 5. 本轮对本能力测试资产的维护（非产品代码改动）

| 文件 | 改动 | 原因 |
|---|---|---|
| `test/host_gxht40_verify_ev.c` | T15 由「观察」改为双向硬门禁；T17/T17b 增 3 条断言（失败后缓存=0、恢复先于重探、`recovery_calls>=1`） | 被测语义在 `aa3bcc4` 依 EV-009 的 OBS-2 裁决变化；断言须跟随**新的**契约，同时用反面对照防止判据被放宽 |
| `test/host_measure_flow_verify_ev.c` | 修复长期无法编译的问题：`light_sample()` 桩改为返回 `light_result_t`（T2 起签名变更，measure.c 只消费 `.dark`）、补 `delay_ms` 计数桩与 `gxht40_diag_fetch` 桩 | 该 harness 自 T2 起因签名漂移而无法编译（`file_manifest.txt` 原已登记为既有问题），本轮恢复其回归覆盖；未放宽任何断言 |
| `evidence/embedded_verify_gxht40_item002.py` | 把原「I OBS 登记性观察」升级为 E 项硬门禁：`s=2` ⇔ 两候选均未 ACK；`s∈{3,4,5}` ⇒ 至少一个候选 ACK | 同一原因：OBS-2 已由实现侧裁决，张力不再允许出现 |
| `evidence/test.md` | 追加本节（EV-010） | 本轮证据 |

以上均**未修改任何产品源码**（`git status` 中的产品文件零改动）。

## 6. 本轮未覆盖项与证据缺口（如实登记）

| 项 | 状态 | 原因 / 说明 |
|---|---|---|
| 成功周期实板形态（`q=1`、无 `G` 行、`t/h` 有效域、连续 ≥3 周期） | **未覆盖** | 器件在 0x08..0x77 全域不应答（§3.2/§3.3）⇒ 当前硬件上不可产生 |
| 手机端读数与 `S` 行 `t/h` 为同一笔 | **未执行（无触发条件）** | 9 个周期 `r=0 s=0`，从未满足上报条件，没有任何上报可比较；该子句以「满足上报条件时」为前提，本轮前提不成立。未安装/启动 Android App 以免产生与本报文无关的副作用 |
| 实板 `ack` 地址表 / `raw` 十六进制形态 | 未覆盖（实板） | 同上；仅由宿主 harness 覆盖（§4.2） |
| `SENSOR_DEBUG_UART=0` 复核 | 本轮已做（编译级，§1.1） | `gxht40.o`/`sf_i2c.o` 两种开关下逐字节相同 |
| I²C 位级电气/时序波形、供电电流、U9 焊接与供电实测 | **未覆盖** | 用户未提供逻辑分析仪/示波器/电流表；**不作为本项通过门槛**，仅在 §7 作为排除「板上 SDA/SCL 接反」等物理原因时的可选手段 |

## 7. 判定汇总与所需现场条件

| 维度 | 结论 |
|---|---|
| 部署与受测件身份 | 通过：`mdk_build rebuild` 0 Error/1 既有告警、`mdk_flash` ×4 均 Verify OK、axf md5 与实现声明逐字符一致 |
| 实现符合性（EV-009 的实现缺陷） | **已修复并复验通过**：FWR-118 重探义务（含 CRC/量程分支）与 OBS-2 结果码自洽在线上字节级成立；CRC/有效域/重试上限/命令白名单均未被放宽 |
| 实板可观察（本项核心验收） | **未达成**：0/9 周期 `q=1`（连续 24 分钟栅格全覆盖），失败环节定位为「地址级完全无应答」 |
| 节拍 / 结构 / 自洽 | 通过：Δk=3 且连续、行长 ≤96 B 且 CRLF、`G` 行位置与字段逐项自洽、失败周期不上报/不推进前值/不构造 0 值 |
| **Outcome** | **BLOCKED**（缺现场器件条件；驱动/总线层已无剩余可修项，返工实现不会改变结果） |

**需要的条件（最小必要现场动作，按顺序；前两步无需仪器）**

1. **给传感器断电复上电**：取下再装回 CR2032（网表：U9 `VCC` 与 MCU `VDD` 同接 `BAT`）。目的是让 GXHT40 彻底上电复位一次——这是唯一无需工具、可立即执行的动作。
2. **目视/放大检查 U9（GXHT40）**：是否在板、5 个引脚是否虚焊/连锡（`pin1=SDA`、`pin2=SCL`、`pin3=VCC(BAT)`、`pin4=GND`、`pin5=PAD=NC`），重点确认 `pin3` 供电与 `pin1/pin2` 的 I²C 连接。
3. 若第 1、2 步后仍无应答：需要示波器/逻辑分析仪观测 **PA04(SDA)/PA03(SCL)** 在地址字节与第 9 个时钟（ACK 时隙）上的波形，用以区分「器件损坏/未供电」与「板上 SDA↔SCL 接反」——**这两种原因在固件可观测的字节流上不可区分**（接反时器件会去拉其 `SDA` 脚，而主机采样的是另一条线，AC 恒读为高，表现为与「器件不存在」完全相同的 `ack=none`）。用户未提供仪器，故不将其列为通过门槛；仅在需要时作为排除手段。
4. 任一条件改善后：本能力可在同一 COM42 上**连续采集 ≥10 分钟**（S 行每 3 分钟一条）即可判定「连续 ≥3 个周期 `q=1`」。这是最小观测窗口，不需要固定样本数门槛，也不需要额外仪器。

在这四个条件中，只有第 1、2 步可立即执行；第 3 步为可选排除手段；第 4 步是本能力在条件具备后的复验动作。

---

# 历史记录（EV-009 / run8 ITEM-002；当轮判 TEST_FAIL：发现 FWR-118 重探义务未满足，并登记器件不应答的条件缺失；已提交 3305b68）

# 嵌入式验证证据（EV-009 / run8 ITEM-002 ↔ GXHT40 温湿度采集在真实目标上恢复有效）

状态：embedded_tester.embedded_verification 独立验证证据（本轮任务项执行轮；重新构建、重新部署、重新采集真实目标字节流，不沿用历史结论）
本轮验证对象：run8 队列项 **ITEM-002**（GXHT40 温湿度采集在真实目标上恢复有效：实板每周期 `q=1`、`t/h` 有效域、连续多周期稳定）
受测提交：`10219a5`（HEAD，接手时工作树干净）
受测件：`MDK/output/exe/Project.axf`，md5 **`196b0040d5378eaf2edcd6a3d5fdf9a5`**（由 HEAD 源码经 `mdk_build rebuild` 构建；与实现记录声明的 md5 **逐字符一致** ⇒ 本轮实测的二进制就是实现交付的那一个）
验证基线：TD-002 rev 5.0 `artifacts/test_design.md`（B1/B16/B18、T-L2-14、T-L0-13、T-L2-11/12、§13 当前构建再观测硬前提）；FWR-002 v5.0 `artifacts/firmware_requirements.md`（FWR-117/118）；FD-002 rev 5.0 `artifacts/firmware_design.md` §6.6.2/§6.6.3/§12

结论：**TEST_FAIL**（两个相互独立的事实，须分别处置）

| # | 事实 | 性质 | 依据 |
|---|---|---|---|
| 1 | 实板 **0/6 个采样周期 `q=1`**；6 个周期全部 `q=0` 且失败形态逐字节相同（`G s=2 a44=0 a45=0 rd=0 at=3`），上电全地址扫描 `ack=none`。本项核心可观察（`q=1`、`t/h` 有效域、连续多周期稳定、手机端同一笔读数）**未达成**。一手证据指向**器件在地址级完全不应答**（非固件可修）⇒ 该项本身属**条件缺失**，需要现场动作，见 §5 | 环境/硬件条件（缺条件，非代码缺陷） | §2、§3、§4.1 |
| 2 | 交付实现**未满足本项验收正文中的重探义务**：FWR-118 / FD-002 §6.6.3(4) / TD-002 B18 要求在**任何**测量失败（正文明确列举「含缓存地址读失败、**CRC 错**、读 NACK 用尽」）后，下一周期必须重探 `0x44`/`0x45` **两个**候选地址；实测在**缓存地址 CRC 错**与**量程无效**后缓存地址仍被保留，下一周期只探 `0x88`（`0x8A=0`），且未调用有界总线恢复。属**已证实的实现不符合**，与实现记录「任一失败即失效缓存地址」的表述不一致 | 实现缺陷（可由实现能力修复） | §4.2 |

> 说明：事实 1 不能用「实现有缺陷」解释，事实 2 也不能用「器件不应答」解释；两条证据链彼此独立，不能以任一项掩盖另一项。

---

## 1. 环境、构建与部署

| 工具 / 命令 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `COM42 = Prolific USB-to-Serial Comm Port (VID:PID 067B:2303)`；`COM26 = USB 串行设备 (C251:F001)` | 传感器调试口 **COM42 在位** |
| `usb_list_devices` | `USB\VID_C251&PID_F001`（HID MI_02 + Ports MI_00，Status OK，SN `87094109484987710672FF50`） | **CW-DAPLink 下载探针在位** |
| `mdk_build rebuild` | `*** Using Compiler 'V6.24'`；`../USER/src/main.c(224): warning: while loop has empty body`；`Program Size: Code=16488 RO-data=624 RW-data=76 ZI-data=1676`；`0 Error(s), 1 Warning(s)` | 0 错误；1 条为**基线既有**空体 `while(k--);`。与实现记录声明的 `Code=16488 RO=624 RW=76 ZI=1676` **逐项一致** |
| `md5sum MDK/output/exe/Project.axf` | `196b0040d5378eaf2edcd6a3d5fdf9a5` | 与 `artifacts/firmware_implementation.md` 声明的 T2 产物 md5 **完全一致** ⇒ 受测件身份闭合 |
| `mdk_flash {}` | `Erase Done.Programming Done.Verify OK.Application running ...` / `Flash Load finished at 12:48:14` | 当前交付镜像已下载并**回读校验成功**；复位发生在 04:48:14Z（落在 cap1 窗口内） |

## 2. 真实目标 UART1 观测（COM42 @9600 8N1，原始字节）

采集方式：先用 `serial_capture`（`duration_sec` 上限 300，而采样周期为 180 s）开窗口，再把 `mdk_flash` 或后续采样周期落在窗口内。原始捕获文件在 Runtime 角色工作目录 `work/verification_tools/`（`serial-capture-*.bin`）：

| 捕获 | 文件（后缀） | 窗口（UTC） | 字节 | 内容 |
|---|---|---|---|---|
| **cap1** | `1791435183558853000.bin` | 04:48:03.355 → 04:53:03.560 | **357** | 复位横幅 ×2 + `BUS` + `S k=0`+`G` + `S k=3`+`G` |
| **cap2** | `1791436238655483000.bin` | 05:05:38.453 → 05:10:38.657 | **226** | `S k=18`+`G` + `S k=21`+`G` |
| **cap3** | `1791436539549883700.bin` | 05:10:39.343 → 05:15:39.551 | **226** | `S k=24`+`G` + `S k=27`+`G` |

**字节账目（逐字节闭合，无未知字节）**

- cap1 = 50 + 50 + 33 + 66 + 46 + 66 + 46 = **357** ✔（两条横幅分别为 `rst=0040`、`rst=0240`）
- cap2 = (67 + 46) × 2 = **226** ✔
- cap3 = (67 + 46) × 2 = **226** ✔

窗口内**除此之外没有任何字节**（采样间期 UART1 已关闭、无输出）。

### 2.1 原始行表（脚本 `evidence/embedded_verify_gxht40_item002.py` 输出）

```text
cap1:
BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600
BOOT fw=FD-002r4 uid=6A002C00 rst=0240 uart=9600
BUS idle=3 scl=1 sda=1 ack=none
S k=0 p=0 H=0 V=0 o=8 n=58 x=61 a=59 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=3 p=0 H=0 V=0 o=8 n=57 x=59 a=58 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------

cap2:
S k=18 p=0 H=0 V=0 o=8 n=60 x=62 a=60 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=21 p=0 H=0 V=0 o=8 n=60 x=62 a=60 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------

cap3:
S k=24 p=0 H=0 V=0 o=8 n=60 x=62 a=61 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=27 p=0 H=0 V=0 o=8 n=61 x=64 a=62 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
```

### 2.2 采样节拍与时间栅格（独立重算，不引用实现常量）

以复位时刻 04:48:14.97Z 为锚点（`k=0` 在复位后首个主循环即采样），按「1 拍 = 1 分钟、每 3 拍采样」推算：

| 周期 | `k` | 推算时刻（UTC） | 实际落入窗口 | 结论 |
|---|---|---|---|---|
| 1 | 0 | 04:48:15 | cap1 [04:48:03, 04:53:03] | ✔ |
| 2 | 3 | 04:51:15 | cap1 | ✔ |
| 3 | 18 | 05:06:15 | cap2 [05:05:38, 05:10:38] | ✔ |
| 4 | 21 | 05:09:15 | cap2 | ✔ |
| 5 | 24 | 05:12:15 | cap3 [05:10:39, 05:15:39] | ✔ |
| 6 | 27 | 05:15:15 | cap3 | ✔ |

- 同一次捕获内 Δk = 3（`18→21`、`24→27`），跨捕获 `21→24` 亦为 3 ⇒ **采样周期恰为 3 个 1 分钟节拍（180 s）**成立。
- 6 个观测点全部落在以复位为锚的 180 s 栅格上（会话跨度 27 分钟），RTC 1 分钟节拍与「每 3 拍采样」均与设计一致。

## 3. 一手失败形态与自洽性

脚本 `evidence/embedded_verify_gxht40_item002.py` 对三份原始字节流独立解析：**42 项检查通过、1 项失败**（唯一失败项即本项核心验收 `q=1`，见 §4.1）。

| 检查组 | 结果 | 说明 |
|---|---|---|
| A 结构 | 通过 | 所有完整行以 CRLF 终止；行长 `BOOT`=50 B、`BUS`=33 B、`S`=66/67 B、`G`=46 B，全部 ≤96 B；行首仅 `BOOT`/`BUS`/`S`/`G` |
| B 字段 | 通过 | `S` 行 15 个冻结字段齐全且顺序为 `k,p,H,V,o,n,x,a,D,t,h,q,r,s,y`；`G` 行为 `s,a44,a45,rd,at,raw`；全部为整数（`raw` 为 12 位大写 hex 或 12 个 `-`） |
| C 取值域 | 通过 | `q∈{0,1}`、`H/V/D/r∈{0,1}`、`s∈{0,1,2}`；`q=1` 的域检查本轮不适用（0 个 `q=1` 样本） |
| D `G` 行位置 | 通过 | 6/6 个 `q=0` 周期**紧跟恰 1 条** `G`；本轮无 `q=1` 周期，故「成功周期无 `G`」形态在实板未被触发，仅由宿主 harness 覆盖 |
| E `G` 自洽 | 通过 | 6 条均为 `s=2 a44=0 a45=0 rd=0 at=3 raw=------------`：结果码「无器件」与**两个候选地址均未 ACK**一致；`rd=0` 说明**从未发起读事务**（失败在 CRC/量程/读时序层之前）；`raw` 用 12 个 `-` 占位（与 6 字节 hex 位宽一致） |
| F 节拍 | 通过 | 见 §2.2 |
| G 前值推进 | 通过 | 6 个 `q=0` 周期后下一行 `p` 始终为 0 且 `H=0` —— **失败周期未推进前一有效温度**（本就从未存在有效温度），亦未构造 0 值上报（`r=0 s=0`） |
| H 验收 | **失败** | 连续 `q=1` 周期数 = **0**（要求 ≥3） |

### 3.1 为什么可以判定失败发生在「器件不应答」层（而非固件可控层）

以下逐条排除固件侧可控因素；★ 为本能力本轮**独立复核**所得（非引用实现自述）：

| 候选环节 | 排除依据 | 来源 |
|---|---|---|
| 引脚接错/地址线交换 | ★ 网表：`N05845` = `R1.2` + `U7.PA04` + `U9.1(SDA)`；`N06024` = `R2.1` + `U9.2(SCL)` + `U7.PA03` ⇒ **SDA=PA04、SCL=PA03**，与固件软 I²C 的 `GPIO_PIN_4|GPIO_PIN_3` 开漏配置一致，无交换 | `sensor_hardware/pstxnet.dat` + `USER/src/measure.c` (`i2c0_phy_init`) |
| 上拉/供电缺失或总线被拉低 | 实板 `BUS idle=3 scl=1 sda=1` ⇒ 释放后两线均被外部上拉拉高（R1/R2 4.7K→BAT 有效），既非缺上拉也非被拉低；★ `U9.4`=GND、`U9.3`=VCC（含 C12 去耦） | cap1 原始行 + 网表 |
| 地址变体未覆盖 | 上电扫描覆盖 `0x08..0x77` 共 112 个地址，**全部无 ACK**（`ack=none`）；两个候选 `0x44`/`0x45` 都在范围内 | cap1 `BUS` 行 |
| CRC / 有效域 / 读时序 | `rd=0` ⇒ 地址写字节即被 NACK，**根本没有进入读事务**，CRC 与量程层不可能成为原因 | `G` 行 6/6 |
| 总线卡死（SDA 长期被从机拉低） | ★ 负对照：SDA 持续拉低会得到**满表 ACK（8 地址 + `+`）**，与实测 `ack=none` 相反 | `test/host_diag_probe_verify_ev.c` S6；本轮 `test/host_sf_i2c_verify_ev.c` T10 |
| 总线速率过高（超器件上限） | ★ 位延时为 `i2c_dev.speed = 100` 次 `volatile` 递减循环（`sf_i2c.c: i2c_delay`），位时钟远低于 100 kHz；GXHT40/SHT40 无最低 SCL 频率要求，**不存在过速** | `USER/src/measure.c`、`USER/src/sf_i2c.c` |
| 上电余量不足 | ★ `GXHT40_POWER_ON_WAIT_MS = 2` ms ≥ 手册 `tPU ≤ 1 ms`，且仅首次访问一次；后续周期距上电已数百秒 | `sensor_config.h`、本轮 `host_gxht40_verify_ev.c` T14 |
| 复位/唤醒未恢复外设 | 复位来源 `rst=0040`（PIN）与 `rst=0240`（PIN|SYSRESETREQ）**均非 0 且随复位原因变化**（D-ITEM001-2 修复在**当前构建**继续成立）；软 I²C 首周期即可工作（`o=8`） | cap1 两条 `BOOT` 行 |
| 光照通路/采样流程异常 | 每周期 `o=8`（8 次 AIN11 转换全部成功），`n/x/a` 在 57..64 的合理亮态区间逐周期微变；失败不中断采样流程 | 6/6 个 `S` 行 |

⇒ 失败发生在**「GXHT40 器件是否在总线上应答」这一电气层**：总线空闲电平正常、112 个地址全覆盖无应答、从未发起读事务。**固件无法使一个不应答的器件应答**；本项亦禁止用软复位 `0x94`/加热命令或放宽校验来「凑」出应答。

## 4. 判定

### 4.1 本项核心可观察：未达成（条件缺失）

| 验收点（任务文本「可观察」） | 实测 | 判定 |
|---|---|---|
| 每个 3 分钟周期 `S` 行 `q=1` | **0/6 个周期**（`q=0` ×6） | **未达成** |
| `t/h` 落在有效域（−40.0..125.0 ℃、0..100.0 %RH）且与环境相符 | 无有效读数可判（`q=0`，`t/h` 保持 0 并由 `q=0` 标记，未上报） | **未达成** |
| 连续多个周期**稳定** | 失败形态在 6 个周期上**逐字节相同**、节拍稳定（Δk=3）——稳定的是**失败**，不是有效读数 | **未达成** |
| 满足上报条件时手机端读数与该周期 `S` 行 `t/h` 为同一笔 | **不可达**：6 个周期 `r=0 s=0`（无待上报、无发送尝试）⇒ 本次没有任何可比较的上报；且升温分支还需有效光照（`V=0`，`LIGHT_DARK_CALIBRATED=0`，属 T3），判定方向仍为 as-built 降温语义（D-01，属 T4）。为避免无证据价值的**生产副作用**，本轮**未**启动网关/手机 App | 未执行（不可达，原因已列） |

「不得用宿主机 mock、桩或历史日志代替本轮实板观测」已遵守：§2 全部为本轮 04:48–05:15Z 的 COM42 原始字节；宿主 harness 仅用于**补充实板当前无法出现的形态**（§4.2），不充当本项实板结论。

### 4.2 实现不符合：FWR-118「任何失败后重探两候选地址」未被满足

**要求原文（三处一致）**

- FWR-118：`任一测量失败（含缓存地址读失败、CRC 错、读 NACK 用尽）后，下一周期必须重新探测 0x44/0x45 并先执行有界总线恢复（如 9 个 SCL 脉冲 + STOP）`
- FD-002 §6.6.3 规则 4：`任何失败后必须重新探测两个候选地址（不得因缓存地址让器件永久失联），并做有界总线恢复`
- TD-002 B18 / T-L0-13 通过判定：`每次失败后均重探两地址`

**实测（受测提交 `10219a5`，本能力独立 harness，线上字节为证）**

| 场景 | 线上探测集合 | 有界恢复调用 | 判定 |
|---|---|---|---|
| 缓存地址**无器件/读用尽**（`ERR_NO_DEVICE` / `ERR_IO`） | `0x88=1 0x8A=1`（两候选都重探） | 1 次 | **符合** ✔ |
| 缓存地址 **CRC 错**（`ERR_CRC`） | `0x88=1 0x8A=0`（**只探缓存地址**） | **0 次** | **不符合** ✘ |
| 缓存地址**量程无效**（`ERR_RANGE`） | `0x88=1 0x8A=0`（**只探缓存地址**） | **0 次** | **不符合** ✘ |

原始 stdout（`test/host_gxht40_verify_ev.c` T17）：

```text
[T17] FWR-118: after a CRC failure the next period must re-probe both candidates
    PASS  CRC-error period is reported as GXHT40_ERR_CRC
    PASS  outputs NOT modified on the CRC-error period
    cached address after the CRC failure = 0x44
    transcript[T17-crc]: START TX(88) ACK ACK STOP START TX(89) ACK RX(66) ... M-NACK STOP
    next-period probes: 0x88=1 0x8A=0 recovery_calls=0
    FAIL  FWR-118: the period after a CRC failure re-probes BOTH candidate addresses
[T17b] FWR-118: after a range failure the next period must re-probe both candidates
    PASS  range-invalid period is reported as GXHT40_ERR_RANGE
    next-period probes: 0x88=1 0x8A=0 recovery_calls=0
    FAIL  FWR-118: the period after a range failure re-probes BOTH candidate addresses
```

代码层根因（与实测一致）：`gxht40_acquire()` 只在 `gxht40_start_and_read()` 返回非 `OK`（`NO_DEVICE`/`IO`）时清空 `s_addr7` 并调用 `i2c_bus_recover()`；CRC 错与量程无效发生在 `gxht40_measure()` 后续判定处，走 `continue` 重测，**缓存地址被保留**。故实现记录所述「缓存地址**任一失败**即 `s_addr7=0`」对其自身列举的 CRC/量程情形并不成立。

**影响范围（如实界定）**：「不得因缓存地址让器件永久失联」这一**目的**未被违反（CRC/量程错误下器件仍在应答，缓存地址仍可用）；被违反的是要求**字面**规定的重探义务，以及与之绑定的「重探前的有界总线恢复」。本轮实板证据（`ack=none`、`s=2`、`a44=a45=0`）**不受**该缺陷影响。修代码或收敛要求措辞，属负责能力（设计/实现）的判断；本能力只给出实测事实与要求原文。

### 4.3 登记性观察（OBS-2，不作通过门槛）

**观察**：当**恰好一个**候选地址 ACK 而读事务始终无法完成时，随周期发布的诊断快照会给出 `s=2`（无器件）**同时** `a44=1`，即结果码与地址 ACK 事实存在张力。实测复现（同 harness T15）：

```text
[T15] observation: diagnostic snapshot with exactly one ACKing candidate
    snapshot: status=2 ack44=1 ack45=0 read_retry=5 attempt=3 raw_valid=0
    UART rendering would be: 'G s=2 a44=1 a45=0 rd=5 at=3 raw=------------'
```

- 成因：T2 引入「失败后强制重探两候选」后，`measure()` 返回的结果码是**最后一次候选探测**的码，而非本周期的主因。
- 对**本轮**证据无影响：实板 6 条 `G` 均为 `a44=a45=0`，与 `s=2` 完全自洽（§3 检查组 E 全通过）。
- **受影响的自有资产**：`evidence/embedded_verify_diag_item001.py`（ITEM-001）内置规则「`s=2` 时两地址 ACK 均应为 0」。若日后实板出现「器件 ACK 地址但读不通」的形态，该脚本会报失败——此时应先判定是脚本规则过严还是结果码语义需收紧，**不得**直接改脚本放行。
- 处置：登记给设计/实现能力决定「结果码语义」与「脚本规则」哪一侧需修正；本能力本轮不修改该脚本。

## 5. 为闭环本项所需的现场动作（最小必要）

本项核心可观察受阻于**器件不应答**，任何固件改动都无法改变；需要现场（用户/硬件侧）完成下列**之一或全部**，并保持 3 分钟节拍采集窗口：

1. **目视/万用表核对 U9（GXHT40）贴装与焊接**：焊点是否虚焊/连锡、`U9.1`↔`PA04`、`U9.2`↔`PA03` 通断、`U9.3`(VCC) 与 `U9.4`(GND) 是否导通。
2. **给传感器一次真实掉电重启**（CR2032 焊接在板上，需现场断开/短接供电或拆焊后复位）：用于排除器件被闩锁在异常保持态。本能力无电源控制工具，且固件复位（PIN / SYSRESETREQ）**不等于**器件掉电。
3. 若手头有**替换的 GXHT40 器件**，可直接换件复测。

时间窗口：任一次现场动作完成后告知，本能力将立即重新采集——每 3 分钟 1 个周期，连续 ≥3 个周期约 9 分钟即可判定 `q=1` 是否恢复。现场动作前后**不需要**重新编译或改动任何代码。

**同步待办（属实现能力）**：§4.2 的 FWR-118 重探不符合，请按证据修复（任一失败——含 CRC 错与量程无效——后失效缓存地址并在重探前调用 `i2c_bus_recover`），修复后重新构建并交接复验。

## 6. 宿主端独立验证（本能力自有资产同步与复跑）

按 TD-002 §3.2 登记的测试资产同步项，本轮完成并对**当前交付源码**复跑：

| 资产 | 本轮变更 | 结果 |
|---|---|---|
| `test/host_sf_i2c_verify_ev.c` | 新增位级 `i2c_bus_recover` 独立模型与用例：**T9** 空闲总线恢复**不产时钟**（`scl_rise_n=0`、事件数 0）；**T10** SDA 持续被拉低时**恰 9 个**恢复脉冲 + **1 个合法 STOP**、返回 `TIMEOUT`、**无 START**、**不写任何器件字节**；**T11** 从机第 3 个脉冲前释放则**提前停止**（3 脉冲 + STOP、返回 `SUCCESS`）；**T11b** 恢复序列无地址字节/无读、无 ACK 槽 | `51 passed, 0 failed`（exit 0）；原 35 项既有用例全部保持通过 |
| `test/host_gxht40_verify_ev.c` | 新增 **T11/T12**（缓存地址失败→立即失效缓存且**两候选**都重探；器件改到另一地址仍可找到、不永久失联）；**T13** 用 `-Wl,--wrap=i2c_bus_recover` 链接期拦截证明**恢复调用发生在重探之前**（`recovery call at event #4, next 0x88 probe at #6, 0x8A probe at #10`）；**T14** `tPU` 仅首次访问一次、`tMEAS` 仍 1 次 10 ms；**T15** 登记 OBS-2；**T17/T17b** FWR-118 一致性（**2 项预期失败 = §4.2 的缺陷证据**）；**T16** 全程命令白名单含「软复位 `0x94` 从未出现」；并把 T5 的旧期望（`ERR_IO`）更新为 T2 后的 as-built 语义并注明原因 | `81 passed, 2 failed`（2 项即 T17/T17b，故意编码要求以防回归被漏掉） |

构建命令（可复跑；注意 `--wrap`）：

```bash
# sf_i2c 恢复原语
gcc -std=c11 -Wall -Wextra -I../USER/inc host_sf_i2c_verify_ev.c ../USER/src/sf_i2c.c -o _hostbin/sfi2c_verify_ev_item002.exe
# GXHT40 驱动（含 FWR-118 重探 / 有界恢复 / tPU）
gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast -I test/mock_mcu -I USER/inc -I COMMON \
    test/host_gxht40_verify_ev.c USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c \
    -Wl,--wrap=i2c_bus_recover -o test/_hostbin/gxht40_verify_ev_item002.exe
```

过程记录（harness 的两处自查，供后续复用）：① 最初的「总线被从机拉低」驱动级模型会让主机把被拉低的线读成 **ACK**，与 mock 从机状态机自相矛盾，故**放弃该模型**——「恰 9 脉冲 / 提前释放 / 无 START / 不写器件字节」改在 `host_sf_i2c_verify_ev.c` 的**原语级**验证，驱动级改用 `--wrap` 拦截验证「恢复在重探之前」；② 一次用例误把 mock 从机停在 `ST_ADDR_BITS`（真实路径 STOP 后应为 `ST_IDLE`），把恢复时钟误读成一个伪字节，已修正并在用例内注明。

## 7. 本轮未覆盖项与证据缺口（如实登记）

| 项 | 状态 | 原因 / 说明 |
|---|---|---|
| 成功周期实板形态（`q=1`、无 `G` 行、`t/h` 有效域） | 未覆盖 | 器件全程不应答（§3.1）⇒ 当前硬件上不可产生 |
| `ack` 地址表形态、`raw` 十六进制可用性 | 未覆盖（实板） | 同上；仅由宿主 harness 覆盖 |
| 手机端读数与 `S` 行同一笔 | 未执行 | 无可比较的上报（`r=0 s=0` ×6），且 T3/T4 未完成（§4.1） |
| `k=6 / 9 / 12 / 15` 四个周期 | **未采集** | 一是 `k=6` 落在 cap1 结束（04:53:03）与后续窗口开始（04:55:2x）之间的空档；二是本能力自建采集脚本的**成功标记匹配写错**（工具输出为 `"success":true`，脚本按 `"success": true` 匹配），导致一次已完成的捕获被重试覆盖。已在 `chain_caps2.sh` 修正（每次尝试写独立文件、仅成功后提升）并复跑。**不影响判定**：本项结论只依赖「是否存在 `q=1`」，而 6 个跨越 27 分钟、节拍自洽的观测点全部为 `q=0` |
| `SENSOR_DEBUG_UART=0` 复核 | 未执行（本轮） | 属 T1 交付面，EV-008 已验证；本轮无相关源码改动 |
| I²C 位级电气/时序、供电电流、CR2032 寿命 | 未覆盖 | 需逻辑分析仪/电流表，用户未提供（TD-002 §13 已列为非强制） |

## 8. 判定汇总

| 维度 | 结论 |
|---|---|
| 部署与受测件身份 | 通过：`mdk_build rebuild` 0 Error、`mdk_flash` Verify OK、axf md5 与实现声明一致 |
| 实板可观察（本项核心） | **未达成**：0/6 周期 `q=1`；一手数据指向器件地址级不应答（条件缺失） |
| 实现符合性（本项验收正文） | **不符合**：FWR-118 对 CRC 错/量程无效情形未重探两候选地址（宿主 harness 线上字节为证） |
| 节拍 / 结构 / 自洽 | 通过：Δk=3、行长 ≤96 B、`G` 行位置与字段自洽、失败周期不推进前值/不构造 0 值 |
| **Outcome** | **TEST_FAIL**（实现缺陷，可由实现能力修复）；同时如实登记**条件缺失**（器件不应答）需现场动作，二者须分别处置 |

---

# 嵌入式验证证据（EV-008 / run8 ITEM-001 ↔ GXHT40 温湿度采集失败在真实目标上可诊断：上电 BUS 行 + 失败周期 G 行）

状态：embedded_tester.embedded_verification 独立验证证据（本轮任务项执行轮；重新部署、重新采集真实目标字节流，不沿用历史结论）
本轮验证对象：run8 队列项 **ITEM-001**（GXHT40 温湿度采集失败在真实目标上可诊断，不依赖逻辑分析仪的实板一手数据）
受测提交：`374fa29`（HEAD，工作树干净）
受测件：`MDK/output/exe/Project.axf`，md5 **`c39bf314851a0988a5f347c73604d105`**（由 HEAD 源码经 `mdk_build rebuild` 构建；与实现声明一致）
验证基线：TD-002 rev 5.0 `artifacts/test_design.md`（§4.1 T-L0-11、§4.3 T-L1-14、§4.4 T-L2-11/T-L2-12）
结论：**TEST_PASS** —— 本项全部**可观测**验收点均在真实目标上以原始字节复现，并在宿主端对真实目标当前无法出现的形态（ACK 地址表、`raw` 十六进制、成功周期）做了独立验证。

| 验收点（任务文本「可观察」） | 实板实测 | 依据 |
|---|---|---|
| 复位后横幅之后可见**一条** `BUS` 行（空闲电平 + ACK 地址集合） | **通过 ×3 次复位**：`BUS idle=1 scl=1 sda=0 ack=none`（boot#1）/ `BUS idle=3 scl=1 sda=1 ack=none`（boot#2、boot#3）；每段恰 1 条，后续周期不再出现 | §2.2 |
| `idle` 位掩码与 `scl`/`sda` 自洽（bit0=SCL、bit1=SDA） | 通过：`idle=1`⇒(1,0)、`idle=3`⇒(1,1)；宿主端 4 种掩码全量展开验证 | §2.2 / §4.2 |
| `ack` 为 `none` 或合法 7bit 地址集合（最多 8、超出 `+`） | 实测 `ack=none`（合法形态）；地址表/截断形态由宿主端最坏情形验证 | §2.2 / §4.2 |
| `q=0` 周期在该周期 `S` 行后**紧跟**一条 `G` 行 | 通过 ×6 个失败周期（4 次捕获）：每个 `q=0` 周期恰 1 条 `G`，且紧跟其 `S` 行 | §2.3 |
| `G` 的结果码/地址 ACK/重读重测次数/原始字节与本周期失败原因自洽 | 通过：全部 `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`，与 `BUS ack=none` 及「无器件」结果码逐项自洽 | §2.3 / §3 |
| 每行 ≤96 B、只用整数与大写十六进制 | 通过：`BUS`=33 B、`G`=46 B、`S`=66 B；脚本逐值断言仅整数/大写 hex | §2.5 / §4.2 |
| 成功且未上报周期的输出与既有 `S` 行逐字节相同 | `S` 行字面/字段集/顺序相对上一轮**未改**（diff 无实现行改动）；宿主端用真实捕获的 `S` 行做逐字节金标准比对 | §4.1 / §4.2 / §4.5 |
| 采样间期仍无任何字节 | 通过：300 s 窗口内仅 2 个 `S+G` 对（224 B）、10 s 窗口 0 B | §2.4 |
| 新增地址探测原语不得改变既有软 I²C 函数语义 | 通过（最强证据）：既有全部 `sf_i2c` 函数机器码逐指令相同、符号大小逐项相等；唯一新增符号 `i2c_probe_addr`(34 B) | §4.4 |
| 诊断不得改变采样、判定、上报与冻结状态 | 通过：实板 `S` 行字段与失败语义自洽；宿主端真实 `measure.c` 流程验证业务状态不被扫描/快照改动 | §4.1 / §4.3 |
| `SENSOR_DEBUG_UART=0` 时全部为空实现 | 通过（编译级）：扫描编译为 `movs r0,#0; bx lr`、`debug_trace.c` 零符号、`measure.o` 无 `i2c_probe_addr`/UART 引用（附 1 项内部观察，见 §5 局限） | §4.3 |

---

## 1. 环境、构建与部署

| 工具 / 命令 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `COM42 = Prolific USB-to-Serial Comm Port (VID:PID 067B:2303)`；`COM26 = USB 串行设备 (C251:F001)` | 传感器调试口 **COM42 在位** |
| `usb_list_devices` | `USB\VID_C251&PID_F001`（HID MI_02 + Ports MI_00，SN `87094109484987710672FF50`，Status OK） | **CW-DAPLink 下载探针在位** |
| `mdk_build rebuild` | `*** Using Compiler 'V6.24'`；`Program Size: Code=16316 RO-data=624 RW-data=76 ZI-data=1676`；`0 Error(s), 1 Warning(s)`（`main.c(224): while loop has empty body` —— **基线既有**空体 `while(k--);`） | 当前提交可构建；与实现声明逐字一致 |
| `mdk_flash {}` ×3（本项部署/复现） | 均 `Erase Done.Programming Done.Verify OK.Application running ...`（03:56:59Z / 04:25:59Z / 04:29:04Z） | 受测镜像已下载并回读校验；在已打印多条轨迹后仍可再次下载 ⇒ **SWD 未被调试通道或诊断路径破坏** |
| `sh gcc/build.sh`（HEAD） | `0 error`；`FLASH: 32840 B / 64 KB (50.11%)`、`RAM: 1768 B / 4 KB (43.16%)` | 与实现声明一致（基线 `1dd8e4b`：31,952/1,736 → 本项 +888/+32 B） |
| `sh test/build_test.sh`（实现侧自检复跑） | `40 + 34 + 33 + 19 + 20`，全部 0 失败 | 交叉确认（不作独立证据） |

工具副作用：`mdk_build`/`mdk_flash` 会在仓库根生成 `build*.log`；已读入本证据后删除，未入库（本轮实测文件名形如 `build_1791431810961820000.log`，`.gitignore` 的 `/build.log` 未覆盖该变体——见 §6 交接）。

## 2. 真实目标 UART1 观测（COM42 @9600 8N1，原始字节）

采集方式：先开一个 `serial_capture` 窗口（300 s 或 120 s，`duration_sec` 上限 300），再在窗口内执行 `mdk_flash`，使复位后的横幅与 `BUS` 行落在窗口内。时间均为 UTC。原始捕获文件（Runtime 角色工作目录 `work/verification_tools/`）：

| 捕获 | 文件 | 窗口（UTC） | 字节 | 内容 |
|---|---|---|---|---|
| **capA** | `serial-capture-1791432093371341400.bin` | 03:56:33.155 → 04:01:33.380 | 409 | 复位前旧镜像横幅 + 截断 `S`；复位后横幅 + `BUS` + `S k=0` + `G` + `S k=3` + `G` |
| **capB** | `serial-capture-1791432459701609400.bin` | 04:02:39.490 → 04:07:39.703 | 224 | 无复位：`S k=6`+`G`、`S k=9`+`G` |
| **capW** | `serial-capture-1791431776097425700.bin` | 03:56:05.882 → 03:56:16.098（10 s） | **0** | 采样间期零输出 |
| **capC** | `serial-capture-1791433645089851300.bin` | 04:25:24.891 → 04:27:25.091 | 245 | 覆盖烧录：横幅×2 + `BUS` + `S k=1` + `G` |
| **capD** | `serial-capture-1791433832424379300.bin` | 04:28:32.212 → 04:30:32.425 | 245 | 覆盖烧录：横幅×2 + `BUS` + `S k=0` + `G` |

**字节账目（逐字节闭合，无未知字节）**

- capA = 50 + 52 + 50 + 33 + 66 + 46 + 66 + 46 = **409** ✔（第 2 项为烧录停止 CPU 造成的无 CRLF 截断行）
- capB = (66+46) × 2 = **224** ✔（300 s 窗口内**除此之外无任何字节**）
- capC = capD = 50 + 50 + 33 + 66 + 46 = **245** ✔

### 2.1 capA 原始行表（脚本 `evidence/embedded_verify_diag_item001.py` 输出）

```
[   0] BOOT  OK    50B  BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600
[  50] S     CUT   52B  S k=0 p=0 H=0 V=1 o=8 n=62 x=64 a=63 D=0 t=0 h=0 q=0
[ 102] BOOT  OK    50B  BOOT fw=FD-002r4 uid=6A002C00 rst=0240 uart=9600
[ 152] BUS   OK    33B  BUS idle=1 scl=1 sda=0 ack=none
[ 185] S     OK    66B  S k=0 p=0 H=0 V=0 o=8 n=63 x=64 a=63 D=0 t=0 h=0 q=0 r=0 s=0 y=0
[ 251] G     OK    46B  G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
[ 297] S     OK    66B  S k=3 p=0 H=0 V=0 o=8 n=62 x=64 a=63 D=0 t=0 h=0 q=0 r=0 s=0 y=0
[ 363] G     OK    46B  G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
全局: 横幅 2 条 / BUS 1 条
```

- 第一条横幅 `rst=0040` 与其后**无 CRLF 的 52 B `S` 行（`V=1`）**来自**烧录前仍在 Flash 中的 run7 镜像**（`V=1` 与 run7 语义一致，本轮 `V=0`）；该行被 `mdk_flash` 擦除/编程时停止 CPU 截断，属**烧录产物**，与 EV-006/EV-007 记录一致，不计固件缺陷。
- 第二条横幅 `rst=0240`（=PIN|SYSRESETREQ）之后即为本项受测件的输出。

### 2.2 上电 BUS 行（三次复位，逐字段）

| 复位 | 捕获 | 原始 BUS 行 | `idle` | `scl` | `sda` | `ack` |
|---|---|---|---|---|---|---|
| boot#1 | capA（03:56:59Z） | `BUS idle=1 scl=1 sda=0 ack=none` | 1 | 1 | 0 | none |
| boot#2 | capC（04:25:59Z） | `BUS idle=3 scl=1 sda=1 ack=none` | 3 | 1 | 1 | none |
| boot#3 | capD（04:29:04Z） | `BUS idle=3 scl=1 sda=1 ack=none` | 3 | 1 | 1 | none |

- 三条 `BUS` 行**均恰好出现一次**，位置**紧随横幅、且在任何 `S`/`G` 行之前**（上电先诊断、任何 I²C 事务之前），后续周期不再出现（capB 300 s 无复位窗口内 0 条）。
- `idle` 与展开位逐条自洽：boot#1 `idle=1` ⇒ SCL 高/SDA 低；boot#2/#3 `idle=3` ⇒ 双线均高（健全空线）。
- `ack=none` 三次一致：**7bit 地址 0x08..0x77（含 0x44/0x45）无任何地址收到 ACK**。
- **可复现性观察**：空闲电平在 3 次上电中为 `1,3,3` —— SDA 空闲读数**不恒定**（1 次读到低、2 次读到高），但 `ack=none` **恒定**。该差异本身是一手事实，见 §3 的解读与负对照。

### 2.3 失败周期 S 行 → G 行（本项捕获到的全部 6 个失败周期）

| 捕获 | S 行（原文） | 紧跟的 G 行（原文） |
|---|---|---|
| capA | `S k=0 p=0 H=0 V=0 o=8 n=63 x=64 a=63 D=0 t=0 h=0 q=0 r=0 s=0 y=0` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` |
| capA | `S k=3 … n=62 x=64 a=63 …` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` |
| capB | `S k=6 … n=60 x=62 a=61 …` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` |
| capB | `S k=9 … n=59 x=62 a=60 …` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` |
| capC | `S k=1 … n=58 x=60 a=59 …` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` |
| capD | `S k=0 … n=58 x=60 a=58 …` | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` |

- 每个 `q=0` 周期**恰好一条** `G` 行且**紧跟**其 `S` 行（中间无其它字节/行）；`G` 行的三处计数与结果码逐周期恒定。
- **逐项自洽**（脚本断言）：`s=2`（`GXHT40_ERR_NO_DEVICE`，取值域 1..5）⇒ 必然 `a44=0 && a45=0` ✔ 与 `BUS ack=none` 一致；`rd=0`（本轮读事务未发生，因为命令从未被 ACK）✔；`at=3`（= `GXHT40_MEAS_RETRY`，整帧重测用尽）✔；`raw=------------`（12 个 `-`，未成功读回 6 字节）✔ 且此时 `rd=0` 与「无成功读」一致 ✔。
- `S` 行字段集与顺序（15 字段 `k p H V o n x a D t h q r s y`）与既有轨迹一致，未因诊断行发生任何变化。

### 2.4 采样间期零输出与节拍

- capB：300 s 窗口内**只有** 2 个 `S+G` 对（224 B），其余时间为空 ⇒ 打印后 UART1 已关闭，采样间期无任何字节。
- capW（10 s）：**0 字节**（同一结论的独立窗口）。
- capA 复位 T0=03:56:59Z：`k=0` 于 T0、`k=3` 于窗口内（≤270 s）⇒ 相邻 `k` 增量恒为 3（3 分钟节拍）；capC 首个 `k=1`、capD 首个 `k=0` 属**非 POR 复位下 RTC 相位随机**的既有行为（EV-007 §2.4 已登记，与本项无关）。

### 2.5 独立字节级复核脚本

`evidence/embedded_verify_diag_item001.py`（本能力新写，不复用实现侧测试）：按 `BOOT/BUS/S/G` 行首切分原始字节，识别烧录截断行（CUT）与窗口起始残行（FRAG）并将其排除出判定，按每次横幅分段，逐行/逐字段断言（行长 ≤96、CRLF 终止、仅整数与大写十六进制、`idle` 位与 `scl`/`sda` 一致、`BUS` 恰 1 条且在横幅后任何 `S/G` 前、`G` 仅随 `q=0` 出现且恰 1 条、结果码与地址 ACK/重读重测/原始字节自洽、`raw` 为 hex 时用**脚本内独立 CRC-8** 复算两个字）。

```
$ python evidence/embedded_verify_diag_item001.py <capA> <capB> <capW> <capC> <capD>
==== result: 139 checks, 0 failed ====      (exit=0)
```

## 3. 诊断解读：GXHT40 采集失败的一手定位（本项交付物）

三条 `BUS` 行 + 6 条 `G` 行给出如下**互相印证**的事实链：

1. **总线空闲**：`scl=1` 恒定；`sda` 在 3 次上电中读到 `0/1/1`。netlist 核定 `PA04`(=SDA) 与 `R1 4.7K→BAT`、`PA03`(=SCL) 与 `R2 4.7K→BAT` 同网（`sensor_hardware/pstxnet.dat` 网 N05845/N06024），故 `idle=3`（boot#2/#3）表示两条线都被外部上拉拉到高，**上拉与供电正常**。
2. **无器件应答**：`ack=none` 三次一致 —— 0x08..0x77 全部无 ACK，含 GXHT40 的 0x44/0x45。
3. **负对照（宿主端可控实验）**：I²C 是线与总线。若 SDA 被**持续**拉低，主机在每个地址槽都会读到「ACK」，扫描必然报出一张**满 8 地址且带 `+`** 的列表。宿主 harness 用「SDA 被外部持续拉低」模型实测得 `ack_count=8, truncated=1`（§4.1 [S6]）。因此**实板 `ack=none` 不可能由 SDA 卡低解释** —— 它直接证明在 ACK 时隙 SDA 为高、且**没有任何器件应答**。
4. **与 G 行一致**：驱动因此走「两地址均无 ACK」路径 ⇒ `s=2`、`a44=a45=0`、未发起读 ⇒ `rd=0`、无成功读 ⇒ `raw=------------`、整帧重测用尽 ⇒ `at=3`。三条输出（`BUS`/`S`/`G`）互相自洽，且与本周期实际失败原因（地址无应答）一致。

**结论（交 T2 实现能力）**：真实目标上的失败发生在「器件是否在总线上应答」这一层，而非协议/CRC/量程层。可排除的候选与仍需现场确认的候选：

- **可排除（有一手数据）**：总线无上拉/被卡低（boot#2/#3 `idle=3`）；UART 观测通路（本项已证）。
- **boot#1 的 `idle=1` 需注意**：该次仅在**空闲读时刻**读到 SDA 低，而该次 112 次地址探测仍全部无 ACK（若持续卡低会得到满表 ACK）。故它更像一次**瞬态/未定态读数**（例如器件在复位瞬间短暂握住 SDA，或释放后未立即建立），不能据此判定「SDA 硬件短路/卡低」，也不宜据此定位根因；重复测量应优先观察该字段是否复现。
- **仍需现场确认（非本能力工具面，需万用表/目视/焊接检查）**：U9(GXHT40) 是否贴装/焊接良好、其 VDD/引脚是否连通到 N05845/N06024、器件是否处于异常保持状态。本项已把「无应答」这一确定事实与「SDA 空闲瞬态」这一不确定观察分离记录，避免把后者当成根因。

## 4. 宿主端独立验证（覆盖真实目标当前无法出现的形态）

### 4.1 `test/host_diag_probe_verify_ev.c`（本能力新写）—— 85/85

链接**真实** `USER/src/{measure.c,gxht40.c,sf_i2c.c,fw_core.c}`，由本能力自写的**位级 mock 总线 + mock 从机**驱动（真实软 I²C 逐位打入，非桩）。

```
$ cc -std=c11 -Wall -Wextra -Wno-unused-function -Wno-misleading-indentation \
     -Wno-unused-const-variable -DSENSOR_DEBUG_UART=1 -Itest/mock_measure_ev \
     -IUSER/inc -IUM2005C -ICOMMON test/host_diag_probe_verify_ev.c USER/src/measure.c \
     USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c USER/src/encrytogate.c -lm -o diag_probe_verify
$ ./diag_probe_verify
==== result: 85 passed, 0 failed ====      (exit=0)
```

| 组 | 独立断言（原始线上转录 / 数值） | 结果 |
|---|---|---|
| **P1 探测原语** | 有器件：`START TX(88) ACK STOP` —— 恰 1 START/1 STOP、**线上恰 1 个地址字节**、不读数据、从机未收到任何命令/寄存器字节、结束后双线均高 | PASS |
| **P2 NACK 释放** | 无器件：`START TX(88) NACK STOP STOP` —— NACK 后仍发 STOP、地址后不再发任何字节、总线空闲高（未卡死） | PASS |
| **P3 地址精确** | 0x44 应答时探测 0x8A：`START TX(8A) NACK …`，只发请求的地址 | PASS |
| **P4 既有原语回归** | `i2c_write_cmd` 仍 `TX(88) ACK TX(FD) ACK`；`i2c_read_bytes(0x89,6)` 仍 `… RX(61) M-ACK … RX(75) M-NACK STOP` | PASS |
| **S1 BUS 扫描** | 112 次探测、`TX=112`（地址字节仅此）、`RX=0`、从机写入 0 字节；地址字节严格按 `(addr7<<1)` 从 0x08 升到 0x77（写位=0）；每次探测 1 START 且均发 STOP（`STOP=224`，NACK 路径双 STOP 属既有写法）；`idle=3`；**业务状态 `sample_flag`/`report_req`/`tempvalue`/`huminityvalue` 完全未变**；`bsp_i2c_init()` 恰好配置 PA3/PA4 一次 | PASS ×10 |
| **S2 幂等** | 第二次扫描同结果、物理层不再重配、仍完整探测 112 地址 | PASS ×3 |
| **S3/S4 ACK 集合** | 器件在 0x44 ⇒ `ack_count=1, ack_addr[0]=0x44`；换成 0x10 ⇒ 原样记录（无硬编码） | PASS |
| **S5 截断上限** | 全部地址应答 ⇒ `ack_count=8, ack_truncated=1`，记录的 8 个为**最先探测的** 0x08..0x0F（即行尾 `+`） | PASS ×3 |
| **S6 负对照（关键）** | 模型「SDA 被外部持续拉低」⇒ `idle=1` 且 **`ack_count=8, truncated=1`** —— 证明实板 `ack=none` 不可能来自 SDA 卡低（见 §3.3） | PASS ×2 |
| **D1..D7 驱动诊断快照** | 成功：`status=0 a44=1 a45=0 rd=0 at=1 raw_valid=1 raw=66669372B0DC`（与实读 6 字节一致）、tMEAS 仍恰 1 次 10 ms；读 NACK×2 后成功：`rd=2 at=1`；CRC 错：`status=4 a44=1 at=3 raw` 保留最后读回帧；无器件：`status=2 a44=a45=0 rd=0 at=3 raw_valid=0`；量程无效：`status=5 a44=1 at=3`；读重试用尽：`status=3 a44=1 rd=5 raw_valid=0`；全部失败路径**输出哨兵值不变**；重复 `gxht40_diag_fetch` 返回同一快照 | PASS ×25 |
| **F1..F3 流程传播** | 真实 `measure.c` 流程：失败周期 `sample_ok=0, diag_valid=1` 且快照字段逐项来自驱动、`temp/hum` 保留上次有效值、不置上报、快照消费一次；成功周期 `sample_ok=1, diag_valid=0`（**不产生 G 行**）且测得值进入 `S` 字段；成功后再失败的周期**前值仍为 250 未被污染** | PASS ×13 |

### 4.2 `test/host_diag_line_verify_ev.c`（本能力新写）—— 36/36

链接**真实** `USER/src/debug_trace.c`，对接到本能力自建的 UART/GPIO 影子层（`test/mock_diagline_ev/`，本能力新写，不复用实现侧 `mock_trace`），逐字节比对 UART1 实际输出。

```
$ cc -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Itest/mock_diagline_ev -IUSER/inc \
     test/host_diag_line_verify_ev.c USER/src/debug_trace.c -o diag_line_verify
$ ./diag_line_verify
==== result: 36 passed, 0 failed ====      (exit=0)
```

| 情形 | 期望（独立写出的字面量） | 实测 |
|---|---|---|
| BUS 最坏（8 地址 + 截断） | `BUS idle=3 scl=1 sda=1 ack=08,1F,2A,44,45,50,6E,77+` | **53 B**（≤96）逐字节相同 |
| BUS 单地址 | `BUS idle=3 scl=1 sda=1 ack=44` | 31 B 相同 |
| BUS 实板形态 | `BUS idle=1 scl=1 sda=0 ack=none` | 33 B 相同 |
| BUS 掩码展开 | `idle=0/1/2/3` ⇒ `scl,sda = 0,0 / 1,0 / 0,1 / 1,1` | 4/4 相同 |
| `BUS` 传 NULL | 不输出任何字节 | 通过 |
| **S 行金标准** | 用**实板 capA 捕获到的原文** `S k=0 p=0 H=0 V=0 o=8 n=63 x=64 a=63 D=0 t=0 h=0 q=0 r=0 s=0 y=0` 作期望 | 逐字节相同、恰 1 个 CRLF、**无 `G` 行**（`diag_valid=0`） |
| S 行最坏宽度 | tick=4294967295、p=-1250、t=-1250、h=1000、ADC=4095、s=2、y=3 | **92 B**（≤96）、恰 1 CRLF |
| G 行（未读成功） | `S …` + `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` | 112 B 两行、恰 2 CRLF、`G` 紧随 `S` |
| G 行最坏（全计数最大 + 12 位大写 hex） | `G s=5 a44=1 a45=1 rd=5 at=3 raw=ABCDEF012345` | **46 B**；`s=1..5` 均渲染为单个十进制位 |
| 整机上电最坏 | 横幅 + 最坏 BUS + 最坏 S + 最坏 G | 4 行、最长 92 B（≤96） |
| 关闭语义 | 排空后复位并关 UART1 时钟、幂等、下一行自动重开 | 通过 |

> 说明：写 `S2/G2` 期望值时，本能力先用独立算式核算得到最坏 `S`=92 B、最坏 `G`=46 B、最坏 `BUS`=53 B，与实现文档所列一致；这不是采信实现数字，而是独立复算后的吻合。

### 4.3 `SENSOR_DEBUG_UART=0` 空实现（编译级证据）

```
$ arm-none-eabi-gcc -c -O1 -DSENSOR_DEBUG_UART=0 -DSENSOR_CONFIG_NO_MCU ... USER/src/measure.c
$ arm-none-eabi-objdump -d -j .text.sensor_bus_diag_scan measure0.o
00000000 <sensor_bus_diag_scan>:
   0: 2000   movs  r0, #0
   2: 4770   bx    lr                 <- 空实现: 不读引脚、不发任何 I2C 探测
$ arm-none-eabi-nm measure0.o | grep -i "i2c_probe\|UART\|scl_pin_read"   -> 无命中
$ arm-none-eabi-gcc -c -DSENSOR_DEBUG_UART=0 ... USER/src/debug_trace.c && arm-none-eabi-nm dt0.o   -> 空（本文件不产生任何符号）
对照 -DSENSOR_DEBUG_UART=1：sensor_bus_diag_scan 为完整实现且引用 i2c_probe_addr
```

即：关闭态下 `BUS`/`G` 的输出与地址扫描均为**空实现**，不引入 UART 符号、不做 I2C 探测、不读 PA03/PA04。

### 4.4 既有软 I²C 函数机器码等价（回归最强证据）

在临时 worktree 独立构建基线 `1dd8e4b`，与 HEAD 构建产物逐函数比对（`arm-none-eabi-objdump -d --disassemble=<fn>`，归一化重定位地址后逐指令比较）：

| 既有函数 | 结果 |
|---|---|
| `i2c_init` / `i2c_start` / `i2c_stop` / `i2c_write_byte` / `i2c_read_byte` / `i2c_wait_ack` / `i2c_write_multi_byte` / `i2c_read_multi_byte` / `i2c_obj_find` / `i2c_write_cmd` / `i2c_read_bytes` | **逐指令完全相同** |
| `i2c_write_multi_byte_16bit` / `i2c_read_multi_byte_16bit` | 仅分支**目标地址**整体平移 `+0x22`（= 新增函数大小），指令编码逐字相同 |
| `nm --print-size` 对照 | 既有函数**大小逐项相等**；唯一新增符号 `i2c_probe_addr` = **0x22(34) B** |

### 4.5 变更范围审查（`git diff 1dd8e4b 374fa29`）

| 文件 | 变更 | 性质 |
|---|---|---|
| `USER/src/sf_i2c.c`（20/0）、`USER/inc/sf_i2c.h`（6/0） | 仅追加 `i2c_probe_addr` | **纯新增** |
| `USER/src/measure.c`（96/0）、`gxht40.c`（56/0）、`main.c`（8/0）、`sensor_config.h`（10/0）、`measure.h`（9/0）、`gxht40.h`（20/0） | 仅追加 | **纯新增** |
| `USER/src/debug_trace.c`（83/3）、`USER/inc/debug_trace.h`（28/1） | 全部 4 处删除**均为注释行**（“S 行最坏宽度核算”注释与 `debug_trace_sample` 的行注释）；`S` 行发射代码无任何改动 | 无实现行改动 |
| `test/*`、`.gitignore`、`artifacts/`、`evidence/` | 实现侧自检/文档 | 非产品代码 |

⇒ 「新增地址探测原语不得改变既有软 I²C 函数语义」与「成功周期输出与既有 `S` 行逐字节相同」在源码与机器码两层同时成立。

### 4.6 宿主自检复跑与遗留测试资产

- 实现侧 `test/build_test.sh`：`40 + 34 + 33 + 19 + 20 = 146` 项 0 失败（交叉确认，不作为独立证据）。
- 本能力既有独立 harness 复跑：`host_sf_i2c_verify_ev` **35/35**、`host_gxht40_verify_ev` **42/42**（T1 相关面未回归）。
- **遗留（如实记录，本轮未修）**：`host_fw_core_verify_ev.c`、`host_light_verify_ev.c`、`host_measure_flow_verify_ev.c` 仍按 run7-rev4.0 的光照语义断言（`light_code_is_dark` / `bool light_sample()`），已无法对着当前 `light_is_dark` / `light_result_t` 编译。它们属**光照判据/标定**相关任务项的验证资产，本项未改（改动会越过本能力当前边界）；该缺口与实现侧交接说明一致。

## 5. 判定与局限

**判定：TEST_PASS（run8 ITEM-001）**

- 本项全部**可观测**验收点在真实目标上以原始字节复现（§2），三条诊断输出线（`BUS`/`S`/`G`）互相自洽并与本周期实际失败原因一致（§3）。
- 真实目标当前**无法**出现的形态（ACK 地址集合、`raw` 十六进制、成功周期的「只有一条 `S` 行」）由宿主端独立 harness 覆盖（§4.1 [S3/S4/S5/S6]、[D1]、[F2]、§4.2）。

**局限（如实记录）**：

1. **成功周期未在实板出现**：GXHT40 每个周期都不应答（`ack=none`、`q=0`），因此「正常周期仍只有一条 `S` 行」**未在实板观测**；该条仅在宿主端（`diag_valid=0` ⇒ 恰一行、且与实板捕获的 `S` 行逐字节相同）得到证据。这是本项要诊断的既有故障（OBS-1）所致，不是本项交付物的缺陷。
2. **`ack` 的地址表/截断形态未在实板出现**（实板为 `none`）：以宿主端最坏情形与 8 地址上限验证。
3. **`raw` 的 12 位十六进制形态未在实板出现**（实板全程无成功读，故打印 12 个 `-`）：以宿主端 + 脚本内独立 CRC-8 复算路径覆盖。实板 `raw` 全 `-` 与 `rd=0` 的自洽性已断言。
4. **`SENSOR_DEBUG_UART=0` 的一处内部观察**：关闭态下 `sensor_bus_diag_scan`/`debug_trace.*` 确为空实现（§4.3），但 `measure.c` 在失败分支仍会调用一次 `gxht40_diag_fetch()` 并把快照写入内部静态变量（`.bss` 约 10 B），驱动侧也仍维护该快照。它是**只读、无 I/O、不参与任何判定/上报/冻结**的内部记账，不改变任何可观测行为（同类的 `s_trace_*` 记账自 run7 T1 起即如此）；本项按「输出与扫描为空实现、行为不变」判通过，并把「是否在关闭态一并编译掉该快照拷贝」登记为**可选**的实现侧改进（见 §6），不作为本项失败依据。
5. 未测：位级 I²C 时序/波形/上升时间、空口 433 与 BLE 波形、功耗电流、绝对温湿度精度、PA04 对 BAT 的实际电平（无逻辑分析仪/示波器/万用表，用户未提供）。本项**未**据这些未测项降低或提高任何判据。
6. `serial_capture` 原始产物无逐字节时间戳，节拍与出现顺序以窗口边界与行序表述。

## 6. 跨项观察与交接

1. **【交 T2 实现能力】GXHT40 失败的一手事实**（本项交付物，见 §3）：总线空闲在 boot#2/#3 为 `idle=3`（上拉正常），三次上电 `ack=none` 恒定，故失败在「器件不应答」层；`S`/`G` 输出（`q=0`、`s=2`、`a44=a45=0`、`rd=0`、`at=3`、`raw` 全 `-`）与之一致。boot#1 的 `idle=1` 是瞬时/未定态读数（同次 `ack=none` 排除持续卡低），建议 T2 判断时以「是否复现」为准，并配合万用表/目视确认 U9 贴装与供电。
2. **【交后续验证轮】** 后续若修好采集，需按本证据 §2/§2.5 复跑 `evidence/embedded_verify_diag_item001.py`（可直接使用），并补验「成功周期只有一条 `S` 行」与 `G` 行不出现的实板形态；届时 `raw` 会出现 12 位十六进制，脚本已内置独立 CRC-8 复算。
3. **【仓库卫生·交实现能力或工具维护】** `mdk_build`/`mdk_flash` 会在仓库根生成 `build_<数字>.log`，`.gitignore` 仅含 `/build.log`，该变体会出现在 `git status` 中（本轮读入证据后已删除，未入库）。建议把忽略规则放宽为 `/build*.log`；本轮未改该文件（越界）。
4. **【交光照相关任务项】** 本能力 3 个旧独立 harness（`host_fw_core_verify_ev` / `host_light_verify_ev` / `host_measure_flow_verify_ev`）仍按 run7-rev4.0 光照语义编写，需在光照判据/标定任务项中重写（§4.6）。
5. **节拍/复位相位**：capC 首个 `k=1`、capD 首个 `k=0`，源于非 POR 复位下 RTC 域跨复位继续计数（EV-007 §2.4 已登记，基线既有行为），与本项无关。

---

# 历史记录（EV-007 / run7 同一 ITEM-001；当轮判 TEST_PASS；已提交 2c0da5f）

# 嵌入式验证证据（EV-007 / ITEM-001 复验 ↔ UART1 调试串口初始化与调试打印可在真实目标上观测）

状态：embedded_tester.embedded_verification 独立验证证据（**EV-006 TEST_FAIL 后的修复复验轮**；重新部署并重新采集真实目标字节流，不沿用 EV-006 结论）
本轮验证对象：当前队列项 **ITEM-001**（UART1 调试串口初始化与调试打印可在真实目标上观测）
受测提交：`da9064a`（HEAD，工作树干净；修复提交 `firmware_engineer.firmware_implementation`，仅改 `debug_trace.c`/`main.c`/自检，未改本项外的业务逻辑）
验证基线：TD-002 rev 4.0 `artifacts/test_design.md`（§4.3 L1 / §4.4 L2）
结论：**TEST_PASS** —— EV-006 的两个缺陷均已在真实目标上复验修复，且未发现新缺陷：

| EV-006 缺陷 | 本轮实测结论 | 关键原始证据 |
|---|---|---|
| **D-ITEM001-1**（S 行末尾 CR+LF 未发出 → 行粘连/截断） | **已修复**：所有未被烧录过程干扰的 S 行均以 CRLF 正确终止；字节计数与「每行 = 完整字段 + CRLF」逐字节吻合 | §2.2（cap4/cap5/cap6/W1/W7 逐消息字节数与十六进制；cap5 全窗口 `CRLF=4 / 裸LF=0`） |
| **D-ITEM001-2**（横幅 `rst` 恒为 `0000`） | **已修复**：修复镜像的两次复位分别报 `rst=0040`（PIN）与 `rst=0240`（PIN\|SYSRESETREQ），取值非零且随复位原因变化 | §2.1（cap5 两条横幅）、§2.2（cap4 第二条横幅） |

其余本项验收面（真实可观测、字段完整/顺序冻结、≤96 B、纯整数无乱码、字段与固件实际状态自洽、采样间期零输出、关闭语义不破坏软 I²C 与 SWD、打印无业务副作用、唤醒后重新初始化、Flash 读保护保持注释）均**复验通过**（§2.3–§2.5）。跨项观察 OBS-1（GXHT40 每周期 `q=0`）**仍存在**且不属于本项，保持登记交接（§4）。

---

## 1. 环境、构建与部署

| 工具 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | COM42 = Prolific USB-to-Serial（VID:PID 067B:2303）；COM26 = USB 串行设备（VID:PID C251:F001） | 传感器调试口 COM42 在线 |
| `usb_list_devices` | 含 `USB\VID_C251&PID_F001`（HND MI_02 + Ports MI_00，SN 8709…，Status OK） | 下载调试探针在线 |
| `mdk_build rebuild` | `V6.24`；`Program Size: Code=14048 RO-data=620 RW-data=76 ZI-data=1644`；`0 Error(s), 1 Warning(s)`（`main.c(217): while loop has empty body`——与 EV-006 同一行号的**基线既有**空体 `while(k--);`，非本项新增） | 当前提交可构建 |
| `mdk_flash`（3 次：部署/中间/复现） | 均 `Erase Done.Programming Done.Verify OK.Application running ...` | 受测镜像已下载并回读校验；在已经打印多条轨迹后仍可再次下载 → SWD 未被破坏 |
| 宿主套件 `test/build_test.sh` | `38/38 + 24/24 + 24/24`，`rc=0`；新自检项 `PASS drains with TXBUSY (vendor UART_SendString pattern), not TC` | 自检期望已随修复更新（本轮为独立复跑，不作为实板证据） |

产物：`MDK/output/exe/Project.axf`，md5 `956dd6790bf11ff8cb584f584bc006d2`（由 HEAD `da9064a` 源码经 `mdk_build rebuild` 0 Error 构建）。

修复源码要点（只读核对，本能力未修改实现）：

```c
/* debug_trace.c: debug_trace_flush_close() —— D-ITEM001-1 */
while (UART_GetFlagStatus(DEBUG_UARTx, UART_FLAG_TXE) == RESET) { }
while (UART_GetFlagStatus(DEBUG_UARTx, UART_FLAG_TXBUSY) == SET) { }
/* 之后才复位/关闭 UART1 与 PA05/PA06 */

/* main.c —— D-ITEM001-2: 在任何系统/RTC 初始化清除标志之前锁存 */
#if SENSOR_DEBUG_UART
    rst_flags = SYSCTRL_GetAllRstFlag();
#endif
...
    debug_trace_boot(mcu_uid, rst_flags);
```

## 2. 真实目标 UART1 观测（COM42 @9600，原始结果）

采集方式同 EV-006：先开一个 300 s 的 `serial_capture` 窗口，再在窗口内执行 `mdk_flash`，使刷新后的启动横幅落在窗口内。时间均为 UTC。

### 2.1 捕获 cap5：窗口 23:07:20.418Z → 23:12:20.617Z，290 字节；窗口内 `mdk_flash`（复位于 23:07:50Z）

```
50  b'BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600\r\n'      <- 下载序列复位#1: 已运行的修复镜像, rst=PIN
52  b'S k=0 p=0 H=0 V=1 o=8 n=607 x=609 a=608 D=1 t=0 h=0 '      <- 下载过程中 CPU 被停止造成的行中截断(烧录产物)
50  b'BOOT fw=FD-002r4 uid=6A002C00 rst=0240 uart=9600\r\n'      <- 下载/校验完成后的复位#2: rst=PIN|SYSRESETREQ
69  b'S k=0 p=0 H=0 V=1 o=8 n=607 x=608 a=607 D=1 t=0 h=0 q=0 r=0 s=0 y=0\r\n'
69  b'S k=3 p=0 H=0 V=1 o=8 n=569 x=572 a=570 D=1 t=0 h=0 q=0 r=0 s=0 y=0\r\n'
CRLF count: 4   bare LF: 0
```

- `rst=0040` = `SYSCTRL_RESETFLAG_PIN`（bv6）；`rst=0240` = PIN|`SYSCTRL_RESETFLAG_SYSRESETREQ`（bv9）→ **非零、可区分、随复位原因变化**（D-ITEM001-2 修好）。
- 两条完整 S 行各自以 `0d 0a` 结束且字节数 = 67 + 2 = 69（D-ITEM001-1 修好）。

### 2.2 捕获 cap4/cap6 与短窗口 W1/W7（无额外复位 / 无烧录干扰）

| 捕获 | 窗口（UTC） | 字节 | 消息（长度 / 内容） |
|---|---|---|---|
| cap4 | 23:01:41.309Z → 23:06:41.510Z（含 `mdk_flash`，复位 23:01:54Z） | 289 | `BOOT … rst=0000`(50) + `S k=0 … t=0 h=0`(51, 烧录截断) + `BOOT … rst=0240`(50) + `S k=0 … y=0\r\n`(69) + `S k=3 … y=0\r\n`(69) |
| cap6 | 23:13:07.229Z → 23:18:07.447Z（无复位） | **69** | 单条 `S k=9 … y=0\r\n`（69）——窗口内**无任何其它字节** |
| W1（30 s） | 23:18:50 → 23:19:20 | **70** | 单条 `S k=12 … y=0\r\n`（68+2） |
| W2–W6、W8–W10（各 30 s） | 23:19:20 → 23:23:56 之间 | **0** | 无任何字节 |
| W7（30 s） | 23:21:54 → 23:22:24 | **70** | 单条 `S k=15 … y=0\r\n`（68+2） |

- cap4 第一条横幅 `rst=0000` 是**烧录开始前仍在 Flash 中的旧（EV-006 受测）镜像**所打：它紧跟一条被烧录过程中断的行（缺 `q/r/s/y`，共 51 B）；编程/校验完成后的新镜像则报 `rst=0240`。cap5 中“旧镜像”已不存在（已在跑修复镜像），其第一条横幅同样报 `rst=0040`，进一步证明该字段现在反映真实复位来源。
- **字节账目**：cap6 = 69 B = 恰好「一条字段完整 + CRLF」；W1/W7 = 70 B = 恰好一条；对照 EV-006 修复前 cap1 期望 194 B 实得 190 B（恰差 2×CRLF）——不再出现尾字节丢失。
- 唯一被截断的行只出现在 `mdk_flash` 下载序列停止 CPU 的瞬间，且截断位置与完整字段无关（EV-006 cap3 与本轮 cap4/cap5 均再现），属**烧录产物**，不计为固件缺陷。

### 2.3 字段完整性、行长与自洽（独立脚本复跑）

```
$ python evidence/embedded_verify_uart_trace_item001.py <cap4> <cap5> <cap6> <W1> <W7>
==== result: 74 checks, 10 failed ====
```

失败项全部为可解释的非缺陷项，逐一归因：

| 失败项 | 次数 | 归因 |
|---|---|---|
| `A BANNER rst nonzero` | 1 | cap4 中烧录前旧镜像的横幅（`rst=0000`），非当前受测件 |
| `A BANNER exists` | 3 | cap6/W1/W7 窗口内无复位，预期无横幅 |
| `B TERM` + `D FIELDS` + `D ORDER` | 6 | cap4/cap5 中各一条被 `mdk_flash` 停止 CPU 截断的行（烧录产物），每条同时触 TERM/FIELDS/ORDER 三项 |

未被干扰行的通过项（逐条）：`B TERM`（CRLF 终止）、`C LEN`（67–68 B ≤ 96 B）、`D FIELDS`/`D ORDER`（15 字段完整且顺序冻结）、`E RANGES`（`V==(o>0)`、`o<=8`、`n<=a<=x`、ADC 0..4095、枚举域）、`F DARK`（用实现的滞回语义独立重算 `D`，与实测一致）、`G REPORT`（用实现的判定函数重算 `r`，与实测一致）。

### 2.4 节拍回归（T-L4-01）

以修复后复位 T0 = 23:07:50Z（cap5 第二条横幅）为基准，`k` 为 RTC 累计 1 分钟节拍：

| 轨迹行 | 到达窗口 | 距 T0（按 60 s/拍推算） |
|---|---|---|
| k=0 | ≈T0（启动采样机会） | 0 |
| k=3 | cap5 窗口内（≤270 s） | ≈180 s |
| k=6 | 落在 cap5 结束与 cap6 开始之间的空隙（23:12:20–23:13:07） | ≈360 s |
| k=9 | cap6 窗口内（23:13:07–23:18:07） | ≈540 s |
| k=12 | W1（23:18:50–23:19:20） | ≈660 s |
| k=15 | W7（23:21:54–23:22:24） | ≈840 s |

- 相邻两条 S 行的 `k` 增量**恒为 3**（一采样周期 = 3 个 1 分钟节拍）。
- 直接测得的样本间隔（k=12→k=15，窗口分辨率 30 s）：**154–214 s**，符合「约 3 分钟」；300 s 窗口内 0–2 条（cap6 为 1 条、W2–W6/W8–W10 为 0 条）排除 1 分钟节拍。
- 解释性观察（不影响本项判定）：非 POR 复位时 vendor `RTC_Init()` 提前返回（不清除、不重配 RTC），RTC 域跨复位继续计数，因此**调试器复位后首个节拍相位是随机的**，这正是 k=6 落在两次采集窗口之间空隙的原因；样本间隔本身仍为 3 拍。该行为属基线（本项未改），且现场上电（POR）路径不受影响。

### 2.5 关闭语义与副作用（实测部分）

- **采样间期零输出**：cap6 = 69 B（仅一条轨迹）；W2–W6、W8–W10 = 0 B → UART1 在打印后被关闭且无泄漏输出。
- **唤醒后重新初始化**：跨多个周期的 k=0/k=3/k=9/k=12/k=15 均有完整行输出 → 每次打印前重新打开 UART1。
- **不破坏软 I²C**：每个周期的光照采样均 `o=8`（PB04/PB05 通路正常）；且 `q=0` 在 k=0（首次关闭之前）即出现，与关闭无关。
- **不破坏 SWD**：目标已在打印多条轨迹之后再次 `mdk_flash` 成功并 `Verify OK`（本轮共 3 次成功下载）。
- **静态**：`grep` `USER/src` 无 `SYSCTRL_AHBPeriphReset`/GPIOA 时钟关闭；UART 操作仅 `debug_trace.c`；`debug_trace.c` 不写 `report_req`/前值/快照/`sample_flag`；AF 宏为 `AFRL_f` 位域写（只触发 PA05/PA06）。
- **Flash 读保护保持注释**：`main.c:219-220` 两行仍为注释。

## 3. 判定与局限

**判定：TEST_PASS（ITEM-001）**

- 本项全部可观测验收点均在真实目标上以原始字节证据复现（§2），EV-006 的两个缺陷已复验修复。
- 局限（如实记录）：(1) `serial_capture` 原始产物无逐字节时间戳，节拍以窗口边界与 `k` 增量表述（分辨率 30 s）；(2) 未构建 `SENSOR_DEBUG_UART=0` 的 A/B 对照（修复仅影响排空时机与复位标志锁存，无观测接口可用），“打印无副作用”以静态核查 + `G REPORT` 自洽为准；(3) 位级 I²C 时序/空口波形/功耗/绝对精度未测（仪器未提供，TD-002 §4.10 列为可选扩展诊断）。

## 4. 跨项观察（保持登记，不计入本项判定）

**OBS-1（仍存在）**：真实目标上 GXHT40 温湿度测量每个周期均失败——本轮所有 6 条未被干扰的 S 行（cap4 k=0/k=3、cap5 k=0/k=3、cap6 k=9、W1 k=12、W7 k=15）均为 `q=0`、`t=0`、`h=0`。与本项无关的论证同 EV-006 §7（在 k=0、首次关闭之前即失败；AF 配置不触 PA03/PA04；光照通路正常）。需由温湿度采集通路负责能力定位（器件/供电/上拉/驱动/总线）。

---

# 历史记录（EV-006：上一轮同一 ITEM-001；当轮判 TEST_FAIL，发现 D-ITEM001-1/D-ITEM001-2；已提交 ae74cf2）

# 嵌入式验证证据（EV-006 / ITEM-001 ↔ UART1 调试串口初始化与调试打印可在真实目标上观测）

状态：embedded_tester.embedded_verification 独立验证证据（当前队列项执行轮；不沿用历史结论）
本轮验证对象：当前队列项 **ITEM-001**（`UART1 调试串口初始化与调试打印可在真实目标上观测`）
受测提交：`fe7acb4`（HEAD，工作树干净）
验证基线：TD-002 **rev 4.0** `artifacts/test_design.md`（§4.3 L1 / §4.4 L2 为主）
结论：**TEST_FAIL** —— UART1 调试通道在真实目标上**确实可观测**（启动横幅 + 每周期 S 轨迹实际输出、字段完整、行长 70 B ≤ 96 B、纯整数、字段与固件实际状态自洽、采样间期零输出、关闭语义未破坏软 I²C 与 SWD，Flash 读保护保持注释），但本项范围内发现 **2 个实现缺陷**，故不判通过：

| 缺陷 | 摘要 | 证据 |
|---|---|---|
| **D-ITEM001-1** | 每条 S 轨迹行末尾的 **CR+LF 两个字节未发出** → 相邻行首尾粘连、行无终止符，违反本项「无截断」与冻结行格式（每行应以 CRLF 结束） | §4、§5（cap1/cap2/cap3 原始字节与字节计数） |
| **D-ITEM001-2** | 启动横幅 `rst=` 字段**恒为 `0000`**，不能反映复位来源（RTC 初始化在横幅读取复位标志之前已清除全部复位标志） | §4、§6（3 次复位均为 `rst=0000`）+ 源码定位 |

跨项观察（**不计入本项判定**，登记交接）：真实目标上 GXHT40 温湿度测量**每个周期均失败**（轨迹 `q=0`、`t=0`、`h=0`，见 §7）。该失败与本项 UART1 改动无因果（见 §7 论证），属温湿度采集通路负责能力。

---

## 1. 环境与工具（本轮实际调用，原始输出）

| 工具 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `[{"port":"COM26",..."vid":49745,"pid":61441,"serial_number":"87094109484987710672FF50"},{"port":"COM42","description":"Prolific USB-to-Serial Comm Port (COM42)","vid":1659,"pid":8963}]` | 配置候选口 **COM42 在线**（传感器 UART1）；COM26 为下载探针 CDC |
| `usb_list_devices` | 含 `USB\VID_C251&PID_F001`（HID MI_02 + Ports MI_00，SN 8709…，Status OK） | **下载调试探针在线** |
| `mdk_build rebuild` | `V6.24`；`Program Size: Code=14020 RO-data=620 RW-data=76 ZI-data=1644`；`0 Error(s), 1 Warning(s)`（`main.c(209): warning: while loop has empty body`——**基线既有**空体 `while(k--);`，非本项新增） | 当前提交可构建 |
| `mdk_flash` ×2 | 两次均 `Erase Done.Programming Done.Verify OK.Application running ...` | 受测镜像已下载并回读校验；第二次证明 SWD 未被调试通道破坏 |

产物：`MDK/output/exe/Project.axf`，md5 `0470e98868e14b8800ce3adb20f62ce7`（由 HEAD `fe7acb4` 源码构建）。

## 2. 选测项与理由

本项是「调试串口真实可观测」，因此**真实目标是主证据**，宿主机 harness 只能证明格式化与关闭调用的形状：

| 选测 | 理由 |
|---|---|
| 启动横幅（T-L2-01） | 本项明确要求「上电后初始化 UART1 并打印启动横幅（固件标识/UID/复位来源/串口参数）」 |
| 每周期 S 轨迹完整性/行长/字段顺序（T-L2-02/03/04） | 本项明确给出字段清单与 ≤96 B 预算；必须逐字段核对，不能只看「有输出」 |
| 字段自洽（T-L2-04/05） | 「字段与该周期的实际测量值与判定一致」是本项可观测性验收的核心，用冻结公式独立重算 |
| 采样间期零输出（T-L2-06） | 「打印后关闭串口」的可观测后果 |
| 关闭语义不破坏软 I²C / SWD（T-L2-07/08） | 本项明确「不得复位 GPIOA、不得关闭 GPIOA 时钟、不得改动 PA03/PA04 与 PA07/PA08」 |
| 唤醒后重新初始化（T-L2-09） | 本项明确「唤醒后重新初始化」 |
| 打印无业务副作用（T-L2-10） | 本项明确「调试打印不得改变采样、判定、发送结果与状态快照」 |
| Flash 读保护保持注释（T-L1-13） | 本轮需求明确「不得恢复 Flash 读保护来妨碍本轮调试」 |
| 节拍（T-L4-01，回归项） | 轨迹 `k` 字段是节拍的直接观测量，可顺带核对 3 分钟节拍未被调试通道改变 |

未选/未执行：位级 I²C 时序、空口波形、功耗电流、绝对温湿度精度（需逻辑分析仪/电流表/参考仪器，用户未提供，TD-002 §4.10 已列为可选扩展诊断，不作为本项通过前提）。

## 3. 构建与部署（T-L1-09 / T-L1-12）

```
# mdk_build rebuild
*** Using Compiler 'V6.24', folder: 'C:\Keil_v5\ARM\ARMCLANG\Bin'
Rebuild target 'Project'
... compiling debug_trace.c ...
Program Size: Code=14020 RO-data=620 RW-data=76 ZI-data=1644
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).

# mdk_flash {}   (第一次, 2026-10-07T22:35:30Z → 22:35:39Z)
Load "...\MDK\output\exe\Project.axf"
Erase Done.Programming Done.Verify OK.Application running ...
Flash Load finished at 06:35:39

# mdk_flash {}   (第二次, 22:47:41Z → 22:47:52Z, 在目标已连续打印多条 S 轨迹之后)
Erase Done.Programming Done.Verify OK.Application running ...
Flash Load finished at 06:47:51
```

- 部署成功且回读校验一致；第二次下载成功证明 **SWD（PA07/PA08）未被调试通道破坏**（T-L2-08 通过）。
- 按能力约定：下载校验成功**不等于**业务功能通过，故继续做串口观测。

## 4. 真实目标 UART1 观测（COM42 @9600，原始结果）

三次 `serial_capture`（`{port:COM42, baudrate:9600, duration_sec:300}`）；第一次在捕获窗口内执行 `mdk_flash` 使复位落在窗口内，以捕获启动横幅。

### 4.1 捕获 1（窗口 22:35:28.111Z → 22:40:28.336Z，190 字节）

```
BOOT fw=FD-002r4 uid=6A002C00 rst=0000 uart=9600\r\n
S k=0 p=0 H=0 V=1 o=8 n=1541 x=1543 a=1541 D=1 t=0 h=0 q=0 r=0 s=0 y=0
S k=3 p=0 H=0 V=1 o=8 n=1387 x=1389 a=1388 D=1 t=0 h=0 q=0 r=0 s=0 y=0
```

原始十六进制（关键尾部，证明第 1 条 S 行后**没有 0D 0A**）：

```
00000060: 2074 3d30 2068 3d30 2071 3d30 2072 3d30
00000070: 2073 3d30 2079 3d30 5320 6b3d 3320 ...   <- "y=0" 紧跟 "S k=3"
```

### 4.2 捕获 2（窗口 22:41:44.339Z → 22:46:44.564Z，70 字节）

```
S k=9 p=0 H=0 V=1 o=8 n=1118 x=1119 a=1118 D=1 t=0 h=0 q=0 r=0 s=0 y=0
```

300 s 窗口内**仅一行、无其它字节** → 采样间期串口零输出（UART 已关闭，T-L2-06 通过）。

### 4.3 捕获 3（窗口 22:47:16.795Z → 22:52:16.993Z，356 字节；窗口内含第二次 `mdk_flash`）

```
S k=12 p=0 H=0 V=1 o=8 n=1007 x=1010 a=1008 D=1 t=0 h=0 q=0 r=0 s=0 y=0      <- 复位前旧镜像的最后一行, 无 CRLF
BOOT fw=FD-002r4 uid=6A002C00 rst=0000 uart=9600\r\n                            <- 下载序列复位#1
S k=0 p=0 H=0 V=1 o=8 n=999 x=1001 a=1000 D=1 t=0 h                          <- 下载过程中 CPU 被停止导致的行中截断(烧录产物, 非固件缺陷)
BOOT fw=FD-002r4 uid=6A002C00 rst=0000 uart=9600\r\n                            <- 下载/校验完成后的复位#2
S k=0 p=0 H=0 V=1 o=8 n=996 x=998 a=997 D=1 t=0 h=0 q=0 r=0 s=0 y=0          <- 无 CRLF
S k=3 p=0 H=0 V=1 o=8 n=906 x=909 a=907 D=1 t=0 h=0 q=0 r=0 s=0 y=0          <- 无 CRLF
```

说明：出现两条横幅与一条行中截断，是 `mdk_flash` 下载序列（擦除/编程/校验过程中停止并复位 CPU）叠加造成的**烧录产物**，不作固件缺陷；判定时只采用**未被烧录过程干扰**的 S 行。

### 4.4 节拍（T-L4-01，回归）

以第一次复位「Application running」时刻 T0≈22:35:39Z 为基准：

| 轨迹行 | 到达窗口（UTC） | 距 T0 | 期望（3 min/样本） |
|---|---|---|---|
| k=0 | ≈T0（启动采样机会） | ≈0 s | 上电即采样 |
| k=3 | (22:35:28, 22:40:28) | ≤289 s | 180 s |
| k=6 | (22:40:28, 22:41:44) | 289–365 s | 360 s |
| k=9 | (22:41:44, 22:46:44) | 365–665 s | 540 s |
| k=12 | (22:47:16, 22:52:16) | 697–997 s | 720 s |

- 相邻两条 S 行的 `k` 增量**恰好为 3**（1 分钟 RTC 节拍 × 3）→ 一个采样周期 = 3 拍。
- 用最紧的界 k=6 ∈ (289,365) s 推算：6 拍耗时 289–365 s → 单拍 48.2–60.8 s → **一个采样周期（3 拍）∈ (144.5, 182.5) s**，排除 1 分钟节拍，符合「约 3 分钟」。RTC 源为 LSI，实测离散度与偏差按 TD-002 要求记录，不自行设定容差。
- 第二次复位 T1≈22:47:51Z：k=0 于 T1，k=3 落在 (T1, 22:52:16] 内（≤265 s），与 3 分钟节拍一致。

### 4.5 启动横幅（T-L2-01）

- 出现时机正确：复位后立即出现，之后才是周期 S 行；同一板多次复位 UID 前 4 字节稳定为 `6A002C00`。
- 字段完整：`fw=FD-002r4`、`uid=` 8 位十六进制、`rst=`、`uart=9600`。
- 9600 下全程无乱码（所有字节均可 ASCII 解码）。
- **缺陷 D-ITEM001-2**：`rst` 三次复位均为 `0000`（见 §6）。

## 5. 独立检查明细（可复跑脚本 `evidence/embedded_verify_uart_trace_item001.py`）

```
$ python evidence/embedded_verify_uart_trace_item001.py <cap1.bin> <cap2.bin> <cap3.bin>
==== result: 57 checks, 13 failed ====
```

失败项全部属于以下三类，均已定位：

| 检查 | 结果 | 说明 |
|---|---|---|
| `A BANNER rst nonzero` | **FAIL ×3** | 三次横幅 `rst=0000` → 缺陷 D-ITEM001-2 |
| `B TERM S#n`（行以 CRLF 终止） | **FAIL ×7**（cap1 2 条、cap2 1 条、cap3 4 条；其中 cap3 含 1 条被烧录截断行） | 未被干扰的 S 行**全部**缺 CRLF → 缺陷 D-ITEM001-1 |
| `D FIELDS/D ORDER`（cap3 那条行中截断） | FAIL ×2 | 烧录过程停止 CPU 造成，非固件缺陷 |
| `A BANNER exists`（cap2） | FAIL | 该窗口内无复位，预期无横幅；不构成缺陷 |

字节计数（决定 D-ITEM001-1 的量化证据）：

```
捕获1: 期望(横幅48+CRLF) + (S行70+CRLF) + (S行70+CRLF) = 194 字节
        实际 = 190 字节            -> 恰好缺少 4 字节 = 2 条 S 行 × (CR+LF)
捕获2: S 行 70 字节, 未以 CRLF 结束 (窗口 300 s 内仅此 70 字节)
捕获3: S k=12 / S k=0(第二条) / S k=3 均未以 CRLF 结束; 横幅(未被紧接 flush) 的 CRLF 正常
```

通过的检查（节选，全部原始输出见脚本运行记录）：

- `D FIELDS/D ORDER`：未被干扰的每条 S 行含**全部 15 个冻结字段且顺序一致**（`k,p,H,V,o,n,x,a,D,t,h,q,r,s,y`）。
- `C LEN`：行长 67–71 字节，**≤ 96 字节预算**；纯整数、无 `%f`、无乱码。
- `E RANGES`：`V==(o>0)`、`o<=8`、`n<=a<=x`、`o==0 ⇒ n=x=a=0`、`s∈{0,1,2}`、ADC 0..4095 —— 全部自洽。
- `F DARK`：用实现的光照判定独立重算（上电初态 LIT；`o==0` 时代码 = 4095；明→暗需 code≥350、暗→明需 code≤250），每条 S 行的 `D` 与重算一致。
- `G REPORT`：用实现的判定函数独立重算 `r = (t>350) or (t!=350 and H and D and (p-t)>9)`，每条 S 行的 `r` 与重算一致 ⇒ 轨迹字段确实反映固件本周期状态（非常量、非伪造）。
- `H ADVANCE`：因本场景温湿度采样全部失败（`q=0`），该状态推进分支未被激励（见 §7 局限）。

## 6. 静态与结构核查（本项边界条款）

| 条款 | 方法 | 结果 |
|---|---|---|
| UART1 与 PA05/PA06 所有权唯一 | `grep` 全部 `USER/src/*.c` 的 `UART_SendData/UART_Init/CW_UART1/DEBUG_UARTx` | 仅 `debug_trace.c` 命中 |
| 关闭只复位/关闭 UART1、改写 PA05/PA06；**不得复位 GPIOA / 关 GPIOA 时钟** | `grep -rn "SYSCTRL_AHBPeriphReset\|AHB_PERIPH_GPIOA, DISABLE\|AHBPeriphClk_Enable(SYSCTRL_AHB_PERIPH_GPIOA" USER/src USER/inc` | **零命中**（旧 `measure.c` 中会复位 GPIOA 的 `DebugUART_Close()`/`UART1_Configure()` 已删除） |
| 不得改动 PA03/PA04、PA07/PA08 | AF 宏为位域写（`CW_GPIOA->AFRL_f.AFR5/AFR6`），只触及 PA05/PA06 | 结构上不触及 I²C/SWD 引脚 |
| 关闭语义的实际后果（软 I²C） | 每次采样光照成功（`o=8`）；且 `q=0` 在**首次 UART 关闭之前**的 k=0 行已出现 | 温湿度失败并非关闭串口所致（详见 §7） |
| 关闭语义的实际后果（SWD） | 目标连续打印多条轨迹后再次 `mdk_flash`：`Verify OK` | 通过 |
| 打印无业务副作用 | `grep` `debug_trace.c` 是否写 `report_req/s_prev_temp_x10/report_temp/sample_flag/s_last_hum`：**零命中**；业务状态仅在 `measure.c` 写 | 结构上无副作用；并有 `G REPORT` 自洽佐证 |
| Flash 读保护保持注释 | `grep main.c`：`211://	__SYSCTRL_FLASH_CLK_ENABLE(); ... 212://	FLASH_SetReadOutLevel(FLASH_RDLEVEL2);` | 两行仍为注释（未恢复读保护） |
| 工程文件列表 | `grep debug_trace MDK/Project.uvprojx EWARM/project.ewp` | 2 处 / 1 处命中，已加入两套工程 |
| 唤醒后重新初始化 | 多个周期各自完整输出（capture3 有 k=0 与 k=3 两条） | 通过 |
| 打印位于判定与发送之后 | `main.c`：`temperature_process()` → `send_data_to_gateway()` → `sensor_trace_fetch()`/`debug_trace_sample()` | 结构符合 |

### 缺陷 D-ITEM001-2 的根因定位

`main.c` 调用顺序：`SYSCTRL_Configuration()` … → **`RTC_Configuration()`** → `DIGITALSIGN_GetChipUid()` → … → **`debug_trace_boot(mcu_uid, SYSCTRL_GetAllRstFlag())`**。
而 `RTC_Configuration()` 内的 `RTC_Init()` 在 vendor 库中对**每一个分支**都会清除全部复位标志：

```c
/* CW32L010_StandardPeripheralLib_V1.0.5/Libraries/src/cw32l010_rtc.c:84-88 */
if ((SYSCTL_GetAllRstFlag() & SYSCTL_RESETFLAG_POR) != SYSCTL_RESETFLAG_POR) {
    SYSCTRL_ClearRstFlag(SYSCTL_RESETFLAG_ALL);
    return SUCCESS;
}
...
/* :98 */ SYSCTRL_ClearRstFlag(SYSCTL_RESETFLAG_ALL);
```

因此横幅读到的 `SYSCTL_GetAllRstFlag()` 在任何复位原因下都已被清零 → `rst=0000` 恒定。这是实现缺陷（本能力不修改实现）。

### 缺陷 D-ITEM001-1 的根因分析（供实现参考）

- `debug_trace_sample()` 末尾确实写 `\r\n`，但 `main.c` 紧接着调用 `debug_trace_flush_close()`；后者等 `UART_FLAG_TC` 后立刻 `SYSCTRL_APBPeriphReset1(UART1)` 并关时钟。
- 现象定量：每行正好丢最后 2 字节（CR、LF），且**横幅的 CRLF 正常**（横幅之后并未立即 flush/close，字节有充足时间移出）。
- 与 vendor 库对照：`UART_SendString()` 用的是排空等待 `while (UART_GetFlagStatus(UARTx, UART_FLAG_TXBUSY) == SET);`（`cw32l010_uart.c:90`），而 `UART_FLAG_TC` 是可清除标志（vendor LIN 示例使用 `UART_ClearFlag(CW_UART1, UART_FLAG_TC)`）。若 TC 在一次发送后保持置位而不被清除，则后续 `TC` 等待即为空操作，随后复位 UART1 会中断仍在移位的最后两字节。
- 结论：需由实现能力改为可靠排空（如等待 `TXBUSY` 清零，或正确清除/使用 TC）后再关闭；判定与修复均不属本能力。

## 7. 跨项观察（登记交接，不计入本项判定）

**OBS-1：真实目标上 GXHT40 温湿度测量每个周期均失败。** 轨迹中 `q=0`、`t=0`、`h=0` 出现在全部 6 条未被干扰的 S 行（capture1 的 k=0/k=3、capture2 的 k=9、capture3 的 k=12/k=0/k=3），`q=0` 即 `gxht40_measure() != GXHT40_OK`。

与本项无关的论证：

1. `q=0` 在 **k=0（上电首次采样）**就已出现，而此时**尚未发生任何 `debug_trace_flush_close()`**（第一次关闭发生在首条 S 行之后）；
2. UART1 的 AF 配置为位域写（`AFRL_f.AFR5/AFR6`），不触碰 PA03/PA04；`bsp_i2c_init()` 在首次采样时另行把 PA03/PA04 配为 OD；
3. 光照通路（PB04/PB05，另一个 GPIO 组）每周期均成功（`o=8`），说明 GPIOA 时钟与端口未被关闭破坏。

故按职责登记为温湿度采集通路（GXHT40 驱动/硬件/供电）问题，供负责能力与该项的独立验证处置；本能力不修改实现，也不据此改变本项判定。

## 8. 判定、局限与交接

**判定：TEST_FAIL**

- 通过：真实目标可观测（横幅 + 周期轨迹）、字段完整/顺序冻结、行长 ≤96 B、纯整数无乱码、字段与固件状态自洽、采样间期零输出、关闭语义不破坏软 I²C 与 SWD、Flash 读保护保持注释、工程文件列表已更新、节拍回归正常。
- 不通过：**D-ITEM001-1**（S 行缺 CRLF → 截断/行粘连）与 **D-ITEM001-2**（`rst` 恒为 0000 → 复位来源无效）。两项均属本项明确要求的可观测性与行格式，故不判 TEST_PASS、也不下调要求。

**局限（如实记录）**：

1. 每次 `serial_capture` 原始产物**不含逐字节时间戳**，节拍用「分区窗口 + `k` 增量」界定（最小窗口 300 s，分辨率即窗口宽度）；已在 §4.4 给出区间而非伪精确值。
2. 本场景温湿度采样全失败，故 `H ADVANCE`（成功推进前一有效温度）与 `r=1/send` 路径**未被激励**；这两条属后续队列项（光照判据/上报判定/冻结发送）的验证范围。
3. 未做 `SENSOR_DEBUG_UART=0` 的 A/B 对照实板构建（需额外构建/烧录，且当前温湿度路径全失败使其无法区分）；「打印无业务副作用」以结构核查 + `G REPORT` 自洽为准。
4. 位级 I²C 时序、空口波形、功耗、绝对温湿度精度未测（仪器未提供，TD-002 §4.10 已列为可选扩展诊断）。

**交接**：

- 实现能力：修复 D-ITEM001-1（关闭前排空策略）与 D-ITEM001-2（在清除前锁存复位标志，或不在横幅前清除）。修复后需重新部署并按本证据 §4/§5 方法复验（脚本 `evidence/embedded_verify_uart_trace_item001.py` 可直接复跑）。
- 温湿度采集通路负责能力：OBS-1（真实目标 GXHT40 每周期失败）需定位（器件/供电/上拉/驱动/总线）。
- 后续嵌入式验证：本项通过后，光照 1/3 判据、条件上报方向与边界、待上报冻结等队列项仍需按 TD-002 rev 4.0 执行。

---

# 历史记录（EV-005：上一轮工作流同一 ITEM-001 / T1；当轮无 UART trace 输出、判 BLOCKED；已提交 7d6d5ec）

# 嵌入式验证证据（EV-005 / ITEM-001 ↔ T1 GXHT40 温湿度采集通路）

状态：embedded_tester.embedded_verification 独立验证证据（同一任务的**重新执行轮**：本轮环境新增了调试探针，按要求重新核查前置条件，未沿用上一轮结论）
本轮验证对象：任务队列 **ITEM-001（= 任务清单 T1：GXHT40 温湿度采集通路）**
受测提交：`f59b656`（工作树干净；上一轮 EV-004 的提交为 `c8200cf`）
验证基线：TD-002 **rev 3.0** `artifacts/test_design.md`
结论：**BLOCKED** —— 本轮新增并完成了**真实目标部署验证**（`mdk_flash`：Erase Done / Programming Done / **Verify OK** / Application running，当前交付件已下载并校验），宿主侧独立验证全部复现通过（含定向变异负对照）；但 T1 的**强制真实目标功能观测**（TD-002 §2.2：真实 I²C 事务抓包 T-L2-01/02/03、与参考温湿度计对照 T-L2-04/05、故障注入 T-L2-06/07/08/09）仍**不可执行**：探针只能由 Keil 的 ULINK2 驱动（`UL2CM3.DLL`）访问，OpenOCD/pyocd 均无法读 DP，且固件无 UART trace 输出（`SENSOR_TEST_TRACE=0` 且未接入打印点），无逻辑分析仪/参考温度源。故不判 TEST_PASS（未发现实现缺陷，也不判 TEST_FAIL）。

## 1. 环境（本轮实际重新核查，原始输出）

| 工具 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports` | `[{"port":"COM26","description":"USB 串行设备 (COM26)","vid":49745,"pid":61441,"serial_number":"87094109484987710672FF50"},{"port":"COM42","description":"Prolific USB-to-Serial Comm Port (COM42)","vid":1659,"pid":8963}]` | 与上一轮（`[]`）**已不同**：现有两个串口 |
| `usb_list_devices` | 含 `USB\VID_C251&PID_F001`（HID MI_02 + Ports MI_00，SN 8709…）、Prolific `VID_067B&PID_2303` | 新增调试探针 + USB-TTL |
| `pyocd list` | `0  P&S CW-DAPLINK(CMSIS-DAP)  87094109484987710672FF50  n/a` | 探针身份确认（CW-DAPLINK / CMSIS-DAP） |
| Keil 工程调试配置 | `Flash2 = BIN\UL2CM3.DLL`、`FlashDriverDll = UL2CM3(...)`、`DriverSelection=4096` | Keil 实际使用 **ULINK2 驱动**（不是 CMSIS-DAP 通路） |

## 2. 本轮完成的真实目标动作：部署 + 校验（T-L1-09）

```
$ mdk_flash {}          # 工具 CLI（先 build 再 download，使用工程保存的调试器/Flash 算法）
{"tool":"mdk_flash","success":true,"exit_code":0,"stdout":
 "build_log:\n*** Using Compiler 'V6.24' ... Build target 'Project' ... 0 Error(s), 0 Warning(s).\n
  flash_log:\nLoad \"...\\MDK\\output\\exe\\Project.axf\" \n
  Erase Done.Programming Done.Verify OK.Application running ...\nFlash Load finished at 21:45:49"}
```

- 当前交付件（`MDK/output/exe/Project.axf`，md5 `c4861a3158a88f225d27ad915a27d918`；由 HEAD `f59b656` 源码经 `mdk_build rebuild` 0 Error/0 Warning 构建）已**下载并 Verify OK**，随后应用运行。
- 该证据只证明“当前镜像被下载且回读校验一致”（T-L1-09；T-L1-12 的“部署件=受测提交”由源码树干净 + axf 来源提交支撑）；**下载校验成功不等于业务功能通过**（能力约定）。

## 3. 真实目标**功能**观测尝试（均未成功，故不能证明 T1 行为）

### 3.1 串口（固件无 trace 输出）

```
# PowerShell System.IO.Ports 直接采集（不依赖 pyserial；install_python_package 本轮返回 "Python package installation is disabled"）
PORT=COM42 BAUD=9600 BYTES=0        # 12 s
PORT=COM26 BAUD=9600 BYTES=0        # 12 s
```

源码核对：`measure.c` 仅定义 `UART1_Configure()`/`DebugUART_Close()`，无活动发送调用；`main.c` 中的 `UART1_Configure()/printf` 均为注释；`sensor_config.h` `SENSOR_TEST_TRACE=0` 且未接入打印点。**结论：串口不能观测 GXHT40 内部结果。**

### 3.2 调试探针读内存（读驱动结果/探测地址）

计划：读 `tempvalue`(0x2000049c)、`huminityvalue`(0x20000468)、`report_req`(0x20000478)、`s_addr7`(0x2000047b)、`s_prev_temp_x10`(0x2000048a)。

```
$ pyocd commander -t cortex_m --elf MDK/output/exe/Project.axf -M attach --no-config -c "read16 0x2000049c"
0001146 E Error while initing target: No ACK received [commander]
# connect=attach / pre-reset / under-reset / halt、-f 500k、-O cmsis_dap.prefer_v1=True 均同样报错

$ openocd -f interface/cmsis-dap.cfg -f <自定义 cortex-m SWD 目标> -c "init" -c "halt" -c "reg pc"
Info : CMSIS-DAP: FW Version = 1.0 ; Interface Initialised (SWD)
Error: Error connecting DP: cannot read IDR
# 100/500/1000/2000/5000 kHz、hid 后端、connect_assert_srst 均同样报错
```

原因：Keil 工程用 `UL2CM3.DLL`（ULINK2 驱动）访问该探针；本能力可用的 OpenOCD 0.12.0（cmsis-dap）与 pyocd 0.36.0 只能走 CMSIS-DAP 通路，读 DP 即失败。因此**无法在真实目标上读取驱动输出、探测地址或设置断点**。

## 4. TD-002 §2.2 T1 强制真实目标项状态

| 用例 | 所需资源 | 本轮状态 | 依据 |
|---|---|---|---|
| T-L2-01 地址/0xFD/6B 线上时序 | 逻辑分析仪 | **未执行** | 无逻辑分析仪工具/仪器 |
| T-L2-02 tMEAS 下界 | 逻辑分析仪 | **未执行** | 同上 |
| T-L2-03 6 字节 ACK/NACK 时序 | 逻辑分析仪 | **未执行** | 同上 |
| T-L2-04 参考温湿度计对照 | 可控温源+参考仪器+可观测读数 | **未执行** | 无可控温源/参考仪；串口无输出、调试读内存不可用 |
| T-L2-05 负温端到端 | 冷箱+网关/手机 | **未执行** | 同上 |
| T-L2-06/07 CRC 错与总线卡死恢复 | 故障注入+逻辑分析仪 | **未执行** | 无仪器，且不可操作探头 |
| T-L2-08 无器件 | 取下器件+电流表 | **未执行** | 无电流表；不可观测 |
| T-L2-09 地址变体 | 两种变体实物 | **未执行** | 无实物 |

## 5. 宿主机独立验证复现（本轮重新运行，当前交付件）

```
$ gcc ... test/host_gxht40_verify_ev.c USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c -o gxht40_verify.exe && ./gxht40_verify.exe
==== result: 42 passed, 0 failed ====   exit=0
$ sed 's/...buf\[5\]/...buf\[2\]/' USER/src/gxht40.c > mutA.c   # 湿度字 CRC 比较字节定向变异
$ ./mutA.exe ; echo exit=$?
==== result: 27 passed, 15 failed ====   exit=1        # 负对照：用例有效
$ ./host_sf_i2c_verify_ev.exe   -> 35 passed, 0 failed   exit=0
$ ./host_fw_core_verify_ev.exe  -> 53 passed, 0 failed   exit=0   （T1 相关：CRC-8/换算/量程子集）
$ ./host_gxht40_check.exe       -> 27 passed, 0 failed
$ sh test/build_test.sh         -> 38 passed, 0 failed ; 24 passed, 0 failed
$ sh gcc/build.sh               -> FLASH 32208 B / 64 KB (49.15%) ; RAM 1712 B / 4 KB (41.80%)
```

T1 三个翻译单元（`gxht40`/`sf_i2c`/`fw_core`）浮点引用为 0（全 ELF 的浮点符号仅来自 `um2005C.o`/`cw32l010_uart.o`/`cw32l010_adc.o` 等既有代码）。

## 6. 判定与局限

1. **判定**：T1 可宿主机验证的行为（地址探测与缓存、0xFD、10 ms 等待、6 字节顺序、双字 CRC-8、x10 换算/截断/量程、读 NACK/CRC/无器件有界重试、失败不改输出、命令白名单）**未发现实现缺陷**；当前交付件已真实下载并 Verify OK。但 T1 的**真实目标功能观测**因仪器/工具条件不足未执行，故结果为 **BLOCKED**。
2. **新获得的可复现证据**：`mdk_flash` 部署 + Verify OK；探针身份与失败模式（pyocd/OpenOCD 无法读 DP，原因是工程使用 ULINK2 驱动）。
3. **可执行但未采用的手段（属于其它能力/需要改实现，未做）**：给固件接入 UART trace（`SENSOR_TEST_TRACE` 打印点）或做带串口输出的验证固件，均会改动产品实现/交付件，属固件实现能力，超出本能力边界。
4. **若要解锁本项**：需提供以下至少一项——逻辑分析仪（抓 PA03/PA04 I²C）或可被 OpenOCD/pyocd/GDB 驱动的调试通道（SWD DP 可读）或固件侧已被授权接入的调试输出；以及参考温湿度计/可控温源与（故障注入用例的）可操作夹具。
5. **后续任务项交接（不属本项）**：FD-002 rev 3.0 的 D-01/D-02/D-03（光照 1/3 判据与有效性、升温方向上报、待上报冻结）仍未实现，属任务项 ITEM-002/004/005；T1 本身无变更。

---

# 历史记录（EV-004：上一轮工作流同一 ITEM-001；当轮无探针，环境为空；已提交 c8200cf）

# 嵌入式验证证据（EV-004 / ITEM-001 ↔ T1 GXHT40 温湿度采集通路）

状态：embedded_tester.embedded_verification 独立验证证据
本轮验证对象：任务队列 **ITEM-001（= 任务清单 T1：GXHT40 温湿度采集通路）**
受测提交：`f59b656`（firmware_engineer.firmware_implementation 的 IMPLEMENTED 提交；本轮开始时 tracked 工作树干净，仅有无意落盘的 MDK 构建日志，已在记录原始输出后删除）
验证基线：TD-002 **rev 3.0** `artifacts/test_design.md`（本能力设计，`n004`）
上游输入：`artifacts/firmware_implementation.md`（FWI-002 rev 3.0 / ITEM-001 节）、`artifacts/firmware_design.md`（FD-002 rev 3.0 §2.2/§6.1/§6.2/§10）、`artifacts/firmware_requirements.md`（FWR-101/107）、`artifacts/interface_contract.md`（IC-002 v3.0 §3）、`gxht40.pdf`
结论：**BLOCKED** —— 宿主机侧独立验证全部通过（含独立参考帧与定向变异负对照，未发现实现缺陷），但 TD-002 rev 3.0 §2.2 对 B1/T1 **强制要求**的真实目标观测（T-L2-01/02/03 实板 I²C 抓包；T-L2-04/05 参考温湿度计对照；T-L2-06/07/08/09 真实故障注入与地址变体）在本环境**不可执行**：无调试探针、无串口、无逻辑分析仪。按能力约定「required real-target work not performed is BLOCKED, not a pass with a handoff note」，本项不判 TEST_PASS，也不判 TEST_FAIL（未发现缺陷）。

> 说明：本能力工具面本轮新增 `mdk_build`/`mdk_flash`/`serial_*`。已按要求**重新核查**环境（不沿用上一轮的失败结论），核查结果见 §1：仍无探针/串口。

---

## 1. 环境与工具（本轮实际调用，原始输出）

| 工具调用 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports {}` | `{"tool":"serial_list_ports","success":true,"exit_code":0,"stdout":"[]", ...}` | **无任何串口**（配置候选 `COM43` 不在位） |
| `usb_list_devices {}` | `Chicony USB2.0 Camera`、`Logitech HID VID_046D:PID_C52F`、USB3 根集线器、`Intel Bluetooth VID_8087:PID_0033` | **无 SWD/调试探针**（无 CMSIS-DAP/J-Link/WCH-Link）、无 USB-TTL |
| `serial_capture {port:COM43,baudrate:9600,duration_sec:3}` | `RuntimeError: pyserial is required for serial_capture` | 串口采集不可用（双前置缺失：无端口 + 运行时缺 pyserial） |
| `mdk_build {action:build}` | `Using Compiler 'V6.24' ... 0 Error(s), 0 Warning(s)` | 工程可构建；但 `.axf` mtime=Oct 3（源码自 Oct 3 未变），需强制 rebuild 才有编译证据 |
| `mdk_build {action:rebuild}` | `Rebuild target 'Project' ... compiling gxht40.c ... compiling fw_core.c ... linking ... Program Size: Code=8498 RO-data=598 RW-data=76 ZI-data=1620 ... 0 Error(s), 0 Warning(s)` | **当前源码在 ARMCLANG V6.24 下全量重编译 0 错 0 警** |
| `mdk_flash {}` | `Load "...Project.axf"` → `Internal DLL Error` / `Error: Flash Download failed  -  Target DLL has been cancelled`（`success:false, exit_code:2, error_code:FLASH_LOG_ERROR`） | **无调试器，无法下载/回读校验**（T-L1-09/T-L1-12 BLOCKED） |

工具副作用：`mdk_build`/`mdk_flash` 在仓库根产生 `build.log`/`build_*.log`，已读入本证据后删除，未入库。

---

## 2. 选测项与理由

任务 ITEM-001 验收文字逐条拆成可观测行为，并按 TD-002 rev 3.0 选取手段。

| 任务验收行为 | TD-002 rev 3.0 用例 | 本轮实际执行（宿主机） | 强制真实目标（未执行） |
|---|---|---|---|
| 探测 0x44/0x45（8bit 0x88/0x89/0x8A/0x8B）带缓存 | T-L0-07 / T-L2-01 / T-L2-09 | 位级 mock 总线 + 真实驱动：两地址、探测顺序、缓存后不再重扫 | 真实器件变体、上拉、每字节 ACK（T-L2-01/09） |
| 发 0xFD 高重复率命令 | T-L0-07 / T-L2-01 | 命令白名单：历史全部命令字节必须为 0xFD | 真实线上字节（T-L2-01） |
| 按 tMEAS 上限等待后读 6 字节 | T-L0-07 / T-L2-02/03 | 断言一次 `GXHT40_MEASURE_WAIT_MS`(10 ms)；6 字节顺序、前 5 ACK/末 NACK | STOP→读真实时间差 ≥8.3 ms（T-L2-02） |
| 温度字/湿度字分别 CRC-8(0x31,init 0xFF) | T-L0-01 | 独立 Python oracle 复算 + mock 帧双字 CRC 校验路径 | 真实器件 CRC 行为（T-L2-01/03） |
| 温度 x10 有符号 −400..1250 | T-L0-02 | oracle 全量边界 + 驱动有效性端点（−40.0/125.0 ℃） | 与参考温湿度计一致（T-L2-04/05） |
| 湿度 x10 截断 0..1000 | T-L0-03 | raw 0xFFFF→1000、raw 0→0 | 同上（T-L2-04） |
| 读 NACK / CRC 错 / 无器件有界重试 | T-L0-07 / T-L6-06 / T-L2-06/08 | mock 注入 NACK/CRC/无器件，断言上限与有界 | 真实 NACK 时序、故障注入恢复（T-L2-06/07/08） |
| 整次失败返回失败码且不改输出 | T-L0-07 | 每类失败码 + 输出哨兵值不变 | 实板失败路径（T-L2-06/08） |

**为何宿主机仍不可替代真实目标**：mock 从机是按手册重建的模型，只能证明驱动软件逻辑；真实器件 ACK/NACK、tMEAS、电气时序与实测值只有实板能证。TD-002 §2.2 因此把 B1 的真实目标观测列为强制项。

---

## 3. 宿主机独立验证结果（命令与原始结果）

### 3.1 独立驱动 harness（真实驱动 + 位级 mock 总线）

```
$ cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
$ gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast -Itest/mock_mcu -IUSER/inc -ICOMMON \
      test/host_gxht40_verify_ev.c USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c \
      -o /tmp/ev004/gxht40_verify.exe && /tmp/ev004/gxht40_verify.exe
[T1] manual reference frame 0xBEEF/0x1234
    PASS  returns GXHT40_OK / temp_x10=855, hum_x10=29 (manual vector)
    PASS  exactly one command byte 0xFD / write 0x88 / read 0x89 / cache=0x44
    PASS  one tMEAS wait of the configured 10 ms
[T2] 25.0C/50%->250/500 ; -10.0C/0%->-100/0 (sign) ; hum 0xFFFF->1000 ; hum 0->0
[T3] device only at 0x45: probe 0x88 then 0x8A, read 0x8B, cache=0x45; 2nd measure uses cache
[T4] 2x read NACK -> success; command NOT re-sent; 3 read attempts; two 1 ms waits
[T5] NACK beyond bound -> GXHT40_ERR_IO; outputs untouched; attempts bounded by MEAS_RETRY / MEAS_RETRY*READ_RETRY
[T6] temp-word CRC bad -> GXHT40_ERR_CRC; hum-word CRC bad -> GXHT40_ERR_CRC; outputs untouched; re-measured==3
[T7] raw 0 (-45.0C) -> GXHT40_ERR_RANGE ; 125.1C -> GXHT40_ERR_RANGE ; outputs untouched
[T8] no device -> GXHT40_ERR_NO_DEVICE (both 0x88/0x8A probed); outputs untouched; bounded
[T9] NULL temp / NULL hum / unbound bus -> GXHT40_ERR_PARAM
[T10] every command byte ever sent was 0xFD (23 total)
==== result: 42 passed, 0 failed ====   exit=0
```

说明：实现 ITEM-005 之后 `gxht40.c` 改为调用 `fw_core.c` 的 `fw_crc8_gxht`/`gxht40_raw_to_x10`，故本 harness 链接目标必须补 `USER/src/fw_core.c`（缺少时链接器报 `undefined reference to fw_crc8_gxht/gxht40_raw_to_x10`）。已据此更新 harness 头部注释中的可复现构建命令。

### 3.2 负对照（定向变异，证明用例不是空跑）

```
$ sed 's/fw_crc8_gxht(&buf\[3\], 2u) != buf\[5\]/fw_crc8_gxht(&buf[3], 2u) != buf[2]/' USER/src/gxht40.c > /tmp/ev004/gxht40_mutA.c
$ diff USER/src/gxht40.c /tmp/ev004/gxht40_mutA.c
125c125
<             (fw_crc8_gxht(&buf[3], 2u) != buf[5])) {
---
>             (fw_crc8_gxht(&buf[3], 2u) != buf[2])) {
$ gcc ... test/host_gxht40_verify_ev.c /tmp/ev004/gxht40_mutA.c USER/src/sf_i2c.c USER/src/fw_core.c -o /tmp/ev004/mutA.exe
$ /tmp/ev004/mutA.exe ; echo exit=$?
==== result: 27 passed, 15 failed ====   exit=1
```
→ 湿度字 CRC 比较字节被定向改错后 harness 失败 15 项且退出码 1，说明 42/42 的通过不是空跑。

### 3.3 相邻独立 harness（T1 相关软件面）

```
$ gcc ... test/host_sf_i2c_verify_ev.c USER/src/sf_i2c.c -o sfi2c_verify.exe && ./sfi2c_verify.exe
==== result: 35 passed, 0 failed ====   exit=0   （命令写/连续读线上字节、START/STOP、主机 ACK/NACK 位置、超时释放、既有函数回归）

$ gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast -Itest/mock_mcu -IUSER/inc -ICOMMON \
      test/host_fw_core_verify_ev.c USER/src/fw_core.c -o fwcore_verify.exe && ./fwcore_verify.exe
==== result: 53 passed, 0 failed ====   exit=0   （其中 T1 相关：65536 温度字/65536 湿度字换算与量程、256x256 双字节 CRC-8 与独立 Python 参考哈希比对）
```

注：该 fw_core harness 的光照/上报用例当前仍按**rev 2.0 语义**（滞回、降温方向）断言，对应当前实现的 T2/T4 行为；本项只取其中的 **CRC-8/换算/量程** 作为 T1 证据，光照与上报留待 ITEM-002/ITEM-004 验证。

### 3.4 maker 自检复跑（交叉确认，不作为独立证据）

```
$ gcc ... test/host_gxht40_check.c USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c -o gxht40_check.exe && ./gxht40_check.exe
==== result: 27 passed, 0 failed ====
$ gcc ... test/host_sf_i2c_bus_check.c USER/src/sf_i2c.c -o sfi2c_check.exe && ./sfi2c_check.exe
==== result: 15 passed, 0 failed ====
$ sh test/build_test.sh
== [1/2] fw_core pure logic ==== result: 38 passed, 0 failed ====
== [2/2] measure flow (mock MCU) ==== result: 24 passed, 0 failed ====   exit=0
```

---

## 4. 当前交付件构建与静态核查

```
$ sh gcc/build.sh        -> 0 errors ; FLASH 32208 B / 64 KB (49.15%) ; RAM 1712 B / 4 KB (41.80%) ; elf/hex/bin 生成
$ arm-none-eabi-size gcc/obj/sensor_fw.elf
   text    data     bss     dec     hex filename
  32208      84    1628   33920    8480 gcc/obj/sensor_fw.elf
$ md5sum gcc/obj/sensor_fw.elf gcc/obj/sensor_fw.hex gcc/obj/sensor_fw.bin
24425d8d0f593ae3846fa9164c65c716 *sensor_fw.elf
ddf2e2e7bac47a80ea00177cb104afc9 *sensor_fw.hex
47abc6dfc798c837b2b68916f29db543 *sensor_fw.bin

$ arm-none-eabi-nm gcc/obj/{gxht40,sf_i2c,fw_core}.o | grep -cE '__aeabi_f|__aeabi_d|__float'
gxht40: 0 ; sf_i2c: 0 ; fw_core: 0        （T1 三个翻译单元无浮点依赖）
# 全 ELF 仍有浮点符号，按对象归因仅来自：um2005C.o(12)、cw32l010_uart.o(7)、cw32l010_adc.o(6)（既有厂商/驱动代码，非 T1）

$ mdk_build {action:rebuild}  -> Rebuild target 'Project' ... 0 Error(s), 0 Warning(s)
   Program Size: Code=8498 RO-data=598 RW-data=76 ZI-data=1620
$ md5sum MDK/output/exe/Project.axf -> c4861a3158a88f225d27ad915a27d918
$ arm-none-eabi-nm MDK/output/exe/Project.axf | grep -E 'gxht40_measure|fw_crc8_gxht|light_sample|i2c_read_bytes'
00001370 T fw_crc8_gxht ; 000013dc T gxht40_measure ; 000017ae T i2c_read_bytes ; 00001a68 T light_sample
```

交付件一致性：`git rev-parse HEAD` = `f59b656eb7d2f3f1b9719397869e726b3e28cbb1`；`git status --porcelain` 对 tracked 源码为空（仅本能力自己的 harness 注释修改），即被编译/验证的源码就是受测提交。

---

## 5. 强制真实目标项（本轮未执行，均为 BLOCKED）

| TD-002 用例 | 所需资源 | 本轮状态 | 阻塞证据 |
|---|---|---|---|
| T-L2-01 地址/0xFD/6B 线上时序 | 逻辑分析仪（PA03/PA04）| 未执行 | 无逻辑分析仪工具/仪器；无探针 |
| T-L2-02 tMEAS 下界 | 逻辑分析仪 | 未执行 | 同上 |
| T-L2-03 6 字节 ACK/NACK 时序 | 逻辑分析仪 | 未执行 | 同上 |
| T-L2-04 与参考温湿度计对照 | 可控温源 + 参考仪器 + 串口/钩子 | 未执行 | 无串口（`[]`）、无温源、钩子未接入 |
| T-L2-05 负温端到端 | 冷箱 + 网关/手机 | 未执行 | 同上 + 无网关/手机确认 |
| T-L2-06 CRC 错误路径 | 故障注入 + 逻辑分析仪 | 未执行 | 无仪器 |
| T-L2-07 总线卡死恢复 | 故障注入 | 未执行 | 无仪器 |
| T-L2-08 无器件 | 取下器件 + 电流表 | 未执行 | 无仪器；无法下载镜像 |
| T-L2-09 地址变体 | 两种变体实物 | 未执行 | 无实物 |
| T-L1-09 量产工具链下载回读 | 调试探针 | 未执行 | `mdk_flash` = `Flash Download failed - Target DLL has been cancelled` |
| T-L1-12 部署镜像=受测提交 | 调试探针 | 未执行 | 无法下载/回读 |

---

## 6. 判定、局限与交接

1. **判定**：T1 的宿主机可验证行为（地址探测与缓存、0xFD、tMEAS 等待、6 字节读取、双字 CRC-8、x10 换算与量程、读 NACK/CRC/无器件有界重试、失败不改输出、命令白名单）**未发现实现缺陷**；真实目标强制项不可执行，故本轮结果为 **BLOCKED**（不是 TEST_FAIL，也不是 TEST_PASS）。
2. **真实目标前置**（供后续执行）：调试探针 + 可下载环境（当前 `Target DLL cancelled`）、`COM43` 串口或等效、逻辑分析仪、可控温源/参考温湿度计、GXHT40 地址变体实物；另 `serial_capture` 仍需在配置运行时安装 `pyserial`（本轮报 `pyserial is required`）。
3. **观测钩子**：TD-002 §2 期望的 `SENSOR_TEST_TRACE` 打印点仍未接入；当前 T-L2-04/05 的内部读数只能靠空口/钩子，建议在具备实板时先接入观测点。
4. **后续任务项交接（不属本项）**：FD-002 rev 3.0 §1.1 的 D-01/D-02/D-03 仍未实现（光照 1/3 判据与有效性、升温方向上报、待上报冻结样本），对应任务项 ITEM-002/004/005；本项 T1 不受其影响（FD-002 rev 3.0 明确 T1 无变更）。
5. **跨项 harness 维护交接（后续 ITEM-003/004/005 验证时）**：`test/host_light_verify_ev.c`（需补 `fw_core.h`）、`test/host_measure_flow_verify_ev.c`、`test/host_rf_report_verify_ev.c`、`test/host_rtc_cadence_verify_ev.c` 仍引用已退役的 `optcfg.h`/`hall.h`/`params.h`/`history.h` 或旧语义；本轮属 T1，未改动它们。

---

# 历史记录（EV-003：上一轮工作流 WF-d5575697，基线 TD-002 rev 2.0；仅供追溯，勿与本轮混用）


状态：embedded_tester.embedded_verification 独立验证证据
本轮验证对象：任务队列 **ITEM-001（= 任务清单 T1：GXHT40 温湿度采集通路）**
被测提交：`cbb2b61`（firmware_engineer.firmware_implementation 的 IMPLEMENTED 提交，工作树干净）
验证基线：`dc5980b`（本能力自己的 TD-002 rev 2.0 测试设计）
结论：**BLOCKED** —— 宿主机侧独立验证全部通过（可复现、含独立 oracle 与负对照），但本项目 TD-002 §2.2 对 T1 强制要求的**真实目标观测（实板 I²C 抓包 / 参考温湿度计对照 / 真实器件 NACK 与 tMEAS）在设备工具禁用下未执行**。按验证能力约定「required real-target work not performed is BLOCKED, not a pass with a handoff note」，**本项不判 TEST_PASS，也不判 TEST_FAIL（未发现实现缺陷）**，阻塞原因是环境不可用。

上游输入：`artifacts/firmware_implementation.md`（FWI-002 rev 3.0 / T1 声明）、`artifacts/test_design.md`（TD-002 rev 2.0）、`artifacts/firmware_design.md`（FD-002 rev 2.0 §2.2/§6/§10）、`artifacts/firmware_requirements.md`（FWR-101/107）、`gxht40.pdf`、任务队列 ITEM-001 描述。
本文件新增内容为第 1–9 节；上一轮工作流的 EV-002 原文作为历史记录附于文末（Git 历史同样可追溯）。

---

## 1. 选测项与理由

任务 ITEM-001 的验收文字（队列描述）逐条拆成可观测行为，并按 TD-002 rev 2.0 §2.2 强制真实目标要求选取验证手段：

| 任务验收行为 | TD-002 用例 | 本轮选测（宿主机） | 强制真实目标（未执行） |
|---|---|---|---|
| 探测 0x44/0x45 地址（8bit 0x88/0x89/0x8A/0x8B）带缓存 | T-L0-07 / T-L2-01 / T-L2-09 | 位级 mock 总线 + 真实驱动，两地址与缓存路径 | 真实器件变体确认、空闲上拉、每字节 ACK（T-L2-01/09） |
| 发 0xFD 高重复率命令 | T-L0-07 / T-L2-01 | 命令白名单：全部命令字节必须为 0xFD | 真实线上字节（T-L2-01） |
| 按 tMEAS 上限等待后读 6 字节 | T-L0-07 / T-L2-02/03 | 断言一次 `GXHT40_MEASURE_WAIT_MS`(=10 ms) 延时；6 字节顺序、前 5 ACK/末 NACK | STOP→读的真实时间差 ≥8.3 ms（T-L2-02） |
| 温度字/湿度字分别 CRC-8(0x31,init 0xFF) | T-L0-01 | 独立 Python oracle 复算 CRC；mock 帧 CRC 校验路径 | 真实器件 CRC 行为（T-L2-01/03） |
| 温度 x10 有符号且 -400..1250 | T-L0-02 | oracle 全量边界 + 驱动有效性判定（-40.0/125.0 端点） | 真实读数与参考温湿度计一致（T-L2-04/05） |
| 湿度 x10 截断 0..1000 | T-L0-03 | raw 0xFFFF→1000、raw 0→0 | 同上（T-L2-04） |
| 读请求 NACK / CRC 错 / 无器件有界重试 | T-L0-07 / T-L6-06 / T-L2-06/08 | mock 注入 NACK/CRC/无器件，断言上限与有界 | 真实 NACK 时序、故障注入恢复（T-L2-06/07/08） |
| 整次失败返回失败码且不改输出 | T-L0-07 | 每类失败码 + 输出哨兵值不变 | 实板失败路径（T-L2-06/08） |

**为什么必须在宿主机做**：上述逻辑在真实目标上不可观测（无稳定周期日志，UART 仅活动期打开且无打印），TD-002 §1.2 已把「内部判定」的观测手段定为钩子或 mock；mock 可确定性地覆盖协议分支与边界，是唯一可重复的手段。
**为什么仍不能判通过**：mock 从机是按手册重建的模型，**不是真实器件**；它只能证明驱动软件逻辑，不能证明真实器件 ACK/NACK、tMEAS、电气时序与实测值。TD-002 §2.2 因此把 B1 的真实目标观测列为强制项。

## 2. 环境与工具（本轮实际调用）

| 工具 | 版本/路径 | 用途 |
|---|---|---|
| MinGW-w64 gcc | 12.2.0（`/c/ProgramData/chocolatey/bin/gcc`） | 宿主机编译驱动/harness/oracle 对照 |
| arm-none-eabi-gcc | 10.3.1 20210824 | 当前交付件交叉编译、浮点引用核查 |
| python | 3.10.9 | 独立参考 oracle（CRC-8/换算/量程） |
| git | 仓库版本 | 受测提交/工作树一致性 |
| SWD/J-Link、串口、逻辑分析仪、示波器、电源/电流表 | **未调用**：设备工具已禁用；宿主机仅暴露 read/bash/write 类能力，无 programmer/jlink/serial/logic_analyzer 工具；环境内也不存在 SEGGER J-Link（`JLink.exe` 为 OpenJDK 的 Java jlink）与逻辑分析仪 CLI | —— |

**结论：本轮不可能执行真实目标验证；未假定任何物理测试已执行。**

## 3. 独立验证方法（不采信 maker 自述）

1. **独立参考 oracle（我自己新写，不复用驱动代码）**：`/tmp/ev003/gxht40_ref.py` 按 gxht40.pdf 事实从头实现 CRC-8(poly 0x31,init 0xFF,MSB-first) 与两条换算公式，用于**独立复算** mock harness 里硬编码的 6 字节期望帧与所有边界值；harness 不参与生成期望。
2. **位级 mock 总线 + 真实驱动**：编译**真实** `USER/src/gxht40.c` + `USER/src/sf_i2c.c` + `USER/src/fw_core.c`，挂到按 I²C 位时序重建的 mock 从机（`test/host_gxht40_verify_ev.c`，上一轮本能力编写的独立 harness）。
3. **负对照（变异测试）**：把驱动复制到临时目录做两处定向变异，确认 harness 会失败——证明用例「有牙齿」而非空跑。
4. **相邻原语独立 harness**：`test/host_sf_i2c_verify_ev.c`（总线原语）、`test/host_fw_core_verify_ev.c`（CRC-8/换算穷举）。
5. **交叉编译当前交付件**：`gcc/build.sh` + T1 三个翻译单元的浮点符号核查。
6. **交付件与受测提交一致性**：`git status` / `git diff HEAD` 确认所编源码即 HEAD。

## 4. 结果明细（命令与原始结果）

### 4.1 独立参考 oracle（期望值独立复算）
```
$ python /tmp/ev003/gxht40_ref.py
manual reference CRC(0xBEEF)=0x92 (datasheet expected 0x92)
F_MANUAL S_T=0xBEEF S_RH=0x1234 T_CRC=0x92 H_CRC=0x37 -> temp=855  hum=29   validT=True
F_ROOM   S_T=0x6666 S_RH=0x72B0 T_CRC=0x93 H_CRC=0xDC -> temp=250  hum=500  validT=True
F_NEG    S_T=0x3333 S_RH=0x0000 T_CRC=0x88 H_CRC=0x81 -> temp=-100 hum=0    validT=True
F_CLAMP  S_T=0x6666 S_RH=0xFFFF T_CRC=0x93 H_CRC=0xAC -> temp=250  hum=1000 validT=True
F_HUM0   S_T=0x6666 S_RH=0x0000 T_CRC=0x93 H_CRC=0x81 -> temp=250  hum=0    validT=True
F_RLOW   S_T=0x0000 S_RH=0x72B0 T_CRC=0x81 H_CRC=0xDC -> temp=-450 hum=500  validT=False
F_RHIGH  S_T=0xF8CA S_RH=0x72B0 T_CRC=0x32 H_CRC=0xDC -> temp=1251 hum=500  validT=False
valid temp raw range: [0x073E..0xF8C2] -> [-400..1250]
  OK x11 ; REFERENCE RESULT: PASS ; exit=0
```
→ 手册参考向量 `CRC(0xBEEF)=0x92` 复现；harness 内所有期望帧的 CRC/换算全部与独立 oracle 一致；有效域端点为 raw `0x073E`(-40.0 ℃) 与 `0xF8C2`(125.0 ℃)，`raw=0`→-450 与 `0xFFFF`→1300 均无效。

### 4.2 独立驱动 harness（真实驱动 + 位级 mock 总线）
```
$ gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast -Itest/mock_mcu -IUSER/inc -ICOMMON \
      test/host_gxht40_verify_ev.c USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c \
      -o /tmp/ev003/gxht40_verify.exe && /tmp/ev003/gxht40_verify.exe
[T1] 0xBEEF/0x1234 -> OK, temp=855 hum=29; cmd=0xFD x1; wr 0x88; rd 0x89; cache=0x44; one 10 ms wait
[T2] 25.0C/50% -> 250/500 ; -10.0C/0% -> -100/0 (sign) ; hum raw 0xFFFF -> 1000 ; hum raw 0 -> 0
[T3] device only at 0x45: probe 0x88 then 0x8A, read 0x8B, cache=0x45; 2nd measure uses cache (no 0x88)
[T4] 2x read NACK -> success; command NOT re-sent; 3 read attempts; two 1 ms waits
[T5] NACK beyond bound -> GXHT40_ERR_IO; outputs untouched; attempts <= MEAS_RETRY / MEAS_RETRY*READ_RETRY
[T6] temp-word CRC bad -> GXHT40_ERR_CRC; hum-word CRC bad -> GXHT40_ERR_CRC; outputs untouched; re-measured == 3
[T7] raw 0 (-45.0C) -> GXHT40_ERR_RANGE ; 125.1C -> GXHT40_ERR_RANGE ; outputs untouched
[T8] no device -> GXHT40_ERR_NO_DEVICE (both 0x88/0x8A probed); outputs untouched; bounded
[T9] NULL temp / NULL hum / unbound bus -> GXHT40_ERR_PARAM
[T10] every command byte ever sent was 0xFD (23 total)
==== result: 42 passed, 0 failed ====   exit=0
```

### 4.3 负对照（变异测试，证明 harness 有效）
```
$ diff USER/src/gxht40.c /tmp/ev003/gxht40_mutA.c   # 湿度字 CRC 比较错字节 buf[5] -> buf[2]
125c125  < (fw_crc8_gxht(&buf[3], 2u) != buf[5])   > (fw_crc8_gxht(&buf[3], 2u) != buf[2])
$ /tmp/ev003/mutA.exe ; echo $?          -> ==== result: 27 passed, 15 failed ====   exit=1
$ diff USER/src/gxht40.c /tmp/ev003/gxht40_mutB.c   # 湿度原始字低字节 buf[4] -> buf[2]
131c131  < ... | (uint16_t)buf[4]);       > ... | (uint16_t)buf[2]);
$ /tmp/ev003/mutB.exe ; echo $?          -> ==== result: 39 passed, 3 failed ====    exit=1
```
→ 两处定向变异均被 harness 检出（非 0 退出、失败计数 >0），说明 42/42 不是空跑。

### 4.4 相邻原语独立 harness
```
$ gcc ... test/host_sf_i2c_verify_ev.c USER/src/sf_i2c.c -o sfi2c_verify.exe && ./sfi2c_verify.exe
  ... (命令写/连续读线上字节、START/STOP、主机 ACK/NACK 位置、超时释放、既有函数回归)
==== result: 35 passed, 0 failed ====   exit=0
$ gcc -DSENSOR_CONFIG_NO_MCU ... test/host_fw_core_verify_ev.c USER/src/fw_core.c -o fwcore_verify.exe && ./fwcore_verify.exe
  ... (65536 温度字/65536 湿度字/256x256 CRC/滞回/上报组合 与独立 Python 参考哈希比对)
==== result: 53 passed, 0 failed ====   exit=0
```

### 4.5 maker 自检 harness（复现，作为交叉确认，不作为独立证据）
```
$ ... test/host_gxht40_check.c ... -> ==== result: 27 passed, 0 failed ====   exit=0
$ sh test/build_test.sh -> [1/2] ==== result: 38 passed, 0 failed ====
                            [2/2] ==== result: 24 passed, 0 failed ====   exit=0
```

### 4.6 交叉编译当前交付件 + 浮点核查
```
$ sh gcc/build.sh
== link ==
Memory region         Used Size  Region Size  %age Used
           FLASH:       32208 B        64 KB     49.15%
             RAM:        1712 B         4 KB     41.80%
== done: obj/sensor_fw.elf/.hex/.bin ==      exit=0
$ arm-none-eabi-nm obj/gxht40.o obj/sf_i2c.o obj/fw_core.o | grep -iE "float|__aeabi_[fd]|soft"
(no output)  -> T1 三个翻译单元无浮点/soft-float 引用（换算为整数运算）
```

### 4.7 交付件与受测提交一致性
```
$ git status --short          -> (空)
$ git diff --stat HEAD -- .../USER/src/gxht40.c .../USER/inc/gxht40.h  -> (空)
```
→ 所编译/所测源码即受测提交 `cbb2b61` 的 HEAD 内容，未拿旧镜像或旧包充当当前交付件。

## 5. 覆盖矩阵与剩余缺口

| 验收行为 | 宿主机独立验证 | 真实目标验证 | 判定 |
|---|---|---|---|
| 地址探测 0x44/0x45 + 缓存 | 通过（含 0x45 顺序与缓存复用） | **未执行**（真实变体、上拉、ACK） | 部分 |
| 0xFD 高重复率命令（不发热/不复位） | 通过（白名单：全部发送命令均为 0xFD） | **未执行**（真实线上字节） | 部分 |
| tMEAS 等待 + 6 字节读取 | 通过（延时调用与字节顺序、ACK/NACK 位置） | **未执行**（真实时间差 ≥8.3 ms） | 部分 |
| 温度/湿度字 CRC-8 | 通过（独立 oracle + 注入坏 CRC） | **未执行** | 部分 |
| 温度 x10 有符号 + 量程 | 通过（端点 -400/1250、-450/1300 无效） | **未执行**（与参考温湿度计对照、负温实读） | 部分 |
| 湿度 x10 截断 0..1000 | 通过（0xFFFF→1000、0→0） | **未执行** | 部分 |
| 读 NACK / CRC / 无器件有界重试 | 通过（上限、次数、时序延时） | **未执行**（真实 NACK、故障注入恢复） | 部分 |
| 失败返回失败码且不改输出 | 通过（IO/CRC/RANGE/NO_DEVICE/PARAM 全分支） | **未执行** | 部分 |

## 6. 未执行的真实目标工作与阻塞原因

以下 TD-002 rev 2.0 用例对 T1 为强制项，本轮**均未执行**：

- T-L2-01 真实 I²C 事务抓包（地址字节、0xFD、6 字节、ACK 位置、空闲上拉）
- T-L2-02 tMEAS 真实时间差（≥8.3 ms，取 10 ms）
- T-L2-03 连续读时序与主机 ACK/NACK 位置的真实波形
- T-L2-04 与参考温湿度计的实测值一致性（≥3 温度点 / 2 湿度点）
- T-L2-05 负温端到端符号
- T-L2-06/07/08 真实故障注入（CRC 错、总线拉低卡死、无器件）
- T-L2-09 0x44/0x45 真实器件变体确认
- T-L1-09/T-L1-12 当前交付件烧录与回读一致性

**阻塞原因（环境不可用）**：项目执行环境声明「设备工具已禁用」；本能力实际可用工具仅 read/bash/write 类，无 programmer/jlink/serial/logic_analyzer/oscilloscope/power_cycle，环境内也没有 SEGGER J-Link 或逻辑分析仪 CLI，且无传感器板/网关板/可控温源/参考温湿度计/433 接收端。故以上必需的真实目标工作无法进行。

## 7. 结论

- **未发现实现缺陷**：宿主机独立验证（真实驱动 + 位级 mock 总线）、独立 oracle 复算与变异负对照全部通过，覆盖任务文字的每一个软件可判分支与边界；`gcc/build.sh` 对当前交付件交叉编译 0 错误、资源在预算内、T1 翻译单元无浮点。
- **但本项不能判 TEST_PASS**：TD-002 §2.2 对 T1 强制要求的真实目标观测（实板 I²C、真实器件 tMEAS/NACK、参考仪器对照、烧录回读一致性）在设备工具禁用下未执行；按能力约定这属于「required real-target work not performed」，必须返回阻塞类结果，且不得用 mock 成功或「后续再补」的说明替代。
- **也不判 TEST_FAIL**：阻塞不是实现缺陷；把未执行记成失败会误导后续修复方向。
- 因此本轮结果：**BLOCKED**（环境不可用，待设备工具解禁后按 TD-002 §4.4/§4.3 补做真实目标用例，并重跑本节宿主机证据）。

## 8. 交接

1. **设备解禁后**：先做 T-L1-09/T-L1-12（烧录当前提交编出的镜像并回读比对），再做 T-L2-01/02/03（I²C 抓包与 tMEAS），随后 T-L2-04/05（参考仪器）与 T-L2-06/07/08/09（故障注入、地址变体）。宿主机证据可用本轮命令原样复跑。
2. **观测钩子**：`SENSOR_TEST_TRACE` 默认关闭；实板做 T-L2-04 前建议按 TD-002 §6.2 打开以获得内部温湿度/重试计数（功耗用例必须关闭态）。
3. **不属本能力**：本项未发现需要固件实现修改的问题，故不产生实现侧改单。

---

# 历史记录（上一轮工作流 EV-002，旧 12 项编号；仅供追溯，勿与本轮 ITEM-001..007 混用）

# 嵌入式验证证据（EV-002 / ITEM-001）

状态：embedded_tester.embedded_verification 独立验证证据
本轮验证对象：任务项 **ITEM-001**（新增 `sensor_config.h` 作为唯一配置点；加入后工程仍可交叉编译）
被测提交：`ab5c5b2`（firmware_engineer.firmware_implementation）
验证基线：`9e90eb5`（本次实现之前，已含 TD-002 测试设计）
结论：**TEST_PASS**（ITEM-001 的声明行为全部独立复现；另发现 1 项与本项无关的既有 Keil 工具链环境问题，见 §6）

上游输入：`artifacts/firmware_implementation.md`（FWI-002 声明）、`artifacts/test_design.md`（TD-002）、`artifacts/firmware_design.md`（FD-002）、IC-002、任务清单 ITEM-001。

---

## 1. 验证范围与选测说明（为什么选这些）

| 选测项 | 理由 |
|---|---|
| 交付物存在性与常量值（TD-002 T-L1-05/T-L1-08 的配置点部分） | ITEM-001 的唯一验收物就是该头文件与其常量 |
| 头文件自包含 / 编译期断言 | 证明常量可被后续 TU 直接使用且取值正确，不靠人工目测 |
| GNU 交叉编译 + 与基线逐字节比对（TD-002 T-L1-02） | 直接检验「加入该头文件后工程仍可交叉编译」 |
| 生产编译器 ARM Compiler 5（Keil AC5）+ 工程 include 路径 | 交叉编译的真实量产工具链是 Keil MDK / IAR，需证明头文件对 AC5 与两个工程都可用 |
| 宿主机既有 L0 回归（TD-002 T-L0-06 的回归部分） | 本次改动不涉及被判定的既有纯逻辑，需确认无回归 |
| 变更范围审查（git） | 确认未夹带无关改动，且 .c 未被改动（支持"无行为变化"结论） |

**不适用（本轮未实现，按能力约定不要求、也不作为本项通过条件）**：TD-002 中 T-L0-01…T-L0-05（CRC-8/换算/光照/上报判定纯逻辑）、T-L0i（空口互操作）、T-L2/T-L3/T-L4/T-L5/T-L6/T-L7/T-L8（I²C、光照、节拍、条件上报、失败路径、端到端、功耗）——这些行为集中在后续 ITEM-002…ITEM-012，本轮不存在可测对象。仅对既有保留逻辑做回归。

## 2. 环境与工具（本轮实际调用）

| 工具 | 版本/路径 | 用途 |
|---|---|---|
| `arm-none-eabi-gcc` | 10.3.1 20210824（Cortex-M0+） | 工程交叉编译、编译期断言 TU |
| ARM Compiler 5 | `C:/Keil_v5/ARM/ARM_Compiler_5.06u7/Bin/armcc.exe` | 量产编译器单 TU 兼容性验证 |
| MinGW-w64 gcc | 12.2.0（`C:/ProgramData/chocolatey/.../mingw64/bin/gcc.exe`） | 宿主机 L0 回归 |
| `mdk_build`（Runtime 工具） | 配置的 Keil 工程 | 尝试真实 Keil 工程构建（结果见 §6） |
| 逻辑分析仪/示波器/J-Link/串口/烧录 | — | **本轮未使用**：ITEM-001 只新增宏定义，无可观测的板上行为；无需也不可能用仪器验证 |

## 3. 检查明细与结果

### 3.1 交付物存在与内容（通过）

`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/sensor_config.h` 存在，135 行，含 6 组配置。独立核对：

| 要求组（ITEM-001 原文） | 实际定义 | 判定 |
|---|---|---|
| PB04/PB05 引脚 | `LIGHT_POWER_PORT=CW_GPIOB`、`LIGHT_POWER_PIN=GPIO_PIN_5`（PB05）；`LIGHT_ADC_PORT=CW_GPIOB`、`LIGHT_ADC_PIN=GPIO_PIN_4`（PB04） | PASS |
| ADC 通道 | `LIGHT_ADC_INPUT_CHANNEL=ADC_InputCH11`（=11）；`LIGHT_ADC_FULL_SCALE=4095`；`LIGHT_ADC_CLK_DIV=ADC_Clk_Div8`；`LIGHT_ADC_SAMPLE_TIME=ADC_SampTime390Clk`；`LIGHT_ADC_SAMPLES=8` | PASS |
| GXHT40 地址/命令/重试/等待 | 0x44/0x45；写读字节 0x88/0x89/0x8A/0x8B；`0xFD`（高重复率，另存备用 0xF6/0xE0 与软复位 0x94 并注明周期路径禁用）；`GXHT40_RESULT_LEN=6`；`GXHT40_MEASURE_WAIT_MS=10`；`GXHT40_READ_RETRY=5`+`1 ms`；`GXHT40_MEAS_RETRY=3`；`GXHT40_POWER_ON_WAIT_MS=2`；CRC8 poly 0x31/init 0xFF；换算常量 `-450/1750/-60/1250/65536/32768` 与量程 `-400..1250`、`0..1000` | PASS |
| 光照明暗阈值与滞回 | `LIGHT_DARK_ENTER=350`、`LIGHT_DARK_EXIT=250`、`LIGHT_DARK_HYSTERESIS_STEP=100`；`LIGHT_DARK_CALIBRATED=0`（明确标注"未标定，须实板标定后回填"） | PASS（并未把未标定默认值谎称已验收） |
| 3 分钟采样计数值 | `SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3` | PASS |
| 0.9 ℃ 下降 / 35.0 ℃ 超温上报常量 | `SENSOR_REPORT_DROP_X10=9`、`SENSOR_REPORT_HIGH_X10=350`，注释写明严格大于语义 | PASS |

附带核对：`SENSOR_RF_FRAME_LEN=10`、`SENSOR_RF_TX_TIMEOUT_MS=200`、`SENSOR_RF_TX_RETRY=3`、调试钩子 `SENSOR_TEST_TRACE` 默认 0。

**独立交叉核对（来源可信度）**：
- PB04 = AIN11 = `ADC_InputCH11`：由厂商库头 `CW32L010_StandardPeripheralLib_V1.0.5/Libraries/inc/cw32l010_adc.h` 的注释与宏独立确认（`ADC_InputCH11 = 0x0B`，注释「通道11输入PB04」），非仅引用设计文档。
- PB04 模拟功能宏 `PB04_ANALOG_ENABLE()` 存在于 `cw32l010_gpio.h`。

### 3.2 头文件自包含 + 编译期断言（通过）

自写独立验证 TU（`sensor_config.h` 为唯一项目头包含，另仅 `_Static_assert`），43 条断言覆盖上表全部常量与关系（`EXIT<ENTER`、`ENTER-EXIT==STEP`、`MEASURE_WAIT_MS>=9`、地址派生、通道=11 等）：

```
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -Wall -Wextra -std=c11 \
  -I USER/inc -I COMMON -I UM2005C -I ../../../Libraries/inc \
  -I <CMSIS 5.9.0 Core Include> -fsyntax-only <verify_tu.c>
=> ASSERT_TU_OK: all 43 static assertions compiled cleanly（0 警告，0 错误）
```

结论：头文件自包含（仅 `<stdint.h>`/`<stdbool.h>` + CW32 库头），所有断言值正确。

### 3.3 唯一配置点（通过，局部）

- 在目标工程范围内（`USER/`、`UM2005C/`、`COMMON/`、`test/`、`gcc/`、`MDK/`、`EWARM/`）检索 `GXHT40_*`/`LIGHT_*`/`SENSOR_RTC_*`/`SENSOR_SAMPLE_*`/`SENSOR_REPORT_*`/`SENSOR_RF_*`/`SENSOR_TEST_*` 的 `#define`：**除 `sensor_config.h` 外为 0 处**，无重复定义/冲突。
- 实际被构建使用（非死文件）：预处理确认 `USER/src/main.c` 与 `USER/src/interrupts_cw32l010.c` 经 `main.h` 包含该头文件；`measure.c` 目前未包含（本次任务未要求，后续 ITEM 接入）。
- 说明：`SENSOR_*` 同名前缀在仓库其它无关示例（`Examples/sensoraiwrite/...`）与 CH592 官方库中存在，但不在本工程构建范围内，不构成冲突。

### 3.4 GNU 交叉编译 + 与基线逐字节比对（通过，核心）

命令：`sh gcc/build.sh`（工程自带脚本，Cortex-M0+，`-O1 -Wall -Wextra`，全工程 TU）。

| 项 | HEAD（ab5c5b2） | 基线（9e90eb5，独立 git worktree 构建） |
|---|---|---|
| 编译/链接 | 0 error | 0 error |
| warning 数 | 29（全部来自既有代码/厂商库） | （同源代码，未变） |
| sensor_config.h 相关告警 | **0** | 不适用 |
| FLASH | 33,896 B（51.72%） | 33,896 B（51.72%） |
| RAM used | 1,960 B（47.85%） | 1,960 B（47.85%） |
| `arm-none-eabi-size` text/data/bss | 33896 / 84 / 1876 | 33896 / 84 / 1876 |
| `sensor_fw.bin` MD5 | `46727d6446edb4adb0c87d26c7933f01` | `46727d6446edb4adb0c87d26c7933f01` |
| `sensor_fw.hex` MD5 | `0da7d963b50d16c2e1bb4fc5d80ce9da` | `0da7d963b50d16c2e1bb4fc5d80ce9da` |

基线为在临时 worktree（`git worktree add --detach <tmp> 9e90eb5`）中独立构建，未复用实现者产物。**固件二进制逐字节相同**，独立证明「加入该头文件后工程可交叉编译且不产生任何代码/数据变化」。

### 3.5 生产编译器与工程 include 路径（通过）

- Keil MDK 工程 include 路径含 `..\USER\inc`（`MDK/Project.uvprojx: <IncludePath>...;..\USER\inc;..\UM2005C;..\COMMON`）。
- IAR 工程 include 路径含 `$PROJ_DIR$\..\USER\inc`（`EWARM/*.ewp`）。
- 用 ARM Compiler 5.06u7 + 工程固定的 CMSIS 5.9.0 单独编译实际解析该头文件的 TU：

```
armcc --cpu=Cortex-M0+ --c99 -I ../../../Libraries/inc -I USER/inc -I UM2005C -I COMMON \
      -I ../../IdeSupport/MDK -I <CMSIS 5.9.0 Core Include> -c <file>
USER/src/main.c                 : 0 error（经 main.h 包含 sensor_config.h）
USER/src/interrupts_cw32l010.c  : 0 error（经 main.h 包含 sensor_config.h）
USER/src/sf_i2c.c               : 0 error
USER/src/measure.c              : 0 error, 1 warning（既有代码）
```

结论：新头文件对量产 ARM Compiler 5 兼容，未破坏既有 TU 编译。

### 3.6 宿主机 L0 既有回归（通过）

```
CC=<mingw64 gcc 12.2.0> sh test/build_test.sh
==== result: 56 passed, 0 failed ====
```

本项未改动 `fw_core.c`/`history.c`/`host_sensor_core_test.c`，56/56 与基线一致。

### 3.7 变更范围审查（通过）

`git show --stat ab5c5b2`：仅 5 个文件 —— `USER/inc/sensor_config.h`（新增）、`USER/inc/main.h`（+1 行 include，位于既有 include 段）、`artifacts/firmware_implementation.md`、`evidence/build.md`、`file_manifest.txt`。**无任何 .c/.ld/工程文件被修改**，无夹带无关改动，与"仅新增配置头"的声明一致。

## 4. 反证与反向检查

| 检查 | 结果 |
|---|---|
| 是否存在与声明不符的"假验证"（例如头文件未被任何 TU 使用） | 否：`main.c`/`interrupts_cw32l010.c` 预处理确认包含 |
| 是否把未标定阈值当作已验收 | 否：`LIGHT_DARK_CALIBRATED=0` 且注释明确要求实板标定后回填（FWR-OPEN-1） |
| 是否引入新的编译告警 | 否：0 条告警指向该头文件；交叉编译告警均来自既有代码/厂商库 |
| 是否存在同值常量重复定义（破坏"唯一配置点"） | 本工程范围内为 0 |
| 二进制是否真的无变化 | 是：与基线 `.bin`/`.hex` MD5 完全相同 |

## 5. 覆盖边界声明（本轮未验证的内容）

- 未在真实传感器板上做任何 I²C/光照/节拍/上报/功耗验证（无非硬件可测对象，且本项不涉及行为）。
- 未烧录、未使用串口/逻辑分析仪/示波器；`mdk_flash` 未调用（无需下载，且本项无板上行为可观测）。
- Keil 完整工程构建未成功（见 §6），因此"MDK 工程端到端编译通过"**未**被证明；已用 GNU 交叉编译 + AC5 单 TU 编译替代覆盖，并如实标注。
- 光照阈值、GXHT40 驱动、条件上报判定等属后续 ITEM，本证据不作为其通过依据。

## 6. 发现的非本项问题（环境/工具链，登记为交接，不计入本项缺陷）

`mdk_build`（action=build）在**当前环境**下失败，日志摘录：

```
*** Using Compiler 'V5.06 update 7 (build 960)'
compiling system_cw32l010.c...
C:\Users\kason\AppData\Local\Arm\Packs\ARM\CMSIS\6.3.0\CMSIS\Core\Include\cmsis_compiler.h(279): error: #35: #error directive: Unknown compiler.
".\output\exe\Project.axf" - 13 Error(s) ...
```

定位（独立复现，证明与 ITEM-001 无关）：

| 复现 | 结果 |
|---|---|
| 未改动的厂商文件 `Libraries/src/system_cw32l010.c` + CMSIS **6.3.0** 头 | 同样 `Unknown compiler` 错误 |
| 同一文件 + 工程记录的固定包 CMSIS **5.9.0** 头 | 0 错误，产出 .o |
| 本工程 `git show HEAD:MDK/output/exe/Project.build_log.htm`（导入基线，早于本次改动） | 上次成功构建使用 `ARM.CMSIS.5.9.0`，`0 Error(s)`；本次 UV4 解析到了 6.3.0 |

即：Keil RTE/pack 解析在本机选到 CMSIS 6.3.0，而 AC5 不被 CMSIS ≥6 支持，导致**所有** MDK 构建失败，与 `sensor_config.h` 无关。
处置建议（交接，非本能力执行）：在 Keil 工程中固定 CMSIS 5.9.0 包，或改用 ARM Compiler 6（AC6）并同步工程选项；修复前 `mdk_flash`/`mdk_build` 不能作为量产构建证据。

**仓库卫生**：`mdk_build` 生成的构建产物（`MDK/output/exe/*` 被跟踪文件）与 `MDK/build.log`、`test/host_sensor_core_test.exe` 已在验证后清理/还原，验证结束时 `git status` 为空。

## 7. 结论

ITEM-001 的声明行为经独立复现全部成立：配置头存在且取值正确、自包含、对 AC5 与 GNU 交叉编译均可用、为工程实际包含、无重复配置点，且固件二进制与改动前**逐字节一致**。既有 L0 回归无退化。

**判定：TEST_PASS**。残留项为 §6 的既有 Keil/CMSIS 工具链环境问题（已登记交接，不影响本项判定）与后续 ITEM 的行为验证（不在本项范围）。

---

# ITEM-002 验证（`sf_i2c` 无寄存器地址总线原语）

验证对象：任务项 **ITEM-002**（命令写 + 连续读原语，ACK 失败可判断；既有 i2c 函数语义与调用方式不变）
被测提交：`306c63d`；源码基线：`5a6210a`
结论：**TEST_PASS**

## A. 选测说明

| 选测项 | 理由 |
|---|---|
| 源码 diff 纯增量审查 | 直接检验「既有 i2c 函数语义与调用方式不变」 |
| 独立 mock 总线协议测试（自研，不复用实现者测试） | 检验「命令写 / 连续读」的线上字节、START/STOP 边界、主机 ACK/NACK 位置与返回值 |
| 既有函数机器码逐指令比对（改动前后两个 elf） | 比 diff 更强的回归证据：证明既有函数编译结果未被改变 |
| GNU 交叉编译 + 符号/体积差分 | 可编译、增量只来自新增函数、无既有代码膨胀 |
| ARM Compiler 5（AC5）单 TU 编译 | 量产工具链兼容性（sf_i2c.c 本次被修改） |
| 宿主机既有 L0 回归 | 无附带回归 |

不适用（后续 ITEM 才存在可测对象）：GXHT40 驱动、光照、节拍、条件上报、端到端与功耗用例。真实电气/时序（上升沿、上拉、时钟频率）、GXHT40 tMEAS 等待与器件地址变体属 TD-002 T-L2-01/02/03/07，须在真实传感器板上用逻辑分析仪完成，本轮**不具备**该条件，不在此判定。

## B. 源码变更范围（通过）

`git show --numstat 306c63d`：`USER/src/sf_i2c.c` **62 insertions / 0 deletions**；`USER/inc/sf_i2c.h` **8 / 0**。即对既有函数无任何行修改，仅在文件尾部新增 `i2c_write_cmd` / `i2c_read_bytes` 及声明。

## C. 独立 mock 总线协议测试（通过，核心）

自研位级 mock 总线 + mock 从机，直接链接**未打桩的** `USER/src/sf_i2c.c`（与实现者的 `test/host_sf_i2c_bus_check.c` 相互独立）。harness 入库于 `.../gpio_input_output/test/host_sf_i2c_verify_ev.c`。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
<mingw64 gcc> -std=c11 -Wall -Wextra -I USER/inc test/host_sf_i2c_verify_ev.c \
              USER/src/sf_i2c.c -o <tmp>/i2c_verify.exe && <tmp>/i2c_verify.exe
=> ==== result: 35 passed, 0 failed ====   (编译 0 警告)
```

原始线上转录（本 harness 输出）：

```
T1  i2c_write_cmd(0x88,0xFD) 有从机 : START TX(88) ACK TX(FD) ACK STOP
T2  i2c_write_cmd 无从机应答       : START TX(88) NACK STOP            -> SF_I2C_TIMEOUT
T6b 地址 ACK、命令字节 NACK         : START TX(88) ACK TX(FD) NACK STOP STOP -> SF_I2C_TIMEOUT
T3  i2c_read_bytes(0x89,6)         : START TX(89) ACK RX(61) M-ACK RX(9C) M-ACK RX(2B) M-ACK RX(8A) M-ACK RX(44) M-ACK RX(75) M-NACK STOP
T4  i2c_read_bytes 无从机应答       : START TX(89) NACK STOP            -> SF_I2C_TIMEOUT
T6  i2c_read_bytes 传写形式地址 0x88: START TX(89) ACK RX(11) M-NACK STOP
T7  回归 i2c_write_multi_byte       : START TX(70) ACK TX(AC) ACK TX(33) ACK TX(00) ACK STOP
T8  回归 i2c_read_multi_byte        : START TX(70) ACK TX(71) ACK START TX(71) ACK RX(A0) M-ACK ... RX(A4) M-NACK STOP
```

| 断言组 | 结果 |
|---|---|
| 命令写：1×START/1×STOP、线上 `0x88`→`0xFD`（无寄存器地址）、两字节均 ACK、从机仅收到命令字节、无读字节、结束后总线空闲高 | PASS |
| 命令写失败：地址 NACK → `SF_I2C_TIMEOUT`、已发 STOP、**命令字节不再发送**（快速失败）、总线释放 | PASS |
| 命令写失败：命令字节 NACK → `SF_I2C_TIMEOUT`、总线释放 | PASS |
| 连续读：1×START/1×STOP、首字节 `0x89`、6 字节与从机数据逐字节一致、**前 5 字节主机 ACK、末字节 NACK**、总线释放 | PASS |
| 连续读失败：读地址 NACK → `SF_I2C_TIMEOUT`、无数据字节、已发 STOP（不卡总线） | PASS |
| `length==0`：返回 SUCCESS 且**无任何总线动作** | PASS |
| 地址形式：`I2C_READ()` 掩码使写形式 0x88 与读形式 0x89 均得 0x89；与 `sensor_config.h` 的 `GXHT40_ADDR_READ_A=0x89` 兼容 | PASS |
| 回归：既有 `i2c_write_multi_byte` / `i2c_read_multi_byte`（含重复 START）字节序列与末字节 NACK 不变 | PASS |

实现者自带 harness 也被独立执行确认：`gcc ... test/host_sf_i2c_bus_check.c USER/src/sf_i2c.c` → **15 passed, 0 failed**（不以其自报为唯一依据）。

## D. 既有函数机器码等价（通过，最强回归证据）

在临时 worktree 中独立构建基线 `5a6210a`，与 HEAD 构建产物逐函数比对（`arm-none-eabi-objdump -d --disassemble=<fn>`，归一化重定位地址后逐指令比较）：

| 既有函数 | 结果 |
|---|---|
| `i2c_init` / `i2c_start` / `i2c_stop` / `i2c_write_byte` / `i2c_read_byte` | 指令完全相同 |
| `i2c_write_multi_byte` / `i2c_read_multi_byte` | 指令完全相同 |
| `i2c_write_multi_byte_16bit` / `i2c_read_multi_byte_16bit` / `i2c_obj_find` | 指令完全相同（仅重定位目标地址随链接位置变化） |

`arm-none-eabi-nm --print-size` 对照：上述函数**大小逐一相等**；新增符号只有 `i2c_write_cmd`（0x32=50 B）与 `i2c_read_bytes`（0x72=114 B）。

## E. 构建与体积（通过）

| 项 | 基线 5a6210a | HEAD 306c63d |
|---|---|---|
| 交叉编译 | 0 error / 29 warning | 0 error / 29 warning（无新增） |
| FLASH | 33,896 B | 34,060 B（**+164 B = 50+114，恰为两个新函数**） |
| RAM | 1,960 B | 1,960 B（不变） |
| `sf_i2c.c` 相关告警 | 2 条既有 `err may be used uninitialized`（`i2c_write_multi_byte`/`_16bit`，line 295/417，非本次引入） | 同左，行号与数量不变 |

AC5（`armcc --cpu=Cortex-M0+ --c99`，工程固定 CMSIS 5.9.0）单 TU 编译：`sf_i2c.c` 0 错误、`main.c` 0 错误、`measure.c` 0 错误（1 既有告警）。宿主机 L0 回归 `test/build_test.sh` → **56 passed, 0 failed**。

## F. 观察项（非缺陷，记录备查）

1. **NACK 失败路径会出现两个 STOP**（`i2c_wait_ack` 超时内已发一次，调用方再发一次；T6b 转录 `... NACK STOP STOP`）。I²C 总线上空闲态重复 STOP 无副作用，且与既有 `i2c_write_multi_byte` 的写法一致；本项不判缺陷，后续如需精简可随 GXHT40 驱动一并评估。
2. **`length==0` 短路返回 `SF_I2C_SUCCESS` 且不发总线**：任务未定义该边界，实现选择明确且可测；记录为行为约定。
3. 调用方需注意 `i2c_read_bytes` 的地址参数可用写/读形式任一种（内部掩码），与 `sensor_config.h` 的 `0x88/0x89` 常量兼容——后续 ITEM-003 接入时不应出现地址形式错用。

## G. ITEM-002 判定

「命令写（START→地址+W→命令→STOP）」「连续读（START→地址+R→N 字节→末字节 NACK→STOP）」「从机 ACK 失败作为可判断返回值」「既有 i2c 函数语义与调用方式不变」四项声明均经独立复现成立，且既有函数机器码逐指令未变。

**判定：TEST_PASS**（真实板电气/时序与 GXHT40 器件交互不在本项范围，交接 TD-002 T-L2-01/02/03/07）。

---

# ITEM-003 验证（GXHT40 驱动 `gxht40.c/.h`）

验证对象：任务项 **ITEM-003**（地址探测 0x44/0x45、`0xFD` 高重复率测量、tMEAS 等待、读 6 字节、温度/湿度字分别 CRC-8 校验、x10 换算与截断、失败返回失败码且不修改输出）
被测提交：`3bbd292`；源码基线：`1712162`
结论：**TEST_PASS**

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 新增文件 diff 审查 | `git show --numstat 3bbd292`：`gxht40.h` 48/0、`gxht40.c` 183/0、`test/host_gxht40_check.c` 308/0，**均为纯新增，无既有源码修改** |
| 独立参考帧生成（Python 独立实现 CRC/换算） | 不让被测驱动的 CRC/换算参与构造期望值，避免同源验证 |
| 自研 mock I²C + mock GXHT40 器件（链接真实 `gxht40.c`/`sf_i2c.c`） | 独立复现地址探测/命令/帧顺序/双字 CRC/重试/失败语义 |
| 失败不改输出（逐失败码） | 任务硬性要求，需直接断言 |
| GNU 交叉编译 + AC5 单 TU 编译 | 可编译性与量产编译器兼容（gxht40.c 尚未加入 Keil/IAR 源列表，属 ITEM-011） |

不适用（后续 ITEM 才有可测对象）：光照通路、采样节拍、条件上报判定、端到端与功耗。**真实 tMEAS 等待下界、地址变体实物确认、电气与真实器件 CRC 实读**属 TD-002 T-L2-01/02/03/06/07，本轮无逻辑分析仪/实物板，不在此判定。

## B. 独立参考帧与 CRC 变体确认（通过）

用独立 Python 实现（poly 0x31、init 0xFF、MSB-first、无反转、xorout 0x00）生成期望值：

```
CRC-8({0xBE,0xEF}) = 0x92   （与 gxht40.pdf / FD-002 §6.1 参考向量一致）
CRC-8("123456789") = 0xF7   （即 CRC-8/NRSC-5 标准校验值，确认变体无歧义）
换算：raw_t=0x0000 -> -450（超范围）; 0xFFFF -> 1300（超范围）; raw_h=0 -> 0; 0xFFFF -> 1190 -> 截断 1000
```

用于 mock 器件的 6 字节帧（**由上述独立脚本生成后硬编码，harness 不调用驱动的 CRC/换算**）：

| 向量 | 6 字节 | 期望结果 |
|---|---|---|
| 手册参考 0xBEEF/0x1234 | `BE EF 92 12 34 37` | OK，temp=855，hum=29 |
| 常温 25.0 ℃/50 %RH | `66 66 93 72 B0 DC` | OK，250 / 500 |
| 负温 −10.0 ℃/0 % | `33 33 88 00 00 81` | OK，−100 / 0 |
| 湿度越界（raw 0xFFFF） | `66 66 93 FF FF AC` | OK，hum 截断 1000 |
| 湿度 raw 0 | `66 66 93 00 00 81` | OK，hum 0 |
| 量程低（raw_t 0） | `00 00 81 72 B0 DC` | `GXHT40_ERR_RANGE`（换算 −450） |
| 量程高 125.1 ℃ | `F8 CA 32 72 B0 DC` | `GXHT40_ERR_RANGE`（换算 1251） |
| 温度字 CRC 错 | `66 66 00 72 B0 DC` | `GXHT40_ERR_CRC` |
| 湿度字 CRC 错 | `66 66 93 72 B0 00` | `GXHT40_ERR_CRC` |

字节序独立验证：若温度字字节序颠倒，0xEFBE 会得到 1189 而非 855；若湿度字颠倒，0x3412 会得到 194 而非 29——实际得到 855/29，证明 `T_MSB,T_LSB,T_CRC,RH_MSB,RH_LSB,RH_CRC` 顺序正确。

## C. 独立 mock 器件功能测试（通过，核心）

harness 入库于 `.../gpio_input_output/test/host_gxht40_verify_ev.c`（链接**未打桩的** `gxht40.c` + `sf_i2c.c`；`delay_ms` 为计数测试桩以断言 tMEAS 等待）。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
<mingw64 gcc> -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast \
  -I../USER/inc -I../COMMON -I../../../../Libraries/inc -I<CMSIS 5.9.0 Core Include> \
  host_gxht40_verify_ev.c ../USER/src/gxht40.c ../USER/src/sf_i2c.c -o gxht40_verify && ./gxht40_verify
=> ==== result: 42 passed, 0 failed ====   (0 编译告警)
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| 手册向量与帧封装 | OK；855/29；命令恰为 0xFD 一次；写地址 `0x88`、读地址 `0x89`；探测地址缓存 0x44；每次测量一次 10 ms tMEAS 等待 | PASS |
| 换算 | 25.0 ℃/50 %→250/500；−10.0 ℃/0 %→−100/0（符号保留）；hum raw 0xFFFF→1000；raw 0→0 | PASS |
| 地址探测与缓存 | 器件仅在 0x45：先 `0x88`（无应答）再 `0x8A`（成功）、读 `0x8B`；缓存 0x45；第二次测量直接用 `0x8A`，**不再探测 0x88** | PASS |
| 读 NACK 重试 | 前 2 次读地址 NACK、第 3 次成功；**命令未重发**（转换中）；三次读尝试；两次 1 ms 重试等待 | PASS |
| 读 NACK 用尽 | `GXHT40_ERR_IO`；输出不变；命令尝试 ≤ `GXHT40_MEAS_RETRY`；读尝试 ≤ `MEAS_RETRY×READ_RETRY` | PASS |
| CRC 错（温度字/湿度字） | 均 `GXHT40_ERR_CRC`；输出不变；**恰好重测 `GXHT40_MEAS_RETRY`(=3) 次** | PASS |
| 超范围（低/高） | 均 `GXHT40_ERR_RANGE`；输出不变 | PASS |
| 无器件 | `GXHT40_ERR_NO_DEVICE`；`0x88` 与 `0x8A` 均被探测；输出不变；探测次数有界 | PASS |
| 入参 | NULL 温度指针 / NULL 湿度指针 / 未绑定总线 均 `GXHT40_ERR_PARAM` | PASS |
| 命令白名单 | 全程共 23 次已发命令**全部为 `0xFD`**（无 `0x94` 软复位、无 `0x39` 加热器、无 `0xE0/0xF6` 低/中重复率） | PASS |

实现者自带 harness 也被独立执行确认：`host_gxht40_check.c` → **27 passed, 0 failed**（不以其自报为唯一依据）。

## D. 失败不修改输出（逐失败码，通过）

每类失败均先写入哨兵值（`temp=0x1111`、`hum=0x2222`，或 `0x7F7F`）再调用：`ERR_IO`、`ERR_CRC`（温度字/湿度字）、`ERR_RANGE`（低/高）、`ERR_NO_DEVICE`、`ERR_PARAM` 全部保持哨兵值不变。代码层核对：输出指针仅在完全成功分支末尾写入。

## E. 构建与对现网固件的影响（通过）

| 项 | 结果 |
|---|---|
| GNU 交叉编译（`gcc/build.sh`，含 `USER/src/gxht40.c`） | 0 error；29 warning（与基线同数，**无一条指向 `gxht40.c`**） |
| `gxht40.o` | text 460 B / data 0 / bss 5；符号 `gxht40_init`、`gxht40_measure`、`gxht40_detected_addr7`（`gxht40_crc8`、`gxht40_start_and_read` 为 static） |
| 固件镜像 | FLASH 34,060 B、RAM 1,960 B；`sensor_fw.bin` MD5 **与 ITEM-002 构建完全相同**（`67917abd…`）——模块尚未被调用，链接器 `--gc-sections` 丢弃，对现网固件零影响 |
| AC5（`armcc --cpu=Cortex-M0+ --c99`，CMSIS 5.9.0） | `gxht40.c` 0 error；`main.c` 0 error |
| 宿主机 L0 回归 | `test/build_test.sh` → 56 passed, 0 failed（未涉及，回归确认） |

## F. 观察项与交接

1. **CRC/换算实现会重复（交 ITEM-005）**：`gxht40.c` 内 `gxht40_crc8` / `gxht40_conv_*` 为 static；而 ITEM-005 要求在 `fw_core.c` 提供可宿主机直测的 `fw_crc8_gxht` / `gxht40_raw_to_x10` 等纯函数。若两份实现并存且不互调，存在后续漂移风险。建议 ITEM-005 完成后让 `gxht40.c` 复用 `fw_core.c` 的纯函数（或至少由 ITEM-005 的宿主测试锁定两者一致性）。**本项不因此判失败**（当前单份实现自洽且经独立验证）。
2. **Keil/IAR 源文件列表尚未包含 `gxht40.c`**（`Project.uvprojx`/`project.ewp` 中 `gxht40` 匹配数为 0）。这属 ITEM-011（“MDK/IAR 工程源文件列表与新增/移除源文件保持一致”）的范围，故在 ITEM-003 不判缺陷；但意味着**当前 Keil/IAR 构建不会编译该驱动**，ITEM-011 必须补上。
3. **无器件时的探测次数**：缓存失效后每次外层尝试会依次试 0x44/0x45，最坏 3×2=6 次命令（每次 10 ms 等待）≈ 60 ms 额外活动；有界且仅发生在器件缺失/故障场景，记录备查。
4. **真实 tMEAS 等待与地址变体**未在实物验证（无板/无逻辑分析仪）：软件侧已确认“命令→等待 10 ms→读”顺序与“读 NACK 不重发命令”，但实测下界（≥8.3 ms）与 0x44/0x45 实物确认仍需 TD-002 T-L2-01/02/03。

## G. ITEM-003 判定

地址探测（0x44/0x45，8 位 0x88/0x8A 写、0x89/0x8B 读）、`0xFD` 高重复率测量与 tMEAS 等待、6 字节读取与温度/湿度字分别 CRC-8（poly 0x31/init 0xFF）校验、x10 整数换算（含负温、0..1000 截断、−40.0..125.0 ℃ 有效域）、有界重试、失败返回失败码且不修改输出——全部经**独立于被测实现的参考帧与 mock 器件**复现成立。

**判定：TEST_PASS**。

---

# ITEM-004 验证（光照通路 `light.c/.h`）

验证对象：任务项 **ITEM-004**（采样时 PB05 输出高 → 稳定延时 → PB04/AIN11 多次取样求均值 → 阈值/滞回输出 DARK/LIT（无光 = 读数 ≥ 进入阈值）→ 采样结束 PB05 置低）
被测提交：`8e91e08`；源码基线：`d91f89c`
结论：**TEST_PASS**（带一项已登记的后续 ITEM 依赖，见 F.1）

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 新增/修改 diff 审查 | `light.h` 44/0、`light.c` 136/0、`test/host_light_check.c` 74/0 新增；`sensor_config.h` **+2/0**（仅新增 `LIGHT_ADC_EOC_GUARD`），无既有行为被改 |
| **阴影头 mock MCU 层**（自研，链接真实 `light.c`） | 实现者的测试只覆盖纯滞回函数（ADC/GPIO 全为不执行的空桩），**采样序列/均值/超时回退未被覆盖**；本轮用仿真 MCU 层把真实 `light.c` 跑起来 |
| 滞回真值表直接调用真实 `light_code_is_dark` | 需求硬性语义（无光 = code ≥ ENTER） |
| 交叉编译 + AC5 + 对现网镜像影响 | 可编译性与回归 |

不适用（后续 ITEM）：采样节拍接入（ITEM-006）、上报判定（ITEM-008）、旧通路退役（ITEM-009）、纯逻辑抽取（ITEM-005）、板级电气/时序（TD-002 T-L3-01/03/04/05，需暗箱/照度计/示波器，本轮无仪器）。

## B. 独立仿真 MCU 层验证（通过，核心）

自研阴影头（`test/mock_mcu/cw32l010_{gpio,adc,sysctrl}.h` + `mock_cw32.h`）置于 include 路径最前，使**真实的 `light.c`** 针对仿真 MCU 层编译；`delay_ms` 为计数桩，ADC 样本序列可脚本化（含 EOC 永不置位 = 转换超时）。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
<mingw64 gcc> -std=c11 -Wall -Wextra -I test/mock_mcu -I USER/inc -I COMMON \
  test/host_light_verify_ev.c USER/src/light.c -o light_verify && ./light_verify
=> ==== result: 45 passed, 0 failed ====   (0 编译告警；未引用任何厂商头)
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| 引脚所有权与 ADC 配置 | PB05 = 推挽输出且初始低；PB04 = 模拟输入；未配置其它引脚；ADC_Init 仅 1 次且通道 = `ADC_InputCH11`(AIN11)、`ADC_Clk_Div8`、`ADC_SampTime390Clk`、单次模式；init 后 ADC 关闭 | PASS ×10 |
| 采样序列（LIT 场景） | PB05 先高、后低（每次采样恰好 1 高 1 低、无其它引脚写）；稳定延时 = `LIGHT_SETTLE_MS`(100 ms) 在采样前；恰好 `LIGHT_ADC_SAMPLES`(8) 次转换；ADC 使能/关闭各 1 次且事后关闭 | PASS ×10 |
| DARK 场景 | code=400 → DARK | PASS |
| 均值 | 4×0 + 4×4095 → `light_last_code()`=2047（整数均值），判定用均值 | PASS ×3 |
| **滞回状态机（经真实 `light_sample`）** | 400→DARK；300（带内）保持 DARK；250(≤EXIT)→LIT；300(<ENTER) 保持 LIT；350(=ENTER)→DARK；349(>EXIT) 保持 DARK | PASS ×6 |
| 部分转换超时 | 2 次超时 + 6×1000 → 均值 1000（仅对成功样本求均值）；仍尝试 8 个时隙 | PASS ×3 |
| **全部转换超时（分压开路）** | 回退满量程 4095 → DARK（倾向上报、不阻塞）；PB05 仍置低；ADC 仍关闭；**轮询次数 800008，严格落在 8×GUARD…8×(GUARD+1) 内（有界，无死等）** | PASS ×5 |
| `light_reset_state()` | 复位后首样本用 ENTER 阈值（300→LIT），随后 350→DARK | PASS ×2 |
| 滞回真值表（直接调用） | 0/349→LIT；350/4095→DARK；251 保持/250→LIT；带内保持原状态 | PASS ×4 |

实现者自带 harness 也被独立执行确认：`host_light_check.c` → **17 passed, 0 failed**。

## C. 配置点与引脚所有权（通过，带依赖）

- `sensor_config.h` 仍为唯一配置点：本轮仅 +2 行（`LIGHT_ADC_EOC_GUARD`），光照阈值/引脚/采样数/稳定延时等常量未重复定义。
- `light.c` 内部：PB05 → `GPIO_MODE_OUTPUT_PP`（初始 `RESET`）；PB04 → `GPIO_MODE_ANALOG`（即 AIN11）；**非采样期 PB05 低、ADC 关闭**（低功耗约束）。
- **整个固件层面的引脚所有权尚未唯一（后续 ITEM 依赖，见 F.1）**。

## D. 构建与对现网镜像的影响（通过）

| 项 | 结果 |
|---|---|
| GNU 交叉编译 | 0 error；29 warning（与基线同数，无一条指向 `light.c`） |
| `light.o` | text 364 B / data 0 / bss 4；符号 `light_init`、`light_sample`、`light_last_code`、`light_code_is_dark`、`light_reset_state`；需 `__aeabi_uidiv`（32 位整数除法，无浮点）与 `memset` |
| 固件镜像 | FLASH 34,060 B、RAM 1,960 B；`sensor_fw.bin` MD5 与 ITEM-003 构建**完全相同**（模块尚未被调用，`--gc-sections` 丢弃） |
| AC5（`--c99`，CMSIS 5.9.0） | `light.c` 0 error；`main.c` 0 error |
| 宿主机 L0 回归 | `test/build_test.sh` → 56 passed, 0 failed |

## E. 观测与边界

- 仿真层验证的是**软件序列与判定逻辑**：PB05 高/低时序、延时值、转换次数、均值、滞回、超时回退与有界轮询。
- **未验证**（属板级）：PB05 高电平的实际建立时间与分压 RC/光敏响应（TD-002 T-L3-05）、实际暗/亮照度下的码值与阈值标定（T-L3-03）、ADC 真实 EOC 时序与 5 MΩ 源阻抗下的采样精度（T-L3-04）、PB04 电压/引脚真实电平（T-L3-01）。本轮无暗箱/照度计/示波器。

## F. 依赖与交接

1. **【重要・交 ITEM-009】整机引脚所有权冲突仍未消除**：`main.c` 目前仍调用 `optcfg_init(); hall_init();`（第 245/246 行），而 `optcfg.h` 定义 `OPT_IN_PIN = GPIO_PIN_5`（PB05 数字输入）、`F2_PWR_EN_PIN = GPIO_PIN_6`（PB06）、`hall.c` 将 `HALL_IN_PIN = GPIO_PIN_4`（PB04）配为带 EXTI 的数字输入。即：**在 ITEM-009 退役 hall/optcfg/params/history 之前，PB04/PB05/PB06 仍会被旧模块重新配置，光照通路在真实板上不会按设计工作**。这不属于 ITEM-004 的实现缺陷（ITEM-004 只交付 `light.c/.h`，且任务清单已把退役列入 ITEM-009），但意味着：在 ITEM-009 完成前，TD-002 的 T-L3-01/03/04/05 板级验证**不具备有效前提**，不得据此判光照功能通过。
2. **接线（交 ITEM-006）**：`light_init()` 与 `light_sample()` 尚未被主循环调用（因此镜像零变化）。ITEM-006 需在采样流程中先 `light_sample()` 后测温湿度，并注意 `light_sample()` 自带 100 ms 阻塞延时（RTA-002 时间预算已计入）。
3. **滞回纯函数重复（交 ITEM-005）**：`light_code_is_dark(code, prev_dark)` 已在 `light.c` 中为可宿主机直测的全局函数；ITEM-005 将其抽取到 `fw_core.c` 时，应保持签名/语义一致并让 `light.c` 复用，避免两份实现漂移。

## G. ITEM-004 判定

“PB05 输出高 → 稳定延时 → PB04/AIN11 多次取样求均值 → 阈值+滞回输出 DARK/LIT（无光=读数≥进入阈值）→ 采样结束 PB05 置低”以及“转换超时不阻塞、回退满量程（无光）”全部经**真实 `light.c` + 独立仿真 MCU 层**复现成立，既有配置点与现网镜像无回归。

**判定：TEST_PASS**（F.1 的整机引脚冲突为已登记的后续 ITEM 依赖，不影响本项模块级判定）。

---

# ITEM-005 验证（`fw_core.c` 纯逻辑：CRC-8 / 换算 / 滞回 / 上报判定）

验证对象：任务项 **ITEM-005**（不依赖 MCU 寄存器的纯逻辑：`fw_crc8_gxht` 满足 `CRC(0xBEEF)=0x92`；GXHT40 原始字→x10 整数换算含负温与 0–100%RH 截断；`light_code_is_dark` 滞回；`sensor_decide_report`）
被测提交：`7f8381e`；源码基线：`7342368`
结论：**TEST_PASS**（含 1 项需需求方确认的 35.0 ℃ 边界语义分歧，见 F.1；实现与已验证上游契约 IC-002 一致）

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 变更范围审查 | `fw_core.c` +95/0、`fw_core.h` +26/2、`sensor_config.h` +22/−14（拆出 `SENSOR_CONFIG_NO_MCU` 纯数值段）、`gxht40.c` +5/−42（删本地静态 CRC/换算）、`light.c` +1/−11（删本地滞回）、`light.h` −8（声明移走） |
| **独立参考穷举校验** | 这是本项目最易穷举的纯逻辑：对**全部 65536 个温度原始字、65536 个湿度原始字、65536 个两字节 CRC、4096×2 滞回输入、572 组上报组合**与独立参考逐位比对 |
| 重构后行为不变（回归） | 重跑 ITEM-003/ITEM-004 的独立 harness 与实现者 harness，确认删掉本地副本后驱动/光照行为不变 |
| 依赖检查 | 证明 `fw_core.c` 真不依赖 MCU 头/外设符号；GNU + AC5 可编译 |

不适用（后续 ITEM）：采样节拍接入（ITEM-006）、条件上报接入（ITEM-008）、旧通路退役（ITEM-009）、MDK/IAR 源列表（ITEM-011）、板级时序/电气（TD-002 T-L2/L3 需仪器）。

## B. 独立参考与穷举校验（通过，核心）

期望值由**独立 Python 参考**生成：换算用精确有理数 `-450 + floor(1750·raw/65536 + 1/2)`（round-half-up，与 C 实现不同的写法），CRC 用从零写的位算法，判定用直接按 IC-002 写的布尔式。harness 对结果计算 FNV-1a 32 位哈希并与参考常量比对。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
<mingw64 gcc> -std=c11 -Wall -Wextra -I USER/inc -I COMMON \
  test/host_fw_core_verify_ev.c USER/src/fw_core.c -lm -o fw_core_verify && ./fw_core_verify
=> ==== result: 53 passed, 0 failed ====   (0 编译告警)
```

| 穷举域 | 哈希（实测 = 参考） | 结论 |
|---|---|---|
| CRC-8：全部 256×256 个两字节输入 | `0x7CC4B9C5` | 65536 个 CRC 全部一致 |
| 温度换算：全部 65536 个原始字 | `0xBB0C5ABB` | 含舍入方向完全一致 |
| 湿度换算：全部 65536 个原始字 | `0x5F674BC5` | 含 0..1000 截断一致 |
| 滞回：全部 4096 个码值 × 2 种前态 | `0x19DB6FE6` | 8192 个判定一致 |
| 上报：13×11×(have,dark) = 572 组合 | `0xF7D66E3E` | 与 IC-002 布尔式一致 |

向量与边界（期望值均来自独立参考）：

- **CRC-8**：`{0xBE,0xEF}→0x92`（手册参考向量）；`{0x00}→0xAC`；`"123456789"→0xF7`（CRC-8/NRSC-5 标准校验值，独立确认变体）；GXHT40 帧字节 `{0x66,0x66}→0x93`、`{0x72,0xB0}→0xDC`；空长度→`0xFF`。
- **温度**：`0x0000→-450`；`0x073E→-400`（−40.0 ℃ 域下界）；`0x41C2→0`；`0x6666→250`；`0xF89D→1250`（125.0 ℃ 域上界）；`0xBEEF→855`；`0xF8CA→1251`（域上界外）；`0xFFFF→1300`；**恰好 0.5 的舍入平分点** `0x4000→-12`、`0xC000→863`（round-half-up）。
- **湿度**：`0x0000→0`（原始 −60 截断）；`0x0C64→1`；`0x1234→29`；`0x72B0→500`；`0xD8FD→1000`；`0xFFFF→1000`（原始 1190 截断）。
- **组合换算 `gxht40_raw_to_x10`**：有效对→`true` 且写输出；温度越域（−450 / 1251）→`false` **且不写输出**；域端点 −400/1250 接受；`NULL` 输出指针不崩且仍返回有效性。
- **滞回**：`0/349→LIT`；`350/4095→DARK`；已暗时 `251` 保持、`250→LIT`；带内保持前态（不抖动）。
- **上报**：无前值 cur=200→false、cur=350→false、cur=351→true；下降恰好 9→false、10→true；下降满足但 LIT→false；prev==cur→false；升温→false；负温（−10.0→−12.0 ℃，降 2.0 ℃）→true；int16 极值不溢出（差值以 32 位算）。

## C. 重构一致性与回归（通过）

- **重复实现已消除**：`fw_crc8_gxht`、`light_code_is_dark` 在源码树中各只有 **1 处定义**（均在 `fw_core.c`）；`gxht40.c` 中已无本地 `gxht40_crc8/conv_*`。
- 重跑 **ITEM-003 独立 harness**（`host_gxht40_verify_ev.c`，链接重构后 `gxht40.c`+`fw_core.c`）：**42/42**；**ITEM-004 独立 harness**（`host_light_verify_ev.c`）：**45/45**。
- 实现者 harness：`host_fw_core_pure_check.c` **36/36**、`host_gxht40_check.c` **27/27**、`host_light_check.c` **17/17**；宿主机 L0 回归 `test/build_test.sh` **56/56**。
- 说明：`light_code_is_dark` 的声明从 `light.h` 移到了 `fw_core.h`；已有调用方（`light.c`）已包含 `fw_core.h`。本轮据此同步更新了我方 ITEM-004 harness 的包含（否则会出现隐式声明告警），属测试侧适配。

## D. 依赖与构建检查（通过）

| 检查 | 结果 |
|---|---|
| `fw_core.c` 不依赖 MCU 头 | 预处理输出中 `cw32l010` 匹配数 = **0**（`SENSOR_CONFIG_NO_MCU` 纯数值段生效） |
| `fw_core.o` 无外设符号 | `nm` 中无 `ADC_*`/`GPIO*`/`CW_*` 引用 |
| GNU 交叉编译 | 0 error；29 warning（与基线同数，无一条指向 `fw_core.c`/`gxht40.c`/`light.c`） |
| 镜像 | FLASH 34,060→**34,352 B**（+292 B，为 `fw_core.o` 新增纯逻辑）；RAM 1,960 B 不变；`.bin` 变化（`fw_core.o` 已被引用，符合预期） |
| AC5（`--c99`，CMSIS 5.9.0） | `fw_core.c` / `gxht40.c` / `light.c` / `main.c` 均 **0 error** |
| 新增符号 | `fw_crc8_gxht`、`gxht40_temp_raw_to_x10`、`gxht40_hum_raw_to_x10`、`gxht40_temp_x10_valid`、`gxht40_raw_to_x10`、`light_code_is_dark`、`sensor_decide_report`（`fw_core.o` text 1588 B） |

## E. 观测边界

- 本项为纯逻辑，宿主机穷举即完整覆盖其输入域；不涉及板上时序。
- **未验证**（后续 ITEM）：这些纯函数是否被采样/上报流程正确调用（ITEM-006/008）、真实温度下报行为（TD-002 T-L5 边界矩阵，需实板/温箱）。

## F. 发现与交接

1. **【需需求方确认・非实现缺陷】恰好 35.0 ℃ 的下降分支语义分歧**：
   - ITEM-005 任务文本与 FD-002 §6.4 的代码片段为 `((prev−cur)>9 且 DARK) 或 (cur>350)`，**无 `cur<350` 条件**；在「`cur` 恰好 =350 且下降 >9 且 DARK」时会判 true。FD-002 §6.4 还注释说 IC-002 的 `T<35` 项“属冗余”。
   - 已验证上游契约 **IC-002 §2** 写的是 `(T[n] < 35.0) AND (下降) AND DARK OR T[n] > 35.0`；**FWR-104** 的边界行也写“恰好 35.0 ℃ 不触发”。即在该点 IC-002/FWR-104 与任务文本/设计代码不一致（FD-002 的“冗余”判断在恰好 35.0 ℃ 处不成立）。
   - 实现选择了 **IC-002/FWR-104**（`cur == 350` 直接返回 false），并在代码中以注释标注依据。
   - 本轮先按任务文本写出参考式，穷举时命中该分歧点；按 IC-002 修正参考式后哈希完全一致（`0xF7D66E3E`）。**精确分歧集仅为**：`have_prev=true, dark=true, cur=350, prev∈{1000,32767}`（共 2 点/572 组合）。
   - 结论：实现与已验证契约一致，不判实现缺陷；但**任务文本/设计注释需同步澄清**（建议需求方确认“恰好 35.0 ℃ 时下降分支是否允许上报”，并修正 FD-002 §6.4 的“冗余”注释）。此为本能力边界外的文档/需求变更，登记为交接。
2. **接口位置变更（交下游调用方）**：`light_code_is_dark` 现仅在 `fw_core.h` 声明；后续 ITEM-006/008 接入时应包含 `fw_core.h`。
3. **MDK/IAR 源列表仍未包含 `gxht40.c`/`light.c`**（匹配数 0），属 ITEM-011；`fw_core.c` 本就在列表中。

## G. ITEM-005 判定

四个纯逻辑均已在 `fw_core.c` 实现且不依赖 MCU 寄存器/头文件，经**独立参考穷举**逐位一致；重构后驱动与光照行为无回归（独立 harness 42/42、45/45），GNU 与 AC5 均 0 error。

**判定：TEST_PASS**（F.1 为需需求方确认的边界语义分歧，实现侧与已验证契约 IC-002/FWR-104 一致）。

---

# ITEM-006 验证（`measure.c` 采样流程改造）

验证对象：任务项 **ITEM-006**（每个周期先光照后温湿度；GXHT40 读 NACK/CRC 错按配置上限有界重试；整周期失败不上报、不更新前一有效温度、不构造 0 值；成功后更新前一有效温度与最近有效湿度）
被测提交：`1a0ba51`；源码基线：`9de0946`
结论：**TEST_PASS**

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 变更范围审查 | `measure.c` **+154/−249**（重写采样部分）、`measure.h` −3（删 `samples_since_report`/`first_sample_reported` 声明）、`fw_core.c` +2（`SENSOR_CONFIG_NO_MCU` 改为可外部预定义，供宿主机 harness 使用）、新增 `host_measure_flow_check.c` + `mock_measure_mcu/` |
| **独立仿真 MCU 层流程验证** | ITEM-006 的新逻辑就是“周期流程 + 状态推进/不推进”；用自研阴影头编译**真实 `measure.c`**，把光/温湿度原语脚本化，用**参考模型逐步比对** |
| 旧序列/旧调度清理检查 | 确认 AHT21 序列与旧上报调度已移除且无残留引用 |
| 构建 + 回归 | GNU/AC5 可编译，既有 harness 无回归 |

不适用（后续 ITEM）：3 分钟节拍（ITEM-007，当前仍为 RTC 1 分钟一拍）、上报发送路径对齐（ITEM-008）、旧通路退役（ITEM-009）、板级时序/上报矩阵（TD-002 T-L2/L3/L5，需仪器）。

## B. 独立仿真 MCU 层流程验证（通过，核心）

自研阴影头（`test/mock_measure_ev/cw32l010_{gpio,adc,sysctrl,uart}.h` + `mock_measure_hw.h`）置于 include 路径最前，编译**真实 `measure.c`**；`light_sample`/`gxht40_measure` 脚本化（含 `ERR_IO`/`ERR_CRC`/`ERR_RANGE`），并链接真实 `fw_core.c`（`sensor_decide_report`）、`sf_i2c.c`、`encrytogate.c`。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
<mingw64 gcc> -std=c11 -Wall -Wextra -Wno-unused-function -I test/mock_measure_ev \
  -I USER/inc -I UM2005C -I COMMON test/host_measure_flow_verify_ev.c USER/src/measure.c \
  USER/src/fw_core.c USER/src/sf_i2c.c USER/src/encrytogate.c -lm -o measure_verify && ./measure_verify
=> ==== result: 79 passed, 0 failed ====
```

方法：harness 内维护一个**参考模型**（与 measure.c 无关的流程重述），逐周期比对 `tempvalue`、`huminityvalue`、`report_req` 与“本周期是否新置上报”。14 个脚本周期：`(dark, gxht, temp, hum)` =
`(0,OK,250,500) (0,OK,240,510) (1,OK,220,600) (1,ERR_IO,-,-) (1,OK,210,610) (1,ERR_CRC,-,-) [待上报中失败] (0,OK,351,700) (0,OK,350,710) (1,OK,340,720) (0,OK,330,730) (1,OK,300,740) (1,ERR_CRC,-,-) (0,OK,280,750)`。

| 组 | 独立断言 | 结果 |
|---|---|---|
| 逐周期状态（14 周期×4） | `tempvalue`/`huminityvalue` 等于最近一次成功值；`report_req` 与参考模型一致；“新置上报”与参考判定逐周期一致；`sample_flag` 被消费 | PASS ×56 |
| 惰性初始化 | `light_init`/`gxht40_init`/I2C 物理层各只初始化 1 次；`i2c_obj_find("i2c0")` 绑定成功 | PASS ×3 |
| 采样顺序 | 每周期恰好 1 次 light + 1 次 gxht，且**恒为 light 先于 gxht**（含失败周期） | PASS ×3 |
| 失败不构造 0 值 | 4 个失败周期后 `tempvalue`=280（最后一次成功值）、`huminityvalue`=750，从未变为 0 | PASS ×3 |
| **失败不推进前值** | 周期 3（ERR_IO，前值 240）失败后，周期 4 测 210（降 30>9，暗）→ **必须上报**；若失败污染前值（如置 0）则降为负、不会上报 | PASS ×2 |
| 失败不清除待上报 | 周期 6 前手动置 `report_req=1`，失败后仍为 1，且状态不变 | PASS ×2 |
| 当前周期光照不泄露 | 周期 11（暗+降温）上报；周期 12（暗但失败）不上报；周期 13（亮+降温）不上报 | PASS ×3 |
| 35.0 ℃ 边界（与 ITEM-005 一致） | 周期 7（351，亮）上报；周期 8（350，亮）不上报 | 已由逐周期判定覆盖 |
| 休眠门控 | 待上报/待采样时**不**进深睡；空闲时进深睡 1 次 | PASS ×3 |
| 节拍门控 | `sample_flag==0` 时无 light/gxht 调用、无状态变化 | PASS ×2 |
| 低功耗 | 采样路径不配置调试 UART（`UART_Init` 调用数 = 0） | PASS |

实现者自带 harness 也被独立执行确认：`host_measure_flow_check.c`（按其在文件头记录的 `-DSENSOR_CONFIG_NO_MCU -Imock_measure_mcu` 命令）→ **15 passed, 0 failed**。

## C. 需求逐条对照

| ITEM-006 声明 | 实现位置 | 独立证据 |
|---|---|---|
| 每周期先取光照再取温湿度 | `measure_sample()`：`light_sample()` → `gxht40_measure()` | B.S2（逐周期调用顺序） |
| 读 NACK / CRC 错按配置上限有界重试 | `gxht40_measure()` 内部（ITEM-003 已验证）；流程每周期只调一次 | B.S1（每周期 gxht 调用数 = 1） |
| 整周期失败不上报/不更新前值/不构造 0 值 | `measure_sample()` 失败即返回，不写 `tempvalue`/`huminityvalue`；`temperature_process()` 仅成功分支判定与推进 | B.S3/S4/S5 |
| 成功后更新前一有效温度与最近有效湿度 | `s_prev_temp_x10`/`s_have_prev`/`s_last_hum_x10` 仅成功分支更新；判定用**更新前**的前值 | B 逐周期比对（含降 1.0/3.0 ℃ 正确触发） |

补充确认：AHT21 序列（`0x70/0xAC/0x71/0x33`）已从 `measure.c` 完全移除（仅注释提到“不再使用 AHT21”）；`first_sample_reported`/`samples_since_report` 已无任何代码引用（仅 `main.c` 一处旧注释提到该名字，属文档残留）；首周期无前值时不上报（符合 FWR-108）。

## D. 回归与构建（通过）

| 项 | 结果 |
|---|---|
| 既有独立 harness | `host_fw_core_verify_ev` 53/53；`host_gxht40_verify_ev` 42/42；`host_light_verify_ev` 45/45 |
| 宿主机 L0 | `test/build_test.sh` 56/56 |
| GNU 交叉编译 | 0 error；28 warning（比上一版少 1 条，无一条指向 `measure.c`） |
| AC5（`--c99`，CMSIS 5.9.0） | `measure.c` **0 error / 1 warning**；`main.c`/`gxht40.c`/`light.c`/`fw_core.c`/`sf_i2c.c` 均 0 error |
| 接线 | `measure.o` 引用 `light_init/light_sample/gxht40_init/gxht40_measure/sensor_decide_report`；三者在 `sensor_fw.elf` 中均已被链接（模块首次真正进入固件） |
| 资源 | FLASH 34,352→**35,884 B**（+1,532，模块接入）；RAM 1,960→**1,896 B**（−64，旧测量缓冲/参数结构体删除） |
| 镜像 | `sensor_fw.bin` MD5 `fff8aa9c…`（较 ITEM-005 变化，因驱动/光照模块接入，符合预期） |

## E. 观察与交接

1. **`s_last_hum_x10` 赋值后从未被读（AC5 `#550-D`）**：当前 `send_data_to_gateway()` 直接用 `huminityvalue` 组帧，未用该变量。任务要求“成功后更新最近有效湿度”，所以赋值本身是合规的；但**ITEM-008 应对齐发送路径（使用该变量或删除）**，以免遗留死存储与告警。
2. **发送路径与睡眠门控仍依赖旧模块**：`send_data_to_gateway()` 仍走 `optcfg_window_active()`/旧有界重试，`go_to_sleep()` 仍查 `hall_event_pending()`——属 ITEM-008（发送对齐）/ITEM-009（旧通路退役），本项未越界。
3. **`main.c` 旧注释**仍提到已删除的 `samples_since_report`（纯注释）；建议 ITEM-007 一并清理。
4. **上电即采一次**：`sample_flag` 初值为 1，故上电后立即执行一个采样周期；无前值时不强制上报（已验）。若产品不希望上电立即测量，属需求变更（FWR-108 仅约束“不强制上报首样本”）。
5. **3 分钟节拍未在本项**（当前 `RTC_IRQHandlerCallBack` 仍为 1 分钟一拍），属 ITEM-007；本项不做判定。

## F. ITEM-006 判定

“先光照后温湿度”“失败不上报/不推进前值/不构造 0 值/不清除待上报”“成功推进前值与最近有效湿度”“每周期一次有界重试调用”均经**真实 `measure.c` + 独立仿真 MCU 层与参考模型**逐周期复现成立；旧 AHT21 序列已清除；GNU 与 AC5 均可编译，既有回归全部通过。

**判定：TEST_PASS**。

---

# ITEM-007 验证（3 分钟采样节拍）

验证对象：任务项 **ITEM-007**（RTC 1 分钟中断累计到配置的 3 次才置采样标志；一个采样周期只执行一次测量；移除小时上报与首样本强制上报逻辑）
被测提交：`fd65dcf`；源码基线：`2ea1431`
结论：**TEST_PASS**

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 变更范围审查 | 仅 `main.c` **+12/−11**：`RTC_IRQHandlerCallBack` 阀值 `>= 1` → `>= SENSOR_SAMPLE_TICKS`；删除未用全局量 `temp_cnt`/`work_period_flag`；注释更新 |
| **真实 `main.c` 的 RTC 回调行为验证** | 用自研仿真 MCU 层直接驱动 `RTC_IRQHandlerCallBack()` 与 `RTC_Configuration()` |
| **与基线的全函数反汇编差异** | 证明本次改动**仅**是节拍阀值 + 2 个已删全局量，无其它代码变化 |
| 旧上报逻辑残留检查 | 确认小时上报/首样本强制上报已无实现（仅注释提及） |
| 回归 | 既有独立 harness + 宿主机 L0 + 构建 |

不适用（后续 ITEM）：发送路径对齐（ITEM-008）、旧通路退役（ITEM-009）、MDK/IAR 源列表（ITEM-011）、板级长时基实测（TD-002 T-L4-01/02，需逻辑分析仪/电流波形）。

## B. 独立 RTC 节拍验证（通过，核心）

自研阴影头（`test/mock_main_ev/`，共 11 个头）置于 include 路径最前，编译**真实 `main.c`**（为避免与本 harness 的 `main` 冲突，编译时用 `-Dmain=firmware_main_entry`，并用 `--gc-sections` 丢弃未引用函数）；harness 直接调用 `RTC_Configuration()` 与 `RTC_IRQHandlerCallBack()`。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
<mingw64 gcc> -c -ffunction-sections -fdata-sections -Dmain=firmware_main_entry \
  -I test/mock_main_ev -I USER/inc -I UM2005C -I COMMON USER/src/main.c -o main_ev.o
<mingw64 gcc> -c -ffunction-sections -fdata-sections -I test/mock_main_ev -I USER/inc \
  -I UM2005C -I COMMON test/host_rtc_cadence_verify_ev.c -o rtc_ev.o
<mingw64 gcc> -Wl,--gc-sections main_ev.o rtc_ev.o -o rtc_verify && ./rtc_verify
=> ==== result: 17 passed, 0 failed ====
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| RTC 间隔 | `RTC_Configuration()` 传给 `RTC_SetInterval` 的值 == `RTC_INTERVAL_EVERY_1M`；并由厂商头 `cw32l010_rtc.h` 独立确认该宏 = `0x03`（1 分钟档，同表还有 0.5s/1s/1h/1d/1month）；RTC 初始化/中断使能/NVIC/LSI 各 1 次 | PASS ×4 |
| 无中断挂起 | `RTC_GetITState` 返回 RESET 时回调不动计数器也不置标志 | PASS |
| 第一周期 | 第 1/2 拍：无标志、计数器 1→2；第 3 拍：**置采样标志且计数器归零** | PASS ×3 |
| 后续周期 | 第 4/5 拍无标志，第 6 拍再次置标志 | PASS ×3 |
| 长时行为 | 连续 30 拍：**恰好 10 次采样机会，且落在第 3/6/9/…/30 拍**；计数器始终 < 3（有界） | PASS ×3 |
| 配置关系 | `SENSOR_RTC_TICK_PERIOD_MIN × SENSOR_SAMPLE_TICKS = 3`（即 3 分钟） | PASS ×2 |

“一个周期只测一次”由两端共同保证：ISR 每 3 拍才置一次 `sample_flag`（本 harness），而 `temperature_process()` 在完成一次测量后清除该标志（**已在 ITEM-006 独立 harness 79/79 中验证**）。

## C. 与基线的机器码差异（通过，最强回归证据）

在临时 worktree 中独立构建基线 `2ea1431`，对全二进制反汇编做归一化逐指令比对：

```
RTC_IRQHandlerCallBack:
  baseline:  cmp r3, #0   ; beq.n ...      (rtc_set_cnt >= 1)
  HEAD:      cmp r3, #2   ; bls.n ...      (rtc_set_cnt >= 3)
```

除该阀值外，全部差异仅为**字面量池地址偏移**（因删除 2 个 .bss 全局量导致地址平移 4 字节）与文件路径行；**没有任何其它指令变化**。基线 ELF 中 `temp_cnt`(0x200004cd)、`work_period_flag`(0x200004cc) 存在，HEAD ELF 中已不存在。

## D. 旧上报逻辑清理（通过）

- 小时上报（旧 `samples_since_report >= 60`）与首样本强制上报（旧 `first_sample_reported`）**已无任何实现或引用**；全树仅两处注释提到该词：`main.c:105`（说明“已无小时/首样本上报”）与 `main.c:260` 的**陈旧注释** `/* 立即/小时/首样本上报 (FR-203) */`。后者是文档级残留，建议 ITEM-008/010 清理（不影响行为）。
- ELF 中无 `first_sample*`/`samples_since*`/`temp_cnt`/`work_period_flag` 符号。

## E. 构建与回归（通过）

| 项 | 结果 |
|---|---|
| GNU 交叉编译 | 0 error；28 warning（与基线同数；`main.c` 的 3 条 warning 均为模板/既有：`__write` 未用参数、`main` 返回类型、`assert_failed` 未用参数） |
| AC5（`--c99`，CMSIS 5.9.0） | `main.c` **0 error / 0 warning** |
| 资源 | FLASH 35,884 B、RAM 1,896 B（与基线**完全相同**，仅指令常量与 2 字节全局量变化）；`.bin` MD5 `2f6efa92…`（基线 `fff8aa9c…`） |
| 既有独立 harness | fw_core 53/53、gxht40 42/42、light 45/45、measure 流程 79/79 |
| 宿主机 L0 | `test/build_test.sh` 56/56 |

## F. 观测与交接

1. **节拍精度未实测**：RTC 时钟源为 LSI，3 分钟为近似值；需求未给精度指标。实际间隔/离散度需板级长时基测量（TD-002 T-L4-01），本轮无逻辑分析仪/电流波形，**未验证**。
2. `main.c:260` 陈旧注释（“立即/小时/首样本上报”）建议后续清理。
3. 采样仍与光照/温湿度模块以及 `send_data_to_gateway`（旧路径，ITEM-008）在同一主循环内，每次 RTC 唤醒都会进一次循环体（无采样时立即回睡）；低功耗实测仍属板级（TD-002 T-L4-04/T-L8）。

## G. ITEM-007 判定

“RTC 1 分钟中断累计 3 次才置采样标志”“一个周期只测一次”“移除小时上报与首样本强制上报”均经**真实 `main.c` + 独立仿真 MCU 层**验证，并与基线逐指令比对确认无其它副作用；既有回归全部通过。

**判定：TEST_PASS**。

---

# ITEM-008 验证（条件上报的 433 发送路径）

验证对象：任务项 **ITEM-008**（满足 `sensor_decide_report` 时 `encode_frame10` 组 10 字节帧并经 `app_um2005C_send_data_timeout` 有界发送；帧布局/字节序/Feistel 不变；仅在发送成功后清除待上报；失败按上限重试后放弃本轮）
被测提交：`6a69601`；源码基线：`cdbc8cb`
结论：**TEST_PASS**

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 变更范围审查 | `measure.c` **+12/−19**：删除手工 `send_data[0..7]` 预组装（`encode_frame10` 会写满 10 字节，属死代码）；改传 `s_prev_temp_x10`/`s_last_hum_x10`（触发上报的样本）；`len` 改用 `SENSOR_RF_FRAME_LEN`；`main.c` 仅注释；**`encrytogate.c`/`feistel_al.c` 未修改** |
| **发送路径独立验证（真实 `measure.c`）** | 用脚本化发送结果驱动成功/失败/用尽/窗口分支，验证“仅成功后清除”与“有界重试后放弃” |
| **空口帧互操作（真实编码器 + 真实网关解码器）** | 验证帧布局/字节序/加密不变；并证明被删的手工组帧确为死代码 |
| 回归与构建 | 既有 harness + GNU/AC5 |

不适用（后续 ITEM）：旧通路退役（ITEM-009）、MDK/IAR 源列表（ITEM-011）、网关兼容性正式核对（ITEM-012）、**真实 433 射频与丢包/失败上限实板验证**（TD-002 T-L6-03/T-L7，需真实射频与仪器）。

## B. 发送路径独立验证（通过，核心）

真实 `measure.c` + 仿真 MCU 层（`test/mock_measure_ev/`），脚本化光/温湿度与 `app_um2005C_send_data_timeout` 返回值，并用**真实采样周期**产生待上报标志。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
<mingw64 gcc> -std=c11 -Wall -Wextra -Wno-unused-function -Wno-misleading-indentation \
  -Wno-unused-const-variable -I mock_measure_ev -I ../USER/inc -I ../UM2005C -I ../COMMON \
  host_rf_report_verify_ev.c ../USER/src/measure.c ../USER/src/fw_core.c \
  ../USER/src/sf_i2c.c ../USER/src/encrytogate.c -lm -o rf_report_verify && ./rf_report_verify
=> ==== result: 28 passed, 0 failed ====
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| 无待上报 | `report_req==0` 时不调用发送器 | PASS |
| 成功路径 | 真实周期（暗+降 2.0 ℃）置起待上报 → 恰好 1 次发送；`len==SENSOR_RF_FRAME_LEN==10`；超时 == `SENSOR_RF_TX_TIMEOUT_MS`；**成功后清除待上报** | PASS ×5 |
| **发送帧内容** | 交给发送器的 10 字节 == `encode_frame10(uid, 触发样本温度, 触发样本湿度)`；且发送时刻 `tempvalue/huminityvalue` 就是该样本，**与旧代码路径编码逐字节相同**（无帧变化） | PASS ×4 |
| 失败不假成功 | 连续失败时 `report_req` 保持 1（不提前清除）；第 3 次成功才清除 | PASS ×4 |
| **有界重试后放弃** | 持续失败 → 恰好 `SENSOR_RF_TX_RETRY`(3) 次尝试后清除待上报并放弃；此后不再发送 | PASS ×3 |
| 新轮次 | 放弃后新的待上报重新发送，重试计数重置（第 2 次成功） | PASS ×2 |
| 配置窗口 | `optcfg_window_active()` 为真 → 不发送且保留待上报；窗口关闭后补发 | PASS ×2 |
| 失败周期 | 测量失败不置待上报、不发送 | PASS ×2 |

## C. 空口帧与网关互操作（通过）

自研 harness 直接链接**真实传感器编码器**（`USER/src/encrytogate.c`）与**真实网关解码器**（`CH592EVT/.../beiwov2/APP/feistel_al.c`）——两份独立实现互证。

```
<mingw64 gcc> -std=c11 -Wall -Wextra -Wno-unused-function -Wno-unused-const-variable \
  -Wno-misleading-indentation -I ../USER/inc -I <gateway APP> host_rf_frame_verify_ev.c \
  ../USER/src/encrytogate.c <gateway APP>/feistel_al.c -o rf_frame_verify && ./rf_frame_verify
=> ==== result: 10 passed, 0 failed ====
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| 往返 | 12 组向量（含 -100/−400/−450/1250/1300/32767/−32768、湿度 0/1000/65535）逐位往返一致（uid[0..3]、temp、hum） | PASS |
| 字段/字节序 | 温度在 `p[4..5]` 小端、湿度在 `p[6..7]` 小端、id = `uid[0..3]`；负温符号保留 | PASS ×3 |
| **全 10 字节写入** | 同一输入写入预填 0xAA 与清零缓冲区结果相同 → **证明 ITEM-008 删除的手工组帧确为死代码** | PASS ×3 |
| 确定性 | 同输入两次编码逐字节相同 | PASS |
| 完整性 | 10 个密文字节逐一翻转均被网关 CRC 拒绝 | PASS |
| 网关→Android 映射 | 解码值按 `id | hum_be | temp_be` 组记录后用 `u16be@4`/`s16be@6` 读回，温度/湿度/id 归属正确（与 `app_um2006A.c` 与 `MqtttService.kt` 源码核对一致） | PASS |

实现者自带 harness 也被独立执行确认：`host_rf_frame_check.c` **14/14**、`host_measure_flow_check.c` **24/24**。

## D. 构建与回归（通过）

| 项 | 结果 |
|---|---|
| GNU 交叉编译 | 0 error；28 warning（无一条指向 `measure.c`） |
| AC5（`--c99`，CMSIS 5.9.0） | `measure.c` **0 诊断**——**ITEM-006 遗留的 `s_last_hum_x10` set-but-unused 告警已消除**（该变量现被 `encode_frame10` 使用） |
| 资源 | FLASH 35,884→**35,856 B**（−28，删除死代码）；RAM 1,896 B 不变；`.bin` MD5 `9b72933e…` |
| 符号 | `measure.o` 引用 `encode_frame10` 与 `app_um2005C_send_data_timeout`；`s_last_hum_x10` 已定义并使用 |
| 既有独立 harness | fw_core 53/53、gxht40 42/42、light 45/45、measure 流程 79/79 |
| 宿主机 L0 | `test/build_test.sh` 56/56 |

## E. 观察与交接

1. **真实射频未验证**：调制/前导/空中速率/丢包以及“发送失败上限”的实板行为需真实 433 链路与接收机（TD-002 T-L6-03/T-L7）；本轮仅验证软件侧调用、参数与状态机。
2. **待上报期间主循环空转**：`go_to_sleep()` 在 `report_req!=0` 时不进深睡，故发送失败重试期间 CPU 保持活跃（次数有界：≤3 次 × ≤200 ms 超时）；能量影响属功耗实测（TD-002 T-L8）。
3. `send_data_to_gateway()` 仍依赖 `optcfg_window_active()`（旧配置窗口），属 ITEM-009 退役范围。
4. 网关 BLE 记录顺序与 Android 解析本轮已做**源码核对 + 宿主机映射验证**；网关固件正式核对属 ITEM-012。
5. 本轮顺带清除了 ITEM-007 登记的 `main.c` 陈旧注释（“立即/小时/首样本上报”已改为条件上报）。

## F. ITEM-008 判定

“条件上报（`report_req` 门控）”“`encode_frame10` 组 10 字节帧 + 有界发送”“帧布局/字节序/Feistel 不变”“仅成功后清除待上报”“失败按上限重试后放弃本轮”均经**真实 `measure.c` 发送路径**与**真实编码器↔真实网关解码器互操作**独立验证；AC5 告警消除，既有回归全部通过。

**判定：TEST_PASS**。

---

# ITEM-009 验证（旧通路退役：hall / OPTCFG / params / history）

验证对象：任务项 **ITEM-009**（从工程与构建中移除 hall/OPTCFG/params/history 及其对 PB04/PB05/PB06 的初始化；移除 GPIOB 霍尔 EXTI 分支与 LPTIM 中 OPTCFG 分支；PB04 仅作 AIN11 模拟输入、PB05 仅作光照供电输出、PB06 不外驱动）
被测提交：`fef788f`；源码基线：`c556196`
结论：**TEST_PASS**

## A. 变更范围

| 类别 | 内容 |
|---|---|
| 删除 | `USER/src/{hall,optcfg,params,history}.c`、`USER/inc/{hall,optcfg,params,history}.h`（共 8 个文件全部删除） |
| 头文件 | `main.h` 移除 4 个 include；`fw_core.h` 移除 params/OPTCFG 声明与常量（保留 `fw_crc16_ccitt`） |
| 源码 | `main.c` 移除 4 个 `*_init()` 与 hall/optcfg 主循环分支；`interrupts_cw32l010.c` 移除 GPIOB 霍尔分支与 LPTIM OPTCFG 分支；`measure.c` 移除 optcfg/hall 依赖（`send_data_to_gateway` 不再延后，`go_to_sleep` 仅看 report/sample）；`fw_core.c` 移除退役纯逻辑 |
| 测试 | `host_sensor_core_test.c` 删除退役用例（−278/+10）、`build_test.sh` 不再链接 `history.c`、`host_measure_flow_check.c` 调整 |

## B. 退役完整性（静态，通过）

- `USER/src`、`USER/inc` 目录中已无任何退役文件（现仅 encrytogate/fw_core/gxht40/interrupts/light/main/measure/sf_i2c）。
- 全树检索 `hall_/optcfg_/OPTCFG/params_/history_/HALL_/OPT_IN_PIN/F2_PWR_EN`：**无任何代码引用**，仅 3 处注释说明“已随 ITEM-009 退役”。
- 工程文件（`MDK/Project.uvprojx`、`EWARM/*.ewp`、`gcc/build.sh`）中退役源文件引用数为 0；`gcc/build.sh` 以通配符编译 `USER/src/*.c`，退役后不再产生对应目标文件。
- ELF 中无 `hall_*`/`optcfg_*`/`params_*`/`history_*` 符号；`fw_core.o` 仅剩 `fw_crc16_ccitt` + 本次需求新增的 7 个纯函数（params/OPTCFG 纯逻辑已消失）。

## C. 中断分支移除（机器级，通过）

对编译产物反汇编核对（比源码审阅更硬）：

```
GPIOB_IRQHandler:
  38c2:  4770   bx lr            <-- 空处理，霍尔 EXTI 分支已彻底移除
LPTIM_IRQHandler 调用的函数集合:
  bl  <app_gtimer_count_irq>     <-- 仅 433 位时钟，OPTCFG 采样分支已移除
```

另外：全树已无 `GPIO_IT_FALLING/RISING/BOTH` 等 EXTI 配置（仅 `GPIO_IT_NONE`），也无 GPIOB 的 NVIC 使能。

## D. 整机引脚所有权唯一（通过）

逐条列出 USER/src 中**全部** GPIO 配置/驱动调用点：

| 端口 | 引脚 | 模块 | 用途 |
|---|---|---|---|
| GPIOB | PB05 | `light.c` | `GPIO_Init(OUTPUT_PP)` + `GPIO_WritePin`（采样高/空闲低） |
| GPIOB | PB04 | `light.c` | `GPIO_Init(ANALOG)`（AIN11） |
| GPIOA | PA03/PA04 | `measure.c` | 软 I²C SCL/SDA |
| GPIOA | PA05/PA06 | `measure.c` | 调试 UART（采样路径不配置，已由 ITEM-006/008 harness 验证 `UART_Init` 调用数 = 0） |

即：**PB04 仅作 AIN11 模拟输入、PB05 仅作光照供电输出、PB06 全树无任何引用（不驱动）**；除 light.c 外无任何模块碰触端口 B。此结果同时解除了 ITEM-004 登记的“整机引脚冲突”依赖（当时 hall/optcfg 仍会重配 PB04/PB05/PB06）。

## E. 构建与资源（通过）

| 项 | 结果 |
|---|---|
| GNU 交叉编译 | 0 error；28 warning（无新增） |
| 退役目标文件 | `gcc/obj` 中无 hall/optcfg/params/history 目标文件 |
| 资源 | FLASH 35,856→**32,208 B**（−3,648）；RAM 1,896→**1,712 B**（−184）；`.bin` MD5 `47abc6df…` |
| AC5（`--c99`，CMSIS 5.9.0） | `main.c`/`interrupts_cw32l010.c`/`measure.c`/`fw_core.c`/`gxht40.c`/`light.c`/`sf_i2c.c` 均 **0 error / 0 warning**；`encrytogate.c` 0 error（3 条既有未用静态量告警） |

## F. 回归（通过）

| 独立 harness | 结果 |
|---|---|
| `host_fw_core_verify_ev`（纯逻辑穷举） | 53/53 |
| `host_gxht40_verify_ev` | 42/42 |
| `host_light_verify_ev` | 45/45 |
| `host_measure_flow_verify_ev` | 79/79 |
| `host_rf_report_verify_ev` | 30/30（T6 已按退役更新） |
| `host_rf_frame_verify_ev`（传感器编码器↔网关解码器） | 10/10 |
| `host_rtc_cadence_verify_ev` | 17/17 |
| 实现者 `host_measure_flow_check.c` | 24/24 |
| 工程自带 `test/build_test.sh` | 2/2（见 G.1） |

## G. 观察与交接

1. **【交 ITEM-010】`test/build_test.sh` 目前只跑 2 项**（`host_sensor_core_test.c` 保留的 CRC16 回归）：退役时删除了 278 行旧用例，而本次需求新增的纯逻辑用例位于单独的 `host_fw_core_pure_check.c`（36 项）**未并入脚本**。任务清单已把“更新 `host_sensor_core_test.c` 与 `build_test.sh` 覆盖 CRC-8/换算/滞回/上报判定”列为 **ITEM-010**，故本轮不判缺陷；但需注意：TD-002 T-L1-01 对 `build_test.sh` 的覆盖期望要等 ITEM-010 完成后才成立。**本轮验证强度不受影响**：我自己的 `host_fw_core_verify_ev` 已对相同纯逻辑做了穷举比对（53 项）。
2. **【交 ITEM-011】MDK/IAR 源文件列表仍不完整**：`Project.uvprojx` 仅列 `main.c/interrupts_cw32l010.c/sf_i2c.c/measure.c/encrytogate.c`（缺 `fw_core.c`/`gxht40.c`/`light.c`），IAR 仅列 `main.c/interrupts_cw32l010.c`。因此**生产工具链链接会因缺符号失败**（`measure.c` 现在引用 `light_*`/`gxht40_*`/`sensor_decide_report`）。这属 ITEM-011 明确范围；退役文件本来就不在列表中，故 ITEM-009 未引入新缺口。本环境 Keil 另受 CMSIS 6.3.0/AC5 不兼容影响（ITEM-001 已登记），因此本轮以 GNU 交叉编译 + AC5 逐文件编译为构建证据。
3. **测试侧适配（非产品改动）**：为反映退役，我更新了 3 个自有 harness —— `host_rf_report_verify_ev` 的 T6 由“配置窗口延后发送”改为“无遗留窗口延后 + 休眠门控仅看 report/sample”；`host_measure_flow_verify_ev`/`host_rtc_cadence_verify_ev` 删除已不再被引用的退役桩。
4. **板级回归未做**：TD-002 T-L9-02（磁铁不产生唤醒、PB06 无输出）需实板 + 示波器；本轮无仪器，仅以源码/机器码证明 EXTI 与 PB06 驱动已不存在。

## H. ITEM-009 判定

hall/OPTCFG/params/history 已从源码、头文件、构建与工程引用中彻底退役；GPIOB 霍尔 EXTI 分支与 LPTIM OPTCFG 分支在**机器码层面**消失（空处理 / 仅 433 位时钟）；整机引脚所有权变为唯一（PB04 仅 AIN11、PB05 仅光照供电、PB06 不驱动）；资源明显下降且既有功能回归全部通过。

**判定：TEST_PASS**。

---

# ITEM-010 验证（宿主机 L0 测试套件更新）

验证对象：任务项 **ITEM-010**（更新 `test/host_sensor_core_test.c` 与 `test/build_test.sh`：覆盖 CRC-8 参考向量、换算边界（负温、0%/100% 截断）、光照阈值与滞回、上报判定边界（恰好 0.9 ℃、恰好 35.0 ℃、无前值、非暗、失败不更新）；移除已退役的 OPTCFG/params 用例；脚本在本机运行通过）
被测提交：`2ab13d8`；源码基线：`b6277fd`
结论：**TEST_PASS**

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 变更范围审查 | `host_sensor_core_test.c` **+116/−23**（删退役用例、增本次需求纯逻辑用例）；`build_test.sh` **+10/−1**（改为两阶段入口） |
| 实际运行 | 脚本必须在本机通过且可重复（TD-002 T-L1-01） |
| **期望值独立性核对** | 防止“测试只自证实现”——用独立 Python 参考逐条复核测试中的每一个期望常量 |
| **门禁有效性** | 验证脚本在用例失败时确实返回非零（而不是永远绿） |
| 退役用例清理 | 确认无 OPTCFG/params/history/Manchester 残留 |

## B. 实际运行（通过）

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
CC=<mingw64 gcc> sh build_test.sh
```

| 阶段 | 内容 | 结果 |
|---|---|---|
| [1/2] | `host_sensor_core_test.c` + `fw_core.c`（纯逻辑） | **38 passed, 0 failed** |
| [2/2] | `host_measure_flow_check.c` + `measure.c`/`sf_i2c.c`/`fw_core.c` + `mock_measure_mcu`（采样/上报流程） | **24 passed, 0 failed** |
| 退出码 | `set -e` 两阶段均通过 | **0** |
| 编译告警 | 两次编译均为 `-Wall -Wextra` | **0 warning** |
| 重复运行 | 连续两次 | 结果一致（确定性） |

覆盖对照 TD-002：T-L0-06（CRC16 回归）→ 阶段 1 [1]；T-L0-01（CRC-8 参考向量/单字节/空长度）→ [2]；T-L0-02（温度换算与有效域，含负温与域外无效）→ [3]/[3b]；T-L0-03（湿度 0..1000 截断）→ [4]；T-L0-04（光照阈值与滞回）→ [5]；T-L0-05（上报边界：恰好 0.9 ℃、恰好 35.0 ℃、无前值、非暗、prev==cur）→ [6]；“**失败不更新前值/不构造 0 值**”属采样流程行为，由**阶段 2** 覆盖（[5]/[6]）。

## C. 期望值独立性核对（通过）

用独立 Python 参考（精确有理数 round-half-up + 从零写的 CRC + 直接按 IC-002 写的布尔式）逐条复核测试文件中的**全部期望常量**，30/30 一致：

| 组 | 复核内容 | 结果 |
|---|---|---|
| CRC16 | `"123456789"→0x29B1`、33 字节参考帧→`0xB540` | 一致 |
| CRC-8 | `{0xBE,0xEF}→0x92`、`{0x00}→0xAC`、空长度→`0xFF` | 一致 |
| 温度 | 1872→−400、16855→0、26214→250、63664→1250、0→−450、0xFFFF→1300 | 一致 |
| 湿度 | 0→0、2247→0、29360→500、55575→1000、0xFFFF→1000 | 一致 |
| 滞回 | 349/350（已明）、251/250（已暗）、0/4095、300（带内两种前态） | 一致 |
| 上报 | 无前值 200/351、下降 9/10/11、非暗、恰好 350、prev==cur | 一致 |

即：测试断言的是**需求/手册/契约推导出的正确值**，而非“把实现输出当期望”的自证式测试。

## D. 门禁有效性（通过，未修改仓库文件）

将 `test/` 与 `USER/`、`COMMON/`、`UM2005C/` 复制到临时目录，在副本中注入预期错误后运行 `build_test.sh`：

| 注入 | 退出码 | 输出 |
|---|---|---|
| 无（基线） | **0** | 38/0 与 24/0 |
| 阶段 1：CRC-8 期望 0x92→0x93 | **1** | `FAIL {0xBE,0xEF} -> 0x92: got 146 expect 147`，阶段 1 `37 passed, 1 failed`，且 `set -e` 使阶段 2 未执行 |
| 阶段 2：首个 `report_req` 期望反转 | **1** | 阶段 1 `38/0` 后阶段 2 `23 passed, 1 failed`，并打印对应 FAIL |
| 恢复后 | **0** | 38/0 与 24/0 |

证明该脚本是一个**真实门禁**（用例不满足即失败），且阶段间失败传播正确。

## E. 退役用例清理（通过）

- 测试文件与脚本中无 `params_*`/`optcfg_*`/`history_*`/`Manchester`/`decoder_t` 等退役用例（仅一处注释说明“ITEM-009 已移除 OPTCFG/params/history 用例”）。
- 脚本不再链接 `history.c`；仅依赖 `USER/inc`、`USER/src/{fw_core,measure,sf_i2c}.c`、`COMMON`、`UM2005C`、`test/mock_measure_mcu`（均已入库）。

## F. 观察与交接

1. **仓库卫生小项**：`build_test.sh` 运行后会在 `test/` 留下 `host_sensor_core_test.exe`、`host_measure_flow_check.exe`，而 `.gitignore` 未包含 `*.exe` → 会在 `git status` 中显示为未跟踪文件（本轮已清理）。建议后续把 `*.exe` 加入 `.gitignore` 或在脚本末尾清理；属卫生问题，不影响功能与判定。
2. `host_fw_core_pure_check.c`（36 项）与综合套件内容重叠但保留；当前 `build_test.sh` 只跑综合套件 + 流程套件。保留重叠无功能风险（互为交叉验证）。
3. 阶段 2 使用 `mock_measure_mcu` 阴影头（实现侧）；我方另有独立阴影头与独立 harness（`host_measure_flow_verify_ev` 79 项）作为交叉验证，两者结论一致。
4. 本轮完成后，TD-002 T-L1-01（“运行 `test/build_test.sh` 且覆盖新纯逻辑”）**已成立**（ITEM-009 登记的缺口已关闭）。

## G. ITEM-010 判定

`host_sensor_core_test.c` 已覆盖 CRC-8 参考向量、温度/湿度换算边界（含负温与 0/100% 截断）、光照阈值与滞回、上报判定边界（含恰好 0.9 ℃、恰好 35.0 ℃、无前值、非暗）；“失败不更新前值”由脚本阶段 2 覆盖；退役用例已彻底移除；`build_test.sh` 在本机两阶段 38+24 全部通过、退出码 0、零编译告警、可重复，且经注入验证为**真实门禁**；测试期望值经独立参考逐条确认正确。

**判定：TEST_PASS**。

---

# ITEM-011 验证（交叉编译与 MDK/IAR 工程源文件列表）

验证对象：任务项 **ITEM-011**（执行 `gcc/build.sh` 完成编译与链接并生成 elf/hex/bin；RAM/Flash 不超过 4 KB/64 KB；MDK/IAR 工程源文件列表与新增、移除的源文件保持一致）
被测提交：`eada262`；源码基线：`74f663f`
结论：**TEST_PASS**

## A. 变更范围

| 文件 | 变化 |
|---|---|
| `.gitignore` | **+1**：新增 `*.exe`（关闭 ITEM-010 登记的“测试脚本留下未跟踪 .exe”卫生项） |
| `EWARM/project.ewp` | **+64**：User 组补齐 8 个 `USER/src`；新增 UM2005C/COMMON 组与对应 include 路径；Driver 组补齐 adc/digitalsign/lptim/rtc/uart |
| `MDK/Project.uvprojx` | **+20**：User 组补 `fw_core.c`/`gxht40.c`/`light.c`；Driver 组补 `cw32l010_adc.c` |
| **源码/头文件** | **无任何改动**（`git show --numstat` 中 `USER/(src\|inc)/` 计数 = 0） |

## B. 交叉编译（通过）

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
sh gcc/build.sh
```

| 项 | 结果 |
|---|---|
| 退出码 | **0** |
| 编译/链接 | **0 error**；28 warning（全部为既有：encrytogate 未用静态量/缩进、main 模板参数与返回类型、sf_i2c maybe-uninitialized；**无一条指向 fw_core/gxht40/light/measure**） |
| 产物 | `gcc/obj/sensor_fw.elf`（156,508 B）、`sensor_fw.hex`（90,878 B）、`sensor_fw.bin`（32,292 B）均生成 |
| 目标 | Cortex-M0+（`-mcpu=cortex-m0plus -mthumb`） |

## C. 内存预算（通过）

| 区域 | 占用 | 限额 | 判定 |
|---|---|---|---|
| FLASH（text+data） | **32,208 B**（链接器报告 49.15%；`size` text 32,208 + data 84 = 32,292） | 64 KB = 65,536 B | **PASS**（余量约 50%） |
| RAM（data+bss） | **1,712 B**（链接器报告 41.80%；data 84 + bss 1,628） | 4 KB = 4,096 B | **PASS**（余量约 58%） |

## D. MDK/IAR 工程源文件列表一致性（通过，含功能完整性）

路径按各自工程目录解析（MDK 的 `FilePath` 相对于 `MDK/`，IAR 的 `$PROJ_DIR$` 相对于 `EWARM/`）。

| 检查 | 结果 |
|---|---|
| 条目数 | MDK 25、IAR 25 |
| **每条路径存在** | 全部存在（无悬空引用） |
| 两列表集合 | **完全相同**（唯一差异是工具链各自的 startup：`IdeSupport/MDK/startup_cw32l010.s` vs `IdeSupport/EWARM/startup_cw32l010.s`） |
| `USER/src` 一致性 | 实际 8 个 .c == 列表 8 个（encrytogate/fw_core/gxht40/interrupts/light/main/measure/sf_i2c） |
| 新增文件 | `fw_core.c`/`gxht40.c`/`light.c` 在**两个**列表中均存在 |
| 退役文件 | `hall/optcfg/params/history` 在列表中**零出现** |
| 新增依赖 | MDK 补 `cw32l010_adc.c`（light.c 需要）；IAR 补 `UM2005C`/`COMMON` 的 include 路径（gxht40.c 需要 `delay.h`，measure.c 需要 `app_um2005c.h`） |
| **功能完整性（链接验证）** | 用 GNU 工具链编译两列表的 25 个文件（工具链专用 startup 以 `gcc/startup_cw32l010.S` 等价替代）→ 均 **0 error**，链接**无未定义符号**；MDK 集合 FLASH 32,300 B / RAM 1,696 B，IAR 集合 FLASH 32,300 B / RAM 1,704 B |

即：列表不仅“名字对齐”，而且**足以完整链接出固件**（无缺文件）。

> 说明：Keil/IAR 专用 startup 使用 ARMASM / IAR 语法，无法用 GNU `as` 汇编（报 `bad instruction`），这是**预期**而非缺陷；`gcc/build.sh` 使用 `gcc/startup_cw32l010.S`。

## E. .gitignore 与仓库卫生（通过）

运行 `test/build_test.sh`（38/38 + 24/24，exit 0）后 `git status` **为空**——`*.exe` 已入 `.gitignore`，ITEM-010 登记的卫生项已关闭。

## F. 回归（通过）

| 项 | 结果 |
|---|---|
| `test/build_test.sh` | 38/38（纯逻辑）+ 24/24（采样/上报流程），exit 0 |
| 独立 harness 抽查（本项未改源码，结论沿用） | fw_core 53/53、measure 流程 79/79、空口帧互操作 10/10 |

## G. 观察与交接

1. **两套构建的编译集合不完全相同（非缺陷）**：`gcc/build.sh` 以通配符编译 `Libraries/src/*.c`（**23 个**），而 MDK/IAR 只列实际使用的 **9 个**库文件；未使用的库代码在 GNU 链接时被 `--gc-sections` 丢弃，因此内存数字仍只反映实际使用代码。验收要求的是“工程列表与新增/移除的源文件一致”，项目自有源码集合（USER/src/UM2005C/COMMON）两列表完全一致，已满足；此差异仅记录备查。
2. **Keil 全量构建仍不可用**：本机 CMSIS 解析到 6.3.0 与 AC5 不兼容（ITEM-001 已登记定位），`mdk_build` 会报 `Unknown compiler`；因此 `mdk_flash`/`mdk_build` 不能作为构建证据（无 Verify OK 不得判成功）。本轮构建证据 = GNU 交叉编译 + AC5 逐文件编译（ITEM-009/010 已做）+ 上述“列表功能完整性链接”。
3. **板级验证未做**：烧录/运行/串口/波形观测本轮无设备确认（无 SWD 探针枚举、无实测环境），属 TD-002 T-L2/L4/L8 范围。

## H. ITEM-011 判定

`gcc/build.sh` 编译链接通过并生成 elf/hex/bin，FLASH 32,208 B（≤ 64 KB）与 RAM 1,712 B（≤ 4 KB）均在预算内；MDK 与 IAR 工程源文件列表已补齐新增源文件、无退役残留、路径全部存在、与 `USER/src` 实际集合一致，且两列表均可完整链接（零未定义符号）；`.gitignore` 补齐 `*.exe`。

**判定：TEST_PASS**。

---

# ITEM-012 验证（网关 CH592 beiwov2 兼容性核对）

验证对象：任务项 **ITEM-012**（确认 `decode_frame10` 取 `temp=p[4..5]`、`hum=p[6..7]`；BLE 每设备 8 字节记录为 `id|hum_be|temp_be` 且与 Android `parsePlainFrame` 的 `u16be@4`/`s16be@6` 一致；一致则不改代码并记录结论）
被测提交：`6769d79`（仅改传感器侧测试与文档；**未改网关/Android**）
结论：**一致，无需修改；TEST_PASS**

## A. 选测说明

本项是**只读跨组件核对**（网关 CH592 固件 + Android 解析），不属传感器固件改动。本轮采用三种独立手段：
1. **逐段源码核对**（每个字节操作的出处）；
2. **独立端到端 harness**：链接**真实传感器编码器**与**真实网关解码器**，并忠实复刻网关的组帧/打包与 Android 的解析代码，跑完整链路；
3. **变更历史核对**（确认网关/Android 未被本工作流改动）。

## B. 逐段源码核对（通过）

| 环节 | 源码位置 | 实际行为 |
|---|---|---|
| 传感器组帧 | `USER/src/encrytogate.c:encode_frame10` | `uid[0..3] | temp_LE(2) | hum_LE(2) | crc16_LE(2)`，8 轮 Feistel |
| 空口 | 433 MHz 10 B | 不变（IC-002 §3） |
| 网关解密 | `APP/feistel_al.c:decode_frame10` | `uid=p[0..3]`；**`temp = p[4] | p[5]<<8`（小端 int16）；`hum = p[6] | p[7]<<8`（小端 uint16）**；crc16 校验覆盖 p[0..7] |
| 网关组 resbf | `APP/app_um2006A.c:169..172` | `resbf[4..5]=temp 大端`、`resbf[6..7]=hum 大端` |
| **网关记录交换** | `APP/app_um2006A.c:192..195` | `sensorres[4..5]=resbf[6..7]`（湿度大端）、`sensorres[6..7]=resbf[4..5]`（温度大端）→ **每设备 8 B = `id | hum_be | temp_be`** |
| 设备归因 | `APP/app_um2006A.c:180` | 仅当 `bind_device_id[i][0..3] == resbf[0..3]` 才写入该设备槽 → 按 ID 归因，不靠顺序 |
| 打包 | `APP/bleencrypt.c` | `FRAME_BLOCK_SIZE=16`、`DEV_PAYLOAD_LEN=8`；头部块 `gwid6[0..5] + devCount[6] + pad9`；设备块 `payload8` 原样置于 `out16[0..7]` + pad8；帧长 `16*(1+devCount)` |
| BLE 加密 | `APP/app_um2006A.c:71..79, 209` | `encrypt_frame_ecb_inplace(..., APP_AES_KEY16)`，按 16 B 块 `LL_Encrypt`（AES-128 ECB），再 `SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR2,...)` |
| Android 解密 | `MqtttService.kt:501` | `Cipher.getInstance("AES/ECB/NoPadding")`，要求密文 16 B 对齐 |
| Android 解析 | `MqtttService.kt:parsePlainFrame` | `gwid6=plain[0..5]`、`devCount=plain[6]`、`expected=16*(1+devCount)`、`off` 从 16 起步进 16、`p8=block[0..7]`、**`humi=u16be(p8,4)`、`temp=s16be(p8,6)`** |

逐段对应：传感器 `temp/hum` 小端 → 网关解密后转大端存入 resbf → 记录内交换为“湿度在前” → Android 按 `u16be@4`（湿度）/`s16be@6`（温度）读取。**全链一致。**

关键常量独立比对：**AES 密钥 16 字节与 Android 逐字节相同**（`91 4E 03 B7 C2 5A 88 1D F4 60 7B 2E A9 17 6C 55`）；帧长规则、记录 8 B、块 16 B 两边一致；网关无对自身温度历史的二次过滤（`tempervalue`/`humivalue` 仅用于组帧）。

## C. 独立端到端 harness（通过）

链接**真实** `encrytogate.c` + **真实** `feistel_al.c`，并**逐字复刻**网关 `bleencrypt.c` 的 `build_header_block`/`build_device_block`/`build_padded_frame_blocks`、`app_um2006A.c` 的记录交换，以及 Android 的 `parsePlainFrame`/`u16be`/`s16be`。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
<mingw64 gcc> -std=c11 -Wall -Wextra -Wno-unused-function -Wno-unused-const-variable \
  -Wno-misleading-indentation -I ../USER/inc -I <gateway APP> host_gw_compat_verify_ev.c \
  ../USER/src/encrytogate.c <gateway APP>/feistel_al.c -o gw_compat_verify && ./gw_compat_verify
=> ==== result: 11 passed, 0 failed ====
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| 全链向量 | 8 组值（含 −100/−400/1250/351/0/289/350 ℃ 与 0/7/100/500/999/1000 %RH）经“传感器编码 → 网关解密 → 网关记录 → 打包 → Android 解析”后 id/温度/湿度均一致 | PASS |
| 多设备 | 4 设备一帧：帧长 = 16*(1+devCount)、头部 `gwid6[0..5]`+`devCount[6]`、每设备 id/温/湿归因正确、设备块前 8 B 与记录逐字节相同 | PASS ×5 |
| **负对照** | 故意按旧顺序 `id|temp_be|hum_be` 组记录 → Android 读出错位值（未通过）→ 证明上述正向结论非空验 | PASS |
| 帧长规则 | devCount 0..5 时网关 `16*(1+N)` == Android `expected` | PASS |
| ID 透传 | 空口 4 B == 编码器 `uid[0..3]` == 记录 4 B（网关原样拷贝，不改序） | PASS ×3 |

实现者自带 harness 也被独立执行确认（需额外链接网关 `bleencrypt.c`，见 G.1）：`host_rf_frame_check.c` → **22 passed, 0 failed**。

## D. 变更历史核对（通过）

| 对象 | 结论 |
|---|---|
| 网关 `CH592EVT/.../beiwov2/` | 仅 `c3baaaa`（**本工作流基线 `b32ea8a` 之前**）修改过 `app_um2006A.c` 的记录顺序（交换为 `id|hum_be|temp_be`，与 IC-001/IC-002 一致）；自 `b32ea8a` 起的本工作流**未触碰**网关任何文件 |
| Android `dengbei_care/` | 自基线导入 `cc6c6f7` 以来**零改动** |
| 本项实现提交 | `6769d79` 仅改传感器侧 `test/host_rf_frame_check.c` 与文档，**无网关/Android 改动**——符合“一致则不改代码”的任务要求 |

## E. 观察与交接

1. **实现者证据的复现命令缺一个源文件**：`evidence/protocol_test.md` 的 ITEM-012 命令未包含网关 `bleencrypt.c`，按原文链接会报 `undefined reference to build_device_block / build_padded_frame_blocks`（我已实测复现）；补上后 22/22 通过。属**证据文档级**问题（非产品缺陷），建议补全命令。
2. **既有“超时无传感器”路径会发布全 0 记录**：网关在 `timeoutcnt` 用尽后 `memset(sensorres,0,...)` 并发布（`id=00000000`）。这是**基线已有、本工作流未改动**的行为；因零 ID 不匹配任何绑定设备，不会把真实传感器数据错配。但 IC-002 §5 提到“不将未收到数据解释为零值”——建议 App/服务器侧确认对零 ID 记录的现有处理（登记为产品级备注，非本项缺陷）。
3. **设计文档措辞与实际 ID 取值不同（无功能影响）**：FD-002 §8.1 写“`mcu_uid` 取 `ptr[0],[3],[6],[8]`”，而 `encrytogate.c` 的 `UID_IDX={0,1,2,3}` 使空口 ID 实际为 `uid[0..3]`（`measure.c` 里写入 `send_data[0..3]` 的代码被 `encode_frame10` 覆盖，ITEM-008 已证为死代码）。网关与 Android 均原样透传这 4 字节，**功能一致**；建议更正设计文档措辞。
4. **真实射频/BLE 空中链路未实测**：本项为软件级一致性核对；调制/前导/空中速率/丢包、BLE 实际收发与 App 读数需真实硬件（TD-002 T-L7、T-L2）。

## F. ITEM-012 判定

`decode_frame10` 的 `temp=p[4..5]`/`hum=p[6..7]` 与 BLE 每设备 `id|hum_be|temp_be`、Android `u16be@4`/`s16be@6` **逐段一致**；AES 密钥与 ECB 块处理一致；网关无二次过滤；网关与 Android 自本工作流基线以来未被修改，且本项实现未改其代码——**符合“一致则不改代码并记录结论”**。

**判定：TEST_PASS**。
