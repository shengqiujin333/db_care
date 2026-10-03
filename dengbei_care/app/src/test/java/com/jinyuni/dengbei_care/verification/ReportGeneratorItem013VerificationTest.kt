package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.AlarmEvent
import com.jinyuni.dengbei_care.DailyStats
import com.jinyuni.dengbei_care.TemperatureDatabaseHelper
import com.jinyuni.dengbei_care.report.DailyReport
import com.jinyuni.dengbei_care.report.DeviceReportSection
import com.jinyuni.dengbei_care.report.ReportGenerator
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-013）——宿主机逻辑层。
 *
 * 关键点：用**从 Git 历史逐字重建的改动前报告生成器**作为参照，对同一批数据分别生成报告，
 * 证明“统计数值与行结构不变、只有措辞变化”，并覆盖零样本表述与禁止位（固定节拍/离线推断）。
 */
class ReportGeneratorItem013VerificationTest {

    // ================= 改动前（基线）参照实现：取自 7e168bd 的 ReportGenerator.kt（仅 3 处文案不同） =================

    private data class LegacyReport(val title: String, val summary: String, val sections: List<Section>, val suggestions: List<String>) {
        data class Section(val name: String, val devId: String, val stats: DailyStats?, val alarmCount: Int)

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
                    sb.appendLine("  - 无数据")
                } else {
                    sb.appendLine("  - 采样 ${s.sampleCount} 条")
                    sb.appendLine("  - 温度:最低 ${fmt(s.tempMin)}°C,最高 ${fmt(s.tempMax)}°C,平均 ${fmt(s.tempAvg)}°C")
                    sb.appendLine("  - 湿度:最低 ${fmt(s.humiMin)}%,最高 ${fmt(s.humiMax)}%,平均 ${fmt(s.humiAvg)}%")
                }
                if (section.alarmCount > 0) {
                    sb.appendLine("  - 报警 ${section.alarmCount} 次")
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

    private fun legacyBuild(
        stats: Map<String, DailyStats>,
        alarms: Map<String, List<AlarmEvent>>,
        deviceNames: Map<String, String>
    ): LegacyReport {
        val totalSamples = stats.values.sumOf { it.sampleCount }
        val totalAlarms = alarms.values.sumOf { it.size }
        val summary = "昨日(DATE)共采集 $totalSamples 条数据,触发 $totalAlarms 次报警。"
        val sections = deviceNames.keys.sorted().map { devId ->
            LegacyReport.Section(
                name = deviceNames[devId] ?: devId,
                devId = devId,
                stats = stats[devId],
                alarmCount = alarms[devId]?.size ?: 0
            )
        }
        val suggestions = mutableListOf<String>()
        stats.forEach { (devId, s) ->
            val name = deviceNames[devId] ?: devId
            val emergencyCount = alarms[devId]?.count { it.type == TemperatureDatabaseHelper.ALARM_TYPE_EMERGENCY } ?: 0
            if (emergencyCount > 0) suggestions.add("[$name] 检测到 $emergencyCount 次紧急报警(温度超 65°C),建议立即排查火源")
            val dropCount = alarms[devId]?.count { it.type == TemperatureDatabaseHelper.ALARM_TYPE_DROP } ?: 0
            if (dropCount >= 3) suggestions.add("[$name] 温湿度下降报警 ${dropCount} 次,建议检查卧室通风和门窗密封")
            val thresholdCount = alarms[devId]?.count { it.type == TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD } ?: 0
            if (thresholdCount >= 5) suggestions.add("[$name] 温湿度超限报警 ${thresholdCount} 次,建议检查是否需要调整报警阈值或环境")
            if (s.sampleCount < 24) suggestions.add("[$name] 昨日采样 ${s.sampleCount} 条(预期约 288 条),设备可能离线过")
        }
        return LegacyReport("爱守护 - 早起报告", summary, sections, suggestions)
    }

    // ================= 测试夹具 =================

    private fun stats(count: Int, tMin: Double, tMax: Double, tAvg: Double, hMin: Double, hMax: Double, hAvg: Double) =
        DailyStats("dev", count, tMin, tMax, tAvg, hMin, hMax, hAvg)

    private fun alarm(devId: String, type: String, time: Long) = AlarmEvent(1L, time, devId, type, "m")

    private val fixtureStats = mapOf(
        "A1B2C3D4" to stats(30, -3.2, 36.4, 22.05, 40.0, 88.0, 61.23),
        "0C118224" to stats(7, 18.0, 19.9, 18.95, 50.0, 60.0, 55.05),
        // "33333333" 当日零上报（stats 中缺失）
    )

    private val fixtureAlarms = mapOf(
        "A1B2C3D4" to listOf(
            alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_EMERGENCY, 1000),
            alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_DROP, 2000),
            alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_DROP, 3000),
            alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_DROP, 4000),
            alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD, 5000),
            alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD, 6000),
            alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD, 7000),
            alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD, 8000),
            alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD, 9000),
        ),
        "0C118224" to emptyList()
    )

    private val fixtureNames = mapOf("A1B2C3D4" to "床垫A", "0C118224" to "床垫B", "33333333" to "床垫C")

    private fun numbers(text: String) = Regex("-?\\d+(?:\\.\\d+)?").findAll(text).map { it.value }.toList()

    private fun textLines(text: String) = text.split("\n").filter { it.isNotBlank() }

    // ================= 1. 统计数值与结构不变（与基线逐项比对） =================

    @Test
    fun statisticsAndStructure_areIdenticalToPreChange() {
        // 日期不属统计值：比较前统一归一化 `昨日(<date>)`
        fun normalize(t: String) = t.replace(Regex("昨日\\([^)]*\\)"), "昨日(DATE)")
        val now = normalize(ReportGenerator.build(fixtureStats, fixtureAlarms, fixtureNames).toText())
        val legacy = normalize(legacyBuild(fixtureStats, fixtureAlarms, fixtureNames).toText())

        // 1) 数值集合：除“预期约 288 条”这一被有意删除的固定节拍数字外，统计数值必须完全一致
        val legacyNums = numbers(legacy)
        val nowNums = numbers(now)
        val removedNums = legacyNums.toMutableList().apply { nowNums.forEach { remove(it) } }
        val addedNums = nowNums.toMutableList().apply { legacyNums.forEach { remove(it) } }
        assertEquals("数值差异仅允许移除固定的 288 条预期", listOf("288"), removedNums)
        assertTrue("不得新增任何数值", addedNums.isEmpty())

        // 3) 行级差异仅限已知的 3 处措辞
        val legacyLines = textLines(legacy)
        val nowLines = textLines(now)
        val removed = legacyLines.toMutableList().apply { nowLines.forEach { remove(it) } }
        val added = nowLines.toMutableList().apply { legacyLines.forEach { remove(it) } }
        assertEquals("仅允许 3 行被改写", 3, removed.size)
        assertEquals(3, added.size)
        assertTrue(removed.any { it.contains("无数据") })
        assertTrue(removed.any { it.contains("共采集") })
        assertTrue(removed.any { it.contains("288") })
        assertTrue(added.any { it.contains("昨日无上报") })
        assertTrue(added.any { it.contains("共上报") && it.contains("条数非等间隔") })
        assertTrue(added.any { it.contains("不一定异常") })

        // 4) 数值行（采样/温度/湿度/报警）逐字未变
        val statLinesLegacy = legacyLines.filter { it.startsWith("  - 采样") || it.startsWith("  - 温度") || it.startsWith("  - 湿度") || it.startsWith("  - 报警") }
        val statLinesNow = nowLines.filter { it.startsWith("  - 采样") || it.startsWith("  - 温度") || it.startsWith("  - 湿度") || it.startsWith("  - 报警") }
        assertEquals(statLinesLegacy, statLinesNow)
    }

    @Test
    fun sameInputIsDeterministic_andNegativeTempsFormattedConsistently() {
        val a = ReportGenerator.build(fixtureStats, fixtureAlarms, fixtureNames).toText()
        val b = ReportGenerator.build(fixtureStats, fixtureAlarms, fixtureNames).toText()
        assertEquals(a, b)
        assertTrue("负温保留符号且一位小数", a.contains("最低 -3.2°C"))
        assertTrue(a.contains("平均 61.2%"))
    }

    // ================= 2. 零样本表述与禁止位 =================

    @Test
    fun zeroSampleDevice_saysNoReportYesterday_notNoData() {
        val text = ReportGenerator.build(fixtureStats, fixtureAlarms, fixtureNames).toText()
        assertTrue(text.contains("【床垫C】"))
        assertTrue(text.contains("昨日无上报"))
        assertFalse("不得再出现“无数据”", text.contains("无数据"))
        // 零样本设备不得出现统计行
        val sectionC = text.substringAfter("【床垫C】").substringBefore("\n\n")
        assertFalse(sectionC.contains("采样"))
        assertFalse(sectionC.contains("温度:"))
    }

    @Test
    fun wording_hasNoFixedCadenceOrOfflineInference() {
        val text = ReportGenerator.build(fixtureStats, fixtureAlarms, fixtureNames).toText()
        for (forbidden in listOf("每 3 分钟", "288", "共采集", "离线", "预期")) {
            assertFalse("不得出现「$forbidden」", text.contains(forbidden))
        }
        assertTrue(text.contains("按条件上报"))
        assertTrue(text.contains("条数非等间隔"))
    }

    // ================= 3. 建议阈值与结构不变（边界） =================

    @Test
    fun suggestionThresholds_areUnchangedAtBoundaries() {
        // 下降恰好 3 次 → 触发；2 次 → 不触发
        for ((drops, expected) in listOf(3 to true, 2 to false)) {
            val alarms = mapOf("A1B2C3D4" to (1..drops).map { alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_DROP, it.toLong()) })
            val s = mapOf("A1B2C3D4" to stats(30, 1.0, 2.0, 1.5, 1.0, 2.0, 1.5))
            val text = ReportGenerator.build(s, alarms, mapOf("A1B2C3D4" to "A")).toText()
            assertEquals("drops=$drops", expected, text.contains("温湿度下降报警"))
        }
        // 超限恰好 5 次 → 触发；4 次 → 不触发
        for ((n, expected) in listOf(5 to true, 4 to false)) {
            val alarms = mapOf("A1B2C3D4" to (1..n).map { alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD, it.toLong()) })
            val s = mapOf("A1B2C3D4" to stats(30, 1.0, 2.0, 1.5, 1.0, 2.0, 1.5))
            val text = ReportGenerator.build(s, alarms, mapOf("A1B2C3D4" to "A")).toText()
            assertEquals("threshold=$n", expected, text.contains("温湿度超限报警"))
        }
        // 紧急 ≥1 → 触发
        val em = mapOf("A1B2C3D4" to listOf(alarm("A1B2C3D4", TemperatureDatabaseHelper.ALARM_TYPE_EMERGENCY, 1)))
        val emText = ReportGenerator.build(mapOf("A1B2C3D4" to stats(30, 1.0, 2.0, 1.5, 1.0, 2.0, 1.5)), em, mapOf("A1B2C3D4" to "A")).toText()
        assertTrue(emText.contains("紧急报警"))
        // 条数边界：23 → 提示；24 → 不提示
        for ((count, expected) in listOf(23 to true, 24 to false)) {
            val s = mapOf("A1B2C3D4" to stats(count, 1.0, 2.0, 1.5, 1.0, 2.0, 1.5))
            val text = ReportGenerator.build(s, emptyMap(), mapOf("A1B2C3D4" to "A")).toText()
            assertEquals("count=$count", expected, text.contains("条数偏少不一定异常"))
        }
    }

    @Test
    fun emptyDeviceList_andAlarmCountLineStructure() {
        val empty = ReportGenerator.build(emptyMap(), emptyMap(), emptyMap()).toText()
        assertTrue(empty.contains("暂无设备数据。"))
        // 无报警的设备不出现“报警 N 次”行
        val s = mapOf("A1B2C3D4" to stats(30, 1.0, 2.0, 1.5, 1.0, 2.0, 1.5))
        val text = ReportGenerator.build(s, emptyMap(), mapOf("A1B2C3D4" to "A")).toText()
        assertFalse(text.contains("报警 0 次"))
        assertFalse(text.contains("  - 报警"))
        // 有报警时行内数字 = 该设备报警条数
        val alarms = mapOf("A1B2C3D4" to listOf(alarm("A1B2C3D4", "drop", 1), alarm("A1B2C3D4", "drop", 2)))
        val t2 = ReportGenerator.build(s, alarms, mapOf("A1B2C3D4" to "A")).toText()
        assertTrue(t2.contains("  - 报警 2 次"))
        assertEquals("爱守护 - 早起报告", ReportGenerator.build(s, alarms, mapOf("A1B2C3D4" to "A")).title)
    }

    @Test
    fun reportStructureTypesAreUnchanged() {
        val r: DailyReport = ReportGenerator.build(fixtureStats, fixtureAlarms, fixtureNames)
        assertEquals(3, r.sections.size)
        r.sections.forEach { sec: DeviceReportSection ->
            assertTrue(sec.devId.isNotBlank())
            assertTrue(sec.alarms.size >= 0)
        }
        assertEquals(setOf("devId", "name", "stats", "alarms"), DeviceReportSection::class.java.declaredFields.map { it.name }.toSet())
    }
}
