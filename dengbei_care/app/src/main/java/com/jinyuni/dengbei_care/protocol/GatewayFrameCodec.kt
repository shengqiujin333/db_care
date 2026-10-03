package com.jinyuni.dengbei_care.protocol

/**
 * BLE 聚合帧（网关 FFE2 读回、AES-ECB 解密后的明文）解码。
 *
 * 线上布局（IC-002 §4 / AA-002 §5.1，勿改）：
 * ```text
 * 头部块 16 B : gateway_id(6 B 原字节序) + devCount(1 B, 偏移 6) + pad(9 B)
 * 设备块 16 B : 取前 8 B 有效 = sensor_id(4 B 原字节序) | humidity_x10(u16be@4) | temperature_x10(s16be@6)
 * 帧长       : 16 * (1 + devCount)
 * ```
 * 本模块为纯逻辑（不依赖 Android），字节偏移、字节序、符号处理与改动前的
 * `MqtttService.parsePlainFrame` 逐位一致；仅把"长度不足/无设备"从抛异常改为
 * 显式拒绝结果，调用方据此"不产生读数、不落库、不上传"。
 */
object GatewayFrameCodec {

    /** 头部块固定长度（字节） */
    const val HEADER_BLOCK_SIZE = 16

    /** 每设备块固定长度（字节，含 8 B 保留填充） */
    const val DEVICE_BLOCK_SIZE = 16

    /** 每设备块中有效载荷长度（字节） */
    const val DEVICE_PAYLOAD_SIZE = 8

    /** 给定 devCount 时的期望帧长：16 * (1 + devCount) */
    fun expectedFrameSize(devCount: Int): Int = HEADER_BLOCK_SIZE * (1 + devCount)

    /** 大端无符号 16 bit（高字节在前） */
    fun u16be(b: ByteArray, offset: Int): Int =
        ((b[offset].toInt() and 0xFF) shl 8) or (b[offset + 1].toInt() and 0xFF)

    /** 大端有符号 16 bit（高字节在前，负温必须保留符号） */
    fun s16be(b: ByteArray, offset: Int): Int {
        val v = u16be(b, offset)
        return if ((v and 0x8000) != 0) v - 0x10000 else v
    }

    /** 形如 "0C:11:82:24"（大写、冒号分隔），与改动前日志格式一致 */
    fun toColonHex(bytes: ByteArray): String =
        bytes.joinToString(":") { "%02X".format(it) }

    /** 形如 "0C118224"（大写、无分隔），即 MacIdBook 使用的设备 ID 形式 */
    fun toCompactHex(bytes: ByteArray): String =
        bytes.joinToString("") { "%02X".format(it) }

    /** 单条设备读数（解码结果，单位与改动前一致：℃，%RH） */
    data class SensorSample(
        val devIdBytes: ByteArray,   // 4 B，原字节序
        val devIdHex: String,        // "0C:11:82:24"
        val devIdCompact: String,    // "0C118224"（MacIdBook 键）
        val humidityPct: Double,     // u16be@4 / 10.0
        val temperatureC: Double     // s16be@6 / 10.0（有符号）
    )

    /** 拒绝原因 */
    enum class RejectionReason {
        /** 明文不足一个头部块 */
        FRAME_TOO_SHORT,

        /** devCount == 0：不是有效遥测帧，必须拒绝 */
        NO_DEVICE,

        /** 明文长度 < 16 * (1 + devCount) */
        LENGTH_MISMATCH,

        /** 密文长度不是 16 B 的整数倍（AES/ECB/NoPadding 无法解密） */
        CIPHERTEXT_NOT_ALIGNED,

        /** AES 解密过程抛出异常 */
        DECRYPT_FAILED
    }

    /** 解码结果：明确区分"成功"与"拒绝"，拒绝时不携带任何读数 */
    sealed class Result {
        data class Success(
            val gatewayIdHex: String,
            val devCount: Int,
            val samples: List<SensorSample>
        ) : Result()

        data class Rejected(
            val reason: RejectionReason,
            val detail: String
        ) : Result()
    }

    /**
     * 解码解密后的聚合明文帧。
     *
     * 成功条件：`size >= 16`、`devCount >= 1`、`size >= 16 * (1 + devCount)`；
     * 明文长于期望长度（协议栈读满尾部填充）时按 devCount 取数并忽略尾部。
     * 任何不满足条件的情形返回 [Result.Rejected]，不抛异常、不产生读数。
     */
    fun decodeAggregatedFrame(frame: ByteArray): Result {
        if (frame.size < HEADER_BLOCK_SIZE) {
            return Result.Rejected(
                RejectionReason.FRAME_TOO_SHORT,
                "plain=${frame.size}B < header=${HEADER_BLOCK_SIZE}B"
            )
        }

        val devCount = frame[6].toInt() and 0xFF
        if (devCount == 0) {
            return Result.Rejected(
                RejectionReason.NO_DEVICE,
                "devCount=0"
            )
        }

        val expected = expectedFrameSize(devCount)
        if (frame.size < expected) {
            return Result.Rejected(
                RejectionReason.LENGTH_MISMATCH,
                "plain=${frame.size}B < expected=${expected}B (devCount=$devCount)"
            )
        }

        val gatewayIdHex = toColonHex(frame.copyOfRange(0, 6))
        val samples = ArrayList<SensorSample>(devCount)
        var off = HEADER_BLOCK_SIZE
        repeat(devCount) {
            samples += decodeDeviceBlock(frame, off)
            off += DEVICE_BLOCK_SIZE
        }
        return Result.Success(gatewayIdHex, devCount, samples)
    }

    /**
     * 解码单个设备块（16 B，仅前 8 B 有效）。
     * 供需要按块复用的调用方使用；不改变字段偏移与换算。
     */
    fun decodeDeviceBlock(block: ByteArray, offset: Int): SensorSample {
        val p8 = block.copyOfRange(offset, offset + DEVICE_PAYLOAD_SIZE)
        val devIdBytes = p8.copyOfRange(0, 4)
        val humi = u16be(p8, 4) / 10.0
        val temp = s16be(p8, 6) / 10.0
        return SensorSample(
            devIdBytes = devIdBytes,
            devIdHex = toColonHex(devIdBytes),
            devIdCompact = toCompactHex(devIdBytes),
            humidityPct = humi,
            temperatureC = temp
        )
    }
}
