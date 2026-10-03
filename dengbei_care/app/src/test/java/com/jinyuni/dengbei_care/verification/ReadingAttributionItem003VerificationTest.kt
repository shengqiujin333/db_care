package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.telemetry.ReadingAttribution
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.util.Random

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-003）。
 *
 * 与实现侧自检相互独立：期望值由本文件按 IC-002 §5 / 任务描述重新推导（独立解析与独立分类），
 * 覆盖穷举计数矩阵、数值/时间 token 分类、无状态性、拒绝结果不携带读数、以及与**改动前
 * `minOf` 截断实现**的对照（一致输入映射相同、不一致输入由"部分处理"变为"拒绝整批"）。
 */
class ReadingAttributionItem003VerificationTest {

    // ---------- 独立参考实现（不复用被测代码） ----------

    /** 独立解析：空白段 → 空表；否则逐 token 解析并要求有限值；任一项非法返回 null */
    private fun refParseSeries(series: String): List<Double>? {
        if (series.isBlank()) return emptyList()
        val values = ArrayList<Double>()
        for (raw in series.split(",")) {
            val v = raw.trim().toDoubleOrNull() ?: return null
            if (!v.isFinite()) return null
            values.add(v)
        }
        return values
    }

    private data class Ref(val time: Long, val readings: List<Triple<String, Double, Double>>)

    /** 独立判定：接受 → Ref；拒绝 → null */
    private fun refAttribute(payload: String, ids: List<String>): Ref? {
        val parts = payload.split(":")
        if (parts.size < 3) return null
        val t = parts[0].trim().toLongOrNull() ?: return null
        if (t <= 0L) return null
        val temps = refParseSeries(parts[1]) ?: return null
        val humis = refParseSeries(parts[2]) ?: return null
        if (temps.isEmpty() || humis.isEmpty()) return null
        if (ids.isEmpty()) return null
        if (ids.size != temps.size || temps.size != humis.size) return null
        val readings = ids.indices.map { Triple(ids[it], temps[it], humis[it]) }
        return Ref(t, readings)
    }

    /** 改动前实现的等价模拟（同一消息内）：`minOf` 截断后按下标处理 */
    private fun legacyTruncating(payload: String, ids: List<String>): List<Triple<String, Double, Double>> {
        val parts = payload.split(":")
        if (parts.size < 3) return emptyList()
        val t = parts[0].toLongOrNull() ?: 0L
        if (t <= 0L) return emptyList()
        val temps = try {
            parts[1].split(",").map { it.toDouble() }.filter { it.isFinite() }
        } catch (e: Exception) {
            emptyList()
        }
        val humis = try {
            parts[2].split(",").map { it.toDouble() }.filter { it.isFinite() }
        } catch (e: Exception) {
            emptyList()
        }
        if (temps.isEmpty() || humis.isEmpty() || ids.isEmpty()) return emptyList()
        val count = minOf(ids.size, temps.size, humis.size)
        return (0 until count).map { Triple(ids[it], temps[it], humis[it]) }
    }

    // ---------- 工具 ----------

    private fun idsOf(n: Int): List<String> = (0 until n).map { "%08X".format(0xA1B2C300L + it) }

    private fun series(n: Int, offset: Double): String =
        if (n <= 0) "" else (0 until n).joinToString(",") { "%.1f".format(offset + it) }

    private fun payload(t: String, temps: String, humis: String) = "$t:$temps:$humis"

    /** 段内 token 个数（空段记 0） */
    private fun tokenCount(series: String): Int = if (series.isBlank()) 0 else series.split(",").size

    private fun batchOf(result: ReadingAttribution.Result): ReadingAttribution.Batch {
        assertTrue("expected Success, got $result", result is ReadingAttribution.Result.Success)
        return (result as ReadingAttribution.Result.Success).batch
    }

    private fun rejectionOf(result: ReadingAttribution.Result): ReadingAttribution.Result.Rejected {
        assertTrue("expected Rejected, got $result", result is ReadingAttribution.Result.Rejected)
        return result as ReadingAttribution.Result.Rejected
    }

    // ---------- 1. 穷举计数矩阵：与独立参考一致（接受/拒绝 + 逐条映射） ----------

    @Test
    fun countMatrix_matchesIndependentReference() {
        var accepted = 0
        var rejected = 0
        for (nIds in 0..4) {
            for (nTemps in 0..4) {
                for (nHumis in 0..4) {
                    val ids = idsOf(nIds)
                    val p = payload("1700000000", series(nTemps, 20.0), series(nHumis, 50.0))
                    val expected = refAttribute(p, ids)
                    val actual = ReadingAttribution.attribute(p, ids)
                    if (expected == null) {
                        rejectionOf(actual)
                        rejected++
                    } else {
                        val b = batchOf(actual)
                        assertEquals("time ids=$nIds t=$nTemps h=$nHumis", expected.time, b.timeSeconds)
                        assertEquals(
                            "mapping ids=$nIds t=$nTemps h=$nHumis",
                            expected.readings.map { Triple(it.first, it.second, it.third) },
                            b.readings.map { Triple(it.devId, it.temperature, it.humidity) }
                        )
                        accepted++
                    }
                }
            }
        }
        assertEquals(5 * 5 * 5, accepted + rejected)
        assertTrue("non-trivial matrix", accepted > 0 && rejected > 0)
    }

    // ---------- 2. 数值 token 分类（穷举单 token 类别） ----------

    @Test
    fun numericTokenClasses_matchSpec() {
        val id = listOf("A1B2C3D4")
        val accepted = mapOf(
            "24.5" to 24.5,
            "-3.2" to -3.2,
            "0" to 0.0,
            "0.0" to 0.0,
            "125.0" to 125.0,
            " 24.5 " to 24.5,
            "1e2" to 100.0,
            ".5" to 0.5
        )
        for ((token, value) in accepted) {
            val b = batchOf(ReadingAttribution.attribute(payload("1700000000", token, "55.0"), id))
            assertEquals("token='$token'", value, b.readings[0].temperature, 0.0)
            assertEquals("strict mapping token='$token'", "A1B2C3D4", b.readings[0].devId)
        }
        val rejected = listOf(
            "abc", "", " ", "1.5.5", "0x10", "NaN", "Infinity", "-Infinity", "1,", ",1", "1,,2", "--1", "1e"
        )
        for (token in rejected) {
            val r = rejectionOf(ReadingAttribution.attribute(payload("1700000000", token, "55.0"), id))
            assertTrue("token='$token' reason=${r.reason}", r.reason in setOf(
                ReadingAttribution.RejectionReason.INVALID_NUMBER,
                ReadingAttribution.RejectionReason.EMPTY_SERIES
            ))
        }
    }

    // ---------- 3. 时间字段与分段数 ----------

    @Test
    fun timeAndSegmentClasses_matchSpec() {
        val id = listOf("A1B2C3D4")
        assertEquals(1700000000L, batchOf(ReadingAttribution.attribute("1700000000:24.5:55.0", id)).timeSeconds)
        assertEquals(1700000000L, batchOf(ReadingAttribution.attribute(" 1700000000 :24.5:55.0", id)).timeSeconds)
        for (t in listOf("0", "-5", "abc", "", " ", "1.5")) {
            assertEquals(
                "time='$t'",
                ReadingAttribution.RejectionReason.INVALID_TIME,
                rejectionOf(ReadingAttribution.attribute("$t:24.5:55.0", id)).reason
            )
        }
        for (p in listOf("24.5:55.0", "1700000000:24.5", "", "1700000000")) {
            assertEquals(
                "payload='$p'",
                ReadingAttribution.RejectionReason.INVALID_TIME,
                rejectionOf(ReadingAttribution.attribute(p, id)).reason
            )
        }
    }

    // ---------- 4. 拒绝结果不携带读数（结构） + 不做部分归因 ----------

    @Test
    fun rejectedResult_carriesNoReadings_andNeverPartiallySucceeds() {
        val fields = ReadingAttribution.Result.Rejected::class.java.declaredFields.map { it.name.lowercase() }
        assertTrue("Rejected fields=$fields", fields.none { it.contains("reading") || it.contains("batch") || it.contains("temperature") })
        assertTrue(
            "no list-returning methods",
            ReadingAttribution.Result.Rejected::class.java.declaredMethods
                .filter { List::class.java.isAssignableFrom(it.returnType) }.isEmpty()
        )
        val id = idsOf(2)
        // 旧实现会"处理前 min(...) 条"并静默丢弃余下数值，新实现必须整批拒绝
        val mismatch = listOf(
            payload("1700000000", "20.0,30.0", "50.0"),      // 湿度少 1
            payload("1700000000", "20.0", "50.0,60.0"),      // 温度少 1
            payload("1700000000", "20.0,30.0,40.0", "50.0,60.0"), // 温度多 1
            payload("1700000000", "", "50.0,60.0"),          // 温度空
            payload("1700000000", "20.0,30.0", "")           // 湿度空
        )
        for (p in mismatch) {
            val r = rejectionOf(ReadingAttribution.attribute(p, id))
            assertTrue("payload='$p' reason=${r.reason}", r.reason in setOf(
                ReadingAttribution.RejectionReason.COUNT_MISMATCH,
                ReadingAttribution.RejectionReason.EMPTY_SERIES
            ))
            // 旧实现至少静默丢弃了一个数值（或整批丢弃），绝不会报错
            val parts = p.split(":")
            val maxValues = maxOf(tokenCount(parts.getOrElse(1) { "" }), tokenCount(parts.getOrElse(2) { "" }))
            assertTrue(
                "legacy silently dropped values for '$p' (legacy=${legacyTruncating(p, id).size}, values=$maxValues)",
                legacyTruncating(p, id).size < maxValues
            )
        }
    }

    // ---------- 5. 与改动前截断实现的对照 ----------

    @Test
    fun legacyComparison_consistentIdentical_inconsistentDiverges() {
        // 一致输入：新旧映射逐条相同（回归安全）
        for (n in 1..4) {
            val ids = idsOf(n)
            val p = payload("1700000000", series(n, 20.0), series(n, 50.0))
            val legacy = legacyTruncating(p, ids)
            val now = batchOf(ReadingAttribution.attribute(p, ids)).readings
            assertEquals("n=$n", legacy.map { Triple(it.first, it.second, it.third) },
                now.map { Triple(it.devId, it.temperature, it.humidity) })
        }
        // 不一致输入：旧实现产生"部分/错位"结果，新实现拒绝整批（本项的核心行为变化）
        val ids = idsOf(3)
        val cases = listOf(
            payload("1700000000", series(3, 20.0), series(2, 50.0)),
            payload("1700000000", series(2, 20.0), series(3, 50.0)),
            payload("1700000000", series(1, 20.0), series(3, 50.0))
        )
        for (p in cases) {
            assertTrue("legacy produced partial result for '$p'", legacyTruncating(p, ids).isNotEmpty())
            assertEquals(
                "new must reject '$p'",
                ReadingAttribution.RejectionReason.COUNT_MISMATCH,
                rejectionOf(ReadingAttribution.attribute(p, ids)).reason
            )
        }
    }

    // ---------- 6. devId 随读数下沉、顺序来自 savedIds（而非下标含义） ----------

    @Test
    fun mappingFollowsSavedIdOrder_andDevIdIsCarried() {
        val a = idsOf(3)
        val b = listOf(a[2], a[0], a[1])
        val p = payload("1700000000", "20.0,30.0,40.0", "50.0,60.0,70.0")
        val ra = batchOf(ReadingAttribution.attribute(p, a)).readings
        val rb = batchOf(ReadingAttribution.attribute(p, b)).readings
        assertEquals(a, ra.map { it.devId })
        assertEquals(b, rb.map { it.devId })
        assertEquals(listOf(20.0, 30.0, 40.0), ra.map { it.temperature })
        // 同一批值在不同绑定顺序下归属不同设备：证明归属来自映射表而非"下标即身份"
        assertEquals(a[2], rb[0].devId)
        assertEquals(b[2], rb[2].devId)
        assertEquals(20.0, rb[0].temperature, 0.0)
        assertEquals(40.0, rb[2].temperature, 0.0)
    }

    // ---------- 7. 无状态性（不沿用上一条消息的值/时间） ----------

    @Test
    fun attribute_isStateless_acrossCalls() {
        val id = idsOf(2)
        val good = payload("1700000000", "20.0,30.0", "50.0,60.0")
        val first = batchOf(ReadingAttribution.attribute(good, id))
        // 中间插入非法/空白消息
        rejectionOf(ReadingAttribution.attribute(payload("1700000001", "", "60.0"), id))
        rejectionOf(ReadingAttribution.attribute(payload("bad", "20.0,30.0", "50.0,60.0"), id))
        val again = batchOf(ReadingAttribution.attribute(good, id))
        assertEquals(first.timeSeconds, again.timeSeconds)
        assertEquals(first.readings, again.readings)
    }

    // ---------- 8. 随机模糊：不得抛异常；成功时必须与独立参考一致 ----------

    @Test
    fun fuzz_neverThrows_andAgreesWithReference() {
        val alphabet = "0123456789:,. -abNfInty \t"
        val validTokens = listOf("24.5", "-3.2", "0", "0.0", "125.0", " 1.5 ", "1e2")
        val invalidTokens = listOf("abc", "", " ", "NaN", "Infinity", "1e", "--1", "1.2.3")
        val times = listOf("1700000000", "1700000001", " 1700000002 ", "0", "-1", "abc", "", "1.5")
        val rnd = Random(0x003C0DE1L)
        var ok = 0
        var rej = 0
        repeat(3000) {
            val p = if (rnd.nextInt(10) < 3) {
                // 纯随机字符（鲁棒性）
                val len = rnd.nextInt(40)
                buildString { repeat(len) { append(alphabet[rnd.nextInt(alphabet.length)]) } }
            } else {
                // 结构化：time:temps:humis（含非法 token 与错位计数）
                fun series(): String {
                    val n = rnd.nextInt(4)
                    if (n == 0) return ""
                    return (0 until n).joinToString(",") {
                        if (rnd.nextInt(4) == 0) invalidTokens[rnd.nextInt(invalidTokens.size)]
                        else validTokens[rnd.nextInt(validTokens.size)]
                    }
                }
                times[rnd.nextInt(times.size)] + ":" + series() + ":" + series()
            }
            val ids = idsOf(rnd.nextInt(4))
            val result = try {
                ReadingAttribution.attribute(p, ids)
            } catch (t: Throwable) {
                throw AssertionError("attribute threw for payload='$p' ids=$ids: $t", t)
            }
            val expected = refAttribute(p, ids)
            if (expected == null) {
                rej++
                rejectionOf(result)
            } else {
                ok++
                val b = batchOf(result)
                assertEquals("fuzz payload='$p'", expected.time, b.timeSeconds)
                assertEquals(
                    "fuzz mapping payload='$p'",
                    expected.readings,
                    b.readings.map { Triple(it.devId, it.temperature, it.humidity) }
                )
            }
        }
        assertTrue("fuzz coverage ok=$ok rej=$rej", ok > 0 && rej > 0)
    }
}
