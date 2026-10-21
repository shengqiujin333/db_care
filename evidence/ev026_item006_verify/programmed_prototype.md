# Programmed prototype — ITEM-006 verification round

状态：本次实际成功下载的固件身份和目标（**只证明下载与回读校验，不代表业务功能合格**）

| 项 | 值 |
|---|---|
| 目标板 | 传感器板 CW32L010F8（GXHT40 + 光敏分压 + UM2005C），MCU UID 见历史横幅 `6A002C00`（本轮横幅未入捕获窗口） |
| 调试串口 | COM43，9600 8N1（Prolific 067B:2303） |
| 调试器 | CMSIS-DAP，`VID_C251/PID_F001`（COM26 为其 CDC 口） |
| 受测提交 | `8971e631872d877375312f8d7a631fea4eb53f48` |
| 下载文件 | `MDK/output/exe/Project.axf` |
| axf md5 | `9cb97918e8832811dfcb00a93646fba1` |
| hex md5 | `3af11e7adb21ba43004fc827649adb44` |
| 构建 | `mdk_build rebuild`：0 Error / 1 既有 warning，Code=19544 RO=644 RW=116 ZI=1684 |
| 下载日志 | `Erase Done.Programming Done.Verify OK.Application running ...` |
| 下载时间 | 2026-10-21 12:28:37（本地）/ 04:28:37 UTC |
| 构建标识常量 | `SENSOR_FW_BUILD_ID = "RUN10-FDS003"`（实板横幅未入窗，见 `test.md` §6.3） |
| 原始日志 | `mdk_flash.log`、`build_rebuild.log`、`image_md5.txt` |

说明：`Verify OK` 仅证明下载回读校验通过；本文件不表示标定、光照判据、采集、上报或任何产品功能已合格。功能结论见 `test.md`。
