package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.report.ReportRegenerateFeedback
import com.jinyuni.dengbei_care.report.ReportRegenerateFeedback.Message
import java.io.File
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * AA-006 N-3 独立验证（软件测试，TD-SW-005 §4/§5.3）。
 *
 * 使用独立期望表验证“重新生成”反馈判定：无数据不算失败但必须提示，
 * 失败有独立提示，任何终态都恢复按钮。页面进度/禁用/连续点击与真机提示
 * 由真机用例覆盖，本文件不替代。
 */
class ReportRegenerateFeedbackItem003VerificationTest {

    private fun decision(
        succeeded: Boolean,
        hasContent: Boolean,
        deviceCount: Int,
        sampleCount: Int = -1
    ) = ReportRegenerateFeedback.afterRun(succeeded, hasContent, deviceCount, sampleCount)

    private fun sourceCode(): String {
        val candidates = listOf(
            File("src/main/java/com/jinyuni/dengbei_care/report/ReportRegenerateFeedback.kt"),
            File("app/src/main/java/com/jinyuni/dengbei_care/report/ReportRegenerateFeedback.kt")
        )
        val file = candidates.firstOrNull { it.isFile }
            ?: error("ReportRegenerateFeedback.kt not found (cwd=${File(".").absolutePath})")
        return file.readText()
    }

    // N3-H-01：零设备 → NO_DATA 且恢复按钮
    @Test
    fun zeroDevices_isNoData_andRestoresButton() {
        val d = decision(succeeded = true, hasContent = false, deviceCount = 0, sampleCount = 0)
        assertEquals(Message.NO_DATA, d.message)
        assertTrue(d.restoreButton)
        assertEquals(
            "暂无可生成的数据（尚无设备读数）",
            ReportRegenerateFeedback.textFor(d.message)
        )
    }

    // N3-H-02：有设备但昨日无样本 → NO_DATA
    @Test
    fun noYesterdaySamples_isNoData() {
        val d = decision(succeeded = true, hasContent = false, deviceCount = 2, sampleCount = 0)
        assertEquals(Message.NO_DATA, d.message)
        assertTrue(d.restoreButton)
    }

    // N3-H-03：成功但内容为空/与上次相同 → NO_DATA，不记失败
    @Test
    fun emptyContent_isNoData_notFailure() {
        val d = decision(succeeded = true, hasContent = false, deviceCount = 2, sampleCount = 5)
        assertEquals(Message.NO_DATA, d.message)
        val text = ReportRegenerateFeedback.textFor(d.message)
        assertTrue(text != null && !text.contains("失败"))
    }

    // N3-H-04：成功且有内容 → 无提示，恢复按钮
    @Test
    fun successWithContent_hasNoMessage_andRestoresButton() {
        val d = decision(succeeded = true, hasContent = true, deviceCount = 2, sampleCount = 5)
        assertEquals(Message.NONE, d.message)
        assertTrue(d.restoreButton)
        assertNull(ReportRegenerateFeedback.textFor(d.message))
    }

    // N3-H-05：失败 → FAILED 独立提示，恢复按钮
    @Test
    fun failure_hasFailureMessage_andRestoresButton() {
        val d = decision(succeeded = false, hasContent = false, deviceCount = 2, sampleCount = 5)
        assertEquals(Message.FAILED, d.message)
        assertTrue(d.restoreButton)
        assertEquals("报告生成失败，请稍后重试", ReportRegenerateFeedback.textFor(d.message))
    }

    // N3-H-06/H-07：所有终态都恢复按钮；文案不随设备数变成“失败”
    @Test
    fun allTerminalStates_restoreButton_andMessagesAreStable() {
        val states = listOf(
            decision(true, false, 0, 0),
            decision(true, false, 1, 0),
            decision(true, false, 1, 3),
            decision(true, true, 1, 3),
            decision(false, true, 1, 3)
        )
        assertTrue("任何终态都必须恢复按钮", states.all { it.restoreButton })
        states.forEach { d ->
            val text = ReportRegenerateFeedback.textFor(d.message)
            if (d.message == Message.NO_DATA) {
                assertTrue(text != null && text.contains("暂无"))
            }
            if (d.message == Message.FAILED) {
                assertTrue(text != null && text.contains("失败"))
            }
        }
    }

    // N3-H-08：未知样本数（-1）按设备与内容判定
    @Test
    fun unknownSampleCount_usesDeviceAndContentOnly() {
        assertEquals(
            Message.NO_DATA,
            decision(true, false, 0, -1).message
        )
        assertEquals(
            Message.NO_DATA,
            decision(true, false, 3, -1).message
        )
        assertEquals(
            Message.NONE,
            decision(true, true, 3, -1).message
        )
    }

    // 纯逻辑，无 Android 依赖（N3 附加断言）
    @Test
    fun feedback_hasNoAndroidImports() {
        val code = sourceCode()
        assertFalse("不得 import android.*", code.contains("import android."))
        assertFalse("不得 import androidx.*", code.contains("import androidx."))
    }

    // 文案常量为固定类别，供界面与测试共享
    @Test
    fun messageConstants_areStable() {
        assertEquals(
            "暂无可生成的数据（尚无设备读数）",
            ReportRegenerateFeedback.TEXT_NO_DATA
        )
        assertEquals("报告生成失败，请稍后重试", ReportRegenerateFeedback.TEXT_FAILED)
    }
}
