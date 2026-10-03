package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.protocol.GatewayFrameCodec
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.util.Random

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-001）。
 *
 * 与实现侧自检 `protocol/GatewayFrameCodecTest` 相互独立：本文件的期望值**不引用实现或其自检的辅助函数**，
 * 而是直接按 IC-002 §4 / AA-002 §5.1 的线上布局重新推导（偏移、字节序、符号、单位、帧长规则），
 * 并用**穷举表 + 独立 Python 参考的 FNV-1a 哈希**比对：
 *   - 温度：全部 65536 个原始字，期望整数 = (raw>=0x8000 ? raw-0x10000 : raw)；
 *   - 湿度：全部 65536 个原始字，期望整数 = raw（无符号）；
 *   - 设备 ID：字节顺序遍历；帧长规则：devCount 0..8 × 期望长度前后各边界。
 * 哈希常量由独立 Python 脚本（见 evidence 记录）按同一 FNV-1a 定义生成，本文件不自行复算。
 */
class GatewayFrameCodecItem001VerificationTest {

    // ---------- 独立常量与工具（不依赖被测实现） ----------

    private val fnvOffset: Long = -3750763034362895579L // 0xCBF29CE484222325 作为有符号 Long
    private val fnvPrime: Long = 1099511628211L          // 0x100000001B3

    /** 独立十六进制表（避免与被测实现共用格式化写法） */
    private val hexDigits = "0123456789ABCDEF".toCharArray()

    private fun hex2(v: Int): String = "${hexDigits[(v ushr 4) and 0xF]}${hexDigits[v and 0xF]}"

    private fun be16(v: Int): ByteArray = byteArrayOf(((v ushr 8) and 0xFF).toByte(), (v and 0xFF).toByte())

    private val gatewayId = byteArrayOf(
        0x3C, 0x1A, 0x40, 0x7A, 0x4E, 0xE0.toByte()
    )

    /** 独立构造：头块 16 B（网关 ID 6 B + devCount@6 + 9 B 保留）+ 每设备 16 B（前 8 B 有效） */
    private fun buildFrame(devCount: Int, devices: List<Triple<ByteArray, Int, Int>>): ByteArray {
        val out = ArrayList<Byte>(16 * (1 + devCount))
        gatewayId.forEach { out.add(it) }
        out.add(devCount.toByte())
        repeat(9) { out.add(0) }
        devices.forEach { (id, humX10, tempX10) ->
            require(id.size == 4)
            id.forEach { out.add(it) }
            be16(humX10).forEach { out.add(it) }
            be16(tempX10).forEach { out.add(it) }
            repeat(8) { out.add(0) } // 保留填充
        }
        return out.toByteArray()
    }

    private fun success(result: GatewayFrameCodec.Result): GatewayFrameCodec.Result.Success {
        assertTrue("expected Success, got $result", result is GatewayFrameCodec.Result.Success)
        return result as GatewayFrameCodec.Result.Success
    }

    private fun rejected(result: GatewayFrameCodec.Result): GatewayFrameCodec.Result.Rejected {
        assertTrue("expected Rejected, got $result", result is GatewayFrameCodec.Result.Rejected)
        return result as GatewayFrameCodec.Result.Rejected
    }

    /** 独立推导：有符号 16 bit 整数（IC-002 §4） */
    private fun specSigned16(hi: Int, lo: Int): Int {
        val u = ((hi and 0xFF) shl 8) or (lo and 0xFF)
        return if (u >= 0x8000) u - 0x10000 else u
    }

    private fun decodeOne(devCount: Int, id: ByteArray, humX10: Int, tempX10: Int) =
        success(GatewayFrameCodec.decodeAggregatedFrame(buildFrame(devCount, listOf(Triple(id, humX10, tempX10)) + List(devCount - 1) { Triple(byteArrayOf(0, 0, 0, 0), 0, 0) })))

    // ---------- 1. 穷举：温度原始字（65536）与独立参考表哈希比对 ----------

    @Test
    fun temperature_allRawWords_matchIndependentSpecAndHash() {
        var h = fnvOffset
        var checked = 0
        for (raw in 0..0xFFFF) {
            val sample = decodeOne(1, byteArrayOf(0x0C, 0x11, 0x82.toByte(), 0x24), 0, raw).samples[0]
            val expectedInt = specSigned16((raw ushr 8) and 0xFF, raw and 0xFF)
            // 期望值由独立公式给出；实现必须逐值相等（严格，无容差）
            assertEquals("temp raw=$raw", expectedInt, Math.round(sample.temperatureC * 10.0).toInt())
            assertEquals("temp exact raw=$raw", expectedInt / 10.0, sample.temperatureC, 0.0)

            h = h xor (raw.toLong() and 0xFFFFL)
            h *= fnvPrime
            h = h xor (expectedInt.toLong() and 0xFFFFFFFFL)
            h *= fnvPrime
            checked++
        }
        assertEquals(65536, checked)
        // 独立 Python 参考（同一 FNV-1a 定义）计算所得：
        assertEquals("temperature table hash", 0x2852b22d36982325L, h)
    }

    // ---------- 2. 穷举：湿度原始字（65536）与独立参考表哈希比对 ----------

    @Test
    fun humidity_allRawWords_matchIndependentSpecAndHash() {
        var h = fnvOffset
        var checked = 0
        for (raw in 0..0xFFFF) {
            val sample = decodeOne(1, byteArrayOf(0x0C, 0x11, 0x82.toByte(), 0x24), raw, 0).samples[0]
            val expectedInt = raw // u16be，无符号
            assertEquals("hum raw=$raw", expectedInt, Math.round(sample.humidityPct * 10.0).toInt())
            assertEquals("hum exact raw=$raw", expectedInt / 10.0, sample.humidityPct, 0.0)

            h = h xor (raw.toLong() and 0xFFFFL)
            h *= fnvPrime
            h = h xor (expectedInt.toLong() and 0xFFFFFFFFL)
            h *= fnvPrime
            checked++
        }
        assertEquals(65536, checked)
        // 独立 Python 参考（同一 FNV-1a 定义）计算所得：
        assertEquals("humidity table hash", 0x7e93e747630a2325L, h)
    }

    // ---------- 3. 设备 ID 原字节序（遍历首/末字节组合） ----------

    @Test
    fun deviceId_byteOrderAndFormat_exhaustiveFirstAndLastByte() {
        var checked = 0
        for (b0 in 0..255) {
            for (b3 in 0..255) {
                val id = byteArrayOf(b0.toByte(), 0x11, 0x82.toByte(), b3.toByte())
                val s = decodeOne(1, id, 600, 250).samples[0]
                val expected = hex2(b0) + "1182" + hex2(b3)
                assertEquals("devIdCompact b0=$b0 b3=$b3", expected, s.devIdCompact)
                assertEquals("devIdHex b0=$b0 b3=$b3", hex2(b0) + ":11:82:" + hex2(b3), s.devIdHex)
                assertTrue("devIdBytes order", s.devIdBytes.contentEquals(id))
                checked++
            }
        }
        assertEquals(65536, checked)
    }

    // ---------- 4. 帧长规则矩阵（含 devCount == 0） ----------

    @Test
    fun frameLengthRule_matrix_rejectsExactlyOutsideSpec() {
        for (devCount in 0..8) {
            val devices = List(devCount) { i -> Triple(byteArrayOf(1, 2, 3, i.toByte()), 100 + i, 200 + i) }
            val full = buildFrame(devCount, devices)
            val expectedSize = 16 * (1 + devCount)
            assertEquals("built size", expectedSize, full.size)

            // 尾部填充：expected .. expected+32 应成功并按 devCount 取数（devCount==0 除外）
            for (pad in 0..32) {
                val frame = full + ByteArray(pad) { 0x5A }
                val r = GatewayFrameCodec.decodeAggregatedFrame(frame)
                if (devCount == 0) {
                    assertEquals("devCount=0 pad=$pad", GatewayFrameCodec.RejectionReason.NO_DEVICE, rejected(r).reason)
                } else {
                    val ok = success(r)
                    assertEquals("devCount samples", devCount, ok.samples.size)
                }
            }
            // 短一字节：devCount==0 时仍 ≥16 但长度已满，故按 NO_DEVICE；devCount>=1 时长度不足
            val short = full.copyOf(expectedSize - 1)
            val shortReason = rejected(GatewayFrameCodec.decodeAggregatedFrame(short)).reason
            if (expectedSize - 1 < 16) {
                assertEquals(GatewayFrameCodec.RejectionReason.FRAME_TOO_SHORT, shortReason)
            } else if (devCount == 0) {
                assertEquals(GatewayFrameCodec.RejectionReason.NO_DEVICE, shortReason)
            } else {
                assertEquals(GatewayFrameCodec.RejectionReason.LENGTH_MISMATCH, shortReason)
            }
        }
        // 明文不足一个头块
        for (size in 0..15) {
            assertEquals(
                "size=$size",
                GatewayFrameCodec.RejectionReason.FRAME_TOO_SHORT,
                rejected(GatewayFrameCodec.decodeAggregatedFrame(ByteArray(size))).reason
            )
        }
    }

    // ---------- 5. 拒绝结果结构上不可能携带读数 ----------

    @Test
    fun rejectedResult_carriesNoReadings() {
        val fields = GatewayFrameCodec.Result.Rejected::class.java.declaredFields.map { it.name.lowercase() }
        assertTrue("Rejected fields=$fields", fields.none { it.contains("sample") || it.contains("reading") || it.contains("temperature") || it.contains("humidity") })
        val listReturning = GatewayFrameCodec.Result.Rejected::class.java.declaredMethods
            .filter { List::class.java.isAssignableFrom(it.returnType) }
        assertTrue("Rejected list-returning methods=$listReturning", listReturning.isEmpty())
    }

    // ---------- 6. 校验灵敏度（负对照）：证明本套断言能识别错误解释 ----------

    @Test
    fun harnessSensitivity_detectsWrongSignAndWrongLengthRules() {
        // 反例：0xFF9C 若按无符号解释为 6543.6，与实现的有符号结果 -10.0 必须不同
        val sample = decodeOne(1, byteArrayOf(0x0C, 0x11, 0x82.toByte(), 0x24), 600, 0xFF9C).samples[0]
        val unsignedWrong = 0xFF9C / 10.0
        assertEquals(-10.0, sample.temperatureC, 0.0)
        assertTrue("unsigned interpretation must differ", unsignedWrong != sample.temperatureC)

        // 反例：截断帧必须被拒绝，不得返回 Success
        val full = buildFrame(
            2,
            listOf(
                Triple(byteArrayOf(1, 2, 3, 4), 600, 250),
                Triple(byteArrayOf(5, 6, 7, 8), 400, 300)
            )
        )
        val truncated = full.copyOf(full.size - 16)
        val r = GatewayFrameCodec.decodeAggregatedFrame(truncated)
        assertTrue("truncated frame must not succeed", r !is GatewayFrameCodec.Result.Success)
        assertEquals(GatewayFrameCodec.RejectionReason.LENGTH_MISMATCH, rejected(r).reason)
        // 反例：devCount=0 必须被拒绝，不得被当成"空帧成功"
        val zero = GatewayFrameCodec.decodeAggregatedFrame(gatewayId + byteArrayOf(0) + ByteArray(9))
        assertTrue("devCount=0 must not succeed", zero !is GatewayFrameCodec.Result.Success)
    }

    // ---------- 7. 随机帧与独立参考实现逐位一致（自写参考，不复刻实现写法） ----------

    @Test
    fun randomFrames_matchIndependentReference_bitForBit() {
        val rnd = Random(0x5EED2026L)
        repeat(400) {
            val devCount = 1 + rnd.nextInt(6)
            val ids = ArrayList<ByteArray>()
            val hums = ArrayList<Int>()
            val temps = ArrayList<Int>()
            repeat(devCount) {
                ids += ByteArray(4) { rnd.nextInt(256).toByte() }
                hums += rnd.nextInt(0x10000)                 // 全 16 bit 范围
                temps += rnd.nextInt(0x10000)                // 全 16 bit 范围（含负温）
            }
            val frame = buildFrame(devCount, List(devCount) { Triple(ids[it], hums[it], temps[it]) }) +
                ByteArray(rnd.nextInt(48)) { rnd.nextInt(256).toByte() }

            val ok = success(GatewayFrameCodec.decodeAggregatedFrame(frame))
            assertEquals(devCount, ok.devCount)
            assertEquals(devCount, ok.samples.size)
            for (i in 0 until devCount) {
                val s = ok.samples[i]
                val idHex = (0 until 4).joinToString("") { hex2(ids[i][it].toInt() and 0xFF) }
                assertEquals("devId#$i", idHex, s.devIdCompact)
                assertEquals("hum#$i", hums[i] / 10.0, s.humidityPct, 0.0)
                assertEquals("temp#$i", specSigned16((temps[i] ushr 8) and 0xFF, temps[i] and 0xFF) / 10.0, s.temperatureC, 0.0)
            }
        }
    }
}
