# 固件方案设计（FDS-003）

状态：`DESIGN_READY`（方案设计完成；**未实现、未烧录、未实板验证、未产品验收**）
能力：`firmware_engineer.firmware_solution_design`
运行：dengbei_run10
基线：Git `12bb5b2`（接口契约 v4 提交）；本能力开始前工作区干净；**本能力不修改产品源码**
输入：`readme.txt`、PDR-001、PSPEC-001、PACC-002、IC-003、FEX-001、SA-001、SARCH-001、SD-001
配套：`artifacts/firmware_requirements_v3.md`（FWR-003）、`artifacts/firmware_rtos_architecture_v3.md`（RTA-003）、`artifacts/firmware_tasks_v3.yaml`

> 本文件描述“准备怎样实现”，不是实现记录。任何“已修正/已通过”表述都必须由后续实现/独立验证的当前版本证据支撑。

---

## 1. 设计目标与非目标

### 1.1 目标

1. 把 PR-004/IC-003 的升温门控、35.0 ℃ 边界、判定前 prev、待发样本冻结、单周期有界发送、`S` 行语义落到现有 `fw_core.c`/`measure.c`/`main.c`/`debug_trace.c`。
2. 把光照 1/3 判据的 `C_dark` 标定、未标定门禁、可分离性检查收敛到唯一配置点，并保持“不伪造暗态”。
3. 把 UART1 诊断的无限等待改为有界故障隔离，保持既有行格式/引脚/启动顺序。
4. 保持 GXHT40 驱动的地址/命令/CRC/量程/有界重试/总线恢复不变；把真实无 ACK（U01）明确为硬件依赖，不用固件绕过。
5. 保持裸机 RTC 深睡、3 分钟采样、无心跳、无跨周期发送队列、无新增协议字段。
6. 提供可宿主机验证的纯逻辑/流程/诊断测试面，且不修改测试能力拥有的独立 harness。

### 1.2 非目标

- 不实现网关、Android、服务器、iOS；不修改 RF/BLE/上传布局、密钥、topic、认证。
- 不新增硬件、不改 PCB/接线/器件、不新增 NVM 参数/标定写协议。
- 不解决 U9 焊接/器件/引脚供电（需硬件/嵌入式测试在另行授权下完成）。
- 不在本能力烧录、复位、采串口、开逻辑分析仪或连接手机。
- 不把 `DESIGN_READY` 当作实现完成或产品通过。

---

## 2. 设计决定

| ID | 决定 | 理由 | 影响文件 |
|---|---|---|---|
| DD-01 | 新增产品路径纯函数 `sensor_gate_decide(sample_valid, prev, have_prev, cur, light_valid, dark)`；内部仅当 `sample_valid` 时调用保留的 `sensor_decide_report(prev, have_prev, cur, light_valid && dark)`。删除 `cur==350` 提前返回；升温用 `int32 cur-prev > 9`；旧 4 参签名保留以兼容独立 harness，但其语义同样按升温修正 | FEX v3/v6/v7 已证明现实现 3/14 不符；IF02/PA-008；避免破坏测试能力已有 4 参调用 | `fw_core.h/.c`、调用点、测试 |
| DD-02 | 判定前快照 `prev_for_decision/have_prev_for_decision`；判定后才推进 `prev`；轨迹用快照 | 现 `measure.c:497/531` 使 `p==t`，PA-017 要求 p 为判定前 prev | `measure.c`、`debug_trace` 调用 |
| DD-03 | 每采样机会只执行一次事务：光照→温湿度→判定→冻结→发送（内部最多 3 次）→唯一 `S`/`G`→关 UART→睡眠 | 现主循环把发送拆到多轮，第一轮就打印 `S`，不能表达最终结果/次数 | `main.c`、`measure.c` |
| DD-04 | 触发时一次性编码 `pending_frame[10]`，重试只发送同一字节缓冲；成功/用尽均清 pending；不跨周期 | FWR-307/PA-010；现每次重试重新编码全局值 | `measure.c`、`encrytogate.c` 调用 |
| DD-05 | `S.y` 保持 IC-003 §9 的现行失败计数器语义（成功/用尽后为 0，不能当总尝试数）；发送尝试次数由实现侧 mock/协议 harness 的同版本证据证明，不通过改字段语义表达 | 避免与接口契约冲突；PA-010 的“次数”用调用计数证据，不冒充 S 字段 | `measure.c`、`debug_trace`、测试 |
| DD-06 | `light_sample()` 算术不变；`valid=adc_ok && Calibrated`；增加配置合法性编译期检查；标定只改 `sensor_config.h` 常量并重建 | 无 NVM/远程标定；FWR-303/304、PA-005/031 | `light.c`、`sensor_config.h`、测试 |
| DD-07 | UART1 输出加“整行预算”式有界等待；超时中止本行、照常关 UART；不使用新定时器/中断 | IC-003 §9 明确 TXE/TXBUSY 无界；FWR-310 | `debug_trace.c`、`sensor_config.h`、mock |
| DD-08 | UID 零致命路径：先打印 BOOT/诊断，再清 `sample_flag/report_req` 并进入真正深睡循环 | 现 `main.c:205-210` 在 `sample_flag=1` 下空转；FWR-311 | `main.c` |
| DD-09 | BOOT 使用本轮构建标识常量；镜像身份仍由提交/axf md5 证据绑定 | 现硬编码 `FD-002r4` 不证明当前构建；IC-003 §9 | `debug_trace.c`/`sensor_config.h` |
| DD-10 | 保持 GXHT40 驱动行为与诊断字段；不新增地址/命令/重试/模拟样本；真实 U01 交硬件/独立测试 | FEX 已证明主机序列正确、器件 NACK；改驱动无效 | `gxht40.c`/`sf_i2c.c` 不改行为 |
| DD-11 | 保留 `report_req` 作为待发活动标志以兼容现有接口/测试；新增内部 `pending_frame`/`pending_fail_count`；不新增跨周期队列 | 减少头文件/测试破坏；PA-010 要求单周期 | `measure.c`/`measure.h` |
| DD-12 | 不引入动态内存/新任务/新外设；状态静态、复位清零；ISR 只置 tick/flag | RTA-003；FWR-313 | 全工程 |

---

## 3. 模块与改动面

| 模块/文件 | 当前职责 | 本设计改动 | 明确不改 |
|---|---|---|---|
| `USER/inc/fw_core.h` | 纯逻辑接口（CRC/换算/光照/门控） | 门控签名与常量名同步；注释改为升温 | 不改 CRC/换算/光照判据语义 |
| `USER/src/fw_core.c` | 纯逻辑实现 | 门控实现改为 DD-01；删除 35.0 短路 | 不改 CRC/换算/有效域 |
| `USER/inc/sensor_config.h` | 唯一配置点 | `RISE` 常量；UART 预算常量；BOOT 构建标识；标定合法性 `#error`；注释同步 | 不改既有引脚/时序/阈值数值语义 |
| `USER/src/measure.c` | 采样/判定/发送/轨迹状态 | DD-02/03/04/05/11：快照、单周期发送循环、冻结帧、轨迹字段 | 不改 GXHT40/光照调用顺序与失败语义 |
| `USER/inc/measure.h` | 采样/诊断接口 | 如新增内部函数则只暴露必要声明；`report_req` 兼容保留 | 不暴露测试注入接口 |
| `USER/src/main.c` | 启动/主循环/RTC | DD-03/08：单次事务调用、S 后睡眠、UID 致命路径、BOOT 顺序 | 不改 RTC 1min×3、不改 GPIOA 复位 |
| `USER/src/debug_trace.c` | UART1 行渲染/开关 | DD-07/09：有界等待、BOOT 标识、整行预算 | 不改行字段集合/96B/引脚 |
| `USER/inc/debug_trace.h` | 行结构 | 如需增加“行开始”内部钩子只在 `.c` 内部；不改字段 | 不新增线上字段 |
| `USER/src/light.c` | 光照采样 | 仅配置校验/注释；算术不变 | 不改 100ms/8 次/均值/极性 |
| `USER/src/gxht40.c`、`sf_i2c.c` | GXHT40/软 I2C | 仅注释/测试需要时不动行为 | 不改地址/命令/CRC/重试/恢复上限 |
| `USER/src/encrytogate.c` | 10B 编码 | 不改；由 `measure.c` 调用一次 | 不改密钥/Feistel/CRC |
| `test/host_*_check.c`、`test/build_test.sh` | 实现侧宿主机自检 | 更新错误方向期望；新增冻结/p/UART/标定用例 | 不修改 `*_verify_ev.c`（测试能力所有） |

> 具体行号以当前工作树为准；实现时先重新定位，不凭本文件行号盲改。

---

## 4. 数据模型

### 4.1 采样事务快照（`measure.c` 内部静态）

```c
typedef struct {
    bool     sample_valid;      /* 温湿度双 CRC + 量程通过 */
    int16_t  temp_x10;          /* 本次有效温度 */
    uint16_t hum_x10;           /* 本次有效湿度 */
    int16_t  prev_for_decision; /* 判定前上一有效温度 */
    bool     have_prev_for_decision;
    light_result_t light;       /* 本次光照结果（含 valid/dark/原始码） */
    bool     report;            /* 门控结果 */
} sample_cycle_t;
```

- 只在一次采样机会内有效；不作为跨周期业务缓存。
- `prev_for_decision` 在进入测量前从 `s_prev_temp_x10` 复制；成功后判定使用该值，之后才写回。
- 失败周期 `sample_valid=false`，`prev_for_decision` 仍为当前 prev（未变），`report=false`。

### 4.2 待发快照（`measure.c` 内部静态）

```c
typedef struct {
    bool     active;            /* 等价于现有 report_req，兼容保留 */
    uint8_t  frame[10];         /* 触发时一次编码后的密文帧 */
    uint8_t  fail_count;        /* 本周期失败发送次数 0..3 */
    uint8_t  result;            /* 0 未发送 / 1 成功 / 2 失败 */
} pending_tx_t;
```

- `frame` 在触发时由 `encode_frame10(mcu_uid, cur_temp, cur_hum, frame)` 生成；重试不再编码。
- `fail_count` 只在本周期发送循环内递增；`S` 读取后清零。
- `active` 在发送循环结束后无条件清 0（成功或用尽）；不跨周期。

### 4.3 轨迹快照（`measure.c` → `debug_trace_sample_t`）

沿用现有 `debug_trace_sample_t` 字段，不新增线上字段；填入语义修正：

| 字段 | 新语义 |
|---|---|
| `tick` | `rtc_tick_total`（分钟 tick） |
| `prev_temp_x10` | `prev_for_decision`（判定前，不是推进后） |
| `have_prev` | `have_prev_for_decision` |
| `light_*` | 本周期 `light_result_t` |
| `temp_x10/hum_x10` | 成功=本次值；失败=最近有效值（`q=0`，仅诊断） |
| `sample_ok` | 本周期温湿度是否有效 |
| `report` | 本周期门控结果（失败=false） |
| `send` | 本周期最终本地结果 0/1/2 |
| `retry` | 本周期失败发送次数 0..3（`S` 后清零） |

---

## 5. 门控纯逻辑（唯一真值）

### 5.1 规范函数

产品路径使用新增的完整包装（纯逻辑，无 MCU 依赖；14 向量以此为准）：

```c
/* 产品路径：先检查 sample_valid，再把 light_valid 折算进 dark */
bool sensor_gate_decide(bool sample_valid,
                        int16_t prev_x10, bool have_prev,
                        int16_t cur_x10,
                        bool light_valid, bool dark)
{
    if (!sample_valid) return false;                 /* 采集失败不判定 */
    return sensor_decide_report(prev_x10, have_prev, cur_x10,
                                (bool)(light_valid && dark));
}
```

保留既有 4 参纯函数（供独立 harness 继续调用，语义同步修正）：

```c
bool sensor_decide_report(int16_t prev_x10, bool have_prev,
                          int16_t cur_x10, bool dark)
{
    if ((int32_t)cur_x10 > 350) return true;         /* 严格 >35.0，独立 */
    if (!have_prev) return false;
    if (!dark) return false;                         /* 光照无效/有光不升温 */
    return ((int32_t)cur_x10 - (int32_t)prev_x10) > 9; /* 升温严格 >0.9 */
}
```

- **不得**在 `cur == 350` 时提前返回；恰好 35.0 ℃ 仍要评估升温分支。
- 产品调用只经 `sensor_gate_decide`；`dark` 由 `light_sample` 给出，包装再次显式要求 `light_valid && dark`，防止误用。
- `sample_valid` 只指温湿度；光照有效性只作用于升温分支。
- 差值用 `int32`；常量 `SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350` 保留在唯一配置点。
- 旧 4 参签名不再接受 `light_valid`；独立 harness 若直接调用，必须传 `dark=(light_valid && dark)`；实现侧 14 向量统一调用包装。

### 5.2 14 向量（实现/实现侧测试必须逐条覆盖）

| # | sample_valid | have_prev | prev | cur | light_valid | dark | 期望 |
|---:|---|---:|---:|---:|---|---|---|
| 1 | 1 | 1 | 300 | 308 | 1 | 1 | 0 |
| 2 | 1 | 1 | 300 | 309 | 1 | 1 | 0 |
| 3 | 1 | 1 | 300 | 310 | 1 | 1 | 1 |
| 4 | 1 | 1 | 300 | 310 | 1 | 0 | 0 |
| 5 | 1 | 1 | 300 | 310 | 0 | 0 | 0 |
| 6 | 1 | 1 | 310 | 300 | 1 | 1 | 0 |
| 7 | 1 | 1 | 340 | 350 | 1 | 1 | 1（升温分支） |
| 8 | 1 | 1 | 340 | 350 | 1 | 0 | 0 |
| 9 | 1 | 0 | 0 | 351 | 0 | 0 | 1（超温） |
| 10 | 1 | 0 | 0 | 350 | 1 | 1 | 0（无前值且非超温） |
| 11 | 1 | 0 | 0 | 360 | 0 | 0 | 1（超温） |
| 12 | 1 | 1 | 352 | 351 | 0 | 0 | 1（仍 >35.0） |
| 13 | 1 | 1 | 300 | 310 | 0 | 0（ADC 全失败） | 0 |
| 14 | 0 | 0 | — | — | — | — | 0（不调用/返回 false） |

> 向量 7 与 10 是当前实现的两个短路点；向量 3/6 是方向错误。实现侧测试必须能同时证明“旧实现失败、新实现通过”，不得只改期望。

---

## 6. 采样事务与发送快照

### 6.1 单次事务伪代码

```text
on_sample_opportunity():
    prev_dec = s_prev; have_dec = s_have
    pending.result = 0; pending.fail_count = 0   /* 不把上一机会的发送结果带进本行 */
    light = light_sample()

    ok = (gxht40_measure(&t, &h) == GXHT40_OK)
    if ok:
        rep = sensor_gate_decide(true, prev_dec, have_dec, t, light.valid, light.dark)
        if rep:
            pending.frame = encode_frame10(uid, t, h)   /* 一次编码 */
            pending.active = true
            pending.fail_count = 0
            pending.result = 0
        s_prev = t; s_have = true; s_last_hum = h       /* 判定后推进 */
        trace = { prev_dec, have_dec, t, h, light, ok, rep }
    else:
        gxht40_diag_fetch(&diag)
        trace = { prev_dec, have_dec, last_t, last_h, light, ok=0, rep=0, diag }

    if pending.active:
        for attempt in 1..3:
            if send(pending.frame, 10, 200):
                pending.result = 1; break
            pending.fail_count = attempt
            pending.result = 2
        pending.active = false

    trace.send = pending.result
    trace.retry = 0                 /* IC-003: 成功/用尽后计数器为 0；次数由 harness 证据 */
    publish trace (one S; if !ok append G)
    clear pending.result/fail_count after S
```

要点：

1. `prev_dec/have_dec` 在测量前复制，失败也不变；判定和轨迹都用它。
2. 触发时先编码再推进 prev；编码参数就是触发本次判定的 `t/h`。
3. 发送循环最多 3 次，全部发送 `pending.frame` 同一字节；不重新编码、不重采样。
4. 成功或用尽都清 `active`；不保留跨周期队列；`result/fail_count` 仅在本次 S 前保留。
5. `S` 在发送循环结束后打印一次；失败周期追加 `G`；`S.s` 是最终结果，`S.y` 按 IC-003 在成功/用尽后为 0（尝试次数见实现侧 mock/协议证据）。
6. 无论发送结果如何，本周期结束后 `pending.active=false`，允许睡眠。

### 6.2 发送结果与 `S` 行

| 情况 | `pending.result` | `S.s` | `S.y` | 说明 |
|---|---:|---:|---:|---|
| 未触发 | 0 | 0 | 0 | 不发送 |
| 第 1 次成功 | 1 | 1 | 0 | 无失败 |
| 第 2/3 次成功 | 1 | 1 | 0 | IC-003：成功即清零 |
| 3 次失败 | 2 | 2 | 0 | IC-003：用尽即清零 |
| 采集失败 | 0 | 0 | 0 | 不判定/不发送 |

> `S.y` 不承载总尝试数；IC-003 §9 明确其现行语义是成功/用尽后为 0。发送尝试次数（1..3）由实现侧 mock 的调用计数和协议 harness 证据证明；若产品要求 S 行可见次数，需接口责任另行决定字段/语义，不得在本轮静默改写。

### 6.3 与现有代码的差异

- 现有 `send_data_to_gateway()` 每次调用重新编码并只做一次尝试；主循环多轮调用，`S` 在第一轮打印。新设计把它改为“单次调用内完成 3 次尝试”，`main` 只调用一次。
- 现有 `report_req` 仍是待发活动标志；新增 `frame/fail_count/result` 静态；不新增头文件业务字段。
- 现有 `report_retry` 被 `pending.fail_count` 取代，仅用于发送循环控制与实现侧调用计数证据；`S.y` 仍按 IC-003 在成功/用尽后为 0，不用它表达总尝试数。`measure.h` 的 `report_req` 兼容保留，测试可继续观察。

---

## 7. 光照与标定

### 7.1 保持的算法

`light.c` 不改：

- `PB05` 高 → `delay_ms(100)` → 最多 8 次 `light_adc_read_once` → 只对成功值求整数均值/min/max → `ADC_Disable()` → `PB05` 低。
- `valid = adc_ok && LIGHT_DARK_CALIBRATED`。
- `dark = light_is_dark(mean, valid, LIGHT_DARK_REF_CODE)`，即 `valid && 3*mean >= C_dark`。
- 全部失败：`samples_ok=0, mean=0, valid=false, dark=false`；不得回退 4095。

### 7.2 配置与编译期校验

`sensor_config.h`：

```c
#ifndef LIGHT_DARK_REF_CODE
#define LIGHT_DARK_REF_CODE 0u      /* 0=未标定；标定后 1..4095 */
#endif
#ifndef LIGHT_DARK_CALIBRATED
#define LIGHT_DARK_CALIBRATED 0     /* 0/1 */
#endif

#if (LIGHT_DARK_CALIBRATED != 0) && (LIGHT_DARK_CALIBRATED != 1)
#error "LIGHT_DARK_CALIBRATED must be 0 or 1"
#endif
#if LIGHT_DARK_CALIBRATED
#if (LIGHT_DARK_REF_CODE < 1u) || (LIGHT_DARK_REF_CODE > 4095u)
#error "LIGHT_DARK_REF_CODE must be 1..4095 when calibrated"
#endif
#endif
```

- 允许构建期 `-D` 覆盖以做宿主机边界测试；交付件用文件值。
- 未标定时 `C_dark=0` 不参与判暗（`valid=false` 已短路）。
- 不新增 `LIGHT_AMBIENT_REF_CODE` 运行期字段；有光均值 `L` 记录在标定记录中，用于 PA-031 的可分离性判定。

### 7.3 标定流程（现场，条件依赖）

1. 用当前构建（`CALIBRATED=0`）在完全遮光下采集 ≥10 个采样机会；从 `S` 行读取 `o/n/x/a`，记录板号、镜像 md5、供电、遮光方法、时间。
2. `C_dark =` 这些有效 `a` 的均值（整数）；必须 1..4095；饱和/0 视为无效，不标定。
3. 在正常安装位置采集 ≥10 个机会，得到 `L`（有光均值）。
4. 检查 `C_dark > 3*L`：
   - 成立：回填 `LIGHT_DARK_REF_CODE=C_dark`、`LIGHT_DARK_CALIBRATED=1`；重建后复测全暗 `dark=1`、有光 `dark=0`、`mean=ceil(C_dark/3)` 边界为暗。
   - 不成立：保持 `CALIBRATED=0`，记录“1/3 规则在该硬件/安装不可分离”，交产品/需求决定；不得改阈值、换基准或用调参伪通过。
5. 标定记录与配置 diff 一并交独立验证；不得写 Flash/NVM。

### 7.4 边界测试

`mean = ceil(C_dark/3)` 判暗；`mean = ceil(C_dark/3)-1` 判亮（对任意 1..4095 的 `C_dark` 成立，因为 `3*(ceil-1) < C_dark`）。宿主机用多组 `C_dark`（含 1/3/4095）验证。

---

## 8. GXHT40 与软 I²C

### 8.1 保持不变

- 地址 `0x44/0x45`，写 `0x88/0x8A`，读 `0x89/0x8B`；命令 `0xFD`；等待 10 ms；读 6 B；双 CRC-8；整数换算；温度有效域；湿度裁剪。
- `GXHT40_READ_RETRY=5`、`GXHT40_MEAS_RETRY=3`、首次上电余量 2 ms；读 NACK 重读；失败后下一机会 `i2c_bus_recover` + 重探两地址。
- 周期路径不用软复位/加热命令；boot 一次性 general-call 恢复保持既有行为，不进入周期。
- 失败不写输出、不改调用方值；`gxht40_diag_fetch` 只读。

### 8.2 为什么不在本轮改驱动

FEX 的外部逻辑分析仪在本轮真实总线上解出 6 笔 `0x44/0x45` 地址写、全部 NACK，且与 `gxht40_acquire()` 的 3 次×2 地址逐项一致；主机位序列/地址/重试编排已被证明正确。无 ACK 的剩余原因在 U9 焊接/器件/引脚供电，需硬件通断/电压/换件证据（U01）。因此：

- 不新增地址、命令、无限重试、软件“假 ACK”或模拟样本。
- 不在固件里尝试“换 SDA/SCL”修复装配；引脚角色由网表/代码固定，逻辑仪标签问题不是固件改线依据。
- 真实采集恢复任务在 U01 关闭后由独立测试验证；本设计只保证失败路径正确、可诊断、可恢复探测。

### 8.3 诊断

保持 `G` 行：

```text
G s=<1..5> a44=<0|1> a45=<0|1> rd=<0..5> at=<1..3> raw=<12HEX|------------>
```

- `s` 为 `gxht40_status_t`（1 PARAM/2 NO_DEVICE/3 IO/4 CRC/5 RANGE）。
- `a44/a45` 为本轮地址 ACK；`rd` 为读重读失败次数；`at` 为整帧重测次数；`raw` 为最近成功读回的 6 B（CRC 可仍失败）。
- 只在本周期温湿度失败时打印；光照失败不触发 G（`q` 仍可为 1）。
- 失败周期 `S` 的 `t/h` 允许为最近有效值，但 `q=0`；不得当本次样本。

---

## 9. UART1 与轨迹

### 9.1 行格式与启动顺序（保持）

- `PA06=TXD/PA05=RXD`，9600 8N1；`BOOT→VDD→IOTEST→IOSIG→BUS`；每周期恰一 `S`，温湿度失败追加 `G`；CRLF；含 CRLF ≤96 B；ASCII 整数/大写 HEX。
- 行字段集合不变（IC-003 §9）。`S.p/H` 语义修正为判定前；`S.s/y` 见 §6.2。

### 9.2 有界等待设计

当前 `trace_putc` 的 TXE 等待和 `debug_trace_flush_close` 的 TXE/TXBUSY 等待没有超时；UART 时钟/引脚异常时可能永久阻塞。设计：

1. 在 `sensor_config.h` 增加：
   - `DEBUG_UART_LINE_GUARD`：整行发送的轮询预算（以 8 MHz 下的循环计数表示）。初始设计值 `600000`（按 4 cycles/loop 估算约 300 ms，约为 96 B@9600 理论线时的 3 倍）；实现必须按实际编译循环校准并在配置注释写明，且不得低于 2×96 B@9600 的理论线时。
   - `DEBUG_UART_CLOSE_GUARD`：关闭前排空的预算，初始设计值 `100000`（约 50 ms）；同样按目标实测校准。
2. `debug_trace_*` 每行开始时重置预算；`trace_putc` 在等待 TXE 时递减预算；减到 0 时置内部 `s_tx_abort=1`，本行后续字符不再输出。
3. `debug_trace_flush_close` 用关闭预算等待 TXE/TXBUSY；超时仍执行 UART 复位/关时钟/PA05/PA06 置输入。
4. `s_tx_abort` 只影响本行输出，不改变采样/判定/发送/睡眠；不新增线上字段；下一行重新开始预算。
5. 不使用新定时器/中断；不以 `delay_ms` 忙等；不修改业务状态。
6. 失败表现：可能看到截断行；这是可诊断现象，不是业务结果。独立验证可用 stuck-UART mock 证明有界返回和 UART 关闭。

### 9.3 BOOT 标识

- `sensor_config.h` 定义 `SENSOR_FW_BUILD_ID`（本轮建议 `"RUN10-FDS003"`）；`debug_trace_boot` 打印 `BOOT fw=<ID> uid=<8HEX> rst=<4HEX> uart=9600`。
- 该标识只用于日志区分；**不证明** Flash 镜像身份；验收必须同时记录提交与 axf md5。
- 不改变行字段数量/顺序。

---

## 10. 启动、主循环与睡眠

### 10.1 启动

```text
锁存 rst_flags
SYSCTRL_Configuration / GPIOA/B 时钟 / RTC / UID
debug_trace_boot(uid, rst)
light_read_vdd_mv → VDD 行
sensor_io_diag_scan → IOTEST
sensor_io_sig_scan → IOSIG
sensor_bus_diag_scan → BUS
if (uid_sum == 0):
    report_req = 0; sample_flag = 0
    light_init();                 /* PB05 低、ADC 关，满足睡眠不变量 */
    debug_trace_flush_close()
    while (1) SYSCTRL_GotoDeepSleep()
sample_flag = 1
while (1):
    if sample_flag: sensor_cycle_run()
    debug_trace_flush_close()
    go_to_sleep()
```

- 把 UID 零判断移到 BOOT/诊断之后，保证致命板仍能看到启动/诊断行。
- 致命路径必须真正深睡；不得在 `sample_flag=1` 下循环调用 `go_to_sleep()`（现 D-05）。
- 不恢复 Flash 历史参数/霍尔/光通讯；`have_prev=false`。

### 10.2 主循环

- `sensor_cycle_run()` 内部完成光照/温湿度/判定/冻结/发送/轨迹填充；`main` 只调用一次，然后 `sensor_trace_fetch` + `debug_trace_*` + `flush_close` + `go_to_sleep`。
- 不再依赖多轮主循环完成 RF 重试；`S` 在发送结束后打印。
- `go_to_sleep()` 条件保持 `pending.active==0 && sample_flag==0`；若仍有 pending（防御性）不得睡，但发送循环已保证清除。
- 每周期只消费一次 `sample_flag`；不在发送/打印中再次采样。

### 10.3 睡眠不变量

- PB05 低（`light_sample` 结束已置低）；ADC 关闭；UART1 关闭且 PA05/PA06 输入；I²C 两线开漏释放（OD 高）；RF sleep（发送函数结束已调用）。
- 不复位 GPIOA、不关闭 GPIOA 时钟、不影响 PA03/PA04/PA07/PA08。
- 深睡由 RTC 1 min 中断唤醒；不在深睡期间打印/发射。

---

## 11. 异常矩阵

| 异常 | 固件行为 | 业务结果 | 诊断 | 恢复 |
|---|---|---|---|---|
| GXHT40 无 ACK | 有界重试/重探/总线恢复后放弃 | 本周期无效；不判定/不发送；prev 不变 | `G s=2 a44/a45` | 下周期重探；U01 外部修复 |
| CRC 错 | 整帧重测 3 次 | 同上 | `G s=4 rd/at/raw` | 下周期重试 |
| 温度超域 | 不写输出 | 同上 | `G s=5` | 下周期重试 |
| 读 NACK | 重读 5 次 | 同上 | `G s=3 rd=5` | 下周期重试 |
| 光照 ADC 全失败 | `valid=false,dark=false` | 温湿度仍可；升温关；超温可 | `S V=0 o=0 D=0` | 下周期重试 |
| 未标定 | `valid=false,dark=false` | 升温关；超温可 | `S V=0 D=0` + 原始 `a` | 现场标定 |
| RF 3 次失败 | 本周期结束，清 pending | 无远端保证；`s=2 y=0`（IC-003 语义；尝试次数见 harness） | `S` | 下周期按新判定 |
| UART 超时 | 中止本行，关 UART | 不影响采样/判定/发送/睡眠 | 截断行 | 下周期重新打开 |
| UID 零 | 打印 BOOT/诊断后深睡 | 不采样/不发送 | BOOT uid=00000000 | 硬件/生产处理 |
| 复位/掉电 | `have_prev=false`，pending 清零 | 首样本仅超温可报 | `S H=0` | 正常启动 |

---

## 12. 时序与资源预算

| 项 | 设计值/估算 | 依据 | 验证 |
|---|---:|---|---|
| 采样周期 | 1 min tick ×3 = 180 s 结构 | RTC | PA-007 主机时间戳 |
| 光照 | 100 ms + 8×390 µs ≈ 104 ms | PSPEC/FEX | PA-004 |
| GXHT40 正常 | ~25–30 ms（含 10 ms 等待） | FEX SCL ~4.7 kHz；地址+命令+6 B 读 + 等待 | PA-001/002 |
| GXHT40 最坏 | ~220–250 ms（读/重测上限） | 配置上限；5 次读×3 次重测 + 等待 | PA-003/020 |
| RF | 既有等待预算 ≤3×200 ms = 600 ms；不含 `radio_init` 前置三线配置 | 既有 API；init 开销未测 | PA-010 + 实测 |
| UART | S+G ~150 ms；单行有界预算目标 ~250–300 ms（按实现循环校准） | 9600/96 B | PA-017 |
| 单周期最坏 | 上述合计估算 <1.5 s，但**不含** `radio_init` 未测开销 | 设计估算，不是硬上界 | 实板记录 |
| Flash | 20,860 B / 64 KB（基线）+ <2 KB | FEX 构建 | 新构建记录 |
| RAM | 1,792 B / 4 KB（基线）+ <128 B | FEX 构建 | 新构建记录 |
| 栈峰值 | 未测 | — | 后续构建/测试 |

这些是设计与估算，不是实测最坏执行时间/功耗/寿命；不得写成已验证。

---

## 13. 接口与兼容

| 接口 | 设计处理 |
|---|---|
| `sensor_decide_report` | 保留 4 参签名以兼容既有独立 harness；内部语义按升温修正；产品路径新增 `sensor_gate_decide(sample_valid, prev, have, cur, light_valid, dark)` 做 `sample_valid` 与 `light_valid && dark` 门禁；不改变对外协议。 |
| `measure.h report_req` | 兼容保留为待发活动标志；新增内部 `pending_frame/fail_count/result`；测试可继续观察。 |
| `debug_trace_sample_t` | 字段集合不变；仅修正 `prev_temp_x10/have_prev/retry` 的填充语义。 |
| 433 帧 | 10 B 明文布局/CRC/Feistel 不变；触发时编码一次，重试复用密文。 |
| BLE/网关/App/云 | 不修改；本设计不新增字段/消息。 |
| 光照配置 | `C_dark/Calibrated` 仍为唯一配置点；不新增 NVM/远程标定。 |
| UART | 行格式/引脚/波特率不变；仅增加内部有界等待与 BOOT 标识。 |

> 新增 `sensor_gate_decide` 包装属固件内部变化；4 参 `sensor_decide_report` 保留，独立 harness 不需改签名即可继续编译/更新期望；`S.y` 保持 IC-003 §9 语义，不改字段含义。若接口责任要求调整 y，应在实现前同步，不得让两种含义并存。

回滚：仅回退源码/配置提交并重新构建；无 NVM 参数、数据库迁移、协议字段或网关/App 变更需要回退。标定常量未写入设备，回退后按旧配置/未标定状态运行。

---

## 14. 测试与交付边界

### 14.1 实现侧（本设计交付后由实现能力完成）

- 纯逻辑 14 向量（含旧实现 3 项失败的负向对照）。
- `measure.c` 流程：判定前快照、成功推进/失败不推进、单周期发送 3 次、帧字节冻结、成功/用尽清除、不跨周期。
- `debug_trace`：p/H 语义、S/G 行长、stuck-UART 有界返回、关闭后引脚输入。
- `light`：未标定门禁、全暗/有光/边界、ADC 全失败。
- RF：同一帧字节在 3 次尝试中不变；成功/失败计数；不重复编码。
- 构建：Keil rebuild 0 Error（仅既有告警），记录 md5/尺寸。
- 静态：无 hall/OPTCFG/params/history 有效路径；无新增 NVM/协议字段/心跳/额外采样。

### 14.2 不由本设计承担

- 真实 GXHT40 ACK/连续 `q=1`：依赖 U01，硬件/嵌入式测试在授权下完成。
- 全暗/有光真实标定：现场窗口；固件只提供记录与回填流程。
- RF→网关→BLE→App→云：独立测试/运维；本设计不宣称链路通过。
- 180 s 物理容差、深睡电流、电池峰值：测试/硬件测试。
- 独立验证 harness（`*_verify_ev.c`）由测试能力维护；实现不得改其断言。

### 14.3 交付判定

- 本设计完成仅代表方案/需求/架构/任务可进入实现。
- 实现完成不等于验收；PACC-002 的 PA-001..031 由独立验证按当前版本执行。
- U01/U02 未关闭时，相关 P0 真实采集/升温分支只能如实报告未验证，不得用宿主机/mock 替代。

---

## 15. 任务映射（详见 `firmware_tasks_v3.yaml`）

| 任务主题 | 覆盖设计 | 依赖 |
|---|---|---|
| 1 门控纯逻辑 + 14 向量 | DD-01、§5 | 无 |
| 2 判定前快照 + 轨迹 | DD-02、§6、§9 | 任务 1 |
| 3 冻结帧 + 单周期发送 | DD-03/04/05/11、§6 | 任务 1/2 |
| 4 UART 有界等待 | DD-07、§9 | 无 |
| 5 主循环/启动/致命路径 | DD-03/08、§10 | 任务 2/3/4 |
| 6 标定配置/编译期校验 | DD-06、§7 | 无 |
| 7 实现侧测试更新 | §14.1 | 任务 1–6 |
| 8 构建/静态核对 | §12/§14.1 | 任务 1–7 |

> 现场全暗 `C_dark` 采集/回填依赖 U02 的现场窗口，不列入本次可执行实现任务；任务 6 只提供配置与编译期校验，现场数据到位后按 §7.3 回填并重建。

任务是实现增量，不是测试计划；独立验证顺序/方法由测试能力决定。

---

## 16. 未决与交接

| ID | 未决 | 本设计的处理 | 交接 |
|---|---|---|---|
| U01 | GXHT40 无 ACK 根因 | 不改驱动；保留失败路径；真实采集不伪造 | 硬件/嵌入式测试（通断/电压/换件） |
| U02 | `C_dark`/亮暗可分离 | 流程与编译期校验；不可分离不标定 | 现场/运维 + 产品决定 |
| U03 | 180 s 容差 | 保持 tick 结构；不宣称 ±10% 已批准 | 测试/产品 |
| U04 | 网关/App/云真实链路 | 不在固件设计内 | 测试/运维 |
| U05 | 生产 UART 策略 | 默认开；关闭不影响业务 | 产品 |
| U06 | 诊断/接口文字（y、p） | 本设计明确 p=判定前；y 保持 IC-003 现行语义（成功/用尽后 0），重试次数用 harness 证据；SARCH AD09 的“尝试”表述与 IC-003 存在差异，建议接口责任统一 | 接口责任 |
| U07 | 433 发送总耗时 | `app_um2005C_send_data_timeout` 的 200 ms 在 `radio_init` 之后计时，init/三线配置开销未测；3×200 ms 不是完整上界 | 保留既有 API；实板/逻辑仪测 init+发送；不改协议 | 独立测试/实测 |

---

## 17. 完成声明

本文件完成固件方案设计：模块/任务划分、数据流、时序/资源预算、接口处理、异常策略、标定与诊断设计、实现约束、任务映射和未决项。**未修改任何产品源码，未烧录/复位/采设备，未执行真实功能测试**。后续实现/独立验证必须使用当前版本证据，不得引用本设计或旧运行结论替代。

---

## 18. 本次核对

- 已读取本轮 PDR-001、PSPEC-001、PACC-002、IC-003、FEX-001、SA-001、SARCH-001、SD-001 及当前 `USER/` 源码与测试资产；仓库无 `.codegraph/`，使用限定目录 `rg`/`Read`，未建索引。
- 已核对 `firmware_tasks_v3.yaml` 可解析为仅含 `items` 的 8 条字符串，未写入 Runtime ID/状态/计数器。
- 本能力开始前 `git status` 干净；本轮只新增设计/需求/架构/任务工件并同步 `file_manifest.txt`，未修改产品源码、测试源码、网关、Android、服务器或配置。
- 本次未执行构建、烧录、复位、串口/逻辑采集、手机/网络/SQL 操作；没有把任何设计或估算写成实测通过。
