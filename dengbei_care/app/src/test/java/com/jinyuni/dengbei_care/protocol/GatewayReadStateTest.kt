package com.jinyuni.dengbei_care.protocol

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/** AA-006 N-2：BLE 读取终态映射矩阵（宿主） */
class GatewayReadStateTest {

    @Test
    fun success_withValidReadings_isDataOk() {
        val status = GatewayReadState.success("21:57", devCount = 2, validCount = 2)
        assertEquals(GatewayReadState.Code.DATA_OK, status.code)
        assertTrue(status.text.contains("最近读取 21:57"))
    }

    @Test
    fun success_allPlaceholder_isGatewayNoData() {
        val status = GatewayReadState.success("21:57", devCount = 2, validCount = 0)
        assertEquals(GatewayReadState.Code.GATEWAY_NO_DATA, status.code)
        assertTrue(status.text.contains("暂无传感器数据"))
    }

    @Test
    fun success_zeroDevice_isGatewayNoDevice() {
        val status = GatewayReadState.success("21:57", devCount = 0, validCount = 0)
        assertEquals(GatewayReadState.Code.GATEWAY_NO_DEVICE, status.code)
        assertTrue(status.text.contains("暂无设备"))
    }

    @Test
    fun rejected_noDevice_isGatewayNoDevice() {
        val status = GatewayReadState.rejected(GatewayFrameCodec.RejectionReason.NO_DEVICE)
        assertEquals(GatewayReadState.Code.GATEWAY_NO_DEVICE, status.code)
    }

    @Test
    fun rejected_lengthMismatch_isReadIncomplete() {
        val status = GatewayReadState.rejected(GatewayFrameCodec.RejectionReason.LENGTH_MISMATCH, "plain<expected")
        assertEquals(GatewayReadState.Code.READ_INCOMPLETE, status.code)
    }

    @Test
    fun rejected_invalidFrames_areReadInvalid() {
        listOf(
            GatewayFrameCodec.RejectionReason.FRAME_TOO_SHORT,
            GatewayFrameCodec.RejectionReason.DECRYPT_FAILED,
            GatewayFrameCodec.RejectionReason.CIPHERTEXT_NOT_ALIGNED
        ).forEach { reason ->
            val status = GatewayReadState.rejected(reason)
            assertEquals("reason=$reason", GatewayReadState.Code.READ_INVALID, status.code)
        }
    }

    @Test
    fun lengthRefused_allReasons_areReadIncomplete() {
        val mtu = GatewayReadState.lengthRefused(
            GatewayReadGuard.ReadRefuseReason.MTU_TOO_SMALL, actualBytes = 0, expectedBytes = 163
        )
        assertEquals(GatewayReadState.Code.READ_INCOMPLETE, mtu.code)
        assertTrue(mtu.text.contains("MTU 不足"))

        val short = GatewayReadState.lengthRefused(
            GatewayReadGuard.ReadRefuseReason.LENGTH_TOO_SMALL, actualBytes = 32, expectedBytes = 64
        )
        assertEquals(GatewayReadState.Code.READ_INCOMPLETE, short.code)
        assertTrue(short.text.contains("32") && short.text.contains("64"))

        val unaligned = GatewayReadState.lengthRefused(
            GatewayReadGuard.ReadRefuseReason.LENGTH_NOT_ALIGNED, actualBytes = 20, expectedBytes = 32
        )
        assertEquals(GatewayReadState.Code.READ_INCOMPLETE, unaligned.code)
        assertTrue(unaligned.text.contains("非 16 字节对齐"))
    }

    @Test
    fun unalignedCallback_isReadIncomplete() {
        val status = GatewayReadState.unaligned(20)
        assertEquals(GatewayReadState.Code.READ_INCOMPLETE, status.code)
        assertTrue(status.text.contains("20"))
    }
}
