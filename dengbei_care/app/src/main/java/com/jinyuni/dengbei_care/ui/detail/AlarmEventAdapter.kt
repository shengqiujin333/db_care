package com.jinyuni.dengbei_care.ui.detail

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.jinyuni.dengbei_care.AlarmEvent
import com.jinyuni.dengbei_care.TemperatureDatabaseHelper
import com.jinyuni.dengbei_care.databinding.ItemAlarmEventBinding
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class AlarmEventAdapter : RecyclerView.Adapter<AlarmEventAdapter.VH>() {

    private val items = mutableListOf<AlarmEvent>()
    private val fmt = SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.getDefault())

    fun submit(list: List<AlarmEvent>) {
        items.clear()
        items.addAll(list)
        notifyDataSetChanged()
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VH {
        val binding = ItemAlarmEventBinding.inflate(
            LayoutInflater.from(parent.context), parent, false
        )
        return VH(binding)
    }

    override fun onBindViewHolder(holder: VH, position: Int) {
        holder.bind(items[position])
    }

    override fun getItemCount(): Int = items.size

    inner class VH(private val binding: ItemAlarmEventBinding) :
        RecyclerView.ViewHolder(binding.root) {

        fun bind(event: AlarmEvent) {
            binding.alarmMessage.text = event.message
            binding.alarmTime.text = fmt.format(Date(event.time * 1000))

            val color = when (event.type) {
                TemperatureDatabaseHelper.ALARM_TYPE_EMERGENCY ->
                    com.jinyuni.dengbei_care.R.color.brand_error
                TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD ->
                    com.jinyuni.dengbei_care.R.color.brand_warning
                TemperatureDatabaseHelper.ALARM_TYPE_DROP ->
                    com.jinyuni.dengbei_care.R.color.brand_secondary
                else -> com.jinyuni.dengbei_care.R.color.brand_primary
            }
            binding.alarmDot.setBackgroundResource(com.jinyuni.dengbei_care.R.drawable.bg_alarm_dot)
            try {
                val ctx = binding.root.context
                val androidColor = ctx.getColor(color)
                val drawable = binding.alarmDot.background
                drawable.setTint(androidColor)
            } catch (_: Exception) {
            }
        }
    }
}
