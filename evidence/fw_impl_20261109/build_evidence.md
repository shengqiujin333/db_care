# 固件构建证据（FW-BUILD-20261109）

负责角色：固件工程
执行时间：2026-11-09 16:15:15–16:15:17（Asia/Shanghai）＝ 08:15:15–08:15:17 UTC
受测对象：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/`（传感器，CW32L010F8）

## 1. 源码身份

| 项 | 值 |
|---|---|
| 仓库 HEAD | `fdfcd852b6282987d022ac7936bc8ba0d6cba5d9` |
| sensor 工程 tree | `09bef6581f39ca50ba2ecfbaa5c77ef9accd68a5` |
| `USER/` tree | `746a72d4cc995490341270441792cae85727e923` |
| 与设计基线 `ef5caa6` 的产品源码差异 | `git diff --stat ef5caa6 HEAD -- USER/ test/` 为空；产品源码/测试资产无功能改动 |
| 构建入口 | `MDK/Project.uvprojx`（Keil MDK，工程保存的器件/Flash 配置） |

## 2. 构建命令与结果

```text
mdk_build {"action":"rebuild"}
```

| 项 | 实际结果 |
|---|---|
| 编译器 | ARMCLANG V6.24（`C:\Keil_v5\ARM\ARMCLANG\Bin`） |
| 结果 | **0 Error(s), 1 Warning(s)** |
| warning | `../USER/src/main.c(254): warning: while loop has empty body`（既有空循环体告警，非本轮引入） |
| 资源摘要 | `Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| `Project.axf` md5 | `a3b08774e764f174eac318f6771d0c42` |
| `Project.hex` md5 | `546ce20a718fa69b40177cce0145f4b5` |
| 构建日志 | `evidence/fw_impl_20261109/mdk_rebuild.result.txt`；`.../mdk_rebuild.stderr.txt`（空） |
| 工程备份产物 | 工具在 role work 目录生成 `Project.uvprojx` 备份；项目 `Project.uvprojx` 未被修改（`git status` 无该文件） |

产物摘要与 FD-002 rev 7 / FW-EXP-002 rev 2 期望一致；当前源码未改，因此重建可复现同一 axf/hex。

## 3. 未执行与边界

- **未执行** `mdk_flash`/下载/复位/串口/实板观测：当前能力只授权 `mdk_build`，无可用的下载或串口入口；构建成功不代表实板功能通过。
- 未执行低功耗/栈峰值测量；资源摘要只证明链接尺寸，不证明运行期栈峰值或 CR2032 电流。
- 本证据只对应上述 commit/tree 与重建产物；若后续 T5 回填 `C_dark` 或修改源码，必须重新构建并记录新摘要。
