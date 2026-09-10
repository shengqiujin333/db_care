package com.jinyuni.dengbei_care.ui.detail

import android.app.DatePickerDialog
import android.app.TimePickerDialog
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.findNavController
import androidx.recyclerview.widget.LinearLayoutManager
import com.github.mikephil.charting.charts.LineChart
import com.github.mikephil.charting.components.XAxis
import com.github.mikephil.charting.components.YAxis
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import com.github.mikephil.charting.formatter.ValueFormatter
import com.jinyuni.dengbei_care.AlarmEvent
import com.jinyuni.dengbei_care.DatabaseHelperInstance
import com.jinyuni.dengbei_care.MacIdBook
import com.jinyuni.dengbei_care.TemperatureDatabaseHelper
import com.jinyuni.dengbei_care.databinding.FragmentDeviceDetailBinding
import java.text.DecimalFormat
import java.text.SimpleDateFormat
import java.util.Calendar
import java.util.Date
import java.util.Locale

class DecimalFormatter : ValueFormatter() {
    override fun getPointLabel(entry: Entry?): String = "%.1f".format(entry?.y)
}

class DeviceDetailFragment : Fragment() {

    private var _binding: FragmentDeviceDetailBinding? = null
    private val binding get() = _binding!!

    private val calendar = Calendar.getInstance()
    private val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.getDefault())

    private lateinit var lineChart: LineChart
    private lateinit var alarmAdapter: AlarmEventAdapter

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentDeviceDetailBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        val devId = arguments?.getString("devid") ?: ""
        val deviceName = MacIdBook.all(requireContext()).find { it.first == devId }?.second
            ?: devId
        binding.toolbar.title = deviceName
        binding.toolbar.setNavigationOnClickListener {
            findNavController().navigateUp()
        }

        lineChart = binding.lineChart
        setupChart()

        binding.startDateText.setOnClickListener { showDateTimePicker(binding.startDateText) }
        binding.endDateText.setOnClickListener { showDateTimePicker(binding.endDateText) }
        binding.searchButton.setOnClickListener { searchTemperature(devId) }

        setupAlarmList(devId)
    }

    private fun setupChart() {
        lineChart.description.text = "温湿度变化"

        val xAxis = lineChart.xAxis
        xAxis.position = XAxis.XAxisPosition.BOTTOM
        xAxis.valueFormatter = object : ValueFormatter() {
            private val fmt = SimpleDateFormat("HH:mm", Locale.getDefault())
            override fun getFormattedValue(value: Float): String {
                return fmt.format(Date(value.toLong()))
            }
        }
        xAxis.setLabelCount(10, false)
        xAxis.labelRotationAngle = -45f
        xAxis.granularity = 1f
        xAxis.isGranularityEnabled = true

        val leftAxis = lineChart.axisLeft
        leftAxis.setAxisMinimum(-30f)
        leftAxis.setAxisMaximum(70f)
        leftAxis.setLabelCount(10, false)
        leftAxis.setDrawLabels(true)
        leftAxis.textColor = requireContext().getColor(com.jinyuni.dengbei_care.R.color.brand_error)
        leftAxis.axisLineColor = requireContext().getColor(com.jinyuni.dengbei_care.R.color.brand_error)
        leftAxis.valueFormatter = object : ValueFormatter() {
            private val df = DecimalFormat("0.0")
            override fun getFormattedValue(value: Float): String = df.format(value)
        }

        val rightAxis = lineChart.axisRight
        rightAxis.setAxisMinimum(0f)
        rightAxis.setAxisMaximum(100f)
        rightAxis.setLabelCount(10, false)
        rightAxis.setDrawLabels(true)
        rightAxis.textColor = requireContext().getColor(com.jinyuni.dengbei_care.R.color.brand_primary)
        rightAxis.axisLineColor = requireContext().getColor(com.jinyuni.dengbei_care.R.color.brand_primary)
        rightAxis.valueFormatter = object : ValueFormatter() {
            private val df = DecimalFormat("0.0")
            override fun getFormattedValue(value: Float): String = df.format(value)
        }

        lineChart.invalidate()
    }

    private fun updateChart(temps: List<Entry>, humis: List<Entry>) {
        val leftDataSet = LineDataSet(temps, "温度 ℃").apply {
            color = requireContext().getColor(com.jinyuni.dengbei_care.R.color.brand_error)
            valueTextColor = requireContext().getColor(com.jinyuni.dengbei_care.R.color.brand_error)
            valueTextSize = 10f
            axisDependency = YAxis.AxisDependency.LEFT
            setDrawValues(true)
            lineWidth = 1.5f
            setDrawCircles(true)
            circleRadius = 2f
        }
        val rightDataSet = LineDataSet(humis, "湿度 RH").apply {
            color = requireContext().getColor(com.jinyuni.dengbei_care.R.color.brand_primary)
            valueTextColor = requireContext().getColor(com.jinyuni.dengbei_care.R.color.brand_primary)
            valueTextSize = 10f
            axisDependency = YAxis.AxisDependency.RIGHT
            setDrawValues(true)
            lineWidth = 1.5f
            setDrawCircles(true)
            circleRadius = 2f
        }

        val lineData = LineData(leftDataSet, rightDataSet)
        lineChart.data = lineData
        lineChart.notifyDataSetChanged()
        lineChart.invalidate()
    }

    private fun showDateTimePicker(textView: TextView) {
        val dateSetListener = DatePickerDialog.OnDateSetListener { _, year, month, dayOfMonth ->
            calendar.set(Calendar.YEAR, year)
            calendar.set(Calendar.MONTH, month)
            calendar.set(Calendar.DAY_OF_MONTH, dayOfMonth)
            TimePickerDialog(
                context,
                { _, hourOfDay, minute ->
                    calendar.set(Calendar.HOUR_OF_DAY, hourOfDay)
                    calendar.set(Calendar.MINUTE, minute)
                    textView.text = dateFormat.format(calendar.time)
                },
                calendar.get(Calendar.HOUR_OF_DAY),
                calendar.get(Calendar.MINUTE),
                true
            ).show()
        }
        DatePickerDialog(
            requireContext(),
            dateSetListener,
            calendar.get(Calendar.YEAR),
            calendar.get(Calendar.MONTH),
            calendar.get(Calendar.DAY_OF_MONTH)
        ).show()
    }

    private fun isDateValid(date: String, fmt: SimpleDateFormat): Boolean = try {
        fmt.parse(date); true
    } catch (e: Exception) {
        false
    }

    private fun searchTemperature(devId: String) {
        val startDate = binding.startDateText.text.toString()
        val endDate = binding.endDateText.text.toString()

        val fmt = SimpleDateFormat("yyyy-M-d HH:mm", Locale.getDefault())
        if (!isDateValid(startDate, fmt) || !isDateValid(endDate, fmt)) {
            Toast.makeText(context, "时间范围必须在 50 分钟以内", Toast.LENGTH_SHORT).show()
            return
        }

        val startTime = fmt.parse(startDate)?.time ?: return
        val endTime = fmt.parse(endDate)?.time ?: return

        if (endTime - startTime > 49 * 60 * 1000) {
            Toast.makeText(context, "时间范围必须在 50 分钟以内", Toast.LENGTH_SHORT).show()
            return
        }

        val dbHelper = DatabaseHelperInstance.getDatabaseHelper(requireContext())
        dbHelper.getTemperatureHumidityByTimeRange(devId, startTime / 1000, endTime / 1000)
            .observe(viewLifecycleOwner) { pair ->
                updateChart(pair.first, pair.second)
            }
    }

    private fun setupAlarmList(devId: String) {
        alarmAdapter = AlarmEventAdapter()
        binding.alarmRecycler.layoutManager = LinearLayoutManager(requireContext())
        binding.alarmRecycler.adapter = alarmAdapter

        val dbHelper = DatabaseHelperInstance.getDatabaseHelper(requireContext())
        val now = System.currentTimeMillis() / 1000
        val start = now - 24 * 60 * 60

        val alarms = dbHelper.getAlarmEvents(devId, start, now)
        if (alarms.isEmpty()) {
            binding.alarmEmpty.visibility = View.VISIBLE
            binding.alarmRecycler.visibility = View.GONE
        } else {
            binding.alarmEmpty.visibility = View.GONE
            binding.alarmRecycler.visibility = View.VISIBLE
            alarmAdapter.submit(alarms)
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
