package com.jinyuni.dengbei_care.cloud

import com.google.gson.Gson
import com.jinyuni.dengbei_care.telemetry.ReadingAttribution
import com.jinyuni.dengbei_care.ui.zhuce.ApiService
import com.jinyuni.dengbei_care.ui.zhuce.LoginData
import com.jinyuni.dengbei_care.ui.zhuce.MacData
import com.jinyuni.dengbei_care.ui.zhuce.RegistrationData
import com.jinyuni.dengbei_care.ui.zhuce.SensorReading
import com.jinyuni.dengbei_care.ui.zhuce.SensorUploadData
import com.jinyuni.dengbei_care.ui.zhuce.VerificationData
import com.jinyuni.dengbei_care.ui.zhuce.VerificationRequest
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import retrofit2.http.POST

/**
 * ITEM-008 实现侧自检：`/upload_data` 请求体构造与既有端点兼容面。
 *
 * 覆盖 TD-SW-002 §4.1 T-SW-L0-11（上传载荷构造）与 §4.2 T-SW-L0d-07（既有 5 个端点契约不变）。
 */
class UploadPayloadTest {

    private val gson = Gson()

    // ---------- 字段名与单位（序列化结果） ----------

    @Test
    fun serializedJson_hasExactContractFieldNames() {
        val payload = UploadPayload.fromAttributed(
            phone = "13800000000",
            mac = "1010100000A1",
            timeSeconds = 1699999999L,
            readings = listOf(
                ReadingAttribution.Reading("A1B2C3D4", 25.5, 60.0)
            )
        )!!
        val json = gson.toJson(payload)

        assertEquals(
            "{\"phone\":\"13800000000\",\"mac\":\"1010100000A1\"," +
                    "\"readings\":[{\"devId\":\"A1B2C3D4\",\"time\":1699999999," +
                    "\"temperature\":25.5,\"humidity\":60.0}]}",
            json
        )
    }

    @Test
    fun unitsAreCelsiusAndPercentRh_notRawX10() {
        val payload = UploadPayload.fromAttributed(
            phone = "13800000000",
            mac = "1010100000A1",
            timeSeconds = 1699999999L,
            // 线上原始值 ×10：255 → 25.5℃，600 → 60.0 %RH（换算在解码/归因阶段完成）
            readings = listOf(ReadingAttribution.Reading("A1B2C3D4", 255 / 10.0, 600 / 10.0))
        )!!
        val reading = payload.readings.single()
        assertEquals(25.5, reading.temperature, 0.0)
        assertEquals(60.0, reading.humidity, 0.0)
        // 负温必须保持负值，不得出现 ×10 或符号丢失
        assertEquals(-10.0, UploadPayload.fromAttributed(
            "1", "M", 1L, listOf(ReadingAttribution.Reading("A1B2C3D4", -100 / 10.0, 0.0))
        )!!.readings.single().temperature, 0.0)
    }

    @Test
    fun timeIsUnixSeconds_passedThroughUnchanged() {
        val payload = UploadPayload.fromAttributed(
            phone = "1", mac = "M", timeSeconds = 1700000001L,
            readings = listOf(ReadingAttribution.Reading("A1B2C3D4", 1.0, 1.0))
        )!!
        assertEquals(1700000001L, payload.readings.single().time)
    }

    // ---------- 归属来自记录自身 ----------

    @Test
    fun devIdComesFromAttributedReading_notFromIndex() {
        val readings = listOf(
            ReadingAttribution.Reading("33333333", 26.0, 61.0),
            ReadingAttribution.Reading("A1B2C3D4", 27.0, 62.0)
        )
        val payload = UploadPayload.fromAttributed("1", "M", 1700000002L, readings)!!
        assertEquals(listOf("33333333", "A1B2C3D4"), payload.readings.map { it.devId })
        assertEquals(listOf(26.0, 27.0), payload.readings.map { it.temperature })
    }

    @Test
    fun multipleReadings_shareBatchTime_andKeepOrder() {
        val payload = UploadPayload.fromAttributed(
            phone = "1", mac = "M", timeSeconds = 1700000003L,
            readings = listOf(
                ReadingAttribution.Reading("A1B2C3D4", 20.0, 50.0),
                ReadingAttribution.Reading("0C118224", 21.0, 51.0),
                ReadingAttribution.Reading("33333333", 22.0, 52.0)
            )
        )!!
        assertEquals(3, payload.readings.size)
        assertEquals(listOf(1700000003L, 1700000003L, 1700000003L), payload.readings.map { it.time })
        assertEquals(listOf("A1B2C3D4", "0C118224", "33333333"), payload.readings.map { it.devId })
    }

    // ---------- 空读数集不产生请求 ----------

    @Test
    fun emptyReadings_produceNoPayload() {
        assertNull(UploadPayload.fromAttributed("1", "M", 1700000004L, emptyList()))
        assertNull(UploadPayload.build("1", "M", emptyList()))
    }

    // ---------- 待发箱补传路径（逐行时间） ----------

    @Test
    fun build_keepsPerReadingTimes_forOutboxBackfill() {
        val payload = UploadPayload.build(
            phone = "1",
            mac = "M",
            readings = listOf(
                SensorReading("A1B2C3D4", 1700000005L, 20.0, 50.0),
                SensorReading("A1B2C3D4", 1700000185L, 19.0, 49.0)
            )
        )!!
        assertEquals(listOf(1700000005L, 1700000185L), payload.readings.map { it.time })
    }

    // ---------- DTO 字段名（与服务器契约逐字一致） ----------

    @Test
    fun dtoFieldNames_matchServerContract() {
        assertEquals(
            listOf("devId", "humidity", "temperature", "time"),
            SensorReading::class.java.declaredFields.map { it.name }.sorted()
        )
        assertEquals(
            listOf("mac", "phone", "readings"),
            SensorUploadData::class.java.declaredFields.map { it.name }.sorted()
        )
    }

    // ---------- 既有 5 个端点与数据类不变 ----------

    @Test
    fun apiService_keepsFiveExistingEndpoints_andAddsUploadData() {
        val paths = ApiService::class.java.declaredMethods
            .mapNotNull { it.getAnnotation(POST::class.java)?.value }
            .toSortedSet()
        assertEquals(
            sortedSetOf(
                "/sendVerificationCode",
                "/verifyCode",
                "/register",
                "/login",
                "/setmac",
                "/upload_data"
            ),
            paths
        )
    }

    @Test
    fun existingRequestDataClasses_areUnchanged() {
        assertEquals(listOf("phoneNumber"), VerificationRequest::class.java.declaredFields.map { it.name })
        assertEquals(
            listOf("code", "phoneNumber"),
            VerificationData::class.java.declaredFields.map { it.name }.sorted()
        )
        assertEquals(
            listOf("password", "phoneNumber"),
            RegistrationData::class.java.declaredFields.map { it.name }.sorted()
        )
        assertEquals(
            listOf("password", "phoneNumber"),
            LoginData::class.java.declaredFields.map { it.name }.sorted()
        )
        assertEquals(
            listOf("macAddress", "phoneNumber"),
            MacData::class.java.declaredFields.map { it.name }.sorted()
        )
    }

    @Test
    fun uploadDataEndpoint_usesJsonContentType() {
        val method = ApiService::class.java.declaredMethods
            .single { it.getAnnotation(POST::class.java)?.value == "/upload_data" }
        val headers = method.annotations
            .filterIsInstance<retrofit2.http.Headers>()
            .flatMap { it.value.toList() }
        assertTrue(headers.any { it.equals("Content-Type: application/json", ignoreCase = true) })
        assertTrue(method.parameterTypes.any { it.name == "com.jinyuni.dengbei_care.ui.zhuce.SensorUploadData" })
    }
}
