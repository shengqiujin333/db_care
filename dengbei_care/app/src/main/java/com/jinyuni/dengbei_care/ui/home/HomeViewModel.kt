package com.jinyuni.dengbei_care.ui.home

import android.app.Application
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.github.mikephil.charting.data.Entry
import com.jinyuni.dengbei_care.DeviceState
import com.jinyuni.dengbei_care.Severity

class HomeViewModel private constructor(): ViewModel() {

    companion object {
        private var instance: HomeViewModel? = null

        fun getInstance(application: Application): HomeViewModel {
            if (instance == null) {
                instance = ViewModelProvider.AndroidViewModelFactory.getInstance(application).create(HomeViewModel::class.java)
            }
            return instance!!
        }

        // 卡片迷你 sparkline 最多保留的采样数(约 2.5 小时 @ 5 分钟间隔)
        private const val MAX_SPARKLINE_SIZE = 30
    }

    // ===== 新多设备 API =====
    // _devices 是设备 ID -> 状态的 Map,卡片墙 observe 这个
    val _devices = MutableLiveData<Map<String, DeviceState>>(emptyMap())
    val devices: LiveData<Map<String, DeviceState>> = _devices

    /**
     * 更新某设备的读数。如果该设备不在 Map 里,会自动创建空白状态再填值。
     */
    fun updateDeviceReading(devId: String, name: String, temp: Double, humi: Double, time: Long) {
        val map = (_devices.value?.toMutableMap() ?: mutableMapOf())
        val current = map[devId] ?: DeviceState.empty(devId, name)
        val newSparkline = (current.sparkline + Entry((time * 1000).toFloat(), temp.toFloat()))
            .takeLast(MAX_SPARKLINE_SIZE)
        val updated = current.copy(
            devId = devId,
            name = if (name.isNotBlank()) name else current.name,
            latestTemp = temp,
            latestHumi = humi,
            latestTime = time,
            sparkline = newSparkline
        )
        map[devId] = updated
        _devices.postValue(map)
    }

    /**
     * 更新某设备的报警状态。
     */
    fun updateDeviceAlarm(devId: String, message: String, severity: Severity) {
        val map = (_devices.value?.toMutableMap() ?: mutableMapOf())
        val current = map[devId] ?: DeviceState.empty(devId)
        map[devId] = current.copy(alarmMessage = message, alarmSeverity = severity)
        _devices.postValue(map)
    }

    /**
     * 清空某设备的 sparkline(老逻辑用 _drawFlag 触发清屏,新逻辑按设备清)。
     */
    fun clearDevice(devId: String) {
        val map = _devices.value?.toMutableMap() ?: return
        val current = map[devId] ?: return
        map[devId] = current.copy(sparkline = emptyList())
        _devices.postValue(map)
    }

    /**
     * 清空所有设备的 sparkline(替代老逻辑里 postValue(emptyList()) 的清屏行为)。
     */
    fun clearAllDevices() {
        val map = _devices.value?.toMutableMap() ?: return
        map.forEach { (id, state) ->
            map[id] = state.copy(sparkline = emptyList())
        }
        _devices.postValue(map)
    }

    /**
     * 清掉所有设备的报警状态(BaseActivity 触屏时调用)。
     */
    fun clearAllDeviceAlarms() {
        val map = _devices.value?.toMutableMap() ?: return
        map.forEach { (id, state) ->
            map[id] = state.copy(alarmMessage = "", alarmSeverity = Severity.NORMAL)
        }
        _devices.postValue(map)
    }

    fun getDevice(devId: String): DeviceState? = _devices.value?.get(devId)

    /**
     * 确保某设备在 Map 里有占位状态(从 MacIdBook 加载列表时调用,让卡片提前出现)。
     */
    fun ensureDevice(devId: String, name: String) {
        val map = (_devices.value?.toMutableMap() ?: mutableMapOf())
        if (!map.containsKey(devId)) {
            map[devId] = DeviceState.empty(devId, name)
            _devices.postValue(map)
        }
    }

    /**
     * 移除某设备(从 MacIdBook 删除时调用)。
     */
    fun removeDevice(devId: String) {
        val map = _devices.value?.toMutableMap() ?: return
        if (map.remove(devId) != null) {
            _devices.postValue(map)
        }
    }

    // ===== 守护状态(全局,所有 Fragment 共享) =====

    val _startData = MutableLiveData<Int>()
    val startData: LiveData<Int> = _startData
}
