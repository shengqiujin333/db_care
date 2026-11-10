package com.jinyuni.dengbei_care.ui.home

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.CountDownTimer
import android.provider.Settings
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.findNavController
import androidx.recyclerview.widget.LinearLayoutManager
import com.jinyuni.dengbei_care.MacIdBook
import com.jinyuni.dengbei_care.MainActivity
import com.jinyuni.dengbei_care.MqtttService
import com.jinyuni.dengbei_care.R
import com.jinyuni.dengbei_care.databinding.FragmentHomeBinding
import java.text.SimpleDateFormat
import java.util.Calendar
import java.util.Locale

class HomeFragment : Fragment() {

    private var _binding: FragmentHomeBinding? = null
    private val binding get() = _binding!!

    private val calendar = Calendar.getInstance()
    private lateinit var buttonToggle: Button
    private var isStarted = false
    private var countDownTimer: CountDownTimer? = null

    private lateinit var homeViewModel: HomeViewModel
    private lateinit var cardAdapter: DeviceCardAdapter

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        homeViewModel = HomeViewModel.getInstance(requireActivity().application)

        _binding = FragmentHomeBinding.inflate(inflater, container, false)
        val root: View = binding.root

        buttonToggle = binding.startGuardButton
        buttonToggle.setOnClickListener { toggleButton() }

        binding.addDeviceButton.setOnClickListener {
            findNavController().navigate(R.id.action_homeFragment_to_sensorIdListFragment)
        }

        setupRecyclerView()
        observeViewModel()
        restoreGuardState()

        return root
    }

    private fun setupRecyclerView() {
        cardAdapter = DeviceCardAdapter { devId ->
            val bundle = Bundle().apply {
                putString("devid", devId)
            }
            findNavController().navigate(R.id.action_homeFragment_to_deviceDetailFragment, bundle)
        }
        binding.deviceCardRecycler.layoutManager = LinearLayoutManager(requireContext())
        binding.deviceCardRecycler.adapter = cardAdapter
    }

    private fun observeViewModel() {
        homeViewModel.devices.observe(viewLifecycleOwner) { devices ->
            val list = devices.values.toList().sortedBy { it.displayName() }
            cardAdapter.submitList(list)
            val isEmpty = list.isEmpty()
            binding.emptyState.visibility = if (isEmpty) View.VISIBLE else View.GONE
            binding.deviceCardRecycler.visibility = if (isEmpty) View.GONE else View.VISIBLE
        }

        homeViewModel.startData.observe(viewLifecycleOwner) { running ->
            updateGuardButtonState(running == 1)
        }

        homeViewModel.linkStatus.observe(viewLifecycleOwner) { status ->
            binding.linkStatusText.text = status
            binding.linkStatusText.visibility =
                if (status.isBlank()) View.GONE else View.VISIBLE
        }

        // 链路状态可点击：权限→应用设置；蓝牙→蓝牙设置；定位→定位设置；网关→提示到“我的”页配置
        binding.linkStatusText.setOnClickListener {
            val status = homeViewModel.linkStatus.value.orEmpty()
            val ctx = requireContext()
            val intent = when {
                status.contains("权限") -> Intent(
                    Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                    Uri.parse("package:" + ctx.packageName)
                )
                status.contains("定位") -> Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS)
                status.contains("蓝牙未开启") -> Intent(Settings.ACTION_BLUETOOTH_SETTINGS)
                else -> null
            }
            if (intent != null) {
                try {
                    startActivity(intent)
                } catch (e: Exception) {
                    Log.w("HomeFragment", "open settings failed: ${e.message}")
                }
            } else if (status.contains("网关")) {
                android.widget.Toast.makeText(
                    ctx, "请在“我的 → 网关与上传设置”中选择或保存网关", android.widget.Toast.LENGTH_SHORT
                ).show()
            }
        }
    }

    private fun restoreGuardState() {
        val prefs = requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
        val tempState = prefs.getString("measure_state", "")
        val timed = prefs.getString("finish_guard_time", "")
        when (tempState) {
            "starting" -> {
                buttonToggle.text = "启动"
                isStarted = false
                homeViewModel._startData.value = 0
            }
            "stopping" -> {
                buttonToggle.text = "停止"
                isStarted = true
                homeViewModel._startData.value = 1
                val timec = getCurrentTime()
                if (!timed.isNullOrEmpty() && isTimeBLaterThanC(timed, timec)) {
                    val minutesDifference = calculateMinutesDifference(timed, timec) * 3600000L
                    cancelPreviousCountDown()
                    startCountDown(minutesDifference)
                } else {
                    buttonToggle.text = "启动"
                    isStarted = false
                    homeViewModel._startData.value = 0
                }
            }
        }
    }

    private fun updateGuardButtonState(running: Boolean) {
        if (running) {
            buttonToggle.text = "停止"
            buttonToggle.setBackgroundColor(
                ContextCompat.getColor(requireContext(), R.color.brand_error)
            )
        } else {
            buttonToggle.text = "启动"
            buttonToggle.setBackgroundColor(
                ContextCompat.getColor(requireContext(), R.color.brand_primary)
            )
        }
    }

    private fun toggleButton() {
        if (!isStarted) {
            // 开启守护前按 API 请求权限（不请求定位；扫描权限由网关设置页按需请求）
            (activity as? MainActivity)?.requestStartupPermissions()
        }
        val guardState = if (isStarted) "0.0" else "1.0"
        val intent = Intent(context, MqtttService::class.java).apply {
            putExtra("guardaction", guardState)
            action = "START_STOP_GUARD"
        }
        context?.startService(intent)

        val buttonState: String
        if (isStarted) {
            buttonState = "starting"
            buttonToggle.setTextColor(ContextCompat.getColor(requireContext(), R.color.brand_on_primary))
            buttonToggle.text = "启动"
            cancelPreviousCountDown()
            val timeB = addHoursToCurrentTime(0.0)
            requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
                .putString("finish_guard_time", timeB)
                .apply()
        } else {
            buttonState = "stopping"
            buttonToggle.setTextColor(ContextCompat.getColor(requireContext(), R.color.white))
            cancelPreviousCountDown()
            buttonToggle.text = "停止"
            val prefs = requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
            val guardtimedata = prefs.getString("guardtime", "")
            if (!guardtimedata.isNullOrEmpty()) {
                val res0 = guardtimedata.toDoubleOrNull()
                if (res0 != null) {
                    val res1 = res0 * 60 * 60 * 1000
                    val res2 = res1.toLong()
                    startCountDown(res2)
                    val timeB = addHoursToCurrentTime(res0)
                    requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
                        .putString("finish_guard_time", timeB)
                        .apply()
                }
            }
        }
        requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
            .putString("measure_state", buttonState)
            .apply()
        isStarted = !isStarted
        homeViewModel._startData.value = if (isStarted) 1 else 0
    }

    private fun getCurrentTime(): String {
        val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())
        return dateFormat.format(calendar.time)
    }

    private fun addHoursToCurrentTime(hours: Double): String {
        val calendar = Calendar.getInstance()
        calendar.add(Calendar.HOUR, hours.toInt())
        calendar.add(Calendar.MINUTE, ((hours % 1) * 60).toInt())
        val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())
        return dateFormat.format(calendar.time)
    }

    private fun isTimeBLaterThanC(timeB: String, timeC: String): Boolean {
        val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())
        return dateFormat.parse(timeB)?.after(dateFormat.parse(timeC)) ?: false
    }

    private fun calculateMinutesDifference(timeB: String, timeC: String): Long {
        val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())
        val dateB = dateFormat.parse(timeB) ?: return 0
        val dateC = dateFormat.parse(timeC) ?: return 0
        val diff = dateB.time - dateC.time
        return diff / (60 * 1000)
    }

    private fun startCountDown(timeInMillis: Long) {
        countDownTimer = object : CountDownTimer(timeInMillis, 60000) {
            override fun onTick(millisUntilFinished: Long) {}
            override fun onFinish() {
                buttonToggle.callOnClick()
            }
        }.start()
    }

    private fun cancelPreviousCountDown() {
        countDownTimer?.cancel()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        cancelPreviousCountDown()
        _binding = null
    }

    override fun onPause() {
        super.onPause()
        Log.d("MqttService", "onPause called")
    }

    override fun onResume() {
        super.onResume()
        Log.d("MqttService", "onResume called")
        // 让 MacIdBook 里的设备提前占位,卡片先显示出来,等数据到了再填充
        MacIdBook.all(requireContext()).forEach { (id, name) ->
            homeViewModel.ensureDevice(id, name)
        }
    }
}
