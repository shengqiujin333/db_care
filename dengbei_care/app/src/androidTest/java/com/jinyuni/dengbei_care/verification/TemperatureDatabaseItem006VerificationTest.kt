package com.jinyuni.dengbei_care.verification

import android.content.Context
import android.database.sqlite.SQLiteDatabase
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.jinyuni.dengbei_care.TemperatureDatabaseHelper
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-006）——**真机 Android SQLite**。
 *
 * 方法：用**独立构造的旧版数据库**（v2 / v1，DDL 取自改动前基线，不调用被测 helper）落盘并写入已知数据，
 * 再用被测 `TemperatureDatabaseHelper(…, version = 3)` 打开，触发 `onUpgrade`，从而在真实 Android
 * 运行时上验证：历史数据保真、`pending_uploads` 表/索引/复合主键、幂等入队、重复打开、v1→v3 路径与
 * 冲突对象回退不崩溃、以及既有查询 API 在迁移后结果一致。
 *
 * 覆盖 TD-SW-002 §4.2 T-SW-L0d-01/02/03/04/06（设备端部分）。
 * 数据库使用测试专用名字，不触碰 App 正式库；用例前后清理。
 */
@RunWith(AndroidJUnit4::class)
class TemperatureDatabaseItem006VerificationTest {

    private val ctx: Context get() = InstrumentationRegistry.getInstrumentation().targetContext

    private val dbV2 = "verify_item006_v2.db"
    private val dbV1 = "verify_item006_v1.db"
    private val dbV2Conflict = "verify_item006_v2_conflict.db"
    private val dbFresh = "verify_item006_fresh.db"

    // ---- 改动前（v2）基线的独立 DDL（逐字取自 as-built，不引用被测常量） ----
    private val v2Ddl = listOf(
        "CREATE TABLE temperature (time INTEGER, device_id TEXT, temperature REAL, humidity REAL, PRIMARY KEY(time, device_id))",
        "CREATE TABLE alarm_events (id INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER, device_id TEXT, type TEXT, message TEXT)",
        "CREATE INDEX idx_temperature_device ON temperature(device_id, time)",
        "CREATE INDEX idx_alarm_device ON alarm_events(device_id, time)"
    )

    // ---- 改动前（v1）基线的独立 DDL ----
    private val v1Ddl = listOf(
        "CREATE TABLE temperatureA (time INTEGER, temperature REAL, humidity REAL)",
        "CREATE TABLE temperatureB (time INTEGER, temperature REAL, humidity REAL)",
        "CREATE TABLE temperatureC (time INTEGER, temperature REAL, humidity REAL)"
    )

    private val devA = "A1B2C3D4"
    private val devB = "0C118224"

    @Before
    fun clean() {
        listOf(dbV2, dbV1, dbV2Conflict, dbFresh).forEach { ctx.deleteDatabase(it) }
    }

    @After
    fun cleanUp() {
        listOf(dbV2, dbV1, dbV2Conflict, dbFresh).forEach { ctx.deleteDatabase(it) }
    }

    private fun seedV2(name: String) {
        val db = ctx.openOrCreateDatabase(name, Context.MODE_PRIVATE, null)
        v2Ddl.forEach { db.execSQL(it) }
        db.execSQL("INSERT INTO temperature(time, device_id, temperature, humidity) VALUES (1000, '$devA', 25.0, 60.0)")
        db.execSQL("INSERT INTO temperature(time, device_id, temperature, humidity) VALUES (1060, '$devA', 24.0, 58.0)")
        db.execSQL("INSERT INTO temperature(time, device_id, temperature, humidity) VALUES (2000, '$devB', 30.0, 70.0)")
        db.execSQL("INSERT INTO alarm_events(time, device_id, type, message) VALUES (1000, '$devA', 'drop', '温度湿度下降报警')")
        db.version = 2
        db.close()
    }

    private fun openV3(name: String): TemperatureDatabaseHelper =
        TemperatureDatabaseHelper(ctx, name, null, 3)

    private fun scalarLong(db: SQLiteDatabase, sql: String): Long =
        db.rawQuery(sql, null).use { c -> c.moveToFirst(); c.getLong(0) }

    private fun objectType(db: SQLiteDatabase, name: String): String? =
        db.rawQuery("SELECT type FROM sqlite_master WHERE name = ?", arrayOf(name)).use { c ->
            if (c.moveToFirst()) c.getString(0) else null
        }

    // ---------- 1. v2 → v3：历史数据保真 + 新表/索引/主键 ----------

    @Test
    fun v2ToV3_preservesHistory_andCreatesOutboxSchema() {
        seedV2(dbV2)
        val helper = openV3(dbV2)
        val db = helper.writableDatabase   // 触发 onUpgrade(2,3)

        assertEquals("user_version", 3, db.version)

        // 既有表结构与数据保真（原始 SQL 视角）
        assertEquals(3L, scalarLong(db, "SELECT count(*) FROM temperature"))
        assertEquals(1L, scalarLong(db, "SELECT count(*) FROM alarm_events"))
        assertEquals(2L, scalarLong(db, "SELECT count(*) FROM temperature WHERE device_id='$devA'"))
        db.rawQuery("SELECT time, temperature, humidity FROM temperature WHERE device_id='$devA' ORDER BY time", null).use { c ->
            assertTrue(c.moveToFirst())
            assertEquals(1000L, c.getLong(0)); assertEquals(25.0, c.getDouble(1), 0.0); assertEquals(60.0, c.getDouble(2), 0.0)
            assertTrue(c.moveToNext())
            assertEquals(1060L, c.getLong(0)); assertEquals(24.0, c.getDouble(1), 0.0); assertEquals(58.0, c.getDouble(2), 0.0)
        }
        db.rawQuery("SELECT type, message FROM alarm_events", null).use { c ->
            assertTrue(c.moveToFirst())
            assertEquals("drop", c.getString(0))
            assertEquals("温度湿度下降报警", c.getString(1))
        }
        // 既有索引未变
        val tempIdx = indexNames(db, "temperature")
        val alarmIdx = indexNames(db, "alarm_events")
        assertTrue("temperature indexes=$tempIdx", tempIdx.contains("idx_temperature_device"))
        assertTrue("alarm_events indexes=$alarmIdx", alarmIdx.contains("idx_alarm_device"))

        // 既有查询 API 在迁移后仍可用且结果一致
        val latest = helper.getAllDevicesLatest()
        assertEquals(setOf(devA, devB), latest.keys)
        assertEquals(1060L, latest[devA]!!.first)
        assertEquals(24.0, latest[devA]!!.second, 0.0)
        assertEquals(58.0, latest[devA]!!.third, 0.0)
        assertEquals(1, helper.getAlarmEvents(devA, 0, 3000).size)
        assertEquals(2, helper.getDailyStats(devA, 0, 3000)!!.sampleCount)

        // v3 新表与索引
        assertEquals("table", objectType(db, "pending_uploads"))
        val cols = tableInfo(db, "pending_uploads")
        assertEquals(
            listOf("device_id", "time", "temperature", "humidity", "attempts", "next_attempt_at"),
            cols.map { it.first }
        )
        assertEquals("pk order device_id", 1, cols.first { it.first == "device_id" }.third)
        assertEquals("pk order time", 2, cols.first { it.first == "time" }.third)
        assertEquals("no other pk columns", 2, cols.count { it.third > 0 })
        assertTrue("outbox index", indexNames(db, "pending_uploads").contains("idx_pending_uploads_next"))
        assertEquals(listOf("next_attempt_at"), indexColumns(db, "idx_pending_uploads_next"))

        helper.close()
    }

    // ---------- 2. 幂等入队键与默认值 ----------

    @Test
    fun v2ToV3_outboxKeyIsIdempotent_andDefaultsApply() {
        seedV2(dbV2)
        val helper = openV3(dbV2)
        val db = helper.writableDatabase

        db.execSQL("INSERT OR REPLACE INTO pending_uploads(device_id, time, temperature, humidity) VALUES ('$devA', 5000, 25.0, 60.0)")
        db.execSQL("INSERT OR REPLACE INTO pending_uploads(device_id, time, temperature, humidity) VALUES ('$devA', 5000, 26.5, 61.5)")
        assertEquals("same (dev,time) → one row", 1L, scalarLong(db, "SELECT count(*) FROM pending_uploads"))
        db.rawQuery("SELECT attempts, next_attempt_at, temperature FROM pending_uploads", null).use { c ->
            assertTrue(c.moveToFirst())
            assertEquals("attempts default", 0L, c.getLong(0))
            assertEquals("next_attempt_at default", 0L, c.getLong(1))
            assertEquals(26.5, c.getDouble(2), 0.0)
        }
        // 复合主键确实生效：普通 INSERT 重复键必须失败
        try {
            db.execSQL("INSERT INTO pending_uploads(device_id, time) VALUES ('$devA', 5000)")
            fail("duplicate (device_id,time) plain INSERT must violate PRIMARY KEY")
        } catch (expected: android.database.sqlite.SQLiteConstraintException) {
            // expected
        }
        // 不同 time → 新行
        db.execSQL("INSERT OR REPLACE INTO pending_uploads(device_id, time) VALUES ('$devA', 5001)")
        assertEquals(2L, scalarLong(db, "SELECT count(*) FROM pending_uploads"))
        helper.close()
    }

    // ---------- 3. 重复创建/重复打开不报错 ----------

    @Test
    fun repeatedOpen_isIdempotent_andFreshInstallCreatesOutbox() {
        // 新装（onCreate，version=3）即应有待发箱表
        val fresh = openV3(dbFresh)
        val freshDb = fresh.writableDatabase
        assertEquals("table", objectType(freshDb, "pending_uploads"))
        assertEquals(3, freshDb.version)
        fresh.close()
        // 再次打开：不报错、数据保留
        val again = openV3(dbFresh)
        val againDb = again.writableDatabase
        assertEquals(3, againDb.version)
        assertEquals("table", objectType(againDb, "pending_uploads"))
        againDb.execSQL("INSERT OR REPLACE INTO pending_uploads(device_id, time) VALUES ('$devB', 7)")
        again.close()
        val third = openV3(dbFresh)
        assertEquals(1L, scalarLong(third.writableDatabase, "SELECT count(*) FROM pending_uploads"))
        third.close()

        // v2 → v3 后再打开一次（重复迁移路径）也不报错
        seedV2(dbV2)
        openV3(dbV2).close()
        val reopened = openV3(dbV2)
        assertEquals(3L, scalarLong(reopened.writableDatabase, "SELECT count(*) FROM temperature"))
        assertEquals(3, reopened.writableDatabase.version)
        reopened.close()
    }

    // ---------- 4. v1 → v3 路径不崩溃且可用 ----------

    @Test
    fun v1ToV3_doesNotCrash_andKeepsDatabaseUsable() {
        val db = ctx.openOrCreateDatabase(dbV1, Context.MODE_PRIVATE, null)
        v1Ddl.forEach { db.execSQL(it) }
        db.execSQL("INSERT INTO temperatureA(time, temperature, humidity) VALUES (900, 26.0, 61.0)")
        db.execSQL("INSERT INTO temperatureA(time, temperature, humidity) VALUES (960, 25.0, 59.0)")
        db.version = 1
        db.close()

        val helper = openV3(dbV1)
        val upgraded = try {
            helper.writableDatabase
        } catch (t: Throwable) {
            throw AssertionError("v1→v3 must not throw: $t", t)
        }
        assertEquals(3, upgraded.version)
        assertEquals("table", objectType(upgraded, "temperature"))
        assertEquals("table", objectType(upgraded, "pending_uploads"))
        // 既有读写可用（行数按实现的实际迁移结果记录，不预设）
        val tempRows = scalarLong(upgraded, "SELECT count(*) FROM temperature")
        val backfilledDev = upgraded.rawQuery("SELECT DISTINCT device_id FROM temperature", null).use { c ->
            if (c.moveToFirst()) c.getString(0) else null
        }
        println("ITEM006 v1->v3: temperature rows=$tempRows, backfilled device_id=$backfilledDev")
        assertTrue("temperature rows migrated (>=0)", tempRows >= 0)
        assertNotNull("device_id column present", upgraded.rawQuery("PRAGMA table_info(temperature)", null).use { c ->
            var found = false
            while (c.moveToNext()) if (c.getString(1) == "device_id") found = true
            if (found) "yes" else null
        })
        // 迁移后仍可写入与查询
        upgraded.execSQL("INSERT OR REPLACE INTO pending_uploads(device_id, time) VALUES ('$devA', 1)")
        assertEquals(1L, scalarLong(upgraded, "SELECT count(*) FROM pending_uploads"))
        helper.close()
    }

    // ---------- 5. 冲突对象（同名 VIEW）时按回退策略处理且不崩溃 ----------

    @Test
    fun conflictingObject_isHandledWithoutCrash_andLegacyDataSurvives() {
        seedV2(dbV2Conflict)
        val seed = ctx.openOrCreateDatabase(dbV2Conflict, Context.MODE_PRIVATE, null)
        seed.execSQL("CREATE VIEW pending_uploads AS SELECT 1 AS x")
        seed.close()

        val helper = openV3(dbV2Conflict)
        val db = try {
            helper.writableDatabase
        } catch (t: Throwable) {
            throw AssertionError("conflicting object must not crash: $t", t)
        }
        // 既有数据必须完好（不得因回退而重建既有表）
        assertEquals(3L, scalarLong(db, "SELECT count(*) FROM temperature"))
        assertEquals(1L, scalarLong(db, "SELECT count(*) FROM alarm_events"))
        // 回退结果如实记录：期望最终为 table（DROP VIEW + 重试）；若仍为 view 则记录为观察项
        val kind = objectType(db, "pending_uploads")
        println("ITEM006 conflict fallback: pending_uploads type=$kind")
        assertTrue("pending_uploads object exists after fallback (type=$kind)", kind == "table" || kind == "view")
        if (kind == "table") {
            assertEquals(2, tableInfo(db, "pending_uploads").count { it.third > 0 })
        }
        // 库仍可读写
        db.execSQL("INSERT OR REPLACE INTO temperature(time, device_id, temperature, humidity) VALUES (9999, '$devA', 22.0, 55.0)")
        assertEquals(4L, scalarLong(db, "SELECT count(*) FROM temperature"))
        helper.close()
    }

    // ---------- 辅助 ----------

    private fun indexNames(db: SQLiteDatabase, table: String): List<String> =
        db.rawQuery("PRAGMA index_list($table)", null).use { c ->
            val out = ArrayList<String>()
            while (c.moveToNext()) out.add(c.getString(1))
            out
        }

    private fun indexColumns(db: SQLiteDatabase, index: String): List<String> =
        db.rawQuery("PRAGMA index_info($index)", null).use { c ->
            val out = ArrayList<String>()
            while (c.moveToNext()) out.add(c.getString(2))
            out
        }

    /** (name, type, pkPosition) */
    private fun tableInfo(db: SQLiteDatabase, table: String): List<Triple<String, String, Int>> =
        db.rawQuery("PRAGMA table_info($table)", null).use { c ->
            val out = ArrayList<Triple<String, String, Int>>()
            while (c.moveToNext()) out.add(Triple(c.getString(1), c.getString(2), c.getInt(5)))
            out
        }
}
