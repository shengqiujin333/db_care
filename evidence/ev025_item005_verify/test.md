# 嵌入式验证证据（EV-025，ITEM-005 主循环单次事务 / BOOT / UID 零）

能力：`embedded_tester.embedded_verification`
受测项：`TL-6c897e7b-38c9-4f2a-9d55-d216ddfaaff2-tasks-ITEM-005`
被测行为：每个采样机会只执行一次完整事务（光照→温湿度→判定→冻结→最多 3 次发送→唯一 `S`，失败追加 `G`→关闭 UART→深睡）；BOOT 位于 UID 零致命判断之前；UID 和为零时清 `sample_flag/report_req`、PB05 低/ADC 关、关闭 UART 并真正深睡，不采样/不发送/不空转；RTC 1 min×3 与 GPIOA/I2C/SWD 边界不变。
结论：**TEST_PASS（ITEM-005 本增量；宿主机独立 harness + 实板正常路径观测）**
日期：2026-10-21（本地 11:47–12:09）

> 范围声明：本结论只覆盖 ITEM-005 的主循环事务边界、BOOT 顺序、UID 零致命路径与声明的边界不变量。
> UID 零无法在真实板上构造（芯片 UID 固定非零），该路径由直接编译**当前产品** `main.c` 的宿主机 harness 证明；
> 实板证明当前镜像已下载、按 180 s 节拍运行、每个机会恰一条 `S`/失败一条 `G`、间期静默、第二次下载仍 `Verify OK`。
> GXHT40 无 ACK（U01）依旧存在（`q=0`），因此实板上没有触发发送的周期；`S` 在发送之后、冻结帧与 1..3 次发送的实板可见部分本轮不可观测，属 ITEM-003 已独立验证范围，未被本项掩盖。

---

## 1. 受测件身份

| 项 | 值 | 来源 |
|---|---|---|
| Git HEAD | `51d8fcc1c459993ef8edefd1949ac276854114e0`（ITEM-005 实现提交；本轮开始时工作区干净） | `git_head.txt` |
| 被测源码 | `USER/src/main.c` SHA-256 `9024055c9bf1b1c63070bc51fdc9479007bcb0bf8e0ff8b93f07d643ce30652e`；`sensor_config.h` `29123ecc…`；`measure.c` `0fe63832…`；`debug_trace.c` `1c2523e3…` | `source_sha256.txt` |
| 构建 | `mdk_build rebuild`：0 Error / 1 既有 warning（`main.c(252) while(k--);`），Code=19400 RO=644 RW=116 ZI=1684 | `build_rebuild.log` |
| 镜像 | `Project.axf` md5 `e0437fd3405531f107bec844d815acf6`；`Project.hex` md5 `c17f42eba0c1c38461c3768c99e68f22`（与实现记录一致，确定性重建） | `image_md5.txt` |
| 下载 1 | `mdk_flash` 11:53:56：`Erase Done. Programming Done. Verify OK. Application running ...` | `mdk_flash.log` |
| 下载 2 | `mdk_flash` 12:08:08：再次 `Verify OK`（运行后调试/SWD 通道仍可用） | `mdk_flash_second.log` |
| 目标板 | 传感器板 CW32L010F8（GXHT40 + 光敏分压 + UM2005C），调试串口 COM43 | 环境配置 + 枚举 |
| 调试串口 | COM43，Prolific `067B:2303`，9600 8N1 | `serial_ports.txt` |
| 调试器 | `VID_C251/PID_F001`（COM26 为其 CDC 口，非传感器 UART1） | `usb_devices.txt` |
| 宿主机编译器 | MinGW-W64 gcc 12.2.0，`-std=c11 -Wall -Wextra` | `compile_*.err` |

本轮重建的 axf/hex md5 与实现证据 `evidence/ev024_item005/image_md5.txt` 完全一致。

---

## 2. 验证方法（独立于实现侧自检）

| 手段 | 文件 | 说明 |
|---|---|---|
| 主流程 harness | `test/host_main_flow_verify_item005.c` | 直接编译**当前产品** `USER/src/main.c`（`-Dmain=firmware_main_entry`），影子 MCU 层 `test/mock_main_ev/`；用事件记录器捕获调用顺序，用 `setjmp/longjmp` 有界退出产品无限循环 |
| 独立判据 | `verify_main_flow_item005.py` | 只读取 harness 原始 `EVENT_SEQUENCE/EVENT_COUNTS` 行，独立重算 ITEM-005 不变量，不信任 harness 的 PASS 行 |
| 父提交负向对照 | `old_main_pre_item005.c`（`git show 847b230:…/main.c`）+ `harness_main_flow_old.out` + `verify_old.out` | 同一 harness/同一刺激编译 ITEM-005 之前的主循环，必须被判为失败 |
| 静态/边界核对 | `static_check_item005.py` / `static_check.out` | BOOT 顺序、UID 零块内容、单次事务调用点、RTC ISR、GPIOA/I²C/SWD/UART/Flash 边界 |
| RTC 节拍回归 | `test/host_rtc_cadence_verify_ev.c` + `test/host_item005_link_stubs.c` | 用当前 `main.c` 重跑 1 min×3 节拍用例（`rtc_verify.out`） |
| 实板观测 | `com43_capture_a.bin` / `com43_capture_b.bin` + `verify_com43_item005.py` | 下载后 COM43 原始字节（300 s×2），独立解析 |
| 回归参考 | `host_tests_regression.log` | 实现侧既有宿主机套件 5 组 233/0（仅参考，不替代独立判据） |

复现命令（在 `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/`）：

```sh
EV=D:/mypro/beiwo2/AIP/evidence/ev025_item005_verify
# current implementation
gcc -c -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Dmain=firmware_main_entry \
    -Itest/mock_main_ev -IUSER/inc -IUM2005C -ICOMMON USER/src/main.c -o $EV/main.o
gcc -c -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 \
    -Itest/mock_main_ev -IUSER/inc -IUM2005C -ICOMMON \
    test/host_main_flow_verify_item005.c -o $EV/harness.o
gcc $EV/main.o $EV/harness.o -o $EV/harness_main_flow.exe
$EV/harness_main_flow.exe > $EV/harness_main_flow.out
python $EV/verify_main_flow_item005.py $EV/harness_main_flow.out
# parent commit negative control
gcc -c -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Dmain=firmware_main_entry \
    -Itest/mock_main_ev -IUSER/inc -IUM2005C -ICOMMON $EV/old_main_pre_item005.c -o $EV/old_main.o
gcc $EV/old_main.o $EV/harness.o -o $EV/harness_main_flow_old.exe
$EV/harness_main_flow_old.exe > $EV/harness_main_flow_old.out
python $EV/verify_main_flow_item005.py $EV/harness_main_flow_old.out --expect-old
# RTC cadence regression with the current main.c
gcc -c -std=c11 -Wall -Wextra -ffunction-sections -fdata-sections -DSENSOR_DEBUG_UART=0 \
    -Dmain=firmware_main_entry -Itest/mock_main_ev -IUSER/inc -IUM2005C -ICOMMON \
    USER/src/main.c -o $EV/main_rtc.o
gcc -c -std=c11 -Wall -Wextra -ffunction-sections -fdata-sections -DSENSOR_DEBUG_UART=0 \
    -Itest/mock_main_ev -IUSER/inc -IUM2005C -ICOMMON test/host_rtc_cadence_verify_ev.c -o $EV/rtc.o
gcc -c -std=c11 -DSENSOR_DEBUG_UART=0 -Itest/mock_main_ev -IUSER/inc \
    test/host_item005_link_stubs.c -o $EV/stubs.o
gcc -Wl,--gc-sections $EV/main_rtc.o $EV/rtc.o $EV/stubs.o -o $EV/harness_rtc_cadence.exe
$EV/harness_rtc_cadence.exe > $EV/rtc_verify.out
python $EV/static_check_item005.py
python $EV/verify_com43_item005.py $EV/com43_capture_a.bin $EV/com43_capture_b.bin
```

---

## 3. 宿主机主流程验证（当前产品源码）

原始观测：`harness_main_flow.out`；独立判据：`verify_current.out`（**26 checks passed / 0 failed**）。

### 3.1 UID 零致命路径

预置 `sample_flag=1`、`report_req=1`，令 UID 10 字节全 0；有界观测退出前记录 5 次深睡。

| 观测量 | 实测 |
|---|---|
| 事件序列 | `BOOT VDD IOTEST IOSIG BUS LIGHT_INIT DEBUG_TRACE_FLUSH_CLOSE DEEP_SLEEP×5` |
| BOOT/诊断 | 各 1 次，且全部在 UID 零分支动作之前 |
| `light_init()` | 1 次（PB05 低 + ADC 配置后关闭，见 §5 静态核对） |
| UART 关闭 | 1 次 |
| `temperature_process` / `send_data_to_gateway` / trace fetch / `S` | 0 / 0 / 0 / 0 |
| `go_to_sleep()`（旧空转入口） | 0 次 |
| 深睡 | 连续 5 次；进入后无任何业务调用 |
| 标志 | `sample_flag=0`、`report_req=0` |

结论：BOOT 先于致命判断；UID 零不采样、不发送、不打印 `S`，清标志后直接进入深睡循环，不因预置 `sample_flag=1` 空转。

### 3.2 正常路径：每机会一次完整事务、唯一 `S`

初始 1 次上电机会 + 第 2 轮另起 1 次机会，其余 6 轮为空闲轮。

| 观测量 | 实测 | 判定 |
|---|---|---|
| `temperature_process` 调用 | 2 | 每机会 1 次 |
| `send_data_to_gateway` 调用 | 2 | 每机会 1 次；不依赖多轮主循环重试 |
| trace fetch / `S` | 2 / 2 | 每机会恰一条 `S` |
| 单次事务顺序 | `temp → send → trace → S → close → sleep → deep sleep` | 与验收顺序一致 |
| 主循环轮数 / 深睡次数 | 9 / 8 | 唯一未深睡的一轮是置起下一机会的 RTC 轮 |
| 空闲轮业务调用 | 0 | 无跨轮补发/重复采样 |
| 标志消费 | `sample_flag=0`、`report_req=0` | 单次事务内消费 |

### 3.3 父提交负向对照

`harness_main_flow_old.out`（父提交 `847b230` 的 `main.c`，同一 harness）：**10 passed / 16 failed**；独立判据 `verify_old.out` = **NEGATIVE_CONTROL_OK**。

- UID 零：旧实现 50 次 `GO_TO_SLEEP` 后仍有界未退出、0 次深睡、无 BOOT、未清标志（对应 D-05 空转）；
- 正常路径：9 轮主循环产生 9 次 `temperature_process` / 9 次发送 / 9 条 `S`，而不是每机会 1 次（对应 D-03）。这证明本 harness 针对的正是 ITEM-005 改变的行为，不是“新旧都全过”。

---

## 4. 静态与边界核对

`static_check.out`：**47 passed / 0 failed**。

| 检查组 | 结果 |
|---|---|
| BOOT 在 `if (k == 0u)` 之前；VDD/IOTEST/IOSIG/BUS 同样在前 | PASS |
| UID 零块：清两标志、`light_init()`、`debug_trace_flush_close()`、`while(1) SYSCTRL_GotoDeepSleep()`；无采样/发送/trace/go_to_sleep | PASS |
| 旧 `while(1){ go_to_sleep(); }` 空转路径不存在 | PASS |
| 主循环由 `sample_flag != 0` 门控；事务顺序 temp→send→trace→S→close；`close/sleep` 每轮执行 | PASS |
| `main.c` 中 `temperature_process`/`send_data_to_gateway`/`debug_trace_sample` 各仅 1 个调用点 | PASS |
| RTC：`RTC_INTERVAL_EVERY_1M`、`SENSOR_SAMPLE_TICKS=3`、`SENSOR_RTC_TICK_PERIOD_MIN=1`；ISR 只计数/置标志，无 GPIO/ADC/UART/I2C/GXHT40/RF 调用 | PASS |
| 无 `SYSCTRL_AHBPeriphReset`（GPIOA 不复位）；USER 源码无 `GPIO_PIN_7/8`（PA07/PA08 SWD）引用 | PASS |
| Flash 读保护两行保持注释 | PASS |
| UART 关闭只复位/关时钟 UART1、只回收 PA06/PA05；`light_init()` 将 PB05 拉低且 ADC 配置后关闭 | PASS |
| `go_to_sleep()` 仅在 `report_req==0 && sample_flag==0` 时深睡 | PASS |

## 4.1 RTC 1 min×3 回归

`rtc_verify.out`：**17 passed / 0 failed**。使用当前 `main.c`，覆盖 RTC 1 分钟间隔配置、3 tick 一次机会、计数 0..2 回绕、30 tick 恰好 10 次机会。

## 4.2 既有宿主机套件（参考）

`host_tests_regression.log`：5 组 **233 passed / 0 failed**（53/72/51/28/29）。仅作回归参考，不作为本项独立判据。

---

## 5. 实板观测（当前镜像已实际下载）

| 项 | 事实 |
|---|---|
| 下载 1 | `Verify OK. Application running ...`，11:53:56 本地 |
| 捕获 A | COM43 9600 8N1，300 s（03:54:01–03:59:01 UTC），112 B：1 个周期 |
| 捕获 B | COM43 9600 8N1，300 s（03:59:05–04:04:06 UTC），224 B：2 个周期 |
| 周期内容 | 每周期恰一条 `S`，失败周期紧随一条 `G`；`S` 66 B、`G` 46 B（含 CRLF，均 ≤96 B） |
| 节拍 | `k=3 → 6 → 9`，相邻增量 3（1 min×3 = 180 s） |
| 间期静默 | 每个周期后无任何额外字节；第二窗口内两条 `S` 间隔 3 个 tick |
| 独立复核 | `verify_com43_item005.py`：**44 checks passed / 0 failed**（`verify_com43.out`） |
| 第二次下载 | 12:08:08 再次 `Verify OK`（运行后 SWD/调试通道未被破坏） |

原始行：

```text
S k=3 p=0 H=0 V=0 o=8 n=44 x=47 a=45 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=6 p=0 H=0 V=0 o=8 n=43 x=45 a=43 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=9 p=0 H=0 V=0 o=8 n=42 x=45 a=43 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
```

实板可证明：

- 当前镜像（axf md5 `e0437fd3…`，两次 `Verify OK`）已下载并运行；
- 3 个真实采样机会各恰一条 `S`，无重复、无拆分；
- `k` 增量 3，与 RTC 1 min×3 一致；上电机会后的节拍未被打乱；
- 失败周期 `S` 后紧跟一条 `G`；字段/键名/顺序、CRLF、行长与契约一致；
- 采样间期无字节输出（UART 已关闭）；
- 光照每周期 8/8 成功（`o=8`），证明 UART 关闭未破坏软 I²C/光照通路；
- 运行结束后再次下载 `Verify OK`，证明 SWD 通道保持可用；
- `V=0/D=0`（未标定）与 `q=0/s=0/y=0`（GXHT40 无 ACK、无上报）语义自洽，无伪造数据。

实板**不能**证明（如实登记）：

- **UID 零路径**：无法在真实芯片上构造 UID 和为 0；仅宿主机真实 `main.c` harness 证明。
- **`S` 在发送之后 / 冻结帧 / 1..3 次发送**：U01 导致每周期 `q=0`，没有触发上报的周期；本项实板只观测到“失败→无发送→`S.s=0`”。发送路径由 ITEM-003 独立验证。
- **上电 `BOOT/VDD/IOTEST/IOSIG/BUS` 顺序**：`mdk_flash` 复位发生在串口捕获开始之前约 5 s，上电行落在窗口外；无额外授权的复位/电源循环入口与捕获重叠。顺序由静态核对与宿主机 harness 证明（`BOOT` 先于 UID 判断）。
- **上电立即机会 `S k=0`**：同样落在捕获 A 开始之前；由 `measure.c` 初值 `sample_flag=1`、宿主机 harness 与 `k=3` 的时间位置共同说明，但本窗口没有其原始字节。
- **180 s 的逐字节时间戳**：串口原始文件无每行时间戳；由 300 s 窗口内的周期数、`k` 增量 3 与 RTC harness 共同证明，不声称毫秒级周期误差。
- **UART 卡死/TXE 有界语义**：属 ITEM-004，已在该项独立验证；本项只复验正常运行行。

---

## 6. 与验收行为的对照

| 验收要求 | 证据 | 结论 |
|---|---|---|
| 每采样机会只执行一次完整事务 | §3.2 计数与顺序；父提交对照 9/9；实板 3 个 `k` 各一条 `S` | 符合 |
| 唯一 `S`（失败追加 `G`） | §3.2 `S`=2 与机会=2；实板 3×(`S`+`G`) | 符合 |
| 不再依赖多轮主循环完成 RF 重试 | §3.2 空闲轮 0 次业务调用；§3.3 旧实现 9 轮 9 发 | 符合 |
| BOOT 移到 UID 零致命判断之前 | §3.1 事件顺序；§4 静态顺序 | 符合 |
| UID 零清两标志、PB05 低/ADC 关、关 UART、真正深睡 | §3.1 事件与标志；§4 `light_init`/close/深睡静态核对 | 符合 |
| UID 零不采样/不发送/不空转 | §3.1 0 次采样/发送/trace、0 次 `go_to_sleep`、连续 5 次深睡 | 符合 |
| RTC 1 min×3 不变 | §4.1 17/0；实板 `k` 增量 3 | 符合 |
| GPIOA/I²C/SWD 边界不变 | §4 无 GPIOA 复位、无 PA07/PA08；实板光照 8/8 与二次 `Verify OK` | 符合 |

---

## 7. 未执行与局限

1. UID 零无法在实板构造；该路径只有宿主机真实 `main.c` 证据。
2. U01（GXHT40 无 ACK）仍存在，所有实板周期 `q=0`；触发上报、冻结帧、`S` 发送后语义与端到端未被本项重新观测。
3. 未捕获上电诊断行与 `S k=0`；原因见 §5。
4. 未使用逻辑分析仪/示波器；本项判定不需要模拟波形，且不把数字旁证写成电气结论。
5. 未修改产品源码、网关、Android、硬件、`runtime/*.yaml`；仅新增测试能力自有 harness/脚本与证据。

---

## 8. 结论

- 当前受测源码（HEAD `51d8fcc`，axf md5 `e0437fd3…`）的主循环在 `sample_flag` 置位时执行 **一次** `temperature_process → send_data_to_gateway → 唯一 S（失败 G）→ UART 关闭 → 深睡`，空闲轮不重复采样/发送；父提交在同一 harness 下按旧语义失败（NEGATIVE_CONTROL_OK）。
- UID 和为零时：BOOT/诊断先执行；清 `sample_flag/report_req`；`light_init()` 置 PB05 低并关闭 ADC；关闭 UART；进入连续深睡循环，不采样、不发送、不空转。
- RTC 1 min×3 回归 17/0；静态/边界 47/0；主流程 26/0；实板 3 个机会 `k=3/6/9`、每机会一条 `S` + 失败一条 `G`、行长合法、间期静默；两次下载均 `Verify OK`。
- **ITEM-005 本增量：TEST_PASS**。范围与限制如 §1/§5/§7。
