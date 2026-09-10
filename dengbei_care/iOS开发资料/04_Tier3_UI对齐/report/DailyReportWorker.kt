package com.jinyuni.dengbei_care.report

import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.work.CoroutineWorker
import androidx.work.WorkerParameters
import com.jinyuni.dengbei_care.DatabaseHelperInstance
import com.jinyuni.dengbei_care.MacIdBook
import com.jinyuni.dengbei_care.R
import java.util.Calendar

class DailyReportWorker(
    appContext: Context,
    params: WorkerParameters
) : CoroutineWorker(appContext, params) {

    companion object {
        const val KEY_REPORT_TEXT = "last_report_text"
        const val KEY_REPORT_TIME = "last_report_time"
        const val CHANNEL_ID = "daily_report_channel"
        const val NOTIFICATION_ID = 2001
        private const val TAG = "DailyReportWorker"
    }

    override suspend fun doWork(): Result {
        return try {
            val ctx = applicationContext
            val now = System.currentTimeMillis()
            val cal = Calendar.getInstance().apply {
                timeInMillis = now
                add(Calendar.DAY_OF_YEAR, -1)
                set(Calendar.HOUR_OF_DAY, 0)
                set(Calendar.MINUTE, 0)
                set(Calendar.SECOND, 0)
                set(Calendar.MILLISECOND, 0)
            }
            val dayStart = cal.timeInMillis / 1000
            val dayEnd = dayStart + 24 * 60 * 60

            val dbHelper = DatabaseHelperInstance.getDatabaseHelper(ctx)
            val savedIds = MacIdBook.all(ctx)
            if (savedIds.isEmpty()) {
                Log.w(TAG, "No saved devices, skipping report")
                return Result.success()
            }

            val statsMap = mutableMapOf<String, com.jinyuni.dengbei_care.DailyStats>()
            val alarmsMap = mutableMapOf<String, List<com.jinyuni.dengbei_care.AlarmEvent>>()
            val names = mutableMapOf<String, String>()
            savedIds.forEach { (id, name) ->
                names[id] = name
                dbHelper.getDailyStats(id, dayStart, dayEnd)?.let { statsMap[id] = it }
                alarmsMap[id] = dbHelper.getAlarmEvents(id, dayStart, dayEnd)
            }

            val report = ReportGenerator.build(statsMap, alarmsMap, names)
            val text = report.toText()

            // 缓存到 SharedPreferences 供 ReportActivity 读
            ctx.getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
                .putString(KEY_REPORT_TEXT, text)
                .putLong(KEY_REPORT_TIME, now)
                .apply()

            // 发通知(打开 ReportActivity 的 PendingIntent)
            sendNotification(ctx, text)

            Log.i(TAG, "Daily report generated for ${savedIds.size} devices")
            Result.success()
        } catch (e: Exception) {
            Log.e(TAG, "Failed to generate daily report", e)
            Result.failure()
        }
    }

    private fun sendNotification(ctx: Context, reportText: String) {
        val intent = Intent(ctx, ReportActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TOP
        }
        val pendingIntent = PendingIntent.getActivity(
            ctx, 0, intent,
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        val preview = reportText.lineSequence().firstOrNull() ?: "早起报告"

        val builder = NotificationCompat.Builder(ctx, CHANNEL_ID)
            .setSmallIcon(R.mipmap.ic_launcher)
            .setContentTitle("爱守护 - 早起报告")
            .setContentText(preview)
            .setStyle(NotificationCompat.BigTextStyle().bigText(reportText))
            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
            .setAutoCancel(true)
            .setContentIntent(pendingIntent)

        try {
            NotificationManagerCompat.from(ctx).notify(NOTIFICATION_ID, builder.build())
        } catch (e: SecurityException) {
            Log.w(TAG, "Notification permission denied: ${e.message}")
        }
    }
}
