# Android 当前候选代码图入口

负责角色：代码智能。检查日期：2026-11-11。接手工作区干净，源码基线 `9b3df8d2baa3d7eb1214467e811f24f321be85a8`；Android工程Git tree `c50fa337de36f5b1a59353acf9fdf8aed31b68f4`。

已同步文件清单、最近提交、Android实现记录中产品验收修复/重建段及FIX-PROD-01。当前候选包含dacf888的布局/文案改动及9b3df8d的重建记录。上游实现侧261项自检、APK和待独立实机复测声明不作本次执行或验收结果。本次只做代码索引，不修订随包手册、不执行产品复验；操作员的整包文档修复要求不扩展本角色职责。

## 真实入口与覆盖

- projectPath：`D:\mypro\beiwo2\AIP\dengbei_care`
- indexPath：`D:\mypro\beiwo2\AIP\dengbei_care\.codegraph`
- 产品容器根无`.codegraph/`；沿用已有Android子索引，不重新初始化或全量重建。
- 已安装工具：`@colbymchenry/codegraph 1.6.0`及其Windows bundle CLI；本次没有安装、升级、网络下载或修改配置。
- 覆盖：现有配置与解析器纳入的Kotlin/Java、AndroidManifest、properties等，含app/src/main、宿主/设备测试和Paho。测试/Paho降低排序优先级；可检索不代表可编译或测试通过。
- 配置排除：`iOS开发资料/`、`.idea/`、`.gradle/`、`build/`、`app/build/`；外部SDK、依赖、APK、iOS、硬件和远端服务不纳入本次代码根。
- **当前三个变更布局XML未被索引**：activity_main.xml、fragment_home.xml、fragment_notifications.xml均不在files输出。不能因状态languages含xml就声称覆盖所有Android XML资源。

证据：[codegraph_android_validation.json](../evidence/codegraph_android_validation.json)。保存命令、UTC起止/限时/退出码/stderr、源码版本、同步前后状态、实际查询、源码节选与响应摘要、105个已索引文件摘要，以及三个未索引布局的文件摘要。先前记录保留于Git历史，本证据包含前次基线与记录摘要；不以旧检查时点充当本次验证。

## 本次同步与查询

2026-11-11 10:58:31.726–10:58:32.307 UTC（本地18:58）正常增量同步：新增1个被索引文件、处理12节点。同步后105文件、2875节点、6139边；state=complete、pendingRefs=0，added/modified/removed=0，worktreeMismatch=null、reindexRecommended=false。

查询结束时间范围10:58:32.849–10:58:33.390 UTC：

| 指定目标 | 实际结果与证据边界 |
|---|---|
| app/src/main/res/layout/activity_main.xml | No relevant code found；不是图内源码，不把查询退出0当命中 |
| app/src/main/res/layout/fragment_notifications.xml | 同上；已有files清单亦不含该文件 |
| app/src/test/java/com/jinyuni/dengbei_care/ui/notifications/NotificationsLayoutContractTest.kt | 精确返回目标当前源码：class:18，navHostHeight_isConstraintBounded_aboveBottomNavigation:32，notifications_isScrollable_andHasNoBottomParentAnchoring:47，notifications_controlsAreOrderedAndDoNotOverlap:55，homeHint_pointsToExistingEntries:77 |
| app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt | 精确返回目标当前源码，含UploadGate.isUploadBlocked:62等上传编排接口；保持已有产品源码可检索上下文 |

新增回归测试的源码可查询，只证明该资产被索引，不证明XML当前约束、滚动效果或实际点击可达。布局检查的消费者应在本次图未命中后，针对这三个明确路径用有界原生读取，并由负责测试角色完成所需验证；无需为此无故重建图或扩大到无关目录。图不承担界面视觉、Android资源解析、APK或真实设备验收。

最小可用查询：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "app/src/test/java/com/jinyuni/dengbei_care/ui/notifications/NotificationsLayoutContractTest.kt"
```

MCP调用也须指定上述projectPath及精确相对路径。直接消费返回源码/调用关系，不重复打开已展示正文；裁剪则定向查缺失符号。固定文件并检查返回源码标题，避免单文件排序仅展示关联文件而误称读到目标。

## 身份、限制与后续同步

105个实际索引文件集合SHA-256：`2459b06d58cb3bae340084daae926979269a7435c6c273b33dd18f552e51d647`。逐文件按原始字节SHA-256；集合为规范化相对路径字典排序后，将`path + TAB + lowercase_sha256 + LF`的UTF-8拼接再SHA-256。三个未索引XML另记摘要，不混入“已索引文件集合”。

所有CLI/Git命令前台完成、exit0、stderr空、未超时，无后台作业；普通命令30s、同步45s上限。两个XML查询虽退出0但没有目标源码，已明确记录局限。git check-ignore和git ls-files确认本地索引被忽略、无数据库受跟踪，不提交大型数据库。

本次声明就绪范围仅Android根内上述实际图覆盖；未验证且必要纳入图的其他代码根：无。传感器/网关已有子索引，但本次不刷新其状态，历史入口见artifacts/codegraph_firmware_initial_index.md。后端无已确认独立本地源码入口，未为其建立索引。未索引布局限制不隐瞒、不伪造图结果。

后续修改代码、切换版本或改变排除配置后，执行：

```powershell
codegraph sync "D:\mypro\beiwo2\AIP\dengbei_care"
codegraph status --json "D:\mypro\beiwo2\AIP\dengbei_care"
codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "<相关文件或符号>"
```

核对来源、状态和当前源码命中后再沿用。覆盖核对用`codegraph files -p "<projectPath>" --format flat --no-metadata`。本次不修改产品源码、用户资料、权限、手册、测试或工作流；不构建、不操作硬件/服务、不自行Git提交。索引就绪不代表产品问题修复完成或整体验收通过。