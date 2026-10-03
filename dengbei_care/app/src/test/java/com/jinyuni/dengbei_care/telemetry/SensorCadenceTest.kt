package com.jinyuni.dengbei_care.telemetry

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * ITEM-010 实现侧自检：采样节拍常量（readme 修改点 3）。
 *
 * 传感器采样周期 3 分钟；App 侧 BLE 读取周期与之一致（`MqtttService.READ_INTERVAL_MS`），
 * 保留手动立即读取通道。此处只断言常量本身与毫秒/秒一致性。
 */
class SensorCadenceTest {

    @Test
    fun samplePeriod_isThreeMinutes() {
        assertEquals(180L, SensorCadence.SAMPLE_PERIOD_SECONDS)
        assertEquals(3 * 60L, SensorCadence.SAMPLE_PERIOD_SECONDS)
        assertEquals(180_000L, SensorCadence.SAMPLE_PERIOD_MS)
        assertEquals(SensorCadence.SAMPLE_PERIOD_SECONDS * 1000L, SensorCadence.SAMPLE_PERIOD_MS)
    }

    @Test
    fun samplePeriod_isNotOneOrFiveMinutes() {
        assertTrue("must not be the legacy 5-minute poll", SensorCadence.SAMPLE_PERIOD_MS != 5 * 60 * 1000L)
        assertTrue("must not be a 1-minute cadence", SensorCadence.SAMPLE_PERIOD_MS != 60 * 1000L)
    }
}
