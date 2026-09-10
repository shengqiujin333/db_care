package com.jinyuni.dengbei_care.ui.gateway

import android.Manifest
import android.R
import android.content.Context
import android.content.pm.PackageManager
import android.net.wifi.ScanResult
import android.net.wifi.WifiManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.EditText
import android.widget.Spinner
import android.widget.Toast
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.view.MenuHost
import androidx.core.view.MenuProvider
import androidx.fragment.app.Fragment
import androidx.lifecycle.Lifecycle
import androidx.navigation.fragment.findNavController
import com.espressif.provisioning.ESPConstants
import com.espressif.provisioning.ESPDevice
import com.espressif.provisioning.ESPProvisionManager
import com.espressif.provisioning.listeners.ProvisionListener
import com.jinyuni.dengbei_care.databinding.FragmentGatewayBinding


//import com.espressif.provisioning.device.WiFiDevice


class GatewayFragment : Fragment(){
    private var _binding: FragmentGatewayBinding? = null
    private val binding get() = _binding ?: throw IllegalStateException("Binding is not initialized")
    private lateinit var etWifiName: EditText
    private lateinit var etWifiPassword: EditText
    private lateinit var btnConnect: Button
    private lateinit var espDevice: ESPDevice
    private lateinit var requestPermissionLauncher: ActivityResultLauncher<String>
    private val handler = Handler(Looper.getMainLooper())
    private lateinit var wifiManager: WifiManager
    private lateinit var spinner: Spinner
    private lateinit var wifiscanButton: Button
    private var delayedTaskscan: Runnable? = null
    private var delayedTaskconnect: Runnable? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Register the permission launcher
        requestPermissionLauncher = registerForActivityResult(
            ActivityResultContracts.RequestPermission()
        ) { isGranted ->
            if (isGranted) {
                // Permission granted, proceed with operation
//                configureEsp32(binding.wifiname.text.toString(), binding.step3EditText.text.toString())
            } else {
                // Permission denied
                Toast.makeText(requireContext(), "请给定位置权限", Toast.LENGTH_SHORT).show()
            }
        }
    }
    private fun setupMenu() {
        val menuHost: MenuHost = requireActivity()
        menuHost.addMenuProvider(object : MenuProvider {
            override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) {
                // No need to inflate menu for now
            }

            override fun onMenuItemSelected(menuItem: MenuItem): Boolean {
                return when (menuItem.itemId) {
                    android.R.id.home -> {
                        findNavController().navigateUp()
                        true
                    }
                    else -> false
                }
            }
        }, viewLifecycleOwner, Lifecycle.State.RESUMED)
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {  // 修改这里
        _binding = FragmentGatewayBinding.inflate(inflater, container, false)
        (activity as AppCompatActivity).supportActionBar?.setDisplayHomeAsUpEnabled(true)
        setupMenu()

        etWifiName = binding.wifiname

        wifiManager = requireContext().getSystemService(Context.WIFI_SERVICE) as WifiManager
        spinner = binding.wifispinner
        wifiscanButton = binding.wifisbutton
        wifiscanButton.setOnClickListener {

            if (!hasPermissions()) {
                Toast.makeText(requireContext(), "扫描附近wifi需要位置相关权限，请给予后重新扫描", Toast.LENGTH_SHORT).show()
                requestPermissions()

            }else {

                binding.wifisbutton.isEnabled = false
                delayedTaskscan = Runnable {
                    binding.wifisbutton.isEnabled = true
                }
                handler.postDelayed(delayedTaskscan!!, 30000)

//            binding.wifisbutton.postDelayed({
//                binding.wifisbutton.isEnabled = true
//            }, 30000) // 15000 milliseconds = 15 seconds
                scanWifi()
            }
        }

        spinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parentView: AdapterView<*>, selectedItemView: View?, position: Int, id: Long) {
                // 获取选中的内容，并将其设置到 EditText 中
                val selectedWifiName = parentView.getItemAtPosition(position).toString()
                etWifiName.setText(selectedWifiName)
            }

            override fun onNothingSelected(parentView: AdapterView<*>) {
                // 如果没有选中任何项，可以设置 EditText 的默认值或保持为空
                etWifiName.setText("")
            }
        }

        return binding.root
    }
    @Suppress("DEPRECATION")
    private fun scanWifi() {
        try {
            if (!wifiManager.isWifiEnabled) {
                Toast.makeText(requireContext(), "请先启用 WiFi", Toast.LENGTH_SHORT).show()
                return
            }

            // 开始扫描
            wifiManager.startScan()
            val results: List<ScanResult> = wifiManager.scanResults

            // 提取 WiFi 名称 (SSID)
            val wifiNames = results.map { it.SSID }.filter { it.isNotEmpty() }.toMutableList()

            // 在列表顶部添加提示项
            wifiNames.add(0, "WiFi 列表")

            if (wifiNames.size == 1) { // 只有提示项，没有扫描结果
                Toast.makeText(requireContext(), "未发现 WiFi 网络", Toast.LENGTH_SHORT).show()
            } else {
                // 更新 Spinner 的数据
                val adapter = ArrayAdapter(requireContext(), R.layout.simple_spinner_item, wifiNames)
                adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
                spinner.adapter = adapter
            }
        }catch (e: SecurityException) {
            Toast.makeText(requireContext(), "缺少必要的权限，请检查设置", Toast.LENGTH_SHORT).show()
        }
    }


    private fun hasPermissions(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.NEARBY_WIFI_DEVICES) == PackageManager.PERMISSION_GRANTED
        } else {
            ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED
        }
    }

    private fun requestPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            ActivityCompat.requestPermissions(requireActivity(), arrayOf(Manifest.permission.NEARBY_WIFI_DEVICES), 1)
        } else {
            ActivityCompat.requestPermissions(requireActivity(), arrayOf(Manifest.permission.ACCESS_FINE_LOCATION), 1)
        }
    }



    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.connectButton.setOnClickListener {
            val wifiName = binding.wifiname.text.toString()
            val wifiPassword = binding.step3EditText.text.toString()
            if (wifiName.isNotEmpty() && wifiPassword.isNotEmpty()) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    if (ContextCompat.checkSelfPermission(
                            requireContext(),
                            Manifest.permission.ACCESS_FINE_LOCATION
                        ) == PackageManager.PERMISSION_GRANTED
                    ) {
                        // Permission is already granted
                        configureEsp32(wifiName, wifiPassword)
                    } else {
                        // Request permission
                        requestPermissionLauncher.launch(Manifest.permission.ACCESS_FINE_LOCATION)
                    }
                } else {
                    // Android version is below M, so permissions are handled differently
                    configureEsp32(wifiName, wifiPassword)
                }

                // Disable the button and start a countdown
                binding.connectButton.isEnabled = false
//                binding.connectButton.postDelayed({
//                    binding.connectButton.isEnabled = true
//                }, 15000) // 15000 milliseconds = 15 seconds

                delayedTaskconnect = Runnable {
                    binding.connectButton.isEnabled = true
                }
                handler.postDelayed(delayedTaskconnect!!, 17000)

            } else {
                Toast.makeText(requireContext(), "Please enter both WiFi name and password", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun saveInput(input: String) {
        // 保存输入到 SharedPreferences
        requireActivity().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
            .putString("prov_flag", input)
            .apply()

        Toast.makeText(requireContext(), "已保存", Toast.LENGTH_SHORT).show()
    }


    private fun configureEsp32(ssid: String, password: String) {
        if (ActivityCompat.checkSelfPermission(
                requireContext(),
                Manifest.permission.ACCESS_FINE_LOCATION
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            // TODO: Consider calling
            //    ActivityCompat#requestPermissions
            // here to request the missing permissions, and then overriding
            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
            //                                          int[] grantResults)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.
            Log.i("GatewayFragment", "ACCESS_FINE_LOCATION 没有")
            return
        }


        espDevice = ESPProvisionManager.getInstance(requireContext()).createESPDevice(
            ESPConstants.TransportType.TRANSPORT_SOFTAP,
            ESPConstants.SecurityType.SECURITY_1
        )

        val wusername = "jinyu333"
        val wpassword = "adskFZ!<3g"

        espDevice.setProofOfPossession(wpassword)
        espDevice.setUserName(wusername)
        espDevice.connectWiFiDevice()

        Handler(Looper.getMainLooper()).postDelayed({
            try {
    //            Toast.makeText(requireContext(), "设备已连接", Toast.LENGTH_SHORT).show()
    //            espDevice.scanNetworks(null);

                if (ssid.isNotEmpty() && password.isNotEmpty()) {
                    Log.i("GatewayFragment", "数据输入正常")
                }else{
                    Log.i("GatewayFragment", "数据输入异常")
                }
                Log.i("GatewayFragment", "SSID: $ssid, Password: $password")


                espDevice.provision(ssid, password, object : ProvisionListener {
                    override fun createSessionFailed(e: Exception) {
                        activity?.runOnUiThread {
                            Toast.makeText(context, "创建会话失败: ${e.message}", Toast.LENGTH_SHORT).show()
                        }
                        Log.e("GatewayFragment", "创建会话失败: ${e.message}", e)
                    }

                    override fun wifiConfigSent() {
                        activity?.runOnUiThread {
                            Toast.makeText(context, "WiFi 配置已发送", Toast.LENGTH_SHORT).show()
                        }
                        Log.i("GatewayFragment", "WiFi 配置已发送")
                    }

                    override fun wifiConfigFailed(e: Exception) {
                        activity?.runOnUiThread {
                            Toast.makeText(context, "WiFi 配置失败: ${e.message}", Toast.LENGTH_SHORT).show()
                        }
                        Log.e("GatewayFragment", "WiFi 配置失败: ${e.message}", e)
                    }

                    override fun wifiConfigApplied() {
                        activity?.runOnUiThread {
                            Toast.makeText(context, "WiFi 配置已应用，请稍等5秒", Toast.LENGTH_SHORT).show()
                        }
                        Log.i("GatewayFragment", "WiFi 配置已应用")
//                        saveInput("prov")
                    }

                    override fun wifiConfigApplyFailed(e: Exception) {
                        activity?.runOnUiThread {
                            Toast.makeText(context, "WiFi 配置应用失败: ${e.message}", Toast.LENGTH_SHORT).show()
                        }
                        Log.e("GatewayFragment", "WiFi 配置应用失败: ${e.message}", e)
//                        saveInput("")
                    }

                    override fun provisioningFailedFromDevice(reason: ESPConstants.ProvisionFailureReason) {
                        activity?.runOnUiThread {
                            Toast.makeText(context, "设备端配网失败: ${reason.name}", Toast.LENGTH_SHORT).show()
                        }

//                        val selectedWifiName = etWifiName.text.toString()
//
//                        // 判断选中的字符串是否包含 "5G"（不区分大小写）
//                        if (selectedWifiName.lowercase().contains("5g")) {
//                            // 如果包含 "5G"（不区分大小写），执行相关操作
//                            Toast.makeText(requireContext(), "网络似乎是5G的wifi，请确认连接2.4GWIFI", Toast.LENGTH_SHORT).show()
//                        } else {
//                            // 如果不包含 "5G"
////                            Toast.makeText(requireContext(), "不包含 5G", Toast.LENGTH_SHORT).show()
//                        }

                        Log.e("GatewayFragment", "设备端配网失败: ${reason.name}")
                    }

                    override fun deviceProvisioningSuccess() {
                        activity?.runOnUiThread {
                            Toast.makeText(context, "设备配网成功", Toast.LENGTH_SHORT).show()
                        }
                        Log.i("GatewayFragment", "设备配网成功")
                    }

                    override fun onProvisioningFailed(e: Exception) {
                        activity?.runOnUiThread {
                            Toast.makeText(context, "配网失败: ${e.message}", Toast.LENGTH_SHORT).show()
                            val selectedWifiName = etWifiName.text.toString()

                            // 判断选中的字符串是否包含 "5G"（不区分大小写）
                            if (selectedWifiName.lowercase().contains("5g")) {
                                // 如果包含 "5G"（不区分大小写），执行相关操作
                                Toast.makeText(requireContext(), "网络似乎是5G的wifi，请确认连接2.4G的WIFI", Toast.LENGTH_SHORT).show()
                            } else {
                                // 如果不包含 "5G"
//                            Toast.makeText(requireContext(), "不包含 5G", Toast.LENGTH_SHORT).show()
                            }
                        }

                        Log.e("GatewayFragment", "配网失败: ${e.message}", e)
                    }
                })
            } catch (e: Exception) {
                activity?.runOnUiThread {
                    Toast.makeText(requireContext(), "设备连接失败: ${e.message}", Toast.LENGTH_SHORT).show()
                }
                Log.e("GatewayFragment", "设备连接失败: ${e.message}", e)
//                saveInput("")
            }
        }, 3000)



    }




    override fun onDestroyView() {
        super.onDestroyView()

        delayedTaskscan?.let {
            handler.removeCallbacks(it)
        }
        delayedTaskscan = null

        delayedTaskconnect?.let {
            handler.removeCallbacks(it)
        }
        delayedTaskconnect = null

        _binding = null
    }


}