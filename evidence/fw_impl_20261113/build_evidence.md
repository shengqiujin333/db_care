# 构建证据（BUILD-004：无改动重建确认）

负责角色：固件工程
时点：2026-11-13（Asia/Shanghai；工具时间戳 UTC）
范围：传感器工程 `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output` 在**产品源零改动**条件下的重建确认；用于本轮"当前实现无需修改"的交付。**不证明实板功能。**

依据：`artifacts/firmware_design.md`（FD-002 rev 8）、`artifacts/firmware_implementation.md`（FWI-004 rev 9）、`artifacts/test_design.md`（TD-002 rev 9.0）、`evidence/product_acceptance_20261112_scope_recheck/PRODUCT-ACCEPTANCE-RECHECK-002.md`、`evidence/product_acceptance_20261112/FIRMWARE-TRIAGE-003.md`。

---

## 1. 源身份

| 项 | 值 |
|---|---|
| 仓库 HEAD | `ef08364`（接手时 `git status` 仅本轮证据目录未跟踪） |
| 传感器产品源提交 | `82a7e74e5d6bae855a573e1f23f83936ba8db049`（FWI-003 rev 8） |
| `82a7e74..HEAD` 对 `sensor/{USER,MDK,EWARM,UM2005C,COMMON}` 的差异 | **空**（本轮实际执行 `git diff --stat`，无输出） |
| 工程文件 | `MDK/Project.uvprojx`（未改；md5 `35b9c3f4f190c623c1e4f666c21d18a4`） |
| 工程源列表 | 27 个源文件；**不含**任何 `test/` 或 `mock/` 路径（见 `md5_list.txt` 与 §4） |

## 2. 构建

| 项 | 结果 |
|---|---|
| 工具 | `mdk_build`（Keil MDK / ARMCLANG **V6.24**，工程保存的 Device/优化/调试器配置） |
| 命令 | `mdk_build {"action":"rebuild"}`（参数文件 `args/mdk_build_nocode_check.json`） |
| 时间（UTC） | 2026-11-13T03:37:52.939 – 03:37:55.483 |
| 结果 | **0 Error / 1 Warning**（既有 `main.c(254) while(k--);` 空循环体，非本轮引入） |
| Program Size | `Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 产物 | `MDK/output/exe/Project.axf` md5 `b6a15fed2ed5e3d0c4adc51ac87a4830`（297,380 B）；`Project.hex` md5 `4d728dec9b0d28a352c46436b7586c72`（75,275 B） |

原始输出：`mdk_build_nocode_check.out.json`、`mdk_build_nocode_check.err.txt`（空）。

## 3. 与交付包的同一性

| 项 | 值 |
|---|---|
| 发布包 | `fw_release/fw_sensor_FD-002r8.zip` 134,791 B / md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf` |
| 包内 `PACKAGE.txt` | 3,202 B / md5 `a58f67fcf5b760767ff5b65888c73dfb` |
| 包内 axf / hex | `b6a15fed…` / `4d728dec…`，与本轮重建**逐字节一致**（同摘要即同一内容） |
| 结论 | 本轮重建未产生新身份；无需重发发布包；`docs/02`、EV-036/037/038 与产品验收报告对包摘要的引用继续有效 |

## 4. 测试资产与正式固件的隔离（本轮未新增任何测试代码）

| 检查 | 结果 |
|---|---|
| 工程源列表是否含 `test/`、`mock_*` | **否**（27 项全部为 `USER/`、`COMMON/`、`UM2005C/`、厂商库与启动文件） |
| 本轮是否新增测试专用文件/符号/编译定义 | **否**（`git status` 无产品源码改动；无新增 `.c/.h`） |
| 正式镜像是否含测试开关 | 否：交付构建仅由工程保存配置生成，测试 harness 仅在宿主机 `test/build_test.sh` 中编译，不参与 Keil 链接 |
| 正式镜像可达性佐证 | 正式 axf 摘要与包内成员一致（§3），且宿主自检单独构建（不注入正式镜像） |

## 5. 边界

1. 未烧录、未复位、未采集串口；板上镜像身份不由本文件证明（EV-036/037/038 由测试角色以同摘要完成下载校验，属其证据）。
2. 构建成功与摘要一致只说明"源—包"身份闭合，不说明需求功能通过；未执行项见 `artifacts/firmware_implementation.md` FWI-004 §3 与 `evidence/fw_impl_20261112/assignment_referral.md` §4。
3. 未测功耗、未调温、未改变光强；无跨链路与平台写入。
