# 爱守护 iOS 版 - 软件功能需求文档

> 版本:v1.0 需求基线
> 日期:2026-07-14
> 目标:开发与 Android 版功能对等的 iOS 应用
> 参考实现:Android 版 `dengbei_care` v1.7

---

## 1. 项目概述

### 1.1 产品定位
爱守护是一款温湿度监控应用,主要面向老人关怀、婴儿房监控、储物环境监控等场景。支持多个温湿度传感器同时接入,提供实时数据展示、异常报警和每日数据汇总报告。

### 1.2 核心价值
- 一个 app 监控多个环境(卧室、客厅、婴儿房、储物间等)
- 报警及时送达(震动 + 铃声 + 系统通知),并明确告知是哪个设备
- 每个设备可独立设置阈值,适应不同环境的基线差异
- 每日早起报告,回顾昨日数据

### 1.3 平台要求
- iOS 14.0+
- iPhone 专属(暂不支持 iPad)
- 需要蓝牙 LE、推送通知、相机权限

---

## 2. 技术栈建议

| 维度 | 建议 | 备选 |
|------|------|------|
| 语言 | Swift 5.9+ | - |
| UI 框架 | SwiftUI(推荐) | UIKit |
| 最低系统 | iOS 14 | iOS 16(用更多新 API) |
| 架构 | MVVM + Combine | TCA、MVVM+Coordinator |
| 本地存储 | SQLite.swift | GRDB、Core Data、Realm |
| BLE | CoreBluetooth | RxBluetoothKit |
| MQTT | CocoaMQTT | Moscapsule |
| HTTP | URLSession + async/await | Alamofire |
| 后台任务 | BGTaskScheduler + UNUserNotificationCenter | - |
| 图表 | SwiftCharts(iOS 16+) | Charts(DGCharts) |
| 依赖注入 | 手动构造 | Resolver、Swinject |
| 包管理 | Swift Package Manager | CocoaPods |

---

## 3. 功能模块详细需求

### 3.1 用户注册与登录

#### 3.1.1 注册
- 输入手机号(11 位,1[3-9]\d{9})
- 点「获取验证码」按钮,调用云平台发送短信验证码(60 秒倒计时,5 分钟有效)
- 输入验证码(6 位数字)
- 输入密码(6-16 位字母/数字/符号)
- 输入确认密码(必须与密码一致)
- 点「注册」按钮 -> 调云平台 API 验证验证码 + 注册

#### 3.1.2 登录
- 输入手机号、密码 -> 调云平台 API 登录
- 登录成功后本地缓存手机号(`registerphone`)
- 同一手机号已登录时拒绝重复登录
- 登录/退出累计 30 次后锁定,需重装

#### 3.1.3 退出登录
- 清除本地 `registerphone`
- 跳转到登录页

#### 3.1.4 注销账号
- 连续点击 3 次以上触发
- 调云平台改密码(将账号"注销")
- 清除本地登录态

#### 3.1.5 忘记密码
- 提示用户用相同手机号重新注册,新密码覆盖旧密码

#### 3.1.6 协议勾选
注册页顶部有两个 checkbox:
- 「登录」勾选时:注册相关字段禁用,登录按钮启用
- 「注册」勾选时:反之

### 3.2 设备管理

#### 3.2.1 设备 ID 规则
- 8 位十六进制,大写,无分隔符(如 `0C118224`)
- 名称:纯中文 ≤ 3 字,或纯英文/数字 1-8 位
- 不允许中英混合或带符号

#### 3.2.2 添加设备
- 进入「传感器 ID 列表」页
- 输入网关地址,点「发送」
- 用摄像头扫码或手动输入 8 位 ID
- 输入设备名称
- 点「添加」

#### 3.2.3 删除设备
- 单个删除:点行右侧垃圾桶图标
- 批量删除:勾选多个后点底部按钮
- 删除时同时清掉该设备的独立参数和 sparkline 缓存

#### 3.2.4 设备列表存储
- UserDefaults 或文件存 `[id: name]` 字典
- 排序按 name -> id

### 3.3 首页 - 设备仪表盘

#### 3.3.1 卡片墙
- 顶部 MaterialToolbar 显示应用名 + 「启动守护」按钮
- 中间 RecyclerView(或 SwiftUI List)显示设备卡片
- 空态:无设备时显示插画 + 「去添加」按钮跳转到传感器列表

#### 3.3.2 设备卡片内容
每张卡片显示:
- 设备名称
- 状态 chip:
  - 灰色「未收到」:从未收到数据
  - 灰色「离线」:超过 5 分钟无新数据
  - 绿色「正常」:数据正常
  - 橙色「警告」:下降/超限报警
  - 红色「紧急」:温度 > 65°C 火灾预警
- 大字号温度(左)+ 大字号湿度(右)
- 迷你 sparkline 图表(最近 30 分钟温度趋势)
- 报警消息(如当前报警)

#### 3.3.3 卡片点击
进入设备详情页。

### 3.4 守护模式

#### 3.4.1 全局开关
- 首页 toolbar 右上角按钮
- 状态:`启动`(绿) / `停止`(红)
- 状态保存在 UserDefaults,app 重启后恢复

#### 3.4.2 倒计时
- 「设定」页设置 `guardtime` 小时
- 启动时记录 `finish_guard_time`
- 倒计时结束后自动停止
- app 重启后根据剩余时间恢复倒计时

#### 3.4.3 触发动作
- 启动/停止时发 MQTT 消息(`guardaction: "0.0"` 或 `"1.0"`)给网关
- 状态变更通知所有订阅者更新 UI

### 3.5 数据采集

支持两种数据源,可同时工作:

#### 3.5.1 BLE 路径
- 扫描特定 service UUID 的 BLE 设备
- 连接后读取 GATT 特征值,获取加密的 hex 数据
- 用 AES-ECB 16 字节密钥解密
- 解析后得到多个 (devId, temperature, humidity) 读数
- 每个 reading 调用 `processTemperatureHumidityData(devId, temp, humi)`

#### 3.5.2 MQTT 路径
- 连接 `ssl://117.72.84.210:8883`(用户名/密码见服务端配置)
- 订阅 `/+/a/r0`、`/+/a/r1`、`/+/a/s0` 等 topic
- payload 格式:`<time>:<temp1,temp2,...>:<humi1,humi2,...>`
- 按 saved IDs 顺序匹配 temp[i] / humi[i] 到第 i 个设备
- 每个 reading 调用 `processTemperatureHumidityData(devId, temp, humi, time)`

> **重要**:BLE 解密密钥、MQTT 服务器证书/用户名/密码等传输层认证信息,需要从 Android 版迁移过来,**不能改**(改了无法与现有网关通信)。见第 8 节「关键认证信息」。

### 3.6 报警系统

#### 3.6.1 报警类型
| 类型 | 触发条件 | 严重度 | 节流 |
|------|---------|--------|------|
| 下降报警 | 5/10/15 分钟前的数据相比当前,温度降 > tempdrop 且 湿度降 > humidrop | WARNING | 15 分钟 |
| 超限报警 | 温度超出 [tempmin, tempmax] 或湿度超出 [humimin, hummax] | WARNING | 15 分钟 |
| 紧急报警 | 温度 > 65°C | ALARM | 不节流 |

#### 3.6.2 报警触发动作
1. **更新 UI**:卡片状态 chip 变色 + 显示报警消息
2. **震动**:`VibrationPlayer.vibratePhone(duration=300s, loop=true)`
3. **响铃**:`RingtonePlayer.startAlarm(loop=true)`(用系统闹钟铃声)
4. **系统通知**:
   - Channel: `alarm_channel` (IMPORTANCE_HIGH)
   - 标题: `[设备名] 报警`
   - 内容: 报警类型消息
   - BigText: 完整报警消息 + 设备名
   - 颜色: 紧急红 / 警告橙 / 正常青绿
   - 点击打开 app(首页卡片墙)
   - notificationId 用 devId.hashCode() 保证不同设备不互相覆盖
5. **报警事件落库**:写入 `alarm_events` 表

#### 3.6.3 停止报警
- 屏幕任意位置点击一下,停止震动 + 铃声
- 卡片报警状态也清掉

#### 3.6.4 通知渠道
- `alarm_channel`: 设备报警(IMPORTANCE_HIGH,震动 + 锁屏可见)
- `daily_report_channel`: 早起报告(IMPORTANCE_DEFAULT)
- foreground service channel: 后台保活(iOS 用 BGTaskScheduler 替代)

### 3.7 参数设置(两级)

#### 3.7.1 全局默认值(设定页)
所有字段如下:
- `tempdrop`: 温度下降阈值(0-100,1 位小数)
- `humdrop`: 湿度下降阈值
- `tempmax` / `tempmin`: 温度上下限
- `hummax` / `hummin`: 湿度上下限
- `guardtime`: 守护时长(小时)
- `delayalarm`: 延时报警(秒)
- `dengbeialarm`: 下降报警开关(0/1)
- `yuzhialarm`: 阈值报警开关(0/1)
- `zhendongalarm`: 震动报警开关(0/1)
- `xianglingalarm`: 蜂鸣报警开关(0/1)

默认值:`tempdrop=2, humdrop=1, tempmax=35, tempmin=29, hummax=75, hummin=40, guardtime=8, delayalarm=8, 所有开关=1`

#### 3.7.2 每设备独立参数
- 设备详情页右上角菜单进入
- 字段与全局一致
- 存储格式:`UserDefaults key = "dev_<devId>_<param>"`
- 读取优先级:每设备值 > 全局值 > 代码默认值
- 删除设备时清理该设备所有 params

### 3.8 历史数据查询

#### 3.8.1 设备详情页
- 顶部 MaterialToolbar 显示设备名 + 返回按钮 + 右上角「该设备参数」菜单
- 温湿度变化 LineChart(双 Y 轴:左温度红、右湿度青绿)
- 时间范围搜索:
  - 起止时间选择器(DatePicker + TimePicker)
  - 时间范围必须 ≤ 50 分钟
  - 查询结果更新图表
- 报警记录列表(最近 24 小时):
  - 圆点(颜色按报警类型)+ 报警消息 + 时间
  - 空态:「最近 24 小时无报警」

#### 3.8.2 图表规格
- X 轴:时间(HH:mm),旋转 -45°
- 左 Y 轴:温度 -30~70°C,红色
- 右 Y 轴:湿度 0~100%,青绿色
- 数据点显示数值
- 线宽 1.5,圆点半径 2

### 3.9 早起报告

#### 3.9.1 定时任务
- 默认每天 07:00 推送
- 推送时间可配置(存 UserDefaults `report_time`)
- 用 `BGTaskScheduler` 注册周期任务(`BGProcessingTaskRequest`)

#### 3.9.2 报告生成
- 取昨日 00:00 ~ 24:00 时间范围
- 对每个设备:
  - 统计:`DailyStats(devId, count, minTemp, maxTemp, avgTemp, minHumi, maxHumi, avgHumi)`
  - 报警事件列表
- 生成结构化文本:
  - 摘要:总数据条数、有报警的设备数
  - 每设备段落:平均温度、最高/最低、报警次数
  - 建议条目:
    - 温度连续下降 > tempDrop -> "建议检查卧室通风"
    - 温度 > 65 -> "建议立即排查火源"
    - 等

#### 3.9.3 通知
- 推送一条本地通知
- 标题:「爱守护 早起报告」
- 内容:摘要
- 点击打开 ReportActivity(独立页面)

#### 3.9.4 按需触发
- 「我的」页底部按钮「查看今日报告」
- 点击立即触发报告生成 + 打开报告页

### 3.10 用户设置页(我的)

包含:
- 用户名(手机号)显示 + 「注册」按钮
- 传感器地址输入框 + 「发送」按钮
- 「扫描」按钮(扫码添加)
- 传感器 ID 输入框 + 「扫描」按钮
- 设备名称输入框
- 「添加」按钮
- 「打开传感器列表」按钮(跳转到设备列表)
- 「查看今日报告」按钮(立即生成报告)

---

## 4. 数据模型

### 4.1 SQLite 表结构

```sql
-- 温湿度数据(单表 + device_id 列)
CREATE TABLE temperature (
    time        INTEGER NOT NULL,     -- Unix 秒
    device_id   TEXT    NOT NULL,     -- 8 位 HEX 大写
    temperature REAL    NOT NULL,
    humidity    REAL    NOT NULL,
    PRIMARY KEY (time, device_id)
);
CREATE INDEX idx_temperature_device_time ON temperature(device_id, time DESC);

-- 报警事件
CREATE TABLE alarm_events (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    time        INTEGER NOT NULL,     -- Unix 秒
    device_id   TEXT    NOT NULL,
    type        TEXT    NOT NULL,     -- 'drop' / 'threshold' / 'emergency'
    message     TEXT    NOT NULL
);
CREATE INDEX idx_alarm_device_time ON alarm_events(device_id, time DESC);
```

### 4.2 数据库版本迁移
- v1: 老结构(3 张表 temperatureA/B/C)
- v2: 新结构(单表 temperature + alarm_events)

迁移逻辑:
1. `ALTER TABLE temperatureA RENAME TO temperature`
2. `ALTER TABLE temperature ADD COLUMN device_id TEXT`
3. 用第一个 MacIdBook 设备 ID 回填 `device_id`
4. `DROP TABLE temperatureB`、`DROP TABLE temperatureC`
5. 建 `alarm_events` 表

iOS 版本可以**直接从 v2 起步**(新装用户),但需要兼容从 Android 版迁移过来的本地数据(如果做跨平台数据迁移)。

### 4.3 关键 DAO API

```swift
protocol Database {
    // 写入
    func storeTemperature(devId: String, temp: Double, humi: Double, time: Int64)
    func storeAlarmEvent(devId: String, type: AlarmType, message: String, time: Int64)
    
    // 查询
    func getTemperatureByTimeRange(devId: String, start: Int64, end: Int64) -> [(Entry, Entry)]  // (tempEntry, humiEntry)
    func getDailyStats(devId: String, dayStart: Int64, dayEnd: Int64) -> DailyStats?
    func getAlarmEvents(devId: String, start: Int64, end: Int64) -> [AlarmEvent]
}
```

### 4.4 内存数据模型

```swift
enum Severity { case normal, warning, alarm }

struct DeviceState: Equatable {
    let devId: String
    var name: String
    var latestTemp: Double
    var latestHumi: Double
    var latestTime: Int64  // 0 表示从未收到
    var alarmMessage: String
    var alarmSeverity: Severity
    var sparkline: [ChartDataEntry]  // 最近 30 个采样
    
    func isOnline() -> Bool {
        guard latestTime > 0 else { return false }
        return Date().timeIntervalSince1970 - Double(latestTime) < 300  // 5 分钟
    }
}

@MainActor
final class DeviceStore: ObservableObject {
    @Published private(set) var devices: [String: DeviceState] = [:]
    @Published var guardRunning: Bool = false
    
    func updateReading(devId: String, name: String, temp: Double, humi: Double, time: Int64)
    func updateAlarm(devId: String, message: String, severity: Severity)
    func clearAllAlarms()
    func ensureDevice(devId: String, name: String)
    func removeDevice(devId: String)
}
```

---

## 5. 关键技术点

### 5.1 BLE 通信

#### 5.1.1 流程
1. `CBCentralManager` 扫描指定 service UUID
2. 发现设备后 `connect`
3. `discoverServices` -> `discoverCharacteristics`
4. `setNotifyValue(true)` 订阅特征
5. `peripheral(_:didUpdateValueFor:error:)` 回调收到 hex 数据
6. AES-ECB 解密(16 字节密钥)
7. 解析解密后的字节流 -> 多个 (devId, temp, humi) 读数

#### 5.1.2 设备 ID 提取
BLE 广播包里的 MAC 地址格式 `0C:11:82:24`,需要 `replace(":", "")` 转成 `0C118224` 匹配 MacIdBook。

#### 5.1.3 注意事项
- 后台模式需启用 `Uses Bluetooth LE accessories` capability
- iOS 后台 BLE 限制多,建议用 `state restoration`
- MTU 协商:iOS 默认不支持 `requestMtu`,用 `maximumWriteValueLength`

### 5.2 MQTT 通信

#### 5.2.1 服务器
- URI: `ssl://117.72.84.210:8883`
- 用户名/密码:从 Android 代码迁移(见第 8 节)
- Client ID: 随机生成(每次启动新 ID)
- Clean Session: true

#### 5.2.2 SSL 配置
- 使用 `.pem` 格式的 CA 证书
- 客户端证书可选(看服务器要求)
- 用 CocoaMQTT 的 `MQTTSSLConfiguration`

#### 5.2.3 订阅 topic
- `/<macAddr>/a/r0` - 温度数据回传
- `/<macAddr>/a/r1` - 湿度数据回传
- `/<macAddr>/a/s0` - 状态回传

#### 5.2.4 发布 topic
- `/<macAddr>/s/r0` - 守护启停命令
- `/<macAddr>/s/p0` - 参数下发(全局阈值)

#### 5.2.5 payload 解析
```
payload 格式: <time>:<temp1,temp2,...>:<humi1,humi2,...>
解析:
  parts = payload.split(":")
  time = parts[0].toLong()
  temps = parts[1].split(",").map { it.toDouble() }
  humis = parts[2].split(",").map { it.toDouble() }
设备匹配(位置匹配):
  for i in temps.indices:
    devId = MacIdBook.all()[i].id
    processTemperatureHumidityData(devId, temps[i], humis[i], time)
```

> ⚠️ **协议风险**:payload 不含设备 ID,假设网关按 saved IDs 顺序回传。如固件不能保证顺序,需要等网关固件升级支持显式 devId。

### 5.3 后台任务

iOS 没有前台服务,需要这样替代:

#### 5.3.1 BLE 数据采集保活
- 启用 Background Modes: `Bluetoth-central`
- 用 `CBCentralManager` 的 state restoration
- 限制:被系统杀掉后重启会延迟

#### 5.3.2 MQTT 保活
- iOS 没有持久后台连接,只能依赖 VoIP push 或 Background Modes
- 实际方案:用 `BGAppRefreshTask` 定时唤醒(每 15-30 分钟),重连 MQTT 拉取最新数据
- 极致方案:用 PushKit + VoIP push(但 Apple 审核严格)

#### 5.3.3 早起报告
- 用 `BGProcessingTaskRequest` 注册周期任务
- `earliestBeginDate: Date(timeIntervalSinceNow: secondsUntilNextReport)`
- 任务执行时生成报告 + 推送本地通知
- 实际触发时间可能漂移几分钟(正常)

#### 5.3.4 报警响铃 + 震动
- 报警触发时,app 可能不在前台:
  - 在前台:`AVAudioPlayer` 播放铃声 + `UIImpactFeedbackGenerator` 震动
  - 在后台:只能通过本地通知的 sound + 振动(系统默认,不能自定义循环)
  - 想要循环响铃:用 `AVAudioSession` 后台音频 + `beginBackgroundTask`
  - **iOS 限制**:无法像 Android 那样震动 5 分钟循环

### 5.4 通知

#### 5.4.1 本地通知
- `UNUserNotificationCenter` 请求权限(alert, sound, badge)
- 报警通知:categoryIdentifier = "alarm",action 可加「停止报警」
- 早起报告:每日 7:00 触发

#### 5.4.2 通知内容
```
标题: [设备名] 报警
副标题: 报警类型(如「温湿度超限报警」)
body: 完整消息 + 设备名
sound: .defaultCritical(循环响到用户处理)
interruptionLevel: .timeSensitive(iOS 15+,紧急报警用)
```

#### 5.4.3 点击通知
- 打开 app 跳到对应设备详情页
- 用 `userInfo` 传 devId

### 5.5 图表

#### 5.5.1 详情页大图
- 用 SwiftCharts(iOS 16+)或 DGCharts
- 双 Y 轴:左温度、右湿度
- X 轴时间格式 HH:mm
- 支持缩放、拖拽

#### 5.5.2 卡片 sparkline
- 80dp 高,无轴标签,无网格
- CUBIC_BEZIER 模式
- 填充色半透明
- 用 SwiftCharts 的 `LinePlot` + 自定义样式

---

## 6. UI 设计规范

### 6.1 品牌色板

| 用途 | 色值 | 说明 |
|------|------|------|
| 主色 Primary | `#00897B` | 温暖青绿,医疗关怀感 |
| 主色变体 Primary Variant | `#00695C` | 状态栏、深色按钮 |
| On Primary | `#FFFFFF` | 主色上的文字 |
| 次色 Secondary | `#FFB300` | 暖琥珀,提示 |
| 次色变体 Secondary Variant | `#FF8F00` | - |
| On Secondary | `#1A1A1A` | 次色上的文字 |
| Surface | `#FAFAFA` | 卡片背景(浅色模式) |
| Surface Variant | `#E0F2F1` | 容器、选中态背景 |
| On Surface | `#1A1A1A` | 主文字 |
| On Surface Variant | `#49454F` | 副文字 |
| Error | `#D32F2F` | 报警红 |
| On Error | `#FFFFFF` | - |
| Warning | `#FB8C00` | 警告橙 |
| OK | `#43A047` | 正常绿 |

### 6.2 深色模式色板

| 用途 | 色值 |
|------|------|
| Primary | `#00897B` |
| Surface | `#121212` |
| Surface Variant | `#1F1F1F` |
| On Surface | `#EAEAEA` |
| On Surface Variant | `#B0B0B0` |
| Error | `#EF5350` |

### 6.3 间距(4dp 网格)

| 名称 | 值 |
|------|-----|
| spacing_xs | 4 |
| spacing_sm | 8 |
| spacing_md | 16 |
| spacing_lg | 24 |
| spacing_xl | 32 |

### 6.4 字号

| 名称 | 值 | 用途 |
|------|-----|------|
| headline | 24 | 卡片大温度数字 |
| title | 18 | 页面标题 |
| body | 14 | 正文 |
| caption | 12 | 副文字、时间戳 |

### 6.5 圆角与阴影
- 卡片圆角 12pt
- 卡片阴影 2pt
- 按钮 4-8pt 圆角

### 6.6 Launcher 图标
- 设计概念:盾牌 + 心形(爱守护 = love protection)
- 前景:白色盾牌轮廓,内嵌品牌主色心形
- 背景:品牌主色青绿
- iOS 单尺寸:1024x1024

---

## 7. 权限清单

| 权限 | Info.plist Key | 用途 | 必须 |
|------|---------------|------|------|
| 通知 | `UIUserNotificationsUsageDescription` | 报警 + 报告 | 是 |
| 蓝牙 | `NSBluetoothAlwaysUsageDescription` | 连接传感器 | 是 |
| 蓝牙(后台) | `Bluetoth-central` Background Mode | 后台数据采集 | 是 |
| 相机 | `NSCameraUsageDescription` | 扫码添加设备 | 否 |
| 位置 | `NSLocationWhenInUseUsageDescription` | 扫描 BLE 设备辅助 | 是(iOS 13+ 不严格需要,但建议) |
| 音频 | `UIBackgroundModes: audio` | 报警响铃 | 是 |
| 后台刷新 | `BGTaskSchedulerPermittedIdentifiers` | 报告 + MQTT 保活 | 是 |

---

## 8. 关键认证信息(从 Android 迁移)

> ⚠️ **以下信息必须原样迁移到 iOS,不能修改**。改了无法与现有网关通信。

### 8.1 MQTT 服务器
- URI: `ssl://117.72.84.210:8883`
- 用户名: 见 Android `MqtttService.kt` 中 `mqttClient.connect(options)`
- 密码: 同上
- CA 证书: 见 `app/src/main/res/raw/` 下的 `.pem` 文件

### 8.2 BLE 解密密钥
- AES-ECB 模式
- 16 字节密钥
- 具体值见 Android `MqtttService.kt` 中 `decrypt()` 函数

### 8.3 BLE Service UUID
- 见 Android `BluetoothGattCallback` 中 `service.uuid.toString()`

### 8.4 云平台 API
- 注册/登录/验证码接口
- 具体域名和 endpoint 见 Android `ZhuCeViewModel.kt`

---

## 9. 数据迁移与版本兼容

### 9.1 首次安装
- 数据库 v2 起步,无需迁移
- SharedPreferences 设置默认参数(tempdrop=2, humdrop=1 等)

### 9.2 跨平台数据迁移
iOS 版**不支持**从 Android 版迁移本地数据(温湿度历史、报警事件)。原因:
- iOS 无法直接访问 Android 的 SQLite 文件
- 用户需要重新添加设备 + 重新设置参数

但可以提供:
- 云端账号关联(若 Android 版接入云平台,可同步用户配置)
- 手动导出/导入 JSON 备份(可选 v2 功能)

### 9.3 iOS 版本内升级
- 用 `SQLite.swift` 的 `DatabaseMigrator` 注册每个版本的 migration
- UserDefaults 用版本号 key(如 `ios_v1_migrated`)控制只跑一次

---

## 10. 测试要点

### 10.1 单元测试
- `DeviceParamsBook` 读写、fallback、清理
- `MacIdBook` 校验逻辑(中英文混合、长度)
- `ReportGenerator` 输入空数据 / 单设备 / 多设备 / 有报警 / 无报警
- 数据库 CRUD
- MQTT payload 解析

### 10.2 集成测试
- BLE 扫描 -> 连接 -> 解密 -> 落库 全链路
- MQTT 订阅 -> payload 解析 -> 落库 全链路
- 守护启动 -> 报警触发 -> 通知弹出

### 10.3 UI 测试
- 注册/登录完整流程
- 添加/删除设备
- 卡片墙点击跳转
- 时间范围搜索(边界:50 分钟整、超出 50 分钟报错)
- 早起报告手动触发

### 10.4 设备测试
- 多设备(2 个以上)同时监控,确认数据正确分流
- 报警节流(同设备 15 分钟内只触发一次)
- 紧急报警(温度 > 65)立即触发不节流
- app 杀进程后,守护状态恢复
- app 杀进程后,倒计时恢复

### 10.5 兼容性测试
- iOS 14 / 15 / 16 / 17 真机
- iPhone SE / 12 / 15 不同屏幕
- 深色模式切换不崩
- 通知权限拒绝时的降级(只显示 Activity 不发通知)

---

## 11. 已知限制与改进建议

### 11.1 当前限制(继承自 Android v1)
1. **MQTT 设备归属靠位置匹配**:payload 不含 devId,假设网关按 saved IDs 顺序回传。如顺序错误会数据错位。
2. **守护状态全局**:多设备共用一个守护开关,不支持「设备 A 守护中,设备 B 暂停」。
3. **早起报告不能 UI 改时间**:只能 adb 改 SharedPreferences。
4. **图表查询限 50 分钟**:查询逻辑限制,可以放开到更长。
5. **报警响铃 5 分钟**:iOS 后台限制可能更短。

### 11.2 iOS 版改进建议
1. **每设备独立守护**:守护状态从全局改成 `[devId: Bool]`,每个设备可以单独启停。
2. **图表查询放开到 24 小时**:加载更多数据。
3. **早起报告时间 UI**:加个 TimePicker。
4. **报警响铃策略**:iOS 用 `AVAudioSession` 后台音频 + 主动唤醒,尽量逼近 Android 体验。
5. **数据云端同步**:接入云平台,跨设备(换手机)同步配置。
6. **Widget**:iOS Today Widget 显示当前所有设备温度。
7. **Apple Watch**:用 watchOS 显示温度 + 报警震动。

---

## 12. 里程碑建议

| 里程碑 | 内容 | 工期估 |
|-------|------|--------|
| M1 | 项目搭建、品牌色板、基础导航、注册登录 | 1 周 |
| M2 | MacIdBook、DeviceParamsBook、本地存储 | 1 周 |
| M3 | BLE 通信 + AES 解密 | 1.5 周 |
| M4 | MQTT 通信 + SSL | 1 周 |
| M5 | 多设备卡片墙 + sparkline | 1 周 |
| M6 | 守护模式 + 报警系统(震动/响铃/通知) | 1.5 周 |
| M7 | 每设备参数 + 详情页 + 图表 + 报警列表 | 1.5 周 |
| M8 | 早起报告(BGTaskScheduler) | 1 周 |
| M9 | 测试 + 优化 + 提审 | 1.5 周 |
| **总计** | | **11 周** |

---

## 13. 文件参考

Android 源码参考路径:
- `app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` - BLE/MQTT 通信 + 报警逻辑
- `app/src/main/java/com/jinyuni/dengbei_care/TemperatureDatabaseHelper.kt` - 数据库 schema + 迁移
- `app/src/main/java/com/jinyuni/dengbei_care/MacIdBox.kt` - 设备 ID 管理
- `app/src/main/java/com/jinyuni/dengbei_care/DeviceParamsBook.kt` - 每设备参数管理
- `app/src/main/java/com/jinyuni/dengbei_care/ui/home/` - 首页卡片墙
- `app/src/main/java/com/jinyuni/dengbei_care/ui/detail/` - 设备详情页
- `app/src/main/java/com/jinyuni/dengbei_care/ui/dashboard/` - 全局参数设置
- `app/src/main/java/com/jinyuni/dengbei_care/report/` - 早起报告
- `app/src/main/res/values/colors.xml` - 品牌色板
- `app/src/main/res/values/dimens.xml` - 4dp 网格

iOS 版实现时应逐文件对照,确保功能对等。

---

## 14. 联系与维护

- 产品负责人:Kason
- 邮箱:`snack_yx@126.com`
- 文档版本:v1.0
- 最后更新:2026-07-14
