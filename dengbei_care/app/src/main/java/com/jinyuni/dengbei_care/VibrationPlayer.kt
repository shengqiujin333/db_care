package com.jinyuni.dengbei_care

import android.content.Context
import android.os.Build
import android.os.VibrationEffect
import android.os.Vibrator
import android.os.VibratorManager
import android.util.Log

object VibrationPlayer {
    private const val TAG = "VibrationPlayer"
    private var vibrator: Vibrator? = null

    /**
     * 震动手机。
     *
     * 本方法**不向外抛异常**：系统服务可能不可用（返回 null）、服务类型转换可能失败、
     * `vibrate()` 可能抛异常；任何失败都只记日志并静默降级，以免影响调用方后续步骤。
     *
     * @param context 上下文
     * @param duration 震动持续时间（毫秒）
     */
    @Suppress("DEPRECATION")
    fun vibratePhone(context: Context, duration: Long, repeat: Boolean = false) {
        try {
            val v = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                // Android 12 (API 31) 及以上版本
                (context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE) as? VibratorManager)?.defaultVibrator
            } else {
                // Android 12 以下版本
                context.getSystemService(Context.VIBRATOR_SERVICE) as? Vibrator
            } ?: run {
                Log.w(TAG, "vibrator service unavailable")
                return
            }
            vibrator = v

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                // API 26 及以上，使用 VibrationEffect.createWaveform 实现循环或自定义震动模式
                val vibrationPattern = longArrayOf(0, 500, 1000) // 震动500ms，停1秒
                val repeatIndex = if (repeat) 0 else -1 // 0 表示从头开始循环，-1 表示不循环

                val vibrationEffect = VibrationEffect.createWaveform(vibrationPattern, repeatIndex)
                v.vibrate(vibrationEffect)
            } else {
                // API 26 以下，使用老的 vibrate 方法模拟震动模式
                if (repeat) {
                    val vibrationPattern = longArrayOf(0, 500, 1000) // 震动500ms，停1秒
                    v.vibrate(vibrationPattern, 0) // 0 表示从头开始循环
                } else {
                    v.vibrate(duration)
                }
            }
        } catch (e: Exception) {
            Log.w(TAG, "vibratePhone failed: ${e.message}")
        }
    }

    /**
     * 取消震动。同样不向外抛异常。
     */
    @Suppress("DEPRECATION")
    fun stopVibration(context: Context) {
        try {
            vibrator?.cancel()  // 停止震动
        } catch (e: Exception) {
            Log.w(TAG, "stopVibration failed: ${e.message}")
        } finally {
            vibrator = null
        }
    }
}
