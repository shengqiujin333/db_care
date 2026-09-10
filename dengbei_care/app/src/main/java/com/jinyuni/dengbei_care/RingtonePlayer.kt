package com.jinyuni.dengbei_care
import android.content.Context
import android.media.MediaPlayer
import android.media.RingtoneManager

object RingtonePlayer {
    private var mediaPlayer: MediaPlayer? = null

    /**
     * 开始播放报警声
     * @param context 上下文
     * @param loop 是否循环播放
     */
    fun startAlarm(context: Context, loop: Boolean = true) {
        if (mediaPlayer?.isPlaying == true) return  // 如果已经在播放，避免重复启动

        // 获取默认铃声的URI
        val alarmUri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_ALARM)
            ?: RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION)

        // 使用 MediaPlayer 播放铃声
        mediaPlayer = MediaPlayer.create(context, alarmUri).apply {
            isLooping = loop  // 让 MediaPlayer 循环播放
            start()
        }
    }

    /**
     * 停止播放报警声
     */
    fun stopAlarm() {
        mediaPlayer?.let {
            if (it.isPlaying) {
                it.stop()
                it.release()
                mediaPlayer = null
            }
        }
    }
}
