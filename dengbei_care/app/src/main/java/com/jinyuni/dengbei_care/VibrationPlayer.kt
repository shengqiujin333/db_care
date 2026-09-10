package com.jinyuni.dengbei_care

import android.content.Context
import android.os.Build
import android.os.VibrationEffect
import android.os.Vibrator
import android.os.VibratorManager

object VibrationPlayer {
    private var vibrator: Vibrator? = null
    /**
     * 震动手机
     * @param context 上下文
     * @param duration 震动持续时间（毫秒）
     */
    @Suppress("DEPRECATION")
    fun vibratePhone(context: Context, duration: Long, repeat: Boolean = false) {
        vibrator = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            // Android 12 (API 31) 及以上版本
            val vibratorManager = context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE) as VibratorManager
            vibratorManager.defaultVibrator
        } else {
            // Android 12 以下版本
            context.getSystemService(Context.VIBRATOR_SERVICE) as Vibrator
        }

        vibrator?.let {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                // API 26 及以上，使用 VibrationEffect.createWaveform 实现循环或自定义震动模式
                val vibrationPattern = longArrayOf(0, 500, 1000) // 震动500ms，停1秒
                val repeatIndex = if (repeat) 0 else -1 // 0 表示从头开始循环，-1 表示不循环

                val vibrationEffect = VibrationEffect.createWaveform(vibrationPattern, repeatIndex)
                it.vibrate(vibrationEffect)
            } else {
                // API 26 以下，使用老的 vibrate 方法模拟震动模式
                if (repeat) {
                    val vibrationPattern = longArrayOf(0, 500, 1000) // 震动500ms，停1秒
                    it.vibrate(vibrationPattern, 0) // 0 表示从头开始循环
                } else {
                    it.vibrate(duration)
                }
            }
        }
    }


    /**
     * 取消震动
     */
    @Suppress("DEPRECATION")
    fun stopVibration(context: Context) {
        vibrator?.cancel()  // 停止震动
    }
}
