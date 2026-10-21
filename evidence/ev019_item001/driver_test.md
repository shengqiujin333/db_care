# Driver/Logic Self-Test Evidence — ITEM-001

状态：实现侧局部自测（宿主机纯逻辑），**不表示实板通过**
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
被测源码：当前工作树 `USER/src/fw_core.c`（基线 `8ae4a33` + ITEM-001 改动）
编译器：MinGW-w64 `gcc 12.2.0`（`-std=c11 -Wall -Wextra`）

## 1. 产品路径与兼容入口

```c
bool sensor_gate_decide(bool sample_valid,
                        int16_t prev_x10, bool have_prev,
                        int16_t cur_x10,
                        bool light_valid, bool dark);
bool sensor_decide_report(int16_t prev_x10, bool have_prev,
                          int16_t cur_x10, bool dark);
```

- 产品路径只经 `sensor_gate_decide`：`sample_valid=false` 直接 false；否则用 `light_valid && dark` 调用 4 参函数。
- 4 参函数：`cur>350` 独立 true；恰好 350 不短路；`!have_prev`/`!dark` false；`(int32)cur-prev>9` true。
- 常量：`SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350`。

## 2. 14 规范向量（FDS-003 §5.2 / PDR PR-004）

原始输出：`evidence/ev019_item001/gate_vectors.log`（`host_sensor_core_test`）。
结果：**14/14 PASS**；v3/v6/v7 分别为 1/0/1，v14（sample_valid=0）为 0。

| # | sample_valid | have_prev | prev | cur | light_valid | dark | 结果 |
|---:|---|---:|---:|---:|---|---|---:|
| 1 | 1 | 1 | 300 | 308 | 1 | 1 | 0 |
| 2 | 1 | 1 | 300 | 309 | 1 | 1 | 0 |
| 3 | 1 | 1 | 300 | 310 | 1 | 1 | **1** |
| 4 | 1 | 1 | 300 | 310 | 1 | 0 | 0 |
| 5 | 1 | 1 | 300 | 310 | 0 | 0 | 0 |
| 6 | 1 | 1 | 310 | 300 | 1 | 1 | 0 |
| 7 | 1 | 1 | 340 | 350 | 1 | 1 | **1**（升温分支） |
| 8 | 1 | 1 | 340 | 350 | 1 | 0 | 0 |
| 9 | 1 | 0 | 0 | 351 | 0 | 0 | 1（超温） |
| 10 | 1 | 0 | 0 | 350 | 1 | 1 | 0 |
| 11 | 1 | 0 | 0 | 360 | 0 | 0 | 1（超温） |
| 12 | 1 | 1 | 352 | 351 | 0 | 0 | 1（仍 >35.0） |
| 13 | 1 | 1 | 300 | 310 | 0 | 0 | 0（ADC 全失败） |
| 14 | 0 | 0 | — | — | — | — | 0（不判定） |

## 3. 结构化网格与 4 参兼容

- `sample_valid∈{0,1} × have_prev∈{0,1} × light_valid∈{0,1} × dark∈{0,1} × prev/cur∈{-400,-1,0,9,349,350,351,1250,32767,-32768}` 共 1,600 点，与独立参考 `ref_gate` 逐点一致（`gate_vectors.log` `[6b]`）。
- 4 参兼容逐项：rise 1.0+dark true；rise 1.0+lit false；340→350 dark true；无前值 350 false；无前值 351 true；rise 0.9 false；drop false（`[6c]`）。
- `int32` 差值覆盖 `32767 - (-32768)`，无溢出。

## 4. 汇总自测

| 测试 | 结果 | 原始输出 |
|---|---:|---|
| `host_sensor_core_test.c`（T-L0-05/07/08 + 既有 CRC/换算/光照） | **53 passed / 0 failed** | `gate_vectors.log` |
| `host_fw_core_pure_check.c`（14 向量 + 4 参兼容） | **52 passed / 0 failed** | `pure_check.log` |
| `host_measure_flow_check.c`（门控调用点/失败不推进/发送路径） | **43 passed / 0 failed** | `measure_flow.log` |
| `test/build_test.sh` 全量（5 组） | **197 passed / 0 failed** | `host_tests_full.log` |

命令（在本目录）：

```sh
gcc -std=c11 -Wall -Wextra -I../USER/inc host_sensor_core_test.c ../USER/src/fw_core.c -lm -o core_check.exe && ./core_check.exe
gcc -std=c11 -Wall -Wextra -I../USER/inc host_fw_core_pure_check.c ../USER/src/fw_core.c -lm -o pure_check.exe && ./pure_check.exe
sh build_test.sh
```

## 5. 限制与交接

- 所有结果为宿主机 `fw_core`/mock 自测，不是实板、独立验证或验收通过；未烧录/未接目标。
- `host_measure_flow_check.c` 的判定输入已按升温调整以保持实现侧自检一致；它不覆盖 ITEM-002 的判定前快照、ITEM-003 的冻结发送/单周期重试、ITEM-004 的 UART 有界等后续增量。
- 测试能力拥有的 `host_fw_core_verify_ev.c`、`host_measure_flow_verify_ev.c`、`host_rf_report_verify_ev.c` 仍按旧降温/旧常量编写；其中 `host_measure_flow_verify_ev.c` 引用已删除的 `SENSOR_REPORT_DROP_X10`，需由测试能力按本轮测试设计重写（不由本实现修改）。旧 harness 的通过/失败不作为本轮门控结论。
