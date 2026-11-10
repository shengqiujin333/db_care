package com.jinyuni.dengbei_care.protocol

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/** AA-005 G-2 / D-17 / D-18：网关 BLE 身份纯逻辑自检 */
class GatewayIdentityTest {

    @Test
    fun normalize_acceptsCommonForms() {
        assertEquals("84:C2:E4:03:02:02", GatewayIdentity.normalize("84:C2:E4:03:02:02"))
        assertEquals("84:C2:E4:03:02:02", GatewayIdentity.normalize("84:c2:e4:3:2:2"))
        assertEquals("84:C2:E4:03:02:02", GatewayIdentity.normalize("84-c2-e4-03-02-02"))
        assertEquals("84:C2:E4:03:02:02", GatewayIdentity.normalize("84C2E4030202"))
        assertEquals("84:C2:E4:03:02:02", GatewayIdentity.normalize("  84C2E4030202  "))
    }

    @Test
    fun normalize_rejectsInvalid() {
        assertNull(GatewayIdentity.normalize(null))
        assertNull(GatewayIdentity.normalize(""))
        assertNull(GatewayIdentity.normalize("84:C2:E4:03:02"))
        assertNull(GatewayIdentity.normalize("84:C2:E4:03:02:02:11"))
        assertNull(GatewayIdentity.normalize("84:C2:E4:03:02:GG"))
        assertNull(GatewayIdentity.normalize("84C2E403020"))
        assertNull(GatewayIdentity.normalize("84C2E40302021"))
        assertFalse(GatewayIdentity.isValid("not-a-mac"))
    }

    @Test
    fun rank_dedupesAndPrefersFfe0ThenRssi() {
        val ranked = GatewayIdentity.rank(
            listOf(
                GatewayIdentity.Candidate("84:C2:E4:03:02:02", "gw-old", -70, false),
                GatewayIdentity.Candidate("84:c2:e4:03:02:02", "gw-new", -60, true),
                GatewayIdentity.Candidate("AA:BB:CC:DD:EE:01", "x", -40, false),
                GatewayIdentity.Candidate("AA:BB:CC:DD:EE:02", "y", -50, true),
                GatewayIdentity.Candidate("bad", "z", -10, true)
            )
        )
        // FFE0 优先，其次 RSSI 降序；同 MAC 合并（保留含 FFE0/更强信号者）
        assertEquals(
            listOf("AA:BB:CC:DD:EE:02", "84:C2:E4:03:02:02", "AA:BB:CC:DD:EE:01"),
            ranked.map { it.mac }
        )
        // 同 MAC 合并为含 FFE0 的更强信号候选
        assertEquals("gw-new", ranked[1].name)
        assertEquals(-60, ranked[1].rssi)
    }

    @Test
    fun bindingState_migrationAndCleared() {
        assertEquals(
            GatewayIdentity.BindingState.DEFAULT_UNCONFIRMED,
            GatewayIdentity.bindingState("", cleared = false)
        )
        assertEquals(
            GatewayIdentity.BindingState.UNSET,
            GatewayIdentity.bindingState("", cleared = true)
        )
        assertEquals(
            GatewayIdentity.BindingState.BOUND,
            GatewayIdentity.bindingState("84:c2:e4:03:02:02", cleared = true)
        )
        assertTrue(GatewayIdentity.isMigrationDefault(null, cleared = false))
        assertFalse(GatewayIdentity.isMigrationDefault(null, cleared = true))
        assertFalse(GatewayIdentity.isMigrationDefault("84:C2:E4:03:02:02", cleared = false))
    }

    @Test
    fun resolveAddress_prefersStored_thenDefault_thenNull() {
        assertEquals(
            "AA:BB:CC:DD:EE:FF",
            GatewayIdentity.resolveAddress("aa-bb-cc-dd-ee-ff", cleared = false)
        )
        assertEquals(
            GatewayIdentity.DEFAULT_GATEWAY_BLE_ADDR,
            GatewayIdentity.resolveAddress(null, cleared = false)
        )
        assertNull(GatewayIdentity.resolveAddress(null, cleared = true))
        assertNull(GatewayIdentity.resolveAddress("invalid", cleared = true))
    }

    @Test
    fun migrationDefault_existsExactlyOnce() {
        // 默认值只在此常量定义；连接路径不得再出现字面量（由静态核查覆盖）
        assertEquals("84:C2:E4:03:02:02", GatewayIdentity.DEFAULT_GATEWAY_BLE_ADDR)
    }
}
