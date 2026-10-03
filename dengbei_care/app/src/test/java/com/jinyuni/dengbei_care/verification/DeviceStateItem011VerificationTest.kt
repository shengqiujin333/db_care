package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.DeviceState
import com.jinyuni.dengbei_care.Severity
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-011）——宿主机逻辑层。
 *
 * 期望值由本文件按 AA-002 §7 / D-10 与任务描述独立写出：
 * 不再有“30 分钟无数据 ⇒ 离线”的判定入口；未收到仍为未收到；相对时间标签分桶精确；
 * 报警级别语义原样传递。
 */
class DeviceStateItem011VerificationTest {

    private val base = 1_700_000_000_000L // 毫秒

    private fun state(latestTimeSec: Long, severity: Severity = Severity.NORMAL, name: String = "") =
        DeviceState.empty("A1B2C3D4", name).copy(latestTime = latestTimeSec, alarmSeverity = severity)

    @Test
    fun neverReported_semanticsAndEmptyLabel() {
        assertTrue(DeviceState.empty("A1B2C3D4").isNeverReported())
        assertFalse(state(base / 1000).isNeverReported())
        assertEquals("从未上报不得给出相对时间标签", "", DeviceState.empty("A1B2C3D4").lastReportAgeLabel(base))
        assertTrue(DeviceState.empty("A1B2C3D4").lastReportAgeLabel(base).isEmpty())
    }

    @Test
    fun ageLabel_bucketBoundariesAreExact() {
        val cases = mapOf(
            0L to "刚刚",
            59L to "刚刚",
            60L to "1 分钟前",          // 恰好 60 s 必须跳出“刚刚”
            61L to "1 分钟前",
            59L * 60 to "59 分钟前",
            60L * 60 to "1 小时前",     // 恰好 60 min 必须跳出“分钟”
            23L * 60 * 60 + 59 * 60 to "23 小时前",
            24L * 60 * 60 to "1 天前",  // 恰好 24 h 必须跳出“小时”
            3L * 24 * 60 * 60 to "3 天前"
        )
        for ((ageSec, expected) in cases) {
            val s = state(base / 1000 - ageSec)
            val label = s.lastReportAgeLabel(base)
            assertEquals("age=$ageSec", expected, label)
            assertFalse("age=$ageSec label=$label", label.contains("离线"))
            assertFalse("age=$ageSec label=$label", label.contains("在线"))
        }
    }

    @Test
    fun staleDevice_getsRelativeLabel_notOffline() {
        val s = state(base / 1000 - 40 * 60)          // 40 分钟无数据
        val label = s.lastReportAgeLabel(base)
        assertEquals("40 分钟前", label)
        assertFalse(label.contains("离线"))
        // 更长（例如 3 天）仍是相对时间，而非离线
        assertEquals("3 天前", state(base / 1000 - 3 * 24 * 60 * 60).lastReportAgeLabel(base))
    }

    @Test
    fun noOnlineOfflineEntryPoints_remain() {
        val cls = DeviceState::class.java
        assertTrue("isOnline 必须已删除", cls.declaredMethods.none { it.name == "isOnline" })
        val fields = cls.declaredFields.map { it.name } + cls.fields.map { it.name }
        assertTrue("OFFLINE_THRESHOLD_MS 必须已删除", fields.none { it.contains("OFFLINE") })
        // 兼容面：其余字段仍在（其他消费者不受影响）
        assertEquals(
            listOf("devId", "name", "latestTemp", "latestHumi", "latestTime", "alarmMessage", "alarmSeverity", "sparkline"),
            cls.declaredFields.map { it.name }.filter { it != "Companion" }
        )
    }

    @Test
    fun severityAndDisplayName_areCarriedThrough() {
        assertEquals(Severity.NORMAL, DeviceState.empty("A1B2C3D4").alarmSeverity)
        assertEquals(Severity.ALARM, state(base / 1000, Severity.ALARM).alarmSeverity)
        assertEquals(Severity.WARNING, state(base / 1000, Severity.WARNING).alarmSeverity)
        assertEquals("别名优先", "床垫A", state(base / 1000, name = "床垫A").displayName())
        assertEquals("无别名用 devId", "A1B2C3D4", state(base / 1000).displayName())
        // 未收到 + 紧急（异常组合）仍以“未收到”优先决定数值展示，且级别不被改写
        val never = DeviceState.empty("A1B2C3D4").copy(alarmSeverity = Severity.ALARM)
        assertTrue(never.isNeverReported())
        assertEquals(Severity.ALARM, never.alarmSeverity)
    }
}
