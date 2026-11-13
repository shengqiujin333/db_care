package com.jinyuni.dengbei_care.ui.notifications

import java.io.File
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * AA-006 N-1：扫描权限接线的宿主静态回归。
 *
 * 说明：真实权限弹窗与授权后自动扫描需真机验收；本测试只防止接线退回到
 * “Activity 级 requestPermissions + 不可达回调”的旧结构。
 */
class ScanPermissionWiringTest {

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

    @Test
    fun notificationsFragment_usesFragmentResultLauncher_forScanPermission() {
        val code = withoutComments(
            source("ui/notifications/NotificationsFragment.kt")
        )
        assertTrue(
            "必须用 fragment registerForActivityResult 接收扫描权限结果",
            code.contains("registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions())")
        )
        assertTrue("launcher 结果必须交给当前网关对话框", code.contains("onPermissionResult(result)"))
        assertFalse(
            "不得保留旧的 Activity 级权限请求包装",
            code.contains("requestPermissionsCompat")
        )
        assertFalse(
            "不得再按 requestCode 依赖 Activity 的权限回调",
            code.contains("REQUEST_SCAN_PERMISSIONS")
        )
    }

    @Test
    fun gatewayDialog_requestsViaCallback_andExplainsRealReason() {
        val code = withoutComments(
            source("ui/gateway/GatewayBleSettingsDialog.kt")
        )
        assertFalse(
            "对话框不得直接调用 ActivityCompat.requestPermissions",
            code.contains("ActivityCompat.requestPermissions")
        )
        assertTrue("权限结果必须重新按策略判定", code.contains("policy.canScan"))
        assertTrue("失败原因必须来自策略 userMessage", code.contains("policy.userMessage"))
        assertFalse(
            "不得再用笼统的未授权提示",
            code.contains("未授予扫描权限，无法搜索网关")
        )
    }

    @Test
    fun gatewayDialog_clearsPendingScan_onResult() {
        val code = withoutComments(
            source("ui/gateway/GatewayBleSettingsDialog.kt")
        )
        val start = code.indexOf("fun onPermissionResult(")
        assertTrue(start >= 0)
        val body = code.substring(start, minOf(code.length, start + 900))
        assertTrue("结果处理必须先取出并清除挂起动作", body.contains("pendingScan"))
        assertTrue("无动作时应直接返回", body.contains("if (action == null) return"))
    }
}
