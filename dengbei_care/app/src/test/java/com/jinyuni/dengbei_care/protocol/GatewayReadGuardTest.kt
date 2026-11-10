package com.jinyuni.dengbei_care.protocol

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/** AA-005 G-7 / D-31 / D-32：容量与读取完整性守卫自检 */
class GatewayReadGuardTest {

    @Test
    fun constants_matchContract() {
        assertEquals(9, GatewayReadGuard.MAX_DEVICES)
        assertEquals(160, GatewayReadGuard.MAX_FRAME_BYTES)
        assertEquals(3, GatewayReadGuard.ATT_OVERHEAD)
    }

    @Test
    fun expectedFrameSize_matchesCodec_forZeroToNine() {
        for (n in 0..9) {
            assertEquals(16 * (1 + n), GatewayReadGuard.expectedFrameSize(n))
            assertEquals(
                "must stay consistent with GatewayFrameCodec",
                GatewayFrameCodec.expectedFrameSize(n),
                GatewayReadGuard.expectedFrameSize(n)
            )
        }
        assertEquals(160, GatewayReadGuard.expectedFrameSize(9))
        assertEquals(176, GatewayReadGuard.expectedFrameSize(10))
    }

    @Test
    fun capacity_allowsNine_rejectsTenAndMore() {
        assertTrue(GatewayReadGuard.capacityVerdict(0) is GatewayReadGuard.Capacity.Ok)
        assertTrue(GatewayReadGuard.capacityVerdict(9) is GatewayReadGuard.Capacity.Ok)
        assertEquals(
            10,
            (GatewayReadGuard.capacityVerdict(10) as GatewayReadGuard.Capacity.Exceeded).bound
        )
        assertEquals(
            255,
            (GatewayReadGuard.capacityVerdict(255) as GatewayReadGuard.Capacity.Exceeded).bound
        )
        assertEquals(
            50,
            (GatewayReadGuard.capacityVerdict(50) as GatewayReadGuard.Capacity.Exceeded).bound
        )
    }

    @Test
    fun mtuVerdict_requiresExpectedPlusAttOverhead() {
        // 1 设备：32 + 3 = 35
        assertTrue(GatewayReadGuard.mtuVerdict(35, 1) is GatewayReadGuard.MtuVerdict.Proceed)
        val tooSmall = GatewayReadGuard.mtuVerdict(23, 1) as GatewayReadGuard.MtuVerdict.Refuse
        assertEquals(35, tooSmall.requiredMtu)
        assertEquals(23, tooSmall.actualMtu)
        // 9 设备：160 + 3 = 163
        assertTrue(GatewayReadGuard.mtuVerdict(163, 9) is GatewayReadGuard.MtuVerdict.Proceed)
        assertEquals(
            163,
            (GatewayReadGuard.mtuVerdict(162, 9) as GatewayReadGuard.MtuVerdict.Refuse).requiredMtu
        )
        assertTrue(GatewayReadGuard.mtuVerdict(240, 9) is GatewayReadGuard.MtuVerdict.Proceed)
    }

    @Test
    fun lengthVerdict_alignmentCheckedFirst() {
        // 20 B：既不足 32 也不是 16 的整数倍 → 对齐优先
        val twenty = GatewayReadGuard.lengthVerdict(20, 1) as GatewayReadGuard.LengthVerdict.Refuse
        assertEquals(GatewayReadGuard.ReadRefuseReason.LENGTH_NOT_ALIGNED, twenty.reason)
        assertEquals(32, twenty.expectedBytes)
        assertEquals(20, twenty.actualBytes)
    }

    @Test
    fun lengthVerdict_tooSmallWhenAlignedButShort() {
        val short = GatewayReadGuard.lengthVerdict(32, 3) as GatewayReadGuard.LengthVerdict.Refuse
        assertEquals(GatewayReadGuard.ReadRefuseReason.LENGTH_TOO_SMALL, short.reason)
        assertEquals(64, short.expectedBytes)
        assertEquals(32, short.actualBytes)
    }

    @Test
    fun lengthVerdict_acceptsExactAndAlignedTail() {
        assertEquals(
            32,
            (GatewayReadGuard.lengthVerdict(32, 1) as GatewayReadGuard.LengthVerdict.Proceed)
                .expectedBytes
        )
        assertEquals(
            160,
            (GatewayReadGuard.lengthVerdict(160, 9) as GatewayReadGuard.LengthVerdict.Proceed)
                .expectedBytes
        )
        // 16 B 对齐的多余尾块允许（codec 只消费 devCount 个设备块，不把尾块当设备）
        assertEquals(
            160,
            (GatewayReadGuard.lengthVerdict(176, 9) as GatewayReadGuard.LengthVerdict.Proceed)
                .expectedBytes
        )
        assertEquals(
            160,
            (GatewayReadGuard.lengthVerdict(256, 9) as GatewayReadGuard.LengthVerdict.Proceed)
                .expectedBytes
        )
    }

    @Test
    fun lengthVerdict_negativeIsRejected() {
        val r = GatewayReadGuard.lengthVerdict(-16, 1) as GatewayReadGuard.LengthVerdict.Refuse
        assertEquals(GatewayReadGuard.ReadRefuseReason.LENGTH_NOT_ALIGNED, r.reason)
    }
}
