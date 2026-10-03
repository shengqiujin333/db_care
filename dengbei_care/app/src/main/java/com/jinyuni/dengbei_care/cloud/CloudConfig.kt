package com.jinyuni.dengbei_care.cloud

/**
 * 平台服务器常量（唯一来源，AA-002 D-13）。
 *
 * 迁移事实来源：`服务器迁移记录.md`（京东云 → 阿里云，2026-09-25~26）：
 * - EMQX MQTT Broker：`8883` SSL/TLS（App 连接使用）、`1883` 明文、`8083/8084` WS/WSS；
 * - 注册/上传服务 `abr.py`（Flask）：`5000`；
 * - 认证与证书**原样迁移**，App 侧只需改 IP：信任锚仍为 `assets/jd-ca.crt`、`TLSv1.2`、
 *   用户名 = 网关 MAC、密码 = MAC + `&^!A:z?`（均在 MqtttService.setupSSL 中保持不变）。
 *
 * 旧京东云主机已退役，其地址不得再作为连接目标出现在 App 源码中（仅 `network_security_config.xml`
 * 按设计“追加而非替换”保留其历史明文放行条目）。
 */
object CloudConfig {

    /** 平台服务器 IPv4（阿里云） */
    const val SERVER_HOST = "8.140.23.253"

    /** EMQX MQTT over TLS 端口 */
    const val MQTT_BROKER_PORT = 8883

    /** 注册/上传 HTTP 端口（abr.py，明文 HTTP） */
    const val HTTP_PORT = 5000

    /** MQTT broker URI（TLS；信任锚与认证见 setupSSL，不在此处） */
    const val MQTT_BROKER_URI = "ssl://$SERVER_HOST:$MQTT_BROKER_PORT"

    /** 注册/登录/上传 HTTP 基址（明文放行见 res/xml/network_security_config.xml） */
    const val HTTP_BASE_URL = "http://$SERVER_HOST:$HTTP_PORT"
}
