package com.jinyuni.dengbei_care.telemetry

/**
 * 报警判定纯逻辑（AA-002 §7 / D-04 / D-05）。
 *
 * 与改动前的关键差异：下降报警原先比较内存里的「最近第 1/2/3 个样本」
 * （`DeviceHistory.data5MinAgo/data10MinAgo/data15MinAgo`），在“3 分钟采样 + 条件上报”
 * 之后样本间隔不再固定，按序号比较已无时间含义。本模块改为**基于时间戳的时间窗**：
 * 对 5/10/15 分钟三个窗口，各取「不晚于 `now - 窗口`、且不早于 `now - 窗口 - 回溯容差`」的
 * **最新**样本作为基准；基准缺失（无历史/样本过旧/样本都太新）时该窗口不成立。
 *
 * 保持现状的部分（T-SW-L0-08/L0-09/L0-10）：
 * - 阈值超限：`temp > tempMax || temp < tempMin || humi > humiMax || humi < humiMin`（严格比较，
 *   恰好等于阈值不触发）；
 * - 下降：`基准温度 - 当前温度 > tempDrop` **且** `基准湿度 - 当前湿度 > humiDrop`（严格比较、AND）；
 * - 开关语义：两个开关都关 → `未设置报警`；只开一个时只判该项；两个都开时**先下降、后超限**，
 *   两者同时成立时超限文案覆盖下降文案（与改动前一致）；
 * - 紧急：`温度 > 65.0℃` 覆盖前述结果，文案 `紧急报警，温度超过65度，谨防火灾`、级别 ALARM、
 *   不受 900 s 去抖限制（去抖在调用方，不在本层）。
 *
 * 本模块为纯函数：不读 SharedPreferences、不写库、不修改入参，输出中**不携带任何温度/湿度数值**
 * （不产生伪造样本）。
 */
object AlarmEvaluator {

    /** 紧急报警阈值（℃）：严格大于才触发，65.0 不触发 */
    const val EMERGENCY_TEMP_C = 65.0

    /** 下降报警的时间窗（分钟） */
    val DROP_WINDOW_MINUTES: List<Int> = listOf(5, 10, 15)

    /** 基准样本的回溯容差（秒）：基准须落在 `[now-窗口-slack, now-窗口]` 内 */
    const val DEFAULT_LOOKBACK_SLACK_SECONDS: Long = 30L * 60L

    /** 文案字面值（兼容面，不得改） */
    const val MSG_NOT_CONFIGURED = "未设置报警"
    const val MSG_NONE = "无报警"
    const val MSG_DROP = "温度湿度下降报警"
    const val MSG_THRESHOLD = "温湿度超限报警"
    const val MSG_EMERGENCY = "紧急报警，温度超过65度，谨防火灾"

    /** 报警严重级别（与 App 的 `com.jinyuni.dengbei_care.Severity` 同名，由调用方映射） */
    enum class Severity { NORMAL, WARNING, ALARM }

    /** 判定结论 */
    enum class Outcome {
        /** 下降与超限报警都未启用 */
        NOT_CONFIGURED,

        /** 已启用但本次不满足任何报警条件 */
        NONE,

        /** 温度湿度下降报警 */
        DROP,

        /** 温湿度超限报警 */
        THRESHOLD,

        /** 紧急报警（温度 > 65.0℃） */
        EMERGENCY
    }

    /** 判定输入参数（单位：℃ / %RH；开关用 Boolean） */
    data class Parameters(
        val tempDrop: Double,
        val humiDrop: Double,
        val tempMax: Double,
        val tempMin: Double,
        val humiMax: Double,
        val humiMin: Double,
        val dropEnabled: Boolean,
        val thresholdEnabled: Boolean
    )

    /** 候选历史样本（Unix 秒 + 读数） */
    data class Sample(
        val timeSeconds: Long,
        val temperature: Double,
        val humidity: Double
    )

    /** 判定结果（不含任何温度/湿度数值） */
    data class Result(
        val outcome: Outcome,
        val message: String,
        val severity: Severity,
        /** 命中的下降窗口（分钟）；未命中下降时为 null */
        val matchedWindowMinutes: Int? = null
    ) {
        /** 紧急报警不受去抖限制，每次都通知 */
        val notifiesImmediately: Boolean get() = outcome == Outcome.EMERGENCY
    }

    /**
     * 评估一次读数。
     *
     * @param now 本次读数时间（Unix 秒）
     * @param temperature 本次温度 ℃
     * @param humidity 本次湿度 %RH
     * @param parameters 该设备的报警参数
     * @param history 候选历史样本（可含任意时间点；本函数自行筛选基准，不修改入参）
     * @param lookbackSlackSeconds 基准回溯容差（秒），默认 [DEFAULT_LOOKBACK_SLACK_SECONDS]
     */
    fun evaluate(
        now: Long,
        temperature: Double,
        humidity: Double,
        parameters: Parameters,
        history: List<Sample>,
        lookbackSlackSeconds: Long = DEFAULT_LOOKBACK_SLACK_SECONDS
    ): Result {
        // 紧急报警（> 65.0℃）优先级最高，且与开关无关
        if (temperature > EMERGENCY_TEMP_C) {
            return Result(Outcome.EMERGENCY, MSG_EMERGENCY, Severity.ALARM)
        }

        if (!parameters.dropEnabled && !parameters.thresholdEnabled) {
            return Result(Outcome.NOT_CONFIGURED, MSG_NOT_CONFIGURED, Severity.NORMAL)
        }

        val dropHit = if (parameters.dropEnabled) {
            matchDropWindow(now, temperature, humidity, parameters, history, lookbackSlackSeconds)
        } else {
            null
        }

        val thresholdHit = parameters.thresholdEnabled && isThresholdExceeded(temperature, humidity, parameters)

        // 与改动前一致：先下降、后超限；两者同时成立时超限文案覆盖下降文案
        return when {
            thresholdHit -> Result(Outcome.THRESHOLD, MSG_THRESHOLD, Severity.WARNING)
            dropHit != null -> Result(Outcome.DROP, MSG_DROP, Severity.WARNING, dropHit)
            else -> Result(Outcome.NONE, MSG_NONE, Severity.NORMAL)
        }
    }

    /**
     * 时间窗下降判定：返回命中的窗口（分钟），未命中返回 null。
     * 对每个窗口取 `[now-W-slack, now-W]` 内的最新样本为基准；基准缺失该窗口不成立。
     */
    private fun matchDropWindow(
        now: Long,
        temperature: Double,
        humidity: Double,
        parameters: Parameters,
        history: List<Sample>,
        lookbackSlackSeconds: Long
    ): Int? {
        DROP_WINDOW_MINUTES.forEach { windowMinutes ->
            val windowSeconds = windowMinutes * 60L
            val upperBound = now - windowSeconds            // 不晚于 now - W
            val lowerBound = upperBound - lookbackSlackSeconds // 不早于 now - W - slack

            val baseline = history
                .asSequence()
                .filter { it.temperature.isFinite() && it.humidity.isFinite() }
                .filter { it.timeSeconds in lowerBound..upperBound }
                .maxByOrNull { it.timeSeconds }

            if (baseline != null) {
                val tempDropHit = baseline.temperature - temperature > parameters.tempDrop
                val humiDropHit = baseline.humidity - humidity > parameters.humiDrop
                if (tempDropHit && humiDropHit) return windowMinutes
            }
        }
        return null
    }

    /** 阈值超限（严格比较；恰好等于阈值不触发） */
    private fun isThresholdExceeded(
        temperature: Double,
        humidity: Double,
        parameters: Parameters
    ): Boolean =
        temperature > parameters.tempMax ||
            temperature < parameters.tempMin ||
            humidity > parameters.humiMax ||
            humidity < parameters.humiMin
}
