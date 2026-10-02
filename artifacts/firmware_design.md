# 传感器/网关固件设计方案（FD-002）

状态：固件方案设计（firmware_engineer.firmware_solution_design）
版本：1.0
范围：本次「GXHT40 替换温湿度传感器、新增光敏分压、采集周期 3 分钟、条件上报」在固件域内的方案。**主要改动在传感器固件（CW32L010Y8M6）**；**网关固件（CH592 beiwov2）经核对无需功能改动**。本方案明确模块/任务划分、数据流、时序、资源预算、接口处理、异常策略与实现约束，**不承担代码实现**（由 `firmware_engineer.firmware_implementation` 承接）。

上游输入：
- 需求：`readme.txt`（权威）
- 接口契约：IC-002 `artifacts/interface_contract.md`（已批准）
- 硬件事实：`sensor_hardware/pstxnet.dat`、`sensor_hardware/MAIN_BOARD.BOM`、`gateway_board.xml`
- 器件手册：`gxht40.pdf`（GXHT4x Datasheet V2.6）
- 既有固件源码：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/`、`CH592EVT/EVT/EXAM/BLE/beiwov2/`
- 伴随产物：FWR-002 `artifacts/firmware_requirements.md`、RTA-002 `artifacts/firmware_rtos_architecture.md`

---

## 1. 现状基线与本次变更范围

### 1.1 传感器现状（源码核定）

- 主循环：`temperature_process()` → `send_data_to_gateway()` → `go_to_sleep()`；深睡由 RTC 唤醒。
- RTC：`RTC_INTERVAL_EVERY_1M`（1 分钟/拍），`rtc_set_cnt >= 1` 置 `sample_flag`，即**每 1 分钟采样**。
- 传感器器件：AHT21B 软 I²C（8 位地址字节 0x70/0x71，即 7 位 0x38）；命令 `0xAC`（初始化）、`0x33 0x00`（触发）、`0x71`（读）。
- 上报：`report_req` 门控；判定 `alarm_triggered()` 使用温度上下限、湿度上下限、20 项历史环的温/湿下降阈值；另有「首样本立即上报」与「满 60 拍小时上报」。
- 配置通路：`hall`（PB04 EXTI）+ `optcfg`（PB05 数字光输入，PB06 F2 供电）+ `params`（Flash 顶层页 0xFF80 存 6 个阈值）。
- 空口：`encode_frame10(mcu_uid, tempvalue, huminityvalue, out)` → `uid(4) | temp_LE(2) | hum_LE(2) | crc16_LE(2)`，再 8 轮 Feistel；`app_um2005C_send_data_timeout(...,200)`。

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

关键器件参数：**R3 = 5 MΩ**；J4 为 2 脚光敏电阻（封装 GL4539_2，BOM 无其它光敏器件）；BOM 中**无霍尔器件**。

**推导（本方案的设计输入）**：
1. 旧固件的 PB04=霍尔 EXTI、PB05=光数字输入、PB06=F2 供电在新板上不再成立 → 霍尔检测、光学配置窗口（OPTCFG）、基于 Flash 的参数下发**在硬件上已不存在**。
2. 分压拓扑：`PB05(VDD) → R3(5 MΩ) → LIGHT_ADC(PB04) → 光敏电阻 → GND`。
3. ADC 满量程参考 = VDD（`Examples/ADC/adc_sgl_sw_vdd`：`MCU_VDD = 4.095 × BGR1.2V_mV / code`，12 bit）。PB05 输出高即 VDD，故 `code = 4095 × R_photo / (R_photo + 5 MΩ)` **与电池电压无关（比率式）**，对 CR2032 电压衰减鲁棒。
4. 光敏电阻受光阻值下降 → 节点电压下降 → **无光（暗）时读数大**。

### 1.3 本次固件变更范围

| 子系统 | 变更 |
|---|---|
| 传感器温湿度驱动 | AHT21B → GXHT40（协议、地址、CRC、换算全换） |
| 传感器光照 | 新增：PB05 输出 + PB04/AIN11 ADC + 明暗判定 |
| 传感器节拍 | 1 分钟采样 → **3 分钟采样** |
| 传感器判定 | 温/湿阈值 + 20 项历史下降 + 小时/首样本 → **单一下降判据 + 光照 + 超温** |
| 传感器退役 | hall / OPTCFG / params / history 通路移除 |
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
| PB04 | LIGHT_ADC（AIN11） | 模拟输入 | `PB04_ANALOG_ENABLE()`；ADC 通道 `ADC_InputCH11` |
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
| 返回 | 6 B：`T_MSB,T_LSB,T_CRC, RH_MSB,RH_LSB,RH_CRC`（手册 §7.2 明确「温度数据在前」） |
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
- 极性：**无光（DARK） ⟺ code ≥ 阈值**。

---

## 3. 模块划分（传感器工程文件级计划）

### 3.1 新增

| 文件 | 职责 |
|---|---|
| `USER/inc/sensor_config.h` | 单一配置头：引脚宏（PB04/PB05）、GXHT40 命令/地址/重试上限/等待时间、光照阈值与滞回、采样节拍计数（3）、上报常量（0.9℃、35.0℃）、光照采样次数 |
| `USER/inc/gxht40.h` / `USER/src/gxht40.c` | GXHT40 协议驱动：地址探测、发送命令、等待、读 6 B、CRC 校验、整数换算；对上层只暴露「一次测量 → temp_x10/hum_x10/结果码」 |
| `USER/inc/light.h` / `USER/src/light.c` | 光照通路：PB05 供电、稳定等待、PB04/AIN11 多次采样、均值、DARK/LIT 判定（含滞回与失效处理） |

### 3.2 修改

| 文件 | 动作 |
|---|---|
| `USER/src/measure.c` | 用 `gxht40_measure()` + `light_sample()` 替换 AHT21 序列；接入新判定与上报；去掉 `history/params/optcfg/hall` 依赖；上报失败有界重试保留 |
| `USER/src/main.c` | RTC 计数 3 → `sample_flag`；初始化改为「参数无、光照 ADC 初始化」；移除 `params_init/history_init/optcfg_init/hall_init` 与 hall/optcfg 主循环分支 |
| `USER/src/interrupts_cw32l010.c` | 移除 `GPIOB_IRQHandler` 霍尔分支与 `LPTIM_IRQHandler` 中 OPTCFG 分支（LPTIM 仅保留 433 位时钟） |
| `USER/inc/main.h`、`USER/inc/measure.h` | 声明调整；移除 hall/optcfg/params/history 头包含 |
| `USER/src/fw_core.c` / `USER/inc/fw_core.h` | 保留为**宿主机可测纯逻辑**：新增 `fw_crc8_gxht()`、`gxht40_raw_to_x10()`、`light_code_is_dark()`、`sensor_decide_report()`；移除 OPTCFG 解码器/参数记录等已退役纯逻辑 |
| `USER/src/sf_i2c.c` / `sf_i2c.h` | 增加两个总线原语：`i2c_write_cmd(dev, addr8, cmd)`（START→addr+W→cmd→STOP）与 `i2c_read_bytes(dev, addr8, buf, len)`（START→addr+R→读→STOP），并把 ACK 失败作为返回值；已有函数不改语义 |
| `gcc/cw32l010.ld` | 移除为 `params.c` 预留的顶层 Flash 页说明（不再需要 NVM 记录） |
| `test/host_sensor_core_test.c`、`test/build_test.sh` | 覆盖新的纯逻辑；移除 OPTCFG/params 用例 |

### 3.3 退役（从工程移除）

`USER/src/hall.c`、`USER/inc/hall.h`、`USER/src/optcfg.c`、`USER/inc/optcfg.h`、`USER/src/params.c`、`USER/inc/params.h`、`USER/src/history.c`、`USER/inc/history.h`。

理由：对应硬件（霍尔、F2 光配置接收、可配置阈值下发）在新板上不存在（§1.2），保留会导致 PB04/PB05 被错误重配置并使光照采样失效。

### 3.4 不动

`encrytogate.c`（Feistel/CRC16）、`UM2005C/*`（433 驱动）、`COMMON/*`、标准外设库、MDK/EWARM 工程文件骨架。

---

## 4. 数据流

```text
[RTC 1min ×3]
      │
      ▼
light_sample():  PB05=1 → delay ≈100ms → ADC(AIN11) ×N → 均值 code
                 PB05=0
                 → dark = code >= LIGHT_DARK_ENTER(带滞回)   ── dark: bool
      │
      ▼
gxht40_measure(): probe 0x44/0x45
                  write 0xFD → wait ≥ tMEAS.H.max → read 6B
                  → CRC(T), CRC(RH) → temp_x10, hum_x10        ── ok: bool
      │
      ▼
sensor_decide_report(prev_temp_x10, have_prev, cur_temp_x10, dark)
      │  report: bool
      ▼
encode_frame10(uid10, temp_x10, hum_x10, frame10)   // 现有
      │
      ▼
app_um2005C_send_data_timeout(frame10, 10, 200)     // 现有, 有界
      │
      ▼ 成功 → 清除 report_req, prev_temp_x10 = cur_temp_x10
        失败 → 有界重试(≤3), 用尽则放弃本轮(前值仍更新为本次有效测量值)
```

RAM 状态（深睡保留）：
```
prev_temp_x10 (int16)   前一有效测量温度; 0.1℃
have_prev     (bool)    是否已有前一有效样本
last_hum_x10  (uint16)  最近有效湿度(仅用于组帧)
light_dark_state (bool) 光照滞回状态
sample_flag / rtc_tick_cnt / report_req / report_retry
```

---

## 5. 时序

### 5.1 采样周期（3 分钟）

```text
t=0        RTC 1min 中断 (rtc_tick_cnt=1) → 立即回深睡
t=60s      RTC 中断 (2) → 深睡
t=120s     RTC 中断 (3) → sample_flag=1, 主循环继续
t=120s     PB05=1, 等待 100ms
t=120.1s   ADC ×8 (≈3.2ms), PB05=0
t=120.1s   GXHT40: 0xFD, 等待 10ms
t=120.11s  读 6B (≈1ms) → 判定
t=120.11s  需要则 433 发射 ≈27ms
t≈120.1s   深睡
t=300s     下一轮
```

### 5.2 3 分钟节拍实现

`RTC_Configuration()` 保持 `RTC_INTERVAL_EVERY_1M`；`RTC_IRQHandlerCallBack()` 中 `rtc_tick_cnt++`，达到 `SENSOR_SAMPLE_TICKS`（=3）时置 `sample_flag` 并清零。**注意**：RTC 时钟源为 LSI，分钟节拍存在 LSI 容差（约 ±5% 量级），3 分钟为近似值；readme 未给出精度要求。

---

## 6. 关键算法（实现必须逐位一致）

### 6.1 GXHT40 CRC-8

```c
uint8_t fw_crc8_gxht(const uint8_t *p, uint8_t n) {   /* poly 0x31, init 0xFF, 无反转, 无 xorout */
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

### 6.3 光照判定

```c
/* N 次采样取算术平均; code ∈ [0,4095] */
bool light_code_is_dark(uint16_t code) {
    if (light_dark_state) return !(code <= LIGHT_DARK_EXIT);   /* 已暗: 低于退出阈值才转明 */
    return (code >= LIGHT_DARK_ENTER);                          /* 已明: 达到进入阈值才转暗 */
}
```
- 初始默认值（**必须由实板标定替换**）：`LIGHT_DARK_ENTER = 350`（≈ R_photo 464 kΩ）、`LIGHT_DARK_EXIT = 250`（≈ R_photo 323 kΩ）。二者相等即关闭滞回。
- 标定方法（交付给实现/测试）：PB05 恒高，在「预期最亮」与「预期最暗」两种实际使用条件下各读 30 次均值，取两者之间、且偏离两端 ≥2 倍噪声带的整数作为 `LIGHT_DARK_ENTER`，退出阈值取 `ENTER − 100`。

### 6.4 上报判定

```c
bool sensor_decide_report(int16_t prev_temp_x10, bool have_prev,
                          int16_t cur_temp_x10, bool dark) {
    if (cur_temp_x10 > SENSOR_REPORT_HIGH_X10) return true;                 /* > 35.0 ℃ */
    if (!have_prev) return false;                                           /* 无前值: 下降分支不成立 */
    if (!dark) return false;                                                /* 需同时「无光」 */
    return ((int32_t)prev_temp_x10 - (int32_t)cur_temp_x10) > SENSOR_REPORT_DROP_X10; /* 下降 > 0.9 ℃ */
}
```
- `SENSOR_REPORT_DROP_X10 = 9`（0.9 ℃，0.1 ℃ 量化下**严格大于 9** 即下降 ≥1.0 ℃ 才触发）——按 IC-002 §2 的严格解释。若需求确认为「至少下降 0.9 ℃」，仅把比较改为 `>=`。
- `SENSOR_REPORT_HIGH_X10 = 350`；`>` 为严格超过，恰好 35.0 ℃ 不触发。
- 与 IC-002 表达式等价：IC-002 的 `T<35` 条件在超温分支已覆盖，属冗余，不影响结果。

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
| 4 | 2 | `temperature_x10` | int16，小端（`temp_x10 >> 8` 先发） |
| 6 | 2 | `humidity_x10` | uint16，小端 |
| 8 | 2 | `crc16` | CRC16-CCITT-FALSE(0x1021, init 0xFFFF)，小端 |

加密前明文 10 B，8 轮 Feistel + 10 字节置换（`encrytogate.c` 原样复用）。**光照状态不编码、不发送**；采样周期不编码为字段。

### 8.2 网关 → 手机（BLE FFE0/FFE2，每设备 8 B）

| 偏移 | 长度 | 字段 | 编码 |
|---:|---:|---|---|
| 0 | 4 | `sensor_id` | 原字节 |
| 4 | 2 | `humidity_x10` | **大端** |
| 6 | 2 | `temperature_x10` | int16 **大端**（负温保留符号） |

网关 `decode_frame10` 取 `temp=p[4..5]`、`hum=p[6..7]`，写 `sensorres` 时交换为 `[id | hum_be | temp_be]`；Android `parsePlainFrame` 按 `u16be@4` / `s16be@6` 解析。**现状源码已一致（§9 已核对），本设计不改动。**

### 8.3 GXHT40 替换对空口单位无影响

温度仍为有符号 0.1 ℃、湿度仍为无符号 0.1 %RH；GXHT40 支持 −40…125 ℃，负温必须保留符号。

---

## 9. 网关固件影响评估（结论：无功能改动）

核对依据（既有源码）：
- `APP/feistel_al.c:decode_frame10()`：`temp = p[4] | p[5]<<8`；`hum = p[6] | p[7]<<8`；
- `APP/app_um2006A.c`：`sensorres[i*8+4..5] = resbf[6..7]`（湿度大端）、`sensorres[i*8+6..7] = resbf[4..5]`（温度大端）；
- Android `MqtttService.kt:parsePlainFrame()`：`humi=u16be(p8,4)`、`temp=s16be(p8,6)`。

三者与 §8 一致，且 GXHT40 不改变帧布局/单位。因此网关固件：
1. 不需要新增光照字段；
2. 不需要改变服务/特征/记录宽度；
3. **不得**按自身温度历史二次过滤（上报门控在传感器）；
4. 保持 `radio`/BLE 时序与 `SimpleProfile` 参数不变。

实现阶段仅需一次**只读核对**（对应任务清单最后一项）；若核对发现不一致，才按接口契约对齐。

---

## 10. 异常策略

| 场景 | 行为 |
|---|---|
| GXHT40 I²C 地址/字节 NACK | 尝试另一地址（0x44/0x45）；均无应答 → 本次测量失败 |
| 读请求 NACK（转换未完成） | 等待 1 ms 后重读，最多 `GXHT40_READ_RETRY`（建议 5）次 |
| 温度字或湿度字 CRC 错 | 丢弃整帧，重发 `0xFD` 重新测量，最多 `GXHT40_MEAS_RETRY`（建议 3）次 |
| 整周期测量失败 | **不上报、不更新 `prev_temp_x10`、不构造 0 值**；湿度沿用 `last_hum_x10` 仅供组帧（不触发上报） |
| 温度/湿度超出物理范围 | 温度超出 −40.0…125.0 ℃ 或换算异常 → 视为无效测量（同上失败路径） |
| 光敏读数 = 满量程 4095 | 视为 DARK（分压上拉/器件开路），记录为异常；方向为「倾向上报」，不阻塞 |
| 光敏读数 = 0 | 视为 LIT |
| 433 发送超时（200 ms 未完成） | 原有有界重试 ≤3，用尽则放弃本轮；**不写 `report_req=0` 的假成功**；下次 3 分钟周期自然重试 |
| I²C 总线被从机拉低卡死 | `sf_i2c` 的 ACK 超时路径已发 STOP 释放；上层按失败处理，不无限等待 |
| 光敏/温湿度同时失效 | 不产生上报（下降分支需 DARK，超温分支需有效温度） |

---

## 11. 实现约束

1. **单一配置点**：所有阈值/引脚/重试/等待常量集中在 `sensor_config.h`；不得散落魔法数。
2. **整数运算**：判定与换算不得使用浮点（Cortex-M0+ 无 FPU）；换算公式必须按 §6.2 取证。
3. **不携带 clock stretching**：GXHT40 不支持，必须「命令 → 按 tMEAS 上限等待 → 读」并容忍读 NACK；等待时间在高重复率下不低于 8.3 ms（建议 10 ms），在 VDD 低至 1.6 V 时仍满足。
4. **引脚所有权唯一**：PB04 只允许 `light.c` 配置为模拟输入，PB05 只允许 `light.c` 配置为输出；任何其它模块不得再配置 PB04/PB05/PB06。
5. **空闲电平**：采样结束 PB05=0，PA03/PA04 释放为输入，UM2005C 调 `radio_sleep()`，调试 UART 关闭。
6. **失败不产生副作用**：失败路径不写空口、不推进前值、不清除 `report_req` 以外无关状态。
7. **不裸拷贝结构体到线上**：多字节字段显式移位组装（现有做法），保持大小端与符号。
8. **可测试性**：CRC、换算、光照判定、上报判定必须是纯函数（无寄存器依赖），可被宿主机 `test/` 直接调用。
9. **不使用 GXHT40 加热器**：本产品无防结露需求；`0x39/0x32/…` 命令不得启用（占空比与峰值电流约束）。
10. **不复位从机**：不在每个周期发 `0x94`；上电天然复位，周期软复位只会增加功耗。
11. **无新增 NVM 写**：前值仅存 RAM（深睡保持）；不得每周期写 Flash（功耗与擦写寿命）。
12. **构建可复现**：MDK/IAR 工程与 `gcc/build.sh` 均需通过；新增源文件必须加入两个工程。

---

## 12. 验证与交接

本能力只做设计；下列为下游能力（`firmware_engineer.firmware_implementation`、嵌入式测试）的验证要点：

| 验证点 | 方法归属 |
|---|---|
| GXHT40 CRC-8 参考向量、换算边界（−45 ℃/125 ℃、0 %/100 %、负温符号） | 宿主机单元测试 |
| 上报判定真值表（0.9 ℃ 边界、35.0 ℃ 边界、无前值、非暗、失败不更新） | 宿主机单元测试 |
| 光照阈值/滞回/失效（0、4095、阈值上下） | 宿主机单元测试 + 实板标定 |
| 3 分钟节拍、单周期单次测量 | 实板/逻辑分析仪（嵌入式测试） |
| GXHT40 实读值与参考温湿度计一致性、地址探测（0x44/0x45） | 实板 I²C 抓包（嵌入式测试） |
| 433 帧布局/加密与网关解析、BLE 记录顺序 | 网关联调（嵌入式测试） |
| 深睡电流、单周期平均电流、CR2032 寿命 | 实测（硬件/低功耗） |
| 光照阈值实板标定值回填 `sensor_config.h` | 嵌入式测试 → 实现 |

**跨角色交接（不属于本能力执行）**：
- Android（readme 修改点 6）：空口/BLE 布局不变，预期无需协议改动；若 App 侧对「每 3 分钟必有数据」有硬编码假设，需 Android 能力确认——已登记 FWR-OPEN-5。
- 需求方：FWR-OPEN-1（光照阈值标定）、FWR-OPEN-2（0.9 ℃ 含等于）、FWR-OPEN-3（是否需保活上报）。

---

## 13. 停止条件

本能力在以下条件满足时结束：`artifacts/firmware_requirements.md`、`artifacts/firmware_rtos_architecture.md`、`artifacts/firmware_design.md`、任务清单文件已产出并登记 `file_manifest.txt`，返回 `DESIGN_READY`。代码实现、实板标定、网关联调与 Android 改动均不在本能力内。
