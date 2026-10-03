package com.jinyuni.dengbei_care.report

import com.jinyuni.dengbei_care.AlarmEvent
import com.jinyuni.dengbei_care.DailyStats
import com.jinyuni.dengbei_care.TemperatureDatabaseHelper
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * ITEM-013 实现侧自检：早起报告文案与零样本表述（ReportGenerator）。
 *
 * 覆盖 TD-SW-002 §4.5 T-SW-L3-07：
 * - 无上报设备显示“昨日无上报”（不是笼统“无数据”）；
 * - 文案说明按条件上报 / 条数非等间隔，且不暗示固定节拍（无“每 3 分钟”“288 条”之类）；
 * - 统计字段与报警次数结构不变（同一批数据的数值与逐行格式与改动前一致）。
 */
class ReportGeneratorTest {

    private fun stats(
        devId: String = "A1B2C3D4",
        count: Int = 10,
        tMin: Double = 20.0,
        tMax: Double = 25.0,
        tAvg: Double = 22.5,
        hMin: Double = 40.0,
        hMax: Double = 60.0,
        hAvg: Double = 50.0
    ) = DailyStats(devId, count, tMin, tMax, tAvg, hMin, hMax, hAvg)

    private fun alarm(devId: String, type: String, time: Long = 1_700_000_000L) =
        AlarmEvent(1L, time, devId, type, "msg")

    // ---------- 零样本：昨日无上报 ----------

    @Test
    fun deviceWithoutReports_showsNoReportYesterday_notNoData() {
        val report = ReportGenerator.build(
            stats = mapOf("A1B2C3D4" to stats()),
            alarms = emptyMap(),
            deviceNames = mapOf("A1B2C3D4" to "客厅", "0C118224" to "卧室")
        )
        val text = report.toText()

        assertTrue(text.contains("【卧室】"))
        assertTrue("zero-sample device must read 昨日无上报", text.contains("  - 昨日无上报"))
        assertFalse("generic 无数据 wording must be gone", text.contains("无数据"))
    }

    // ---------- 摘要：按条件上报 / 非等间隔 ----------

    @Test
    fun summary_statesConditionalReportingAndNonEqualIntervals() {
        val report = ReportGenerator.build(
            stats = mapOf("A1B2C3D4" to stats(count = 7)),
            alarms = mapOf("A1B2C3D4" to listOf(alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_DROP))),
            deviceNames = mapOf("A1B2C3D4" to "客厅")
        )
        val text = report.toText()

        assertTrue(text.contains("按条件上报"))
        assertTrue(text.contains("非等间隔"))
        assertTrue(text.contains("共上报 7 条数据"))
        assertTrue(text.contains("触发 1 次报警"))
        // 不暗示固定节拍
        assertFalse(text.contains("每 3 分钟"))
        assertFalse(text.contains("每3分钟"))
        assertFalse(text.contains("288"))
        assertFalse(text.contains("共采集"))
    }

    // ---------- 统计结构与数值稳定 ----------

    @Test
    fun statsLines_andAlarmCountStructure_unchanged() {
        val report = ReportGenerator.build(
            stats = mapOf("A1B2C3D4" to stats(count = 10)),
            alarms = mapOf(
                "A1B2C3D4" to listOf(
                    alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_DROP),
                    alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD)
                )
            ),
            deviceNames = mapOf("A1B2C3D4" to "客厅")
        )
        val text = report.toText()

        assertTrue(text.contains("【客厅】"))
        assertTrue(text.contains("  - 采样 10 条"))
        assertTrue(text.contains("  - 温度:最低 20.0°C,最高 25.0°C,平均 22.5°C"))
        assertTrue(text.contains("  - 湿度:最低 40.0%,最高 60.0%,平均 50.0%"))
        assertTrue(text.contains("  - 报警 2 次"))
        // 结构顺序（在设备段内）：采样 → 温度 → 湿度 → 报警
        val sectionStart = text.indexOf("【客厅】")
        val idx = listOf(
            "  - 采样 10 条",
            "  - 温度:",
            "  - 湿度:",
            "  - 报警 2 次"
        ).map { text.indexOf(it, sectionStart) }
        assertTrue(idx.all { it >= 0 })
        assertEquals(idx.sorted(), idx)
    }

    @Test
    fun sameInput_producesIdenticalStatistics() {
        val statsMap = mapOf("A1B2C3D4" to stats(count = 3, tMin = -5.0, tMax = 36.1, tAvg = 12.34))
        val alarmsMap = mapOf("A1B2C3D4" to listOf(alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_EMERGENCY)))
        val names = mapOf("A1B2C3D4" to "客厅")

        val first = ReportGenerator.build(statsMap, alarmsMap, names).toText()
        val second = ReportGenerator.build(statsMap, alarmsMap, names).toText()

        assertEquals(first, second)
        // 统计数值逐字保留（含负温与一位小数格式）
        assertTrue(first.contains("  - 温度:最低 -5.0°C,最高 36.1°C,平均 12.3°C"))
    }

    // ---------- 低条数建议：不再推断离线 / 不再引用固定节拍 ----------

    @Test
    fun lowSampleCountSuggestion_hasNoCadenceOrOfflineInference() {
        val report = ReportGenerator.build(
            stats = mapOf("A1B2C3D4" to stats(count = 5)),
            alarms = emptyMap(),
            deviceNames = mapOf("A1B2C3D4" to "客厅")
        )
        val text = report.toText()

        assertTrue(text.contains("建议:"))
        assertTrue(text.contains("昨日上报 5 条"))
        assertTrue(text.contains("按条件上报"))
        assertFalse("must not infer offline from data age/count", text.contains("离线"))
        assertFalse("must not imply a fixed cadence expectation", text.contains("288"))
        assertFalse(text.contains("预期"))
    }

    @Test
    fun enoughSamples_producesNoLowCountSuggestion() {
        val report = ReportGenerator.build(
            stats = mapOf("A1B2C3D4" to stats(count = 24)),
            alarms = emptyMap(),
            deviceNames = mapOf("A1B2C3D4" to "客厅")
        )
        val text = report.toText()

        assertFalse(text.contains("昨日上报 24 条；"))
        assertFalse(text.contains("建议:"))
    }

    // ---------- 报警建议结构不变 ----------

    @Test
    fun alarmBasedSuggestions_stillEmitted() {
        val report = ReportGenerator.build(
            stats = mapOf("A1B2C3D4" to stats(count = 30)),
            alarms = mapOf(
                "A1B2C3D4" to listOf(
                    alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_EMERGENCY),
                    alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_DROP),
                    alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_DROP),
                    alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_DROP),
                    alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD),
                    alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD),
                    alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD),
                    alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD),
                    alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD)
                )
            ),
            deviceNames = mapOf("A1B2C3D4" to "客厅")
        )
        val text = report.toText()

        assertTrue(text.contains("紧急报警(温度超 65°C)"))
        assertTrue(text.contains("温湿度下降报警 3 次"))
        assertTrue(text.contains("温湿度超限报警 5 次"))
    }

    @Test
    fun noDevices_showsPlaceholder() {
        val report = ReportGenerator.build(emptyMap(), emptyMap(), emptyMap())
        assertEquals(true, report.toText().contains("暂无设备数据。"))
    }
}
