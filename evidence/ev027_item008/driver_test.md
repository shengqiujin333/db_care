# Driver/Logic Self-Test and Static Audit Evidence — ITEM-008

状态：实现侧最终静态核对/自检（**不表示实板通过**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
源码基线：`8ae4a33`（原始固件）→ 当前 HEAD `90974f0` + ITEM-008 工作树
编译器：MinGW-w64 gcc 12.2.0；Keil ARMCLANG V6.24

## 1. 静态核对（12/12 PASS）

脚本：`static_audit.py`；原始输出：`static_audit.txt`。

| 检查 | 结果 |
|---|---|
| 无 hall/OPTCFG/params/history 有效调用（注释已剥离） | PASS |
| 无 NVM/Flash 参数写 | PASS |
| `sample_flag=1` 仅初值与 RTC 节拍 | PASS |
| `temperature_process()` 在 USER 中仅主循环 1 处调用 | PASS |
| `send_data_to_gateway()` 在 USER 中仅主循环 1 处调用 | PASS |
| USER 代码无 GPIOA 复位 | PASS |
| USER 代码无 GPIOA 时钟关闭 | PASS |
| 新增代码行无 GPIO/NVIC/RTC 配置 | PASS |
| 网关固件无本轮改动 | PASS |
| RF/encrytogate/UM2005C 无改动 | PASS |
| light/gxht40/sf_i2c 行为无改动 | PASS |
| S/G 最坏行 ≤96 B | PASS |

产品源码相对 `8ae4a33` 的改动仅：`fw_core.h`、`sensor_config.h`、`debug_trace.c`、`fw_core.c`、`main.c`、`measure.c`（见 `static_audit.txt`）。没有新增 RF/BLE/上传字段、心跳、额外采样或引脚/中断配置。

## 2. S/G 行预算

`host_debug_trace_check` 实测：
- worst S = **67 B**（含 CRLF）
- worst G = **46 B**（含 CRLF）
均 ≤ `SENSOR_DEBUG_UART_MAXLINE=96`。

## 3. 实现侧全量自检

`test/build_test.sh` 7 组：**295 passed / 0 failed**（`host_tests_full.log`）：
57（fw_core 14 向量/网格/负向）+ 52（pure check）+ 72（measure/p/H/冻结）+ 51（UART/BOOT/卡死）+ 28/29（light 未标定/标定）+ 6（标定编译门禁）。

## 4. 构建

`mdk_build rebuild`：0 Error，唯一既有 `main.c(252)` 空循环告警；Code=19544 RO=644 RW=116 ZI=1684。
完整日志/尺寸：`build_rebuild.log`、`build.md`。

## 5. 限制与交接

- 全部为源码静态/宿主机自检，不是实板、独立验证或产品验收；未烧录/未采集 COM43。
- U01 GXHT40 ACK、U02 全暗 C_dark 仍未闭合，真实目标/端到端不能据此宣称通过。
- 测试能力旧独立 harness（`host_diag_line_verify_ev.c`、`host_measure_flow_verify_ev.c`）仍待其按 `test_design_v3.md` 更新；本轮未修改。
