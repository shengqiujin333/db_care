package com.jinyuni.dengbei_care.telemetry

/**
 * 传感器采样节拍（readme 修改点 3：采集时间改为 3 分钟）。
 *
 * 3 分钟是**传感器**的采样周期；它不等于每 3 分钟上报一次——上报由传感器侧条件门控
 * （温度较上次下降 >0.9℃ 且无光，或温度 >35.0℃）决定（IC-002 §2/§6）。
 * App 侧用途：BLE 读取轮询周期与之对齐（不再系统性落后一个采样周期），以及界面文案；
 * 任何情况下都**不得**据此合成缺失样本或按固定节奏解释数据（IC-002 §5）。
 */
object SensorCadence {

    /** 采样周期（秒） */
    const val SAMPLE_PERIOD_SECONDS: Long = 180L

    /** 采样周期（毫秒） */
    const val SAMPLE_PERIOD_MS: Long = SAMPLE_PERIOD_SECONDS * 1000L
}
