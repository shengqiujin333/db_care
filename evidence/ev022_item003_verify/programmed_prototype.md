# 下载身份记录（ITEM-003 受测镜像）

能力：`embedded_tester.embedded_verification`
日期：2026-10-21（本地 10:50:33）

| 项 | 值 |
|---|---|
| 构建/下载工具 | Keil MDK `C:/Keil_v5/UV4/UV4.exe`，工程 `MDK/Project.uvprojx`，工程保存的 CMSIS-DAP + CW32L010 Flash 算法 |
| 受测提交 | Git `09d4c52592d0a711e506d91bd4ecb6c0ccddc14a`（工作区干净） |
| 被测文件 | `USER/src/measure.c` SHA-256 `0fe638320f04d6a5d41067b0a2313c8f0260ca56f13019bff9d5e38931769edf` |
| 构建结果 | `mdk_build rebuild`：0 Error / 1 既有 warning（`main.c(245) while(k--);`）；Code=20160 RO-data=644 RW-data=116 ZI-data=1676 |
| 镜像 | `Project.axf` md5 `789a199e720436dff1de2d51f2cbd9cd`；`Project.hex` md5 `afc12baa6cca101578da15cbdec34e58` |
| 目标 | 传感器板（CW32L010F8 + GXHT40 + 光敏分压 + UM2005C），调试串口 COM43 |
| 下载结果 | `Erase Done. Programming Done. Verify OK. Application running ...`（`mdk_flash`，10:50:33 本地） |
| 运行佐证 | 下载后 COM43 收到 `k=3/6/9` 三行 `S`，与 3 分钟节拍一致（见 `com43_capture_a.dat`/`_b.dat`） |

**边界**：`Verify OK` 只证明擦除/编程/回读校验成功，不表示任何业务功能合格。axf/hex md5 与实现记录 `artifacts/firmware_implementation_v5.md` 一致，说明本次下载的确是当前受测提交编出的镜像。

原始日志：`build_rebuild.log`（编译输出）、`mdk_flash.log`（下载输出）；镜像 md5 见 `image_md5.txt`。
