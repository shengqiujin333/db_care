package com.jinyuni.dengbei_care
import android.content.Context
import android.media.MediaPlayer
import android.media.RingtoneManager
import android.util.Log

object RingtonePlayer {
    private const val TAG = "RingtonePlayer"
    private var mediaPlayer: MediaPlayer? = null

    /**
     * 开始播放报警声。
     *
     * 本方法**不向外抛异常**：铃声 URI 可能为空、`MediaPlayer.create` 在资源不可用时返回 null
     * （平台类型）、`start()` 可能抛 `IllegalStateException`；任何失败都只记日志并保持静默降级，
     * 以免影响调用方（读数入库/上传/后台循环）的后续步骤。
     *
     * @param context 上下文
     * @param loop 是否循环播放
     */
    fun startAlarm(context: Context, loop: Boolean = true) {
        try {
            if (mediaPlayer?.isPlaying == true) return  // 如果已经在播放，避免重复启动

            // 获取默认铃声的URI（可能为 null）
            val alarmUri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_ALARM)
                ?: RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION)
                ?: run {
                    Log.w(TAG, "no default alarm/notification uri available")
                    return
                }

            // 使用 MediaPlayer 播放铃声；create 失败返回 null，必须显式判空
            val player = MediaPlayer.create(context, alarmUri) ?: run {
                Log.w(TAG, "MediaPlayer.create returned null for $alarmUri")
                return
            }
            player.isLooping = loop
            player.start()
            mediaPlayer = player
        } catch (e: Exception) {
            Log.w(TAG, "startAlarm failed: ${e.message}")
            releaseQuietly()
        }
    }

    /**
     * 停止播放报警声。同样不向外抛异常。
     */
    fun stopAlarm() {
        try {
            mediaPlayer?.let {
                if (it.isPlaying) {
                    it.stop()
                }
                it.release()
            }
        } catch (e: Exception) {
            Log.w(TAG, "stopAlarm failed: ${e.message}")
        } finally {
            mediaPlayer = null
        }
    }

    private fun releaseQuietly() {
        try {
            mediaPlayer?.release()
        } catch (e: Exception) {
            Log.w(TAG, "release failed: ${e.message}")
        } finally {
            mediaPlayer = null
        }
    }
}
