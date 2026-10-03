package com.jinyuni.dengbei_care.db

import com.jinyuni.dengbei_care.TemperatureDatabaseHelper
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * ITEM-007 实现侧自检：单条读数写入的时间戳与幂等语义（TemperatureDatabaseHelper）。
 *
 * 覆盖 TD-SW-002 §4.2 中可在宿主机确定断言的部分：
 * - 写入语句为 `INSERT OR REPLACE` + `(time, device_id)` 复合主键 ⇒ 同一 (dev_id, time) 重复写入不产生重复行；
 * - 绑定参数列序为 (time, device_id, temperature, humidity)，即写入行的 time 就是调用方传入的真实采样时间（无偏移回填）；
 * - 守护门控 `storeflag == 1` 才允许写入（其它值不写任何行）。
 *
 * 说明：真实行内容与行数（含重复写入只保留一行）需要 Android 运行时，属软件测试能力 T-SW-L0d-04 执行范围。
 */
class TemperatureWriteItem007Test {

    @Test
    fun insertStatement_isReplaceInto_withFourBoundColumns() {
        val sql = TemperatureDatabaseHelper.SQL_INSERT_TEMPERATURE_READING
        assertTrue(sql.startsWith("INSERT OR REPLACE INTO temperature"))
        assertEquals(
            "INSERT OR REPLACE INTO temperature (time, device_id, temperature, humidity) VALUES (?, ?, ?, ?)",
            sql
        )
        assertEquals("must bind exactly 4 values", 4, sql.count { it == '?' })
    }

    @Test
    fun idempotency_isGuaranteedByCompositePrimaryKeyAndReplace() {
        // 幂等键：复合主键 (time, device_id) —— 与写入语句的 OR REPLACE 组合
        assertTrue(
            TemperatureDatabaseHelper.SQL_CREATE_TABLE_TEMPERATURE
                .contains("PRIMARY KEY(time, device_id)")
        )
        assertTrue(
            TemperatureDatabaseHelper.SQL_INSERT_TEMPERATURE_READING.contains("INSERT OR REPLACE")
        )
    }

    @Test
    fun writtenTimeIsTheCallerProvidedSampleTime_noOffsetBackfill() {
        val sql = TemperatureDatabaseHelper.SQL_INSERT_TEMPERATURE_READING
        // 时间列是普通占位符绑定，语句内不存在任何偏移/回填表达式
        assertFalse(sql.contains("60"))
        assertFalse(sql.contains("- "))
        assertFalse(sql.contains("+ "))
        assertTrue(sql.contains("(time, device_id, temperature, humidity)"))
    }

    @Test
    fun guardGate_onlyStoreflagOneAllowsWriting() {
        assertTrue(TemperatureDatabaseHelper.isGuardActive(1))
        assertFalse(TemperatureDatabaseHelper.isGuardActive(0))
        assertFalse(TemperatureDatabaseHelper.isGuardActive(-1))
        assertFalse(TemperatureDatabaseHelper.isGuardActive(2))
    }
}
