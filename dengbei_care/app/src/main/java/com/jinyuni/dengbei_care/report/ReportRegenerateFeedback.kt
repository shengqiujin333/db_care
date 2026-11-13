package com.jinyuni.dengbei_care.report

/**
 * 报告页“重新生成”的反馈判定（AA-006 N-3 / RECHECK-002 O-2）。
 *
 * 纯逻辑：无设备/无当日读数/无内容都算“没有可生成的数据”，但不是错误；
 * 失败需要区分提示；任何结束状态都必须恢复按钮，避免静默完成。
 * [sampleCount] 传入 -1 表示未知（只按设备数与内容判定）。
 */
object ReportRegenerateFeedback {

    enum class Message {
        NONE,
        NO_DATA,
        FAILED
    }

    data class Decision(val message: Message, val restoreButton: Boolean = true)

    const val TEXT_NO_DATA = "暂无可生成的数据（尚无设备读数）"
    const val TEXT_FAILED = "报告生成失败，请稍后重试"

    fun afterRun(
        succeeded: Boolean,
        hasContent: Boolean,
        deviceCount: Int,
        sampleCount: Int = -1
    ): Decision = when {
        !succeeded -> Decision(Message.FAILED)
        !hasContent -> Decision(Message.NO_DATA)
        deviceCount <= 0 -> Decision(Message.NO_DATA)
        sampleCount == 0 -> Decision(Message.NO_DATA)
        else -> Decision(Message.NONE)
    }

    fun textFor(message: Message): String? = when (message) {
        Message.NONE -> null
        Message.NO_DATA -> TEXT_NO_DATA
        Message.FAILED -> TEXT_FAILED
    }
}
