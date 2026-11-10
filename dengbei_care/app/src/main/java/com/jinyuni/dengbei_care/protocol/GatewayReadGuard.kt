package com.jinyuni.dengbei_care.protocol

/**
 * BLE 快照容量与读取完整性守卫（AA-005 D-31 / D-32 / G-7，IC-002 v4.0 §6）。
 *
 * 纯逻辑，不引用 Android 类型。
 *
 * 依据：FFE2 属性 160 B、设备块 16 B，完整快照最多 9 个设备（16 + 9×16 = 160；10 台需 176 B）。
 * 超出容量时必须**明确拒绝**，不得截断、少报或构造部分批次。
 *
 * 读取完整性：GATT 读单次返回长度受 MTU 限制（MTU−3）。只有协商 MTU 足以承载
 * `16×(1+N)` 且实际返回密文覆盖完整快照（16 B 对齐）时才允许解码；不足即整帧拒绝，
 * 不补零、不拼接跨次快照、不裁掉余字节（16 B 对齐的多余尾块按 devCount 消费）。
 */
object GatewayReadGuard {

    /** 完整快照可承载的最大设备数（160 B / 16 B − 1 个头块） */
    const val MAX_DEVICES = 9

    /** FFE2 属性长度上限（字节） */
    const val MAX_FRAME_BYTES = 160

    /** GATT 读：单次返回长度上限 = MTU − 3 */
    const val ATT_OVERHEAD = 3

    /** 期望语义帧长：16 × (1 + devCount) */
    fun expectedFrameSize(devCount: Int): Int {
        val n = devCount.coerceAtLeast(0)
        return 16 * (1 + n)
    }

    sealed interface Capacity {
        data object Ok : Capacity
        data class Exceeded(val bound: Int) : Capacity
    }

    enum class ReadRefuseReason {
        MTU_TOO_SMALL,
        LENGTH_TOO_SMALL,
        LENGTH_NOT_ALIGNED
    }

    sealed interface MtuVerdict {
        data object Proceed : MtuVerdict
        data class Refuse(val requiredMtu: Int, val actualMtu: Int) : MtuVerdict
    }

    sealed interface LengthVerdict {
        data class Proceed(val expectedBytes: Int) : LengthVerdict
        data class Refuse(
            val reason: ReadRefuseReason,
            val expectedBytes: Int,
            val actualBytes: Int
        ) : LengthVerdict
    }

    fun capacityVerdict(boundCount: Int): Capacity =
        if (boundCount in 0..MAX_DEVICES) Capacity.Ok else Capacity.Exceeded(boundCount)

    fun mtuVerdict(negotiatedMtu: Int, devCount: Int): MtuVerdict {
        val required = expectedFrameSize(devCount) + ATT_OVERHEAD
        return if (negotiatedMtu >= required) {
            MtuVerdict.Proceed
        } else {
            MtuVerdict.Refuse(requiredMtu = required, actualMtu = negotiatedMtu)
        }
    }

    /**
     * 判定顺序固定：先判 16 B 对齐，再判是否覆盖期望长度（避免“20 B / N=1”被归类不明确）。
     * 返回长度大于期望且 16 B 对齐时视为可解码（codec 只消费 devCount 个设备块）。
     */
    fun lengthVerdict(returnedBytes: Int, devCount: Int): LengthVerdict {
        val expected = expectedFrameSize(devCount)
        return when {
            returnedBytes < 0 || returnedBytes % 16 != 0 -> LengthVerdict.Refuse(
                ReadRefuseReason.LENGTH_NOT_ALIGNED, expected, returnedBytes
            )
            returnedBytes < expected -> LengthVerdict.Refuse(
                ReadRefuseReason.LENGTH_TOO_SMALL, expected, returnedBytes
            )
            else -> LengthVerdict.Proceed(expected)
        }
    }
}
