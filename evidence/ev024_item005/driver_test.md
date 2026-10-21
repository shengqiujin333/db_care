# Driver/Logic Self-Test Evidence — ITEM-005

状态：实现侧自检（**不表示实板通过**；未烧录/未接目标）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
被测源码：当前工作树 `USER/src/main.c`（父基线 `847b230` + ITEM-005 改动）
编译器：MinGW-w64 gcc 12.2.0（静态检查/RTC 回归）；Keil ARMCLANG V6.24（构建）

## 1. 源码结构自检

脚本：`evidence/ev024_item005/main_static_check.py`，输出 `main_static_check.txt`。
结果：**16/16 PASS**。

覆盖：
- `debug_trace_boot` 位于 `if (k == 0u)` 之前；
- UID 零块清 `report_req/sample_flag`、调用 `light_init()`、`debug_trace_flush_close()`、`SYSCTRL_GotoDeepSleep()`，不含 `temperature_process`/`send_data_to_gateway`；
- 主循环由 `sample_flag` 门控，包含 temperature→send→trace→close→sleep；
- 旧 UID 空转循环不存在；main 不调用 `SYSCTRL_AHBPeriphReset`。

## 2. RTC 节拍回归

测试能力资产 `host_rtc_cadence_verify_ev.c` + `mock_main_ev/` 未修改；因 main 新增 UID 路径引用 `report_req`、`light_init`，本轮用**临时外部 stub**（位于仓库外，未入库）完成链接：

```text
host_rtc_cadence_verify_ev.c  17 passed / 0 failed
```

原始输出：`rtc_verify.log`。覆盖：RTC 1 min 配置、3 tick 一次机会、计数 0..2 回绕、30 tick 10 次机会。

> 这是回归证据，不是 UID 零/单次事务的行为级验证。

## 3. 全量宿主机自检

`test/build_test.sh` 5 组：**233 passed / 0 failed**（原始输出 `host_tests_full.log`）。

## 4. 限制与交接

- UID 零“不采样/不发送/不空转”和“每机会一行 S”的行为级验证属于测试设计 **T-L0-19/T-L0-20**，由独立测试执行；本实现只做源码结构自检 + RTC 回归，不宣称独立通过。
- 测试能力 `host_rtc_cadence_verify_ev.c` 需要补 `report_req` 数据定义与 `light_init()` 桩，才能直接链接新 main；本轮未修改该测试资产。
- 未烧录、未采集 COM43；未验证实板启动/深睡/节拍。
