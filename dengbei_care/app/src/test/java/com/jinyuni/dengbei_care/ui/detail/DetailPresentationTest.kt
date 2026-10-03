package com.jinyuni.dengbei_care.ui.detail

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * ITEM-012 实现侧自检：设备详情页的只读规则呈现与高温强调（DetailPresentation）。
 *
 * 覆盖 TD-SW-002 §4.5：
 * - T-SW-L3-04：规则文案与 readme 一致（采样周期 3 分钟 + 下降超过 0.9℃且无光 / 超过 35.0℃），
 *   且不显示环境光状态或照度信息、不给 App 推断的上报原因；
 * - T-SW-L3-05：图表范围上限 24 小时（不再是 50 分钟）；
 * - T-SW-L3-06：仅 `> 35.0℃` 强调，`= 35.0℃` 不强调。
 */
class DetailPresentationTest {

    // ---------- 规则文案 ----------

    @Test
    fun ruleText_matchesReadme() {
        val text = DetailPresentation.REPORT_RULE_TEXT
        assertTrue("must state the 3-minute sampling period", text.contains("3 分钟"))
        assertTrue("must state the 0.9℃ drop condition", text.contains("0.9"))
        assertTrue("must state the dark condition (readme wording)", text.contains("无光"))
        assertTrue("must state the >35.0℃ condition", text.contains("35.0"))
        assertTrue("must state that reports are conditional", text.contains("才上报"))
    }

    @Test
    fun ruleText_containsNoAmbientLightOrIlluminanceInfo() {
        val text = DetailPresentation.REPORT_RULE_TEXT
        assertFalse(text.contains("光照"))
        assertFalse(text.lowercase().contains("lux"))
        assertFalse(text.contains("有光"))
        assertFalse(text.contains("照度"))
    }

    @Test
    fun ruleText_givesNoAppInferredReportingReason() {
        val text = DetailPresentation.REPORT_RULE_TEXT
        // 不得给出“因下降 x℃ 上报”“本次因…上报”之类的推断结论
        assertFalse(text.contains("因下降"))
        assertFalse(text.contains("本次上报原因"))
        assertFalse(text.contains("上报原因"))
    }

    // ---------- 图表范围 ----------

    @Test
    fun chartRangeLimit_isTwentyFourHours() {
        assertEquals(24, DetailPresentation.MAX_CHART_RANGE_HOURS)
        assertEquals(24L * 60L * 60L * 1000L, DetailPresentation.MAX_CHART_RANGE_MS)
        assertEquals(86_400_000L, DetailPresentation.MAX_CHART_RANGE_MS)
    }

    @Test
    fun rangeLimitMessage_matchesLimit_andHasNoFiftyMinuteWording() {
        val msg = DetailPresentation.rangeLimitMessage()
        assertTrue(msg.contains("24 小时"))
        assertFalse(msg.contains("50"))
        assertFalse(msg.contains("分钟以内"))
    }

    // ---------- 高温强调边界 ----------

    @Test
    fun highTemp_isStrictlyAbove35() {
        assertTrue(DetailPresentation.isHighTemperature(35.1))
        assertTrue(DetailPresentation.isHighTemperature(36.0))
        assertTrue(DetailPresentation.isHighTemperature(100.0))
        assertFalse("exactly 35.0 must not be emphasized", DetailPresentation.isHighTemperature(35.0))
        assertFalse(DetailPresentation.isHighTemperature(34.9))
        assertFalse(DetailPresentation.isHighTemperature(0.0))
        assertFalse(DetailPresentation.isHighTemperature(-10.0))
    }

    @Test
    fun highTempSuffix_onlyForHighReadings() {
        assertEquals(" · 高温", DetailPresentation.highTempSuffix(true))
        assertEquals("", DetailPresentation.highTempSuffix(false))
    }

    @Test
    fun highTempThreshold_equalsReadmeValue() {
        assertEquals(35.0, DetailPresentation.HIGH_TEMP_C, 0.0)
    }
}
