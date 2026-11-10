package com.jinyuni.dengbei_care.cloud

import com.jinyuni.dengbei_care.ui.zhuce.SensorUploadData
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * AA-005 G-6 / D-27：上传失败分类、阻塞可观测与手动重试（宿主机编排自检）。
 */
class CloudUploadFailureClassificationTest {

    private var clock = 1_700_000_000L
    private val store = InMemoryStore()
    private val outbox = UploadOutbox(store) { clock }
    private val api = FakeDetailedApi()
    private val gate = FakeGate()

    private fun repo(
        config: CloudUploadRepository.Config = CloudUploadRepository.Config()
    ) = CloudUploadRepository(
        api = api,
        outbox = outbox,
        gate = gate,
        config = config,
        now = { clock },
        scope = CoroutineScope(Dispatchers.Unconfined)
    )

    private fun enqueue(vararg times: Long) {
        times.forEach { outbox.enqueue("A1B2C3D4", it, 25.5, 60.0) }
    }

    @Test
    fun authRejected_parksRows_noAutoRetry_andIsObservable() = runBlocking {
        enqueue(1L, 2L)
        api.detailed = UploadResult.AuthRejected(401)

        val outcome = repo().uploadPendingOnce()

        assertEquals("upload_blocked", outcome.skipped)
        assertEquals(2, outcome.failed)
        assertEquals(2, outbox.pendingCount())
        // 不产生自动重试：attempts 置为上限，dueBatch 取不到
        assertEquals(0, outbox.dueBatch(CloudUploadRepository.DEFAULT_MAX_ATTEMPTS, 50).size)
        assertEquals(CloudUploadRepository.DEFAULT_MAX_ATTEMPTS, store.rows.values.first().attempts)
        // 阻塞状态可观测
        assertEquals("AUTH_REJECTED", gate.blockedReason)
        assertEquals(401, gate.blockedStatus)
        assertEquals(2, gate.blockedPending)
        assertEquals(1, api.calls)
    }

    @Test
    fun requestRejected_parksRows_andIsObservable() = runBlocking {
        enqueue(1L)
        api.detailed = UploadResult.RequestRejected(422)

        val outcome = repo().uploadPendingOnce()

        assertEquals("upload_blocked", outcome.skipped)
        assertEquals(1, outbox.pendingCount())
        assertEquals("REQUEST_REJECTED", gate.blockedReason)
        assertEquals(422, gate.blockedStatus)
    }

    @Test
    fun retryable_keepsBackoff_behaviour() = runBlocking {
        enqueue(1L)
        api.detailed = UploadResult.Retryable(503, "http_503")

        val outcome = repo().uploadPendingOnce()

        assertEquals("upload_failed", outcome.skipped)
        assertEquals(1, store.rows.values.single().attempts)
        assertEquals(clock + CloudUploadRepository.DEFAULT_INITIAL_BACKOFF_SECONDS,
            store.rows.values.single().nextAttemptAt)
        assertNull(gate.blockedReason)
    }

    @Test
    fun success_deletesRows_andReportsOk() = runBlocking {
        enqueue(1L, 2L)
        val outcome = repo().uploadPendingOnce()

        assertEquals(2, outcome.uploaded)
        assertEquals(0, outbox.pendingCount())
        assertEquals("OK", gate.lastStatus)
    }

    @Test
    fun manualRetry_clearsBlocked_restoresRows_andUploadsImmediately() = runBlocking {
        enqueue(1L, 2L)
        api.detailed = UploadResult.AuthRejected(403)
        val repository = repo()
        repository.uploadPendingOnce()
        assertEquals(0, outbox.dueBatch(CloudUploadRepository.DEFAULT_MAX_ATTEMPTS, 50).size)

        // 用户点“重试上传”：清除阻塞 + 恢复被暂停的行 + 立即上传一次
        api.detailed = UploadResult.Success
        repository.retryBlockedNow()

        assertNull(gate.blockedReason)
        assertEquals(0, outbox.pendingCount())
        assertEquals("OK", gate.lastStatus)
    }

    /**
     * D-AA005-1 回归：401/403/4xx 后必须**全局**暂停自动上传。
     *
     * 单批只覆盖 1 行时，另 1 行仍在待发箱且到期；下一次自动触发不得再请求（返回 empty，调用数不变），
     * 直到手动重试清除阻塞。
     */
    @Test
    fun globalBlocked_stopsAllAutomaticUploads_untilManualRetry() = runBlocking {
        enqueue(10L, 11L)
        api.detailed = UploadResult.AuthRejected(401)
        val repository = repo(CloudUploadRepository.Config(batchLimit = 1))

        val first = repository.uploadPendingOnce()
        assertEquals("upload_blocked", first.skipped)
        assertEquals(1, api.calls)
        assertEquals(2, outbox.pendingCount())
        assertTrue(repository.isUploadBlocked())

        // 模拟下一次自动触发（每轮 BLE / 入队后 / 网络恢复）：不得取行、不得请求
        clock += 200
        val second = repository.uploadPendingOnce()
        assertEquals("empty", second.skipped)
        assertEquals(0, second.attempted)
        assertEquals("401 后不得再产生自动上传请求", 1, api.calls)
        assertEquals(2, outbox.pendingCount())

        // 手动重试：解除阻塞并恢复被暂停行；单批 1 行 → 成功删除 1 行，另 1 行仍保留且不丢
        api.detailed = UploadResult.Success
        repository.retryBlockedNow()
        assertEquals(1, outbox.pendingCount())
        assertFalse(repository.isUploadBlocked())
    }

    @Test
    fun blockedState_stillEnqueuesNewReadings_withoutAutomaticRequest() = runBlocking {
        enqueue(10L)
        api.detailed = UploadResult.AuthRejected(401)
        val repository = repo(CloudUploadRepository.Config(batchLimit = 1))
        repository.uploadPendingOnce()
        val callsAfterBlock = api.calls

        // 阻塞期间新读数仍入队（不丢数据），但不得自动触发上传
        assertTrue(repository.enqueueAndUpload("0C118224", 12L, 26.0, 61.0))
        assertEquals(callsAfterBlock, api.calls)
        assertTrue(outbox.pendingCount() >= 2)
    }

    @Test
    fun booleanOnlyApi_staysCompatible() = runBlocking {
        // 既有伪实现只实现 Boolean upload：false 仍按可重试失败处理
        val legacy = object : CloudUploadRepository.UploadApi {
            override suspend fun upload(data: SensorUploadData): Boolean = false
        }
        val repository = CloudUploadRepository(
            api = legacy,
            outbox = outbox,
            gate = gate,
            now = { clock },
            scope = CoroutineScope(Dispatchers.Unconfined)
        )
        enqueue(1L)

        val outcome = repository.uploadPendingOnce()

        assertEquals("upload_failed", outcome.skipped)
        assertEquals(1, outbox.pendingCount())
    }

    // ---------- 测试替身 ----------

    private class InMemoryStore : UploadOutboxStore {
        val rows = LinkedHashMap<Pair<String, Long>, UploadOutboxRow>()

        override fun insertIfAbsent(row: UploadOutboxRow): Boolean {
            val key = row.devId to row.time
            if (rows.containsKey(key)) return false
            rows[key] = row
            return true
        }

        override fun dueRows(now: Long, maxAttempts: Int, limit: Int): List<UploadOutboxRow> =
            rows.values
                .filter { it.nextAttemptAt <= now && it.attempts < maxAttempts }
                .sortedWith(compareBy({ it.nextAttemptAt }, { it.time }, { it.devId }))
                .take(limit)

        override fun delete(devId: String, time: Long) {
            rows.remove(devId to time)
        }

        override fun updateRetry(devId: String, time: Long, attempts: Int, nextAttemptAt: Long) {
            val key = devId to time
            rows[key]?.let { rows[key] = it.copy(attempts = attempts, nextAttemptAt = nextAttemptAt) }
        }

        override fun count(): Int = rows.size

        override fun resetForManualRetry(maxAttempts: Int) {
            rows.keys.toList().forEach { key ->
                val row = rows.getValue(key)
                if (row.attempts >= maxAttempts) {
                    rows[key] = row.copy(attempts = 0, nextAttemptAt = 0L)
                }
            }
        }
    }

    private class FakeDetailedApi(
        var detailed: UploadResult = UploadResult.Success
    ) : CloudUploadRepository.UploadApi {
        var calls = 0

        override suspend fun upload(data: SensorUploadData): Boolean =
            uploadDetailed(data) is UploadResult.Success

        override suspend fun uploadDetailed(data: SensorUploadData): UploadResult {
            calls++
            return detailed
        }
    }

    private class FakeGate : CloudUploadRepository.UploadGate {
        override fun guardActive(): Boolean = true
        override fun phone(): String = "13800000000"
        override fun gatewayMac(): String = "1010100000A1"

        var blockedReason: String? = null
        var blockedStatus: Int? = null
        var blockedPending: Int = 0
        var lastStatus: String? = null

        override fun markUploadBlocked(result: UploadResult, pendingCount: Int) {
            blockedReason = when (result) {
                is UploadResult.AuthRejected -> "AUTH_REJECTED"
                is UploadResult.RequestRejected -> "REQUEST_REJECTED"
                else -> "RETRYABLE"
            }
            blockedStatus = when (result) {
                is UploadResult.AuthRejected -> result.httpStatus
                is UploadResult.RequestRejected -> result.httpStatus
                else -> null
            }
            blockedPending = pendingCount
        }

        override fun clearUploadBlocked() {
            blockedReason = null
            blockedStatus = null
            blockedPending = 0
        }

        override fun markUploadRetryScheduled(pendingCount: Int) {
            lastStatus = "RETRYING"
        }

        override fun markUploadSucceeded(pendingCount: Int) {
            lastStatus = "OK"
            blockedReason = null
        }
    }
}
