package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.protocol.GatewayFrameCodec
import com.jinyuni.dengbei_care.protocol.GatewayReadGuard
import com.jinyuni.dengbei_care.protocol.GatewayReadState
import java.io.File
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * AA-006 N-2 独立验证（软件测试，TD-SW-005 §4/§5.2）。
 *
 * 使用独立期望表验证读取终态映射：每次读取尝试都必须落到可区分终态，
 * 枚举中不存在 CONNECTING。真实网关 devCount=0 的显示与下一周期由真机用例覆盖。
 */
class GatewayReadStateItem002VerificationTest {

    private fun assertState(
        expectedCode: GatewayReadState.Code,
        expectedKey: String,
        status: GatewayReadState.Status
    ) {
        assertEquals(expectedCode, status.code)
        assertTrue(
            "文案应包含关键子串 '$expectedKey'，实际='${status.text}'",
            status.text.contains(expectedKey)
        )
        assertFalse(
            "终态文案/状态不得为 CONNECTING",
            status.code.name.contains("CONNECTING") || status.text.contains("正在连接")
        )
    }

    // N2-H-01/H-02/H-03/H-04
    @Test
    fun successMatrix_dataOk_noData_noDevice() {
        assertState(
            GatewayReadState.Code.DATA_OK, "网关已连接（最近读取",
            GatewayReadState.success("21:57", devCount = 1, validCount = 1)
        )
        assertState(
            GatewayReadState.Code.GATEWAY_NO_DATA, "网关暂无传感器数据",
            GatewayReadState.success("21:57", devCount = 3, validCount = 0)
        )
        assertState(
            GatewayReadState.Code.DATA_OK, "网关已连接（最近读取",
            GatewayReadState.success("21:57", devCount = 3, validCount = 2)
        )
        assertState(
            GatewayReadState.Code.GATEWAY_NO_DEVICE, "N=0",
            GatewayReadState.rejected(GatewayFrameCodec.RejectionReason.NO_DEVICE)
        )
    }

    // N2-H-05/H-06/H-07/H-08
    @Test
    fun rejectedMatrix_mapsEachReason() {
        assertState(
            GatewayReadState.Code.READ_INVALID, "帧头不足",
            GatewayReadState.rejected(GatewayFrameCodec.RejectionReason.FRAME_TOO_SHORT)
        )
        assertState(
            GatewayReadState.Code.READ_INCOMPLETE, "长度不足",
            GatewayReadState.rejected(GatewayFrameCodec.RejectionReason.LENGTH_MISMATCH)
        )
        assertState(
            GatewayReadState.Code.READ_INVALID, "密文未对齐",
            GatewayReadState.rejected(GatewayFrameCodec.RejectionReason.CIPHERTEXT_NOT_ALIGNED)
        )
        assertState(
            GatewayReadState.Code.READ_INVALID, "解密失败",
            GatewayReadState.rejected(GatewayFrameCodec.RejectionReason.DECRYPT_FAILED)
        )
    }

    // N2-H-09：长度守卫拒绝全部归为 READ_INCOMPLETE，并携带实际/期望字节
    @Test
    fun lengthRefusedMatrix_isReadIncomplete() {
        assertState(
            GatewayReadState.Code.READ_INCOMPLETE, "MTU 不足",
            GatewayReadState.lengthRefused(
                GatewayReadGuard.ReadRefuseReason.MTU_TOO_SMALL, actualBytes = 0, expectedBytes = 160
            )
        )
        val tooSmall = GatewayReadState.lengthRefused(
            GatewayReadGuard.ReadRefuseReason.LENGTH_TOO_SMALL, actualBytes = 32, expectedBytes = 48
        )
        assertState(GatewayReadState.Code.READ_INCOMPLETE, "返回 32 字节", tooSmall)
        assertTrue(tooSmall.text.contains("需要 48 字节"))

        assertState(
            GatewayReadState.Code.READ_INCOMPLETE, "非 16 字节对齐",
            GatewayReadState.lengthRefused(
                GatewayReadGuard.ReadRefuseReason.LENGTH_NOT_ALIGNED, actualBytes = 37, expectedBytes = 48
            )
        )
    }

    // N2-H-10/H-11：读取回调直接失败/非对齐输入也落终态，且映射稳定
    @Test
    fun unalignedAndStability() {
        assertState(
            GatewayReadState.Code.READ_INCOMPLETE, "非 16 字节对齐",
            GatewayReadState.unaligned(returnedBytes = 0)
        )

        val first = GatewayReadState.rejected(
            GatewayFrameCodec.RejectionReason.DECRYPT_FAILED, "bad key"
        )
        val second = GatewayReadState.rejected(
            GatewayFrameCodec.RejectionReason.DECRYPT_FAILED, "bad key"
        )
        assertEquals(first, second)
        assertTrue(first.text.contains("bad key"))
    }

    // 枚举全集不得包含 CONNECTING（N2-H-11 附加断言）
    @Test
    fun enumHasNoConnectingValue() {
        assertFalse(
            GatewayReadState.Code.values().any { it.name.contains("CONNECTING") }
        )
        assertEquals(
            setOf(
                GatewayReadState.Code.DATA_OK,
                GatewayReadState.Code.GATEWAY_NO_DATA,
                GatewayReadState.Code.GATEWAY_NO_DEVICE,
                GatewayReadState.Code.READ_INCOMPLETE,
                GatewayReadState.Code.READ_INVALID
            ),
            GatewayReadState.Code.values().toSet()
        )
    }

    // N2-S-02：纯逻辑，无 Android 依赖
    @Test
    fun gatewayReadState_hasNoAndroidImports() {
        val candidates = listOf(
            File("src/main/java/com/jinyuni/dengbei_care/protocol/GatewayReadState.kt"),
            File("app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayReadState.kt")
        )
        val file = candidates.firstOrNull { it.isFile }
            ?: error("GatewayReadState.kt not found (cwd=${File(".").absolutePath})")
        val code = file.readText()
        assertFalse("不得 import android.*", code.contains("import android."))
        assertFalse("不得 import androidx.*", code.contains("import androidx."))
    }
}
