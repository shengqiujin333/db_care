package com.jinyuni.dengbei_care.ui.home

import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.github.mikephil.charting.charts.LineChart
import com.github.mikephil.charting.components.XAxis
import com.github.mikephil.charting.components.YAxis
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import com.google.android.material.chip.Chip
import com.jinyuni.dengbei_care.DeviceState
import com.jinyuni.dengbei_care.R
import com.jinyuni.dengbei_care.Severity
import com.jinyuni.dengbei_care.databinding.ItemDeviceCardBinding
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class DeviceCardAdapter(
    private val onCardClick: (String) -> Unit
) : ListAdapter<DeviceState, DeviceCardAdapter.CardViewHolder>(DIFF) {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): CardViewHolder {
        val binding = ItemDeviceCardBinding.inflate(
            LayoutInflater.from(parent.context), parent, false
        )
        return CardViewHolder(binding)
    }

    override fun onBindViewHolder(holder: CardViewHolder, position: Int) {
        holder.bind(getItem(position))
    }

    inner class CardViewHolder(
        private val binding: ItemDeviceCardBinding
    ) : RecyclerView.ViewHolder(binding.root) {

        init {
            binding.root.setOnClickListener {
                val devId = binding.root.tag as? String ?: return@setOnClickListener
                onCardClick(devId)
            }
            setupSparkline(binding.sparkline)
        }

        fun bind(state: DeviceState) {
            binding.root.tag = state.devId

            binding.deviceName.text = state.displayName()

            binding.tempValue.text = if (state.latestTime == 0L) {
                "--"
            } else {
                "%.1f°C".format(state.latestTemp)
            }
            binding.humiValue.text = if (state.latestTime == 0L) {
                "--"
            } else {
                "%.0f%%".format(state.latestHumi)
            }

            bindStatusChip(binding.statusChip, state)

            if (state.alarmMessage.isNotBlank() && state.alarmSeverity != Severity.NORMAL) {
                binding.alarmMessage.text = state.alarmMessage
                binding.alarmMessage.visibility = View.VISIBLE
            } else {
                binding.alarmMessage.visibility = View.GONE
            }

            bindSparkline(binding.sparkline, state.sparkline)
        }

        private fun bindStatusChip(chip: Chip, state: DeviceState) {
            when {
                state.latestTime == 0L -> {
                    chip.text = "未收到"
                    chip.setChipBackgroundColorResource(R.color.brand_secondary)
                    chip.setTextColor(Color.BLACK)
                }
                !state.isOnline() -> {
                    chip.text = "离线"
                    chip.setChipBackgroundColorResource(R.color.brand_secondary)
                    chip.setTextColor(Color.BLACK)
                }
                state.alarmSeverity == Severity.ALARM -> {
                    chip.text = "紧急"
                    chip.setChipBackgroundColorResource(R.color.brand_error)
                    chip.setTextColor(Color.WHITE)
                }
                state.alarmSeverity == Severity.WARNING -> {
                    chip.text = "警告"
                    chip.setChipBackgroundColorResource(R.color.brand_warning)
                    chip.setTextColor(Color.BLACK)
                }
                else -> {
                    chip.text = "正常"
                    chip.setChipBackgroundColorResource(R.color.brand_ok)
                    chip.setTextColor(Color.WHITE)
                }
            }
        }

        private fun setupSparkline(chart: LineChart) {
            chart.description.isEnabled = false
            chart.legend.isEnabled = false
            chart.setTouchEnabled(false)
            chart.setDrawGridBackground(false)
            chart.setDrawBorders(false)
            chart.setNoDataText("")
            chart.setPinchZoom(false)
            chart.isDragEnabled = false
            chart.setScaleEnabled(false)

            val xAxis = chart.xAxis
            xAxis.setDrawGridLines(false)
            xAxis.setDrawAxisLine(false)
            xAxis.setDrawLabels(false)
            xAxis.position = XAxis.XAxisPosition.BOTTOM

            val left = chart.axisLeft
            left.setDrawGridLines(false)
            left.setDrawAxisLine(false)
            left.setDrawLabels(false)
            left.setDrawZeroLine(false)

            val right = chart.axisRight
            right.setDrawGridLines(false)
            right.setDrawAxisLine(false)
            right.setDrawLabels(false)
            right.setDrawZeroLine(false)
            right.isEnabled = false
            chart.axisLeft.isEnabled = false
        }

        private fun bindSparkline(chart: LineChart, data: List<Entry>) {
            if (data.isEmpty()) {
                chart.clear()
                return
            }
            val ctx = chart.context
            val dataSet = LineDataSet(data, "").apply {
                color = ctx.getColor(R.color.brand_primary)
                setDrawCircles(false)
                setDrawValues(false)
                lineWidth = 1.5f
                mode = LineDataSet.Mode.CUBIC_BEZIER
                setDrawFilled(true)
                fillColor = ctx.getColor(R.color.brand_primary)
                fillAlpha = 60
                isHighlightEnabled = false
            }
            val lineData = LineData(dataSet)
            chart.data = lineData
            chart.invalidate()
        }
    }

    companion object {
        private val DIFF = object : DiffUtil.ItemCallback<DeviceState>() {
            override fun areItemsTheSame(oldItem: DeviceState, newItem: DeviceState): Boolean =
                oldItem.devId == newItem.devId

            override fun areContentsTheSame(oldItem: DeviceState, newItem: DeviceState): Boolean =
                oldItem == newItem
        }
    }
}
