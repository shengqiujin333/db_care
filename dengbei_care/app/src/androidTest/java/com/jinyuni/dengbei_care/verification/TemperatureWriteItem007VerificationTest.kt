package com.jinyuni.dengbei_care.verification

import android.content.Context
import android.database.sqlite.SQLiteDatabase
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.jinyuni.dengbei_care.TemperatureDatabaseHelper
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-007）——**真机 Android SQLite**。
 *
 * 用测试专用数据库直接调用被测单条写入 API，验证：写入行 `time` 恰为传入的真实采样时间（无 ±60 s 偏移）、
 * 值列不串位、同 `(dev_id, time)` 重复写入不增行、守护关闭（storeflag != 1）写入 0 行且返回 -1。
 * 覆盖 TD-SW-002 §4.2 T-SW-L0d-04 的设备端部分。
 */
@RunWith(AndroidJUnit4::class)
class TemperatureWriteItem007VerificationTest {

    private val ctx: Context get() = InstrumentationRegistry.getInstrumentation().targetContext
    private val dbName = "verify_item007.db"

    @Before
    fun clean() {
        ctx.deleteDatabase(dbName)
    }

    @After
    fun cleanUp() {
        ctx.deleteDatabase(dbName)
    }

    private fun open(): TemperatureDatabaseHelper =
        TemperatureDatabaseHelper(ctx, dbName, null, TemperatureDatabaseHelper.DATABASE_VERSION)

    private fun rows(db: SQLiteDatabase, devId: String): List<Triple<Long, Double, Double>> =
        db.rawQuery(
            "SELECT time, temperature, humidity FROM temperature WHERE device_id = ? ORDER BY time",
            arrayOf(devId)
        ).use { c ->
            val out = ArrayList<Triple<Long, Double, Double>>()
            while (c.moveToNext()) out.add(Triple(c.getLong(0), c.getDouble(1), c.getDouble(2)))
            out
        }

    @Test
    fun writtenTimeIsExactSampleTime_andValuesAreNotSwapped() {
        val helper = open()
        val db = helper.writableDatabase
        val dev = "A1B2C3D4"
        val sampleTime = 1_700_000_123L

        val rowId = helper.storeTemperatureReading(dev, sampleTime, 25.5, 60.25, 1)
        assertEquals("rowId should be a real insert id", true, rowId > 0)
        assertEquals(listOf(Triple(sampleTime, 25.5, 60.25)), rows(db, dev))
        // 显式排除 ±60 s / ±120 s 回填
        for (offset in listOf(60L, 120L, -60L)) {
            assertTrueFalse(
                "no backfilled row at time${if (offset > 0) "-" else "+"}${kotlin.math.abs(offset)}",
                rows(db, dev).none { it.first == sampleTime - offset }
            )
        }
        helper.close()
    }

    @Test
    fun sameDeviceAndTime_isIdempotent() {
        val helper = open()
        val db = helper.writableDatabase
        val dev = "A1B2C3D4"
        val t = 1_700_000_500L

        helper.storeTemperatureReading(dev, t, 25.0, 60.0, 1)
        helper.storeTemperatureReading(dev, t, 26.5, 61.5, 1)
        assertEquals("same (dev,time) must not duplicate", listOf(Triple(t, 26.5, 61.5)), rows(db, dev))
        // 不同时间 → 新行
        helper.storeTemperatureReading(dev, t + 180, 24.0, 58.0, 1)
        assertEquals(2, rows(db, dev).size)
        helper.close()
    }

    @Test
    fun guardOff_writesNothing_andReturnsMinusOne() {
        val helper = open()
        val db = helper.writableDatabase
        val dev = "0C118224"
        val t = 1_700_001_000L

        for (flag in listOf(0, 2, -1)) {
            val r = helper.storeTemperatureReading(dev, t, 30.0, 70.0, flag)
            assertEquals("storeflag=$flag must return -1", -1L, r)
        }
        assertEquals("guard off must write zero rows", emptyList<Triple<Long, Double, Double>>(), rows(db, dev))
        // 开启后同一时刻可正常写入
        assertEquals(true, helper.storeTemperatureReading(dev, t, 30.0, 70.0, 1) > 0)
        assertEquals(1, rows(db, dev).size)
        helper.close()
    }

    private fun assertTrueFalse(msg: String, cond: Boolean) = org.junit.Assert.assertTrue(msg, cond)
}
