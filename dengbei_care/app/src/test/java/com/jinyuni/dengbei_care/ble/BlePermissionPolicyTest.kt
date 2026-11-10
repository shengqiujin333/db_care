package com.jinyuni.dengbei_care.ble

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * AA-005 G-1 / D-16：BLE 权限策略宿主机自检。
 *
 * 矩阵覆盖 SDK 26/29/30/31/33 × 直连/扫描 × 授予/拒绝 × 适配器/定位开关。
 */
class BlePermissionPolicyTest {

    private fun eval(
        sdk: Int,
        granted: Set<PermissionNeed> = emptySet(),
        needScan: Boolean = false,
        adapter: Boolean = true,
        location: Boolean = true
    ) = BlePermissionPolicy.evaluate(sdk, granted, needScan, adapter, location)

    @Test
    fun directConnect_api29_neverRequiresFineLocation() {
        // 未授予任何运行时权限、定位关闭：API<31 直连仍应允许
        val p = eval(29, granted = emptySet(), needScan = false, adapter = true, location = false)
        assertTrue(p.canConnect)
        assertFalse(p.missing.contains(PermissionNeed.FINE_LOCATION))
        assertEquals(LinkReason.OK, p.reason)
        assertTrue(p.userMessage.isBlank())
    }

    @Test
    fun directConnect_api31_requiresBluetoothConnect() {
        val denied = eval(31, granted = emptySet(), needScan = false)
        assertFalse(denied.canConnect)
        assertEquals(LinkReason.CONNECT_PERMISSION_MISSING, denied.reason)
        assertEquals(listOf(PermissionNeed.BLUETOOTH_CONNECT), denied.missing)
        assertTrue(denied.userMessage.contains("附近设备"))

        val granted = eval(31, granted = setOf(PermissionNeed.BLUETOOTH_CONNECT))
        assertTrue(granted.canConnect)
        assertEquals(LinkReason.OK, granted.reason)
    }

    @Test
    fun adapterOff_isBluetoothOff_andBlocksBoth() {
        val p = eval(29, granted = setOf(PermissionNeed.FINE_LOCATION), needScan = true, adapter = false)
        assertFalse(p.canConnect)
        assertFalse(p.canScan)
        assertEquals(LinkReason.BLUETOOTH_OFF, p.reason)
        assertTrue(p.userMessage.contains("蓝牙未开启"))
    }

    @Test
    fun scan_api29_requiresLocationPermission_andLocationServices() {
        val noPermission = eval(29, granted = emptySet(), needScan = true, location = true)
        assertFalse(noPermission.canScan)
        assertTrue(noPermission.requiresLocationServices)
        assertEquals(LinkReason.LOCATION_PERMISSION_MISSING, noPermission.reason)
        assertEquals(listOf(PermissionNeed.FINE_LOCATION), noPermission.missing)

        val permissionButServicesOff = eval(
            29,
            granted = setOf(PermissionNeed.FINE_LOCATION),
            needScan = true,
            location = false
        )
        assertFalse(permissionButServicesOff.canScan)
        assertEquals(LinkReason.LOCATION_OFF, permissionButServicesOff.reason)

        val ready = eval(29, granted = setOf(PermissionNeed.FINE_LOCATION), needScan = true, location = true)
        assertTrue(ready.canScan)
        assertEquals(LinkReason.OK, ready.reason)
    }

    @Test
    fun scan_api31_requiresScan_andConnect() {
        val p = eval(31, granted = emptySet(), needScan = true)
        assertFalse(p.canScan)
        assertEquals(LinkReason.CONNECT_PERMISSION_MISSING, p.reason)
        assertTrue(p.missing.containsAll(
            listOf(PermissionNeed.BLUETOOTH_CONNECT, PermissionNeed.BLUETOOTH_SCAN)
        ))

        val onlyScan = eval(31, granted = setOf(PermissionNeed.BLUETOOTH_SCAN), needScan = true)
        assertFalse(onlyScan.canScan)
        assertEquals(LinkReason.CONNECT_PERMISSION_MISSING, onlyScan.reason)

        val ready = eval(
            31,
            granted = setOf(PermissionNeed.BLUETOOTH_CONNECT, PermissionNeed.BLUETOOTH_SCAN),
            needScan = true
        )
        assertTrue(ready.canConnect)
        assertTrue(ready.canScan)
        assertEquals(LinkReason.OK, ready.reason)
    }

    @Test
    fun scan_api31_missingOnlyScan() {
        val p = eval(31, granted = setOf(PermissionNeed.BLUETOOTH_CONNECT), needScan = true)
        assertEquals(LinkReason.SCAN_PERMISSION_MISSING, p.reason)
        assertEquals(listOf(PermissionNeed.BLUETOOTH_SCAN), p.missing)
    }

    @Test
    fun requestSets_onlyDeclaredAndApiRelevant() {
        assertEquals(
            emptyList<PermissionNeed>(),
            BlePermissionPolicy.connectRequestPermissions(29, emptySet())
        )
        assertEquals(
            listOf(PermissionNeed.BLUETOOTH_CONNECT),
            BlePermissionPolicy.connectRequestPermissions(31, emptySet())
        )
        assertEquals(
            emptyList<PermissionNeed>(),
            BlePermissionPolicy.connectRequestPermissions(31, setOf(PermissionNeed.BLUETOOTH_CONNECT))
        )
        // API<31 扫描只请求定位；API31+ 只请求 SCAN/CONNECT，绝不请求定位
        assertEquals(
            listOf(PermissionNeed.FINE_LOCATION),
            BlePermissionPolicy.scanRequestPermissions(29, emptySet())
        )
        assertEquals(
            listOf(PermissionNeed.BLUETOOTH_CONNECT, PermissionNeed.BLUETOOTH_SCAN),
            BlePermissionPolicy.scanRequestPermissions(31, emptySet())
        )
        assertEquals(
            listOf(PermissionNeed.BLUETOOTH_SCAN),
            BlePermissionPolicy.scanRequestPermissions(
                31, setOf(PermissionNeed.BLUETOOTH_CONNECT)
            )
        )
        assertFalse(
            BlePermissionPolicy.scanRequestPermissions(31, emptySet())
                .contains(PermissionNeed.FINE_LOCATION)
        )
    }

    @Test
    fun notificationPermission_isApi33Only() {
        assertFalse(BlePermissionPolicy.needsNotificationPermission(32, emptySet()))
        assertTrue(BlePermissionPolicy.needsNotificationPermission(33, emptySet()))
        assertFalse(
            BlePermissionPolicy.needsNotificationPermission(
                33, setOf(PermissionNeed.POST_NOTIFICATIONS)
            )
        )
    }

    @Test
    fun userMessages_areActionable() {
        assertTrue(
            BlePermissionPolicy.evaluate(31, emptySet(), false, true, true).userMessage.contains("去授权")
        )
        val locationOff = BlePermissionPolicy.evaluate(
            29,
            setOf(PermissionNeed.FINE_LOCATION),
            needScan = true,
            adapterEnabled = true,
            locationEnabled = false
        )
        assertTrue(locationOff.userMessage.contains("开启定位"))
    }
}
