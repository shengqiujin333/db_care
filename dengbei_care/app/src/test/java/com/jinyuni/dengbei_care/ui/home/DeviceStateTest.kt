package com.jinyuni.dengbei_care.ui.home

import com.jinyuni.dengbei_care.DeviceState
import com.jinyuni.dengbei_care.Severity
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * ITEM-011 实现侧自检：卡片状态语义（DeviceState）。
 *
 * 覆盖 TD-SW-002 §4.1 T-SW-L0-14：
 * - `latestTime == 0` → 未收到语义（温度/湿度由适配器显示 `--`，此处断言 `isNeverReported`）；
 * - 长时间无数据（如 40 分钟前）**不产生“离线”**，只产出相对时间标签；
 * - 不存在 `isOnline` / 30 分钟离线阈值等旧判定入口。
 */
class DeviceStateTest {

    private val now = 1_700_000_000_000L // 毫秒

    private fun state(latestTimeSeconds: Long, severity: Severity = Severity.NORMAL) = DeviceState(
        devId = "A1B2C3D4",
        name = "客厅",
        latestTemp = 25.5,
        latestHumi = 60.0,
        latestTime = latestTimeSeconds,
        alarmMessage = "",
        alarmSeverity = severity,
        sparkline = emptyList()
    )

    // ---------- 未收到 ----------

    @Test
    fun neverReported_isNeverReported_andHasNoAgeLabel() {
        val s = state(0L)
        assertTrue(s.isNeverReported())
        assertEquals("", s.lastReportAgeLabel(now))
    }

    // ---------- 相对时间标签 ----------

    @Test
    fun ageLabel_buckets() {
        fun label(secondsAgo: Long) = state(now / 1000 - secondsAgo).lastReportAgeLabel(now)

        assertEquals("刚刚", label(0))
        assertEquals("刚刚", label(59))
        assertEquals("1 分钟前", label(60))
        assertEquals("40 分钟前", label(40 * 60))
        assertEquals("59 分钟前", label(59 * 60 + 59))
        assertEquals("1 小时前", label(60 * 60))
        assertEquals("23 小时前", label(23 * 60 * 60 + 59 * 60))
        assertEquals("1 天前", label(24 * 60 * 60))
        assertEquals("3 天前", label(3 * 24 * 60 * 60))
    }

    @Test
    fun staleDevice_getsRelativeLabel_notOffline() {
        // 40 分钟无数据：旧实现会判“离线”，新实现只给相对时间
        val s = state(now / 1000 - 40 * 60)
        assertEquals("40 分钟前", s.lastReportAgeLabel(now))
        assertFalse(s.isNeverReported())
        // 相对时间标签不得包含“离线”字样
        assertFalse(s.lastReportAgeLabel(now).contains("离线"))
    }

    // ---------- 旧判定入口已移除 ----------

    @Test
    fun noOnlineOrOfflineThresholdEntryPoints() {
        val methods = DeviceState::class.java.declaredMethods.map { it.name }
        assertFalse("isOnline must be gone", methods.any { it == "isOnline" })

        val companionFields = DeviceState.Companion::class.java.declaredFields.map { it.name } +
            DeviceState::class.java.declaredFields.map { it.name }
        assertFalse(
            "30-minute offline threshold must be gone",
            companionFields.any { it.contains("OFFLINE") }
        )
    }

    // ---------- 其余语义不变 ----------

    @Test
    fun displayName_prefersAlias_thenDevId() {
        assertEquals("客厅", state(now / 1000).displayName())
        assertEquals("A1B2C3D4", state(now / 1000).copy(name = "").displayName())
        assertEquals("A1B2C3D4", DeviceState.empty("A1B2C3D4").displayName())
    }

    @Test
    fun severity_isCarriedThroughUnchanged() {
        assertEquals(Severity.ALARM, state(now / 1000, Severity.ALARM).alarmSeverity)
        assertEquals(Severity.WARNING, state(now / 1000, Severity.WARNING).alarmSeverity)
        assertEquals(Severity.NORMAL, state(now / 1000).alarmSeverity)
        assertNull(null)
    }

    @Test
    fun emptyPlaceholder_matchesNeverReportedSemantics() {
        val empty = DeviceState.empty("0C118224")
        assertTrue(empty.isNeverReported())
        assertEquals(0.0, empty.latestTemp, 0.0)
        assertEquals(0.0, empty.latestHumi, 0.0)
        assertTrue(empty.sparkline.isEmpty())
    }
}
