package com.jinyuni.dengbei_care.report

import android.content.Context
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.Observer
import androidx.work.WorkInfo
import androidx.work.WorkManager
import com.jinyuni.dengbei_care.databinding.ActivityReportBinding
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class ReportActivity : AppCompatActivity() {

    private lateinit var binding: ActivityReportBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityReportBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.toolbar.setNavigationOnClickListener { finish() }
        binding.regenerateButton.setOnClickListener {
            ReportScheduler.runNow(this)
        }

        observeWork()
        loadCachedReport()
    }

    private fun observeWork() {
        WorkManager.getInstance(this)
            .getWorkInfosForUniqueWorkLiveData("daily_report_oneshot")
            .observe(this, Observer { infos ->
                if (infos.isNullOrEmpty()) return@Observer
                val info = infos[0]
                when (info.state) {
                    WorkInfo.State.RUNNING -> {
                        binding.loading.visibility = View.VISIBLE
                    }
                    WorkInfo.State.SUCCEEDED, WorkInfo.State.FAILED -> {
                        binding.loading.visibility = View.GONE
                        loadCachedReport()
                    }
                    else -> {
                        // ENQUEUED / CANCELLED / BLOCKED - 不动
                    }
                }
            })
    }

    private fun loadCachedReport() {
        val prefs = getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
        val text = prefs.getString(DailyReportWorker.KEY_REPORT_TEXT, null)
        val time = prefs.getLong(DailyReportWorker.KEY_REPORT_TIME, 0L)

        if (text != null) {
            binding.reportContent.text = text
        } else {
            binding.reportContent.text = "暂无报告。点击下方「重新生成」可立即生成今日报告。"
        }

        if (time > 0L) {
            val fmt = SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.getDefault())
            binding.reportTimeText.text = "生成于 ${fmt.format(Date(time))}"
        } else {
            binding.reportTimeText.text = "尚未生成过报告"
        }
    }
}
