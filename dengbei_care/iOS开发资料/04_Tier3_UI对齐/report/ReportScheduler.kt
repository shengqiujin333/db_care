package com.jinyuni.dengbei_care.report

import android.content.Context
import android.util.Log
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.ExistingWorkPolicy
import androidx.work.OneTimeWorkRequestBuilder
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import java.util.Calendar
import java.util.concurrent.TimeUnit

object ReportScheduler {

    private const val WORK_NAME_PERIODIC = "daily_report_periodic"
    private const val WORK_NAME_ONESHOT = "daily_report_oneshot"
    private const val DEFAULT_REPORT_TIME = "07:00"
    private const val TAG = "ReportScheduler"

    /**
     * 排定每天定时生成报告的任务。
     * @param hhmm "HH:mm" 格式(24 小时制),如 "07:00"
     */
    fun scheduleDaily(context: Context, hhmm: String? = null) {
        val time = hhmm?.takeIf { it.matches(Regex("^\\d{1,2}:\\d{2}$")) } ?: DEFAULT_REPORT_TIME
        val initialDelaySec = calcInitialDelaySeconds(time)

        val request = PeriodicWorkRequestBuilder<DailyReportWorker>(24, TimeUnit.HOURS)
            .setInitialDelay(initialDelaySec, TimeUnit.SECONDS)
            .build()

        WorkManager.getInstance(context.applicationContext).enqueueUniquePeriodicWork(
            WORK_NAME_PERIODIC,
            ExistingPeriodicWorkPolicy.UPDATE,
            request
        )
        Log.i(TAG, "Scheduled daily report at $time (initial delay ${initialDelaySec}s)")
    }

    /**
     * 立即跑一次(给"查看今日报告"按钮用)。
     */
    fun runNow(context: Context) {
        val request = OneTimeWorkRequestBuilder<DailyReportWorker>().build()
        WorkManager.getInstance(context.applicationContext).enqueueUniqueWork(
            WORK_NAME_ONESHOT,
            ExistingWorkPolicy.REPLACE,
            request
        )
        Log.i(TAG, "Enqueued one-shot daily report")
    }

    fun cancel(context: Context) {
        WorkManager.getInstance(context.applicationContext).also {
            it.cancelUniqueWork(WORK_NAME_PERIODIC)
            it.cancelUniqueWork(WORK_NAME_ONESHOT)
        }
        Log.i(TAG, "Cancelled daily report work")
    }

    private fun calcInitialDelaySeconds(hhmm: String, now: Long = System.currentTimeMillis()): Long {
        val parts = hhmm.split(":")
        if (parts.size != 2) return 0L
        val hour = parts[0].toIntOrNull() ?: return 0L
        val minute = parts[1].toIntOrNull() ?: 0
        val cal = Calendar.getInstance().apply {
            timeInMillis = now
            set(Calendar.HOUR_OF_DAY, hour)
            set(Calendar.MINUTE, minute)
            set(Calendar.SECOND, 0)
            set(Calendar.MILLISECOND, 0)
        }
        if (cal.timeInMillis <= now) {
            cal.add(Calendar.DAY_OF_YEAR, 1)
        }
        return (cal.timeInMillis - now) / 1000
    }
}
