package com.jinyuni.dengbei_care.telemetry

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * ITEM-003 实现侧自检：MQTT 批量载荷严格归因（telemetry/ReadingAttribution）。
 *
 * 覆盖 TD-SW-002 §4.1：
 * - T-SW-L0-04 归因：数目一致（含负数温度与 0 值），devId 取自 savedIds 顺序且随读数下沉；
 * - T-SW-L0-05 归因：拒绝整批（数目不一致 / 序列为空 / 含非法数值 / 时间非法或缺失）。
 */
class ReadingAttributionTest {

    private val savedIds = listOf("A1B2C3D4", "0C118224", "33333333")

    private fun success(result: ReadingAttribution.Result): ReadingAttribution.Batch {
        assertTrue("expected Success but got $result", result is ReadingAttribution.Result.Success)
        return (result as ReadingAttribution.Result.Success).batch
    }

    private fun rejected(
        result: ReadingAttribution.Result,
        reason: ReadingAttribution.RejectionReason
    ) {
        assertTrue("expected Rejected but got $result", result is ReadingAttribution.Result.Rejected)
        assertEquals(reason, (result as ReadingAttribution.Result.Rejected).reason)
    }

    // ---------- L0-04 数目一致 ----------

    @Test
    fun consistentBatch_isAttributedInSavedIdOrder() {
        val batch = success(
            ReadingAttribution.attribute("1699999999:24.5,25.5,26.5:55.0,60.0,65.0", savedIds)
        )
        assertEquals(1699999999L, batch.timeSeconds)
        assertEquals(3, batch.readings.size)
        assertEquals(listOf("A1B2C3D4", "0C118224", "33333333"), batch.readings.map { it.devId })
        assertEquals(listOf(24.5, 25.5, 26.5), batch.readings.map { it.temperature })
        assertEquals(listOf(55.0, 60.0, 65.0), batch.readings.map { it.humidity })
    }

    @Test
    fun negativeAndZeroValues_areAccepted() {
        val batch = success(
            ReadingAttribution.attribute("1700000000:-3.2,0.0,125.0:0.0,55.0,100.0", savedIds)
        )
        assertEquals(listOf(-3.2, 0.0, 125.0), batch.readings.map { it.temperature })
        assertEquals(listOf(0.0, 55.0, 100.0), batch.readings.map { it.humidity })
    }

    @Test
    fun devIdTravelsWithReading_notWithIndex() {
        // 同一个值集合配不同绑定顺序 → 归因结果随 savedIds 走（证明归属来自映射而非下标含义）
        val swapped = listOf("33333333", "0C118224", "A1B2C3D4")
        val batch = success(
            ReadingAttribution.attribute("1700000001:24.5,25.5,26.5:55.0,60.0,65.0", swapped)
        )
        assertEquals(listOf("33333333", "0C118224", "A1B2C3D4"), batch.readings.map { it.devId })
        assertEquals(24.5, batch.readings[0].temperature, 0.0)
        assertEquals(26.5, batch.readings[2].temperature, 0.0)
    }

    @Test
    fun singleDevice_isAccepted() {
        val batch = success(ReadingAttribution.attribute("1700000002:30.1:44.4", listOf("A1B2C3D4")))
        assertEquals(1, batch.readings.size)
        assertEquals("A1B2C3D4", batch.readings[0].devId)
        assertEquals(30.1, batch.readings[0].temperature, 0.0)
        assertEquals(44.4, batch.readings[0].humidity, 0.0)
    }

    @Test
    fun spacedSeparators_areTolerated() {
        // App 自身发布 BLE 读数时用 ", " 连接，接收侧容忍逗号后的空格
        val batch = success(
            ReadingAttribution.attribute("1700000003: 24.5 , 25.5 : 55.0 , 60.0 ", listOf("A1B2C3D4", "0C118224"))
        )
        assertEquals(listOf(24.5, 25.5), batch.readings.map { it.temperature })
        assertEquals(listOf(55.0, 60.0), batch.readings.map { it.humidity })
    }

    // ---------- L0-05 拒绝整批：数目不一致 ----------

    @Test
    fun extraTemperature_rejectsWholeBatch() {
        rejected(
            ReadingAttribution.attribute("1700000004:24.5,25.5,26.5,27.5:55.0,60.0,65.0", savedIds),
            ReadingAttribution.RejectionReason.COUNT_MISMATCH
        )
    }

    @Test
    fun missingHumidity_rejectsWholeBatch() {
        rejected(
            ReadingAttribution.attribute("1700000005:24.5,25.5,26.5:55.0,60.0", savedIds),
            ReadingAttribution.RejectionReason.COUNT_MISMATCH
        )
    }

    @Test
    fun deviceCountDiffersFromSeries_rejectsWholeBatch() {
        rejected(
            ReadingAttribution.attribute("1700000006:24.5,25.5:55.0,60.0", savedIds),
            ReadingAttribution.RejectionReason.COUNT_MISMATCH
        )
    }

    // ---------- L0-05 拒绝整批：空序列 / 非法数值 / 时间 ----------

    @Test
    fun emptyTemperatureSeries_rejectsWholeBatch() {
        rejected(
            ReadingAttribution.attribute("1700000007::55.0,60.0,65.0", savedIds),
            ReadingAttribution.RejectionReason.EMPTY_SERIES
        )
    }

    @Test
    fun emptyHumiditySeries_rejectsWholeBatch() {
        rejected(
            ReadingAttribution.attribute("1700000008:24.5,25.5,26.5:", savedIds),
            ReadingAttribution.RejectionReason.EMPTY_SERIES
        )
    }

    @Test
    fun nonNumericToken_rejectsWholeBatch() {
        listOf(
            "1700000009:abc,25.5,26.5:55.0,60.0,65.0",
            "1700010000:24.5,,26.5:55.0,60.0,65.0",
            "1700010001:24.5,25.5,:55.0,60.0,65.0",
            "1700010002:24.5,25.5,26.5:55.0,abc,65.0"
        ).forEach { payload ->
            rejected(
                ReadingAttribution.attribute(payload, savedIds),
                ReadingAttribution.RejectionReason.INVALID_NUMBER
            )
        }
    }

    @Test
    fun nonFiniteValue_rejectsWholeBatch() {
        listOf("NaN", "Infinity", "-Infinity").forEach { token ->
            rejected(
                ReadingAttribution.attribute("1700010003:$token,25.5,26.5:55.0,60.0,65.0", savedIds),
                ReadingAttribution.RejectionReason.INVALID_NUMBER
            )
        }
    }

    @Test
    fun invalidOrMissingTime_rejectsWholeBatch() {
        listOf(
            "abc:24.5,25.5,26.5:55.0,60.0,65.0",
            ":24.5,25.5,26.5:55.0,60.0,65.0",
            "0:24.5,25.5,26.5:55.0,60.0,65.0",
            "-5:24.5,25.5,26.5:55.0,60.0,65.0",
            "24.5:55.0,60.0,65.0" // 仅两段 → 缺时间与湿度段
        ).forEach { payload ->
            rejected(
                ReadingAttribution.attribute(payload, savedIds),
                ReadingAttribution.RejectionReason.INVALID_TIME
            )
        }
    }

    @Test
    fun noBoundDevice_rejectsWholeBatch() {
        rejected(
            ReadingAttribution.attribute("1700010004:24.5:55.0", emptyList()),
            ReadingAttribution.RejectionReason.NO_BOUND_DEVICE
        )
    }

    // ---------- 兼容与确定性 ----------

    @Test
    fun extraColonSegments_areIgnored_likeBefore() {
        // 旧实现取 parts[0..2] 并忽略多余段；本模块保持同样口径
        val batch = success(
            ReadingAttribution.attribute("1700010005:24.5,25.5,26.5:55.0,60.0,65.0:extra", savedIds)
        )
        assertEquals(3, batch.readings.size)
        assertEquals(1700010005L, batch.timeSeconds)
    }

    @Test
    fun rejectedResult_neverCarriesReadings() {
        val result = ReadingAttribution.attribute("1700010006:24.5,25.5:55.0,60.0,65.0", savedIds)
        assertTrue(result is ReadingAttribution.Result.Rejected)
        // Rejected 类型本身不携带任何读数（无 samples/readings 字段）
        assertEquals(
            ReadingAttribution.RejectionReason.COUNT_MISMATCH,
            (result as ReadingAttribution.Result.Rejected).reason
        )
    }
}
