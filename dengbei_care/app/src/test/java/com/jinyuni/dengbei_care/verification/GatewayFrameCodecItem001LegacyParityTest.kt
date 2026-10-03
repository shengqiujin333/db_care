package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.protocol.GatewayFrameCodec
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.util.Random

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-001）：与**改动前实现**逐位一致。
 *
 * 本文件内的 `legacy*` 段是从 Git 历史中**逐字提取**的改动前实现，来源：
 *   commit `fe9a81c`（android_engineer.android_design 后、本项实现前）文件
 *   `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt`
 *   —— `ByteArray.toHexSep` / `u16be` / `s16be` / `parsePlainFrame` 及其使用的 `DeviceReading`/`ParsedFrame` 定义。
 * 仅用于比对期望值；不参与被测代码路径、不引用被测实现。已验证的期望：对**所有被接受**的帧，
 * 网关 ID、devCount、每设备 devIdHex、湿度、温度必须与改动前**逐位相等**（Double 用 toRawBits 比较）。
 */
class GatewayFrameCodecItem001LegacyParityTest {

    // ================= 以下为 Git 历史逐字提取的改动前实现（比对基准） =================

    private data class LegacyDeviceReading(
        val devIdBytes: ByteArray,
        val devIdHex: String,
        val humidityPct: Double,
        val temperatureC: Double
    )

    private data class LegacyParsedFrame(
        val gatewayIdHex: String,
        val devCount: Int,
        val readings: List<LegacyDeviceReading>
    )

    private fun ByteArray.legacyToHexSep(colon: Boolean = true): String =
        this.joinToString(if (colon) ":" else "") { "%02X".format(it) }

    private fun legacyU16be(b: ByteArray, i: Int): Int =
        ((b[i].toInt() and 0xFF) shl 8) or (b[i + 1].toInt() and 0xFF)

    private fun legacyS16be(b: ByteArray, i: Int): Int {
        val v = legacyU16be(b, i)
        return if ((v and 0x8000) != 0) v - 0x10000 else v
    }

    private fun legacyParsePlainFrame(plain: ByteArray): LegacyParsedFrame {
        require(plain.size >= 16) { "plain too short" }
        val gwid6 = plain.copyOfRange(0, 6)
        val devCount = plain[6].toInt() and 0xFF
        val expected = 16 * (1 + devCount)
        require(plain.size >= expected) { "plain length ${plain.size} < expected $expected" }

        val readings = ArrayList<LegacyDeviceReading>(devCount)
        var off = 16
        repeat(devCount) {
            val block = plain.copyOfRange(off, off + 16)
            val p8 = block.copyOfRange(0, 8)

            val devIdBytes = p8.copyOfRange(0, 4)             // 大端原样
            val devIdHex = devIdBytes.legacyToHexSep()
            val humi = legacyU16be(p8, 4) / 10.0
            val temp = legacyS16be(p8, 6) / 10.0

            readings += LegacyDeviceReading(devIdBytes, devIdHex, humi, temp)
            off += 16
        }
        return LegacyParsedFrame(gwid6.legacyToHexSep(), devCount, readings)
    }

    // ================= 向量构造与比对 =================

    private val gatewayId = byteArrayOf(0x3C, 0x1A, 0x40, 0x7A, 0x4E, 0xE0.toByte())

    private fun frame(devCount: Int, blocks: List<ByteArray>, pad: Int, seedByte: Int): ByteArray {
        val out = ArrayList<Byte>(16 * (1 + devCount) + pad)
        gatewayId.forEach { out.add(it) }
        out.add(devCount.toByte())
        repeat(9) { out.add(0) }
        blocks.forEach { blk ->
            blk.forEach { out.add(it) }
            repeat(8) { out.add(0) } // 每设备块后 8 B 保留
        }
        repeat(pad) { out.add((seedByte + it).toByte()) }
        return out.toByteArray()
    }

    private fun be16(v: Int) = byteArrayOf(((v ushr 8) and 0xFF).toByte(), (v and 0xFF).toByte())

    private fun deviceBlock(id: ByteArray, humX10: Int, tempX10: Int): ByteArray =
        id + be16(humX10) + be16(tempX10)

    private fun assertBitIdentical(plain: ByteArray, label: String) {
        val legacy = legacyParsePlainFrame(plain)
        val parsed = GatewayFrameCodec.decodeAggregatedFrame(plain)
        assertTrue("$label: expected Success, got $parsed", parsed is GatewayFrameCodec.Result.Success)
        parsed as GatewayFrameCodec.Result.Success
        assertEquals("$label gatewayIdHex", legacy.gatewayIdHex, parsed.gatewayIdHex)
        assertEquals("$label devCount", legacy.devCount, parsed.devCount)
        assertEquals("$label sample count", legacy.readings.size, parsed.samples.size)
        legacy.readings.forEachIndexed { i, exp ->
            val act = parsed.samples[i]
            assertEquals("$label devIdHex#$i", exp.devIdHex, act.devIdHex)
            assertEquals("$label devIdBytes#$i", exp.devIdBytes.toList(), act.devIdBytes.toList())
            // 逐位（bit-for-bit）比较 Double
            assertEquals("$label humidity bits#$i", exp.humidityPct.toRawBits(), act.humidityPct.toRawBits())
            assertEquals("$label temperature bits#$i", exp.temperatureC.toRawBits(), act.temperatureC.toRawBits())
        }
    }

    /** 边界矩阵：devCount 1..8、每设备全 16 bit 极值组合、尾部填充 0..32 */
    @Test
    fun acceptedFrames_areBitIdenticalToPreChangeImplementation() {
        val extremePairs = listOf(
            0x0000 to 0x0000,
            0xFFFF to 0xFFFF,
            0x8000 to 0x8000,
            0x7FFF to 0x7FFF,
            0x03E8 to 0x04E2,
            0x0000 to 0xFF9C,
            0xFFFF to 0x00FA
        )
        var cases = 0
        for (devCount in 1..8) {
            for (pad in 0..32) {
                extremePairs.forEach { (hum, temp) ->
                    val blocks = List(devCount) { i ->
                        deviceBlock(byteArrayOf(i.toByte(), 0x11, 0x82.toByte(), (0xFF - i).toByte()), hum, temp)
                    }
                    assertBitIdentical(frame(devCount, blocks, pad, 0x33), "dc=$devCount pad=$pad hum=$hum temp=$temp")
                    cases++
                }
            }
        }
        assertEquals(8 * 33 * extremePairs.size, cases)
    }

    /** 随机向量：全 16 bit 范围、随机设备 ID、随机填充，逐位比对 */
    @Test
    fun randomFrames_areBitIdenticalToPreChangeImplementation() {
        val rnd = Random(0x1F2E3D4C5B6A7988L)
        repeat(600) {
            val devCount = 1 + rnd.nextInt(5)
            val blocks = List(devCount) {
                deviceBlock(
                    ByteArray(4) { rnd.nextInt(256).toByte() },
                    rnd.nextInt(0x10000),
                    rnd.nextInt(0x10000)
                )
            }
            assertBitIdentical(frame(devCount, blocks, rnd.nextInt(40), rnd.nextInt(256)), "random devCount=$devCount")
        }
    }

    /** 拒绝边界与改动前的差异只应发生在"拒绝方式"（devCount==0 / 长度不足），不得读出数值 */
    @Test
    fun rejectionBoundaries_differFromLegacyOnlyByExplicitRejection() {
        // 长度不足：改动前抛 IllegalArgumentException；现在必须显式 Rejected
        val short = frame(2, listOf(deviceBlock(byteArrayOf(1, 2, 3, 4), 600, 250)), 0, 0).copyOf(31)
        val legacyThrew = try {
            legacyParsePlainFrame(short); false
        } catch (e: IllegalArgumentException) {
            true
        }
        assertTrue("legacy must reject short frame", legacyThrew)
        assertTrue(
            "new must reject short frame",
            GatewayFrameCodec.decodeAggregatedFrame(short) is GatewayFrameCodec.Result.Rejected
        )

        // devCount == 0：改动前是"空帧成功"，现在必须显式拒绝（任务要求的唯一语义变化）
        val zero = gatewayId + byteArrayOf(0) + ByteArray(9)
        val legacyEmpty = legacyParsePlainFrame(zero)
        assertEquals(0, legacyEmpty.readings.size)
        val newZero = GatewayFrameCodec.decodeAggregatedFrame(zero)
        assertTrue("devCount=0 must be rejected now", newZero is GatewayFrameCodec.Result.Rejected)
        assertEquals(
            GatewayFrameCodec.RejectionReason.NO_DEVICE,
            (newZero as GatewayFrameCodec.Result.Rejected).reason
        )
    }
}
