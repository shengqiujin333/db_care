package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.ui.detail.DetailPresentation
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-012）——宿主机逻辑层。
 *
 * 期望值由本文件按 readme 修改点 3/4 与 AA-002 §5.3/§7（D-12/D-14/D-03）独立写出：
 * 规则文案与 readme 一致且不含推断结论；图表范围上限 24 h；高温强调严格 `> 35.0℃`。
 */
class DetailPresentationItem012VerificationTest {

    private val expectedRule =
        "采样周期 3 分钟。仅当温度较上次下降超过 0.9℃且无光，或温度超过 35.0℃ 时才上报；" +
            "未满足条件时不会上报，因此可能长时间没有新数据。"

    @Test
    fun ruleText_isExactlyTheReadmeRule() {
        assertEquals(expectedRule, DetailPresentation.REPORT_RULE_TEXT)
        val t = DetailPresentation.REPORT_RULE_TEXT
        listOf("采样周期 3 分钟", "0.9℃", "无光", "35.0℃", "才上报", "可能长时间没有新数据")
            .forEach { assertTrue("规则文案缺少「$it」", t.contains(it)) }
    }

    @Test
    fun ruleText_hasNoAmbientLightValueOrAppInferredCause() {
        val t = DetailPresentation.REPORT_RULE_TEXT
        for (forbidden in listOf("lux", "照度", "光照", "勒克斯")) {
            assertFalse("规则文案不得出现 $forbidden", t.contains(forbidden, ignoreCase = true))
        }
        for (inference in listOf("因下降", "因为温度", "由于温度", "上报原因", "触发上报的原因", "推测")) {
            assertFalse("规则文案不得给出推断结论：$inference", t.contains(inference))
        }
        // “无光”只作为 readme 给定的上报条件出现（且只出现一次）
        assertEquals(1, Regex("无光").findAll(t).count())
    }

    @Test
    fun chartRangeLimit_isTwentyFourHours_andMessageHasNoFiftyMinutes() {
        assertEquals(24, DetailPresentation.MAX_CHART_RANGE_HOURS)
        assertEquals(86_400_000L, DetailPresentation.MAX_CHART_RANGE_MS)
        assertEquals(24L * DetailPresentation.HOUR_MS, DetailPresentation.MAX_CHART_RANGE_MS)
        val msg = DetailPresentation.rangeLimitMessage()
        assertEquals("时间范围必须在 24 小时以内", msg)
        assertFalse("提示不得再出现 50 分钟", msg.contains("50"))
        assertFalse(msg.contains("分钟"))
    }

    @Test
    fun highTemperature_isStrictlyAbove35() {
        assertFalse("恰好 35.0 不强调", DetailPresentation.isHighTemperature(35.0))
        assertFalse(DetailPresentation.isHighTemperature(34.9))
        assertFalse(DetailPresentation.isHighTemperature(0.0))
        assertFalse(DetailPresentation.isHighTemperature(-10.0))
        assertTrue(DetailPresentation.isHighTemperature(35.1))
        assertTrue(DetailPresentation.isHighTemperature(36.0))
        assertTrue(DetailPresentation.isHighTemperature(100.0))
        assertTrue(DetailPresentation.isHighTemperature(35.0 + 1e-9))
        assertEquals(35.0, DetailPresentation.HIGH_TEMP_C, 0.0)
    }

    @Test
    fun highTempSuffix_onlyForHighReadings() {
        assertEquals(" · 高温", DetailPresentation.highTempSuffix(true))
        assertEquals("", DetailPresentation.highTempSuffix(false))
        // 后缀与阈值判定一致
        assertEquals("", DetailPresentation.highTempSuffix(DetailPresentation.isHighTemperature(35.0)))
        assertEquals(" · 高温", DetailPresentation.highTempSuffix(DetailPresentation.isHighTemperature(35.1)))
    }
}
