package com.jinyuni.dengbei_care.ui.gateway

import android.Manifest
import android.app.AlertDialog
import android.bluetooth.BluetoothManager
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.location.LocationManager
import android.os.Build
import android.text.InputType
import android.util.Log
import android.view.Gravity
import android.view.ViewGroup
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import com.jinyuni.dengbei_care.MqtttService
import com.jinyuni.dengbei_care.ble.BlePermissionPolicy
import com.jinyuni.dengbei_care.ble.GatewayDiscovery
import com.jinyuni.dengbei_care.ble.PermissionNeed
import com.jinyuni.dengbei_care.cloud.PrefsUploadGate
import com.jinyuni.dengbei_care.protocol.GatewayIdentity

/**
 * 网关 BLE 身份与上传状态设置（AA-005 G-2 / D-17 / D-29；G-6 / D-27）。
 *
 * 用对话框实现（既有通知页布局是固定约束，不重构整页）：
 * - 显示当前 `gateway_ble_addr`（使用迁移默认值时标注“未确认”）与 `mac_addr`（平台身份，只读）；
 * - 有界扫描（10 s，FFE0 过滤优先，可显式“扩大搜索”），候选显示名称/MAC/RSSI；
 * - 允许手动输入/保存/清除；清除后写“已清除”标记，不回退默认；
 * - 显示上传阻塞原因/HTTP 状态/待发条数，提供“重试上传”。
 */
class GatewayBleSettingsDialog(
    private val fragment: Fragment,
    private val requestPermissions: (Array<String>) -> Unit
) {

    companion object {
        private const val TAG = "GatewayBleSettings"
    }

    private var dialog: AlertDialog? = null
    private var discovery: GatewayDiscovery? = null
    private var pendingScan: (() -> Unit)? = null

    private lateinit var statusView: TextView
    private lateinit var manualInput: EditText
    private lateinit var candidateBox: LinearLayout
    private lateinit var uploadView: TextView

    private val prefs get() = fragment.requireContext().getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)

    fun show() {
        val ctx = fragment.requireContext()
        val root = LinearLayout(ctx).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(48, 24, 48, 24)
        }

        statusView = TextView(ctx).apply {
            textSize = 14f
            setPadding(0, 0, 0, 16)
        }
        manualInput = EditText(ctx).apply {
            hint = "网关 BLE 地址（AA:BB:CC:DD:EE:FF）"
            inputType = InputType.TYPE_CLASS_TEXT
            setSingleLine(true)
        }
        manualInput.setText(
            prefs.getString(GatewayIdentity.PREF_GATEWAY_BLE_ADDR, "")
        )

        candidateBox = LinearLayout(ctx).apply { orientation = LinearLayout.VERTICAL }

        val scanButton = Button(ctx).apply { text = "搜索附近网关" }
        val expandButton = Button(ctx).apply { text = "扩大搜索（不过滤）" }
        val saveButton = Button(ctx).apply { text = "保存网关地址" }
        val clearButton = Button(ctx).apply { text = "清除网关地址" }

        uploadView = TextView(ctx).apply {
            textSize = 13f
            setPadding(0, 24, 0, 8)
        }
        val retryUploadButton = Button(ctx).apply { text = "重试上传" }

        root.addView(statusView)
        root.addView(manualInput)
        val scanRow = LinearLayout(ctx).apply {
            orientation = LinearLayout.HORIZONTAL
            addView(scanButton, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
            addView(expandButton, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
            addView(clearButton, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        }
        root.addView(scanRow)
        root.addView(candidateBox)
        root.addView(saveButton)
        root.addView(uploadView)
        root.addView(retryUploadButton)

        dialog = AlertDialog.Builder(ctx)
            .setTitle("网关与上传设置")
            .setView(ScrollView(ctx).apply { addView(root) })
            .setNegativeButton("关闭", null)
            .setOnDismissListener { stopScan() }
            .create()
        dialog?.show()

        scanButton.setOnClickListener { ensureScanPermission { startScan(filterByFfe0 = true) } }
        expandButton.setOnClickListener { ensureScanPermission { startScan(filterByFfe0 = false) } }
        saveButton.setOnClickListener { saveManual() }
        clearButton.setOnClickListener { clearGateway() }
        retryUploadButton.setOnClickListener { retryUpload() }

        refresh()
    }

    fun onPermissionResult(@Suppress("UNUSED_PARAMETER") result: Map<String, Boolean>) {
        val action = pendingScan
        pendingScan = null
        if (action == null) return
        val policy = BlePermissionPolicy.evaluate(
            sdkInt = Build.VERSION.SDK_INT,
            granted = grantedNeeds(),
            needScan = true,
            adapterEnabled = adapterEnabled(),
            locationEnabled = locationEnabled()
        )
        if (policy.canScan) {
            action()
        } else {
            // 区分权限缺失/定位关闭/蓝牙关闭，不使用笼统的“未授予扫描权限”
            val message = policy.userMessage.ifBlank { "无法开始扫描" }
            statusView.text = message
            toast(message)
        }
    }

    fun stopScan() {
        // 对话框关闭/页面销毁时挂起的扫描动作一并失效，避免权限结果迟到后启动无界面扫描
        pendingScan = null
        discovery?.stop("扫描已停止")
        discovery = null
    }

    /** 页面销毁/切换时关闭对话框，避免窗口泄漏 */
    fun dismiss() {
        stopScan()
        dialog?.dismiss()
        dialog = null
    }

    private fun refresh() {
        if (dialog?.isShowing != true) return
        val state = GatewayIdentity.bindingState(
            prefs.getString(GatewayIdentity.PREF_GATEWAY_BLE_ADDR, ""),
            prefs.getBoolean(GatewayIdentity.PREF_GATEWAY_BLE_ADDR_CLEARED, false)
        )
        val platformMac = prefs.getString(PrefsUploadGate.KEY_MAC_ADDR, "").orEmpty()
        val stored = prefs.getString(GatewayIdentity.PREF_GATEWAY_BLE_ADDR, "").orEmpty()
        statusView.text = when (state) {
            GatewayIdentity.BindingState.BOUND ->
                "BLE 连接目标：$stored\n平台身份（mac_addr）：$platformMac"
            GatewayIdentity.BindingState.DEFAULT_UNCONFIRMED ->
                "正在使用默认网关地址（未确认）：${GatewayIdentity.DEFAULT_GATEWAY_BLE_ADDR}\n" +
                        "平台身份（mac_addr）：$platformMac"
            GatewayIdentity.BindingState.UNSET ->
                "未配置网关地址\n平台身份（mac_addr）：$platformMac"
        }
        uploadView.text = uploadStatusText()
    }

    private fun uploadStatusText(): String {
        val pending = prefs.getInt(PrefsUploadGate.KEY_UPLOAD_PENDING_COUNT, 0)
        val reason = prefs.getString(PrefsUploadGate.KEY_UPLOAD_BLOCKED_REASON, "").orEmpty()
        val status = prefs.getString(PrefsUploadGate.KEY_UPLOAD_LAST_STATUS, "").orEmpty()
        val at = prefs.getLong(PrefsUploadGate.KEY_UPLOAD_BLOCKED_AT, 0L)
        val base = when {
            reason == PrefsUploadGate.KEY_REASON_AUTH ->
                "云端上传被拒绝（HTTP ${status.ifBlank { "401/403" }}），数据已保留（待发 $pending 条）"
            reason == PrefsUploadGate.KEY_REASON_REQUEST ->
                "云端上传请求被拒绝（HTTP ${status.ifBlank { "4xx" }}），数据已保留（待发 $pending 条）"
            status == PrefsUploadGate.KEY_STATUS_RETRYING ->
                "云端上传失败，重试中（待发 $pending 条）"
            status == PrefsUploadGate.KEY_STATUS_OK -> "云端上传正常（待发 $pending 条）"
            else -> "云端上传状态：无（待发 $pending 条）"
        }
        return if (at > 0) "$base\n最近被拒时间：$at" else base
    }

    private fun saveManual() {
        val normalized = GatewayIdentity.normalize(manualInput.text.toString())
        if (normalized == null) {
            toast("地址格式不正确，应为 6 组十六进制（AA:BB:CC:DD:EE:FF）")
            return
        }
        prefs.edit()
            .putString(GatewayIdentity.PREF_GATEWAY_BLE_ADDR, normalized)
            .putBoolean(GatewayIdentity.PREF_GATEWAY_BLE_ADDR_CLEARED, false)
            .apply()
        manualInput.setText(normalized)
        Log.i(TAG, "gateway_ble_addr saved=$normalized (platform mac_addr untouched)")
        notifyService("GATEWAY_CHANGED")
        refresh()
        toast("已保存网关地址")
    }

    private fun clearGateway() {
        prefs.edit()
            .putString(GatewayIdentity.PREF_GATEWAY_BLE_ADDR, "")
            .putBoolean(GatewayIdentity.PREF_GATEWAY_BLE_ADDR_CLEARED, true)
            .apply()
        manualInput.setText("")
        notifyService("GATEWAY_CHANGED")
        refresh()
        toast("已清除网关地址（不会回退默认值）")
    }

    private fun retryUpload() {
        notifyService("RETRY_UPLOAD")
        toast("已触发上传重试")
        fragment.view?.postDelayed({ refresh() }, 800)
    }

    private fun notifyService(action: String) {
        try {
            val ctx = fragment.requireContext().applicationContext
            ctx.startService(Intent(ctx, MqtttService::class.java).apply { this.action = action })
        } catch (e: Exception) {
            Log.w(TAG, "notify service failed: ${e.message}")
        }
    }

    private fun ensureScanPermission(action: () -> Unit) {
        if (hasScanPermission()) {
            action()
            return
        }
        val needed = BlePermissionPolicy.scanRequestPermissions(
            Build.VERSION.SDK_INT,
            grantedNeeds()
        )
        if (needed.isEmpty()) {
            toast(BlePermissionPolicy.evaluate(
                sdkInt = Build.VERSION.SDK_INT,
                granted = grantedNeeds(),
                needScan = true,
                adapterEnabled = adapterEnabled(),
                locationEnabled = locationEnabled()
            ).userMessage.ifBlank { "无法开始扫描" })
            return
        }
        pendingScan = action
        requestPermissions(needed.map { platformPermission(it) }.toTypedArray())
    }

    private fun startScan(filterByFfe0: Boolean) {
        val ctx = fragment.requireContext()
        candidateBox.removeAllViews()
        discovery?.stop("重新开始扫描")
        discovery = GatewayDiscovery(
            context = ctx.applicationContext,
            onState = { text -> statusView.text = text },
            onCandidates = { candidates -> renderCandidates(candidates) },
            onFinished = { candidates ->
                if (candidates.isEmpty()) {
                    statusView.text = "未找到网关：可重试、扩大搜索（不过滤）或手动输入 MAC"
                } else {
                    statusView.text = "找到 ${candidates.size} 个候选：点击一项填入地址后保存"
                }
            }
        ).also { it.start(filterByFfe0) }
        statusView.text = "正在搜索…（最多 10 秒）"
    }

    private fun renderCandidates(candidates: List<GatewayIdentity.Candidate>) {
        val ctx = fragment.requireContext()
        candidateBox.removeAllViews()
        candidates.forEach { candidate ->
            val label = buildString {
                append(candidate.name ?: "（无名称）")
                append("  ").append(candidate.mac)
                append("  RSSI=").append(candidate.rssi)
                if (candidate.hasFfe0) append("  FFE0")
            }
            val row = TextView(ctx).apply {
                text = label
                textSize = 13f
                gravity = Gravity.START
                setPadding(8, 12, 8, 12)
                setOnClickListener { manualInput.setText(candidate.mac) }
            }
            candidateBox.addView(row)
        }
    }

    private fun hasScanPermission(): Boolean {
        val granted = grantedNeeds()
        val policy = BlePermissionPolicy.evaluate(
            sdkInt = Build.VERSION.SDK_INT,
            granted = granted,
            needScan = true,
            adapterEnabled = adapterEnabled(),
            locationEnabled = locationEnabled()
        )
        return policy.canScan
    }

    private fun grantedNeeds(): Set<PermissionNeed> {
        val ctx = fragment.requireContext()
        val granted = mutableSetOf<PermissionNeed>()
        fun has(p: String) =
            ContextCompat.checkSelfPermission(ctx, p) == PackageManager.PERMISSION_GRANTED
        if (has(Manifest.permission.BLUETOOTH_CONNECT)) granted += PermissionNeed.BLUETOOTH_CONNECT
        if (has(Manifest.permission.BLUETOOTH_SCAN)) granted += PermissionNeed.BLUETOOTH_SCAN
        if (has(Manifest.permission.ACCESS_FINE_LOCATION)) granted += PermissionNeed.FINE_LOCATION
        return granted
    }

    private fun platformPermission(need: PermissionNeed): String = when (need) {
        PermissionNeed.BLUETOOTH_CONNECT -> Manifest.permission.BLUETOOTH_CONNECT
        PermissionNeed.BLUETOOTH_SCAN -> Manifest.permission.BLUETOOTH_SCAN
        PermissionNeed.FINE_LOCATION -> Manifest.permission.ACCESS_FINE_LOCATION
        PermissionNeed.POST_NOTIFICATIONS -> Manifest.permission.POST_NOTIFICATIONS
    }

    private fun adapterEnabled(): Boolean = try {
        val manager = fragment.requireContext()
            .getSystemService(Context.BLUETOOTH_SERVICE) as? BluetoothManager
        manager?.adapter?.isEnabled == true
    } catch (e: Exception) {
        false
    }

    private fun locationEnabled(): Boolean = try {
        val lm = fragment.requireContext()
            .getSystemService(Context.LOCATION_SERVICE) as? LocationManager
        lm?.isLocationEnabled == true
    } catch (e: Exception) {
        false
    }

    private fun toast(message: String) {
        Toast.makeText(fragment.requireContext(), message, Toast.LENGTH_SHORT).show()
    }
}
