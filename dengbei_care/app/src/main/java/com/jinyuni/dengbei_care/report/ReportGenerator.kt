package com.jinyuni.dengbei_care.report

import com.jinyuni.dengbei_care.AlarmEvent
import com.jinyuni.dengbei_care.DailyStats
import com.jinyuni.dengbei_care.MacIdBook
import com.jinyuni.dengbei_care.TemperatureDatabaseHelper

data class DeviceReportSection(
    val devId: String,
    val name: String,
    val stats: DailyStats?,
    val alarms: List<AlarmEvent>
)

data class DailyReport(
    val title: String,
    val summary: String,
    val sections: List<DeviceReportSection>,
    val suggestions: List<String>
) {
    fun toText(): String {
        val sb = StringBuilder()
        sb.appendLine(title)
        sb.appendLine()
        sb.appendLine(summary)
        sb.appendLine()
        if (sections.isEmpty()) {
            sb.appendLine("暂无设备数据。")
            return sb.toString()
        }
        sections.forEach { section ->
            sb.appendLine("【${section.name.ifBlank { section.devId }}】")
            val s = section.stats
            if (s == null) {
                // 按条件上报：未满足上报条件时当日没有任何记录，不等于“数据缺失”
                sb.appendLine("  - 昨日无上报")
            } else {
                sb.appendLine("  - 采样 ${s.sampleCount} 条")
                sb.appendLine("  - 温度:最低 ${fmt(s.tempMin)}°C,最高 ${fmt(s.tempMax)}°C,平均 ${fmt(s.tempAvg)}°C")
                sb.appendLine("  - 湿度:最低 ${fmt(s.humiMin)}%,最高 ${fmt(s.humiMax)}%,平均 ${fmt(s.humiAvg)}%")
            }
            if (section.alarms.isNotEmpty()) {
                sb.appendLine("  - 报警 ${section.alarms.size} 次")
            }
            sb.appendLine()
        }
        if (suggestions.isNotEmpty()) {
            sb.appendLine("建议:")
            suggestions.forEach { sb.appendLine("  - $it") }
        }
        return sb.toString()
    }

    private fun fmt(v: Double): String = "%.1f".format(v)
}

object ReportGenerator {

    /**
     * 给定昨日所有设备的统计数据和报警事件,生成一份报告。
     */
    fun build(
        stats: Map<String, DailyStats>,
        alarms: Map<String, List<AlarmEvent>>,
        deviceNames: Map<String, String>
    ): DailyReport {
        val totalSamples = stats.values.sumOf { it.sampleCount }
        val totalAlarms = alarms.values.sumOf { it.size }

        val dateStr = java.text.SimpleDateFormat("yyyy-MM-dd", java.util.Locale.getDefault())
            .format(java.util.Date(System.currentTimeMillis() - 24 * 60 * 60 * 1000))

        // 按条件上报（3 分钟采样 ≠ 每 3 分钟一条）：条数非等间隔，不得暗示固定节拍
        val summary = "昨日(${dateStr})共上报 $totalSamples 条数据（按条件上报，条数非等间隔），触发 $totalAlarms 次报警。"

        val sections = deviceNames.keys.sorted().map { devId ->
            DeviceReportSection(
                devId = devId,
                name = deviceNames[devId] ?: devId,
                stats = stats[devId],
                alarms = alarms[devId] ?: emptyList()
            )
        }

        val suggestions = mutableListOf<String>()
        stats.forEach { (devId, s) ->
            val name = deviceNames[devId] ?: devId
            // 紧急报警(温度 > 65) - 火灾预警
            val emergencyCount = alarms[devId]?.count {
                it.type == TemperatureDatabaseHelper.ALARM_TYPE_EMERGENCY
            } ?: 0
            if (emergencyCount > 0) {
                suggestions.add("[$name] 检测到 $emergencyCount 次紧急报警(温度超 65°C),建议立即排查火源")
            }
            // 温度持续下降 - 检查通风
            val dropCount = alarms[devId]?.count {
                it.type == TemperatureDatabaseHelper.ALARM_TYPE_DROP
            } ?: 0
            if (dropCount >= 3) {
                suggestions.add("[$name] 温湿度下降报警 ${dropCount} 次,建议检查卧室通风和门窗密封")
            }
            // 超限报警 - 调整阈值或环境
            val thresholdCount = alarms[devId]?.count {
                it.type == TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD
            } ?: 0
            if (thresholdCount >= 5) {
                suggestions.add("[$name] 温湿度超限报警 ${thresholdCount} 次,建议检查是否需要调整报警阈值或环境")
            }
            // 上报条数偏少：按条件上报（未满足条件不上报），少不一定异常
            if (s.sampleCount < 24) {
                suggestions.add("[$name] 昨日上报 ${s.sampleCount} 条；按条件上报且非等间隔，条数偏少不一定异常")
            }
        }

        return DailyReport(
            title = "爱守护 - 早起报告",
            summary = summary,
            sections = sections,
            suggestions = suggestions
        )
    }
}
