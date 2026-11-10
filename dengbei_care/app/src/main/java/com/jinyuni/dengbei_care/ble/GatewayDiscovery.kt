package com.jinyuni.dengbei_care.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.os.ParcelUuid
import android.util.Log
import com.jinyuni.dengbei_care.protocol.GatewayIdentity
import java.util.UUID

/**
 * 有界 BLE 网关发现（AA-005 G-2 / D-29）。
 *
 * - 10 s 上限，超时/取消/页面销毁/权限撤销/异常都必须 stopScan 并释放回调；
 * - 优先按 16-bit 服务 UUID FFE0 过滤；用户显式选择“扩大搜索”时才执行无过滤扫描；
 * - 结果去重排序交给 [GatewayIdentity.rank]；
 * - 不自动连接任何候选，保存动作由用户在界面确认。
 *
 * 权限由调用方按 [BlePermissionPolicy] 判定后再启动；本类只做扫描并捕获 SecurityException。
 */
class GatewayDiscovery(
    context: Context,
    private val onState: (String) -> Unit,
    private val onCandidates: (List<GatewayIdentity.Candidate>) -> Unit,
    private val onFinished: (List<GatewayIdentity.Candidate>) -> Unit
) {

    companion object {
        const val SCAN_TIMEOUT_MS = 10_000L
        val FFE0_SERVICE: UUID = UUID.fromString("0000ffe0-0000-1000-8000-00805f9b34fb")
        private const val TAG = "GatewayDiscovery"
    }

    private val appContext = context.applicationContext
    private val handler = Handler(Looper.getMainLooper())
    private val results = LinkedHashMap<String, GatewayIdentity.Candidate>()

    @Volatile
    private var scanning = false
    private var finished = false

    private val timeout = Runnable { finish("扫描超时（10 秒）") }

    private val callback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            // 停止/超时/异常后不再交付候选（保证“停止后无新回调”）
            if (!scanning) return
            val device = result.device ?: return
            val hasFfe0 = result.scanRecord?.serviceUuids?.any {
                it.uuid == FFE0_SERVICE
            } == true
            val name = try {
                result.scanRecord?.deviceName ?: device.name
            } catch (_: SecurityException) {
                null
            }
            val mac = GatewayIdentity.normalize(device.address) ?: return
            results[mac] = GatewayIdentity.Candidate(
                mac = mac,
                name = name,
                rssi = result.rssi,
                hasFfe0 = hasFfe0
            )
            onCandidates(GatewayIdentity.rank(results.values.toList()))
        }

        override fun onBatchScanResults(results: MutableList<ScanResult>?) {
            results?.forEach { onScanResult(ScanSettings.CALLBACK_TYPE_ALL_MATCHES, it) }
        }

        override fun onScanFailed(errorCode: Int) {
            Log.w(TAG, "scan failed errorCode=$errorCode")
            finish("扫描失败（errorCode=$errorCode）")
        }
    }

    val isScanning: Boolean get() = scanning

    /**
     * 启动一次有界扫描。
     * @param filterByFfe0 true=仅 FFE0（默认）；false=用户显式“扩大搜索（不过滤）”
     */
    @SuppressLint("MissingPermission")
    fun start(filterByFfe0: Boolean = true) {
        if (scanning) stopInternal()
        results.clear()
        finished = false

        val adapter = (appContext.getSystemService(Context.BLUETOOTH_SERVICE) as? BluetoothManager)?.adapter
        if (adapter == null || !adapter.isEnabled) {
            onState("蓝牙未开启（去开启）")
            onFinished(emptyList())
            return
        }
        val scanner = adapter.bluetoothLeScanner
        if (scanner == null) {
            onState("蓝牙扫描不可用")
            onFinished(emptyList())
            return
        }

        try {
            val settings = ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .build()
            val filters = if (filterByFfe0) {
                listOf(
                    ScanFilter.Builder()
                        .setServiceUuid(ParcelUuid(FFE0_SERVICE))
                        .build()
                )
            } else {
                emptyList()
            }
            scanner.startScan(filters, settings, callback)
            scanning = true
            onState(if (filterByFfe0) "正在搜索附近网关（按 FFE0 过滤）…" else "正在扩大搜索（不过滤）…")
            handler.postDelayed(timeout, SCAN_TIMEOUT_MS)
            Log.i(TAG, "scan started filterByFfe0=$filterByFfe0 timeoutMs=$SCAN_TIMEOUT_MS")
        } catch (e: SecurityException) {
            Log.w(TAG, "scan denied: ${e.message}")
            onState("搜索网关缺少权限（去授权）")
            onFinished(emptyList())
        } catch (e: Exception) {
            Log.w(TAG, "scan error: ${e.message}", e)
            onState("搜索网关失败（重试）")
            onFinished(emptyList())
        }
    }

    /** 用户取消 / 页面销毁 / 蓝牙关闭 / 权限撤销时调用；可重复调用 */
    fun stop(reason: String = "扫描已取消") {
        if (scanning) {
            finish(reason)
        } else {
            release()
        }
    }

    private fun finish(state: String) {
        if (finished) return
        finished = true
        onState(state)
        val ranked = GatewayIdentity.rank(results.values.toList())
        stopInternal()
        onCandidates(ranked)
        onFinished(ranked)
    }

    @SuppressLint("MissingPermission")
    private fun stopInternal() {
        handler.removeCallbacks(timeout)
        if (!scanning) return
        scanning = false
        try {
            val adapter = (appContext.getSystemService(Context.BLUETOOTH_SERVICE) as? BluetoothManager)?.adapter
            adapter?.bluetoothLeScanner?.stopScan(callback)
            Log.i(TAG, "scan stopped")
        } catch (e: Exception) {
            Log.w(TAG, "stopScan failed: ${e.message}")
        }
    }

    private fun release() {
        handler.removeCallbacks(timeout)
        results.clear()
    }
}
