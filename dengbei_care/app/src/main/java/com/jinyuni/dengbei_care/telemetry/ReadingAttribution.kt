package com.jinyuni.dengbei_care.telemetry

/**
 * MQTT 批量载荷归因（IC-002 §5 / AA-002 §5.2 / D-08）。
 *
 * 网关遥测载荷格式（既有兼容格式，不改）：
 * ```text
 * <epoch_seconds>:<temperature_1,temperature_2,...>:<humidity_1,humidity_2,...>
 * ```
 * 该载荷**不含 device ID**，只能按 App 侧保存的设备顺序（`MacIdBook.all()`，即写入网关
 * `0xA1` 配置包的同一顺序）把每个值归因到具体传感器。
 *
 * 本模块为纯逻辑（不依赖 Android）。与改动前的关键差异：改动前用 `minOf(...)` **静默截断**
 * 后按下标处理，数目不一致时会把 A 的读数记到 B 上；本模块改为**严格映射**——数目不一致、
 * 序列为空、数值非法或时间非法时**拒绝整批**，调用方据此不落库、不上传、不报警。
 *
 * 归因结果中的 `devId` 随读数一起向下游传递，下游不得再依赖列表下标。
 */
object ReadingAttribution {

    /** 归因后的单条读数（单位与线上一致：℃，%RH） */
    data class Reading(
        val devId: String,        // 8 位 HEX 大写无分隔（MacIdBook 键）
        val temperature: Double,  // ℃
        val humidity: Double      // %RH
    )

    /** 一批已归因的读数（时间取自载荷首段） */
    data class Batch(
        val timeSeconds: Long,
        val readings: List<Reading>
    )

    /** 拒绝原因 */
    enum class RejectionReason {
        /** 载荷不足三段（`time:temps:humis`），或首段时间缺失/非法/非正 */
        INVALID_TIME,

        /** 温度或湿度序列为空（空白段） */
        EMPTY_SERIES,

        /** 序列中存在非数值、空白项或非有限值（NaN/Infinity） */
        INVALID_NUMBER,

        /** 设备数、温度个数、湿度个数三者不一致 */
        COUNT_MISMATCH,

        /** 本地没有已绑定的设备 ID，无法归因 */
        NO_BOUND_DEVICE
    }

    /** 归因结果：明确区分"整批成功"与"整批拒绝"，拒绝时不携带任何读数 */
    sealed class Result {
        data class Success(val batch: Batch) : Result()
        data class Rejected(val reason: RejectionReason, val detail: String) : Result()
    }

    /** 载荷分段数（time / temps / humis） */
    private const val REQUIRED_PARTS = 3

    /**
     * 把 MQTT 批量载荷严格归因到 [savedIds]（顺序即网关回传顺序，来自 `MacIdBook.all()`）。
     *
     * 成功条件：三段齐备、时间为正的长整数、温度与湿度序列长度均等于 [savedIds] 且每一项都是
     * 有限数值。任何不满足条件的情形返回 [Result.Rejected]，不抛异常、不产生读数。
     * 数值范围（物理量程）校验不属本模块职责。
     */
    fun attribute(payload: String, savedIds: List<String>): Result {
        val parts = payload.split(":")
        if (parts.size < REQUIRED_PARTS) {
            return Result.Rejected(
                RejectionReason.INVALID_TIME,
                "parts=${parts.size} < $REQUIRED_PARTS"
            )
        }

        val timeSeconds = parts[0].trim().toLongOrNull()
        if (timeSeconds == null || timeSeconds <= 0L) {
            return Result.Rejected(
                RejectionReason.INVALID_TIME,
                "time='${parts[0]}'"
            )
        }

        val temperatures = parseSeries(parts[1])
            ?: return Result.Rejected(
                RejectionReason.INVALID_NUMBER,
                "temperature series='${parts[1]}'"
            )
        val humidities = parseSeries(parts[2])
            ?: return Result.Rejected(
                RejectionReason.INVALID_NUMBER,
                "humidity series='${parts[2]}'"
            )

        if (temperatures.isEmpty() || humidities.isEmpty()) {
            return Result.Rejected(
                RejectionReason.EMPTY_SERIES,
                "temps=${temperatures.size}, humis=${humidities.size}"
            )
        }

        if (savedIds.isEmpty()) {
            return Result.Rejected(
                RejectionReason.NO_BOUND_DEVICE,
                "savedIds=0"
            )
        }

        if (savedIds.size != temperatures.size || temperatures.size != humidities.size) {
            return Result.Rejected(
                RejectionReason.COUNT_MISMATCH,
                "saved=${savedIds.size}, temps=${temperatures.size}, humis=${humidities.size}"
            )
        }

        val readings = ArrayList<Reading>(savedIds.size)
        for (i in savedIds.indices) {
            readings += Reading(
                devId = savedIds[i],
                temperature = temperatures[i],
                humidity = humidities[i]
            )
        }
        return Result.Success(Batch(timeSeconds, readings))
    }

    /**
     * 解析逗号分隔的数值序列。
     * 空段返回空列表；任一项为空白、非数值或非有限值（NaN/±Infinity）返回 null（整批拒绝）。
     * 允许逗号后带空格的写法（App 自身发布 BLE 读数时用 `", "` 连接）。
     */
    private fun parseSeries(series: String): List<Double>? {
        if (series.isBlank()) return emptyList()
        val out = ArrayList<Double>()
        series.split(",").forEach { token ->
            val v = token.trim().toDoubleOrNull()
            if (v == null || !v.isFinite()) return null
            out += v
        }
        return out
    }
}
