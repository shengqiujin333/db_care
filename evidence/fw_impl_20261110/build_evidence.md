# 构建证据（BUILD-003）

负责角色：固件工程
时点：2026-11-10（Asia/Shanghai；工具时间戳为 UTC）
基线仓库 HEAD：`ef5d1af`（接手时 `git status --porcelain` 为空）
范围：传感器工程 `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output` 的「as-built 基线重建」与「LIGHT-DEF-2 判据形制落地后重建」两次构建，外加 GNU 交叉编译验证。**只证明可构建与产物身份，不证明实板功能。**

依据：`artifacts/firmware_design.md`（FD-002 rev 8 §3.2/§6.2/§12）、`artifacts/firmware_requirements.md`（FWR-002 rev 8）、`artifacts/test_design.md`（MCH-07/ASB-10）。

---

## 1. 工具与工程配置

| 项 | 值 | 来源 |
|---|---|---|
| 构建工具 | `mdk_build`（受审计 tool_cli → Keil MDK） | 当前能力唯一授权设备工具 |
| 编译器 | ARMCLANG **V6.24**（`C:\Keil_v5\ARM\ARMCLANG\Bin`） | 构建日志首行 |
| 工程文件 | `MDK/Project.uvprojx`（工程保存的 Device/优化/调试器配置，未改） | 环境登记 `Keil工程` |
| 目标 | CW32L010（工程 Device 字段 `CW32L010F8`；BOM 为 `CW32L010Y8M6`，差异保留不擅改） | `runtime/environment.yaml`、`MAIN_BOARD.BOM` |
| 交叉编译验证 | `gcc/build.sh` + `arm-none-eabi-gcc`（`C:\ProgramData\chocolatey\bin\arm-none-eabi-gcc.exe`） | 本轮实际执行 |

命令（两条均以工程为 cwd 由工具执行）：

```text
tool_cli.py --tool mdk_build --arguments-file evidence/fw_impl_20261110/args/mdk_build_baseline.json
tool_cli.py --tool mdk_build --arguments-file evidence/fw_impl_20261110/args/mdk_build_lightdef2.json
```

原始输出：`mdk_build_baseline.out.json`、`mdk_build_lightdef2.out.json`、`mdk_build_*.err.txt`（空）、`gcc_build_baseline.log`、`gcc_build_lightdef2.log`。

## 2. 第一次构建：as-built 基线（未改源码）

| 项 | 结果 |
|---|---|
| 动作 | `{"action":"rebuild"}` |
| 时间（UTC） | 2026-11-10T08:36:23.728 – 08:36:26.165（第二遍 08:37 复跑同结果） |
| 结果 | `0 Error(s), 1 Warning(s)`（既有 `main.c(254) while(k--);` 空循环体） |
| Program Size | `Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| `Project.axf` md5 | `a3b08774e764f174eac318f6771d0c42` |
| `Project.hex` md5 | `546ce20a718fa69b40177cce0145f4b5` |

**判读**：与 2026-11-09 EV-030 交付镜像、以及 `artifacts/firmware_design.md` §1.1 记录的历史摘要**逐字符一致** ⇒ 本轮起点源码与历史交付树在构建层面等价，没有未声明的功能改动（ASB-10 的一半：源码未变时摘要必须一致）。

## 3. 第二次构建：LIGHT-DEF-2 形制落地后

| 项 | 结果 |
|---|---|
| 动作 | `{"action":"rebuild"}` |
| 时间（UTC） | 2026-11-10T08:39:52.114 – 08:39:53.976 |
| 结果 | `0 Error(s), 1 Warning(s)`（同一既有告警，未新增） |
| Program Size | `Code=25964 RO-data=656 RW-data=116 ZI-data=1676`（与基线相同） |
| `Project.axf` md5 | `b6a15fed2ed5e3d0c4adc51ac87a4830` |
| `Project.hex` md5 | `4d728dec9b0d28a352c46436b7586c72` |

**判读**：
1. 摘要变化是**代码变化**的直接结果（`light_is_dark` 由 `3*mean >= c_dark` 改为 `mean >= threshold` 并新增 `threshold==0` 防御分支），不是工程配置变化。
2. `Program Size` 三项未变、Code 也恰好相同，属指令选择后的净结果；不以“大小未变”推断未改动——源码 diff 与摘要变化同时存在，见 `git diff --stat`（10 文件、+118/−92 行）。
3. 未改 `IROM/IRAM` 配置、未改优化等级（工程仍 `uAC6=1/Optim=2`）、未增删源文件，故工程文件列表与链接脚本均未变。

## 4. GNU 交叉编译验证（gcc/build.sh）

| 项 | 基线 | LIGHT-DEF-2 后 |
|---|---|---|
| 结果 | 0 error / 27 warning（均为厂商库 `-Wextra` 既有告警与 note） | 0 error / 27 warning（同集合） |
| FLASH | 34836 B / 64 KB（53.16%） | 34840 B / 64 KB（53.16%） |
| RAM | 1816 B / 4 KB（44.34%） | 1816 B / 4 KB（44.34%） |
| 产物 md5 | elf `7934b270…`（基线未留存 elf 摘要；见下） | elf `7934b270997688891e2a670d7d06862d`、hex `b5473d41c392f5dde9f2408883dabf95` |

说明：基线 gcc 日志中未记录 elf 摘要（两次运行相隔数分钟，基线日志仅保留 FLASH 34836 B 与 0 error），因此上表只对**变化量**（+4 B）与告警集合做对照，不声明基线 elf 摘要。两套工具链的绝对大小不可直接比较（不同优化与库）。

## 5. 产物身份（本轮交付构建）

| 产物 | 路径 | md5 |
|---|---|---|
| AXF | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` | `b6a15fed2ed5e3d0c4adc51ac87a4830` |
| HEX | 同目录 `Project.hex` | `4d728dec9b0d28a352c46436b7586c72` |
| GNU ELF | `.../gcc/obj/sensor_fw.elf` | `7934b270997688891e2a670d7d06862d` |
| GNU HEX | `.../gcc/obj/sensor_fw.hex` | `b5473d41c392f5dde9f2408883dabf95` |

完整源文件摘要清单见 `md5_list.txt`（10 个改动文件 + 4 个构建产物）。

## 6. 构建边界（不得外推）

1. **未下载、未烧录、未复位**：`mdk_flash` 需要 `mdk_flash` 设备工具 + `flash_device` 外部动作，本能力 `sensor_programming` 入口为 `unavailable_for_current_capability`；本轮未申请、未尝试。故**板上镜像身份未知**，`b6a15fed…` 只是本地构建产物。
2. **未做 COM43 采集**：`serial_capture` 同属未授予的设备工具。任何“判据未启用 → `S` 行 V=0 D=0”的实板断言都不由本文件提供（宿主等价性见 `driver_test_evidence.md`）。
3. 构建成功 ≠ 功能通过；`Program Size` 与 md5 只标识产物，不证明采样/上报/无光判定正确。
4. 本文件不含功耗、时序或电气结论；未使用示波器/电流表/逻辑分析仪。
