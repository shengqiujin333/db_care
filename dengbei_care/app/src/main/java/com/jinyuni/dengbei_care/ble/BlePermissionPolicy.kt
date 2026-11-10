package com.jinyuni.dengbei_care.ble

/**
 * BLE 运行时权限策略（AA-005 D-16 / G-1）。
 *
 * 纯逻辑：不引用任何 Android 类型，可在宿主机 JVM 直接断言。
 *
 * 规则（与 IC-002 / AA-005 §7.2 一致）：
 * - API 26–30 **直连 GATT** 只需要清单中的普通蓝牙权限（安装时授予），**不要求** `ACCESS_FINE_LOCATION`；
 * - API 31+ 直连需要 `BLUETOOTH_CONNECT`；
 * - 仅当实际执行 BLE **扫描**时：API<31 才要求 `ACCESS_FINE_LOCATION` 且定位总开关开启；
 *   API 31+ 要求 `BLUETOOTH_SCAN`。
 *
 * 注意：`ACCESS_FINE_LOCATION` 仍保留在 Manifest（既有 Wi-Fi 配网页 GatewayFragment 需要），
 * 本策略只决定“BLE 连接/扫描是否需要它”，不改变清单声明。
 */
enum class PermissionNeed {
    BLUETOOTH_CONNECT,
    BLUETOOTH_SCAN,
    FINE_LOCATION,
    POST_NOTIFICATIONS
}

enum class LinkReason {
    OK,
    BLUETOOTH_OFF,
    CONNECT_PERMISSION_MISSING,
    SCAN_PERMISSION_MISSING,
    LOCATION_PERMISSION_MISSING,
    LOCATION_OFF
}

data class Policy(
    val canConnect: Boolean,
    val canScan: Boolean,
    /** 当前请求场景下真正缺失、需要用户处理的权限（顺序稳定） */
    val missing: List<PermissionNeed>,
    /** 仅 needScan 且 API<31 时为 true：扫描还要求系统定位总开关开启 */
    val requiresLocationServices: Boolean,
    val reason: LinkReason,
    /** 面向用户的一句原因与动作；OK 时为空串 */
    val userMessage: String
)

object BlePermissionPolicy {

    const val API_S = 31
    const val API_TIRAMISU = 33

    fun evaluate(
        sdkInt: Int,
        granted: Set<PermissionNeed>,
        needScan: Boolean,
        adapterEnabled: Boolean,
        locationEnabled: Boolean
    ): Policy {
        val missing = mutableListOf<PermissionNeed>()
        val modern = sdkInt >= API_S

        // 直连：API31+ 需要 BLUETOOTH_CONNECT；API<31 直连不要求定位权限
        val connectMissing = modern && PermissionNeed.BLUETOOTH_CONNECT !in granted
        if (connectMissing) missing += PermissionNeed.BLUETOOTH_CONNECT

        // 扫描：API31+ 需要 BLUETOOTH_SCAN；API<31 需要 FINE_LOCATION（且定位总开关开启）
        val requiresLocationServices = needScan && !modern
        val scanMissing = when {
            !needScan -> null
            modern && PermissionNeed.BLUETOOTH_SCAN !in granted -> PermissionNeed.BLUETOOTH_SCAN
            !modern && PermissionNeed.FINE_LOCATION !in granted -> PermissionNeed.FINE_LOCATION
            else -> null
        }
        if (scanMissing != null) missing += scanMissing

        val canConnect = adapterEnabled && !connectMissing
        val canScan = when {
            !needScan -> false
            !adapterEnabled -> false
            modern -> !connectMissing && scanMissing == null
            else -> scanMissing == null && locationEnabled
        }

        val reason: LinkReason = when {
            !adapterEnabled -> LinkReason.BLUETOOTH_OFF
            connectMissing -> LinkReason.CONNECT_PERMISSION_MISSING
            scanMissing == PermissionNeed.BLUETOOTH_SCAN -> LinkReason.SCAN_PERMISSION_MISSING
            scanMissing == PermissionNeed.FINE_LOCATION -> LinkReason.LOCATION_PERMISSION_MISSING
            requiresLocationServices && !locationEnabled -> LinkReason.LOCATION_OFF
            else -> LinkReason.OK
        }

        return Policy(
            canConnect = canConnect,
            canScan = canScan,
            missing = missing.distinct(),
            requiresLocationServices = requiresLocationServices,
            reason = reason,
            userMessage = userMessage(reason, missing)
        )
    }

    /** 通知权限单独判定（不参与 BLE 连接，仅用于系统通知/前台服务提示） */
    fun needsNotificationPermission(sdkInt: Int, granted: Set<PermissionNeed>): Boolean =
        sdkInt >= API_TIRAMISU && PermissionNeed.POST_NOTIFICATIONS !in granted

    /** 直连请求集合：只返回当前 API 真正需要的权限（不请求定位） */
    fun connectRequestPermissions(sdkInt: Int, granted: Set<PermissionNeed>): List<PermissionNeed> =
        if (sdkInt >= API_S && PermissionNeed.BLUETOOTH_CONNECT !in granted) {
            listOf(PermissionNeed.BLUETOOTH_CONNECT)
        } else {
            emptyList()
        }

    /** 扫描请求集合：API<31 只请求定位；API31+ 只请求 SCAN（CONNECT 缺失时一并请求） */
    fun scanRequestPermissions(sdkInt: Int, granted: Set<PermissionNeed>): List<PermissionNeed> {
        val out = mutableListOf<PermissionNeed>()
        if (sdkInt >= API_S) {
            if (PermissionNeed.BLUETOOTH_CONNECT !in granted) out += PermissionNeed.BLUETOOTH_CONNECT
            if (PermissionNeed.BLUETOOTH_SCAN !in granted) out += PermissionNeed.BLUETOOTH_SCAN
        } else if (PermissionNeed.FINE_LOCATION !in granted) {
            out += PermissionNeed.FINE_LOCATION
        }
        return out
    }

    fun userMessage(reason: LinkReason, missing: List<PermissionNeed>): String = when (reason) {
        LinkReason.OK -> ""
        LinkReason.BLUETOOTH_OFF -> "蓝牙未开启（去开启）"
        LinkReason.CONNECT_PERMISSION_MISSING -> "蓝牙权限未授予：需要附近设备权限（去授权）"
        LinkReason.SCAN_PERMISSION_MISSING -> "搜索网关需要附近设备权限（去授权）"
        LinkReason.LOCATION_PERMISSION_MISSING -> "搜索网关需要定位权限（去授权）"
        LinkReason.LOCATION_OFF -> "搜索网关需要开启定位（去开启）"
    }.let { base ->
        if (base.isEmpty() || missing.isEmpty()) base
        else base + "：缺少 " + missing.joinToString("、") { permissionLabel(it) }
    }

    fun permissionLabel(need: PermissionNeed): String = when (need) {
        PermissionNeed.BLUETOOTH_CONNECT -> "BLUETOOTH_CONNECT"
        PermissionNeed.BLUETOOTH_SCAN -> "BLUETOOTH_SCAN"
        PermissionNeed.FINE_LOCATION -> "ACCESS_FINE_LOCATION"
        PermissionNeed.POST_NOTIFICATIONS -> "POST_NOTIFICATIONS"
    }
}
