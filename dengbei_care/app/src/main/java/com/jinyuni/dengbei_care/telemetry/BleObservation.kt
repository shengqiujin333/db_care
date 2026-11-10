package com.jinyuni.dengbei_care.telemetry

import com.jinyuni.dengbei_care.protocol.GatewayFrameCodec

/**
 * BLE 观测有效性（AA-005 G-5 / D-21 / D-25）。
 *
 * 纯逻辑：
 * - 网关“无数据占位”指解密后的设备块 8 B 全零：ID=00000000 **且** temperature_x10=0 **且** humidity_x10=0；
 *   占位不产生读数、不入库、不入待发箱、不生成设备卡片、不触发报警；
 * - 带有效绑定 ID 的 `0.0℃/0.0%RH` 是合法读数，首次即处理；
 * - 判定使用原始 x10 整数，不用浮点比较。
 */
object BleObservation {

    data class Reading(
        val devId: String,
        val tX10: Int,
        val hX10: Int
    ) {
        val temperatureC: Double get() = tX10 / 10.0
        val humidityPct: Double get() = hX10 / 10.0
    }

    sealed interface Observation {
        data object Placeholder : Observation
        data class Valid(val reading: Reading) : Observation
    }

    /** 占位块：8 B 全零（ID 00000000 + 两个 x10 值均为 0） */
    fun isPlaceholder(devId: String, tX10: Int, hX10: Int): Boolean =
        tX10 == 0 && hX10 == 0 && normalizeId(devId) == "00000000"

    fun classify(devId: String, tX10: Int, hX10: Int): Observation =
        if (isPlaceholder(devId, tX10, hX10)) {
            Observation.Placeholder
        } else {
            Observation.Valid(Reading(normalizeId(devId), tX10, hX10))
        }

    /** 从协议解码结果分类（SensorSample 已带 x10 整数字段） */
    fun classify(sample: GatewayFrameCodec.SensorSample): Observation =
        classify(sample.devIdCompact, sample.temperatureX10, sample.humidityX10)

    private fun normalizeId(devId: String): String =
        devId.replace(":", "").replace("-", "").uppercase()
}
