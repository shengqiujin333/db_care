package com.jinyuni.dengbei_care.protocol

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.util.Random

/**
 * ITEM-001 实现侧自检：BLE 聚合帧解码（protocol/GatewayFrameCodec）。
 *
 * 覆盖：
 * - 帧长规则（恰等 / 更长带尾部填充 / 不足 / 不足一个头部块 / devCount == 0 必须拒绝）；
 * - 每设备 8 B 字段解码（id 原字节序、hum u16be@4、temp s16be@6 有符号）与改动前逐位一致；
 * - 多设备归属随块内 ID 走，不依赖下标；
 * - 与改动前 parsePlainFrame 参考实现在固定种子随机向量上逐位相等。
 */
class GatewayFrameCodecTest {

    // ---------- 参考实现（改动前 MqtttService.parsePlainFrame 的逐字复刻，仅用于比对） ----------

    private fun legacyU16be(b: ByteArray, i: Int): Int =
        ((b[i].toInt() and 0xFF) shl 8) or (b[i + 1].toInt() and 0xFF)

    private fun legacyS16be(b: ByteArray, i: Int): Int {
        val v = legacyU16be(b, i)
        return if ((v and 0x8000) != 0) v - 0x10000 else v
    }

    private data class LegacySample(val devIdHex: String, val humidityPct: Double, val temperatureC: Double)

    private fun legacyParsePlainFrame(plain: ByteArray): Pair<String, List<LegacySample>> {
        require(plain.size >= 16) { "plain too short" }
        val gwid6 = plain.copyOfRange(0, 6)
        val devCount = plain[6].toInt() and 0xFF
        val expected = 16 * (1 + devCount)
        require(plain.size >= expected) { "plain length ${plain.size} < expected $expected" }

        val readings = ArrayList<LegacySample>(devCount)
        var off = 16
        repeat(devCount) {
            val block = plain.copyOfRange(off, off + 16)
            val p8 = block.copyOfRange(0, 8)
            val devIdBytes = p8.copyOfRange(0, 4)
            readings += LegacySample(
                devIdBytes.joinToString(":") { "%02X".format(it) },
                legacyU16be(p8, 4) / 10.0,
                legacyS16be(p8, 6) / 10.0
            )
            off += 16
        }
        return gwid6.joinToString(":") { "%02X".format(it) } to readings
    }

    // ---------- 构造工具 ----------

    private fun be16(v: Int): ByteArray =
        byteArrayOf(((v shr 8) and 0xFF).toByte(), (v and 0xFF).toByte())

    private val gatewayId = byteArrayOf(
        0x3C, 0x1A, 0x40, 0x7A, 0x4E, 0xE0.toByte()
    )

    private fun deviceBlock(idHex8: String, humX10: Int, tempX10: Int): ByteArray {
        val id = ByteArray(4) { idHex8.substring(it * 2, it * 2 + 2).toInt(16).toByte() }
        return id + be16(humX10) + be16(tempX10) + ByteArray(8) // 后 8 B 为保留填充
    }

    private fun frame(devCount: Int, vararg blocks: ByteArray): ByteArray =
        gatewayId + byteArrayOf(devCount.toByte()) + ByteArray(9) + blocks.reduce { a, b -> a + b }

    private fun expectSuccess(result: GatewayFrameCodec.Result): GatewayFrameCodec.Result.Success {
        assertTrue("expected Success but got $result", result is GatewayFrameCodec.Result.Success)
        return result as GatewayFrameCodec.Result.Success
    }

    private fun expectRejected(
        result: GatewayFrameCodec.Result,
        reason: GatewayFrameCodec.RejectionReason
    ) {
        assertTrue("expected Rejected but got $result", result is GatewayFrameCodec.Result.Rejected)
        assertEquals(reason, (result as GatewayFrameCodec.Result.Rejected).reason)
    }

    // ---------- L0-01 帧长规则 ----------

    @Test
    fun exactLength_isAccepted_withDevCountSamples() {
        val f = frame(
            2,
            deviceBlock("A1B2C3D4", 600, 250),
            deviceBlock("0C118224", 400, -100)
        )
        assertEquals(16 * (1 + 2), f.size)
        val ok = expectSuccess(GatewayFrameCodec.decodeAggregatedFrame(f))
        assertEquals("3C:1A:40:7A:4E:E0", ok.gatewayIdHex)
        assertEquals(2, ok.devCount)
        assertEquals(2, ok.samples.size)
    }

    @Test
    fun longerThanExpected_isAccepted_andExtraTailIgnored() {
        val f = frame(1, deviceBlock("A1B2C3D4", 600, 250)) + ByteArray(15) { 0x7F }
        val ok = expectSuccess(GatewayFrameCodec.decodeAggregatedFrame(f))
        assertEquals(1, ok.devCount)
        assertEquals(1, ok.samples.size)
        assertEquals(60.0, ok.samples[0].humidityPct, 1e-9)
        assertEquals(25.0, ok.samples[0].temperatureC, 1e-9)
    }

    @Test
    fun shorterThanExpected_isRejected_withoutSamples() {
        val f = frame(3, deviceBlock("A1B2C3D4", 600, 250), deviceBlock("0C118224", 400, -100))
        // 少一个设备块（15 B 尾部填充不足以构成第 3 块）
        val truncated = f.copyOf(f.size - 1)
        expectRejected(
            GatewayFrameCodec.decodeAggregatedFrame(truncated),
            GatewayFrameCodec.RejectionReason.LENGTH_MISMATCH
        )
    }

    @Test
    fun shorterThanHeaderBlock_isRejected() {
        expectRejected(
            GatewayFrameCodec.decodeAggregatedFrame(ByteArray(15)),
            GatewayFrameCodec.RejectionReason.FRAME_TOO_SHORT
        )
        expectRejected(
            GatewayFrameCodec.decodeAggregatedFrame(ByteArray(0)),
            GatewayFrameCodec.RejectionReason.FRAME_TOO_SHORT
        )
    }

    @Test
    fun zeroDeviceCount_isRejected_withoutSamples() {
        val f = gatewayId + byteArrayOf(0) + ByteArray(9) // devCount == 0, 长度恰为 16 B
        assertEquals(16, f.size)
        val rejected = GatewayFrameCodec.decodeAggregatedFrame(f)
        expectRejected(rejected, GatewayFrameCodec.RejectionReason.NO_DEVICE)
    }

    // ---------- L0-02 字段解码 ----------

    @Test
    fun temperatureVectors_areSignedAndScaledBy10() {
        val vectors = listOf(
            25.0 to 0x00FA,
            -10.0 to 0xFF9C,
            0.0 to 0x0000,
            125.0 to 0x04E2,
            -40.0 to 0xFE70
        )
        vectors.forEach { (expected, raw) ->
            val f = frame(1, deviceBlock("A1B2C3D4", 600, raw))
            val s = expectSuccess(GatewayFrameCodec.decodeAggregatedFrame(f)).samples[0]
            assertEquals("temp raw=0x%04X".format(raw), expected, s.temperatureC, 1e-9)
        }
    }

    @Test
    fun humidityVectors_areUnsignedAndScaledBy10() {
        val vectors = listOf(0.0 to 0x0000, 60.0 to 0x0258, 100.0 to 0x03E8)
        vectors.forEach { (expected, raw) ->
            val f = frame(1, deviceBlock("A1B2C3D4", raw, 250))
            val s = expectSuccess(GatewayFrameCodec.decodeAggregatedFrame(f)).samples[0]
            assertEquals("hum raw=0x%04X".format(raw), expected, s.humidityPct, 1e-9)
        }
    }

    @Test
    fun humidityAndTemperatureAreNotSwapped() {
        // 湿度 0x0258(600 -> 60.0)、温度 0x00FA(250 -> 25.0)
        val f = frame(1, deviceBlock("A1B2C3D4", 0x0258, 0x00FA))
        val s = expectSuccess(GatewayFrameCodec.decodeAggregatedFrame(f)).samples[0]
        assertEquals(60.0, s.humidityPct, 1e-9)
        assertEquals(25.0, s.temperatureC, 1e-9)
    }

    @Test
    fun deviceId_formatsAndByteOrderArePreserved() {
        val f = frame(1, deviceBlock("0C118224", 600, 250))
        val s = expectSuccess(GatewayFrameCodec.decodeAggregatedFrame(f)).samples[0]
        assertEquals("0C:11:82:24", s.devIdHex)
        assertEquals("0C118224", s.devIdCompact)
        assertEquals(8, s.devIdCompact.length)
        assertEquals(s.devIdCompact, s.devIdCompact.uppercase())
        assertTrue(s.devIdBytes.contentEquals(byteArrayOf(0x0C, 0x11, 0x82.toByte(), 0x24)))
    }

    // ---------- L0-03 多设备与归属 ----------

    @Test
    fun multiDevice_samplesCarryTheirOwnId() {
        val a = deviceBlock("11111111", 600, 250)
        val b = deviceBlock("22222222", 300, -50)
        val c = deviceBlock("33333333", 900, 400)

        val inOrder = expectSuccess(GatewayFrameCodec.decodeAggregatedFrame(frame(3, a, b, c)))
        assertEquals(listOf("11111111", "22222222", "33333333"), inOrder.samples.map { it.devIdCompact })

        val shuffled = expectSuccess(GatewayFrameCodec.decodeAggregatedFrame(frame(3, c, a, b)))
        assertEquals(listOf("33333333", "11111111", "22222222"), shuffled.samples.map { it.devIdCompact })
        // 归属随块内 ID 走，不按下标绑定
        assertEquals(mapOf("33333333" to 40.0, "11111111" to 25.0, "22222222" to -5.0),
            shuffled.samples.associate { it.devIdCompact to it.temperatureC })
    }

    // ---------- 与改动前实现逐位一致（固定种子随机向量） ----------

    @Test
    fun matchesLegacyImplementation_onRandomVectors() {
        val rnd = Random(20260911L)
        var compared = 0
        repeat(500) {
            val devCount = 1 + rnd.nextInt(5)
            val blocks = (0 until devCount).map {
                val id = ByteArray(4) { rnd.nextInt(256).toByte() }
                val humX10 = rnd.nextInt(1001)          // 0..100.0 %RH
                val tempX10 = -400 + rnd.nextInt(1651)  // -40.0 .. 125.0 ℃
                id + be16(humX10) + be16(tempX10) + ByteArray(8)
            }.toTypedArray()
            val f = frame(devCount, *blocks) + ByteArray(rnd.nextInt(16)) { rnd.nextInt(256).toByte() }

            val expected = legacyParsePlainFrame(f)
            val actual = expectSuccess(GatewayFrameCodec.decodeAggregatedFrame(f))

            assertEquals("gatewayIdHex", expected.first, actual.gatewayIdHex)
            assertEquals("devCount", devCount, actual.devCount)
            assertEquals("samples", devCount, actual.samples.size)
            expected.second.forEachIndexed { i, exp ->
                val act = actual.samples[i]
                assertEquals("devIdHex#$i", exp.devIdHex, act.devIdHex)
                assertEquals("humidityPct#$i", exp.humidityPct, act.humidityPct, 0.0)
                assertEquals("temperatureC#$i", exp.temperatureC, act.temperatureC, 0.0)
            }
            compared++
        }
        assertEquals(500, compared)
    }
}
