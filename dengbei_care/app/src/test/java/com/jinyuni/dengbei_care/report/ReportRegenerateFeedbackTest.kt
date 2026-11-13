package com.jinyuni.dengbei_care.report

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/** AA-006 N-3：报告再生成反馈判定矩阵（宿主） */
class ReportRegenerateFeedbackTest {

    @Test
    fun failure_showsFailedAndRestoresButton() {
        val decision = ReportRegenerateFeedback.afterRun(
            succeeded = false, hasContent = false, deviceCount = 0
        )
        assertEquals(ReportRegenerateFeedback.Message.FAILED, decision.message)
        assertTrue(decision.restoreButton)
        assertEquals(ReportRegenerateFeedback.TEXT_FAILED, ReportRegenerateFeedback.textFor(decision.message))
    }

    @Test
    fun zeroDevice_isNoData_withExplicitMessage() {
        val decision = ReportRegenerateFeedback.afterRun(
            succeeded = true, hasContent = false, deviceCount = 0
        )
        assertEquals(ReportRegenerateFeedback.Message.NO_DATA, decision.message)
        assertTrue(decision.restoreButton)
        assertEquals(ReportRegenerateFeedback.TEXT_NO_DATA, ReportRegenerateFeedback.textFor(decision.message))
    }

    @Test
    fun emptyContent_isNoData_evenWithDevices() {
        val decision = ReportRegenerateFeedback.afterRun(
            succeeded = true, hasContent = false, deviceCount = 2, sampleCount = 5
        )
        assertEquals(ReportRegenerateFeedback.Message.NO_DATA, decision.message)
    }

    @Test
    fun zeroYesterdaySamples_isNoData() {
        val decision = ReportRegenerateFeedback.afterRun(
            succeeded = true, hasContent = true, deviceCount = 2, sampleCount = 0
        )
        assertEquals(ReportRegenerateFeedback.Message.NO_DATA, decision.message)
    }

    @Test
    fun unknownSampleCount_withContentAndDevices_isSilentSuccess() {
        val decision = ReportRegenerateFeedback.afterRun(
            succeeded = true, hasContent = true, deviceCount = 1, sampleCount = -1
        )
        assertEquals(ReportRegenerateFeedback.Message.NONE, decision.message)
        assertTrue(decision.restoreButton)
        assertNull(ReportRegenerateFeedback.textFor(decision.message))
    }

    @Test
    fun samplesPresent_isSilentSuccess() {
        val decision = ReportRegenerateFeedback.afterRun(
            succeeded = true, hasContent = true, deviceCount = 1, sampleCount = 1
        )
        assertEquals(ReportRegenerateFeedback.Message.NONE, decision.message)
    }

    @Test
    fun negativeDeviceCount_isNoData() {
        val decision = ReportRegenerateFeedback.afterRun(
            succeeded = true, hasContent = true, deviceCount = -1
        )
        assertEquals(ReportRegenerateFeedback.Message.NO_DATA, decision.message)
    }
}
