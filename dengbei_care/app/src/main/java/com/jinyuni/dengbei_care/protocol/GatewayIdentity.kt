package com.jinyuni.dengbei_care.protocol

/**
 * 网关 BLE 连接身份（AA-005 D-17 / D-18 / G-2）。
 *
 * 纯逻辑：MAC 规范化与校验、扫描候选去重排序、迁移默认值判定。
 *
 * 身份分离：
 * - [PREF_GATEWAY_BLE_ADDR] 仅用于 GATT 连接（冒号分隔 MAC）；
 * - `mac_addr` 仍是网关平台身份（MQTT topic 与 `/upload_data.mac`），两者互不推导。
 */
object GatewayIdentity {

    /** 唯一迁移默认值：只允许在此处出现一次；清除后不得回退 */
    const val DEFAULT_GATEWAY_BLE_ADDR = "84:C2:E4:03:02:02"

    const val PREF_GATEWAY_BLE_ADDR = "gateway_ble_addr"
    const val PREF_GATEWAY_BLE_ADDR_CLEARED = "gateway_ble_addr_cleared"

    data class Candidate(
        val mac: String,
        val name: String?,
        val rssi: Int,
        val hasFfe0: Boolean
    )

    enum class BindingState {
        /** 未配置（用户已清除或从未选择） */
        UNSET,
        /** 正在使用迁移默认值，界面必须标注“未确认” */
        DEFAULT_UNCONFIRMED,
        /** 用户已确认并保存 */
        BOUND
    }

    /**
     * 规范化 MAC：接受 `AA:BB:CC:DD:EE:FF`、`aa-bb-cc-dd-ee-ff`、`AABBCCDDEEFF`
     * （各段允许 1–2 位），输出大写冒号形式；非法返回 null。
     */
    fun normalize(raw: String?): String? {
        val s = raw?.trim().orEmpty()
        if (s.isEmpty()) return null

        val hexOnly: String = when {
            s.contains(':') || s.contains('-') -> {
                val parts = s.split(':', '-')
                if (parts.size != 6) return null
                if (parts.any { it.isEmpty() || it.length > 2 || !it.all(::isHex) }) return null
                parts.joinToString("") { it.padStart(2, '0') }
            }
            else -> {
                if (s.length != 12 || !s.all(::isHex)) return null
                s
            }
        }
        if (hexOnly.length != 12) return null
        return hexOnly.uppercase().chunked(2).joinToString(":")
    }

    fun isValid(raw: String?): Boolean = normalize(raw) != null

    private fun isHex(c: Char): Boolean =
        (c in '0'..'9') || (c in 'a'..'f') || (c in 'A'..'F')

    /** 候选去重（同 MAC 取更强信号/含 FFE0 者）并按 FFE0 优先、RSSI 降序、MAC 升序排序 */
    fun rank(candidates: List<Candidate>): List<Candidate> {
        val byMac = LinkedHashMap<String, Candidate>()
        candidates.forEach { c ->
            val mac = normalize(c.mac) ?: return@forEach
            val normalized = c.copy(mac = mac)
            val old = byMac[mac]
            byMac[mac] = when {
                old == null -> normalized
                normalized.hasFfe0 && !old.hasFfe0 -> normalized
                !normalized.hasFfe0 && old.hasFfe0 -> old
                normalized.rssi > old.rssi -> normalized
                else -> old
            }
        }
        return byMac.values.sortedWith(
            compareByDescending<Candidate> { it.hasFfe0 }
                .thenByDescending { it.rssi }
                .thenBy { it.mac }
        )
    }

    fun bindingState(stored: String?, cleared: Boolean): BindingState {
        val normalized = normalize(stored)
        return when {
            normalized != null -> BindingState.BOUND
            cleared -> BindingState.UNSET
            else -> BindingState.DEFAULT_UNCONFIRMED
        }
    }

    fun isMigrationDefault(stored: String?, cleared: Boolean): Boolean =
        bindingState(stored, cleared) == BindingState.DEFAULT_UNCONFIRMED

    /** 实际 GATT 目标：已保存值优先；未保存且未清除时用迁移默认；已清除返回 null */
    fun resolveAddress(stored: String?, cleared: Boolean): String? = when (bindingState(stored, cleared)) {
        BindingState.BOUND -> normalize(stored)
        BindingState.DEFAULT_UNCONFIRMED -> DEFAULT_GATEWAY_BLE_ADDR
        BindingState.UNSET -> null
    }
}
