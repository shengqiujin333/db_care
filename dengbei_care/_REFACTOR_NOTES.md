# 重构笔记 - 多设备 + 早起报告 + Material3

> 这份文件是项目内的"持久化记忆",防止上下文压缩后丢失进度。
> 每完成一个步骤就回来更新对应的 checkbox 和"当前进度"。
> 详细方案见 `C:\Users\kason\.claude\plans\spicy-swimming-sphinx.md`。

---

## 续接指针(每次开干先看这里)

- **当前阶段**: Phase 3 完成,准备开始 Phase 4
- **下一步**: Phase 4.1 重写 `themes.xml`(启用 `Base.Theme.Dengbei_care` Material3)
- **已完成的文件**:
  - Phase 1:`TemperatureDatabaseHelper.kt`、`DeviceState.kt`、`HomeViewModel.kt`、`MqtttService.kt`(多设备路由 + 报警落库)
  - Phase 2:`HomeFragment.kt`、`DeviceCardAdapter.kt`、`item_device_card.xml`、`DeviceDetailFragment.kt`、`fragment_device_detail.xml`、`AlarmEventAdapter.kt`、`item_alarm_event.xml`、`mobile_navigation.xml`、`BaseActivity.kt`、`strings.xml`、`ic_arrow_back.xml`、`bg_alarm_dot.xml`
  - Phase 3:
    - `report/ReportGenerator.kt` - 规则文本生成,数据类 DailyReport(DeviceReportSection)
    - `report/DailyReportWorker.kt` - CoroutineWorker,查昨日数据+发通知+缓存到 SharedPreferences
    - `report/ReportScheduler.kt` - scheduleDaily(hhmm)/runNow()/cancel()
    - `report/ReportActivity.kt` + `activity_report.xml` - 报告页,observe WorkInfo live update,重新生成按钮
    - `MqtttService.kt:createNotificationChannel()` - 加 `daily_report_channel`
    - `fragment_notifications.xml` + `NotificationsFragment.kt` - 加"查看今日报告"按钮
    - `MainActivity.kt` - onCreate 调 `scheduleDailyReport()` + setupDefaultPreferences 加 `report_time="07:00"`
    - `AndroidManifest.xml` - 注册 ReportActivity
- **待解决的问题**: 无
- **构建状态**: `./gradlew assembleDebug` BUILD SUCCESSFUL(2026-07-14)
- **重要注意事项**: 
  - HomeViewModel legacy LiveData(`_tempentries`/`_humientries`/`_mqttData`/`_drawFlag`)MqtttService 仍在写,Phase 4 清理
  - WorkManager PeriodicWorkRequest 漂移几分钟,v1 接受
  - 卡片状态 chip 用现有 teal/purple/red 颜色,Phase 4 换品牌色

---

## 绝对不能动的内容(用户明确要求:动了会失联)

涉及网络传输、加密、设备配对的所有常量和协议格式必须**原样保留**。重构只能改"数据怎么分流/存储/显示",不能改"数据怎么来/怎么解密"。

### MQTT 相关(`MqtttService.kt`)
- **Broker URI**: `ssl://117.72.84.210:8883` (line 104)
- **Client ID**: `android_client` (line 105)
- **用户名构造**: 用 `mac_addr` SharedPreferences 值 (line 1266-1270)
- **密码构造**: `savedInput + "&^!A:z?"` (line 1271) - **这个拼接规则绝不能改**
- **CA 证书**: assets 里的 `jd-ca.crt` (line 1253) - 不能删,不能换
- **SSL 版本**: TLSv1.2 (line 1263)
- **KeepAlive**: 20 秒 (line 1278)
- **Connection timeout**: 30 秒 (line 1201)

### MQTT 主题格式(`MqtttService.kt:1019-1025`)
- `rtopic0 = "/$macAddr/a/r0"` - 读主题0
- `rtopic1 = "/$macAddr/a/r1"` - 读主题1
- `stopic0 = "/$macAddr/a/s0"` - 发送主题
- 默认 `/topic/get_0123`、`/topic/get_01234`、`/topic/test0` (line 106-112)
- 这些字符串里的 `a`、`r0`、`r1`、`s0` 是协议约定,不能改

### MQTT Payload 解析格式(`MqtttService.kt:1310-1614`)
- payload 格式: `time:temp1,temp2,...:humi1,humi2,...` 三段冒号分隔
- 解析逻辑 `parts[0]=时间`、`parts[1]=温度逗号列表`、`parts[2]=湿度逗号列表` 不能改
- 时间校准 payload 模板: `publish_start_data + ",$currtime,3.1,3.1,3.1,3.1,3.1,3.1,1,1"` (line 1607) - 那堆 `3.1` 和最后的 `1,1` 是固件约定,不能改

### BLE 蓝牙相关(`MqtttService.kt`)
- **目标设备 MAC**: `84:C2:E4:03:02:02` (line 216) - 测试设备,不能改
- **GW_SERVICE_UUID**: `0000ffe0-0000-1000-8000-00805F9B34FB` (line 119)
- **GW_WRITE_UUID**: `0000ffe3-0000-1000-8000-00805F9B34FB` (line 118)
- **读特征 UUID**: `0000ffe2-0000-1000-8000-00805F9B34FB` (line 647, onServicesDiscovered 里)
- **MTU 协商值**: 240 (line 628, 1073)
- **读取间隔**: 5 分钟 `READ_INTERVAL_MS = 5 * 60 * 1000L` (line 208) - 这是行为参数,理论上可调但用户没要求改,保留

### 加密相关(`MqtttService.kt:128-133, 463-469`)
- **AES 密钥 `APP_AES_KEY16`** - 16 字节硬编码,**绝不能改**:
  ```
  0x91, 0x4E, 0x03, 0xB7, 0xC2, 0x5A, 0x88, 0x1D,
  0xF4, 0x60, 0x7B, 0x2E, 0xA9, 0x17, 0x6C, 0x55
  ```
- **解密模式**: `AES/ECB/NoPadding` (line 466) - 模式不能改
- **帧解析格式**: 16 字节块,前 6 字节网关 ID,第 7 字节 devCount,每个设备 16 字节(其中前 8 字节有效:4 字节 devId + 2 字节湿度×0.1 + 2 字节温度×0.1 带符号)
- **协议字节 0xA1**: 写入 payload 头部 (line 363, 379) - 不能改

### HTTP API(`ApiService.kt`)
- 5 个 endpoint 路径保持不变:
  - `/sendVerificationCode`
  - `/verifyCode`
  - `/register`
  - `/login`
  - `/setmac`
- 不要改 ApiService 接口签名(数据类字段也不要动)

### SharedPreferences Key 名(改了就读不到老数据)
- `MyPrefs` 文件名
- `mac_addr`、`registerphone`、`sensor`、`prov_flag`
- `measure_state`、`finish_guard_time`、`guardtime`
- `privacy_policy_accepted`
- `macid_name_map`(MacIdBook 用)
- 报警参数:`tempdrop`、`humdrop`、`tempmax`、`tempmin`、`hummax`、`hummin`、`delayalarm`、`dengbeialarm`、`yuzhialarm`、`zhendongalarm`、`xianglingalarm`
- `isFirstRun`(首次运行标志)

### 报警文本字符串(改了用户认不出来)
保留这些字面值:
- `"温度湿度下降报警"`
- `"温湿度超限报警"`
- `"紧急报警，温度超过65度，谨防火灾"`
- `"未设置报警"`
- `"无报警，一切正常"`(BaseActivity 里)
- 65 度紧急报警阈值(`newData.temperature > 65f`, line 938、1512)
- 报警去抖 900 秒(`900 * 1000`, line 1000、1573)

### 签名(`build.gradle.kts:27`)
- `signingConfig = signingConfigs.getByName("debug")` - 用 debug 签名
- `C:\Users\kason\.android\debug.keystore` 是签名文件(readme.txt 提到)

---

## 4 阶段进度跟踪

### Phase 1 - 数据层基础(后续所有阶段依赖)

- [x] **1.1 数据库迁移**(已完成)
  - 重写 `TemperatureDatabaseHelper.kt`
  - DATABASE_VERSION 1 -> 2
  - 新表结构:`temperature(time, device_id, temperature, humidity, PK(time, device_id))` + `alarm_events(id, time, device_id, type, message)`
  - `onUpgrade`:rename temperatureA -> temperature,加 device_id 列,用 `MacIdBook.all().firstOrNull()?.first ?: "UNKNOWN"` 回填,drop B/C 空表,建 alarm_events
  - 新 API:`storeTemperatureData(ctx, temp, humi, time, devId, storeflag)`、`getTemperatureHumidityByTimeRange(devId, ...)`、`getAllDevicesLatest()`、`getDailyStats(devId, ...)`、`getAlarmEvents(devId, ...)`、`getAllAlarmEvents(...)`、`getSparkline(devId, limit)`、`storeAlarmEvent(devId, type, msg, time)`
  - 新数据类:`DailyStats`、`AlarmEvent`
  - 临时改动:`MqtttService.kt:956/1526` + `HomeFragment.kt:570` 调用点用 `"UNKNOWN"` devId,1.4 / Phase 2 替换
- [x] **1.2 新建 DeviceState.kt**(已完成)
- [x] **1.3 重写 HomeViewModel.kt**(`Map<devId, DeviceState>`)(已完成,保留 legacy LiveData 等 Phase 2 删)
- [x] **1.4 重构 MqtttService.kt**(按 devId 路由 + 报警落库)(已完成)

### Phase 2 - 多设备 UI(卡片仪表盘)

- [x] **2.1** 重写 `fragment_home.xml`(RecyclerView + 空态 + toolbar 含守护按钮)
- [x] **2.2** 重写 `HomeFragment.kt`(observe devices)
- [x] **2.3** 新建 `DeviceCardAdapter.kt` + `item_device_card.xml`
- [x] **2.4** 新建 `DeviceDetailFragment.kt` + `fragment_device_detail.xml`(LineChart + 搜索 + 报警列表搬来)
- [x] **2.5** 修改 `mobile_navigation.xml` 加 detail destination

### Phase 3 - 早起报告

- [x] **3.1** 新建 `report/DailyReportWorker.kt`(CoroutineWorker)
- [x] **3.2** 新建 `report/ReportScheduler.kt`(scheduleDaily + runNow)
- [x] **3.3** 新建 `report/ReportGenerator.kt`(规则文本生成)
- [x] **3.4** 新建 `report/ReportActivity.kt` + `activity_report.xml`
- [x] **3.5** `NotificationsFragment` 加"查看今日报告"按钮
- [x] **3.6** `MainActivity` 初始化 scheduleDaily + `report_time="07:00"` 默认
- [x] **3.7** `AndroidManifest` 注册 ReportActivity
- [x] **3.8** `MqtttService.createNotificationChannel` 加 `daily_report_channel`

### Phase 4 - Material3 全量重做

- [ ] **4.1** 重写 `themes.xml`(启用 `Base.Theme.Dengbei_care` Material3)
- [ ] **4.2** 重写 `colors.xml`(品牌色板:温暖青绿 + 琥珀)
- [ ] **4.3** 重写 `dimens.xml`(4dp 网格)
- [ ] **4.4** 扫描所有布局替换硬编码(尺寸/颜色/系统 drawable)
- [ ] **4.5** 重写 `fragment_zhuce.xml`(最乱的一个)
- [ ] **4.6** 新建 drawable:卡片背景、状态 chip、sparkline 圆背景、删除 icon
- [ ] **4.7** 重做 launcher icon(心形+盾牌)
- [ ] **4.8** 清理 `build.gradle.kts:74/76` 重复的 `workruntime` 声明
- [ ] **4.9** 修改 `strings.xml`:`title_home` "数据"->"首页"

---

## 决策日志(关键选择记下来,避免后期反复)

1. **MQTT 设备归属用位置匹配**: payload `time:t1,t2:...,h1,h2,...` 不带 devId。按 `MacIdBook.all()` 顺序的 index i 对应 `t[i]/h[i]`。**前提是网关按 saved IDs 顺序回传** - 如果实测发现错位,需要改协议让网关带 devId。位置不匹配时加 `Log.w` 告警。
2. **老数据迁移策略**: table A 老数据全部回填 `device_id = MacIdBook.all().firstOrNull()?.first ?: "UNKNOWN"`。原 app 单设备场景,可接受。
3. **守护按钮位置**: 从老 HomeFragment 搬到新 home 的 toolbar,保留 `measure_state`/`finish_guard_time` SharedPreferences 持久化和倒计时逻辑。
4. **ReportActivity 用 Activity 而非 Fragment**: 从通知 PendingIntent 启动更简单,代价是不能用 NavController。
5. **WorkManager 定时精度**: 接受 `PeriodicWorkRequest` 的漂移(可能差几分钟),v1 不引入 AlarmManager 精确闹钟。
6. **图表库**: 保留 MPAndroidChart,迷你 sparkline 和详情页都用它。
7. **品牌色板**: 温暖青绿 #00897B(主)+ 暖琥珀 #FFB300(次)+ #FAFAFA(底)+ #D32F2F(报警红)。关怀/医疗感。
