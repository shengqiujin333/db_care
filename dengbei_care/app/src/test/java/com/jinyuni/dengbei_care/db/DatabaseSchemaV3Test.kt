package com.jinyuni.dengbei_care.db

import com.jinyuni.dengbei_care.TemperatureDatabaseHelper
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * ITEM-006 实现侧自检：SQLite v3 schema 与迁移步骤（TemperatureDatabaseHelper）。
 *
 * 覆盖 TD-SW-002 §4.2 中可在宿主机确定断言的部分：
 * - v3 版本号与待发箱表/索引 DDL 的形状（表名、列、主键、索引列）；
 * - v2 既有 `temperature` / `alarm_events` 的 DDL 与索引**逐字未变**（结构保持不动）；
 * - v2→v3 迁移步骤只新建新表/索引，不含对既有表的 DROP/ALTER/UPDATE。
 *
 * 说明：真实设备上的迁移数据保真（T-SW-L0d-01/02/03）需要 Android 运行时，属软件测试能力执行范围。
 */
class DatabaseSchemaV3Test {

    private val helper = TemperatureDatabaseHelper

    // ---------- 版本与 v3 新表 ----------

    @Test
    fun databaseVersion_isThree() {
        assertEquals(3, TemperatureDatabaseHelper.DATABASE_VERSION)
    }

    @Test
    fun pendingUploadsTable_hasRequiredColumnsAndCompositePrimaryKey() {
        val sql = TemperatureDatabaseHelper.SQL_CREATE_TABLE_PENDING_UPLOADS
        assertTrue(sql.startsWith("CREATE TABLE IF NOT EXISTS pending_uploads ("))
        assertTrue(sql.contains("device_id TEXT"))
        assertTrue(sql.contains("time INTEGER"))
        assertTrue(sql.contains("temperature REAL"))
        assertTrue(sql.contains("humidity REAL"))
        assertTrue(sql.contains("attempts INTEGER DEFAULT 0"))
        assertTrue(sql.contains("next_attempt_at INTEGER DEFAULT 0"))
        assertTrue("composite PK must be (dev_id, time) for idempotent enqueue",
            sql.contains("PRIMARY KEY(device_id, time)"))
    }

    @Test
    fun pendingUploadsIndex_coversNextAttemptAt_andIsIdempotent() {
        val sql = TemperatureDatabaseHelper.SQL_CREATE_INDEX_PENDING_UPLOADS_NEXT
        assertTrue(sql.startsWith("CREATE INDEX IF NOT EXISTS idx_pending_uploads_next"))
        assertTrue(sql.endsWith("pending_uploads(next_attempt_at)"))
    }

    @Test
    fun tableAndColumnNames_areStable() {
        assertEquals("pending_uploads", TemperatureDatabaseHelper.TABLE_PENDING_UPLOADS)
        assertEquals("attempts", TemperatureDatabaseHelper.COLUMN_PENDING_ATTEMPTS)
        assertEquals("next_attempt_at", TemperatureDatabaseHelper.COLUMN_PENDING_NEXT_ATTEMPT_AT)
    }

    // ---------- v2 既有结构逐字未变 ----------

    @Test
    fun temperatureTable_ddlIsUnchangedFromV2() {
        assertEquals(
            "CREATE TABLE temperature (" +
                    "time INTEGER, device_id TEXT, temperature REAL, humidity REAL, " +
                    "PRIMARY KEY(time, device_id))",
            TemperatureDatabaseHelper.SQL_CREATE_TABLE_TEMPERATURE
        )
    }

    @Test
    fun alarmEventsTable_ddlIsUnchangedFromV2() {
        assertEquals(
            "CREATE TABLE alarm_events (" +
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER, device_id TEXT, " +
                    "type TEXT, message TEXT)",
            TemperatureDatabaseHelper.SQL_CREATE_TABLE_ALARM_EVENTS
        )
    }

    @Test
    fun existingIndexes_areUnchangedFromV2() {
        assertEquals(
            "CREATE INDEX idx_temperature_device ON temperature(device_id, time)",
            TemperatureDatabaseHelper.SQL_CREATE_INDEX_TEMPERATURE_DEVICE
        )
        assertEquals(
            "CREATE INDEX idx_alarm_device ON alarm_events(device_id, time)",
            TemperatureDatabaseHelper.SQL_CREATE_INDEX_ALARM_DEVICE
        )
    }

    // ---------- v2 → v3 迁移只新建新对象 ----------

    @Test
    fun migrationV2ToV3_onlyCreatesNewObjects() {
        val steps = TemperatureDatabaseHelper.MIGRATION_V2_TO_V3
        assertEquals(2, steps.size)
        assertEquals(TemperatureDatabaseHelper.SQL_CREATE_TABLE_PENDING_UPLOADS, steps[0])
        assertEquals(TemperatureDatabaseHelper.SQL_CREATE_INDEX_PENDING_UPLOADS_NEXT, steps[1])

        steps.forEach { sql ->
            val upper = sql.uppercase()
            assertFalse("migration must not drop existing tables: $sql", upper.contains("DROP TABLE"))
            assertFalse("migration must not alter existing tables: $sql", upper.contains("ALTER TABLE"))
            assertFalse("migration must not rewrite existing rows: $sql", upper.contains("UPDATE "))
            assertTrue(upper.contains("IF NOT EXISTS"))
        }
    }

    @Test
    fun migrationV2ToV3_neverReferencesLegacyTables() {
        val joined = TemperatureDatabaseHelper.MIGRATION_V2_TO_V3.joinToString(" ").uppercase()
        assertFalse(joined.contains("TEMPERATURE("))
        assertFalse(joined.contains("ALARM_EVENTS"))
        assertTrue(joined.contains("PENDING_UPLOADS"))
    }
}
