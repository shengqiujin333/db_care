package com.jinyuni.dengbei_care.ui.notifications

import java.io.File
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 产品验收 D-01/D-02/D-03/D-04a 的布局/文案结构回归（实现侧，宿主机）。
 *
 * 只读取仓库内资源 XML，验证修复结构不退回：
 * - NavHost 高度受约束（不再 match_parent 覆盖底部导航）；
 * - 「我的」页为滚动容器、无 parent 底部锚定、控件顺序不重叠、快速入口置顶；
 * - 首页空态文案指向实际可达入口。
 *
 * 真实控件 bounds 与点击行为仍需真机复验（本测试不替代）。
 */
class NotificationsLayoutContractTest {

    private fun layout(name: String): String {
        // Gradle 单元测试工作目录为 app/ 模块目录
        val candidates = listOf(
            File("src/main/res/layout/$name"),
            File("app/src/main/res/layout/$name")
        )
        val file = candidates.firstOrNull { it.isFile }
            ?: error("layout not found: $name (cwd=${File(".").absolutePath})")
        return file.readText()
    }

    @Test
    fun navHostHeight_isConstraintBounded_aboveBottomNavigation() {
        val xml = layout("activity_main.xml")
        val block = Regex(
            "<fragment[^>]*nav_host_fragment_activity_main(.*?)/>",
            RegexOption.DOT_MATCHES_ALL
        ).find(xml)?.groupValues?.get(1) ?: error("NavHost block missing")
        assertTrue("NavHost 必须使用 0dp 高度", block.contains("android:layout_height=\"0dp\""))
        assertTrue(
            "NavHost 底部必须约束到 nav_view 顶部",
            block.contains("app:layout_constraintBottom_toTopOf=\"@id/nav_view\"")
        )
        assertFalse("NavHost 不得再用 match_parent 高度", block.contains("android:layout_height=\"match_parent\""))
    }

    @Test
    fun notifications_isScrollable_andHasNoBottomParentAnchoring() {
        val xml = layout("fragment_notifications.xml")
        assertTrue("根布局必须是 NestedScrollView", xml.contains("<androidx.core.widget.NestedScrollView"))
        assertTrue("必须启用 fillViewport", xml.contains("android:fillViewport=\"true\""))
        assertFalse("不得再用 bottom_toBottomOf=parent 固定底部", xml.contains("bottom_toBottomOf=\"parent\""))
    }

    @Test
    fun notifications_controlsAreOrderedAndDoNotOverlap() {
        val xml = layout("fragment_notifications.xml")
        val required = listOf(
            "usernameText", "registerButton", "sensorAddressLabel", "sensorAddressInput",
            "macSendButton", "macScanButton", "sensoridinfo", "sensoridinput",
            "sensoridscan", "sensornameinput", "addsensorid", "opensensorlistbutton",
            "viewReportButton", "gatewayBleButton"
        )
        required.forEach { id ->
            assertTrue("缺少控件 id=$id", xml.contains("@+id/$id\""))
        }
        val add = xml.indexOf("@+id/addsensorid\"")
        val gateway = xml.indexOf("@+id/gatewayBleButton\"")
        assertTrue("注册传感器必须排在网关设置之前", add in 0 until gateway)
        val report = xml.indexOf("@+id/viewReportButton\"")
        val address = xml.indexOf("@+id/sensorAddressLabel\"")
        val list = xml.indexOf("@+id/opensensorlistbutton\"")
        assertTrue("报告入口应位于页面顶部（地址区之前）", report in 0 until address)
        assertTrue("传感器列表入口应位于页面顶部（地址区之前）", list in 0 until address)
    }

    @Test
    fun homeHint_pointsToExistingEntries() {
        val xml = layout("fragment_home.xml")
        assertTrue("首页引导应指向「去添加」", xml.contains("去添加"))
        assertTrue("首页引导应指向「打开传感器列表」", xml.contains("打开传感器列表"))
        assertFalse("不得保留旧的「我的」→「传感器列表」路径", xml.contains("「我的」→「传感器列表」"))
    }
}
