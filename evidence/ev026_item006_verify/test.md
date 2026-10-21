# 嵌入式验证证据（EV-026，ITEM-006 标定配置收口与构建标识）

能力：`embedded_tester.embedded_verification`
受测项：`TL-6c897e7b-38c9-4f2a-9d55-d216ddfaaff2-tasks-ITEM-006`
被测行为：`sensor_config.h` 将 `LIGHT_DARK_CALIBRATED` 限定为 0/1；已标定时 `LIGHT_DARK_REF_CODE` 必须在 1..4095，否则编译期报错；未标定时 `light.valid/dark=false` 且升温分支关闭；保留 100 ms / 8 次 / 整数均值；BOOT 使用本轮构建标识常量 `SENSOR_FW_BUILD_ID`，不得硬编码 `FD-002r4`。
结论：**TEST_PASS（ITEM-006 本增量；独立编译门禁 + 独立宿主机 harness + 实板观测）**
日期：2026-10-21（UTC 04:27–04:35；本地 12:27–12:35）

> 范围声明：本结论只覆盖 ITEM-006 的上述行为。GXHT40 真实 ACK（U01）仍未闭合，实板每个周期 `q=0`，这不属于本项判据，也未用宿主机结果替代。全暗 `C_dark` 现场标定（U02）仍未执行；交付配置保持未标定。
> 标定后的判据（`mean=ceil(C/3)` 为暗、减一为亮）由**同一份当前产品源码**以 `-D` 编译成三个标定变体，在本能力独立 harness 中验证；未烧录标定镜像，也不宣称实板已标定。
> BOOT 横幅文字由**编译当前 `debug_trace.c`** 的独立 harness 逐字节证明，并用 ITEM-006 之前的源码做负向对照；实板串口窗口未包含上电横幅（原因与限制见 §6.3）。

---

## 1. 受测件身份

| 项 | 值 | 来源 |
|---|---|---|
| Git HEAD | `8971e631872d877375312f8d7a631fea4eb53f48`（ITEM-006 实现提交；本轮开始时工作区干净） | `git_head.txt` |
| 被测源码 | `sensor_config.h` `b328f79a…`；`debug_trace.c` `4430bf7a…`；`light.c` `e45bd25f…`；`fw_core.c` `5d23673a…`；`measure.c` `0fe63832…`；`main.c` `9024055c…`（SHA-256 全文见 `source_sha256.txt`） | `sha256sum` |
| 构建 | `mdk_build {"action":"rebuild"}`：0 Error / 1 既有 warning（`main.c(252) while(k--);`），Code=19544 RO=644 RW=116 ZI=1684 | `build_rebuild.log` |
| 镜像 | `Project.axf` md5 `9cb97918e8832811dfcb00a93646fba1`；`Project.hex` md5 `3af11e7adb21ba43004fc827649adb44` | `image_md5.txt` |
| 下载 | `mdk_flash {}` 12:28:37 本地：`Erase Done.Programming Done.Verify OK.Application running ...` | `mdk_flash.log` |
| 目标板 | 传感器板 CW32L010F8（GXHT40 + 光敏分压 + UM2005C），调试串口 COM43 | `serial_ports.txt` |
| 调试串口 | COM43，Prolific `067B:2303`，9600 8N1；COM26 为调试器 CDC，不是传感器 UART1 | `serial_ports.txt` |
| 宿主机编译器 | MinGW-W64 gcc 12.2.0（`-std=c11 -Wall -Wextra`） | 各 `compile_*.err` |
| Keil 编译器 | ARMCLANG V6.24 | `build_rebuild.log` |

同一工作树独立 rebuild 的 axf/hex md5 与实现证据 `evidence/ev025_item006/build.md` 逐字符一致 ⇒ 所下镜像可由当前受测源码确定性重建。

---

## 2. 验证方法（独立于实现侧自检）

| 手段 | 文件 | 说明 |
|---|---|---|
| 编译期门禁 | `check_calib_gate_item006.py` → `calib_gate.out` | 直接用 `gcc -fsyntax-only` 编译**真实** `sensor_config.h`（`SENSOR_CONFIG_NO_MCU`），11 组合法/非法组合；不调用实现的 `check_calib_config.sh` |
| 标定/未标定语义 | `test/host_item006_calib_verify.c` → `calib_*.out` | 直接编译**真实** `light.c` + `fw_core.c`；本能力自有影子 MCU 层 `test/mock_vdd_ev`；同一源码 4 个配置变体 |
| BOOT 构建标识 | `test/host_diag_line_verify_ev.c`（已同步） + 负向对照 | 直接编译**真实** `debug_trace.c`，捕获 UART1 字节；另用 `8f9f86c` 的旧 `debug_trace.c` 跑同一 harness |
| 静态核对 | `static_check_item006.py` → `static_check.out` | 门禁形态、单一构建标识定义、`FD-002r4` 清零、100 ms/8 次/整数均值/`valid`/`dark` 规则、产品门控接线 |
| 实板观测 | `com43_capture_a.bin` / `com43_capture_b.bin` + `verify_com43_item006.py` | 下载后 COM43 原始字节（90 s + 200 s），独立解析 |
| 回归参考 | `host_tests_regression.log` | 实现侧既有套件 5 组 233/0，仅参考 |

复现命令（在 `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/`）：

```sh
EV=D:/mypro/beiwo2/AIP/evidence/ev026_item006_verify

# 1) compile-time gate (11 legal/illegal combinations)
python $EV/check_calib_gate_item006.py

# 2) real light.c + fw_core.c, four configuration variants
gcc -std=c11 -Wall -Wextra -DLIGHT_BGR_TRIM_MV=mock_bgr_trim_value \
    -Itest/mock_vdd_ev -IUSER/inc -ICOMMON \
    test/host_item006_calib_verify.c USER/src/light.c USER/src/fw_core.c \
    -o $EV/calib_delivery.exe && $EV/calib_delivery.exe
gcc -std=c11 -Wall -Wextra -DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=1 \
    -DLIGHT_BGR_TRIM_MV=mock_bgr_trim_value -Itest/mock_vdd_ev -IUSER/inc -ICOMMON \
    test/host_item006_calib_verify.c USER/src/light.c USER/src/fw_core.c \
    -o $EV/calib_cal1.exe && $EV/calib_cal1.exe
#   ... same with REF_CODE=3000 and REF_CODE=4095

# 3) BOOT banner from the real debug_trace.c (and the pre-ITEM-006 negative control)
gcc -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Itest/mock_diagline_ev -IUSER/inc \
    test/host_diag_line_verify_ev.c USER/src/debug_trace.c -o $EV/diag_line_verify.exe
$EV/diag_line_verify.exe
gcc -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Itest/mock_diagline_ev -IUSER/inc \
    test/host_diag_line_verify_ev.c $EV/old_debug_trace_pre_item006.c \
    -o $EV/diag_line_verify_old.exe && $EV/diag_line_verify_old.exe

# 4) static + real-board captures
python $EV/static_check_item006.py
python $EV/verify_com43_item006.py --calibrated 0 --ref 0 --build-id RUN10-FDS003 \
    $EV/com43_capture_a.bin $EV/com43_capture_b.bin
```

---

## 3. 编译期标定门禁（独立）

`calib_gate.out`：**11 passed / 0 failed**。编译真实头文件，检查退出码与 `#error` 文本：

| 配置 | 期望 | 实测 |
|---|---|---|
| 交付默认（CAL=0, REF=0） | 通过 | 通过，exit 0 |
| CAL=0, REF=4096 | 通过（范围门禁只在已标定时生效） | 通过，exit 0 |
| CAL=1, REF=1 | 通过 | 通过，exit 0 |
| CAL=1, REF=3000 | 通过 | 通过，exit 0 |
| CAL=1, REF=4095 | 通过 | 通过，exit 0 |
| CAL=2 | 拒绝 | **编译失败**，`#error "LIGHT_DARK_CALIBRATED must be 0 or 1"` |
| CAL=-1 | 拒绝 | **编译失败**，同一 `#error` |
| CAL=1, REF=0 | 拒绝 | **编译失败**，`#error "LIGHT_DARK_REF_CODE must be 1..4095 when LIGHT_DARK_CALIBRATED=1"` |
| CAL=1, REF=4096 | 拒绝 | 同上（拒绝） |
| CAL=1, REF=-1 | 拒绝 | 同上（拒绝） |

原始错误文本均指向 `USER/inc/sensor_config.h:121`（CALIBRATED 门禁）或 `:125`（REF 范围门禁），证明失败原因是标定门禁本身，而不是无关的语法/包含错误。非法组合不会生成固件。

---

## 4. 未标定与 1/3 边界（独立宿主机 harness，真实产品源码）

四个变体均由同一份当前 `light.c`/`fw_core.c` 编译，仅标定宏不同：

| 变体 | 结果 | 证据 |
|---|---:|---|
| 交付配置 CAL=0 REF=0 | **48 passed / 0 failed** | `calib_delivery.out` |
| CAL=1 REF=1 | **43 passed / 0 failed** | `calib_cal1.out` |
| CAL=1 REF=3000 | **43 passed / 0 failed** | `calib_cal3000.out` |
| CAL=1 REF=4095 | **43 passed / 0 failed** | `calib_cal4095.out` |

每个变体的 `light_is_dark` 全域检查另含 **8190 条断言**（C=1..4095，逐个检查 `ceil(C/3)` 为暗、`ceil(C/3)-1` 为亮），四个变体均 0 不匹配。

### 4.1 采样算术保持（全部变体）

- `PB05` 高 → **恰好一次 100 ms** 稳定等待 → **恰好 8 次**转换 → 整数均值 → `PB05` 低；PB05 只在采样窗口内为高，无其它引脚写。
- `{1..8}` 的 8 次采样得到均值 **4**（36/8 截断），min=1、max=8。
- ADC 在 burst 前后各使能/关闭一次，采样后保持关闭。
- 失败路径仍拉低 PB05、关闭 ADC，EOC 轮询受 `LIGHT_ADC_EOC_GUARD` 有界限制。

### 4.2 未标定（交付配置）

| 观测 | 实测 |
|---|---|
| ADC 8/8 成功、均值 4095 | `adc_ok=true`、`samples_ok=8`、`mean=4095`，但 **`valid=false`、`dark=false`** |
| 全部转换超时 | `adc_ok=false`、`samples_ok=0`、`mean/min/max=0`、`valid=false`、`dark=false`（**不合成 4095**） |
| 升温分支 | `sensor_gate_decide(true, 200, true, 210, light.valid=0, light.dark=0) = false`；`>35.0 ℃` 分支仍独立为 true |

### 4.3 已标定边界（编译变体）

| 变体 | `ceil(C/3)` | mean=ceil → 暗 | mean=ceil−1 → 亮 |
|---|---:|---|---|
| REF=1 | 1 | **PASS** | mean=0 → **PASS** |
| REF=3000 | 1000 | **PASS** | mean=999 → **PASS** |
| REF=4095 | 1365 | **PASS** | mean=1364 → **PASS** |

标定变体下 `valid=true`，且 `dark` 与独立参考模型 `valid && 3*mean >= C` 逐点一致；升温分支在“已标定且判暗”时触发，恰好 `+0.9 ℃`（差值 9）不触发。

---

## 5. BOOT 使用构建标识（独立 harness，真实 `debug_trace.c`）

`diag_line_verify.out`：**36 passed / 0 failed**。首个捕获行逐字节为：

```text
BOOT fw=RUN10-FDS003 uid=6A002C00 rst=0240 uart=9600\r\n
```

- 期望值由 `"BOOT fw=" SENSOR_FW_BUILD_ID ...` 构造，配置常量在 harness 中同时断言为 `RUN10-FDS003`；
- 字段数量/顺序/CRLF/≤96 B 均保持，其余 `BUS/G/S` 行格式检查全部通过。

**负向对照**：把同一 harness 链接 ITEM-006 之前的 `debug_trace.c`（`git show 8f9f86c:...`，存为 `old_debug_trace_pre_item006.c`）：

```text
[boot] BOOT fw=FD-002r4 uid=6A002C00 rst=0240 uart=9600
    FAIL  boot banner uses SENSOR_FW_BUILD_ID (ITEM-006 build identifier)
==== result: 35 passed, 1 failed ====
```

⇒ 该检查确实区分“硬编码旧标识”和“使用配置常量”，不是恒真断言。`static_check.out`（23/0）另证明：`SENSOR_FW_BUILD_ID` 在整个 `USER/` 只有一处定义；所有 `USER/` 源文件中 `FD-002r4` 出现 **0 次**。

---

## 6. 实板观测（当前镜像已实际下载）

### 6.1 下载与运行

| 项 | 事实 |
|---|---|
| 构建 | `mdk_build rebuild` 0 Error / 1 既有 warning，Code=19544 |
| 镜像 md5 | axf `9cb97918…` / hex `3af11e7a…`（与实现记录一致） |
| 下载 | `Verify OK. Application running ...`（04:28:37 UTC） |
| 捕获 A | COM43 9600 8N1，90 s（04:28:38–04:30:08），145 B |
| 捕获 B | COM43 9600 8N1，200 s（04:31:26–04:34:46），224 B |

### 6.2 原始行

```text
BUS idle=3 scl=1 sda=1 ack=none
S k=0 p=0 H=0 V=0 o=8 n=40 x=43 a=41 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=3 p=0 H=0 V=0 o=8 n=40 x=42 a=40 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=6 p=0 H=0 V=0 o=8 n=39 x=43 a=40 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
```

`verify_com43.out`：**58 passed / 0 failed**。实板可证明：

- 下载后的镜像确实重新启动并运行：捕获 A 的首行是复位后的 `BUS`，随后是**上电立即采样机会 `S k=0`**；`k=0 → 3 → 6` 三个连续机会，间隔 3 个 tick。
- **未标定语义在真实目标上成立**：三个周期均 `V=0`、`D=0`；即使光照均值 `a=41/40/40`、`o=8`（8 次转换全部成功），也**不判暗**。
- 失败周期不伪造数据：`q=0 → r=0、s=0、t=0、h=0`，每周期 `S` 后恰一条 `G`；UART 行均 ≤96 B、CRLF 结尾、字段顺序固定。
- `o=8` 且 `n≤a≤x` 证明光照采样在下载后的运行中持续工作（100 ms/8 次路径未被破坏）。

### 6.3 实板未证明的事项（如实登记）

| 项 | 原因 |
|---|---|
| 上电 `BOOT fw=RUN10-FDS003` 行 | MCP 工具调用串行：`mdk_flash` 返回时复位已发生，`serial_capture` 在其后约 40 ms 才打开端口，而 BOOT/VDD/IOTEST/IOSIG 在约 0.2 s 内已发完（窗口 A 首行即 `BUS`）。没有与复位重叠的独立复位/电源循环入口。横幅文字由 §5 的**当前源码编译** harness + 负向对照证明；板上镜像身份由 md5 可重建 + `Verify OK` + 复位后 `S k=0` 共同限定。 |
| 已标定镜像的实板边界 | 未做全暗现场标定（U02），交付配置必须保持 `CALIBRATED=0`；标定变体只做编译期宿主验证，**未烧录**。 |
| `C_dark` 实际数值 / 亮暗可分离性 | 需要完全遮光现场窗口；本项不负责取值，也不得用模拟值回填。 |
| 触发上报 / 端到端 | GXHT40 仍无 ACK（U01），实板 `q=0`，无触发周期；属其它项范围。 |

---

## 7. 静态与回归

| 检查 | 结果 | 证据 |
|---|---:|---|
| 独立静态核对（门禁形态/单一标识/无旧硬编码/100 ms/8 次/整数均值/valid/dark/产品接线） | **23 passed / 0 failed** | `static_check.out` |
| 本能力独立标定 harness（4 变体） | 48 + 43 + 43 + 43，**0 failed**；全域 8190×4 断言 0 不匹配 | `calib_*.out` |
| BOOT 行逐字节 + 负向对照 | 36/0；旧源码 35/1（按预期失败） | `diag_line_verify*.out` |
| 实板原始字节独立解析 | **58 passed / 0 failed** | `verify_com43.out` |
| 实现侧既有宿主机套件（参考） | 5 组 **233 passed / 0 failed**（53/72/51/28/29） | `host_tests_regression.log` |

测试能力本轮修改/新增的测试资产（非产品源码）：

- 修改 `test/host_diag_line_verify_ev.c`：BOOT 期望由硬编码 `FD-002r4` 改为 `SENSOR_FW_BUILD_ID`（实现交接中登记的过期测试资产）。
- 新增 `test/host_item006_calib_verify.c`（独立标定/门控 harness）。
- 新增 `evidence/ev026_item006_verify/` 下的脚本与原始证据。
- **未修改** `USER/**` 产品源码、Keil 工程、网关、Android、硬件或 `runtime/*.yaml`。

---

## 8. 验收行为对照

| 验收要求 | 证据 | 结论 |
|---|---|---|
| `LIGHT_DARK_CALIBRATED` 只允许 0/1 | §3：CAL=2/-1 编译失败且错误文本明确 | 符合 |
| 已标定时 `LIGHT_DARK_REF_CODE` 必须在 1..4095 | §3：REF=0/4096/-1 编译失败；1/3000/4095 通过 | 符合 |
| 未标定时 `light.valid/dark=false` | §4.2 宿主：8/8 成功且 mean=4095 仍 false/false；§6.2 实板三个周期 `V=0 D=0` | 符合 |
| 未标定时升温分支关闭 | §4.2 gate 返回 false；`>35.0 ℃` 分支独立 | 符合 |
| 保留 100 ms / 8 次 / 整数均值 | §4.1 实测一次 100 ms、8 次转换、36/8=4；静态检查通过 | 符合 |
| BOOT 使用构建标识常量，不硬编码 `FD-002r4` | §5 逐字节 `fw=RUN10-FDS003` + 旧源码负向对照；静态 `FD-002r4` 0 处 | 符合（横幅文字为宿主机编译证据，实板未入窗，见 §6.3） |
| `mean=ceil(C/3)` 判暗、其减一判亮 | §4.3 三个 C 值边界 + 全域 C=1..4095（8190 断言/变体） | 符合 |

---

## 9. 未执行与局限

1. 实板未捕获上电横幅（工具串行导致窗口错过），已在 §6.3 说明；未用旧日志代替，也未把宿主机结果写成实板结果。
2. 标定变体仅编译并在宿主机运行，未烧录；交付件保持未标定是当前需求约束，不是测试降级。
3. GXHT40 无 ACK（U01）持续存在，所有实板周期 `q=0`；本项不据此判定器件损坏，也不宣称采集/上报通过。
4. 未使用逻辑分析仪/示波器：本项判据为编译期与数字诊断行语义，不需要模拟波形；不把数字观测写成电气结论。
5. 未修改产品源码；环境与授权未变更。

---

## 10. 结论

- 非法标定组合在**编译期**被真实头文件拒绝，合法组合编译通过；错误文本可区分两类门禁。
- 未标定时真实 `light.c` 在 8/8 ADC 成功、均值为 4095 时仍 `valid=false/dark=false`，全超时不合成 4095；升温分支由产品 `sensor_gate_decide` 正确关闭。
- 已标定时 `mean=ceil(C/3)` 判暗、`ceil(C/3)-1` 判亮，对 C=1/3000/4095 及全 1..4095 域成立；100 ms / 8 次 / 整数均值保持不变。
- BOOT 由 `SENSOR_FW_BUILD_ID`（`RUN10-FDS003`）渲染，源码中无 `FD-002r4` 残留；负向对照证明检查有效。
- 当前镜像已下载（axf md5 `9cb97918…`，`Verify OK`），实板三个连续采样机会 `k=0/3/6` 均为 `V=0/D=0`、失败不伪造数据、行长/字段/节拍合法。
- **ITEM-006 本增量：TEST_PASS**。范围与限制如 §1/§6.3/§9。
