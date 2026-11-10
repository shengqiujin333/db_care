# Android 候选代码图入口

负责角色：代码智能。检查日期：2026-11-10。源码基线：`7848b997466091b4eb9ab2c22e8543a9736ee953`；Android 工程 Git tree：`de89c68da8c7de69f51e27c0a5c2b757bd76c610`。接手工作区干净，已同步共享清单、最近提交、当前 Android 实现记录中 AA-005 七项增量及既有系统分析。

本次只确认当前 Android 候选索引；不运行构建、App 测试、设备或服务访问，不判定业务实现正确性。实现报告中的 235 项自检和 APK 身份是上游记录，不是本次执行结果。产品容器根无 `.codegraph/`，不能据此遗漏 Android 既有子索引。

## 入口、范围和版本

- projectPath：`D:\mypro\beiwo2\AIP\dengbei_care`
- indexPath：`D:\mypro\beiwo2\AIP\dengbei_care\.codegraph`
- 工具：已安装 `@colbymchenry/codegraph 1.6.0`，沿用同一 Windows bundle CLI；没有初始化、全量重建、安装、升级或网络下载。
- 覆盖：当前 Gradle 工程内可解析 Kotlin/Java、Manifest、properties 等，含 `app/src/main/`、宿主/设备测试和内嵌 Paho。测试与 Paho 降低排序优先级，不代表测试有效或已执行。
- 排除：`iOS开发资料/`、`.idea/`、`.gradle/`、`build/`、`app/build/`。外部 SDK、Gradle 依赖、APK、iOS 与硬件资料不作为本次索引对象；不为已部署后端虚构本地源码入口。
- 当前代码配置 `codegraph.json` 未修改；生成目录、数据库与业务文件未提交或改写。

证据：[codegraph_android_validation.json](../evidence/codegraph_android_validation.json)。记录源码版本、配置、实际命令、UTC 起止与超时、退出码/stderr、同步前后状态、精确查询的源码文件名/行号节选、响应摘要及逐文件 SHA-256，没有导出全图或保存整个源码。

## 本次同步与查询

同步窗口：2026-11-10 10:02:45.594–10:02:46.691 UTC。同步前 added=15、modified=15；正常增量同步 30 个文件、处理 977 个节点。完成后 103 文件、2787 节点、5774 边；`state=complete`、`pendingRefs=0`，added/modified/removed=0，`worktreeMismatch=null`、`reindexRecommended=false`。

精确文件查询窗口：10:03:10.509–10:03:11.369 UTC。下面路径均以 `app/src/main/java/com/jinyuni/dengbei_care/` 为前缀：

| 当前源码 | 已返回的符号或行号 | 检索用途 |
|---|---|---|
| `ble/BlePermissionPolicy.kt` | object:45、evaluate:50、connectRequestPermissions:106、scanRequestPermissions:114 | 定位权限策略；符号图列出 MainActivity、MqtttService 和设置对话框调用方 |
| `protocol/GatewayReadGuard.kt` | object:15、MAX_DEVICES:18、MAX_FRAME_BYTES:21、expectedFrameSize:27 | 定位容量/完整性守卫；图列出服务和设置页调用方 |
| `telemetry/BleBatchForwarding.kt` | object:15、Forward/Refuse:18–19、plan:31 | 定位完整批次载荷生成及拒绝结果 |
| `cloud/UploadResult.kt` | Success:16、AuthRejected:18、RequestRejected:20、Retryable:22、fromHttp:25 | 定位上传分类；图列出 CloudUploadRepository 调用关系 |
| `ble/GatewayDiscovery.kt` | class:28、SCAN_TIMEOUT_MS:36、start:91 | 定位扫描生命周期；图列出设置对话框调用关系 |
| `protocol/GatewayFrameCodec.kt` | object:16、u16be:31、s16be:35、SensorSample:49 | 定位当前帧解析及样本类型 |

表中内容只是定位证据，不表示权限行为、扫描停止、MTU、上传或端到端测试通过。符号查询 `--max-files 1` 可能优先展示高连接度的关联文件：本次 UploadResult、GatewayDiscovery、GatewayFrameCodec 的初始查询出现了此情况。证据保留初始查询与限制说明；随后每项精确到文件路径，逐项检查返回的源码文件名匹配目标且有当前行号源码，才作可用性依据。不能仅凭概览中提到目标符号就宣称已读到其实现。

最小查询建议直接固定文件，避免单文件排序歧义：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayReadGuard.kt"
```

MCP 等价调用使用上述真实 projectPath、完整相对文件路径及 maxFiles=1。直接使用返回源码和调用路径，不重复打开已返回正文；若输出裁剪，针对缺失符号继续小查询。

## 身份、限制和后续同步

当前 103 个实际索引文件集合 SHA-256：`1c1db9d0bbf6cfbb770427d14e9784e627349e14abf437617b09067671e9ee3a`。逐文件按原始字节计算 SHA-256；集合按规范化相对路径排序，将 `path + TAB + lowercase_sha256 + LF` 拼接为 UTF-8 后计算。不同序列化格式的集合摘要不能直接比较；源码变化用同路径摘要或 Git tree 确认。

全部命令已完成、退出 0、stderr 为空、未超时。普通 CLI/Git 命令上限 30s，同步上限 45s；无遗留后台任务。`git check-ignore` 确认本地 `.codegraph/` 被忽略，`git ls-files` 确认没有索引数据库被跟踪。细粒度图留在本机，不提交大型数据库。

本次必需且未验证的代码根：无（本次声明范围仅 Android）。传感器与网关子索引仍是已知入口，但本次没有重新查询或刷新其就绪结论；此前版本/查询证据保留于 `evidence/codegraph_firmware_initial_validation.json`，不能作为新的检查时点。原入口说明的问号编码损坏本次修复为可读摘要，旧证据与版本不变。

代码图仅覆盖可解析静态关系，不证明条件编译、外部库、真实 BLE/RF/HTTP 行为、APK 安装状态或候选验收通过。后续修改、切换版本或改变排除规则后依次执行：

```powershell
codegraph sync "D:\mypro\beiwo2\AIP\dengbei_care"
codegraph status --json "D:\mypro\beiwo2\AIP\dengbei_care"
codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "<相关文件或符号>"
```

核对同步状态和当前源码查询后才可沿用索引；需要核对覆盖时使用 `codegraph files -p "<projectPath>" --format flat --no-metadata`。本次没有改产品源码、用户资料、权限或工作流，不自行提交 Git。
