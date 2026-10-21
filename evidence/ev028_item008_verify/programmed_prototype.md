# Programmed Prototype — ITEM-008 最终镜像下载记录

- 能力：`embedded_tester.embedded_verification`
- 项目：dengbei_run10
- 目标：CW32L010F8 传感器板（当前 Keil 工程 Device 字段）
- 调试/烧录后端：Keil MDK 保存的 CMSIS-DAP（CW-DAPLink）与 CW32L010 Flash 算法
- 下载时间：2026-10-21T05:01:41Z → 05:01:51Z（本地 13:01:51）

## 实际下载身份

| 产物 | md5 |
|---|---|
| `MDK/output/exe/Project.axf` | `9cb97918e8832811dfcb00a93646fba1` |
| `MDK/output/exe/Project.hex` | `3af11e7adb21ba43004fc827649adb44` |

源码身份：Git `c176d02dd0e6220a605c90bf34e84ac7a929565b`；产品源码 SHA-256 见
`source_sha256.txt`。

## 下载结果（原始日志 `mdk_flash.log`）

```text
Load "...\MDK\output\exe\Project.axf"
Erase Done.Programming Done.Verify OK.Application running ...
Flash Load finished at 13:01:51
```

下载后重新计算 axf/hex md5 与上表一致（`image_md5_after_flash.txt`）。

## 边界

- `Verify OK` 仅证明下载校验通过；**不表示**采样、判定、上报、UART、低功耗或端到端功能合格。
- 下载后 COM43 300 s 观测为 1 个失败周期（`S ... q=0` + `G s=2`），见 `test.md` §5；
  U01/U02 仍未闭合。
