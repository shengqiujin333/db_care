package com.jinyuni.dengbei_care

import com.github.mikephil.charting.data.Entry

// 设备报警严重级别(用于卡片状态 chip 颜色)
enum class Severity {
    NORMAL,    // 正常(无报警)
    WARNING,   // 警告(温湿度下降/超限)
    ALARM      // 紧急报警(温度>65°C 火灾预警)
}

// 单设备在 UI 上的完整状态(给 HomeFragment 卡片墙用)
// 设计为不可变 data class,更新时用 copy() 创建新实例(LiveData 线程安全)
data class DeviceState(
    val devId: String,              // 8 位 HEX(与 MacIdBook 一致,大写无分隔)
    val name: String,               // 用户给的别称,空时 UI 显示 devId
    val latestTemp: Double,          // 最新温度 °C
    val latestHumi: Double,          // 最新湿度 %RH
    val latestTime: Long,            // 最新数据时间戳(Unix 秒),0 表示从未收到
    val alarmMessage: String,        // 当前报警文案,空串表示无报警
    val alarmSeverity: Severity,     // 当前报警级别
    val sparkline: List<Entry>       // 最近 N 条温度采样(给卡片迷你图用)
) {
    companion object {
        // 数据过期阈值:30 分钟没收到新数据视为离线
        const val OFFLINE_THRESHOLD_MS = 30L * 60 * 1000

        // 创建一个空白设备占位
        fun empty(devId: String, name: String = ""): DeviceState = DeviceState(
            devId = devId,
            name = name,
            latestTemp = 0.0,
            latestHumi = 0.0,
            latestTime = 0L,
            alarmMessage = "",
            alarmSeverity = Severity.NORMAL,
            sparkline = emptyList()
        )
    }

    // 是否在线(最近 30 分钟有数据)
    fun isOnline(now: Long = System.currentTimeMillis()): Boolean {
        if (latestTime == 0L) return false
        val ageMs = now - latestTime * 1000
        return ageMs < OFFLINE_THRESHOLD_MS
    }

    // 显示名:有别名用别名,否则用 devId
    fun displayName(): String = if (name.isNotBlank()) name else devId
}
