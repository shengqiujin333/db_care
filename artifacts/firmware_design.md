# 传感器/网关固件设计方案（FD-002）

状态：固件方案设计（firmware_engineer.firmware_solution_design）
版本：3.0（替代 rev 2.0；按 IC-002 v3.0 冻结上报方向与边界、光敏 1/3 全暗判据与光照有效性、待上报冻结样本；并登记对既有源码的 D-01/D-02 偏差）
范围：本次「GXHT40 替换温湿度传感器、新增光敏分压、采集周期 3 分钟、条件上报」在固件域内的方案。**主要改动在传感器固件（CW32L010Y8M6）**；**网关固件（CH592 beiwov2）经核对无需功能改动**。本方案明确模块/任务划分、数据流、时序、资源预算、接口处理、异常策略与实现约束，**不承担代码实现**（由 `firmware_engineer.firmware_implementation` 承接）。
上游输入：
- 需求：`readme.txt`（权威）
- 接口契约：IC-002 `artifacts/interface_contract.md` v3.0（CONTRACT_READY，冻结规范化语义）
- 硬件事实：`sensor_hardware/pstxnet.dat`、`sensor_hardware/MAIN_BOARD.BOM`、`gateway_board.xml`
- 器件手册：`gxht40.pdf`（GXHT4x Datasheet V2.6）
- 既有固件源码：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/`、`CH592EVT/EVT/EXAM/BLE/beiwov2/`
- 伴随产物：FWR-002 v3.0 `artifacts/firmware_requirements.md`、RTA-002 v3.0 `artifacts/firmware_rtos_architecture.md`

---

## 1. 现状基线与本次变更范围

### 1.1 传感器现状（as-built 源码核定）

前一轮 workflow 已把 AHT21B 替换为 GXHT40 并新增光照通路，旧 hall/OPTCFG/params/history 已退役；**但以下三处与 IC-002 v3.0 不一致**，本设计明确要求修正（D-01/D-02）：

| 偏差 | 位置（当前源码） | IC-002 v3.0 要求 |
|---|---|---|
| 上报方向为「降温」且整体排除 35.0℃ | `USER/src/fw_core.c:105–121`：`(int32)prev-cur > 9`，并对 `cur == 350` 直接 `return false` | 方向为**升温** `(int32)cur-prev > 9`；恰好 35.0℃ 不满足超温分支但**可经升温分支上报**；判定还需 `light_valid` |
| 光照采用 350/250 滞回、且 ADC 全超时合成满量程暗态 | `USER/src/light.c:93–120`、`USER/src/fw_core.c:94–99`、`USER/inc/sensor_config.h:100–103` | `dark = valid AND 3*mean >= C_dark`（C_dark=全暗基准）；**每笔独立、无滞回**；ADC 全超时 → `valid=false`，**不得**合成满量程暗态 |
| 待上报帧读取「前值」而非冻结快照 | `USER/src/measure.c:238–262`（`encode_frame10(..., s_prev_temp_x10, s_last_hum_x10, ...)`） | 置待上报时冻结当次 `{temp,hum}` 快照，重试期间帧内容不得改变（FWR-114） |

其余（GXHT40 驱动、3 分钟节拍、433 帧布局、网关链路）已与 IC-002 一致，本设计保持并要求回归。

### 1.2 本次硬件事实（网表/BOM 核定，不可改）

传感器板（`sensor_hardware/pstxnet.dat` + `MAIN_BOARD.BOM`，OrCAD 2026-10-02）：

| 网络 | 器件/引脚 | 说明 |
|---|---|---|
| N05845 | U9.GXHT40.SDA — U7.PA04；R1 4.7 K 上拉至 BAT | 软 I²C 数据 |
| N06024 | U9.GXHT40.SCL — U7.PA03；R2 4.7 K 上拉至 BAT | 软 I²C 时钟 |
| LIGHT_ADC | J4.1（光敏 2 脚器件，封装 GL4539_2）— R3.1 — U7.**PB04** | ADC 输入（CW32L010 AIN11） |
| LIGTHT_POWER | R3.2 — U7.**PB05** | 分压供电（GPIO 输出） |
| NC | U7.PB06、PA00、PA01、PA02、PB00、PB01 | 未连接 |
| RF_CLK / RF_DATA | U7.PB02 / U7.PB03 — U4.UM2005C | 433 发射 |
| BAT | U4.UM2005C.VDD — U7.VDD — U9.VCC — CR2032（J1） | 常供电，无 MCU 控制 |

关键器件参数：**R3 = 5 MΩ**；J4 为 2 脚光敏电阻（BOM 无其它光敏器件，无霍尔器件）。

**推导（本方案的设计输入）**：
1. 旧固件的 PB04=霍尔 EXTI、PB05=光数字输入、PB06=F2 供电在新板上不再成立 → 霍尔检测、光学配置窗口（OPTCFG）、基于 Flash 的参数下发在硬件上已不存在（已退役）。
2. 分压拓扑：`PB05(VDD) → R3(5 MΩ) → LIGHT_ADC(PB04) → 光敏电阻 → GND`。
3. ADC 满量程参考 = VDD（`Examples/ADC/adc_sgl_sw_vdd`：`MCU_VDD = 4.095 × BGR1.2V_mV / code`，12 bit）。PB05 输出高即 VDD，故 `code = 4095 × R_photo / (R_photo + 5 MΩ)` **与电池电压无关（比率式）**，对 CR2032 电压衰减鲁棒。
4. 光敏电阻受光阻值下降 → 节点电压下降 → **无光（暗）时读数大**。

### 1.3 本次固件变更范围

| 子系统 | 变更 |
|---|---|
| 传感器温湿度驱动 | AHT21B → GXHT40（协议、地址、CRC、换算全换）——已完成，保持 |
| 传感器光照 | PB05 输出 + PB04/AIN11 ADC + **1/3 全暗基准判据 + 有效性**（替代旧 350/250 滞回） |
| 传感器节拍 | 1 分钟采样 → **3 分钟采样**——已完成，保持 |
| 传感器判定 | 温/湿阈值 + 历史下降 + 小时/首样本 → **升温 >0.9℃ 且无光，或 >35.0℃**（方向更正） |
| 传感器上报 | 触发样本**冻结快照**后发送，成功才清除 |
| 传感器退役 | hall / OPTCFG / params / history 通路移除——已完成，保持 |
| 网关 | 无功能变更（§9 核对） |
| 空口/BLE/云端字段 | 不变（§8） |
| Android/iOS/硬件 | 不在本能力 |

---

## 2. 硬件接口核定

### 2.1 引脚分配（传感器，新板）

| 引脚 | 功能 | 方向 | 配置 |
|---|---|---|---|
| PA03 | GXHT40 SCL | 开漏输出 / 输入 | 软 I²C（现有 `sf_i2c`）；空闲保持输入（外部 4.7 K 上拉） |
| PA04 | GXHT40 SDA | 开漏输出 / 输入 | 同上 |
| PB04 | LIGHT_ADC（AIN11） | 模拟输入 | `GPIO_MODE_ANALOG`；ADC 通道 `ADC_InputCH11` |
| PB05 | LIGTHT_POWER | 推挽输出 | 采样时高，其余低 |
| PB02/PB03 | 433 CLK/DATA | 复用 | 现有 `um2005C_hal`，不改 |
| PA05/PA06 | 调试 UART1 | 复用 | 用后 `DebugUART_Close()` |
| PA07/PA08 | SWD | 复用 | — |
| PB07 | NRST | — | — |
| PA00/PA01/PA02/PB00/PB01/PB06 | 未连接 | — | 不配置、不外驱动 |

> 时钟：MCU 走内部 HSI（`SYSCTRL_HSIOSC_DIV6` → 8 MHz）；32 MHz 晶振 U6 接 UM2005C，不接 MCU。

### 2.2 GXHT40 电气与协议（手册核定）

| 项 | 值 |
|---|---|
| 供电 | 1.6–5.5 V（直接接 BAT，无需稳压） |
| 平均功耗 | 0.4 µA（1 次/秒转换）；待机 0.1 µA |
| I²C | 标准/快速/快速增强；**不支持 clock stretching**；数据未就绪时对读请求返回 **NACK** |
| 地址（7 bit） | GXHT40-AD = **0x44**；GXHT40-BD / GXHT40C-AD = **0x45**（BOM 仅写 `gxht40`，变体未定 → 运行期探测） |
| 8 bit 地址字节 | 0x44 → 写 `0x88` / 读 `0x89`；0x45 → 写 `0x8A` / 读 `0x8B` |
| 测量命令 | `0xFD` 高重复率、`0xF6` 中重复率、`0xE0` 低重复率 |
| 返回 | 6 B：`T_MSB,T_LSB,T_CRC, RH_MSB,RH_LSB,RH_CRC`（手册 §7.2「温度数据在前」） |
| 测量时间 | 低 1.3/1.6 ms，中 3.7/4.5 ms，**高 6.9/8.3 ms**（typ/max，max 在 VDD=1.6 V 测得） |
| 软复位 | `0x94`，tSR ≤ 1 ms |
| CRC | CRC-8，poly `0x31`，init `0xFF`，不反转，xorout `0x00`；`CRC(0xBEEF)=0x92`（已复算确认） |
| 换算 | `T[℃] = -45 + 175·S_T/2^16`；`RH[%] = -6 + 125·S_RH/2^16`（RH 可越界，需截断 0–100） |
| 上电时间 | tPU ≤ 1 ms（MCU 启动远晚于此，无需特殊等待） |

### 2.3 光敏分压

- 拓扑：`PB05 高(=VDD)` → R3 5 MΩ → PB04(AIN11) → R_photo → GND。
- 读数（12 bit，VDD 参考）：`code = 4095 · R_photo/(R_photo + 5 MΩ)`。
  - 受光（R_photo 小，如 10 kΩ）→ code ≈ 8；暗（R_photo 大，如 1 MΩ）→ code ≈ 683；开路 → 4095。
- 建立时间：源阻抗最高 5 MΩ，ADC 采样电容 + 引线电容约数 pF → RC 约数十 µs；光敏器件本身响应为数十 ms 量级。**设计采取 PB05 上电后 ~100 ms 稳定等待 + ADC 390 clk（1 MHz 时钟下 390 µs）采样保持 + 多次取样求均值**。
- 极性：**无光（DARK） ⟺ code ≥ 阈值**，阈值 = 完全遮光基准码 `C_dark` 的 1/3（readme 修改点 7；IC-002 §3）。

---

## 3. 模块划分（传感器工程文件级计划）

### 3.1 新增（已完成，保持）

| 文件 | 职责 |
|---|---|
| `USER/inc/sensor_config.h` | 单一配置头：引脚宏、GXHT40 命令/地址/重试上限/等待时间、**全暗基准 `C_dark` 与标定标志**、采样节拍计数（3）、上报常量（0.9℃、35.0℃）、光照采样次数 |
| `USER/inc/gxht40.h` / `USER/src/gxht40.c` | GXHT40 协议驱动：地址探测、发送命令、等待、读 6 B、CRC 校验、整数换算；对上层只暴露「一次测量 → temp_x10/hum_x10/结果码」 |
| `USER/inc/light.h` / `USER/src/light.c` | 光照通路：PB05 供电、稳定等待、PB04/AIN11 多次采样、均值、**结果 `{valid,mean_adc_code,dark}`** |

### 3.2 修改（本设计确定的更正点）

| 文件 | 动作 |
|---|---|
| `USER/inc/light.h` / `USER/src/light.c` | 采样返回结构化结果 `light_result_t {valid, mean_adc_code, samples_ok, dark}`（替代 `bool light_sample()`）；**ADC 全部超时 → valid=false**，删除满量程回退；删除滞回状态与 `light_reset_state` 的滞回语义 |
| `USER/inc/fw_core.h` / `USER/src/fw_core.c` | 纯逻辑改为 `light_is_dark(mean, valid, c_dark)`（uint32 乘法）与 `sensor_decide_report(prev, have_prev, cur, light_valid, light_dark)`（升温方向、无 35.0℃ 特例排除）；删除 `light_code_is_dark`、`SENSOR_REPORT_DROP_X10` |
| `USER/inc/measure.h` / `USER/src/measure.c` | 接入新判定签名；温湿度成功后**先推进前值**（含光照无效/未上报），触发时**冻结快照**；发送路径改为读取冻结快照 |
| `USER/inc/sensor_config.h` | 删除 `LIGHT_DARK_ENTER/EXIT/HYSTERESIS_STEP`；新增 `LIGHT_DARK_REF_CODE`（C_dark）与 `LIGHT_DARK_CALIBRATED`；上报常量更名 `SENSOR_REPORT_RISE_X10` |
| `test/*` | 旧 350/250 滞回用例、旧降温方向用例须同步重写（由固件实现/嵌入式测试负责，本能力只登记交接） |
| `USER/src/main.c`、`USER/src/interrupts_cw32l010.c` | 保持已退役状态（无 hall/OPTCFG 分支）；RTC 3 拍逻辑不变 |

### 3.3 已退役（保持移除）

`USER/{src,inc}/{hall,optcfg,params,history}.{c,h}`。理由：对应硬件（霍尔、F2 光配置接收、可配置阈值下发）在新板上不存在（§1.2），保留会使 PB04/PB05 被错误重配置并使光照采样失效。

### 3.4 不动

`encrytogate.c`（Feistel/CRC16）、`UM2005C/*`（433 驱动）、`COMMON/*`、标准外设库、MDK/EWARM 工程文件骨架、网关固件。

---

## 4. 数据流

```text
[RTC 1min ×3]
      │
      ▼
light_sample(&light):  PB05=1 → delay ≈100ms → ADC(AIN11) ×8 → 均值/成功数 → PB05=0
                       adc_ok = (samples_ok > 0)
                       valid  = adc_ok AND LIGHT_DARK_CALIBRATED
                       dark   = valid AND (uint32)3*mean >= C_dark
      │  {valid, mean_adc_code, samples_ok, dark}
      ▼
gxht40_measure(&t,&h): probe 0x44/0x45 → write 0xFD → wait ≥ tMEAS.H.max → read 6B
                       → CRC(T), CRC(RH) → temp_x10, hum_x10        ── ok
      │
      ├─ 失败(重试用尽) → 不上报 / 不推进前值 / 不冻结 → 本周期结束
      │
      ▼ 成功
sensor_decide_report(s_prev_temp_x10, s_have_prev, t, light.valid, light.dark)
      │  report: bool
      ├─ true  → report_req=1; report_temp_x10=t; report_hum_x10=h   (冻结快照)
      │
      ▼
s_prev_temp_x10 = t; s_have_prev = true; s_last_hum_x10 = h           (成功即推进)
      │
      ▼
send_data_to_gateway()   [仅 report_req==1]
      encode_frame10(mcu_uid, report_temp_x10, report_hum_x10, frame10)
      → app_um2005C_send_data_timeout(frame10, 10, 200)
      成功 → report_req=0, retry=0
      失败 → retry++; 达到 SENSOR_RF_TX_RETRY → report_req=0（放弃本轮，不伪造成功）
```

RAM 状态（深睡保留）：
```
prev_temp_x10 (int16)   前一有效测量温度; 0.1℃
have_prev     (bool)    是否已有前一有效样本
last_hum_x10  (uint16)  最近有效湿度(仅用于兼容显示/组帧参考)
report_req    (uint8)   有待上报的冻结样本
report_temp_x10 (int16) 冻结样本温度
report_hum_x10  (uint16)冻结样本湿度
sample_flag / rtc_tick_cnt / report_retry
（不再有 light_dark_state：光照为每笔独立结果，无跨周期滞回状态）
```

---

## 5. 时序

### 5.1 采样周期（3 分钟）

```text
t=0        RTC 1min 中断 (rtc_tick_cnt=1) → 立即回深睡
t=60s      RTC 中断 (2) → 深睡
t=120s     RTC 中断 (3) → sample_flag=1, 主循环继续
t=120s     PB05=1, 等待 100ms
t=120.1s   ADC ×8 (≈3.2ms), PB05=0 → 光照结果
t=120.1s   GXHT40: 0xFD, 等待 10ms
t=120.11s  读 6B (≈1ms) → 判定 → 冻结
t=120.11s  需要则 433 发射 ≈27ms (有界 200ms, 最多 3 次)
t≈120.1s   深睡
t=300s     下一轮
```

### 5.2 3 分钟节拍实现

`RTC_Configuration()` 保持 `RTC_INTERVAL_EVERY_1M`；`RTC_IRQHandlerCallBack()` 中 `rtc_tick_cnt++`，达到 `SENSOR_SAMPLE_TICKS`（=3）时置 `sample_flag` 并清零。**注意**：RTC 时钟源为 LSI，分钟节拍存在 LSI 容差（约 ±5% 量级），3 分钟为近似值；readme 未给出精度要求。

### 5.3 启动采样机会

`sample_flag` 上电初值为 1（与 IC-002 §2 生命周期决策一致）：上电后立即获得一次采样机会，无需等待 3 分钟；此后按 RTC 3 拍节拍。首笔样本 `have_prev=false`，仅超温分支可上报。

---

## 6. 关键算法（实现必须逐位一致）

### 6.1 GXHT40 CRC-8

```c
uint8_t fw_crc8_gxht(const uint8_t *p, uint16_t n) {   /* poly 0x31, init 0xFF, 无反转, 无 xorout */
    uint8_t crc = 0xFF;
    while (n--) { crc ^= *p++; for (uint8_t i = 0; i < 8; i++)
        crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1); }
    return crc;
}
/* 参考向量: fw_crc8_gxht({0xBE,0xEF},2) == 0x92 */
```

### 6.2 原始值 → ×10 整数（避免浮点）

```c
/* T = -45 + 175*S/65536  → x10 = -450 + round(1750*S/65536) */
temp_x10 = (int16_t)(-450 + (int32_t)((1750uL * s_t + 32768uL) >> 16));      /* 范围 -400..1250 */
/* RH = -6 + 125*S/65536  → x10 = -60 + round(1250*S/65536), 再截断 0..1000 */
int32_t rh = -60 + (int32_t)((1250uL * s_rh + 32768uL) >> 16);
hum_x10 = (uint16_t)(rh < 0 ? 0 : (rh > 1000 ? 1000 : rh));
```

### 6.3 光照结果与无光判据（本版冻结，取代旧滞回）

```c
/* 由 light.c 采样得到: mean = sum(成功转换) / samples_ok (整数除法), samples_ok ∈ 0..LIGHT_ADC_SAMPLES */
/* 纯逻辑判定（无 MCU 依赖，宿主机可测）: */
bool light_is_dark(uint16_t mean_adc_code, bool valid, uint16_t c_dark)
{
    if (!valid) return false;                              /* 无效不能证明无光 */
    return ((uint32_t)3u * (uint32_t)mean_adc_code) >= (uint32_t)c_dark;  /* 等价 mean >= ceil(c_dark/3) */
}
```

规则（IC-002 §3、readme 修改点 7）：
1. `valid` 的构成：`adc_ok && LIGHT_DARK_CALIBRATED`。`adc_ok = (samples_ok > 0)`；**全部转换超时 → valid=false、dark=false**，不得用满量程合成暗态。
2. `C_dark`（`LIGHT_DARK_REF_CODE`）= **完全遮光**、相同供电/ADC 参考/建立时间/平均策略下的实测有效均值码（1..4095）。阈值 = `ceil(C_dark/3)`（readme：无光阈值取完全无光的 1/3）。乘法用 `uint32`，**边界相等为暗**。
3. **每笔独立判断，不使用滞回或历史暗态**；不保留旧 350/250，避免历史暗态绕过 1/3 阈值。初始无状态。
4. 部分成功：均值仅取成功转换；`samples_ok` 记录成功数供验证。全部失败不写有效码。
5. 真实满量程读数（4095，疑似器件开路）仍按同一判据参与判定（校准后通常为暗），但记录为硬件诊断线索；**ADC 超时不得合成为 4095**。
6. `LIGHT_DARK_CALIBRATED=0`（当前默认，尚未实板标定）时 `valid=false` → 光照无法证明无光，升温分支不触发；超温分支不受影响（§6.4）。这保证「未标定」不会被当成「已验收无光」。

标定方法（交付给实板标定能力，见 §13）：
- 条件：PB05 恒高（用固件/测试夹具保持），器件**完全遮光**，供电为实际电池/等效电源，等待时间与量产一致（≥100 ms），采样次数与量产一致（8 次）。
- 记录：板件标识、供电电压、遮光方式、每次原始码样本分布（最小/最大/均值/中位）、采样次数、固件配置版本号。
- 取 `C_dark` = 全暗样本均值（或稳健中位），回填 `LIGHT_DARK_REF_CODE` 并置 `LIGHT_DARK_CALIBRATED=1`。
- 验证：当前有光环境的均值必须明显低于 `ceil(C_dark/3)`（否则阈值不可用，需换器件/检查分压）。
- 不得使用满量程、旧默认 350/250、或当前亮态读数冒充 `C_dark`。

### 6.4 条件上报判定（本版冻结，取代旧降温方向）

```c
/* 语义 (IC-002 §2):  report = valid_sample
 *   AND ( (have_prev AND (int32)cur - (int32)prev > 9 AND light_valid AND light_dark)
 *         OR (int32)cur > 350 )
 * valid_sample (温湿度均有效) 由调用方保证: 仅温湿度成功时才调用本函数。
 */
bool sensor_decide_report(int16_t prev_temp_x10, bool have_prev,
                          int16_t cur_temp_x10,
                          bool light_valid, bool light_dark)
{
    bool rising = have_prev
               && (((int32_t)cur_temp_x10 - (int32_t)prev_temp_x10) > (int32_t)SENSOR_REPORT_RISE_X10) /* 升温 > 0.9℃ */
               && light_valid && light_dark;
    bool superheat = ((int32_t)cur_temp_x10 > (int32_t)SENSOR_REPORT_HIGH_X10);  /* 严格 > 35.0℃ */
    return rising || superheat;
}
```

- `SENSOR_REPORT_RISE_X10 = 9`（0.9℃，0.1℃ 量化下**严格大于 9** 即升温 ≥1.0℃ 才触发）——IC-002 §2 严格解释。若需求确认为「至少升温 0.9℃」，仅把比较改为 `>=`（单点配置）。
- `SENSOR_REPORT_HIGH_X10 = 350`；`>` 为严格超过。**不得再对 `cur == 350` 特判排除**：恰好 35.0℃ 时超温分支为假，但若同时满足升温分支（如 34.0→35.0、无光）仍应上报。
- 差值用 `int32_t` 防溢出；降温（cur<prev）恒不满足 `>9`，无需额外分支。
- 前值语义：`prev` 为**上一次有效采集**温度（含未上报/光照无效的采样），失败周期不推进。

**IC-002 §2 规范向量（实现/验证必须覆盖）**：

| 条件 | 前值 / 本次（℃） | 光照 | 有前值 | 上报 |
|---|---|---|---|---|
| D=+8 | 30.0 / 30.8 | 暗 | 是 | 否 |
| D=+9 | 30.0 / 30.9 | 暗 | 是 | 否 |
| D=+10 | 30.0 / 31.0 | 暗 | 是 | 是 |
| D=+10 | 30.0 / 31.0 | 明或无效 | 是 | 否 |
| D=−10 | 30.0 / 29.0 | 暗 | 是 | 否 |
| 35.0℃ 升温 | 34.0 / 35.0 | 暗 | 是 | 是 |
| 35.0℃ 升温 | 34.0 / 35.0 | 明 | 是 | 否 |
| 独立超温 | 无 / 35.1 | 明或无效 | 否 | 是 |
| 首笔非超温 | 无 / 35.0 | 暗 | 否 | 否 |

---

## 7. 资源预算

见 RTA-002 §3。要点：
- Flash/SRAM 均为**净减少**（退役模块大于新增模块）；无动态内存、无浮点库依赖（换算为整数）。
- 单周期唤醒活动时间 ≈113 ms（上报时 ≈140 ms），3 分钟占空比 ≈0.06%。
- 估算日均 ≈100 µAh；**必须实测** UM2005C 静态电流、MCU 深睡电流、GXHT40 待机电流后回填。

---

## 8. 接口处理（必须保持不变）

### 8.1 传感器 → 网关（433 MHz，10 B）

| 偏移 | 长度 | 字段 | 编码 |
|---:|---:|---|---|
| 0 | 4 | `sensor_id` | `mcu_uid` 取 `ptrs[0], ptrs[3], ptrs[6], ptrs[8]` 现有方式 |
| 4 | 2 | `temperature_x10` | int16，小端 |
| 6 | 2 | `humidity_x10` | uint16，小端 |
| 8 | 2 | `crc16` | CRC16-CCITT-FALSE(0x1021, init 0xFFFF)，小端 |

加密前明文 10 B，8 轮 Feistel + 10 字节置换（`encrytogate.c` 原样复用）。**光照状态/有效性/上报原因不编码、不发送**；采样周期不编码为字段。兼容向量：ID=`01 02 03 04`、T=−5.0℃、RH=60.0% → RF 前 8 B=`01 02 03 04 CE FF 58 02`。

### 8.2 网关 → 手机（BLE FFE0/FFE2，每设备 16 B 中前 8 B 有效）

| 偏移 | 长度 | 字段 | 编码 |
|---:|---:|---|---|
| 0 | 4 | `sensor_id` | 原字节 |
| 4 | 2 | `humidity_x10` | **大端** |
| 6 | 2 | `temperature_x10` | int16 **大端**（负温保留符号） |

网关 `decode_frame10` 取 `temp=p[4..5]`、`hum=p[6..7]`，写 `sensorres` 时交换为 `[id | hum_be | temp_be]`；Android `parsePlainFrame` 按 `u16be@4` / `s16be@6` 解析。**现状源码已一致（§9 已核对），本设计不改动。** 兼容向量：BLE 设备前 8 B=`01 02 03 04 02 58 FF CE`。

### 8.3 GXHT40 替换对空口单位无影响

温度仍为有符号 0.1 ℃、湿度仍为无符号 0.1 %RH；GXHT40 支持 −40…125 ℃，负温必须保留符号。

---

## 9. 网关固件影响评估（结论：无功能改动）

核对依据（既有源码）：
- `APP/feistel_al.c:decode_frame10()`：`temp = p[4] | p[5]<<8`；`hum = p[6] | p[7]<<8`；
- `APP/app_um2006A.c:183–195`：`sensorres[i*8+4..5] = resbf[6..7]`（湿度大端）、`sensorres[i*8+6..7] = resbf[4..5]`（温度大端）；
- Android `MqtttService.kt:parsePlainFrame()`：`humi=u16be(p8,4)`、`temp=s16be(p8,6)`。

三者与 §8 一致，且 GXHT40 不改变帧布局/单位。因此网关固件：
1. 不需要新增光照字段或上报原因；
2. 不需要改变服务/特征/记录宽度；
3. **不得**按自身温度历史二次过滤（上报门控在传感器）；
4. 保持 `radio`/BLE 时序与 `SimpleProfile` 参数不变。

实现阶段仅需一次**只读核对**（任务清单最后一项）；若核对发现不一致，才按接口契约对齐。

---

## 10. 异常策略

| 场景 | 行为 |
|---|---|
| GXHT40 I²C 地址/字节 NACK | 尝试另一地址（0x44/0x45）；均无应答 → 本次测量失败 |
| 读请求 NACK（转换未完成） | 等待 1 ms 后重读，最多 `GXHT40_READ_RETRY`（5）次 |
| 温度字或湿度字 CRC 错 | 丢弃整帧，重发 `0xFD` 重新测量，最多 `GXHT40_MEAS_RETRY`（3）次 |
| 整周期温湿度失败 | **不上报、不推进 `prev_temp_x10`、不冻结快照、不构造 0 值** |
| 温度/湿度超出物理范围 | 温度超出 −40.0…125.0 ℃ 或换算异常 → 视为无效测量（同上失败路径） |
| 光照 ADC 全部转换超时 | `light_valid=false`、`dark=false`；**不得**合成满量程暗态；升温分支关闭 |
| 光照部分转换成功 | 均值取成功样本，记录 `samples_ok`；仍按 1/3 判据 |
| `LIGHT_DARK_CALIBRATED=0`（未标定） | `light_valid=false`；升温分支不触发，超温分支仍独立工作；不得宣称无光已验收 |
| 真实满量程读数 4095 | 按同一判据参与判定（校准后通常为暗），记录为器件开路/未装诊断线索；不阻塞 |
| 433 发送超时（200 ms 未完成） | 有界重试 ≤3，**重试期间发送冻结快照**；用尽则放弃本轮（`report_req=0`），不伪造成功；下个 3 分钟周期自然重试 |
| I²C 总线被从机拉低卡死 | `sf_i2c` 的 ACK 超时路径已发 STOP 释放；上层按失败处理，不无限等待 |
| 光照无效 + 温湿度有效且 >35.0℃ | **上报**（超温分支独立于光照） |

---

## 11. 实现约束

1. **单一配置点**：所有阈值/引脚/重试/等待常量集中在 `sensor_config.h`；不得散落魔法数。
2. **整数运算**：判定与换算不得使用浮点（Cortex-M0+ 无 FPU）；1/3 判据必须用 `uint32` 乘法 `3*mean >= C_dark`，不得先做整数除法截断。
3. **不携带 clock stretching**：GXHT40 不支持，必须「命令 → 按 tMEAS 上限等待 → 读」并容忍读 NACK；等待时间在高重复率下不低于 8.3 ms（建议 10 ms）。
4. **引脚所有权唯一**：PB04 只允许 `light.c` 配置为模拟输入，PB05 只允许 `light.c` 配置为输出；任何其它模块不得再配置 PB04/PB05/PB06。
5. **空闲电平**：采样结束 PB05=0，PA03/PA04 释放为输入，UM2005C 调 `radio_sleep()`，调试 UART 关闭。
6. **失败不产生副作用**：失败路径不写空口、不推进前值、不冻结样本、不清除无关状态。
7. **光照有效性优先**：无有效全暗基准或 ADC 全失败时，`light_valid=false`；不得以任何默认值替代。
8. **待上报冻结**：触发时快照温度/湿度，发送路径只读快照；重试期间禁止用新采样覆盖。
9. **不裸拷贝结构体到线上**：多字节字段显式移位组装（现有做法），保持大小端与符号。
10. **可测试性**：CRC、换算、光照判定、上报判定必须是纯函数（无寄存器依赖），可被宿主机 `test/` 直接调用。
11. **不使用 GXHT40 加热器**：`0x39/0x32/…` 命令不得启用。
12. **不复位从机**：不在每个周期发 `0x94`。
13. **无新增 NVM 写**：前值/快照仅存 RAM（深睡保持）；标定值以编译期常量回填，不写 Flash。
14. **构建可复现**：MDK/IAR 工程与 `gcc/build.sh` 均需通过；新增/改名源文件必须同步两个工程文件列表。

---

## 12. 实现增量划分（任务清单来源）

`artifacts/firmware_tasks.yaml` 的条目按「可独立检查的功能增量」划分：每一项都自带必需前置步骤（新增/修改文件、配置、驱动接入、构建）与对应的可观察行为；**文件编辑、配置、驱动接入与构建不作为独立任务项**，而是增量内部的步骤；项目要求的全暗标定包含在 T2 增量内。前置顺序 T1 → T2 → T3 → T4 → T5 → T6 → T7，靠前项不依赖靠后项。

| 增量 | 预期行为（可观察） | 主要设计依据 |
|---|---|---|
| T1 GXHT40 温湿度采集 | 探测 0x44/0x45、0xFD 高重复率测量、6 B 读取、温度字/湿度字分别 CRC-8、整数 x10 换算与有效域、读 NACK/CRC 错有界重试、失败不修改输出 | §2.2、§6.1、§6.2、§10 |
| T2 光照采集与「全暗基准 1/3」无光判据 | PB05 供电 → 稳定等待 → PB04/AIN11 多次均值 → PB05 断电；结果含 valid/mean；`dark = valid && 3*mean >= C_dark`，每笔独立无滞回；ADC 全超时 valid=false；C_dark 由完全遮光实板标定取得并回填，未标定时不得声称无光 | §2.3、§6.3、§11.7 |
| T3 3 分钟节拍与采样顺序 | RTC 1 分钟节拍累计 3 次触发；单周期只测一次；光照先于温湿度；温湿度失败不推进前值/不冻结/不构造 0 值 | §4、§5.1–5.3、§10 |
| T4 条件上报判定（升温方向 + 严格边界） | 升温 >0.9℃ 且光照有效且无光，或 >35.0℃ 才置待上报；IC-002 九组向量一致；差值 int32；光照无效只关升温分支 | §6.4 |
| T5 条件上报发送与冻结样本 | 仅有待上报才组 10 B 帧并 Feistel 加密；触发样本冻结；重试期间帧内容不变；失败有界重试、用尽放弃、成功才清标志；布局/字节序不变、无光照字段 | §4、§8.1、§10、§11.8 |
| T6 旧通路退役与引脚所有权 | hall/OPTCFG/params/history 及初始化、GPIOB 霍尔中断、LPTIM OPTCFG 分支、NVM 参数写路径移除；PB04 仅 AIN11 模拟输入、PB05 仅光照供电输出、PB06 不外驱动 | §3.3、§11.4 |
| T7 网关与手机链路兼容核对 | 网关按 `temp=p[4..5]`、`hum=p[6..7]` 解密；BLE 记录 `id|hum_be|temp_be` 与 Android `u16be@4`/`s16be@6` 一致；一致则记录「无需改动网关代码」结论 | §8.2、§9 |

标定要求：全暗基准 `C_dark` 的实板标定属 T2 增量内的交付内容，不单列任务。当实板标定不可执行时，必须在证据中如实记录 `LIGHT_DARK_CALIBRATED=0`（未标定）并保持未决状态，**不得**宣称「无光判定已验收」。

真实目标观测（逻辑分析仪抓 I²C/引脚时序、串口 9600 日志、供电回路电流）需要实板与仪器授权；本设计不把主机模拟结果标记为实板通过。该类观测项属下游 `embedded_tester` 能力的职责，见 FWR-OPEN-1 与 §13。

---

## 13. 验证与交接

本能力只做设计；下列为下游能力（`firmware_engineer.firmware_implementation`、嵌入式测试）的验证要点：

| 验证点 | 方法归属 |
|---|---|
| GXHT40 CRC-8 参考向量、换算边界（−45 ℃/125 ℃、0 %/100 %、负温符号） | 宿主机单元测试 |
| 上报判定真值表（IC-002 §2 九组向量：D=8/9/10、降温、明/无效光照、35.0℃ 升温分支、首笔 35.0/35.1℃） | 宿主机单元测试 |
| 光照 1/3 判据：`3*mean` 与 `C_dark` 边界、`C_dark=1..4095` 全覆盖、每笔独立（同码不同历史结果相同）、无效不判暗 | 宿主机单元测试 |
| ADC 全超时 → valid=false（不合成 4095）、部分成功均值与成功数 | 宿主机仿真 MCU harness |
| 3 分钟节拍、单周期单次测量 | 实板/逻辑分析仪（嵌入式测试） |
| GXHT40 实读值与参考温湿度计一致性、地址探测（0x44/0x45） | 实板 I²C 抓包（嵌入式测试） |
| 433 帧布局/加密与网关解析、BLE 记录顺序、冻结样本一致性 | 网关联调（嵌入式测试） |
| 深睡电流、单周期平均电流、CR2032 寿命 | 实测（硬件/低功耗） |
| 完全遮光基准 `C_dark` 实板标定与回填配置头 | 嵌入式测试 → 实现 |

**跨角色交接（不属于本能力执行）**：
- **D-01（固件实现）**：`fw_core.c:105–121` 需改为升温方向、移除 35.0℃ 特例排除、增加 `light_valid` 入参；`light_code_is_dark`/`SENSOR_REPORT_DROP_X10` 及其宿主机用例（`test/host_fw_core_pure_check.c`、`host_fw_core_verify_ev.c`、`host_light_check.c`、`host_light_verify_ev.c`、`host_measure_flow_check.c`、`host_rf_report_verify_ev.c` 等）须同步重写。
- **D-02（固件实现 + 实板标定）**：`light.c` 需返回 `{valid,mean,dark}` 并删除满量程回退；`sensor_config.h` 需以 `LIGHT_DARK_REF_CODE`+`LIGHT_DARK_CALIBRATED` 取代 `LIGHT_DARK_ENTER/EXIT`；取得完全遮光实测值前不得宣称无光已验收。
- **D-03（Android）**：批次顺序与数量归因、避免 BLE→MQTT 只保留末设备导致错配、同步「升温」规则文案；属 Android 能力，本设计只保证空口/BLE 布局不变。
- 需求方：FWR-OPEN-1（`C_dark` 标定）、FWR-OPEN-2（0.9℃ 含等于）、FWR-OPEN-3（是否需保活上报）。

---

## 14. 停止条件

本能力在以下条件满足时结束：`artifacts/firmware_requirements.md`、`artifacts/firmware_rtos_architecture.md`、`artifacts/firmware_design.md`、`artifacts/firmware_tasks.yaml`（T1–T7 功能增量）已产出并登记 `file_manifest.txt`，返回 `DESIGN_READY`。代码实现、实板标定、网关联调与 Android 改动均不在本能力内。
