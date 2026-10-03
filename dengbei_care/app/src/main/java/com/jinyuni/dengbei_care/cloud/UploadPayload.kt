package com.jinyuni.dengbei_care.cloud

import com.jinyuni.dengbei_care.telemetry.ReadingAttribution
import com.jinyuni.dengbei_care.ui.zhuce.SensorReading
import com.jinyuni.dengbei_care.ui.zhuce.SensorUploadData

/**
 * `/upload_data` 请求体构造（AA-002 §5.5）。
 *
 * 契约（`服务器迁移记录.md` §八）：
 * ```json
 * {"phone":"13...","mac":"网关MAC",
 *  "readings":[{"devId":"A1B2C3D4","time":1699999999,"temperature":25.5,"humidity":60.0}]}
 * ```
 * 单位：`temperature` = 摄氏度、`humidity` = %RH、`time` = Unix 秒。
 * 线上原始值是 ×10 整数（0.1℃ / 0.1 %RH），**除以 10 的换算在解码/归因阶段完成**
 * （`protocol/GatewayFrameCodec`、`telemetry/ReadingAttribution` 输出的已是 ℃/%RH）；
 * 本模块只做字段与结构组装，不再做第二次换算，避免双重缩放。
 *
 * 本模块为纯逻辑：不读 SharedPreferences、不访问网络、不修改入参；空读数集不产生请求体。
 */
object UploadPayload {

    /**
     * 由一批**已归因**读数构造请求体（生产路径）。
     *
     * @param timeSeconds 该批读数的时间（MQTT = payload 首段网关时间，BLE = 接收时刻）
     * @param readings 归因结果，`devId` 来自记录自身（不依赖列表下标）
     * @return 请求体；`readings` 为空时返回 null（不产生请求）
     */
    fun fromAttributed(
        phone: String,
        mac: String,
        timeSeconds: Long,
        readings: List<ReadingAttribution.Reading>
    ): SensorUploadData? {
        if (readings.isEmpty()) return null
        return SensorUploadData(
            phone = phone,
            mac = mac,
            readings = readings.map { r ->
                SensorReading(
                    devId = r.devId,
                    time = timeSeconds,
                    temperature = r.temperature,
                    humidity = r.humidity
                )
            }
        )
    }

    /**
     * 由已构造好的 [SensorReading] 列表构造请求体（待发箱补传路径：各行可携带不同时间）。
     *
     * @return 请求体；`readings` 为空时返回 null（不产生请求）
     */
    fun build(phone: String, mac: String, readings: List<SensorReading>): SensorUploadData? {
        if (readings.isEmpty()) return null
        return SensorUploadData(phone = phone, mac = mac, readings = readings)
    }
}
