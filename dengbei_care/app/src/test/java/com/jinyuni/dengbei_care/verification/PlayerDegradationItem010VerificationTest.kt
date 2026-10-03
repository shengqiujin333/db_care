package com.jinyuni.dengbei_care.verification

import android.content.ContextWrapper
import com.jinyuni.dengbei_care.RingtonePlayer
import com.jinyuni.dengbei_care.VibrationPlayer
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-010 修复项 F2）——宿主机可执行部分。
 *
 * 验证两个告警副作用辅助类**不向外抛异常**：铃声/震动依赖的系统服务不可用时（此处用 `MockContext`
 * 返回 null 服务、`Build.VERSION.SDK_INT == 0` 走老分支）必须静默降级；`stopAlarm/stopVibration`
 * 可重复调用且不抛。
 */
class PlayerDegradationItem010VerificationTest {

    @Test
    fun stopAlarm_isRepeatableAndDoesNotThrow() {
        RingtonePlayer.stopAlarm()
        RingtonePlayer.stopAlarm()
        assertTrue(true)
    }

    @Test
    fun vibratePhone_withUnavailableSystemService_degradesSilently() {
        // 系统服务在 mockable android.jar 下返回默认值 null；ContextWrapper(null) 仅用于承载调用
        val ctx = ContextWrapper(null)
        // 系统服务不可用（返回 null）+ API<26 分支 → 必须静默返回，不抛出
        VibrationPlayer.vibratePhone(ctx, 300_000L, true)
        VibrationPlayer.vibratePhone(ctx, 300_000L, false)
        VibrationPlayer.stopVibration(ctx)
        assertTrue(true)
    }
}
