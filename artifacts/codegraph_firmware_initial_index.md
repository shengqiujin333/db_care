# 固件候选更新后的代码图入口（既有检查记录）

负责角色：代码智能。本文件修复先前说明文字被问号替换的编码问题；依据原始结构化证据重新整理，不重新赋予检查时点、不修改原始证据，也不宣称当前再次核验三个根。Android 新候选的当前入口见 artifacts/codegraph_android_index.md。

原始检查源码基线：`82a7e74e5d6bae855a573e1f23f83936ba8db049`。原始证据：[codegraph_firmware_initial_validation.json](../evidence/codegraph_firmware_initial_validation.json)。当时产品容器根无 .codegraph，沿用三个现有子索引，使用已安装 CodeGraph 1.6.0，未初始化、重建或安装软件。

## sensor

- projectPath：`D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output`
- indexPath：`D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output\.codegraph`
- 当时工程 Git tree：`5666aa1c9d86618144fda4c0e046c8f84fd81a95`；最近工程提交：`82a7e74e5d6bae855a573e1f23f83936ba8db049`。
- 同步结束 UTC：2026-11-10T08:46:43.587892+00:00；文件/节点/边：99/1255/3498。
- 当时排除：MDK/RTE/、MDK/output/、EWARM/Debug/、EWARM/Release/、obj/、build/。
- 当时文件集合 SHA-256：`36c2e8c0ac67aa0fc50c2cc3220e5b110add9273a225b8976dae4302e1c55f86`；逐文件摘要及序列化格式见原证据。
- 实际查询：light_is_dark（结束UTC 2026-11-10T08:47:13.946967+00:00）；light_sample（结束UTC 2026-11-10T08:47:14.128094+00:00）；gxht40_measure（结束UTC 2026-11-10T08:47:14.302825+00:00）；trace_uart_open（结束UTC 2026-11-10T08:47:14.487430+00:00）。当前行号源码节选、调用方及完整响应摘要在原证据中，不以报告文字替代查询。

最小查询示例：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output" --max-files 1 "light_is_dark"
```

## gateway

- projectPath：`D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2`
- indexPath：`D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2\.codegraph`
- 当时工程 Git tree：`22c8a193daffea87408c971a53db566633f7c6ea`；最近工程提交：`4d101ae6459f89fa7f26f0d437d1d86a8be7a369`。
- 同步结束 UTC：2026-11-10T08:46:44.308164+00:00；文件/节点/边：34/508/1066。
- 当时排除：.settings/、.idea/、obj/、Debug/、Release/、build/。
- 当时文件集合 SHA-256：`2d7a74f42db9cbbd4a6db67aacc32ac3abb59125810fec441225e6a33408979d`；逐文件摘要及序列化格式见原证据。
- 实际查询：decode_frame10（结束UTC 2026-11-10T08:47:14.667091+00:00）。当前行号源码节选、调用方及完整响应摘要在原证据中，不以报告文字替代查询。

最小查询示例：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2" --max-files 1 "decode_frame10"
```

## android

- projectPath：`D:\mypro\beiwo2\AIP\dengbei_care`
- indexPath：`D:\mypro\beiwo2\AIP\dengbei_care\.codegraph`
- 当时工程 Git tree：`3c9feeb9ce399b3ba5ac359b31090af583ce4781`；最近工程提交：`4d101ae6459f89fa7f26f0d437d1d86a8be7a369`。
- 同步结束 UTC：2026-11-10T08:46:45.095375+00:00；文件/节点/边：88/2396/4770。
- 当时排除：iOS开发资料/、.idea/、.gradle/、build/、app/build/。
- 当时文件集合 SHA-256：`f0ef6c66056f70c3857f4881bc7dcfc6e958a4bad2462067ca33e2124327f824`；逐文件摘要及序列化格式见原证据。
- 实际查询：GatewayFrameCodec（结束UTC 2026-11-10T08:47:14.860850+00:00）。当前行号源码节选、调用方及完整响应摘要在原证据中，不以报告文字替代查询。

最小查询示例：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "GatewayFrameCodec"
```

## 覆盖和限制

传感器覆盖 USER/、COMMON/、UM2005C/、rfconfig.h 与本工程可解析 C 测试/头文件，不扩到外部 CW32 SDK 或其他示例；网关覆盖 APP/、Profile/，不扩到上级 HAL、预编译库或其他 EVT；Android 覆盖可解析 Kotlin/Java、Manifest、properties 及工程内测试/Paho，不扩到外部 SDK、Gradle 缓存、APK 和 iOS。测试/mock 的排序降级不是测试有效性或实板行为证明。

原检查中传感器同步9个变更文件；网关和Android返回 Already up to date。三个根同步后均 state=complete、pendingRefs=0、pendingChanges=0、worktreeMismatch=null、reindexRecommended=false，命令 exit0、stderr空、未超时，无遗留后台任务。该结论仅对应原证据窗口，不能覆盖后续 Android 源码变化。

普通命令原上限30s，同步45s。原证据确认三个 .codegraph 被Git忽略且无数据库被跟踪；细粒度图只留本地。初次同级bundle路径断言失败后，确认实际嵌套依赖目录并执行同一CLI，没有安装或联网。

后续消费者必须传真实 projectPath；直接使用图返回源码和调用关系。修改代码、切换版本或改变排除规则后，依次执行 codegraph sync、codegraph status --json、相关 explore 小查询，确认来源与状态再沿用。需要覆盖清单时使用 codegraph files -p "<projectPath>" --format flat --no-metadata。图不证明编译、设备、跨进程链路、业务测试或部署验收通过；硬件、iOS、其他SDK/归档/生成目录及无独立本地源码入口的后端均未纳入。
