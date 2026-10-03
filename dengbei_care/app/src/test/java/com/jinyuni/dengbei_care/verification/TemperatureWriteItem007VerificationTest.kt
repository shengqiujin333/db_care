package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.TemperatureDatabaseHelper
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-007）——宿主机可确定断言部分。
 *
 * 期望值由本文件按 AA-002 §5.4 / D-06 与任务描述独立写出（不复用被测常量作为期望）：
 * - 插入语句形状：`INSERT OR REPLACE INTO temperature (time, device_id, temperature, humidity) VALUES (?,?,?,?)`；
 * - 守护门控纯函数语义：仅 `storeflag == 1` 返回 true（覆盖负值/0/2/大值）；
 * - API 形状：单条写入方法存在（5 参数）且旧的列表式方法已不存在；
 * - 顶层包装函数同样为单条签名。
 *
 * 真实行级行为（time 无偏移、幂等、门控 0 行）见
 * `app/src/androidTest/.../TemperatureWriteItem007VerificationTest.kt` 与
 * `evidence/software_verify_timestamp_write_item007.py`。
 */
class TemperatureWriteItem007VerificationTest {

    private val expectedInsert =
        "INSERT OR REPLACE INTO temperature (time, device_id, temperature, humidity) VALUES (?, ?, ?, ?)"

    @Test
    fun insertStatement_shapeIsExplicitlyBound() {
        assertEquals(expectedInsert, TemperatureDatabaseHelper.SQL_INSERT_TEMPERATURE_READING)
    }

    @Test
    fun guardGate_isTrueOnlyForStoreflagOne() {
        val trueValues = (-1000..1000).filter { TemperatureDatabaseHelper.isGuardActive(it) }
        assertEquals("only storeflag == 1 may enable writing", listOf(1), trueValues)
        assertTrue(TemperatureDatabaseHelper.isGuardActive(1))
        for (v in listOf(-1, 0, 2, 3, 99, Int.MIN_VALUE, Int.MAX_VALUE)) {
            assertTrue("storeflag=$v must not write", !TemperatureDatabaseHelper.isGuardActive(v))
        }
    }

    @Test
    fun helperApi_isSingleReading_andLegacyListWriterIsGone() {
        val cls = TemperatureDatabaseHelper::class.java
        val single = cls.getDeclaredMethod(
            "storeTemperatureReading",
            String::class.java,
            Long::class.javaPrimitiveType,
            Double::class.javaPrimitiveType,
            Double::class.javaPrimitiveType,
            Int::class.javaPrimitiveType,
        )
        assertNotNull(single)
        assertEquals("returns rowId", Long::class.javaPrimitiveType, single.returnType)
        val legacy = cls.declaredMethods.filter { it.name == "storeTemperatureData" }
        assertTrue("legacy list writer must be gone, found=$legacy", legacy.isEmpty())
    }

    @Test
    fun topLevelFacade_exposesSingleReadingWriterOnly() {
        val facade = Class.forName("com.jinyuni.dengbei_care.TemperatureDatabaseHelperKt")
        val names = facade.declaredMethods.map { it.name }
        assertTrue("top-level storeTemperatureReading missing: $names", names.contains("storeTemperatureReading"))
        assertTrue("legacy top-level writer still present: $names", !names.contains("storeTemperatureData"))
    }
}
