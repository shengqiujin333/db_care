package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.cloud.CloudUploadRepository
import com.jinyuni.dengbei_care.cloud.UploadOutbox
import com.jinyuni.dengbei_care.cloud.UploadOutboxRow
import com.jinyuni.dengbei_care.cloud.UploadOutboxStore
import com.jinyuni.dengbei_care.ui.zhuce.SensorUploadData
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.delay
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeout
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.IOException
import java.util.concurrent.atomic.AtomicInteger

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-009）——宿主机纯逻辑层。
 *
 * 用**本文件自带的**内存待发箱 / 伪 Api / 伪门控独立验证队列与编排语义（期望值按 AA-002 §5.5/§6、
 * D-07/D-09 与任务描述写出，不复用被测测试的构造）：
 * 常量集中性与退避曲线、门控（守护/手机号）、幂等入队、成功删除、失败保留+退避+到期重试、
 * 最大尝试次数后不再自动重试但保留、单批上限、Api 异常不抛出、并发串行、入队不阻塞调用方、
 * 载荷逐行时间与单位。Android SQLite 适配器的行级行为见 `androidTest` 与独立静态脚本。
 */
class CloudUploadItem009VerificationTest {

    // ---------- 独立测试替身 ----------

    private class MemStore : UploadOutboxStore {
        val rows = LinkedHashMap<Pair<String, Long>, UploadOutboxRow>()

        override fun insertIfAbsent(row: UploadOutboxRow): Boolean {
            val key = row.devId to row.time
            if (rows.containsKey(key)) return false
            rows[key] = row.copy()
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
            val old = rows[key] ?: return
            rows[key] = old.copy(attempts = attempts, nextAttemptAt = nextAttemptAt)
        }

        override fun count(): Int = rows.size
    }

    private class FakeGate(
        var guard: Boolean = true,
        var phone: String = "13800000000",
        var mac: String = "AABBCCDDEEFF"
    ) : CloudUploadRepository.UploadGate {
        override fun guardActive(): Boolean = guard
        override fun phone(): String = phone
        override fun gatewayMac(): String = mac
    }

    private class FakeApi(private val result: () -> Boolean = { true }) : CloudUploadRepository.UploadApi {
        val calls = ArrayList<SensorUploadData>()
        override suspend fun upload(data: SensorUploadData): Boolean {
            calls += data
            return result()
        }
    }

    private var clock = 1_700_000_000L
    private val now: () -> Long = { clock }
    private val store = MemStore()
    private val outbox = UploadOutbox(store, now)
    private val gate = FakeGate()

    private fun repo(
        api: CloudUploadRepository.UploadApi,
        config: CloudUploadRepository.Config = CloudUploadRepository.Config(),
        scope: CoroutineScope = CoroutineScope(Dispatchers.Unconfined)
    ) = CloudUploadRepository(api, outbox, gate, config, now, scope)

    // ---------- 1. 常量集中性与退避曲线 ----------

    @Test
    fun configDefaults_andBackoffCurve_areBounded() {
        val c = CloudUploadRepository.Config()
        assertEquals(50, c.batchLimit)
        assertEquals(60L, c.initialBackoffSeconds)
        assertEquals(1800L, c.maxBackoffSeconds)
        assertEquals(20, c.maxAttempts)
        assertEquals(listOf(60L, 120L, 240L, 480L, 960L, 1800L, 1800L, 1800L),
            (1..8).map { c.backoffSeconds(it) })
        // 单调不减且有上界
        val seq = (1..40).map { c.backoffSeconds(it) }
        assertTrue(seq.zipWithNext().all { (a, b) -> b >= a })
        assertTrue(seq.all { it in 1..1800L })
    }

    // ---------- 2. 门控：守护关闭 / 无手机号 → 不入队不发请求 ----------

    @Test
    fun gateClosed_neverEnqueuesNorRequests() = runBlocking {
        for (state in listOf(FakeGate(guard = false, phone = "13800000000"),
                             FakeGate(guard = true, phone = ""),
                             FakeGate(guard = true, phone = "   "))) {
            val api = FakeApi()
            val r = CloudUploadRepository(api, outbox, state, CloudUploadRepository.Config(), now)
            assertFalse("must not enqueue", r.enqueueAndUpload("A1B2C3D4", 1L, 25.0, 60.0))
            assertEquals(0, outbox.pendingCount())
            val outcome = r.uploadPendingOnce()
            assertEquals("gate_closed", outcome.skipped)
            assertEquals(0, outcome.attempted)
            assertTrue("no HTTP request may be issued", api.calls.isEmpty())
        }
    }

    // ---------- 3. 幂等入队（(dev_id, time)） ----------

    @Test
    fun enqueue_isIdempotentByDevIdAndTime() {
        assertTrue(outbox.enqueue("A1B2C3D4", 100L, 25.0, 60.0))
        assertFalse("same (devId,time) must not create a second row", outbox.enqueue("A1B2C3D4", 100L, 26.0, 61.0))
        assertEquals(1, outbox.pendingCount())
        assertTrue(outbox.enqueue("A1B2C3D4", 101L, 25.0, 60.0))
        assertTrue(outbox.enqueue("0C118224", 100L, 25.0, 60.0))
        assertEquals(3, outbox.pendingCount())
        // 既有行不被后续同键入队覆盖
        assertEquals(25.0, store.rows["A1B2C3D4" to 100L]!!.temperature, 0.0)
    }

    // ---------- 4. 成功 → 删除 + 载荷逐行映射 ----------

    @Test
    fun success_deletesRows_andPayloadCarriesRowValues() = runBlocking {
        val api = FakeApi { true }
        val r = repo(api)
        store.rows["A1B2C3D4" to 1000L] = row("A1B2C3D4", 1000L, -3.2, 55.0)
        store.rows["0C118224" to 1060L] = row("0C118224", 1060L, 26.5, 61.5)

        val outcome = r.uploadPendingOnce()
        assertEquals(2, outcome.attempted)
        assertEquals(2, outcome.uploaded)
        assertEquals(0, outcome.failed)
        assertNull(outcome.skipped)
        assertEquals(0, outbox.pendingCount())

        assertEquals(1, api.calls.size)
        val payload = api.calls.single()
        assertEquals("13800000000", payload.phone)
        assertEquals("AABBCCDDEEFF", payload.mac)
        assertEquals(
            setOf(Triple("A1B2C3D4", 1000L, -3.2), Triple("0C118224", 1060L, 26.5)),
            payload.readings.map { Triple(it.devId, it.time, it.temperature) }.toSet()
        )
        assertEquals(listOf(55.0, 61.5).toSet(), payload.readings.map { it.humidity }.toSet())
    }

    // ---------- 5. 失败 → 保留 + 退避；到期后重试成功 ----------

    @Test
    fun failure_retainsRowWithBackoff_thenSucceedsWhenDue() = runBlocking {
        var ok = false
        val api = FakeApi { ok }
        val r = repo(api)
        outbox.enqueue("A1B2C3D4", 2000L, 25.0, 60.0)

        val failed = r.uploadPendingOnce()
        assertEquals("upload_failed", failed.skipped)
        assertEquals(1, failed.failed)
        val row = store.rows["A1B2C3D4" to 2000L]!!
        assertEquals("attempts must increment", 1, row.attempts)
        assertEquals("next attempt = now + initial backoff", clock + 60L, row.nextAttemptAt)
        assertEquals("data must not be lost", 1, outbox.pendingCount())

        // 未到期 → 不再发请求
        val tooEarly = r.uploadPendingOnce()
        assertEquals("empty", tooEarly.skipped)
        assertEquals(1, api.calls.size)

        // 到期后重试成功 → 删除
        clock += 60L
        ok = true
        val retried = r.uploadPendingOnce()
        assertEquals(1, retried.uploaded)
        assertEquals(0, outbox.pendingCount())
        assertEquals(2, api.calls.size)
    }

    // ---------- 6. 达最大尝试次数 → 保留但不再自动重试 ----------

    @Test
    fun maxAttempts_stopsAutoRetry_butKeepsRow() = runBlocking {
        val api = FakeApi { false }
        val r = repo(api, CloudUploadRepository.Config(maxAttempts = 2, initialBackoffSeconds = 60L))
        outbox.enqueue("A1B2C3D4", 3000L, 25.0, 60.0)

        r.uploadPendingOnce()                      // attempts 1
        clock += 60
        r.uploadPendingOnce()                      // attempts 2 == max
        clock += 1800
        val third = r.uploadPendingOnce()
        assertEquals("no further auto retry", "empty", third.skipped)
        assertEquals(2, api.calls.size)
        val row = store.rows["A1B2C3D4" to 3000L]!!
        assertEquals(2, row.attempts)
        assertEquals("row kept for manual/next-run handling", 1, outbox.pendingCount())
    }

    // ---------- 7. 单批上限分批 ----------

    @Test
    fun batchLimit_splitsRequests_andProcessesAllRows() = runBlocking {
        val api = FakeApi { true }
        val r = repo(api, CloudUploadRepository.Config(batchLimit = 2))
        repeat(5) { outbox.enqueue("A1B2C3D4", 5000L + it, 20.0 + it, 50.0) }

        repeat(3) { r.uploadPendingOnce() }
        assertEquals(listOf(2, 2, 1), api.calls.map { it.readings.size })
        assertEquals(0, outbox.pendingCount())
        assertTrue("每批不超过上限", api.calls.all { it.readings.size <= 2 })
    }

    // ---------- 8. Api 抛异常视为失败，不抛出到调用方 ----------

    @Test
    fun apiException_isTreatedAsFailure_withoutThrowing() = runBlocking {
        val api = object : CloudUploadRepository.UploadApi {
            override suspend fun upload(data: SensorUploadData): Boolean = throw IOException("network down")
        }
        val r = CloudUploadRepository(api, outbox, gate, CloudUploadRepository.Config(), now)
        outbox.enqueue("A1B2C3D4", 6000L, 25.0, 60.0)

        val outcome = r.uploadPendingOnce()
        assertEquals("upload_failed", outcome.skipped)
        assertEquals(1, outcome.failed)
        assertEquals(1, store.rows["A1B2C3D4" to 6000L]!!.attempts)
    }

    // ---------- 9. 并发串行（同一时刻不产生并发重复请求） ----------

    @Test
    fun concurrentUploads_areSerialized() = runBlocking {
        val started = CompletableDeferred<Unit>()
        val release = CompletableDeferred<Boolean>()
        val calls = AtomicInteger(0)
        val api = object : CloudUploadRepository.UploadApi {
            override suspend fun upload(data: SensorUploadData): Boolean {
                calls.incrementAndGet()
                started.complete(Unit)
                return release.await()
            }
        }
        val scope = CoroutineScope(Dispatchers.Default)
        val r = CloudUploadRepository(api, outbox, gate, CloudUploadRepository.Config(), now, scope)
        outbox.enqueue("A1B2C3D4", 7000L, 25.0, 60.0)

        val first = scope.async { r.uploadPendingOnce() }
        started.await()                            // 第一次已在 Api 内挂起
        val second = r.uploadPendingOnce()         // 同一时刻第二次
        assertEquals("busy", second.skipped)
        assertEquals(0, second.attempted)
        release.complete(true)
        assertEquals(1, first.await().uploaded)
        assertEquals("only one HTTP request", 1, calls.get())
        assertEquals(0, outbox.pendingCount())
    }

    // ---------- 10. 入队不阻塞调用方（异步作用域） ----------

    @Test(timeout = 10_000)
    fun enqueueAndUpload_returnsWithoutWaitingForHttp() {
        val started = CompletableDeferred<Unit>()
        val release = CompletableDeferred<Boolean>()
        val api = object : CloudUploadRepository.UploadApi {
            override suspend fun upload(data: SensorUploadData): Boolean {
                started.complete(Unit)
                return release.await()
            }
        }
        val r = CloudUploadRepository(
            api, outbox, gate, CloudUploadRepository.Config(), now, CoroutineScope(Dispatchers.Default)
        )
        // 若实现为同步阻塞，此调用会在此等待 release —— 测试将超时失败
        assertTrue(r.enqueueAndUpload("A1B2C3D4", 8000L, 25.0, 60.0))
        assertFalse("HTTP 尚未完成", release.isCompleted)
        assertNotNull("入队已完成", store.rows["A1B2C3D4" to 8000L])

        runBlocking { withTimeout(5_000) { started.await() } }
        release.complete(true)
        runBlocking { withTimeout(5_000) { while (outbox.pendingCount() > 0) delay(10) } }
        assertEquals(0, outbox.pendingCount())
    }

    // ---------- 11. dueBatch 过滤与排序（策略层委派语义） ----------

    @Test
    fun dueBatch_filtersByAttemptsAndDueTime_andOrders() {
        clock = 100L
        store.rows["C" to 30L] = row("C", 30L, 1.0, 1.0).copy(nextAttemptAt = 500L, attempts = 0)
        store.rows["B" to 20L] = row("B", 20L, 1.0, 1.0).copy(nextAttemptAt = 100L, attempts = 0)
        store.rows["A" to 10L] = row("A", 10L, 1.0, 1.0).copy(nextAttemptAt = 100L, attempts = 0)
        store.rows["D" to 40L] = row("D", 40L, 1.0, 1.0).copy(nextAttemptAt = 50L, attempts = 5)

        // now=100：A/B 到期（同 nextAttemptAt，按 time 升序）；C 未到期；D attempts 超限
        assertEquals(listOf("A", "B"), outbox.dueBatch(maxAttempts = 3, limit = 10).map { it.devId })
        assertEquals(listOf("A"), outbox.dueBatch(maxAttempts = 3, limit = 1).map { it.devId })
        // 放宽 attempts 上限后 D 也到期，并按 nextAttemptAt 排在最前
        assertEquals(listOf("D", "A", "B"), outbox.dueBatch(maxAttempts = 6, limit = 10).map { it.devId })
        // 时间推进到 500 后 C 才到期
        clock = 500L
        assertEquals(listOf("A", "B", "C"), outbox.dueBatch(maxAttempts = 3, limit = 10).map { it.devId })
    }

    private fun row(devId: String, time: Long, t: Double, h: Double) =
        UploadOutboxRow(devId, time, t, h, attempts = 0, nextAttemptAt = 0L)
}
