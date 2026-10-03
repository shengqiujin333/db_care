package com.jinyuni.dengbei_care.cloud

import com.jinyuni.dengbei_care.ui.zhuce.SensorUploadData
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * ITEM-009 实现侧自检：待发箱与上传编排（`UploadOutbox` + `CloudUploadRepository`）。
 *
 * 覆盖 TD-SW-002 §4.1 T-SW-L0-12（伪 Api + 内存 Outbox，8 个判定点）与 T-SW-L0-11 的载荷联通性。
 */
class CloudUploadRepositoryTest {

    private var clock = 1_700_000_000L
    private val now: () -> Long = { clock }

    private val store = InMemoryStore()
    private val outbox = UploadOutbox(store, now)
    private val api = FakeApi()
    private val gate = FakeGate()

    private fun repo(
        config: CloudUploadRepository.Config = CloudUploadRepository.Config(),
        scope: CoroutineScope = CoroutineScope(Dispatchers.Unconfined)
    ) = CloudUploadRepository(api, outbox, gate, config, now, scope)

    private fun enqueue(times: List<Long>, devId: String = "A1B2C3D4") {
        times.forEach { outbox.enqueue(devId, it, 25.5, 60.0) }
    }

    // ---------- ① 成功 → 删除 ----------

    @Test
    fun success_deletesRows_andReportsOutcome() = runBlocking {
        enqueue(listOf(1L, 2L))
        val outcome = repo().uploadPendingOnce()

        assertEquals(2, outcome.attempted)
        assertEquals(2, outcome.uploaded)
        assertEquals(0, outcome.failed)
        assertNull(outcome.skipped)
        assertEquals(0, outbox.pendingCount())
    }

    // ---------- ② 失败 → 保留 + attempts+1 + 退避 ----------

    @Test
    fun failure_keepsRows_andSchedulesBackoff() = runBlocking {
        api.result = false
        enqueue(listOf(1L))
        val outcome = repo().uploadPendingOnce()

        assertEquals(1, outcome.failed)
        assertEquals("upload_failed", outcome.skipped)
        assertEquals(1, outbox.pendingCount())
        val row = store.rows.values.single()
        assertEquals(1, row.attempts)
        assertEquals(clock + CloudUploadRepository.DEFAULT_INITIAL_BACKOFF_SECONDS, row.nextAttemptAt)
    }

    @Test
    fun retryBecomesDue_afterBackoffElapses_andSucceeds() = runBlocking {
        api.result = false
        enqueue(listOf(1L))
        val r = repo()
        r.uploadPendingOnce()

        // 退避期内不再取到该行
        assertEquals(0, outbox.dueBatch(20, 50).size)

        clock += CloudUploadRepository.DEFAULT_INITIAL_BACKOFF_SECONDS
        api.result = true
        val outcome = r.uploadPendingOnce()

        assertEquals(1, outcome.uploaded)
        assertEquals(0, outbox.pendingCount())
    }

    // ---------- ③ 达最大尝试次数后不再自动重试 ----------

    @Test
    fun maxAttempts_stopsAutoRetry_butKeepsRow() = runBlocking {
        api.result = false
        val config = CloudUploadRepository.Config(maxAttempts = 3)
        val r = repo(config)
        enqueue(listOf(1L))

        repeat(5) {
            clock += 10_000
            r.uploadPendingOnce()
        }

        val row = store.rows.values.single()
        assertEquals("attempts must stop at maxAttempts", 3, row.attempts)
        assertEquals("row must be kept for manual/later handling", 1, outbox.pendingCount())
        assertEquals("no further automatic retry", 0, outbox.dueBatch(config.maxAttempts, config.batchLimit).size)
        // 从第 3 次失败起不再产生请求
        assertEquals(3, api.payloads.size)
    }

    // ---------- ④ 单批条数上限 ----------

    @Test
    fun batchLimit_splitsIntoMultipleBatches() = runBlocking {
        val config = CloudUploadRepository.Config(batchLimit = 50)
        val r = repo(config)
        enqueue((1L..60L).toList())

        val first = r.uploadPendingOnce()
        assertEquals(50, first.uploaded)
        assertEquals(50, api.payloads.first().readings.size)
        assertEquals(10, outbox.pendingCount())

        val second = r.uploadPendingOnce()
        assertEquals(10, second.uploaded)
        assertEquals(0, outbox.pendingCount())
        assertEquals(2, api.payloads.size)
    }

    // ---------- ⑤⑥ 门控：守护关闭 / 无手机号 ----------

    @Test
    fun guardClosed_noEnqueue_andNoRequest() = runBlocking {
        gate.guard = false
        val r = repo()

        assertFalse(r.enqueueAndUpload("A1B2C3D4", 1L, 25.5, 60.0))
        assertEquals(0, outbox.pendingCount())
        assertEquals("gate_closed", r.uploadPendingOnce().skipped)
        assertEquals(0, api.payloads.size)
    }

    @Test
    fun noRegisteredPhone_noEnqueue_andNoRequest() = runBlocking {
        gate.phone = ""
        val r = repo()

        assertFalse(r.enqueueAndUpload("A1B2C3D4", 1L, 25.5, 60.0))
        assertEquals(0, outbox.pendingCount())
        assertEquals("gate_closed", r.uploadPendingOnce().skipped)
        assertEquals(0, api.payloads.size)
    }

    // ---------- ⑦ 幂等入队 ----------

    @Test
    fun duplicateEnqueue_keepsSingleRow_andSecondReturnsFalse() {
        assertTrue(outbox.enqueue("A1B2C3D4", 100L, 25.5, 60.0))
        assertFalse(outbox.enqueue("A1B2C3D4", 100L, 26.0, 61.0)) // 同 (devId, time) → 忽略
        assertEquals(1, outbox.pendingCount())
        assertEquals(25.5, store.rows.values.single().temperature, 0.0)

        // 不同时间 → 新行
        assertTrue(outbox.enqueue("A1B2C3D4", 200L, 26.0, 61.0))
        assertEquals(2, outbox.pendingCount())
    }

    // ---------- ⑧ 串行：不产生并发重复请求 ----------

    @Test
    fun concurrentUploads_areSerialized_secondIsBusy() = runBlocking {
        val entered = CompletableDeferred<Unit>()
        val release = CompletableDeferred<Unit>()
        api.onUpload = { entered.complete(Unit); release.await() }
        enqueue(listOf(1L))
        val r = repo()

        val first = launch { r.uploadPendingOnce() }
        entered.await()

        val second = r.uploadPendingOnce()
        assertEquals("busy", second.skipped)
        assertEquals(0, second.attempted)

        release.complete(Unit)
        first.join()
        assertEquals(1, api.payloads.size)
        assertEquals(0, outbox.pendingCount())
    }

    // ---------- 参数：退避增长与上限 ----------

    @Test
    fun backoff_growsExponentially_andCapsAtMaximum() {
        val config = CloudUploadRepository.Config()
        assertEquals(60L, config.backoffSeconds(1))
        assertEquals(120L, config.backoffSeconds(2))
        assertEquals(240L, config.backoffSeconds(3))
        assertEquals(480L, config.backoffSeconds(4))
        assertEquals(960L, config.backoffSeconds(5))
        assertEquals(1800L, config.backoffSeconds(6))
        assertEquals(1800L, config.backoffSeconds(20))
        assertEquals(CloudUploadRepository.DEFAULT_INITIAL_BACKOFF_SECONDS, config.backoffSeconds(0))
    }

    @Test
    fun dueBatch_ordersByNextAttemptAt_thenTime() {
        outbox.enqueue("A1B2C3D4", 300L, 1.0, 1.0)
        outbox.enqueue("A1B2C3D4", 100L, 1.0, 1.0)
        outbox.enqueue("0C118224", 100L, 1.0, 1.0)
        store.updateRetry("A1B2C3D4", 300L, 0, 50L)
        store.updateRetry("A1B2C3D4", 100L, 0, 10L)
        store.updateRetry("0C118224", 100L, 0, 10L)

        val batch = outbox.dueBatch(20, 50)
        assertEquals(listOf(10L, 10L, 50L), batch.map { it.nextAttemptAt })
        // 同一 nextAttemptAt/时间时按 devId 升序
        assertEquals(listOf("0C118224", "A1B2C3D4", "A1B2C3D4"), batch.map { it.devId })
    }

    // ---------- 空队列 / 载荷内容 ----------

    @Test
    fun emptyOutbox_producesNoRequest() = runBlocking {
        val outcome = repo().uploadPendingOnce()
        assertEquals("empty", outcome.skipped)
        assertEquals(0, api.payloads.size)
    }

    @Test
    fun payloadFromOutbox_carriesDevIdPerRowTimeAndUnits() = runBlocking {
        outbox.enqueue("A1B2C3D4", 111L, 25.5, 60.0)
        outbox.enqueue("0C118224", 222L, -3.2, 41.5)
        repo().uploadPendingOnce()

        val payload: SensorUploadData = api.payloads.single()
        assertEquals("13800000000", payload.phone)
        assertEquals("1010100000A1", payload.mac)
        assertEquals(listOf("A1B2C3D4", "0C118224"), payload.readings.map { it.devId })
        assertEquals(listOf(111L, 222L), payload.readings.map { it.time })
        assertEquals(listOf(25.5, -3.2), payload.readings.map { it.temperature })
        assertEquals(listOf(60.0, 41.5), payload.readings.map { it.humidity })
    }

    // ---------- 异步触发不阻塞调用方 ----------

    @Test
    fun enqueueAndUpload_enqueuesAndTriggersAsyncUpload() {
        val r = repo()
        val enqueued = r.enqueueAndUpload("A1B2C3D4", 999L, 25.5, 60.0)

        assertTrue(enqueued)
        assertNotNull(api.payloads.firstOrNull()) // Unconfined scope：launch 立即执行
        assertEquals(0, outbox.pendingCount())
        assertEquals(999L, api.payloads.single().readings.single().time)
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
    }

    private class FakeApi(var result: Boolean = true) : CloudUploadRepository.UploadApi {
        val payloads = mutableListOf<SensorUploadData>()
        var onUpload: (suspend () -> Unit)? = null

        override suspend fun upload(data: SensorUploadData): Boolean {
            onUpload?.invoke()
            payloads += data
            return result
        }
    }

    private class FakeGate(
        var guard: Boolean = true,
        var phone: String = "13800000000",
        var mac: String = "1010100000A1"
    ) : CloudUploadRepository.UploadGate {
        override fun guardActive(): Boolean = guard
        override fun phone(): String = phone
        override fun gatewayMac(): String = mac
    }
}
