package com.jinyuni.dengbei_care.verification

import com.google.gson.Gson
import com.google.gson.JsonParser
import com.jinyuni.dengbei_care.cloud.UploadPayload
import com.jinyuni.dengbei_care.telemetry.ReadingAttribution
import com.jinyuni.dengbei_care.ui.zhuce.ApiService
import com.jinyuni.dengbei_care.ui.zhuce.LoginData
import com.jinyuni.dengbei_care.ui.zhuce.LoginResponse
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
import retrofit2.http.Headers
import retrofit2.http.POST

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-008）。
 *
 * 期望值全部由本文件按 `服务器迁移记录.md` §八 / AA-002 §5.5 独立写出（不复用被测常量）：
 * - `/upload_data` 请求体的**逐字 JSON**（字段名、单位、time 为 Unix 秒）；
 * - 结构校验（key 集合与 JSON 类型，不依赖格式化细节）；
 * - 单位不二次换算（传入已是 ℃/%RH 时原样输出；负温保号）；
 * - `devId` 来自读数自身而非列表下标；
 * - DTO 字段名/类型 + `ApiService` 6 个端点（既有 5 个路径/参数/返回不变，新增 `/upload_data` 带 JSON 头）。
 */
class UploadPayloadItem008VerificationTest {

    private val gson = Gson()

    private fun reading(devId: String, t: Double, h: Double) =
        ReadingAttribution.Reading(devId = devId, temperature = t, humidity = h)

    // ---------- 1. 逐字 JSON 契约 ----------

    @Test
    fun serializedJson_isExactlyTheServerContract() {
        val payload = UploadPayload.fromAttributed(
            phone = "13800000000",
            mac = "1010100000A1",
            timeSeconds = 1699999999L,
            readings = listOf(reading("A1B2C3D4", 25.5, 60.0))
        )!!
        assertEquals(
            "{\"phone\":\"13800000000\",\"mac\":\"1010100000A1\"," +
                "\"readings\":[{\"devId\":\"A1B2C3D4\",\"time\":1699999999," +
                "\"temperature\":25.5,\"humidity\":60.0}]}",
            gson.toJson(payload)
        )
    }

    @Test
    fun jsonStructure_hasExactKeysAndTypes() {
        val payload = UploadPayload.fromAttributed(
            "13900000001", "AABBCCDDEEFF", 1700000000L,
            listOf(reading("0C118224", -3.2, 0.0))
        )!!
        val root = JsonParser.parseString(gson.toJson(payload)).asJsonObject
        assertEquals(setOf("phone", "mac", "readings"), root.keySet())
        assertEquals("13900000001", root.get("phone").asString)
        assertEquals("AABBCCDDEEFF", root.get("mac").asString)
        val arr = root.getAsJsonArray("readings")
        assertEquals(1, arr.size())
        val item = arr[0].asJsonObject
        assertEquals(setOf("devId", "time", "temperature", "humidity"), item.keySet())
        assertEquals("0C118224", item.get("devId").asString)
        // time 必须是 JSON 数字（Unix 秒），不能是字符串
        assertTrue("time must be a JSON number", item.get("time").isJsonPrimitive && item.get("time").asJsonPrimitive.isNumber)
        assertEquals(1700000000L, item.get("time").asLong)
        assertTrue("temperature must be numeric", item.get("temperature").asJsonPrimitive.isNumber)
        assertEquals(-3.2, item.get("temperature").asDouble, 0.0)
        assertEquals(0.0, item.get("humidity").asDouble, 0.0)
    }

    // ---------- 2. 单位：不在本模块二次换算 ----------

    @Test
    fun unitsArePassedThrough_soNoDoubleScaling() {
        // 线上原始 ×10：255 → 25.5℃、600 → 60.0 %RH（换算只发生在解码/归因阶段）
        val p = UploadPayload.fromAttributed(
            "1", "M", 42L,
            listOf(reading("A1B2C3D4", 255 / 10.0, 600 / 10.0))
        )!!
        assertEquals(25.5, p.readings[0].temperature, 0.0)
        assertEquals(60.0, p.readings[0].humidity, 0.0)
        // 负对照：若本模块再除一次 10，值会变成 2.55 / 6.0
        assertTrue(p.readings[0].temperature != 2.55)
        assertTrue(p.readings[0].humidity != 6.0)
        // 负温保号，不出现 ×10 大数
        val neg = UploadPayload.fromAttributed("1", "M", 43L, listOf(reading("A1B2C3D4", -10.0, 0.1)))!!
        assertEquals(-10.0, neg.readings[0].temperature, 0.0)
        assertEquals(0.1, neg.readings[0].humidity, 0.0)
        assertTrue(neg.readings[0].temperature > -100.0)
    }

    @Test
    fun timeIsUnixSeconds_notMillis() {
        val p = UploadPayload.fromAttributed("1", "M", 1700000001L, listOf(reading("A1B2C3D4", 25.0, 60.0)))!!
        assertEquals(1700000001L, p.readings.single().time)
        assertTrue("must not be milliseconds", p.readings.single().time < 10_000_000_000L)
    }

    // ---------- 3. devId 来自读数自身，与列表下标无关 ----------

    @Test
    fun devIdComesFromEachReading_notFromIndex() {
        // 打乱顺序：devId 与数值必须成对保留
        val p = UploadPayload.fromAttributed(
            "1", "M", 100L,
            listOf(reading("33333333", 30.0, 70.0), reading("A1B2C3D4", 20.0, 50.0))
        )!!
        assertEquals(listOf("33333333", "A1B2C3D4"), p.readings.map { it.devId })
        assertEquals(listOf(30.0, 20.0), p.readings.map { it.temperature })
        assertEquals(listOf(70.0, 50.0), p.readings.map { it.humidity })
        assertEquals(listOf(100L, 100L), p.readings.map { it.time })
    }

    // ---------- 4. 空集不产生请求体；待发箱路径保留逐行时间 ----------

    @Test
    fun emptyReadings_produceNoPayload_butOutboxPathKeepsPerRowTimes() {
        assertNull(UploadPayload.fromAttributed("1", "M", 1L, emptyList()))
        assertNull(UploadPayload.build("1", "M", emptyList()))

        val rows = listOf(
            SensorReading("A1B2C3D4", 1700000005L, 25.0, 60.0),
            SensorReading("0C118224", 1700000185L, 24.0, 58.0)
        )
        val p = UploadPayload.build("1", "M", rows)!!
        assertEquals(listOf(1700000005L, 1700000185L), p.readings.map { it.time })
        assertEquals(listOf("A1B2C3D4", "0C118224"), p.readings.map { it.devId })
    }

    // ---------- 5. DTO 字段名/类型（独立期望） ----------

    @Test
    fun dtoFieldNamesAndTypes_matchServerContract() {
        assertEquals(
            listOf("devId", "time", "temperature", "humidity"),
            SensorReading::class.java.declaredFields.map { it.name }
        )
        assertEquals(
            listOf(String::class.java, Long::class.javaPrimitiveType, Double::class.javaPrimitiveType, Double::class.javaPrimitiveType),
            SensorReading::class.java.declaredFields.map { it.type }
        )
        assertEquals(
            listOf("phone", "mac", "readings"),
            SensorUploadData::class.java.declaredFields.map { it.name }
        )
        assertEquals(List::class.java, SensorUploadData::class.java.getDeclaredField("readings").type)
    }

    @Test
    fun existingDataClasses_areUnchanged() {
        val expected = mapOf(
            VerificationRequest::class.java to listOf("phoneNumber"),
            VerificationData::class.java to listOf("phoneNumber", "code"),
            RegistrationData::class.java to listOf("phoneNumber", "password"),
            LoginData::class.java to listOf("phoneNumber", "password"),
            LoginResponse::class.java to listOf("success", "token"),
            MacData::class.java to listOf("phoneNumber", "macAddress"),
        )
        for ((cls, fields) in expected) {
            assertEquals(cls.simpleName, fields, cls.declaredFields.map { it.name })
        }
    }

    // ---------- 6. ApiService：6 个端点、既有 5 个不变、新增端点为 JSON POST ----------

    @Test
    fun apiService_endpointInventoryAndContentType() {
        val expectedPaths = mapOf(
            "sendVerificationCode" to "/sendVerificationCode",
            "verifyCode" to "/verifyCode",
            "register" to "/register",
            "login" to "/login",
            "setMacAddress" to "/setmac",
            "uploadData" to "/upload_data",
        )
        val methods = ApiService::class.java.declaredMethods
        assertEquals("exactly 6 endpoints", 6, methods.size)
        val actualPaths = HashMap<String, String>()
        for (m in methods) {
            val post = m.getAnnotation(POST::class.java) ?: error("${m.name} missing @POST")
            actualPaths[m.name] = post.value
            assertTrue("${m.name} must be a suspend fn (Continuation param)",
                m.parameterTypes.any { it.simpleName == "Continuation" })
        }
        assertEquals(expectedPaths, actualPaths)

        val upload = methods.single { it.name == "uploadData" }
        val headers = upload.getAnnotation(Headers::class.java)
        assertTrue("uploadData must declare JSON content type", headers != null && headers.value.toList() == listOf("Content-Type: application/json"))
        // suspend 方法多一个 Continuation 参数；其余参数应为 SensorUploadData
        val paramTypes = upload.parameterTypes.filter { it.simpleName != "Continuation" }
        assertEquals(listOf(SensorUploadData::class.java), paramTypes)
    }
}
