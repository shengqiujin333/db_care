package com.jinyuni.dengbei_care.cloud

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

/**
 * ITEM-004 实现侧自检：平台服务器常量集中化与明文放行。
 *
 * 覆盖 TD-SW-002 §4.1 T-SW-L0-13（服务器常量）与 §4.3 T-SW-L1-06（明文放行最小化）。
 */
class CloudConfigTest {

    private val newHost = "8.140.23.253"
    private val hotspotHost = "192.168.4.1"

    @Test
    fun brokerUri_andHttpBaseUrl_pointToNewServer() {
        assertEquals("ssl://8.140.23.253:8883", CloudConfig.MQTT_BROKER_URI)
        assertEquals("http://8.140.23.253:5000", CloudConfig.HTTP_BASE_URL)
        assertEquals(newHost, CloudConfig.SERVER_HOST)
        assertEquals(8883, CloudConfig.MQTT_BROKER_PORT)
        assertEquals(5000, CloudConfig.HTTP_PORT)
    }

    @Test
    fun constants_areComposedFromSingleHostAndPorts() {
        assertEquals("ssl://${CloudConfig.SERVER_HOST}:${CloudConfig.MQTT_BROKER_PORT}", CloudConfig.MQTT_BROKER_URI)
        assertEquals("http://${CloudConfig.SERVER_HOST}:${CloudConfig.HTTP_PORT}", CloudConfig.HTTP_BASE_URL)
        assertTrue(CloudConfig.MQTT_BROKER_URI.startsWith("ssl://"))
        assertTrue(CloudConfig.HTTP_BASE_URL.startsWith("http://"))
    }

    @Test
    fun networkSecurityConfig_permitsNewHost_andKeepsExistingOnes_only() {
        val xml = readNetworkSecurityConfig()

        val domains = Regex("<domain[^>]*>([^<]+)</domain>")
            .findAll(xml)
            .map { it.groupValues[1].trim() }
            .toList()

        assertTrue("must permit $newHost (cleartext upload)", domains.contains(newHost))
        assertTrue("must keep local hotspot $hotspotHost", domains.contains(hotspotHost))
        // 设计要求“追加而非替换”：第三个条目是既有（已退役）主机，故总数仍为 3，且不得新增其它主机。
        // 此处不写退役主机字面值，以保持 Android 源码/测试中不出现已退役地址。
        assertEquals("only the new host may be appended; no other hosts permitted", 3, domains.size)
        assertEquals("the new host must not duplicate an existing entry", 3, domains.distinct().size)

        // 每个 domain-config 都必须显式允许明文（App 用 http:// 访问 5000 端口）
        val configCount = Regex("<domain-config[^>]*cleartextTrafficPermitted=\"true\"[^>]*>")
            .findAll(xml).count()
        assertEquals(3, configCount)
    }

    private fun readNetworkSecurityConfig(): String {
        val candidates = listOf(
            File("src/main/res/xml/network_security_config.xml"),
            File("app/src/main/res/xml/network_security_config.xml")
        )
        val file = candidates.firstOrNull { it.isFile }
            ?: throw AssertionError("network_security_config.xml not found from ${File(".").absolutePath}")
        return file.readText()
    }
}
