# Android 真机 / 模拟环境可行性探索（AF-001）

状态：探索完成，结论 **WORK_FAILED**（真机关键行为未证实 + 当前候选在所选真机系统上存在会阻断 BLE 读取的权限门缺陷）
执行角色：Android 工程
日期：2026-10-27
被测产品源码：`D:\mypro\beiwo2\AIP`（Git HEAD `4d101ae`，产品基线 `f38b5a21e5f13120c1b8a09f6b98c7447779d584`）
关联需求：`readme.txt` 修改点 3（3 分钟采集）、4（条件上报）、6（同步 Android）；`artifacts/android_architecture.md`（AA-002）
授权范围：本次能力仅获 `usb_list_devices` 设备工具 + 原生 `read/bash/edit/write/grep/find/ls`；设备入口只有 `android_readonly_inspection`（有界身份/状态/日志查询）。**未**安装/卸载应用、**未**切换蓝牙或定位开关、**未**烧录、**未**探测服务器。

---

## 0. 结论摘要

| 问题 | 结论 | 依据 |
|---|---|---|
| 真机是否具备 BLE 硬件能力 | **是**（`android.hardware.bluetooth_le` 存在） | §2 T1 |
| 真机当前是否具备执行 BLE 读取的条件 | **否**：应用未安装、蓝牙关闭（从未开启）、定位关闭、无默认网络 | §2 T2–T5 |
| 当前候选能否在该真机上跑通 BLE 读取 | **不能（代码级确定）**：API 29 上服务要求 `ACCESS_FINE_LOCATION`，而正常启动流程从不请求该运行时权限 | §3 F-1 |
| 模拟器能否代替真机取证 | **不能**：虚拟蓝牙控制器（无射频）、API 34/Android 14、已装应用为旧版 1.7 | §4 |
| 网关身份/地址是否与设计一致 | **未确认**：代码硬编码 MAC，且无 BLE 扫描/发现实现；环境登记也标注地址未确认 | §3 F-2 |
| 云端上传链路的设备侧可行性 | **未证实**：真机无网络；服务器入口未在本能力授权内 | §3 F-5 |
| 是否可执行设备 UI/BLE 实验 | **否**：本能力有效设备工具仅 `usb_list_devices`，无 `android_test`/uiautomator2 | §1.3 |

判定说明：不是"模拟器通过"，也不是"真机连接失败"。真机侧**未执行**连接；失败原因是关键可行性缺证据，且静态证据表明当前构建在该机型上必然跳过 BLE 读取，属于需要实现侧修复后再复测的业务失败。

---

## 1. 目标、问题与证据方法

### 1.1 必须证实的设备能力（探测前列出）

| ID | 待证实项 | 为什么影响 Android 方案 |
|---|---|---|
| Q1 | 真机 Android 版本 / API 级别 / OEM 定制 | 决定运行时权限模型（<31 用 `ACCESS_FINE_LOCATION`，≥31 用 `BLUETOOTH_SCAN/CONNECT`）、前台服务类型与后台限制 |
| Q2 | 真机 BLE 中央设备能力与当前蓝牙/定位状态 | BLE GATT 连接网关、扫描发现的前提 |
| Q3 | 应用在真机上的安装/版本身份 | 旧 APK（1.0）和旧模拟器安装（1.7）不能代表当前候选（源码 9/1.8） |
| Q4 | 真机网络与服务器可达前提 | MQTT 8883 / HTTP 5000 上传路径需要真实网络 |
| Q5 | 真机 OEM（MIUI）对前台服务/后台轮询的影响 | 3 分钟轮询与长驻 `dataSync` 服务能否存活 |
| Q6 | 模拟器可替代范围 | 决定哪些结论只能来自真机 |

### 1.2 方法（有限、只读）

1. 读取 `readme.txt`、`file_manifest.txt`、`artifacts/android_architecture.md`、`dengbei_care/app/build.gradle.kts`、`AndroidManifest.xml` 与 BLE/权限相关 Kotlin 源码（用 CodeGraph 索引定位到 `MqtttService` / `CloudUploadRepository` 后按行阅读）。
2. `adb -s 12f046690506`（登记入口）执行有界只读查询：属性、feature、蓝牙/定位/网络/电源状态、包安装查询、logcat 历史。
3. 启动本地 AVD `Pixel_3a_API_34_extension_level_7_x86_64` 做**独立记录**的模拟器对照实验；结束后 `adb emu kill`，确认只剩真机在线。
4. 静态核对权限门与 BLE 连接路径（文件:行号见 §3）。

### 1.3 未执行的通道及原因

| 通道 | 状态 | 原因 |
|---|---|---|
| `android_app_testing`（adb + uiautomator2） | 本能力不可用 | 有效设备工具只有 `usb_list_devices`，缺 `android_test`；无法安装/启动/点按 |
| 安装当前候选到真机 | 未执行 | 探索阶段禁止安装/卸载；且会连生产 broker |
| 开蓝牙 / 开定位 | 未执行 | 属设备状态修改，本能力 `allowed_external_actions` 为空 |
| 服务器/数据库核验 | 未执行 | 服务器入口未在环境配置中确认，未授权，不猜测凭据 |

---

## 2. 真机原始事实（Redmi 8A，序列号 12f046690506）

### T1 身份与系统

```
$ adb -s 12f046690506 devices
List of devices attached
12f046690506    device

$ adb -s 12f046690506 shell 'getprop ro.product.model; getprop ro.product.brand; getprop ro.build.version.release; getprop ro.build.version.sdk; getprop ro.miui.ui.version.name; getprop ro.build.version.incremental; getprop ro.build.fingerprint'
Redmi 8A
Xiaomi
10
29
V125
V12.5.3.0.QCPCNXM
Xiaomi/olivelite/olivelite:10/QKQ1.191014.001/V12.5.3.0.QCPCNXM:user/release-keys
```

补充原始值：`ro.build.version.security_patch=2021-10-01`、`ro.product.cpu.abi=armeabi-v7a`、`wm size=720x1520`、`MemTotal=3719964 kB`。

### T2 BLE 硬件能力存在

```
$ adb ... shell 'pm list features'    # 过滤后
feature:android.hardware.bluetooth_le
feature:android.hardware.location
feature:android.hardware.location.gps
feature:android.hardware.location.network
```

预期：Redmi 8A 支持 BLE 4.2，可作 GATT 中央设备。实际：`bluetooth_le` feature 存在。**能力存在，不等于已连通过真实网关。**

### T3 蓝牙当前关闭且从未开启

```
$ adb ... shell dumpsys bluetooth_manager
Bluetooth Status
  enabled: false
  state: OFF
  address: 00:00:00:BA:30:DE
  name: 2342485495的Redmi 8A

Bluetooth never enabled!
Bluetooth crashed 0 times

0 BLE appsregistered
Bluetooth Service not connected
```

`settings get global bluetooth_on` = `0`。含义：本机自本次开机以来蓝牙适配器从未启用，没有可用的已连接 GATT 设备，也没有历史 BLE 会话证据。

### T4 定位与网络

```
$ adb ... shell 'settings get secure location_mode'
0                      # 定位总开关关闭

$ adb ... shell 'dumpsys connectivity | head -8'
Active default network: none
Current Networks:
Restrict background: false

$ adb ... shell 'settings get global wifi_on; settings get global airplane_mode_on; getprop gsm.sim.state'
0
0
ABSENT,ABSENT
```

含义：Wi-Fi 关闭、飞行模式关闭、无 SIM；当前无默认数据网络。任何 MQTT/HTTP 云端链路在当前条件下不可执行。

### T5 当前候选未安装（不存在可复用的真机运行证据）

```
$ adb ... shell 'pm list packages com.jinyuni'
(空)
$ adb ... shell 'dumpsys package com.jinyuni.dengbei_care'
Package Changes:
  Sequence number=1
  User 0:            # 仅 PackageManager 变更记录，无包信息
$ adb ... shell 'logcat -d -t 4000 | grep -iE "jinyuni|MqttService|Bluetooth permission not granted"'
(无输出)
```

### T6 电源状态

```
$ adb ... shell 'dumpsys battery | head -12'
AC powered: true
level: 41
settings get global low_power = 0
```

---

## 3. 源码路径核查（真机相关，按文件:行号）

> 以下为静态事实，不是真机运行结果；用于解释为什么当前条件下无法得到真机 BLE 证据。

### F-1（阻断级）API 29 上 BLE 读取门的权限要求与请求流程不一致

- `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt:552-560`
  ```kotlin
  private fun checkBluetoothPermission(): Boolean {
      return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
          ... BLUETOOTH_CONNECT ...
      } else {
          ... ACCESS_FINE_LOCATION ...   // API 29 走这里
      }
  }
  ```
- `MqtttService.kt:583-590`：`connectToBluetoothDevice()` 首先 `if (!checkBluetoothPermission()) { Log.i("MqttService","Bluetooth permission not granted"); return }`，随后才检查蓝牙开关并连接。
- 请求侧：`MainActivity.kt:63-77` 启动时请求 `POST_NOTIFICATIONS`/`ACCESS_FINE_LOCATION` 的整段代码**被注释**；`:143-144` 仅 API 33+ 请求 `POST_NOTIFICATIONS`；`:267` 的 `REQUEST_CODE_LOCATION` 分支只有在被请求后才会进入。唯一仍会请求 `ACCESS_FINE_LOCATION` 的入口是 `ui/gateway/GatewayFragment.kt:185-192`（用户点"扫描 Wi-Fi"时）。
- 结论：Android 10 手机上正常启动 → 没有任何运行时权限弹窗 → `ACCESS_FINE_LOCATION` 保持未授予 → 每 180 s 的 BLE 轮询在 `MqtttService` 内直接返回，永远不会尝试 GATT。
- 附加事实：本应用是**直连固定 MAC**，不做 BLE 扫描，因此系统本身并不要求定位权限即可 `connectGatt`；该阻断来自应用自身的门，而不是 Android 的限制。若后续改用扫描发现，则 Android 10 还额外要求定位总开关开启（当前 `location_mode=0`）。
- 预期：授予所需权限后进入 GATT 连接。实际（静态）：该机型进入条件不成立。**真机执行未进行（应用未安装），此项需实现修复后在真机复测。**

### F-2 网关身份：硬编码 MAC，无发现流程

- `MqtttService.kt:261`：`private val deviceMacAddress = "84:C2:E4:03:02:02" // 替换为目标设备的 MAC 地址`
- 全仓 `startScan/startLeScan/LeScanCallback/ScanFilter/ScanSettings` 无 BLE 使用点（仅有 Wi-Fi `wifiManager.startScan()`）。
- 与环境登记"网关由本项目手机 App 发现并连接，实际设备名称和地址尚未确认"不一致：当前实现既不发现，也无法在地址变化时工作。
- 影响：真机联调前必须先确认网关 MAC/服务 UUID（FFE0/FFE2）与广播行为，否则"连接成功"无法归因到正确网关。

### F-3 前台服务与 OEM 后台策略（只能用真机验证）

- `AndroidManifest.xml`：`MqtttService` `foregroundServiceType="dataSync"`，声明 `FOREGROUND_SERVICE`、`FOREGROUND_SERVICE_DATA_SYNC`、`WAKE_LOCK`。
- `MqtttService.kt:1015`：`startForeground(NOTIFICATION_ID, notification)`（两参重载）；`MqtttService.kt:253` + `telemetry/SensorCadence.kt:14-17`：`READ_INTERVAL_MS = 180_000`。
- Android 14（模拟器）对 `dataSync` 前台服务有运行时长限制；Android 10（真机）没有同一条款，但 MIUI 12.5 的自启动/省电/后台清理策略是 OEM 行为，模拟器无法复现。**真机长时存活未验证。**

### F-4 版本身份

- 源码：`dengbei_care/app/build.gradle.kts` → `versionCode = 9`、`versionName = "1.8"`、`minSdk 26`、`targetSdk 34`、`compileSdk 34`。
- 仓库内参考 APK：`aapt2 dump badging dengbei_care/爱守护.apk` → `versionCode='1' versionName='1.0'`。**该 APK 不是当前候选**，不得作为安装/验收对象。

### F-5 云端链路依赖

- `cloud/CloudConfig.kt` 为服务器常量唯一来源（HOST 8.140.23.253、`ssl://…:8883`、`http://…:5000`）；`res/xml/network_security_config.xml` 放行该主机明文。
- 真机当前无网络（T4），且服务器入口未在本能力授权内 → 设备侧上传/持久化可行性**未证实**；不得用主机侧或模拟器推断真机结果。

---

## 4. 模拟器对照实验（独立记录，不充当真机证据）

### 4.1 环境与启动

| 项 | 值 |
|---|---|
| AVD | `Pixel_3a_API_34_extension_level_7_x86_64`（`google_apis` / `x86_64`，`PlayStore.enabled=false`） |
| AVD 配置 | `hw.gps=yes`；`config.ini` 无 `hw.bluetooth` 键；`hw.ramSize=1536` |
| 加速检查 | `emulator -accel-check` → `WHPX (10.0.26120) is installed and usable`（同时提示 Hyper-V 相关告警） |
| 启动命令 | `emulator.exe -avd Pixel_3a_API_34_extension_level_7_x86_64 -no-window -no-audio -no-boot-anim -no-snapshot -gpu swiftshader_indirect -port 5556` |
| 启动耗时 | `sys.boot_completed=1` 约 70 s |
| 清理 | `adb -s emulator-5556 emu kill` → `OK: killing emulator, bye bye`；随后 `adb devices` 仅剩真机 |

### 4.2 原始事实

```
$ adb -s emulator-5556 shell 'getprop ro.build.version.release; getprop ro.build.version.sdk; getprop ro.product.cpu.abi'
14
34
x86_64

$ adb -s emulator-5556 shell 'pm list features | grep -i blue'
feature:android.hardware.bluetooth
feature:android.hardware.bluetooth_le

$ adb -s emulator-5556 shell 'dumpsys bluetooth_manager | head -8'
Bluetooth Status
  enabled: true
  state: ON
  address: DA:4C:10:DE:17:00
  name: sdk_gphone64_x86_64
  time since enabled: 00:00:20.073

$ adb -s emulator-5556 shell 'settings get secure location_mode'
3

$ adb -s emulator-5556 shell 'dumpsys package com.jinyuni.dengbei_care | grep -E "versionCode=|versionName=|targetSdk|firstInstallTime|lastUpdateTime"'
    versionCode=8 minSdk=26 targetSdk=34
    versionName=1.7
    lastUpdateTime=2026-09-12 07:00:10
    firstInstallTime=2026-09-12 06:01:43
$ adb -s emulator-5556 shell 'pm list packages com.jinyuni.dengbei_care'
package:com.jinyuni.dengbei_care.test
package:com.jinyuni.dengbei_care
```

模拟器启动日志含：`INFO | Activated packet streamer for bluetooth emulation`。

### 4.3 模拟器证据的边界

- 蓝牙地址 `DA:4C:10:DE:17:00` 是模拟器虚拟控制器（rootcanal）身份，**没有物理射频**；无法扫描/连接真实 433→BLE 网关，也无法验证 RSSI、配对、MTU 协商等真实空口行为。
- 已安装的是 **versionCode 8 / 1.7**（2026-09-12），不是当前 9/1.8；模拟器上的任何旧结论都不能代表当前候选。
- 模拟器为 Android 14/API 34：`BLUETOOTH_SCAN/CONNECT`、`POST_NOTIFICATIONS` 为运行时权限，`dataSync` 前台服务受限；真机为 Android 10/API 29：`BLUETOOTH_SCAN/CONNECT` 与通知权限不存在，定位权限用于扫描，前台服务无同等时长限制。**同一份代码在两端的权限分支和后台行为不同。**

---

## 5. 真机 vs 模拟器差异（对 Android 设计的影响）

| 维度 | 真机（12f046690506） | 模拟器（AVD API 34） | 对设计/验证的影响 |
|---|---|---|---|
| 系统 | Android 10 / API 29 / MIUI V12.5.3.0 | Android 14 / API 34 / AOSP Google APIs | 权限分支、FGS 规则不同；模拟器 PASS/FAIL 不可外推 |
| BLE 权限 | 无 `BLUETOOTH_SCAN/CONNECT`；BLE 扫描需 `ACCESS_FINE_LOCATION` + 定位开启；直连 GATT 不需定位 | `BLUETOOTH_SCAN/CONNECT` 运行时权限 | 需按 API 级别分流，不能只测一端 |
| 蓝牙适配器 | 存在但关闭，自开机从未启用 | 虚拟适配器开启（`DA:4C:…`） | 真机 BLE 行为完全未验证；模拟器无射频 |
| 定位 | `location_mode=0` | `3` | 若改用扫描发现，真机还需用户开启定位总开关 |
| 通知权限 | API 29 无运行时通知权限 | API 33+ 需 `POST_NOTIFICATIONS` | 通知/报警在两端触发条件不同 |
| 前台服务 | API 29，无 dataSync 时长上限 | API 34，`dataSync` 受时长限制 | 长驻轮询的真实风险在 OEM（MIUI），不在模拟器 |
| OEM 后台策略 | MIUI 自启动/省电/清理，未验证 | 无 OEM 策略 | 3 分钟轮询存活必须真机长跑 |
| 已安装版本 | 未安装 | 8 / 1.7（旧） | 两者都不能作为当前 9/1.8 的运行证据 |
| 网络 | 无默认网络、无 SIM、Wi-Fi 关 | 主机 NAT 网络 | 上传/MQTT 只能在真机联网后验证 |
| 参考 APK 身份 | `爱守护.apk` = 1 / 1.0 | — | 禁止把该 APK 当作候选 |

---

## 6. 已尝试、已排除与未排除

### 6.1 已尝试

1. 真机只读身份/状态/包/日志查询（全部成功，见 §2）。
2. 模拟器冷启动 + 系统/蓝牙/定位/已装包查询（成功，见 §4）。
3. 源码级权限门与 BLE 连接路径核对（`MqtttService`、`MainActivity`、`GatewayFragment`、`SensorCadence`、`CloudConfig`、Manifest、Gradle）。
4. 参考 APK 版本核对（`aapt2 dump badging`）。

### 6.2 已排除的假设

| 假设 | 排除依据 |
|---|---|
| "真机已装当前 App，可直接看日志判断" | `pm list packages com.jinyuni` 为空；logcat 无相关记录 |
| "真机蓝牙可用，随时能连网关" | 蓝牙 OFF 且 `Bluetooth never enabled!`；`bluetooth_on=0` |
| "模拟器结果可以代表真机 BLE" | 虚拟控制器无射频、API 34 vs 29、模拟器装的是旧版 1.7 |
| "仓库内 爱守护.apk 就是当前候选" | badging 为 1 / 1.0，源码为 9 / 1.8 |
| "真机缺少 BLE 硬件" | `android.hardware.bluetooth_le` 存在 |
| "BLE 读取被系统限制" | 应用自身 `checkBluetoothPermission()` 在 API 29 要求从未被请求的定位权限 |
| "手机能直接跑云端上传验证" | 无默认网络（`Active default network: none`），无 SIM |

### 6.3 未排除（影响方案，尚无证据）

- U-1：MIUI 12.5 是否会在息屏/省电下限制或杀死 `dataSync` 前台服务与 3 分钟轮询。
- U-2：真实网关的 BLE 名称/MAC/服务 UUID 与 `84:C2:E4:03:02:02` 是否一致；网关当前是否上电、在范围内。
- U-3：真机 GATT 连接、MTU 240、FFE0/FFE2 读取、AES/ECB 解密在该机型协议栈上的实际行为。
- U-4：真机联网后 MQTT 8883（TLS）与 `POST /upload_data` 的时延、失败与重试行为（含服务端幂等）。
- U-5：Android 10 上 `SCHEDULE_EXACT_ALARM`/WorkManager 早报在该 OEM 上的实际调度精度。
- U-6：安装 9/1.8 覆盖安装/数据保留路径（需安装授权）。

---

## 7. 对 Android 设计的影响（不在此能力实施）

1. **权限门必须先修**：API < 31 的 BLE 直连不应要求 `ACCESS_FINE_LOCATION`；如保留该门，则必须在启动/进入首页时请求并处理拒绝与"不再询问"。否则 3 分钟采集节拍在该真机上完全空转（且日志只有一行 `Bluetooth permission not granted`，用户无感）。
2. **网关发现/身份**：硬编码 MAC 与"App 发现并连接"的需求不符；设计需明确选择"按服务 UUID 扫描 + 首次绑定保存 MAC"或"用户输入/扫码 MAC"，并把网关身份写入运行记录。
3. **真机长跑是独立验收项**：3 分钟轮询 + 前台服务存活必须在 MIUI 真机上跨息屏/充电/省电模式验证；模拟器不能替代。
4. **版本门禁**：真机安装/验收必须使用由当前源码重建的 9/1.8 构建并记录 APK 哈希；不得使用 `爱守护.apk`（1.0）或模拟器里的 1.7。
5. **网络前置条件**：真机联调前需开启 Wi-Fi/数据；服务器侧核验需另行授权，不并入 Android 结论。

---

## 8. 最小下一步与必要人工动作

| 顺序 | 动作 | 责任 | 前置条件 |
|---|---|---|---|
| 1 | 修复 API<31 BLE 权限门与请求流程；决定并实现网关发现/绑定方式 | Android 工程（实现能力） | 本报告 F-1/F-2 |
| 2 | 从当前源码重建 9/1.8，记录 APK/提交身份 | Android 工程（构建） | 构建环境（已具备：Gradle 8.4 / JDK 21 / compileSdk 34） |
| 3 | 真机开启蓝牙、定位、Wi-Fi/数据；确认网关上电并在范围内 | 用户（人工） | 设备状态修改不在本能力授权内 |
| 4 | 授权 `android_app_testing`（`android_test` 工具）后安装当前构建，验证权限弹窗、GATT 连接/读取、解密、界面显示 | 软件测试（真机独立验收） | 步骤 1–3 完成 |
| 5 | 跨息屏/省电/充电长跑 ≥ 2 个采样周期（≥ 6 min），记录服务存活与上报时间戳 | 软件测试 | 步骤 4 |
| 6 | 真机联网验证 MQTT 与 `/upload_data`，并由服务器侧核验持久化 | 软件测试 + 服务器能力（需授权入口） | 网络与服务器入口确认 |
| 7 | 若仅需复现本报告的静态结论：在任意 API 29 设备/模拟器安装后观察 `MqttService: Bluetooth permission not granted` | Android 工程 | 可先做，不依赖网关 |

---

## 9. 未执行项与限制（如实登记）

1. 未安装/启动任何应用到真机；未开蓝牙/定位；未修改任何设备设置；未清数据。
2. 未做真机 BLE 连接，因此"真机 BLE 可用/不可用"没有被执行性证据；本报告的阻断结论来自代码路径与系统 API 级别的确定性推理。
3. 模拟器实验只证明"模拟器有虚拟蓝牙、系统为 API 34、装的是旧版 1.7"，不证明任何真机或网关行为。
4. 未探测服务器（8.140.23.253:8883/5000），未读取数据库；无授权入口。
5. 本次能力有效设备工具仅 `usb_list_devices`，未使用 `android_test`/uiautomator2；UI 层可行性未验证。
6. 命令均有明确超时；模拟器进程已按 ID 关闭并确认退出，未遗留后台任务。

---

## 10. 复现命令（只读）

```bash
ADB="D:/software/platform-tools-latest-windows/platform-tools/adb.exe"
"$ADB" devices
"$ADB" -s 12f046690506 shell 'getprop ro.product.model; getprop ro.build.version.release; getprop ro.build.version.sdk; getprop ro.miui.ui.version.name'
"$ADB" -s 12f046690506 shell 'pm list features' | grep bluetooth_le
"$ADB" -s 12f046690506 shell dumpsys bluetooth_manager | head -12
"$ADB" -s 12f046690506 shell 'settings get secure location_mode; settings get global bluetooth_on; settings get global wifi_on'
"$ADB" -s 12f046690506 shell 'dumpsys connectivity | head -8'
"$ADB" -s 12f046690506 shell 'pm list packages com.jinyuni'
"D:/mylearn/android_jdk/build-tools/34.0.0/aapt2.exe" dump badging dengbei_care/爱守护.apk | head -3
```
