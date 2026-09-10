# 爱守护 iOS 版 - 开发所需资料清单

> 配套文档:`iOS版功能需求文档.md`
> 用途:开发 iOS 版前,需要补齐的**协议细节确认**与**Android 源码文件**
> 维护:Claude Code
> 日期:2026-07-16

---

## 0. 背景

iOS 版要和现有 `beiwov2` 固件网关 + 云端 MQTT 对接。数据链路:

```
传感器 ──433MHz──> UM2006A ──(固件 beiwov2 / CH59x)──> ┌─ BLE 直连手机
                                                       └─ 云 MQTT(ssl://117.72.84.210:8883)──> 手机
```

本仓内已有的:
- ✅ 固件 GATT Service UUID = `0xFFE0`(在 `Profile/include/gattprofile.h`),特征 `0xFFE1`~`0xFFE5`
- ✅ 完整需求文档(功能/UI/里程碑)

本仓内**缺失的**(本文档要解决):BLE 协议细节、AES 密钥、MQTT 凭证、云 API、以及若干产品决策。

---

## 1. 需要确认的功能问题

### 1.1 协议 / 数据层(建议从 Android 代码核对)

| # | 问题 | 为什么需要 |
|---|------|-----------|
| Q1 | 温度/湿度解密后的**字节编码**?温度是「整数×10」还是浮点?负温度怎么表示?湿度同问 | BLE 解析核心,解析错则数据全错 |
| Q2 | 一个 BLE 包里含**几条读数**?devId 在包内,还是靠 BLE MAC 地址映射? | 决定解析循环和设备匹配方式 |
| Q3 | **"下降报警"**精确判定:5/10/15 分钟前三个时间点,是**全部满足**还是**任一**?且温度降 >tempdrop **与** 湿度降 >humdrop 是「且」还是「或」? | 报警逻辑不能靠猜 |

### 1.2 产品决策(需要产品负责人 Kason 拍板)

| # | 问题 | 选项 | 影响范围 |
|---|------|------|---------|
| Q4 | BLE 直连这条路现在还用吗?还是已全转 MQTT 云? | 保留 / 砍掉 / 降优先级 | 砍掉可省 M3(1.5 周)及 iOS 后台保活最麻烦的部分 |
| Q5 | BLE 和 MQTT 同时开时,同一设备同一条数据会进两次吗?是否按 (time, device_id) 去重?报警会否双触发? | 去重 / 不去重 / 只走一路 | 数据层与报警节流逻辑 |
| Q6 | 最低系统版本? | iOS 14 / **iOS 16(推荐)** | 16 可用 SwiftCharts、`.timeSensitive` 通知、后台音频;14 需引 DGCharts 并降级 |
| Q7 | 守护模式做全局还是每设备独立? | 全局(对齐 Android)/ 每设备(§11.2 建议) | §3.4 整套逻辑 |
| Q8 | 图表时间范围放开到 24 小时? | 是 / 否(§11.2 建议) | 详情页查询逻辑 |
| Q9 | 早起报告时间加 TimePicker 让用户改? | 是 / 否(§11.2 建议) | §3.9 |

---

## 2. 需要的 Android 源码文件

> 路径以 Android 工程根目录为准,对应需求文档 §13。

### 🔴 Tier 1 - 硬前置(不给则 M3/M4 无法开工)

| 文件 | 路径 | 解锁内容 |
|------|------|---------|
| **MqtttService.kt** | `app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` | 一个文件解决四件事:① BLE GATT 回调(确认订阅哪个特征、NOTIFY 通道)② `decrypt()` 里的 **AES-ECB 16 字节密钥** ③ MQTT connect 的**用户名/密码** ④ payload 解析 + 报警触发逻辑 |
| **MQTT CA 证书** | `app/src/main/res/raw/*.pem` | SSL 连接必需,缺它连不上 8883 |
| **ZhuCeViewModel.kt** | `app/src/main/java/com/jinyuni/dengbei_care/ZhuCeViewModel.kt`(或 ui/register 下) | 云平台**注册/登录/验证码 API 的域名与请求/响应格式** |

### 🟡 Tier 2 - 行为对齐(M1/M2/M7)

| 文件 | 路径 | 解锁内容 |
|------|------|---------|
| MacIdBox.kt | `app/src/main/java/com/jinyuni/dengbei_care/MacIdBox.kt` | 设备 ID 校验正则(中英文混合/长度规则)、`[id:name]` 存储格式 |
| DeviceParamsBook.kt | `app/src/main/java/com/jinyuni/dengbei_care/DeviceParamsBook.kt` | 每设备参数 `dev_<devId>_<param>` key 格式 + 读取 fallback 优先级 |
| TemperatureDatabaseHelper.kt | `app/src/main/java/com/jinyuni/dengbei_care/TemperatureDatabaseHelper.kt` | DB schema v1→v2 迁移细节(确认字段、类型、索引与 iOS 对齐) |

### 🟢 Tier 3 - UI / 文案对齐(M5/M7/M8)

| 文件 | 路径 | 解锁内容 |
|------|------|---------|
| 首页卡片墙 | `app/src/main/java/com/jinyuni/dengbei_care/ui/home/` | 卡片布局、状态 chip 颜色映射、空态、sparkline |
| 设备详情页 | `app/src/main/java/com/jinyuni/dengbei_care/ui/detail/` | 双 Y 轴图表、报警列表、时间范围搜索、右上角菜单 |
| 全局参数页 | `app/src/main/java/com/jinyuni/dengbei_care/ui/dashboard/` | 参数表单字段、默认值、校验 |
| 早起报告生成 | `app/src/main/java/com/jinyuni/dengbei_care/report/` | 报告**精确文案模板**(摘要/每设备段落/建议条目) |
| colors.xml + dimens.xml | `app/src/main/res/values/` | 可不给(需求 §6 已列色板),给了更稳 |

---

## 3. 交付建议

- **最省事路径**:先给 `MqtttService.kt` + `res/raw/*.pem` 两个文件。单这一个 .kt 能同时解锁密钥、MQTT 凭证、BLE 协议、报警逻辑四项。
- **格式**:原文件即可,不用转换;`.kt` / `.pem` / `.xml` 直接拷进 `wdk/` 下任意子目录(我建个 `android-ref/` 收着),不参与 iOS 工程编译。
- **密钥安全**:AES 密钥、MQTT 口令最终会以常量进 iOS 工程源码(和 Android 一样),这是对等移植的既定做法(需求 §8 明确"不能改")。如有合规顾虑请提前说明。

---

## 4. 优先级总览

```
能开工的顺序:
  M1 项目搭建、导航、注册登录  ←  需 Q6(系统版本) + ZhuCeViewModel.kt
  M2 设备 ID / 参数 / 本地存储  ←  需 MacIdBox + DeviceParamsBook + DBHelper
  M3 BLE + AES 解密            ←  需 MqtttService.kt + Q4(是否还用 BLE)
  M4 MQTT + SSL                ←  需 MqtttService.kt + .pem
  M5~M8 卡片墙/守护/报警/报告   ←  需 ui/* + report/ + Q7/Q8/Q9
```

**当前阻塞点**:`MqtttService.kt` 是 M3/M4 的唯一前置文件,优先级最高。
