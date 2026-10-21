# EV-030 / 固件集成调试（EV-029 PA-001 失败）

本目录是 `firmware_engineer.firmware_integration_debug` 对 EV-029 `TEST_FAIL` 的集成调试证据；**未提交固件代码修复**，结论为 `WRONG_OWNER`（硬件/电气与嵌入式验证）。

| 文件 | 内容 |
|---|---|
| `debug.md` | 复现/诊断可信性/失败环节/竞争假设/归属与下一步 |
| `debug_audit.py` / `debug_audit.txt` | 身份、原始捕获、源码边界、主机自检 10/0 审计 |
| `host_tests_full.log` | 当前实现侧 7 组 295/0（仅宿主） |
| `build_current.log` | 当前树 `mdk_build build` 0/0，无可重建项 |

原始失败证据：`../ev029_full_firmware_verify/`（未修改）。
未烧录、未复位、未采集 COM43/逻辑分析仪；未修改产品源码、网关、Android、硬件。
