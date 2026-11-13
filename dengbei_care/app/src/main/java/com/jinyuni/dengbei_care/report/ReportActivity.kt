package com.jinyuni.dengbei_care.report

import android.content.Context
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.Observer
import androidx.work.WorkInfo
import androidx.work.WorkManager
import com.jinyuni.dengbei_care.DatabaseHelperInstance
import com.jinyuni.dengbei_care.MacIdBook
import com.jinyuni.dengbei_care.databinding.ActivityReportBinding
import java.text.SimpleDateFormat
import java.util.Calendar
import java.util.Date
import java.util.Locale

class ReportActivity : AppCompatActivity() {

    private lateinit var binding: ActivityReportBinding

    /** 本次点击触发的生成是否尚未结束；用于阻止并行触发与避免初始观察误提示。 */
    private var generating = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityReportBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.toolbar.setNavigationOnClickListener { finish() }
        binding.regenerateButton.setOnClickListener { startRegenerate() }

        observeWork()
        loadCachedReport()
    }

    /** 点击「重新生成」：串行触发一次，立即反馈进度并禁用按钮（AA-006 N-3）。 */
    private fun startRegenerate() {
        if (generating) return
        generating = true
        binding.loading.visibility = View.VISIBLE
        binding.regenerateButton.isEnabled = false
        try {
            ReportScheduler.runNow(this)
        } catch (e: Exception) {
            generating = false
            binding.loading.visibility = View.GONE
            binding.regenerateButton.isEnabled = true
            Toast.makeText(this, ReportRegenerateFeedback.TEXT_FAILED, Toast.LENGTH_SHORT).show()
        }
    }

    private fun observeWork() {
        WorkManager.getInstance(this)
            .getWorkInfosForUniqueWorkLiveData("daily_report_oneshot")
            .observe(this, Observer { infos ->
                if (infos.isNullOrEmpty()) return@Observer
                val info = infos[0]
                when (info.state) {
                    WorkInfo.State.RUNNING, WorkInfo.State.ENQUEUED -> {
                        if (generating) {
                            binding.loading.visibility = View.VISIBLE
                            binding.regenerateButton.isEnabled = false
                        }
                    }
                    WorkInfo.State.SUCCEEDED, WorkInfo.State.FAILED -> {
                        val wasGenerating = generating
                        generating = false
                        binding.loading.visibility = View.GONE
                        binding.regenerateButton.isEnabled = true
                        val text = loadCachedReport()
                        if (wasGenerating) {
                            showRegenerateFeedback(info.state == WorkInfo.State.SUCCEEDED, text)
                        }
                    }
                    else -> {
                        // CANCELLED / BLOCKED：保持当前界面；若本次点击尚未结束，等待后续终态。
                    }
                }
            })
    }

    private fun showRegenerateFeedback(succeeded: Boolean, text: String?) {
        val deviceCount = try {
            MacIdBook.all(this).size
        } catch (e: Exception) {
            0
        }
        val decision = ReportRegenerateFeedback.afterRun(
            succeeded = succeeded,
            hasContent = !text.isNullOrBlank(),
            deviceCount = deviceCount,
            sampleCount = yesterdaySampleCount()
        )
        ReportRegenerateFeedback.textFor(decision.message)?.let { message ->
            Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
        }
    }

    /** 昨日样本数（与 DailyReportWorker 的“昨日”窗口一致）；查询失败返回 -1（未知）。 */
    private fun yesterdaySampleCount(): Int {
        return try {
            val savedIds = MacIdBook.all(this)
            if (savedIds.isEmpty()) return 0
            val cal = Calendar.getInstance().apply {
                add(Calendar.DAY_OF_YEAR, -1)
                set(Calendar.HOUR_OF_DAY, 0)
                set(Calendar.MINUTE, 0)
                set(Calendar.SECOND, 0)
                set(Calendar.MILLISECOND, 0)
            }
            val dayStart = cal.timeInMillis / 1000
            val dayEnd = dayStart + 24 * 60 * 60
            val db = DatabaseHelperInstance.getDatabaseHelper(applicationContext)
            savedIds.sumOf { (id, _) ->
                try {
                    db.getSamplesInRange(id, dayStart, dayEnd).size
                } catch (e: Exception) {
                    0
                }
            }
        } catch (e: Exception) {
            -1
        }
    }

    /** 读取缓存报告；返回文本（无缓存为 null）。 */
    private fun loadCachedReport(): String? {
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
        return text
    }
}
