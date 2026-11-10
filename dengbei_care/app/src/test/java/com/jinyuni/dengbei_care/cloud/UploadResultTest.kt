package com.jinyuni.dengbei_care.cloud

import java.io.IOException
import java.net.SocketTimeoutException
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/** AA-005 G-6 / D-27：上传结果分类纯逻辑自检 */
class UploadResultTest {

    @Test
    fun twoHundreds_areSuccess() {
        listOf(200, 201, 204, 299).forEach {
            assertEquals(UploadResult.Success, UploadResult.fromHttp(it))
        }
    }

    @Test
    fun authRejected_is401Or403() {
        assertEquals(UploadResult.AuthRejected(401), UploadResult.fromHttp(401))
        assertEquals(UploadResult.AuthRejected(403), UploadResult.fromHttp(403))
    }

    @Test
    fun otherClientErrors_areRequestRejected_except408And429() {
        listOf(400, 404, 413, 422, 418).forEach {
            assertEquals(UploadResult.RequestRejected(it), UploadResult.fromHttp(it))
        }
        assertTrue(UploadResult.fromHttp(408) is UploadResult.Retryable)
        assertTrue(UploadResult.fromHttp(429) is UploadResult.Retryable)
    }

    @Test
    fun serverErrorsAndUnknown_areRetryable() {
        listOf(500, 502, 503, 599).forEach {
            assertTrue(UploadResult.fromHttp(it) is UploadResult.Retryable)
        }
        assertTrue(UploadResult.fromHttp(null) is UploadResult.Retryable)
        assertTrue(UploadResult.fromHttp(0) is UploadResult.Retryable)
        assertTrue(UploadResult.fromHttp(302) is UploadResult.Retryable)
    }

    @Test
    fun exceptions_areRetryableWithCause() {
        val timeout = UploadResult.fromError(SocketTimeoutException("timeout"))
        assertTrue(timeout is UploadResult.Retryable)
        assertTrue((timeout as UploadResult.Retryable).cause.contains("SocketTimeoutException"))

        val io = UploadResult.fromError(IOException("network down"))
        assertTrue(io is UploadResult.Retryable)
        assertTrue((io as UploadResult.Retryable).cause.contains("network down"))
    }
}
