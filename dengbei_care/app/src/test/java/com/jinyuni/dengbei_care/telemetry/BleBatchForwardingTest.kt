package com.jinyuni.dengbei_care.telemetry

import com.jinyuni.dengbei_care.telemetry.BleBatchForwarding.Plan
import com.jinyuni.dengbei_care.telemetry.BleBatchForwarding.RefuseReason
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/** AA-005 G-3 / D-20 / D-26：BLE 批次转发纯逻辑自检 */
class BleBatchForwardingTest {

    private fun reading(devId: String, tX10: Int, hX10: Int) =
        BleObservation.Reading(devId, tX10, hX10)

    @Test
    fun singleDevice_payloadIsExactlyTimeTempsHumis() {
        val plan = BleBatchForwarding.plan(
            readings = listOf(reading("A1B2C3D4", 255, 600)),
            boundOrder = listOf("A1B2C3D4"),
            timeSeconds = 1_700_000_000L
        )
        assertEquals(Plan.Forward("1700000000:25.5:60.0"), plan)
        // 可被自身归因器解析
        val attributed = ReadingAttribution.attribute(
            (plan as Plan.Forward).payload, listOf("A1B2C3D4")
        )
        assertTrue(attributed is ReadingAttribution.Result.Success)
    }

    @Test
    fun multiDevice_usesBoundOrder_andNegativeTemperature() {
        val plan = BleBatchForwarding.plan(
            readings = listOf(
                reading("A1B2C3D4", 255, 600),
                reading("0C118224", -52, 415),
                reading("11223344", 350, 0)
            ),
            boundOrder = listOf("A1B2C3D4", "0C118224", "11223344"),
            timeSeconds = 42L
        )
        assertEquals(Plan.Forward("42:25.5,-5.2,35.0:60.0,41.5,0.0"), plan)
    }

    @Test
    fun refuse_whenNoBoundDevice() {
        val plan = BleBatchForwarding.plan(emptyList(), emptyList(), 1L)
        assertEquals(Plan.Refuse(RefuseReason.NO_BOUND_DEVICE), plan)
    }

    @Test
    fun refuse_whenCountMismatch_andNoPartialPayload() {
        val plan = BleBatchForwarding.plan(
            listOf(reading("A1B2C3D4", 1, 1)),
            listOf("A1B2C3D4", "0C118224"),
            1L
        )
        assertEquals(Plan.Refuse(RefuseReason.COUNT_MISMATCH), plan)
    }

    @Test
    fun refuse_whenUnknownId() {
        val plan = BleBatchForwarding.plan(
            listOf(reading("DEADBEEF", 1, 1)),
            listOf("A1B2C3D4"),
            1L
        )
        assertEquals(Plan.Refuse(RefuseReason.UNKNOWN_ID), plan)
    }

    @Test
    fun refuse_whenDuplicateId() {
        val plan = BleBatchForwarding.plan(
            listOf(reading("A1B2C3D4", 1, 1), reading("A1:B2:C3:D4", 2, 2)),
            listOf("A1B2C3D4", "0C118224"),
            1L
        )
        assertEquals(Plan.Refuse(RefuseReason.DUPLICATE_ID), plan)
    }

    @Test
    fun refuse_whenOrderDiffersFromBinding() {
        val plan = BleBatchForwarding.plan(
            listOf(reading("0C118224", 1, 1), reading("A1B2C3D4", 2, 2)),
            listOf("A1B2C3D4", "0C118224"),
            1L
        )
        assertEquals(Plan.Refuse(RefuseReason.ORDER_MISMATCH), plan)
    }

    @Test
    fun refuse_allPlaceholderBatch() {
        val plan = BleBatchForwarding.plan(
            readings = emptyList(),
            boundOrder = listOf("A1B2C3D4"),
            timeSeconds = 1L,
            allPlaceholder = true
        )
        assertEquals(Plan.Refuse(RefuseReason.ALL_PLACEHOLDER), plan)
        // 空批次但不是占位 → 数量不符
        assertEquals(
            Plan.Refuse(RefuseReason.COUNT_MISMATCH),
            BleBatchForwarding.plan(emptyList(), listOf("A1B2C3D4"), 1L)
        )
    }

    @Test
    fun formatting_isLocaleUsWithOneDecimal() {
        assertEquals("25.5", BleBatchForwarding.formatX10(255))
        assertEquals("-0.1", BleBatchForwarding.formatX10(-1))
        assertEquals("0.0", BleBatchForwarding.formatX10(0))
        assertEquals("100.0", BleBatchForwarding.formatX10(1000))
    }
}
