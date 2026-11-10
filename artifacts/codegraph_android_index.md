# Android 修复候选代码图入口

负责角色：代码智能。检查日期：2026-11-10。源码基线：`e6fc602cb675d85294acd34b937a8732636fd284`；Android 工程 Git tree：`fb74a365211407cacb03a428623df825da46259c`。接手工作区干净。

已同步共享清单、最近提交、当前 Android 实现记录的修复与重建段、FIX-AA005-1 修复说明及软件失败归因。d6fc6cd 修改上传全局阻塞逻辑和实现侧回归；e6fc602 记录完整交付重建。修复前失败与实现侧复验均是上游证据，本次不执行用例、不判断缺陷已独立验收或真实链路通过。

本文件替代上次7848b99候选入口；旧报告和查询记录保留在Git历史。当前证据同时保留先前检查的基线与记录摘要，不能沿用旧源码节选代表修复后实现。

## 入口与覆盖

- projectPath：`D:\mypro\beiwo2\AIP\dengbei_care`
- indexPath：`D:\mypro\beiwo2\AIP\dengbei_care\.codegraph`
- 产品容器根无 `.codegraph/`，本次沿用Android已有子索引。
- 工具：已安装 `@colbymchenry/codegraph 1.6.0`，通过其现有Windows bundle CLI执行；没有安装、升级、初始化或全量重建。
- 覆盖：本地Gradle工程内可解析Kotlin/Java、Manifest、properties等，包括app/src/main、宿主/设备测试和内嵌Paho。测试/Paho仅降低排序优先级，不表示测试能编译、有效或已执行。
- 排除：`iOS开发资料/`、`.idea/`、`.gradle/`、`build/`、`app/build/`；不扩到外部Android SDK、Gradle依赖、APK、iOS、硬件或远端服务。codegraph.json未修改。

证据：[codegraph_android_validation.json](../evidence/codegraph_android_validation.json)。含实际路径、源码版本、配置、变更摘要、同步前后状态、命令/UTC起止/超时/退出码/stderr、精确文件查询节选与响应摘要、104个索引文件摘要。未导出全图或整段源码。

## 同步和当前源码查询

同步窗口：2026-11-10 13:47:36.898–13:47:37.703 UTC（本地21:47）。同步前added=1、modified=2；正常增量同步3文件、处理193节点。完成后104文件、2863节点、6113边，`state=complete`、`pendingRefs=0`、added/modified/removed=0、`worktreeMismatch=null`、`reindexRecommended=false`。

精确文件查询窗口13:47:38.268–13:47:38.630 UTC，各查询实际返回的源码文件名与指定目标一致且含当前行号：

| 路径（相对Android根） | 已确认定位 | 用途及限制 |
|---|---|---|
| app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt | UploadGate.isUploadBlocked:62、globalBlocked:111、isUploadBlocked:114、enqueueAndUpload:123及阻塞判断:138 | 定位修复后的全局阻塞接口与接线；未据此判定并发、冷启动或HTTP真实行为通过 |
| app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayReadGuard.kt | object:15、MAX_DEVICES:18、capacityVerdict:57、mtuVerdict:60、lengthVerdict:73 | 当前读取保护上下文可查询；不证明真实MTU或9设备支持 |
| app/src/test/java/com/jinyuni/dengbei_care/cloud/CloudUploadFailureClassificationTest.kt | class:16、globalBlocked_stopsAllAutomaticUploads_untilManualRetry:120、blockedState_stillEnqueuesNewReadings_withoutAutomaticRequest:147 | 定位实现侧新增回归；图可查询不代表本次运行或独立测试通过 |

最小查询示例：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt"
```

MCP等价调用需显式传上述projectPath、完整相对文件路径及maxFiles=1。之前符号查询在单文件排序下可能展示其他高连接度文件，本次直接固定目标文件并验证源码标题，不仅依据概览中出现符号名称。直接使用返回源码和调用关系，不重复打开已返回正文；裁剪时继续定向查询缺失符号。

## 身份、状态与后续使用

当前104个实际索引文件集合SHA-256：`5b4eb564e8a084c56f8aaaa3d493170d769e0553bc312ba986b0c1d2cc5d128c`。逐文件对原始字节求SHA-256；集合按规范化相对路径排序，将`path + TAB + lowercase_sha256 + LF`拼接为UTF-8再求SHA-256。与前次采用同一格式；证据中逐项保留文件摘要。

全部CLI/Git命令已完成、exit0、stderr空、未超时；普通命令30s、同步45s上限，无后台任务。git check-ignore确认本地索引被忽略，git ls-files确认没有数据库被跟踪；细粒度图只留本机，不提交大型数据库。

本次必需且未验证根：无（本次声明范围仅Android）。传感器与网关有既有子索引及先前版本记录，本次未重新验证，不刷新其就绪结论；可参考artifacts/codegraph_firmware_initial_index.md及原始证据获取历史入口。后端未给独立本地源码入口，不为远端服务虚构索引。

索引只证明可解析的当前静态源码和关系可检索，不认证APK、手机安装、设备在线、修复验收或跨RF/BLE/HTTP链路。上游257项自检、重建APK及独立失败均保持各自版本和证据边界。本次不修改业务代码、设计、验收、权限或用户资料，不操作硬件/网络服务，不运行构建或产品测试。

后续代码修改、版本切换或排除配置改变后，执行：

```powershell
codegraph sync "D:\mypro\beiwo2\AIP\dengbei_care"
codegraph status --json "D:\mypro\beiwo2\AIP\dengbei_care"
codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "<相关文件或符号>"
```

确认状态及当前源码查询后再沿用索引。覆盖核对使用`codegraph files -p "<projectPath>" --format flat --no-metadata`。本次只更新入口、证据和文件清单，不自行Git提交。