package com.jinyuni.dengbei_care

import android.os.Bundle
import android.view.MotionEvent
import androidx.appcompat.app.AppCompatActivity
import com.jinyuni.dengbei_care.ui.home.HomeViewModel

open class BaseActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // 其他初始化代码
    }

    override fun onTouchEvent(event: MotionEvent?): Boolean {
        if (event?.action == MotionEvent.ACTION_DOWN) {
            RingtonePlayer.stopAlarm()
            VibrationPlayer.stopVibration(this)
            val homeViewModel = HomeViewModel.getInstance(application)
            // 触屏清掉所有设备的报警状态
            homeViewModel.clearAllDeviceAlarms()
        }
        return super.onTouchEvent(event)
    }
}
