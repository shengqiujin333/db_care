package com.jinyuni.dengbei_care.ui.detail

/**
 * 设备详情页的只读呈现常量与判定（纯逻辑，无 Android 依赖，可在宿主机断言）。
 *
 * 边界（AA-002 D-03/D-14、IC-002 §2/§5）：
 * - 只**呈现** readme 给定的采样周期与条件上报规则；App **不重算**传感器门控、不推断上报原因；
 * - 环境光状态不上空口、不上 BLE，App 无法得知；因此除规则原文里的“无光”条件外，
 *   界面不显示任何环境光状态或照度数值，也不显示“因下降 x℃ 上报”之类结论；
 * - 高温强调只按读数数值做样式（`> 35.0℃` 强调，`= 35.0℃` 不强调），不构成门控逻辑。
 */
object DetailPresentation {

    /** 高温强调阈值（℃）：严格大于才强调（readme 修改点 4 的“超过 35 度”） */
    const val HIGH_TEMP_C = 35.0

    /** 图表可查询时间范围上限（小时）：稀疏样本下需要更长窗口（原上限过短，常查不到数据） */
    const val MAX_CHART_RANGE_HOURS = 24

    /** 图表可查询时间范围上限（毫秒）；毫秒常量由小时常量派生，避免散落的秒/分换算字面量 */
    const val HOUR_MS = 3_600_000L
    const val MAX_CHART_RANGE_MS = MAX_CHART_RANGE_HOURS * HOUR_MS

    /**
     * 只读规则说明（与 readme 修改点 3/4 一致）：
     * 采样周期 3 分钟；仅当温度较上次下降超过 0.9℃且无光，或温度超过 35.0℃ 时才上报。
     */
    val REPORT_RULE_TEXT: String =
        "采样周期 3 分钟。仅当温度较上次下降超过 0.9℃且无光，或温度超过 35.0℃ 时才上报；" +
                "未满足条件时不会上报，因此可能长时间没有新数据。"

    /** 高温强调判定：`> 35.0℃` 才为真 */
    fun isHighTemperature(temperatureC: Double): Boolean = temperatureC > HIGH_TEMP_C

    /** 图表范围超限提示（与上限常量同源） */
    fun rangeLimitMessage(): String = "时间范围必须在 $MAX_CHART_RANGE_HOURS 小时以内"

    /** 高温强调后缀（用于文本强调，与颜色强调并行） */
    fun highTempSuffix(isHigh: Boolean): String = if (isHigh) " · 高温" else ""
}
