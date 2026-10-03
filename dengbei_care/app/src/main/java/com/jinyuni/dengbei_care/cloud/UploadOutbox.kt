package com.jinyuni.dengbei_care.cloud

import android.database.Cursor
import android.util.Log
import com.jinyuni.dengbei_care.TemperatureDatabaseHelper

/**
 * 云上传待发箱（outbox）——AA-002 §5.4/§5.5、D-07。
 *
 * 表由 `TemperatureDatabaseHelper`（DATABASE_VERSION = 3）建立：
 * `pending_uploads(device_id, time, temperature, humidity, attempts, next_attempt_at, PK(device_id, time))`。
 * 幂等键是 `(dev_id, time)`：同一设备同一采样时刻只入队一行；上传成功后删除；失败则保留并按
 * `attempts` / `next_attempt_at` 有界退避重试（参数集中在 [CloudUploadRepository.Config]）。
 *
 * 本文件分两层，便于宿主机独立验证：
 * - [UploadOutbox]：队列策略（幂等入队、按到期时间取批、确认删除、失败记账），只依赖 [UploadOutboxStore]；
 * - [SqliteUploadOutboxStore]：SQLite 适配器；表缺失或数据库异常时降级为空操作（不崩溃）。
 */
data class UploadOutboxRow(
    val devId: String,
    val time: Long,
    val temperature: Double,
    val humidity: Double,
    val attempts: Int,
    val nextAttemptAt: Long
)

/** 待发箱底层存取（可由内存实现替换以便独立测试） */
interface UploadOutboxStore {

    /** 幂等插入：已存在 `(devId, time)` 时返回 false 且不改动既有行 */
    fun insertIfAbsent(row: UploadOutboxRow): Boolean

    /** 取 `next_attempt_at <= now` 且 `attempts < maxAttempts` 的到期行，按 (next_attempt_at, time, devId) 升序 */
    fun dueRows(now: Long, maxAttempts: Int, limit: Int): List<UploadOutboxRow>

    fun delete(devId: String, time: Long)

    fun updateRetry(devId: String, time: Long, attempts: Int, nextAttemptAt: Long)

    fun count(): Int
}

/** 队列策略（纯逻辑，时间可注入） */
class UploadOutbox(
    private val store: UploadOutboxStore,
    private val now: () -> Long = { System.currentTimeMillis() / 1000 }
) {

    /** 幂等入队：返回 true 表示本次新入队；已存在返回 false（不重复行） */
    fun enqueue(devId: String, time: Long, temperature: Double, humidity: Double): Boolean =
        store.insertIfAbsent(
            UploadOutboxRow(
                devId = devId,
                time = time,
                temperature = temperature,
                humidity = humidity,
                attempts = 0,
                nextAttemptAt = 0L
            )
        )

    /** 取一批到期行（`attempts >= maxAttempts` 的行不再自动重试，保留在待发箱） */
    fun dueBatch(maxAttempts: Int, limit: Int): List<UploadOutboxRow> =
        store.dueRows(now(), maxAttempts, limit)

    /** 上传成功：删除该行 */
    fun ack(row: UploadOutboxRow) = store.delete(row.devId, row.time)

    /** 上传失败：`attempts + 1`，并按 `backoffSeconds` 推后下次尝试时间；返回更新后的行 */
    fun markFailed(row: UploadOutboxRow, backoffSeconds: Long): UploadOutboxRow {
        val attempts = row.attempts + 1
        val nextAttemptAt = now() + backoffSeconds
        store.updateRetry(row.devId, row.time, attempts, nextAttemptAt)
        return row.copy(attempts = attempts, nextAttemptAt = nextAttemptAt)
    }

    fun pendingCount(): Int = store.count()
}

/**
 * SQLite 适配器（生产实现）。
 * 所有操作容错：表缺失（迁移异常回退后）或数据库异常时记录日志并返回安全默认值，不抛到调用方。
 */
class SqliteUploadOutboxStore(
    private val helper: TemperatureDatabaseHelper
) : UploadOutboxStore {

    private val table = TemperatureDatabaseHelper.TABLE_PENDING_UPLOADS
    private val colDevId = TemperatureDatabaseHelper.COLUMN_DEVICE_ID
    private val colTime = TemperatureDatabaseHelper.COLUMN_TIME
    private val colTemp = TemperatureDatabaseHelper.COLUMN_TEMPERATURE
    private val colHumi = TemperatureDatabaseHelper.COLUMN_HUMIDITY
    private val colAttempts = TemperatureDatabaseHelper.COLUMN_PENDING_ATTEMPTS
    private val colNext = TemperatureDatabaseHelper.COLUMN_PENDING_NEXT_ATTEMPT_AT

    override fun insertIfAbsent(row: UploadOutboxRow): Boolean = runCatching {
        val db = helper.writableDatabase
        db.beginTransaction()
        try {
            if (exists(db, row.devId, row.time)) {
                db.setTransactionSuccessful()
                return@runCatching false
            }
            db.execSQL(
                "INSERT OR IGNORE INTO $table ($colDevId, $colTime, $colTemp, $colHumi, $colAttempts, $colNext) " +
                        "VALUES (?, ?, ?, ?, 0, 0)",
                arrayOf(row.devId, row.time, row.temperature, row.humidity)
            )
            db.setTransactionSuccessful()
            true
        } finally {
            db.endTransaction()
        }
    }.getOrElse {
        Log.w(TAG, "outbox insert failed (${it.message}); treating as not enqueued")
        false
    }

    override fun dueRows(now: Long, maxAttempts: Int, limit: Int): List<UploadOutboxRow> = runCatching {
        val rows = ArrayList<UploadOutboxRow>()
        helper.readableDatabase.rawQuery(
            "SELECT $colDevId, $colTime, $colTemp, $colHumi, $colAttempts, $colNext FROM $table " +
                    "WHERE $colNext <= ? AND $colAttempts < ? " +
                    "ORDER BY $colNext ASC, $colTime ASC, $colDevId ASC LIMIT ?",
            arrayOf(now.toString(), maxAttempts.toString(), limit.toString())
        ).use { c: Cursor ->
            while (c.moveToNext()) {
                rows += UploadOutboxRow(
                    devId = c.getString(0),
                    time = c.getLong(1),
                    temperature = c.getDouble(2),
                    humidity = c.getDouble(3),
                    attempts = c.getInt(4),
                    nextAttemptAt = c.getLong(5)
                )
            }
        }
        rows
    }.getOrElse {
        Log.w(TAG, "outbox query failed (${it.message}); returning empty batch")
        emptyList()
    }

    override fun delete(devId: String, time: Long) {
        runCatching {
            helper.writableDatabase.execSQL(
                "DELETE FROM $table WHERE $colDevId = ? AND $colTime = ?",
                arrayOf(devId, time)
            )
        }.onFailure { Log.w(TAG, "outbox delete failed (${it.message})") }
    }

    override fun updateRetry(devId: String, time: Long, attempts: Int, nextAttemptAt: Long) {
        runCatching {
            helper.writableDatabase.execSQL(
                "UPDATE $table SET $colAttempts = ?, $colNext = ? WHERE $colDevId = ? AND $colTime = ?",
                arrayOf(attempts, nextAttemptAt, devId, time)
            )
        }.onFailure { Log.w(TAG, "outbox retry update failed (${it.message})") }
    }

    override fun count(): Int = runCatching {
        helper.readableDatabase.rawQuery("SELECT COUNT(*) FROM $table", null).use { c ->
            if (c.moveToFirst()) c.getInt(0) else 0
        }
    }.getOrElse {
        Log.w(TAG, "outbox count failed (${it.message})")
        0
    }

    private fun exists(
        db: android.database.sqlite.SQLiteDatabase,
        devId: String,
        time: Long
    ): Boolean = db.rawQuery(
        "SELECT 1 FROM $table WHERE $colDevId = ? AND $colTime = ? LIMIT 1",
        arrayOf(devId, time.toString())
    ).use { it.moveToFirst() }

    private companion object {
        const val TAG = "UploadOutbox"
    }
}
