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
        /** 创建一个空白设备占位 */
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

    /** 是否从未收到过任何上报（卡片显示 `--`） */
    fun isNeverReported(): Boolean = latestTime == 0L

    /**
     * 最近上报的相对时间标签（卡片展示用）。
     *
     * 注意：**不用数据龄推断在线/离线**。传感器改为 3 分钟采样 + 条件上报后，长时间无新数据是
     * 正常产品行为（温度未下降、未超温就不上报，IC-002 §2/§6）；连通性由 MQTT/BLE 链路状态表达，
     * 不由此处推断。
     *
     * @param nowMillis 当前时间（毫秒），默认取系统时间
     * @return "刚刚" / "N 分钟前" / "N 小时前" / "N 天前"；从未上报时返回空串
     */
    fun lastReportAgeLabel(nowMillis: Long = System.currentTimeMillis()): String {
        if (isNeverReported()) return ""
        val ageSeconds = (nowMillis / 1000) - latestTime
        return when {
            ageSeconds < 60 -> "刚刚"
            ageSeconds < 60 * 60 -> "${ageSeconds / 60} 分钟前"
            ageSeconds < 24 * 60 * 60 -> "${ageSeconds / (60 * 60)} 小时前"
            else -> "${ageSeconds / (24 * 60 * 60)} 天前"
        }
    }

    // 显示名:有别名用别名,否则用 devId
    fun displayName(): String = if (name.isNotBlank()) name else devId
}
