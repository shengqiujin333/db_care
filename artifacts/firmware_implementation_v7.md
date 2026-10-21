# 固件实现记录（FWI-007，ITEM-005）

状态：`IMPLEMENTED`（仅 ITEM-005 主循环单次事务、BOOT/UID 零致命路径；**未烧录、未实板验证、未产品验收**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `847b230`（ITEM-004 已提交；本 item 开始时工作区干净）
实现范围：`artifacts/firmware_tasks_v3.yaml` 第 5 项
设计依据：FDS-003 DD-03/08、FWR-301/311、IC-003 IF08、PACC-002 PA-017/PA-019
构建证据：`evidence/ev024_item005/build.md`
自测证据：`evidence/ev024_item005/driver_test.md`

---

## 1. 本次改动

| 文件 | 改动 |
|---|---|
| `CW32.../USER/src/main.c` | 删除 UID 和为零时 `while(1){ go_to_sleep(); }` 空转路径；BOOT/VDD/IOTEST/IOSIG/BUS 保持在 UID 判断之前 |
| `CW32.../USER/src/main.c` | UID 零：`report_req=0`、`sample_flag=0`、`light_init()`（PB05 低/ADC 关）、`debug_trace_flush_close()`，然后 `while(1) SYSCTRL_GotoDeepSleep()` |
| `CW32.../USER/src/main.c` | 主循环只在 `sample_flag != 0` 时执行一次 `temperature_process()` → `send_data_to_gateway()` → 唯一 S（失败 G）；随后关闭 UART、`go_to_sleep()` |

未修改：采样/判定/发送函数、UART 实现、标定、协议、引脚、硬件、网关、Android、服务器。未新增 GPIOA 复位/关闭时钟。

完整源码 SHA-256：`evidence/ev024_item005/source_sha256.txt`（main.c 与静态检查脚本）。
构建产物 md5：axf `e0437fd3405531f107bec844d815acf6`，hex `c17f42eba0c1c38461c3768c99e68f22`。

---

## 2. 行为实现

### 2.1 UID 零致命路径

```c
/* BOOT/VDD/IOTEST/IOSIG/BUS 已先执行 */
if (k == 0u) {
    report_req = 0u;
    sample_flag = 0u;
    light_init();                  /* PB05 低 + ADC 配置后关闭 */
    debug_trace_flush_close();
    while (1) SYSCTRL_GotoDeepSleep();
}
```

- BOOT/诊断在致命分支之前；UID 零仍可看到启动/诊断行。
- 不进入采样/发送；`sample_flag=0` 保证 `go_to_sleep()` 空转条件不再成立；使用直接深睡循环。
- PB05 低/ADC 关由 `light_init()` 完成；UART 关闭由 `debug_trace_flush_close()` 完成。

### 2.2 单次事务主循环

```c
while (1) {
    if (sample_flag != 0u) {
        temperature_process();       /* 光照→温湿度→判定→冻结 */
        send_data_to_gateway();      /* 内部 1..3 次，成功/用尽清 pending */
        /* debug: 唯一 S（失败 G），发送结束后 */
    }
    debug_trace_flush_close();
    go_to_sleep();
}
```

- 每个采样机会只执行一次完整事务；不再依赖多轮主循环完成 RF 重试（ITEM-003 已把 3 次重试放入 `send_data_to_gateway` 单次调用）。
- S 在发送结束后打印一次；失败周期由 trace 追加 G。
- `go_to_sleep()` 仅在 `report_req==0 && sample_flag==0` 时深睡。
- RTC 1min×3 节拍、`sample_flag` 消费、GPIOA/I²C/SWD 边界未改。

---

## 3. 验证

### 3.1 源码结构自检

`evidence/ev024_item005/main_static_check.py`：**16/16 PASS**。
- BOOT 在 UID 致命判断前；
- UID 块清两标志、`light_init`、关 UART、深睡，不含采样/发送；
- 主循环由 `sample_flag` 门控并含 temp/send/trace/close/sleep；
- 旧 UID 空转循环不存在；main 无 GPIOA 复位调用。

### 3.2 RTC 回归

测试能力 `host_rtc_cadence_verify_ev.c` + `mock_main_ev/`（未修改）配临时外部 stub 后：**17 passed / 0 failed**（`rtc_verify.log`）。验证 1min×3、计数回绕、30 tick 10 次机会。

### 3.3 全量宿主机自检

`test/build_test.sh` 5 组：**233 passed / 0 failed**（`host_tests_full.log`）。

### 3.4 构建

- `mdk_build build`/`rebuild` 0 Error；唯一告警为既有 `main.c(252) while(k--);`。
- Code=19400，RO=644，RW=116，ZI=1684。
- 未烧录、未复位、未连接 COM43；**不宣称实板通过**。

---

## 4. 未完成/交接

| 项 | 状态 |
|---|---|
| UID 零/单次事务行为级验证 | 由独立测试 T-L0-19/T-L0-20 执行；本实现只有静态结构自检 |
| 测试能力 RTC harness | 需补 `report_req` 数据定义与 `light_init()` 桩以适配新 main；本轮未修改测试资产 |
| ITEM-006 标定配置 | 未实现 |
| 实板启动/节拍/深睡 | 未执行 |

## 5. 结论

ITEM-005 的 BOOT 先于 UID 致命判断、UID 零不采样/不发送/真正深睡、每采样机会一次完整事务/唯一 S、RTC 1min×3 与引脚边界保持已实现；构建 0 Error，RTC 回归 17/0、全量宿主机 233/0、源码结构 16/16。结论仅覆盖该 item；实板、UID 零真实行为、端到端与产品验收未通过。
