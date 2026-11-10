package com.jinyuni.dengbei_care

import android.Manifest
import android.app.AlertDialog
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.navigation.findNavController
import androidx.navigation.ui.setupWithNavController
import com.google.android.material.bottomnavigation.BottomNavigationView
import com.jinyuni.dengbei_care.ble.BlePermissionPolicy
import com.jinyuni.dengbei_care.ble.PermissionNeed
import com.jinyuni.dengbei_care.databinding.ActivityMainBinding
import com.jinyuni.dengbei_care.report.ReportScheduler
import com.jinyuni.dengbei_care.ui.zhuce.PrivacyPolicyActivity
import com.jinyuni.dengbei_care.ui.zhuce.TermsOfServiceActivity

class MainActivity : BaseActivity() {

    private lateinit var binding: ActivityMainBinding
    private val REQUEST_CODE_STARTUP_PERMISSIONS = 1231
    private var permissionRequestInFlight = false
    private var denialToastShown = false
    private lateinit var dialog: AlertDialog // 声明对话框对象
    private val REQUEST_CODE_SCHEDULE_EXACT_ALARM = 1233

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // 检查用户是否已经同意隐私政策
        if (!isPrivacyPolicyAccepted()) {
            showPrivacyPolicyDialog()
        }

//        val toolbar = findViewById<androidx.appcompat.widget.Toolbar>(R.id.toolbar)
//        setSupportActionBar(toolbar)
        supportActionBar?.hide()

        val navView: BottomNavigationView = binding.navView

        val navController = findNavController(R.id.nav_host_fragment_activity_main)
        // Passing each menu ID as a set of Ids because each
        // menu should be considered as top level destinations.
//        val appBarConfiguration = AppBarConfiguration(
//            setOf(
//                R.id.navigation_home, R.id.navigation_dashboard, R.id.navigation_notifications
//            )
//        )
//        setupActionBarWithNavController(navController, appBarConfiguration)
        navView.setupWithNavController(navController)

        setupDefaultPreferences(this)
        startMqttService()
        scheduleDailyReport()
        if (isPrivacyPolicyAccepted()) {
            requestStartupPermissions()
        }

    }

    private fun scheduleDailyReport() {
        val prefs = getSharedPreferences("MyPrefs", MODE_PRIVATE)
        val time = prefs.getString("report_time", "07:00") ?: "07:00"
        ReportScheduler.scheduleDaily(this, time)
    }





    private fun isPrivacyPolicyAccepted(): Boolean {
        // 使用 SharedPreferences 存储用户的选择
        val sharedPreferences = getSharedPreferences("MyPrefs", MODE_PRIVATE)
        return sharedPreferences.getBoolean("privacy_policy_accepted", false)
    }

    private fun showPrivacyPolicyDialog() {
        // 使用自定义布局创建对话框
        val dialogView = LayoutInflater.from(this).inflate(R.layout.dialog_privacy_policy, null)

        // 获取两个 TextView
        val userAgreementTextView = dialogView.findViewById<TextView>(R.id.userAgreementTextView)
        val privacyPolicyTextView = dialogView.findViewById<TextView>(R.id.privacyPolicyTextView)

        // 设置《用户服务协议》的点击事件
        userAgreementTextView.setOnClickListener {
            val intent = Intent(this, TermsOfServiceActivity::class.java)
            startActivity(intent)
        }

        // 设置《隐私政策》的点击事件
        privacyPolicyTextView.setOnClickListener {
            val intent = Intent(this, PrivacyPolicyActivity::class.java)
            startActivity(intent)
        }

        // 获取两个按钮
        val disagreeButton = dialogView.findViewById<Button>(R.id.disagreeButton)
        val agreeButton = dialogView.findViewById<Button>(R.id.agreeButton)
        // 设置“不同意”按钮的点击事件
        disagreeButton.setOnClickListener {
            // 用户选择不同意，提示并退出应用
            Toast.makeText(this, "您需要同意隐私政策才能继续使用应用", Toast.LENGTH_SHORT).show()
            finish() // 关闭应用
        }

        // 设置“同意并继续”按钮的点击事件
        agreeButton.setOnClickListener {
            // 用户选择同意，保存选择并继续
            savePrivacyPolicyAccepted()
            dialog.dismiss() // 关闭对话框
            Toast.makeText(this, "我们需要您授予通知与蓝牙权限以便及时提醒", Toast.LENGTH_SHORT).show()
            requestStartupPermissions()
        }

        // 创建对话框
        val builder = AlertDialog.Builder(this)
        builder.setView(dialogView)
        builder.setCancelable(false) // 防止用户通过点击外部区域关闭对话框

        // 显示对话框
        dialog = builder.create()
        dialog.show()
    }


    fun setupDefaultPreferences(context: Context) {
        val sharedPreferences = context.getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)

        // 检查是否已经设置过默认参数
        if (!sharedPreferences.contains("isFirstRun")) {
            // 设置默认参数
            sharedPreferences.edit().apply {
                putString("tempdrop", "2")
                putString("humdrop", "0.1")
                putString("tempmax", "35")
                putString("tempmin", "29")
                putString("hummax", "75")
                putString("hummin", "40")
                putString("guardtime", "8")
                putString("delayalarm", "10")
                putString("dengbeialarm", "1")
                putString("yuzhialarm", "1")
                putString("zhendongalarm", "1")
                putString("xianglingalarm", "1")
                putString("report_time", "07:00")

                // 设置标志位，表示已经设置过默认参数
                putBoolean("isFirstRun", true)

                apply()
            }
        }
    }

    private fun savePrivacyPolicyAccepted() {
        // 使用 SharedPreferences 保存用户的选择
        val sharedPreferences = getSharedPreferences("MyPrefs", MODE_PRIVATE)
        val editor = sharedPreferences.edit()
        editor.putBoolean("privacy_policy_accepted", true)
        editor.apply()
    }


    private fun startMqttService() {
        val serviceIntent = Intent(this, MqtttService::class.java)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(serviceIntent)
        } else {
            startService(serviceIntent)
        }
    }

    // ---------- 运行时权限（AA-005 G-1 / D-16） ----------
    // 只请求当前 API 真正需要的、且清单已声明的权限：
    // - API>=31：BLUETOOTH_CONNECT（直连）；扫描场景另由网关设置页请求 BLUETOOTH_SCAN；
    // - API>=33：POST_NOTIFICATIONS；
    // - API<31：直连不需要定位；仅扫描流程请求 ACCESS_FINE_LOCATION。
    // 拒绝或“不再询问”只提示一次，不重复请求、不回退到定位权限。

    private fun grantedNeeds(): Set<PermissionNeed> {
        val granted = mutableSetOf<PermissionNeed>()
        fun has(p: String) = ContextCompat.checkSelfPermission(this, p) == PackageManager.PERMISSION_GRANTED
        if (has(Manifest.permission.BLUETOOTH_CONNECT)) granted += PermissionNeed.BLUETOOTH_CONNECT
        if (has(Manifest.permission.BLUETOOTH_SCAN)) granted += PermissionNeed.BLUETOOTH_SCAN
        if (has(Manifest.permission.ACCESS_FINE_LOCATION)) granted += PermissionNeed.FINE_LOCATION
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU ||
            has(Manifest.permission.POST_NOTIFICATIONS)
        ) {
            granted += PermissionNeed.POST_NOTIFICATIONS
        }
        return granted
    }

    private fun platformPermission(need: PermissionNeed): String = when (need) {
        PermissionNeed.BLUETOOTH_CONNECT -> Manifest.permission.BLUETOOTH_CONNECT
        PermissionNeed.BLUETOOTH_SCAN -> Manifest.permission.BLUETOOTH_SCAN
        PermissionNeed.FINE_LOCATION -> Manifest.permission.ACCESS_FINE_LOCATION
        PermissionNeed.POST_NOTIFICATIONS -> Manifest.permission.POST_NOTIFICATIONS
    }

    /** 启动/开启守护时调用：请求连接与通知所需权限，不请求定位 */
    fun requestStartupPermissions() {
        if (permissionRequestInFlight) return
        val granted = grantedNeeds()
        val request = mutableListOf<String>()
        if (BlePermissionPolicy.needsNotificationPermission(Build.VERSION.SDK_INT, granted)) {
            request += platformPermission(PermissionNeed.POST_NOTIFICATIONS)
        }
        BlePermissionPolicy.connectRequestPermissions(Build.VERSION.SDK_INT, granted)
            .forEach { request += platformPermission(it) }
        if (request.isEmpty()) return

        permissionRequestInFlight = true
        Log.i(
            "Permission",
            "request startup permissions: ${request.distinct()} sdkInt=${Build.VERSION.SDK_INT}"
        )
        ActivityCompat.requestPermissions(
            this,
            request.distinct().toTypedArray(),
            REQUEST_CODE_STARTUP_PERMISSIONS
        )
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != REQUEST_CODE_STARTUP_PERMISSIONS) return

        permissionRequestInFlight = false
        val denied = permissions.filterIndexed { index, _ ->
            grantResults.getOrNull(index) != PackageManager.PERMISSION_GRANTED
        }
        Log.i(
            "Permission",
            "startup permissions result: requested=${permissions.toList()} denied=$denied " +
                    "sdkInt=${Build.VERSION.SDK_INT}"
        )
        if (denied.isNotEmpty() && !denialToastShown) {
            denialToastShown = true
            Toast.makeText(
                this,
                "权限被拒绝：将无法读取网关数据。可到系统设置中授予附近设备/通知权限",
                Toast.LENGTH_LONG
            ).show()
        }
    }

//    private fun hasVibratePermission(): Boolean {
//        return ContextCompat.checkSelfPermission(this, Manifest.permission.VIBRATE) == PackageManager.PERMISSION_GRANTED
//    }
//
//    private fun requestVibratePermission() {
//        ActivityCompat.requestPermissions(
//            this,
//            arrayOf(Manifest.permission.VIBRATE),
//            REQUEST_CODE_VIBRATE
//        )
//    }
//
//    companion object {
//        const val REQUEST_CODE_VIBRATE = 1
//    }


//    override fun onSupportNavigateUp(): Boolean {
//        val navController = findNavController(R.id.nav_host_fragment_activity_main)
//        return navController.navigateUp() || super.onSupportNavigateUp()
//    }
}