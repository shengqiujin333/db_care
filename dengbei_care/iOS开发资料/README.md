# 爱守护 iOS 版 - 开发资料包

> 配套文档:`01_需求文档/iOS版功能需求文档.md` + `01_需求文档/iOS开发所需资料清单.md`
> 来源:Android v1.7 (dengbei_care, 2026-07-16)
> 整理人:Claude Code

---

## 📁 文件夹结构

```
iOS开发资料/
├── README.md                          ← 本文件(索引 + 关键信息速查)
│
├── 01_需求文档/                       ← 先看这里
│   ├── iOS版功能需求文档.md           ← 主需求文档(14 章,全功能+UI+里程碑)
│   ├── iOS开发所需资料清单.md         ← 本资料包对应的清单
│   └── 用户使用说明书.md              ← 终用户视角的功能说明(辅助理解)
│
├── 02_Tier1_硬前置/                   ← M3/M4 开工必需,优先看
│   ├── MqtttService.kt              ← BLE+MQTT+AES+报警逻辑(一个文件解四件事)
│   ├── ZhuCeViewModel.kt            ← 云平台 API 调用(Retrofit base URL 在这)
│   ├── ApiService.kt                ← API 端点定义 + 数据类
│   └── CA证书/
│       ├── jd-ca.crt                ← MQTT SSL CA 证书(主用)
│       └── emqxsl-ca.crt            ← EMQX SSL CA 证书(备用)
│
├── 03_Tier2_行为对齐/                 ← M1/M2/M7 用
│   ├── MacIdBox.kt                  ← 设备 ID 校验 + 存储
│   ├── DeviceParamsBook.kt          ← 每设备参数 key 格式
│   └── TemperatureDatabaseHelper.kt ← SQLite schema + v1→v2 迁移
│
├── 04_Tier3_UI对齐/                  ← M5/M7/M8 用
│   ├── ui_home/                     ← 卡片墙
│   ├── ui_detail/                   ← 详情页 + 报警列表
│   ├── ui_dashboard/                ← 设定页(全局参数 + 设备选择)
│   ├── ui_zhuce/                    ← 注册登录
│   ├── report/                      ← 早起报告生成
│   └── res_values/                  ← colors/dimens/themes/strings
│
└── 05_布局与资源/                    ← UI 还原参考
    ├── layout/                       ← 所有 fragment/activity 的 XML 布局
    ├── drawable/                     ← 图标 + 状态 chip 背景
    └── assets/                       ← 隐私政策/服务条款 HTML
```

---

## 🔑 关键信息速查(硬前置)

### A. BLE GATT

| 项目 | 值 |
|------|-----|
| Service UUID | `0000ffe0-0000-1000-8000-00805F9B34FB` |
| Write Char UUID | `0000ffe3-0000-1000-8000-00805F9B34FB` |
| Notify Char (frame) | `0000ffe2-0000-1000-8000-00805F9B34FB` |
| 其他特征 | `0000ffe1` / `0000ffe4` / `0000ffe5`(见 MqtttService.kt) |

### B. AES 解密

- 算法:**AES/ECB/NoPadding**
- 密钥长度:16 字节
- 密钥(十六进制):`91 4E 03 B7 C2 5A 88 1D F4 60 7B 2E A9 17 6C 55`
- 密钥(Kotlin byteArrayOf,直接抄):见 `02_Tier1_硬前置/MqtttService.kt:129-134`
- 解密入口:`MqtttService.decryptAndParseEcbFrame(value: ByteArray, key16: ByteArray)` (line 486)
- 解密后字节流解析:`parseHexData()` (line 776)

### C. MQTT 服务器

| 项目 | 值 |
|------|-----|
| Broker URI | `ssl://117.72.84.210:8883` |
| CA 证书 | `02_Tier1_硬前置/CA证书/jd-ca.crt` |
| Username | 用户在 app 里设的 `mac_addr`(SharedPreferences "MyPrefs" 的 "mac_addr" key),12 位数字 |
| Password | `<mac_addr>&^!A:z?` (用户名后接字面量 `&^!A:z?`) |
| Client ID | 每次启动随机生成 |
| Keep Alive | 20 秒 |
| 协议 | TLSv1.2 |

> ⚠️ 密码规则在 `MqtttService.kt:1346`: `(savedInput + "&^!A:z?").toCharArray()`

### D. MQTT Topic

| 方向 | Topic 模板 | 用途 |
|------|-----------|------|
| 订阅(收) | `/<macAddr>/a/r0` | 温度回传 |
| 订阅(收) | `/<macAddr>/a/r1` | 湿度回传 |
| 订阅(收) | `/<macAddr>/a/s0` | 状态回传 |
| 发布(发) | `/<macAddr>/s/r0` | 守护启停 |
| 发布(发) | `/<macAddr>/s/p0` | 参数下发 |

### E. MQTT Payload 格式

```
<time>:<temp1,temp2,...>:<humi1,humi2,...>
```
- `time`:Unix 秒(Long)
- `temp1,temp2,...`:按 saved IDs 顺序的温度值(Double)
- `humi1,humi2,...`:同上,湿度

> ⚠️ payload 不含设备 ID,靠 `MacIdBook.all()[i]` 位置匹配。如固件不能保证顺序,会错位。

### F. 云平台 API

| 项目 | 值 |
|------|-----|
| Base URL | `http://117.72.84.210:5000` |
| 协议 | HTTP(未上 HTTPS,需 iOS ATS 例外) |

端点(全部 POST,Content-Type: application/json):

| 端点 | 请求体 | 响应 |
|------|--------|------|
| `/sendVerificationCode` | `{"phoneNumber": "..."}` | Void |
| `/verifyCode` | `{"phoneNumber": "...", "code": "..."}` | Void |
| `/register` | `{"phoneNumber": "...", "password": "..."}` | Void |
| `/login` | `{"phoneNumber": "...", "password": "..."}` | `{"success": Bool, "token": String?}` |
| `/setmac` | `{"phoneNumber": "...", "macAddress": "..."}` | Void |

详见 `02_Tier1_硬前置/ApiService.kt`。

---

## ❓ 待 Kason 拍板的产品决策(对应清单 Q4-Q9)

> 这些是产品决策,不是协议问题。建议默认值如下,如有调整请直接告诉 iOS 开发。

| # | 问题 | 建议默认 | 理由 |
|---|------|---------|------|
| Q4 | BLE 直连是否还用? | **保留** | Android 版还在用,且本地 BLE 不依赖云服务,稳定性更好;iOS 用 CoreBluetooth state restoration 能保住后台 |
| Q5 | BLE 和 MQTT 同时开时去重? | **按 (time, device_id) 去重** | 同一条数据会从 BLE 和 MQTT 都进来一次,不去重会导致数据翻倍 + 报警双触发 |
| Q6 | 最低 iOS 版本? | **iOS 16** | 用 SwiftCharts、`.timeSensitive` 通知、后台音频;iOS 14 需引 DGCharts 并降级,工作量增加 1 周 |
| Q7 | 守护模式全局还是每设备? | **先做全局(对齐 Android)** | 后续可在 v2 加每设备守护 |
| Q8 | 图表时间范围放开到 24 小时? | **放开** | iOS SQLite.swift 性能足够;Android 限 50 分钟是历史遗留 |
| Q9 | 早起报告时间加 TimePicker? | **加** | 工作量小,用户期望高 |

---

## 🚀 开工建议

### 第一步:看文档
1. `01_需求文档/iOS版功能需求文档.md` - 整体架构和功能边界
2. `01_需求文档/用户使用说明书.md` - 用户视角的功能流(辅助理解)

### 第二步:对协议
1. 打开 `02_Tier1_硬前置/MqtttService.kt`
2. 重点看 5 处:
   - `APP_AES_KEY16` (line 129) - AES 密钥
   - `setupSSL()` (line 1323) - MQTT SSL + 用户名/密码
   - `decryptAndParseEcbFrame()` (line 486) - BLE 解密
   - `parseHexData()` (line 776) - 字节流解析
   - `processTemperatureHumidityData()` (line 801) - 报警逻辑
3. CA 证书:`02_Tier1_硬前置/CA证书/jd-ca.crt` 拖进 iOS 工程

### 第三步:对 API
1. `02_Tier1_硬前置/ApiService.kt` - 端点定义
2. `02_Tier1_硬前置/ZhuCeViewModel.kt:35` - Retrofit base URL

### 第四步:对存储
1. `03_Tier2_行为对齐/MacIdBox.kt` - 设备 ID 规则
2. `03_Tier2_行为对齐/DeviceParamsBook.kt` - 每设备参数 key 格式
3. `03_Tier2_行为对齐/TemperatureDatabaseHelper.kt` - SQLite schema

### 第五步:对 UI
- 看 `04_Tier3_UI对齐/` 下各模块的 Kotlin 文件
- 看 `05_布局与资源/layout/` 下对应的 XML 布局
- 配色见 `04_Tier3_UI对齐/res_values/colors.xml`

---

## ⚠️ 安全说明

- AES 密钥、MQTT 密码规则、CA 证书等敏感信息,最终会以常量进 iOS 工程源码(和 Android 一样),这是对等移植的既定做法。
- 云平台 base URL 是 HTTP 不是 HTTPS,iOS 需要在 `Info.plist` 加 `NSAppTransportSecurity` 例外,或服务端上 HTTPS。
- 所有敏感信息**不要**改值,改了无法与现有网关/服务端通信。

---

## 📞 联系

- 产品负责人:Kason
- 邮箱:`snack_yx@126.com`
- Android 源码:`D:\mylearn\android_app\dengbei_care`
- 资料包整理日期:2026-07-16
