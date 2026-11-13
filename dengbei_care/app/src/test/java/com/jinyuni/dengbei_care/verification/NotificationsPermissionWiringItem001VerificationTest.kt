package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.ble.BlePermissionPolicy
import com.jinyuni.dengbei_care.ble.LinkReason
import com.jinyuni.dengbei_care.ble.PermissionNeed
import java.io.File
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * AA-006 N-1 独立验证（软件测试，TD-SW-005 §4/§5.1）。
 *
 * 期望表独立于实现自检：策略原因/文案矩阵 + 接线静态核查。
 * 真实权限弹窗、授权后自动扫描、拒绝/不再询问、销毁清理仍需真机验收，本文件不替代。
 */
class NotificationsPermissionWiringItem001VerificationTest {

    private fun source(relative: String): String {
        val candidates = listOf(
            File("src/main/java/com/jinyuni/dengbei_care/$relative"),
            File("app/src/main/java/com/jinyuni/dengbei_care/$relative")
        )
        val file = candidates.firstOrNull { it.isFile }
            ?: error("source not found: $relative (cwd=${File(".").absolutePath})")
        return file.readText()
    }

    private fun withoutComments(text: String): String =
        text.replace(Regex("/\\*.*?\\*/", RegexOption.DOT_MATCHES_ALL), "")
            .replace(Regex("//.*"), "")

    private fun bodyOf(code: String, signature: String): String {
        val start = code.indexOf(signature)
        assertTrue("未找到方法 $signature", start >= 0)
        val end = code.indexOf("\n    private fun ", start + signature.length)
            .let { if (it < 0) code.length else it }
        return code.substring(start, end)
    }

    // N1-H-01：canScan=false 的每种输入给出不同且具体的原因
    @Test
    fun policyReasons_areSpecific_whenScanBlocked() {
        val api29 = 29
        val api31 = 31

        val btOff = BlePermissionPolicy.evaluate(
            sdkInt = api29,
            granted = setOf(PermissionNeed.FINE_LOCATION),
            needScan = true,
            adapterEnabled = false,
            locationEnabled = true
        )
        assertEquals(LinkReason.BLUETOOTH_OFF, btOff.reason)
        assertEquals(false, btOff.canScan)
        assertTrue(btOff.userMessage.contains("蓝牙未开启"))

        val locOff = BlePermissionPolicy.evaluate(
            sdkInt = api29,
            granted = setOf(PermissionNeed.FINE_LOCATION),
            needScan = true,
            adapterEnabled = true,
            locationEnabled = false
        )
        assertEquals(LinkReason.LOCATION_OFF, locOff.reason)
        assertEquals(false, locOff.canScan)
        assertTrue(locOff.userMessage.contains("需要开启定位"))

        val locPermMissing = BlePermissionPolicy.evaluate(
            sdkInt = api29,
            granted = emptySet(),
            needScan = true,
            adapterEnabled = true,
            locationEnabled = true
        )
        assertEquals(LinkReason.LOCATION_PERMISSION_MISSING, locPermMissing.reason)
        assertEquals(false, locPermMissing.canScan)
        assertTrue(locPermMissing.userMessage.contains("定位权限"))

        val scanPermMissing = BlePermissionPolicy.evaluate(
            sdkInt = api31,
            granted = setOf(PermissionNeed.BLUETOOTH_CONNECT),
            needScan = true,
            adapterEnabled = true,
            locationEnabled = true
        )
        assertEquals(LinkReason.SCAN_PERMISSION_MISSING, scanPermMissing.reason)
        assertEquals(false, scanPermMissing.canScan)
        assertTrue(scanPermMissing.userMessage.contains("附近设备权限"))

        val messages = listOf(
            btOff.userMessage,
            locOff.userMessage,
            locPermMissing.userMessage,
            scanPermMissing.userMessage
        )
        assertEquals("被阻塞原因不得共用同一句笼统文案", messages.size, messages.toSet().size)
        assertFalse(
            "不得统一为旧的笼统提示",
            messages.any { it.contains("未授予扫描权限，无法搜索网关") }
        )
    }

    // N1-H-01（放行侧）：API29 定位已授予且开启 → 可扫描；API31 双权限齐备 → 可扫描
    @Test
    fun policyAllowsScan_whenAllPrerequisitesMet() {
        val api29 = BlePermissionPolicy.evaluate(
            sdkInt = 29,
            granted = setOf(PermissionNeed.FINE_LOCATION),
            needScan = true,
            adapterEnabled = true,
            locationEnabled = true
        )
        assertTrue(api29.canScan)
        assertEquals(LinkReason.OK, api29.reason)

        val api31 = BlePermissionPolicy.evaluate(
            sdkInt = 31,
            granted = setOf(PermissionNeed.BLUETOOTH_CONNECT, PermissionNeed.BLUETOOTH_SCAN),
            needScan = true,
            adapterEnabled = true,
            locationEnabled = false
        )
        assertTrue(api31.canScan)
        assertEquals(LinkReason.OK, api31.reason)
    }

    // N1-S-01：权限结果由 Fragment launcher 发起/消费
    @Test
    fun fragmentOwnsPermissionLauncher_andConsumesResult() {
        val code = withoutComments(source("ui/notifications/NotificationsFragment.kt"))
        assertTrue(
            "必须注册 RequestMultiplePermissions launcher",
            code.contains("registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions())")
        )
        assertTrue("launcher 必须把结果交给对话框", code.contains("onPermissionResult(result)"))
        assertFalse("不得保留旧包装入口", code.contains("requestPermissionsCompat"))
        assertTrue(
            "扫描按钮路径必须通过 launcher 发起请求",
            code.contains("scanPermissionLauncher.launch")
        )
    }

    // N1-S-02：对话框无 Activity 级权限请求；停止/销毁时清除挂起扫描
    @Test
    fun dialogUsesCallback_andClearsPendingScanOnStop() {
        val code = withoutComments(source("ui/gateway/GatewayBleSettingsDialog.kt"))
        assertFalse(
            "扫描权限不得由对话框直接向 Activity 请求",
            code.contains("ActivityCompat.requestPermissions")
        )
        val stop = bodyOf(code, "fun stopScan()")
        assertTrue("stopScan 必须清除挂起扫描", stop.contains("pendingScan = null"))
        assertTrue("stopScan 必须停止扫描器", stop.contains("discovery?.stop"))
        val dismiss = bodyOf(code, "fun dismiss()")
        assertTrue("dismiss 必须先停止扫描/清除挂起", dismiss.contains("stopScan()"))
    }

    // N1-S-03：启动、保存、清除路径不请求权限
    @Test
    fun saveAndClearPaths_doNotRequestPermissions() {
        val code = withoutComments(source("ui/gateway/GatewayBleSettingsDialog.kt"))
        listOf("fun saveManual()", "fun clearGateway()").forEach { signature ->
            val body = bodyOf(code, signature)
            assertFalse("$signature 不得发起权限请求", body.contains("requestPermissions("))
            assertFalse("$signature 不得触发扫描", body.contains("startScan("))
        }

        val fragment = withoutComments(source("ui/notifications/NotificationsFragment.kt"))
        val saveInput = bodyOf(fragment, "private fun saveInput(")
        assertFalse("保存平台地址不得请求定位", saveInput.contains("ACCESS_FINE_LOCATION"))
        val loadInput = bodyOf(fragment, "private fun loadInput(")
        assertFalse("启动载入不得请求权限", loadInput.contains("requestPermissions("))
    }

    // N1-S-03：扫描请求集合按 API 区分，API29 只请求定位，API31+ 不请求定位
    @Test
    fun scanRequestSets_matchApiLevel() {
        val api29 = BlePermissionPolicy.scanRequestPermissions(29, emptySet())
        assertEquals(listOf(PermissionNeed.FINE_LOCATION), api29)

        val api31 = BlePermissionPolicy.scanRequestPermissions(31, emptySet())
        assertEquals(
            listOf(PermissionNeed.BLUETOOTH_CONNECT, PermissionNeed.BLUETOOTH_SCAN),
            api31
        )
        assertFalse("API31+ 扫描请求不得包含定位", api31.contains(PermissionNeed.FINE_LOCATION))
    }
}
