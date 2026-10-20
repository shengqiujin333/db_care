# EV-018 原始证据（run10 固件可行性探索）

本目录只保存本轮 `firmware_engineer.firmware_feasibility_exploration` 实际产生的原始证据与可复跑原型，
**不是**正式实现，也**不是**独立验收通过证据。所有结论见 `artifacts/firmware_exploration.md`。

## 0. 身份与时间

| 项 | 值 |
|---|---|
| 项目源码 | `D:\mypro\beiwo2\AIP` |
| 接手 HEAD | `834af9a`（产品固件源码最后一次改动 `8ae4a33`；本能力未改产品源码） |
| 固件工程 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx` |
| Keil 编译器 | ARMCLANG V6.24 |
| 本轮 rebuild 产物 | `Project.axf` md5 `5fc16235ccd043f4edb48b16ee44da0b`；`Project.hex` md5 `9a32cf3bcdbca2c1346da9c3bb3581da` |
| 构建结果 | 0 Error(s), 1 Warning(s)（既有 `main.c(245) while(k--);` 空循环体）；Code=20100 RO=644 RW=116 ZI=1676 |
| 调试串口 | `COM43`，Prolific VID:PID `067B:2303`，9600 8N1 |
| 受测板 | 同一块传感器板；上一轮 EV-017 记录 MCU UID `6A002C00`（本轮未复位，未重新读取 UID） |
| 本轮时间 | 2026-10-20 16:11–16:25 UTC（本地 2026-10-21 00:11–00:25 +08:00） |

> 镜像身份说明：本轮 `mdk_build`（增量）与 `mdk_build rebuild` 产出的 axf/hex md5 与 EV-017
> 记录的实际下载镜像**逐字节相同**，说明当前工作树可确定性重建该镜像。本轮**没有**执行
> `mdk_flash`/复位（当前能力未获授权），因此“板上正在运行的镜像就是该 md5”是**一致性推断**，
> 不是本轮重新下载证明；串口输出格式与 EV-017 相同。

## 1. COM43 串口原始捕获

| 文件 | 内容 | 字节 | md5 |
|---|---|---:|---|
| `com43_300s.bin` | `serial_capture` COM43 @9600，300 s 原始字节（无时间戳，工具只给窗口起止） | 234 | `0aa4cb9b2e51eb60f5a6d87d0ea6b6dd` |
| `host_tests_current.log` | 当前工作树 `test/build_test.sh` 宿主机测试输出（5 组，184 项） | 14376 | `25a242781d8716857b8f82a248c71e65` |
| `build_rebuild.log` | 本轮 `mdk_build rebuild` 原始构建日志（0 Error/1 Warning、Code/RO/RW/ZI） | 1257 | `0113808ea5a03f3d3381a7ca1e3df69d` |

捕获窗口（工具元数据）：2026-10-20T16:11:51Z → 16:16:51Z。原始内容（逐字节）：

```text
S k=105 p=0 H=0 V=0 o=8 n=785 x=787 a=785 D=0 t=0 h=0 q=0 r=0 s=0 y=0\r\n
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------\r\n
S k=108 p=0 H=0 V=0 o=8 n=783 x=787 a=785 D=0 t=0 h=0 q=0 r=0 s=0 y=0\r\n
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------\r\n
```

限制：只有 2 个采样周期、单个窗口、无逐字节时间戳、未复位、未采集 `BOOT/VDD/IOTEST/IOSIG/BUS`。
`p=0/H=0` 是因为从未有过有效温湿度样本（不是“前值被清”）。

## 2. 逻辑分析仪被动捕获

| 文件 | 内容 | md5 |
|---|---|---|
| `sigrok/capture.sr` | fx2lafw `conn=2.35`，20,000,000 样点 @4 MHz = 5.0 s，D0/D1，触发 `D1=f` | `3dc8d459173e74ffaf1ee059f90546d0` |
| `sigrok/capture_metadata.json` | 工具记录的实际采样率/样点数/时长/命令参数 | — |
| `sigrok/decoded_i2c_swapped_correct.txt` | 按实际接线 `scl=D1, sda=D0` 的 I2C 离线解码（正确的 6 笔地址事务） | `3ba6ebb786f2d0ba80ea4c2d0d4d0ada` |
| `sigrok/decoded_i2c_configured_mapping_wrong.txt` | 按 `runtime/environment.yaml` 标注 `scl=D0, sda=D1` 的解码（错误映射，出现伪 `0x10` 读 + ACK） | `1d807d2136d1fae95346e0daf105e4b2` |

关键事实：正确映射下正好 6 笔单字节事务，依次 `0x44(W)`、`0x45(W)`、`0x44(W)`、`0x45(W)`、`0x44(W)`、`0x45(W)`，
每笔均为 **NACK**，无 `0xFD` 命令字节、无读事务；6 笔跨约 20.9 ms，SCL 位周期约 214 µs（约 4.7 kHz；D1 相邻上升沿 856 样点 @4 MHz）。
与 `gxht40_acquire()` 的“3 次整帧重试 × 2 个候选地址，地址 NACK 即返回”逐项一致。

> **通道标签与实测不一致**：`environment.yaml` 记 `D0=SCL、D1=SDA`，但本轮实测只有
> `D1=SCL、D0=SDA` 才能解出固件实际发送的 `0x44/0x45` 地址序列。按配置标注解码会得到
> 一个看似合法的 `0x10` 读 + ACK 伪结果。后续任何逻辑分析仪使用前必须重新核对夹线，
> 并同步修正配置（本能力不改 `runtime/environment.yaml`）。

限制：数字采样只证明逻辑电平与样点分辨率内时序，不证明模拟幅度/上升沿、不证明 U9 焊点连通或器件
是否得电；单次 5 s 窗口、单板。触发配置 `D1=f` 在实测映射下是 SCL 下降沿（不是 START）。

### 2.1 工具路径问题（环境记录，不是产品结论）

第一次 `sigrok_capture` 在角色工作目录下生成输出路径长度为 260 字符，`srzip` 创建临时文件失败
（`Error saving zipfile: Failure to create temporary file: No error`），未生成 `.sr`。
第二次使用同一受审计 `tool_cli.py` 入口，仅把 `--role-workspace` 指向短路径临时目录
`D:\rd10\roles\firmware_engineer`，并把 `D:\rd10\runtime` 设为真实 `runtime` 目录的 junction
（配置内容未复制、未修改），采集成功；随后把原始 `.sr`、metadata 复制到本目录并删除临时目录。
`D:\rd10` 已清理。该缺陷建议由 Runtime/工具维护方修复（缩短 session 目录名或使用短路径输出）。

## 3. 上报判据最小原型

| 文件 | 内容 | md5 |
|---|---|---|
| `proto_report_rule.c` | 只链接真实 `fw_core.c`，对比当前 `sensor_decide_report()` 与 PR-004 参考判据；不修改产品源码 | `d01dd18155d7510795297978718e9ead` |
| `proto_report_rule.out` | 14 向量输出：当前实现 3 项不符（v3/v6/v7），参考实现 0 项不符 | `e5704a7ec5741e6f9460a6b4d913fa6e` |

复跑（在仓库根）：

```sh
gcc -std=c11 -Wall -Wextra \
  -I CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc \
  evidence/ev018/proto_report_rule.c \
  CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/fw_core.c \
  -o /tmp/proto_report_rule && /tmp/proto_report_rule
```

## 4. 本轮未执行（如实登记）

- 未烧录、未复位、未改接线、未改供电（当前能力未授权）。
- 未做全暗标定，未测 LDR 暗阻，未使用万用表/示波器（未登记/未确认）。
- 未做 RF 发射、网关接收、BLE、手机 App、服务器验证（不在本能力/未授权）。
- 未做电池电流/发射峰值电流、未测 LSI 实际频率。
