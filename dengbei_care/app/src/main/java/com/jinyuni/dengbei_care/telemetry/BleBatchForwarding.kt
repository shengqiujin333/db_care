package com.jinyuni.dengbei_care.telemetry

import java.util.Locale

/**
 * BLE 读取批次 → MQTT 载荷（AA-005 G-3 / D-20 / D-26，IC-002 §7）。
 *
 * 纯逻辑：
 * - 一次成功读取的全部有效读数作为一个**不可拆分母批次**；
 * - 仅当数量与当前绑定集合一致、ID 均已知且无重复、顺序与绑定顺序逐项匹配时转发；
 * - 载荷逐字为 `<epoch_seconds>:<temps csv>:<humis csv>`：冒号三段、温度在前、逗号分隔、
 *   无空格、小数点固定为 `.`（Locale.US）；
 * - 不满足条件返回 Refuse（不携带任何部分载荷），本地处理不受影响。
 */
object BleBatchForwarding {

    sealed interface Plan {
        data class Forward(val payload: String) : Plan
        data class Refuse(val reason: RefuseReason) : Plan
    }

    enum class RefuseReason {
        NO_BOUND_DEVICE,
        COUNT_MISMATCH,
        UNKNOWN_ID,
        DUPLICATE_ID,
        ORDER_MISMATCH,
        ALL_PLACEHOLDER
    }

    fun plan(
        readings: List<BleObservation.Reading>,
        boundOrder: List<String>,
        timeSeconds: Long,
        allPlaceholder: Boolean = false
    ): Plan {
        val bound = boundOrder.map(::normalizeId)
        if (bound.isEmpty()) {
            return Plan.Refuse(RefuseReason.NO_BOUND_DEVICE)
        }
        if (readings.isEmpty()) {
            return Plan.Refuse(
                if (allPlaceholder) RefuseReason.ALL_PLACEHOLDER else RefuseReason.COUNT_MISMATCH
            )
        }
        if (readings.size != bound.size) {
            return Plan.Refuse(RefuseReason.COUNT_MISMATCH)
        }

        val ids = readings.map { normalizeId(it.devId) }
        if (ids.toSet().size != ids.size) {
            return Plan.Refuse(RefuseReason.DUPLICATE_ID)
        }
        if (ids.any { it !in bound }) {
            return Plan.Refuse(RefuseReason.UNKNOWN_ID)
        }
        if (ids != bound) {
            return Plan.Refuse(RefuseReason.ORDER_MISMATCH)
        }

        val temps = readings.joinToString(",") { formatX10(it.tX10) }
        val humis = readings.joinToString(",") { formatX10(it.hX10) }
        return Plan.Forward("$timeSeconds:$temps:$humis")
    }

    /** 0.1 单位整数 → 固定一位小数（负温保留符号，Locale.US） */
    fun formatX10(value: Int): String = String.format(Locale.US, "%.1f", value / 10.0)

    private fun normalizeId(devId: String): String =
        devId.replace(":", "").replace("-", "").uppercase()
}
