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
import com.jinyuni.dengbei_care.databinding.ActivityMainBinding
import com.jinyuni.dengbei_care.report.ReportScheduler
import com.jinyuni.dengbei_care.ui.zhuce.PrivacyPolicyActivity
import com.jinyuni.dengbei_care.ui.zhuce.TermsOfServiceActivity

class MainActivity : BaseActivity() {

    private lateinit var binding: ActivityMainBinding
    private val REQUEST_CODE_POST_NOTIFICATIONS = 1231
    private val REQUEST_CODE_LOCATION = 1232
    private val REQUEST_BLUETOOTH_PERMISSION = 1
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

//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
//            if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
//                ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.POST_NOTIFICATIONS), REQUEST_CODE_POST_NOTIFICATIONS)
//            }
//        }
//
//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
//            if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
//                != PackageManager.PERMISSION_GRANTED) {
//                ActivityCompat.requestPermissions(
//                    this,
//                    arrayOf(Manifest.permission.ACCESS_FINE_LOCATION),
//                    REQUEST_CODE_LOCATION
//                )
//            }
//        }

//        if (!hasVibratePermission()) {
//            requestVibratePermission()
//        }


        setupDefaultPreferences(this)
        startMqttService()
        scheduleDailyReport()

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
            Toast.makeText(this, "我们需要您授予通知权限和位置权限以便触发条件时能及时提醒", Toast.LENGTH_SHORT).show()
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.POST_NOTIFICATIONS), REQUEST_CODE_POST_NOTIFICATIONS)
                }
            }


//            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
//                if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
//                    != PackageManager.PERMISSION_GRANTED) {
//                    ActivityCompat.requestPermissions(
//                        this,
//                        arrayOf(Manifest.permission.ACCESS_FINE_LOCATION),
//                        REQUEST_CODE_LOCATION
//                    )
//                }
//            }
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

    private fun requestBluetoothPermissions() {
        // Request Bluetooth permissions for Android 12 and above
        ActivityCompat.requestPermissions(
            this,
            arrayOf(Manifest.permission.BLUETOOTH_CONNECT, Manifest.permission.BLUETOOTH_SCAN),
            REQUEST_BLUETOOTH_PERMISSION
        )
    }



    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        when (requestCode) {
            REQUEST_CODE_POST_NOTIFICATIONS -> {
                if (grantResults.isNotEmpty() &&
                    grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    // 通知权限被授予
                    Log.d("Permission", "Notification permission granted")

                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
                            != PackageManager.PERMISSION_GRANTED) {
                            ActivityCompat.requestPermissions(
                                this,
                                arrayOf(Manifest.permission.ACCESS_FINE_LOCATION),
                                REQUEST_CODE_LOCATION
                            )
                        }
                    }
                } else {
                    // 通知权限被拒绝
                    Log.d("Permission", "Notification permission denied")
                    if (!shouldShowRequestPermissionRationale(Manifest.permission.POST_NOTIFICATIONS)) {
                        Toast.makeText(this, "需要通知权限以提供及时的消息提醒", Toast.LENGTH_SHORT)
                            .show()
                    }
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                        if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.POST_NOTIFICATIONS), REQUEST_CODE_POST_NOTIFICATIONS)
                        }
                    }
                }
            }
            REQUEST_CODE_LOCATION -> {
                if (grantResults.isNotEmpty() &&
                    grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    // 位置权限被授予
                    Log.d("Permission", "Location permission granted")
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                        // For Android 12 (API level 31) and above
                        if (ContextCompat.checkSelfPermission(
                                this, Manifest.permission.BLUETOOTH_CONNECT
                            ) != PackageManager.PERMISSION_GRANTED ||
                            ContextCompat.checkSelfPermission(
                                this, Manifest.permission.BLUETOOTH_SCAN
                            ) != PackageManager.PERMISSION_GRANTED
                        ) {
                            // Request Bluetooth permissions
                            requestBluetoothPermissions()
                        }
                    }


                } else {
                    // 位置权限被拒绝
                    Log.d("Permission", "Location permission denied")
                    if (!shouldShowRequestPermissionRationale(Manifest.permission.ACCESS_FINE_LOCATION)) {
                        Toast.makeText(this, "需要位置相关服务才能及时获取服务器消息", Toast.LENGTH_SHORT)
                            .show()
                    }
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
                            != PackageManager.PERMISSION_GRANTED) {
                            ActivityCompat.requestPermissions(
                                this,
                                arrayOf(Manifest.permission.ACCESS_FINE_LOCATION),
                                REQUEST_CODE_LOCATION
                            )
                        }
                    }
                }
            }
            REQUEST_BLUETOOTH_PERMISSION -> {
                if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    // Bluetooth permissions granted
                    Log.d("Permission", "Bluetooth permissions granted")

                } else {
                    if (!shouldShowRequestPermissionRationale(Manifest.permission.ACCESS_FINE_LOCATION)) {
                        Toast.makeText(this, "需要蓝牙相关权限才能及时获取传感器数据", Toast.LENGTH_SHORT)
                            .show()
                    }
                    // Bluetooth permissions denied
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                        // For Android 12 (API level 31) and above
                        if (ContextCompat.checkSelfPermission(
                                this, Manifest.permission.BLUETOOTH_CONNECT
                            ) != PackageManager.PERMISSION_GRANTED ||
                            ContextCompat.checkSelfPermission(
                                this, Manifest.permission.BLUETOOTH_SCAN
                            ) != PackageManager.PERMISSION_GRANTED
                        ) {
                            // Request Bluetooth permissions
                            requestBluetoothPermissions()
                        }
                    }
                }
            }




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