# 嵌入式验证证据 — ITEM-008 最终构建与静态核对（独立验证）

- 能力：`embedded_tester.embedded_verification`
- 项目：dengbei_run10
- 任务：`TL-6c897e7b-38c9-4f2a-9d55-d216ddfaaff2-tasks-ITEM-008`
- 结论：**TEST_PASS（仅限本 item 的构建/静态/诊断行预算范围）**
- 说明：本证据不构成产品验收；U01（真实 GXHT40 ACK）、U02（全暗标定）与端到端/真实总功能仍未闭合。

## 1. 受测身份

| 项 | 值 |
|---|---|
| Git HEAD | `c176d02dd0e6220a605c90bf34e84ac7a929565b`（工作区开始验证时干净） |
| 源码基线（对比） | `8ae4a33`（改写前固件） |
| 工程 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx` |
| 编译器 | Keil MDK ARMCLANG `V6.24` |
| `Project.axf` md5 | `9cb97918e8832811dfcb00a93646fba1`（261,832 B） |
| `Project.hex` md5 | `3af11e7adb21ba43004fc827649adb44`（57,185 B） |
| 产品源码 SHA-256 | `source_sha256.txt`（6 个文件） |

产品源码相对 `8ae4a33` 仅 6 个文件变更：`fw_core.h`、`sensor_config.h`、`debug_trace.c`、`fw_core.c`、`main.c`、`measure.c`；其余变更全部位于 `test/` 测试资产。网关 `CH592EVT/.../beiwov2`、`encrytogate.c`、`UM2005C/**`、`light.c`、`gxht40.c`、`sf_i2c.c` 无变更。

## 2. 独立构建（本能力执行）

`mdk_build {"action":"rebuild"}` → 原始日志 `build_rebuild.log`：

```text
*** Using Compiler 'V6.24', folder: 'C:\Keil_v5\ARM\ARMCLANG\Bin'
Rebuild target 'Project'
../USER/src/main.c(252): warning: while loop has empty body [-Wempty-body]
Program Size: Code=19544 RO-data=644 RW-data=116 ZI-data=1684
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).
```

- **0 Error / 1 既有 warning**：唯一告警为 `main.c(252) while(k--);` 延时空循环。
- Flash = Code+RO+RW = 20,304 B / 64 KB（31.0%）；RAM = RW+ZI = 1,800 B / 4 KB（43.9%）。
- 构建成功只证明可生成镜像，不代表板上功能。

## 3. 独立静态核对（23/23 PASS）

脚本：`static_audit_item008.py`；原始输出：`static_audit.out`。未复用实现侧
`evidence/ev027_item008/static_audit.py`。

| # | 检查 | 结果 |
|---|---|---|
| 1 | 受测 Git HEAD 记录 | PASS |
| 2 | 产品源码变更集恰为 6 个已审阅文件 | PASS |
| 3 | USER 之外仅 `test/` 资产变化（RF/网关/light/gxht40/sf_i2c/encrytogate 未动） | PASS |
| 4 | 注释剥离后无 hall/OPTCFG/params/history 有效调用（0 token） | PASS |
| 5 | USER 无 NVM/Flash 参数写调用 | PASS |
| 6 | 433 帧长仍 10 B，无新增 RF 字段 | PASS |
| 7 | 新增源码行无 RF/BLE/上传字段 | PASS |
| 8 | `sample_flag=1` 仅初值与 RTC 节拍（2 处） | PASS |
| 9 | `temperature_process()` 在 USER 中仅 1 处调用 | PASS |
| 10 | `send_data_to_gateway()` 在 USER 中仅 1 处调用 | PASS |
| 11 | 采样节拍仍为 RTC 1 min × 3；无新增定时器/回调 | PASS |
| 12 | 每个采样机会最多 1 个受 `sample_flag` 门控的事务块 | PASS |
| 13 | 19 个引脚/外设宏取值与基线一致 | PASS |
| 14 | 配置中仅移除被改名的旧门控常量 `SENSOR_REPORT_DROP_X10` | PASS |
| 15 | `interrupts_cw32l010.c` 未变更 | PASS |
| 16 | 新增行无 GPIO/NVIC/RTC 配置 | PASS |
| 17 | 无新增 IRQ handler | PASS |
| 18 | 构建日志编译器 V6.24 | PASS |
| 19 | 构建 0 Error / 1 Warning | PASS |
| 20 | 唯一告警为既有 `main.c:252` 空循环 | PASS |
| 21 | Code/RO/RW/ZI = 19544/644/116/1684 | PASS |
| 22 | S/G 最坏行 ≤96 B（独立 harness） | PASS |
| 23 | 独立 harness 0 failed | PASS |

结论：无 hall/OPTCFG/params/history 有效调用、无新增 NVM 参数写、无新增 RF/BLE/上传字段、
无心跳/额外采样、引脚与中断边界未变。

### 3.1 既有宿主机套件回归（实现侧资产，仅回归参考）

`test/build_test.sh` 在最终工作树上复跑：**295 passed / 0 failed**（7 组：
57 + 52 + 72 + 51 + 28 + 29 + 6；`host_tests_regression.log`）。该套件为实现侧资产，
不能替代本能力的独立判据，仅用于确认最终树无回归。

## 4. S/G 行最坏长度（独立宿主机 harness）

`host_diag_line_verify_ev.c` 直编当前产品 `USER/src/debug_trace.c`，使用本能力自建
`test/mock_diagline_ev` 影子 MCU 层；编译/运行：`compile_diag_line.err`、
`diag_line_verify.out`（**36 passed / 0 failed**）。

| 行 | 构造的最坏值 | 线上字节（含 CRLF） | 预算 |
|---|---|---|---|
| S | tick=4294967295、p/t=-1250、n/x/a=4095、h=1000、s=2、y=3 | **92 B** | 96 B |
| G | s=5、a44=a45=1、rd=5、at=3、raw=12 位大写 HEX | **46 B** | 96 B |
| BUS | 8 个 ACK 地址 + `+`、idle=3 | 53 B | 96 B |
| IOTEST / IOSIG / VDD / BOOT | 文档最坏值 | 49 / 27 / 39 / ≤96 B | 96 B |

注：实现侧 `evidence/ev027_item008/driver_test.md` 报告的 “worst S=67 B” 来自其自检所选
样本值；按字段格式的**真正最坏宽度**为 92 B（`debug_trace.c` 末尾注释亦为 92 B），仍满足
≤96 B。本项以 92 B 作为独立判据。

## 5. 实板下载与 COM43 观测（补充证据）

- `mdk_flash`：`Erase Done. Programming Done. Verify OK. Application running ...`
  （`mdk_flash.log`）；下载后 axf/hex md5 与构建一致（`image_md5_after_flash.txt`）。
- COM43 9600 8N1、300 s 原始字节 `com43_capture_a.dat`（112 B，md5
  `5a6a5731ce57e07d0056a21e991a6a90`），独立解析 `verify_com43_item008.py`
  → **13 passed / 0 failed**（`verify_com43.out`）：

```text
S k=3 p=0 H=0 V=0 o=8 n=40 x=43 a=41 D=0 t=0 h=0 q=0 r=0 s=0 y=0\r\n   (66 B)
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------\r\n                      (46 B)
```

- 300 s 窗口内**恰有 1 个采样周期**（k=3），无额外采样、无心跳、无周期间输出；仅 CRLF；
  每行 ≤96 B；G 紧随对应 S。
- `q=0` 与 `G s=2 a44=0 a45=0` 是已知的 GXHT40 无 ACK 现场状态（U01），不是本 item
  引入的回归；S/G 行格式、节拍和行长均符合要求。
- 限制：BOOT 横幅是上电一次性输出，本次 `mdk_flash` 复位与串口采集启动之间存在间隙，
  未能入窗；BOOT 标识 `RUN10-FDS003` 已由独立 harness 逐字节验证。

## 6. 局限与交接

| 项 | 状态 |
|---|---|
| BOOT 实板窗口 | 本次未捕获（上电一次性，采集启动晚于复位）；标识由 host harness 验证 |
| U01 GXHT40 真实 ACK/连续采样 | 仍未闭合；本次实板 q=0/G s=2 复现该现场状态 |
| U02 全暗 C_dark 标定 | 未完成；`LIGHT_DARK_CALIBRATED=0`，`V=0/D=0` 符合未标定语义 |
| 产品总功能/端到端验收 | 未执行，不属本 item |
| 旧测试 harness | `host_measure_flow_verify_ev.c` 旧模型仍待按 TD-003 更新（本 item 未使用） |

本 item 的交付范围（构建 0 Error、镜像身份、静态边界、S/G ≤96 B）已独立验证通过；
上述未闭合项由后续能力/真实总功能测试负责，不得据本证据宣称产品验收通过。
