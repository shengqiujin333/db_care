# 实现侧自检证据（DRV-003）

负责角色：固件工程
时点：2026-11-10（Asia/Shanghai）
范围：传感器工程实现侧宿主机自检（`test/build_test.sh`，5 个阶段）在「as-built 基线」与「LIGHT-DEF-2 形制落地后」两次运行的结果。
证据类别：**宿主机 mock 自检 + 构建**；不声明实板、电气或跨链路通过。

依据：`artifacts/firmware_design.md`（FD-002 rev 8 §6.2/§12/§13）、`artifacts/firmware_requirements.md`（FWR-002 rev 8）、`artifacts/test_design.md`（REG/MCH 用例）。

---

## 1. 运行方式与环境

```text
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
bash build_test.sh
```

| 项 | 值 |
|---|---|
| 宿主 shell | Git Bash（Windows） |
| 宿主编译器 | `gcc`（MinGW-w64，`C:\ProgramData\chocolatey\bin\gcc.exe`） |
| 产物目录 | `test/_hostbin/`（`.gitignore` 覆盖 `*.exe`，不入库） |
| 原始输出 | `host_build_test_baseline.log`（基线）、`host_build_test_lightdef2.log`（本轮） |

## 2. 两次运行结果

| 阶段 | 内容 | 基线 | LIGHT-DEF-2 后 |
|---|---|---:|---:|
| [1/4] | `fw_core` 纯逻辑（CRC16/CRC-8/换算/无光判据/上报判定） | 46 / 0 | **48 / 0** |
| [2/4] | `measure.c` 采样流程（mock MCU：冻结/重试/失败不推进） | 61 / 0 | 61 / 0 |
| [3/4] | UART1 调试通道（行预算/`P` 行/TXE、TXBUSY 卡死有界） | 54 / 0 | 54 / 0 |
| [4/4a] | 光照通路，交付默认（判据未启用 `T=0`） | 28 / 0 | 28 / 0 |
| [4/4b] | 光照通路，启用变体（`-DLIGHT_DARK_THRESHOLD_CODE=3590u`） | 29 / 0 | **31 / 0** |
| 合计 | | 218 / 0 | **222 / 0** |
| 退出码 | | 0 | 0 |

差异解释：`[1/4]` 由 1/3 向量改为阈值形制向量（新增 `T=0`、`mean=T-1`、`mean=T`、`T=4095` 边界与 `T=1..4095` 全量断言，删除 `3*mean>=C_dark` 全量断言）；`[4/4b]` 由「标定后 1/3」改为「启用后阈值」，并在 `T ∈ (3514, 3987]` 时额外核对设计 §6.2 的两个实测参考值（房间灯 3514 → LIT、近饱和 3987 → DARK）。其余三个阶段断言数与内容未变。

## 3. 本轮新增/替换的断言（逐条可复核）

### 3.1 纯逻辑 `light_is_dark(mean, valid, T)`（`host_fw_core_pure_check.c`、`host_sensor_core_test.c`）

| 输入 | 期望 | 覆盖点 |
|---|---|---|
| `valid=false`, `T=1`, `mean=4095` | false | 无读数依据不得判暗 |
| `valid=true`, `T=0`, `mean=4095` | false | 判据未启用（唯一禁用值）不得判暗 |
| `T=1`: `mean=0` / `mean=1` | false / true | 阈值下界与边界相等为暗 |
| `T=3590`: `mean=3589` / `3590` | false / true | 候选阈值的边界 |
| `T=3590`: `mean=3514` / `3987` | false / true | 设计 §6.2 实测参考值方向 |
| `T=4095`: `mean=4094` / `4095` | false / true | 上界（仅满量程为暗） |
| `T=1..4095` 全量：`mean=T-1` / `T` | false / true | 全参数域无例外（取代旧 1/3 全量断言） |
| 同输入重复调用 | 同输出 | 无滞回/无跨调用状态 |
| 交付常量 `LIGHT_DARK_THRESHOLD_CODE == 0` | true | 交付件判据未启用 |

### 3.2 光照通路接线（`host_light_check.c`，真实 `light.c` + 影子 ADC/GPIO）

| 场景 | 期望 |
|---|---|
| `T=0`，8 次 ADC 均成功且读数满量程 4095 | `adc_ok=true`、`valid=false`、`dark=false`（不得把满量程解释为暗） |
| `T=3590`，8 次均 3590 | `valid=true`、`dark=true`（边界相等） |
| `T=3590`，8 次均 3589 | `dark=false` |
| `T=3590`，8 次均 3514 | `valid=true`、`dark=false`（房间灯参考） |
| `T=3590`，8 次均 3987 | `valid=true`、`dark=true`（近饱和参考） |
| ADC 8 次全部超时（任一 `T`） | `samples_ok=0`、`adc_ok=false`、`mean/min/max=0`、`valid=false`、`dark=false`、PB05 回低、ADC 关闭 |
| 部分超时（4/8 成功） | 均值只取成功样本，`o=4`，不冒充 `o=8` |
| 每笔独立 | 相同输入前后两次结果一致；无历史暗态 |
| VDD 测量后 | `light_sample` 仍 8/8 成功（未破坏 AIN11 通路） |

### 3.3 未启用等价性（对 as-built 的行为等价）

`T=0` 时 `valid = adc_ok && false = false`、`dark = false`，与 as-built（`LIGHT_DARK_CALIBRATED=0`）逐项相同；采样时序、`S` 行长度与字段集合、`G/P` 行条件、433/BLE 布局均未触及（`measure.c`、`debug_trace.c`、`encrytogate.c` 本轮零改动，见 `git diff --stat`）。实板 `S` 行 `V=0 D=0` 的再观测属未执行项（`FW-OBS-PASSIVE`，无下载/采集授权）。

## 4. 与设计验收项对照（ITEM-003 / MCH）

| 设计/TD 项 | 本轮证据 | 结论 |
|---|---|---|
| 形制向量（`T=0/1/4095`、边界、`valid=false`） | §3.1 表全部 PASS | 满足 |
| 接线 `valid=adc_ok&&(T!=0)`、`dark=light_is_dark(mean,valid,T)` | §3.2 表 + 源码 diff | 满足 |
| ADC 全超时不合成暗态 | `[4/4a]`/`[4/4b]` 全超时用例（28/31 项内） | 满足 |
| 未启用等价性 | §3.3 | 宿主等价；实板未复观测（未执行） |
| 旧宏清除（`LIGHT_DARK_REF_CODE`/`LIGHT_DARK_CALIBRATED`） | `USER/` 与实现侧测试 grep 无命中；`debug_trace.h` 内遗留注释已改 | 满足（验证角色资产除外，见 §5） |
| 不引入滞回/温补/自适应 | `light_is_dark` 为纯比较，无状态；`light.c` 无新变量 | 满足 |
| 不改格式/布局/节拍 | `measure.c`/`debug_trace.c`/`encrytogate.c` 零 diff；`build_test.sh` [2/3] 阶段 61+54 项全 PASS | 满足 |
| Keil + gcc/build.sh + 宿主测试均 0 Error | `build_evidence.md` §2–§4；本文件 §2 | 满足 |
| 验证资产零修改 | `git status`：`test/*_verify_ev.c`、`test/mock_*_ev/` 均未改 | 满足 |

## 5. 跨角色同步（本单位已登记，未修改他方资产）

| 资产 | 现状 | 需同步原因 |
|---|---|---|
| `test/host_fw_core_verify_ev.c` | 仍按 `3*mean >= c_dark` 断言（含 `c_dark=0 → 每个 valid 样本 DARK`、1365 边界） | 形制已改为 `mean >= T`，旧断言将失败；需验证角色按 FWR-121 重写向量 |
| `test/host_vdd_verify_ev.c:261` | 引用 `LIGHT_DARK_CALIBRATED` | 宏已删除，**编译失败**；需验证角色改为 `LIGHT_DARK_THRESHOLD_CODE` |
| `test/host_light_verify_ev.c`、`host_rf_report_verify_ev.c` | 在此前已被登记为陈旧不可编译 | 与本轮无关，仍需验证角色重写 |
| `artifacts/interface_contract.md` §4.2/§10 | 仍描述旧判据与“不批准替代阈值” | 判据被接受后由系统架构同步；本轮只提供实现输入 |

## 6. 边界与未证明项

1. **全部证据为宿主机 mock**：ADC/GPIO/UART 由影子层替身，`light.c` 与 `fw_core.c` 为真实源码。mock 不能证明真实 ADC 时序、PB05 供电建立、器件响应或电气兼容。
2. **未证明**：实板 `S` 行在 `T=0` 下确为 `V=0 D=0`（需下载 + COM43）；`T=3590` 启用后的实板分离（需接受记录 + 被动双态证据）；真实升温/>35.0℃ 触发（本阶段禁止调温）；功耗（本阶段禁止测量）。
3. 未启用等价性在本轮只做**源码级 + 宿主级**论证；历史实板 `V=0 D=0` 记录来自旧镜像（`a3b08774…`），不能作为新镜像证据。
