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

