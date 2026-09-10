package com.jinyuni.dengbei_care.ui.notifications


import android.Manifest
import android.app.AlarmManager
import android.app.AlertDialog
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.lifecycle.ViewModelProvider
import androidx.navigation.fragment.findNavController
import com.jinyuni.dengbei_care.MacIdBook
import com.jinyuni.dengbei_care.R
import com.jinyuni.dengbei_care.databinding.FragmentNotificationsBinding
import com.jinyuni.dengbei_care.ui.zhuce.ZhuCeViewModel
import com.journeyapps.barcodescanner.ScanContract
import com.journeyapps.barcodescanner.ScanOptions


class NotificationsFragment : Fragment() {

    private var _binding: FragmentNotificationsBinding? = null
    private lateinit var maceditText: EditText
    private lateinit var macsaveButton: Button
    private lateinit var macscanButton: Button
    private lateinit var provtextview: TextView
    private lateinit var notificationViewModel: NotificationsViewModel

    private lateinit var sensornameinputText: EditText
    private lateinit var sensoridinputText: EditText
    private lateinit var sensoridscanButton: Button
    private lateinit var sensoridsaveButton: Button

    // This property is only valid between onCreateView and
    // onDestroyView.
    private val binding get() = _binding!!
    private val viewModel: ZhuCeViewModel by viewModels()
    private lateinit var registerstateview: TextView
    private var registerstate = ""
    private val CAMERA_PERMISSION_REQUEST_CODE = 100
    private var isPermissionRequested = false // 标志位，记录是否已经弹过窗

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        val notificationsViewModel =
            ViewModelProvider(this).get(NotificationsViewModel::class.java)

        _binding = FragmentNotificationsBinding.inflate(inflater, container, false)
        val root: View = binding.root

        maceditText = binding.sensorAddressInput
        macsaveButton = binding.macSendButton
//        provtextview = binding.gatewayLabel
        macscanButton = binding.macScanButton

        sensoridinputText = binding.sensoridinput
        sensoridscanButton = binding.sensoridscan
        sensoridsaveButton = binding.addsensorid

        sensornameinputText = binding.sensornameinput


        macsaveButton.setOnClickListener {
            val input = maceditText.text.toString()
            if (input.length == 12) {
                saveInput(input)
            } else {
                Toast.makeText(requireContext(), "请输入12个字符", Toast.LENGTH_SHORT).show()
            }
        }

        sensoridsaveButton.setOnClickListener{
            val input = sensoridinputText.text.toString()
            val nameinput = sensornameinputText.text.toString()
            if ((nameinput.length > 0)&&(input.length == 8)) {
                val ok = MacIdBook.add(requireContext(),input,nameinput,immediate = true)
                if(ok == false){
                    Toast.makeText(requireContext(), "添加传感器失败,请联系客服", Toast.LENGTH_SHORT).show()
                }else{
//                    val it = Intent(requireContext().applicationContext, MqtttService::class.java).apply {
//                        action = "BLE_ENQUEUE_WRITE_IDS"
//                        putExtra("id", input)
//                    }
//                    requireContext().applicationContext.startService(it)
                    Toast.makeText(requireContext(), "添加传感器成功", Toast.LENGTH_SHORT).show()
                }
            } else {
                Toast.makeText(requireContext(), "请输入传感器编码和别称", Toast.LENGTH_SHORT).show()
            }
        }


        macscanButton.setOnClickListener{
            onScanButtonClick()
        }

        sensoridscanButton.setOnClickListener {
            onScanSensorButtonClick()
        }

        registerstateview = binding.usernameText
        loadInput() // 调用 loadInput 函数来加载之前保存的数据

        notificationViewModel =  NotificationsViewModel.getInstance(requireActivity().application)
//        notificationViewModel.mqtt_broker_state.observe(viewLifecycleOwner) { data ->
//            // 在这里更新 UI，例如更新图表
//            provtextview.text = data
//        }



        return root
    }


//    fun startScan() {
//        val integrator = IntentIntegrator(this)
//        integrator.setDesiredBarcodeFormats(IntentIntegrator.ALL_CODE_TYPES)
//        integrator.setPrompt("请将二维码放入框内扫描")
//        integrator.setCameraId(0) // 使用后置摄像头
//        integrator.setBeepEnabled(true) // 扫描成功后播放提示音
//        integrator.setBarcodeImageEnabled(false) // 不保存扫描的二维码图像
//        integrator.initiateScan()
//    }

    private val barcodeLauncher = registerForActivityResult(ScanContract()) { result ->
        if (result.contents == null) {
            Toast.makeText(requireContext(), "取消", Toast.LENGTH_LONG).show()
        } else {
            val scannedData = result.contents
            maceditText.setText(scannedData)
            Toast.makeText(requireContext(), "扫描完成", Toast.LENGTH_LONG).show()
        }
    }

    fun onScanButtonClick() {
        if (ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
//            if (shouldShowRequestPermissionRationale(Manifest.permission.CAMERA)) {
//                // 向用户解释为什么需要相机权限
//                // 例如，显示一个对话框或提示
//                Toast.makeText(requireContext(), "需要您提供相机权限，才可以扫描二维码", Toast.LENGTH_LONG).show()
//            } else {
//                // 直接请求相机权限
//                ActivityCompat.requestPermissions(requireActivity(), arrayOf(Manifest.permission.CAMERA), CAMERA_PERMISSION_REQUEST_CODE)
//            }
            Toast.makeText(requireContext(), "需要您提供相机权限，才可以扫描二维码", Toast.LENGTH_LONG).show()
            ActivityCompat.requestPermissions(requireActivity(), arrayOf(Manifest.permission.CAMERA), CAMERA_PERMISSION_REQUEST_CODE)
        } else {
            // 权限已授予，启动扫码功能
            val options = ScanOptions()
            options.setDesiredBarcodeFormats(ScanOptions.ALL_CODE_TYPES) // 只扫描二维码
            options.setPrompt("请扫描机身二维码") // 提示文本
            options.setCameraId(0) // 使用后置摄像头
            options.setBeepEnabled(true) // 扫描成功时发出声音
            options.setBarcodeImageEnabled(false) // 不保存扫描的图像
            // 在此处可以设置 ScanOptions 的各种参数，例如扫码格式、提示信息等
            barcodeLauncher.launch(options)
        }
    }

    private val barcodeLauncherSensor = registerForActivityResult(ScanContract()) { result ->
        if (result.contents == null) {
            Toast.makeText(requireContext(), "取消", Toast.LENGTH_LONG).show()
        } else {
            val scannedData = result.contents
            sensoridinputText.setText(scannedData)
            Toast.makeText(requireContext(), "扫描完成", Toast.LENGTH_LONG).show()
        }
    }
    fun onScanSensorButtonClick() {
        if (ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
//            if (shouldShowRequestPermissionRationale(Manifest.permission.CAMERA)) {
//                // 向用户解释为什么需要相机权限
//                // 例如，显示一个对话框或提示
//                Toast.makeText(requireContext(), "需要您提供相机权限，才可以扫描二维码", Toast.LENGTH_LONG).show()
//            } else {
//                // 直接请求相机权限
//                ActivityCompat.requestPermissions(requireActivity(), arrayOf(Manifest.permission.CAMERA), CAMERA_PERMISSION_REQUEST_CODE)
//            }
            Toast.makeText(requireContext(), "需要您提供相机权限，才可以扫描二维码", Toast.LENGTH_LONG).show()
            ActivityCompat.requestPermissions(requireActivity(), arrayOf(Manifest.permission.CAMERA), CAMERA_PERMISSION_REQUEST_CODE)
        } else {
            // 权限已授予，启动扫码功能
            val options = ScanOptions()
            options.setDesiredBarcodeFormats(ScanOptions.ALL_CODE_TYPES) // 只扫描二维码
            options.setPrompt("请扫描机身二维码") // 提示文本
            options.setCameraId(0) // 使用后置摄像头
            options.setBeepEnabled(true) // 扫描成功时发出声音
            options.setBarcodeImageEnabled(false) // 不保存扫描的图像
            // 在此处可以设置 ScanOptions 的各种参数，例如扫码格式、提示信息等
            barcodeLauncherSensor.launch(options)
        }
    }

    private fun addsensor(input: String){
        val prefs = requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
        prefs.edit().putString("sensor", input).apply()
    }

    private fun saveInput(input: String) {

        // 保存输入到 SharedPreferences
        if (isAdded) {
//            requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
//                .putString("mac_addr", input)
//                .apply()
            val prefs = requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
            prefs.edit().putString("mac_addr", input).apply()
            val registerstatee = prefs.getString("registerphone", "")
            if ((registerstatee != "")&&(registerstatee != null)) {
                registerstate = registerstatee
            }
            if (registerstate != "") {
                if(isValidPhoneNumber(registerstate)) {
                    viewModel.setMacAddress(registerstate, input)
                        .observe(viewLifecycleOwner) { success ->
                            if (success) {
                                Toast.makeText(requireContext(), "mac 设置成功", Toast.LENGTH_SHORT)
                                    .show()
                            } else {
                                Toast.makeText(
                                    requireContext(),
                                    "mac 设置失败，请先注册...",
                                    Toast.LENGTH_SHORT
                                ).show()
                            }
                        }
                }else{
                    Toast.makeText(requireContext(), "请登录...", Toast.LENGTH_SHORT).show()
                }
            } else {
                Toast.makeText(requireContext(), "mac 设置失败，请先登录", Toast.LENGTH_SHORT).show()
            }
        }else{
            Toast.makeText(requireContext(), "mac 设置失败，请重试...", Toast.LENGTH_SHORT).show()
        }
        checkAndRequestExactAlarmPermission()
    }

    private fun isValidPhoneNumber(phoneNumber: String): Boolean {
        return phoneNumber.matches(Regex("^1[3-9]\\d{9}$"))
    }

    private fun loadInput() {
        // 从 SharedPreferences 中读取数据
        val prefs = requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
        val savedInput = prefs.getString("mac_addr", "")
        if ((savedInput != null)&&(savedInput != "")) {
            maceditText.setText(savedInput)
        }
        val registerstatee = prefs.getString("registerphone", "")
        if ((registerstatee != "")&&(registerstatee != null)) {
            registerstateview.text = "用户：已登录"
            registerstate = registerstatee
        }else{
            registerstateview.text = "用户：未登录"
        }

//        val provf = prefs.getString("prov_flag", "")
//        if (provf != null) {
//            provtextview.setText("网关：已配网")
//        }else{
//            provtextview.setText("网关：未配网")
//        }
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

//        binding.gatewayButton.setOnClickListener {
//            val currentDestination = findNavController().currentDestination?.id
//            if (currentDestination == R.id.navigation_notifications) {
//                findNavController().navigate(R.id.action_notificationsFragment_to_gatewayFragment)
//            }
//        }
//        binding.loginButton.setOnClickListener {
//            findNavController().navigate(R.id.action_notificationsFragment_to_dengLuFragment)
//        }
        binding.registerButton.setOnClickListener {
            val currentDestination = findNavController().currentDestination?.id
            if (currentDestination == R.id.navigation_notifications) {
                findNavController().navigate(R.id.action_notificationsFragment_to_zhuCeFragment)
            }
        }

        binding.opensensorlistbutton.setOnClickListener {
            val currentDestination = findNavController().currentDestination?.id
            if (currentDestination == R.id.navigation_notifications) {
                findNavController().navigate(R.id.action_notificationsFragment_to_SensorIdListFragment)
            }
        }

        binding.viewReportButton.setOnClickListener {
            com.jinyuni.dengbei_care.report.ReportScheduler.runNow(requireContext())
            val intent = Intent(requireContext(), com.jinyuni.dengbei_care.report.ReportActivity::class.java)
            startActivity(intent)
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }


    private fun checkAndRequestExactAlarmPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val alarmManager = requireContext().getSystemService(Context.ALARM_SERVICE) as AlarmManager
            if (!alarmManager.canScheduleExactAlarms()) {
                // 没有权限，提示用户去申请
                showExactAlarmPermissionDialog()
            } else {
                // 已经拥有权限，可以继续使用 AlarmManager
                Log.d("Permission", "Exact alarm permission already granted")
            }
        } else {
            // Android 12 以下版本不需要处理此权限
            Log.d("Permission", "Exact alarm permission not required")
        }
    }

    private fun showExactAlarmPermissionDialog() {
        isPermissionRequested = true // 标记为已弹过窗
        AlertDialog.Builder(requireContext())
            .setTitle("需要精确闹钟权限")
            .setMessage("为了确保任务在精确时间执行，请授予精确闹钟权限。")
            .setPositiveButton("去设置") { _, _ ->
                requestExactAlarmPermission() // 调用申请权限的方法
            }
            .setNegativeButton("取消", null)
            .setOnDismissListener {
                // 弹窗关闭时的处理
                isPermissionRequested = false // 重置标志位，允许再次弹窗
            }
            .show()
    }

    private fun requestExactAlarmPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val intent = Intent(Settings.ACTION_REQUEST_SCHEDULE_EXACT_ALARM)
            startActivity(intent)
        }
    }
}