package com.jinyuni.dengbei_care.verification

import android.content.Context
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.jinyuni.dengbei_care.TemperatureDatabaseHelper
import com.jinyuni.dengbei_care.cloud.SqliteUploadOutboxStore
import com.jinyuni.dengbei_care.cloud.UploadOutbox
import com.jinyuni.dengbei_care.cloud.UploadOutboxRow
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-009）——**真机 Android SQLite**。
 *
 * 直接用测试专用数据库验证 `SqliteUploadOutboxStore` 的 SQL 语义（幂等插入 / 到期过滤与排序 / LIMIT /
 * 删除 / 重试记账 / 表缺失降级）以及 `UploadOutbox` 策略在真实存储上的幂等入队与 ack。
 * 覆盖 TD-SW-002 §4.1 T-SW-L0-12（①④⑦）的设备端部分。
 */
@RunWith(AndroidJUnit4::class)
class UploadOutboxItem009VerificationTest {

    private val ctx: Context get() = InstrumentationRegistry.getInstrumentation().targetContext
    private val dbName = "verify_item009.db"
    private var helper: TemperatureDatabaseHelper? = null

    @Before
    fun setUp() {
        ctx.deleteDatabase(dbName)
    }

    @After
    fun tearDown() {
        helper?.close()
        ctx.deleteDatabase(dbName)
    }

    private fun store(): SqliteUploadOutboxStore {
        val h = TemperatureDatabaseHelper(ctx, dbName, null, TemperatureDatabaseHelper.DATABASE_VERSION)
        helper = h
        return SqliteUploadOutboxStore(h)
    }

    private fun row(devId: String, time: Long, t: Double = 25.0, h: Double = 60.0, attempts: Int = 0, next: Long = 0L) =
        UploadOutboxRow(devId, time, t, h, attempts, next)

    @Test
    fun insertIfAbsent_isIdempotentOnRealSqlite() {
        val s = store()
        assertTrue(s.insertIfAbsent(row("A1B2C3D4", 100L, 25.5, 60.0)))
        assertFalse("duplicate (devId,time) must be rejected", s.insertIfAbsent(row("A1B2C3D4", 100L, 99.0, 99.0)))
        assertEquals(1, s.count())
        // 首次入队值保留
        val stored = s.dueRows(now = 100L, maxAttempts = 20, limit = 10).single()
        assertEquals(25.5, stored.temperature, 0.0)
        assertEquals(60.0, stored.humidity, 0.0)
        // 不同 time / 不同设备 → 新行
        assertTrue(s.insertIfAbsent(row("A1B2C3D4", 101L)))
        assertTrue(s.insertIfAbsent(row("0C118224", 100L)))
        assertEquals(3, s.count())
    }

    @Test
    fun dueRows_filtersByDueTimeAndAttempts_ordersAndLimits() {
        val s = store()
        s.insertIfAbsent(row("C", 30L, next = 500L))
        s.insertIfAbsent(row("B", 20L, next = 100L))
        s.insertIfAbsent(row("A", 10L, next = 100L))
        s.insertIfAbsent(row("D", 40L, attempts = 5, next = 50L))

        // now=100：A/B 到期（同 next，按 time 升序）；C 未到期；D attempts 超限
        assertEquals(listOf("A", "B"), s.dueRows(100L, 3, 10).map { it.devId })
        assertEquals(listOf("A"), s.dueRows(100L, 3, 1).map { it.devId })
        assertEquals(listOf("D", "A", "B"), s.dueRows(100L, 6, 10).map { it.devId })
        assertEquals(listOf("A", "B", "C"), s.dueRows(500L, 3, 10).map { it.devId })
    }

    @Test
    fun updateRetryAndDelete_affectOnlyTargetRow() {
        val s = store()
        s.insertIfAbsent(row("A1B2C3D4", 1L))
        s.insertIfAbsent(row("A1B2C3D4", 2L))
        s.updateRetry("A1B2C3D4", 1L, attempts = 3, nextAttemptAt = 900L)

        assertEquals(listOf("A1B2C3D4" to 2L), s.dueRows(1000L, 20, 10).map { it.devId to it.time })
        val retried = s.dueRows(1000L, 20, 10).single()
        assertEquals(0, retried.attempts) // 目标行已不在到期集合（未达 next=900 前）
        s.delete("A1B2C3D4", 2L)
        assertEquals(1, s.count())
        s.delete("A1B2C3D4", 1L)
        assertEquals(0, s.count())
    }

    @Test
    fun policyOverSqlite_enqueuesOnce_acksAndAccountsFailure() {
        val s = store()
        var clock = 1_700_000_000L
        val outbox = UploadOutbox(s) { clock }

        assertTrue(outbox.enqueue("A1B2C3D4", 1000L, 25.0, 60.0))
        assertFalse("幂等：同 (devId,time) 不再入队", outbox.enqueue("A1B2C3D4", 1000L, 26.0, 61.0))
        assertEquals(1, outbox.pendingCount())

        val due = outbox.dueBatch(maxAttempts = 20, limit = 50).single()
        val failed = outbox.markFailed(due, backoffSeconds = 60L)
        assertEquals(1, failed.attempts)
        assertEquals(clock + 60L, failed.nextAttemptAt)
        assertTrue("退避期内不再到期", outbox.dueBatch(20, 50).isEmpty())

        clock += 60L
        val again = outbox.dueBatch(20, 50).single()
        assertEquals(1, again.attempts)
        outbox.ack(again)
        assertEquals(0, outbox.pendingCount())
    }

    @Test
    fun missingTable_degradesToNoOp_withoutCrash() {
        val s = store()
        helper!!.writableDatabase.execSQL("DROP TABLE IF EXISTS pending_uploads")
        // 全部操作必须降级而非抛异常
        assertFalse("table missing → not enqueued", s.insertIfAbsent(row("A1B2C3D4", 1L)))
        assertTrue(s.dueRows(100L, 20, 10).isEmpty())
        assertEquals(0, s.count())
        s.delete("A1B2C3D4", 1L)
        s.updateRetry("A1B2C3D4", 1L, 1, 2L)
    }
}
