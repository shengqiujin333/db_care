# 嵌入式验证证据 EV-027 — ITEM-007 实现侧自检资产同步

状态：`TEST_PASS`（**仅本增量**：ITEM-007 实现侧自检资产同步；宿主机验证，未烧录/未实板）
能力：`embedded_tester.embedded_verification`
项目：dengbei_run10
受测提交：Git `baf9d0f4c2e5c553e6bfc76cba1ad1baa2b39751`（工作区在本次取证前干净）
被测增量：`artifacts/firmware_tasks_v3.yaml` 第 7 项 / `firmware_implementation_v9.md`
测试设计：`artifacts/test_design_v3.md` T-L1-01、T-L9-05/06（ITEM-007 行）
上游实现证据：`evidence/ev026_item007/{build.md,driver_test.md,host_tests_full.log}`
本目录原始证据：

| 文件 | 内容 |
|---|---|
| `run_context.txt` | UTC/本地时间、Git HEAD、gcc 12.2.0、宿主环境 |
| `source_sha256.txt` | 3 个自检资产、UART/光照/门禁资产与 4 个产品源码的 SHA-256 |
| `commit_files.txt` | 受测提交实际改动文件清单 |
| `host_tests_run1.out` / `host_tests_run2.out` | 本能力独立执行 `test/build_test.sh` 的完整原始输出（含退出码） |
| `run_host_suite.sh` | 上述执行的唯一入口包装（原样调用被测 `build_test.sh`） |
| `static_check_item007.py` / `static_check.out` | 独立静态核对脚本与原始输出（42 passed / 0 failed / 1 warning） |
| `negative_controls_item007.py` / `negative_controls.out` | 5 组“错误实现”变异负向对照与逐条 FAIL 原始输出 |
| `negctl/` | 仅用于负向对照的产品源码**副本**（非产品源码，不参与构建） |

---

## 1. 受测身份与范围

- 受测对象是**实现侧宿主机自检资产**：`test/build_test.sh`、`test/host_sensor_core_test.c`，以及其调用的 `host_fw_core_pure_check.c`、`host_measure_flow_check.c`、`host_debug_trace_check.c`、`host_light_check.c`、`check_calib_config.sh`。
- 本提交实际只改了 `test/build_test.sh` 与 `test/host_sensor_core_test.c`；`USER/**` 产品源码、Keil 工程、协议、引脚均未改（`commit_files.txt`、§4-E）。
- 上一项 ITEM-006 已验证的产品镜像未被本提交改变：`Project.axf` md5 `9cb97918e8832811dfcb00a93646fba1`、`Project.hex` md5 `3af11e7adb21ba43004fc827649adb44`，与实现记录一致。
- **本项没有新增板级行为**，验收行为本身是“自检能判错、当前实现能通过”，因此证据以宿主机为限。未烧录、未复位、未连接 COM43/逻辑分析仪；不据此宣称任何实板或端到端通过。

## 2. 动态验证：当前实现通过（T-L1-01）

`test/build_test.sh` 由本能力独立执行两次，均退出码 0，输出逐字节相同：

| 组 | run1 | run2 |
|---|---:|---:|
| [1/5] host_sensor_core_test（14 向量/网格/旧方向负向对照） | 57 / 0 | 57 / 0 |
| [1b/5] host_fw_core_pure_check（14 向量/4 参/C_dark 全量） | 52 / 0 | 52 / 0 |
| [2/5] host_measure_flow_check（p/H、冻结帧、3 次、不跨周期） | 72 / 0 | 72 / 0 |
| [3/5] host_debug_trace_check（UART 有界/行长/BOOT） | 51 / 0 | 51 / 0 |
| [4/5a] light 未标定变体 | 28 / 0 | 28 / 0 |
| [4/5b] light 标定 C_dark=3000 变体 | 29 / 0 | 29 / 0 |
| [5/5] 标定配置编译门禁 | 6 / 0 | 6 / 0 |
| **合计** | **295 / 0** | **295 / 0** |

实现侧 `evidence/ev026_item007/host_tests_full.log` 的 393 行正文与本次 run1 完全一致；run1 仅多出本能力追加的 `EXIT_CODE=0` 一行。

## 3. 动态验证：错误实现会被判失败（验收行为）

没有依赖实现侧内置的 `legacy_drop` replica 自证。本能力对**当前产品源码副本**施加 5 组定向变异，再用同一批实现侧自检 harness 编译运行；每组均被判失败：

| 变异（仅副本） | 自检 harness | 结果 | 关键 FAIL |
|---|---|---|---|
| N1 门控改回 `prev-cur > 9` 且 `cur==350` 整体排除 | `host_sensor_core_test` | 退出 1 | v3/v6/v7 三条不符；网格/4 参同样失败 |
| N1 同上 | `host_fw_core_pure_check` | 退出 1 | v3/v6/v7；`rise 1.0C`、`exact 35.0C`、`drop` |
| N2 轨迹 `p` 读取推进后的 `s_prev_temp_x10` | `host_measure_flow_check` | 退出 1 | `A 周期 p/H 为判定前值 (160/1)`、`C 周期 p/H ... 不是推进后的 180` |
| N3 每次重试重新 `encode_frame10` | `host_measure_flow_check` | 退出 1 | `发送路径不重新编码`、`3 次发送不重新编码`、`成功路径不重新编码` |
| N4 TXE/TXBUSY 预算放大到实际无界 | `host_debug_trace_check` | 退出 1 | `TXE 等待不超过 DEBUG_UART_LINE_GUARD`、`TXBUSY 等待不超过 DEBUG_UART_CLOSE_GUARD` |
| N5 未标定时仍 `valid=adc_ok` | `host_light_check`（未标定变体） | 退出 1 | `uncalibrated: adc_ok=true but valid=false`、`uncalibrated: dark=false even at full-scale reading` |

结论：五个要求类别（14 向量/升温、判定前 prev、冻结帧/单次编码、UART 有界、未标定门禁）都能被对应的实现侧自检**实际判失败**；当前实现 295/0 通过，符合“错误实现会被自检判失败，修正实现通过”。

## 4. 静态核对（`static_check_item007.py`，42/0，1 warning）

- 两个 14 向量套件的前 14 条与已独立验证的冻结表（EV-020 harness）逐条相同；v3=1、v6=0、v7=1、v8=0、v14 不判定。
- `host_sensor_core_test.c` 的旧方向语义只存在于显式 `legacy_drop` 负向对照中，且断言其恰好失败 3/13；`host_fw_core_pure_check.c`、`host_measure_flow_check.c` 无旧方向/35.0 整体排除期望。
- `host_measure_flow_check.c` 含判定前快照、失败不推进、冻结帧逐字节、单次编码、3 次用尽清除、不跨周期、`S.s=2/S.y=0`。
- `host_debug_trace_check.c` 含 T14/T15 TXE/TXBUSY 卡死预算；`host_light_check.c` 含未标定 valid/dark=false 与标定 1000/999 边界。
- `build_test.sh` 单一入口确实执行全部 7 组（含 `host_fw_core_pure_check` 与 `check_calib_config.sh`）。
- 产品源码 `fw_core.c`/`measure.c` 的升温比较、判定前快照、冻结发送缓冲与自检断言一致。
- 受测提交未修改任何 `*_verify_ev.c`，也未修改 `USER/src/**`。

## 5. 差异与限制（如实登记）

1. **实现证据文字与执行序列不符（不影响本项判据）**：`host_measure_flow_check.c` 第 [14] 段标题写“成功 A -> 成功 B -> 失败 -> 成功 C”，实际执行序列是 **A → 失败 → C（没有 B 周期）**；`driver_test.md` 的“B p=A、C p=B”没有对应断言。判定前快照覆盖本身成立（A 周期 `p=160` 而 `t=170`；失败周期保持 `p=170`；C 周期 `p=170`），因此不构成本项失败，但该段文字应更正。
2. **测试能力旧 harness 未更新**：`host_diag_line_verify_ev.c` 仍硬编码旧 BOOT 标识，`host_measure_flow_verify_ev.c` 仍为旧降温/跨轮模型。本项明确要求不改测试能力 `*_verify_ev.c`，故未修改，也未用它们支撑本项结论；属后续测试资产维护项。
3. **宿主机结论边界**：295/0 只证明纯逻辑/流程/影子 UART 层；不证明真实 I²C、空口、功耗、实板时序。产品镜像与本项提交无关且未重新烧录。
4. **C_dark 仍未标定**：交付配置 `LIGHT_DARK_CALIBRATED=0`，实板“无光”判定仍不可用；自检只证明未标定时不伪造暗态。

## 6. 判定

本增量范围内：实现侧自检资产已同步到 14 向量/升温、判定前 prev、冻结帧/3 次、UART 有界、未标定/标定边界；当前提交执行 295/0；5 组错误实现均被对应自检判失败；提交未触碰产品源码与 tester harness。

**ITEM-007 本增量 TEST_PASS。** 该结论不替代实板、端到端或产品验收结论。
