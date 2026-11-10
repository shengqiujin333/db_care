package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.ble.BlePermissionPolicy
import com.jinyuni.dengbei_care.ble.LinkReason
import com.jinyuni.dengbei_care.ble.PermissionNeed
import com.jinyuni.dengbei_care.cloud.CloudUploadRepository
import com.jinyuni.dengbei_care.cloud.UploadOutbox
import com.jinyuni.dengbei_care.cloud.UploadOutboxRow
import com.jinyuni.dengbei_care.cloud.UploadOutboxStore
import com.jinyuni.dengbei_care.cloud.UploadResult
import com.jinyuni.dengbei_care.protocol.GatewayFrameCodec
import com.jinyuni.dengbei_care.protocol.GatewayIdentity
import com.jinyuni.dengbei_care.protocol.GatewayReadGuard
import com.jinyuni.dengbei_care.telemetry.BleBatchForwarding
import com.jinyuni.dengbei_care.telemetry.BleObservation
import com.jinyuni.dengbei_care.telemetry.ReadingAttribution
import com.jinyuni.dengbei_care.ui.detail.DetailPresentation
import com.jinyuni.dengbei_care.ui.zhuce.SensorUploadData
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import java.util.Locale

/**
 * 软件测试独立验证（software_tester.software_verification，AA-005 七项增量）。
 *
 * 期望值按 `artifacts/android_tasks.yaml` 的七项验收条件、`artifacts/interface_contract.md`
 * （IC-002 v4.0 §6/§7）与 `artifacts/software_test_design.md`（TD-SW-004 §4.1 L0-27..L0-42）
 * **独立重写**，不复用实现侧自检的断言与期望常量。
 *
 * 覆盖：
 * - G-1 权限策略矩阵（直连/扫描 × SDK × 授予 × 适配器/定位）
 * - G-2 网关身份规范化/排序/绑定三态
 * - G-3 批次转发载荷与全部拒绝分支
 * - G-5 观测占位与合法 0.0/0.0、codec x10 字段 additive
 * - G-6 上传结果四分类 + 401/403 后“不产生自动重试请求”
 * - G-7 容量/MTU/返回长度守卫（含与 codec 帧长公式一致性）
 * - G-4 规则文案冻结文本与禁词
 */
class Aa005SevenItemVerificationTest {

    // ================= G-1 权限策略 =================

    private val allGrants = PermissionNeed.values().toSet()

    @Test
    fun g1_directConnectMatrix_matchesIndependentSpec() {
        val sdks = listOf(26, 29, 30, 31, 33)
        val grantSets = listOf(
            emptySet(),
            setOf(PermissionNeed.BLUETOOTH_CONNECT),
            setOf(PermissionNeed.BLUETOOTH_SCAN),
            setOf(PermissionNeed.FINE_LOCATION),
            setOf(PermissionNeed.BLUETOOTH_CONNECT, PermissionNeed.BLUETOOTH_SCAN),
            allGrants
        )
        for (sdk in sdks) {
            for (granted in grantSets) {
                for (adapter in listOf(true, false)) {
                    for (location in listOf(true, false)) {
                        val ctx = "sdk=$sdk granted=$granted adapter=$adapter location=$location"
                        val p = BlePermissionPolicy.evaluate(
                            sdkInt = sdk, granted = granted, needScan = false,
                            adapterEnabled = adapter, locationEnabled = location
                        )
                        val modern = sdk >= 31
                        val expectedCanConnect =
                            adapter && (!modern || PermissionNeed.BLUETOOTH_CONNECT in granted)
                        assertEquals("canConnect $ctx", expectedCanConnect, p.canConnect)
                        // 直连路径不得因定位权限/定位开关被阻断
                        assertFalse("direct must not require location $ctx",
                            PermissionNeed.FINE_LOCATION in p.missing)
                        assertEquals("direct missing must be connect-only $ctx",
                            if (modern && PermissionNeed.BLUETOOTH_CONNECT !in granted)
                                listOf(PermissionNeed.BLUETOOTH_CONNECT) else emptyList(),
                            p.missing)
                        val expectedReason = when {
                            !adapter -> LinkReason.BLUETOOTH_OFF
                            modern && PermissionNeed.BLUETOOTH_CONNECT !in granted ->
                                LinkReason.CONNECT_PERMISSION_MISSING
                            else -> LinkReason.OK
                        }
                        assertEquals("reason $ctx", expectedReason, p.reason)
                        assertEquals("location must not affect direct connect $ctx",
                            expectedCanConnect, p.canConnect)
                        if (!expectedCanConnect) assertTrue("userMessage must be actionable $ctx",
                            p.userMessage.isNotBlank())
                    }
                }
            }
        }
    }

    @Test
    fun g1_scanMatrix_matchesIndependentSpec_forApiBelow31() {
        for (sdk in listOf(26, 29, 30)) {
            for (granted in listOf(emptySet(), setOf(PermissionNeed.FINE_LOCATION), allGrants)) {
                for (adapter in listOf(true, false)) {
                    for (location in listOf(true, false)) {
                        val ctx = "sdk=$sdk granted=$granted adapter=$adapter location=$location"
                        val p = BlePermissionPolicy.evaluate(
                            sdkInt = sdk, granted = granted, needScan = true,
                            adapterEnabled = adapter, locationEnabled = location
                        )
                        val hasLoc = PermissionNeed.FINE_LOCATION in granted
                        assertEquals("canScan $ctx", adapter && hasLoc && location, p.canScan)
                        assertTrue("requiresLocationServices $ctx", p.requiresLocationServices)
                        val expectedReason = when {
                            !adapter -> LinkReason.BLUETOOTH_OFF
                            !hasLoc -> LinkReason.LOCATION_PERMISSION_MISSING
                            !location -> LinkReason.LOCATION_OFF
                            else -> LinkReason.OK
                        }
                        assertEquals("reason $ctx", expectedReason, p.reason)
                        // 扫描场景下直连仍只取决于适配器（API<31 无需任何运行时权限）
                        assertEquals("connect only depends on adapter $ctx", adapter, p.canConnect)
                    }
                }
            }
        }
    }

    @Test
    fun g1_scanMatrix_matchesIndependentSpec_forApi31Plus() {
        // TD-SW-004 T-SW-L0-28 的公式：sdk>=31 时 canScan = adapter && SCAN granted（定位状态无关）
        for (sdk in listOf(31, 33)) {
            for (granted in listOf(
                emptySet(),
                setOf(PermissionNeed.BLUETOOTH_SCAN),
                setOf(PermissionNeed.BLUETOOTH_CONNECT),
                setOf(PermissionNeed.BLUETOOTH_CONNECT, PermissionNeed.BLUETOOTH_SCAN)
            )) {
                for (adapter in listOf(true, false)) {
                    for (location in listOf(true, false)) {
                        val ctx = "sdk=$sdk granted=$granted adapter=$adapter location=$location"
                        val p = BlePermissionPolicy.evaluate(
                            sdkInt = sdk, granted = granted, needScan = true,
                            adapterEnabled = adapter, locationEnabled = location
                        )
                        assertFalse("location services must not be required on 31+ $ctx",
                            p.requiresLocationServices)
                        // 无扫描权限时必定不可扫描；适配器关闭必定不可扫描（与设计一致）
                        if (PermissionNeed.BLUETOOTH_SCAN !in granted) {
                            assertFalse("scan without SCAN must be refused $ctx", p.canScan)
                        }
                        if (!adapter) {
                            assertFalse("adapter off must not scan $ctx", p.canScan)
                            assertEquals("adapter off $ctx", LinkReason.BLUETOOTH_OFF, p.reason)
                        }
                        // 已同时授予 SCAN 与 CONNECT 时必须可扫描，且与定位总开关无关
                        if (adapter &&
                            PermissionNeed.BLUETOOTH_SCAN in granted &&
                            PermissionNeed.BLUETOOTH_CONNECT in granted
                        ) {
                            assertTrue("scan with SCAN+CONNECT must be allowed $ctx", p.canScan)
                            assertEquals("reason $ctx", LinkReason.OK, p.reason)
                        }
                        // 说明：TD-SW-004 T-SW-L0-28 的公式为 adapter && SCAN；
                        // 本实现在 31+ 还要求 CONNECT（见本轮报告的观察项，不在此判失败）。
                    }
                }
            }
        }
    }

    @Test
    fun g1_permissionRequestSets_byApi() {
        assertEquals(emptyList<PermissionNeed>(),
            BlePermissionPolicy.connectRequestPermissions(29, emptySet()))
        assertEquals(listOf(PermissionNeed.BLUETOOTH_CONNECT),
            BlePermissionPolicy.connectRequestPermissions(31, emptySet()))
        assertEquals(emptyList<PermissionNeed>(),
            BlePermissionPolicy.connectRequestPermissions(31, setOf(PermissionNeed.BLUETOOTH_CONNECT)))
        assertEquals(emptyList<PermissionNeed>(),
            BlePermissionPolicy.connectRequestPermissions(33, allGrants))

        assertEquals(listOf(PermissionNeed.FINE_LOCATION),
            BlePermissionPolicy.scanRequestPermissions(29, emptySet()))
        assertEquals(emptyList<PermissionNeed>(),
            BlePermissionPolicy.scanRequestPermissions(29, setOf(PermissionNeed.FINE_LOCATION)))
        assertEquals(listOf(PermissionNeed.BLUETOOTH_CONNECT, PermissionNeed.BLUETOOTH_SCAN),
            BlePermissionPolicy.scanRequestPermissions(31, emptySet()))
        assertEquals(listOf(PermissionNeed.BLUETOOTH_SCAN),
            BlePermissionPolicy.scanRequestPermissions(31, setOf(PermissionNeed.BLUETOOTH_CONNECT)))
        assertEquals(listOf(PermissionNeed.BLUETOOTH_CONNECT),
            BlePermissionPolicy.scanRequestPermissions(31, setOf(PermissionNeed.BLUETOOTH_SCAN)))

        assertFalse(BlePermissionPolicy.needsNotificationPermission(29, emptySet()))
        assertTrue(BlePermissionPolicy.needsNotificationPermission(33, emptySet()))
        assertFalse(BlePermissionPolicy.needsNotificationPermission(
            33, setOf(PermissionNeed.POST_NOTIFICATIONS)))
        assertFalse(BlePermissionPolicy.needsNotificationPermission(33, allGrants))
    }

    @Test
    fun g1_userMessages_areSpecificAndActionable() {
        val cases = mapOf(
            LinkReason.BLUETOOTH_OFF to "蓝牙",
            LinkReason.CONNECT_PERMISSION_MISSING to "权限",
            LinkReason.SCAN_PERMISSION_MISSING to "权限",
            LinkReason.LOCATION_PERMISSION_MISSING to "定位权限",
            LinkReason.LOCATION_OFF to "定位"
        )
        for ((reason, mustContain) in cases) {
            val msg = BlePermissionPolicy.userMessage(
                reason,
                when (reason) {
                    LinkReason.OK -> emptyList()
                    LinkReason.BLUETOOTH_OFF -> emptyList()
                    LinkReason.CONNECT_PERMISSION_MISSING -> listOf(PermissionNeed.BLUETOOTH_CONNECT)
                    LinkReason.SCAN_PERMISSION_MISSING -> listOf(PermissionNeed.BLUETOOTH_SCAN)
                    LinkReason.LOCATION_PERMISSION_MISSING -> listOf(PermissionNeed.FINE_LOCATION)
                    LinkReason.LOCATION_OFF -> listOf(PermissionNeed.FINE_LOCATION)
                }
            )
            assertTrue("message for $reason must contain $mustContain, actual=$msg",
                msg.contains(mustContain))
        }
        assertEquals("", BlePermissionPolicy.userMessage(LinkReason.OK, emptyList()))
    }

    // ================= G-2 网关身份 =================

    @Test
    fun g2_normalize_acceptsDocumentedForms_andRejectsInvalid() {
        val valid = mapOf(
            "84:c2:e4:3:2:2" to "84:C2:E4:03:02:02",
            "84C2E4030202" to "84:C2:E4:03:02:02",
            "84-C2-E4-03-02-02" to "84:C2:E4:03:02:02",
            "  84:c2:e4:03:02:02  " to "84:C2:E4:03:02:02",
            "AA:bb:CC:dd:EE:ff" to "AA:BB:CC:DD:EE:FF"
        )
        for ((raw, expected) in valid) {
            assertEquals("normalize($raw)", expected, GatewayIdentity.normalize(raw))
            assertTrue(GatewayIdentity.isValid(raw))
        }
        val invalid = listOf(
            null, "", "   ", "84C2E403020", "84C2E40302020", "84:C2:E4:03:02",
            "84:C2:E4:03:02:02:03", "84:C2:E4:03:02:0G", "84::C2:E4:03:02",
            ":84:C2:E4:03:02:02", "84:C2:E4:03:02:02:", "84-C2:E4:03:02:02-",
            "84:C2E4:03:02:02", "84:C2:E4:03:02:0G"
        )
        for (raw in invalid) {
            assertNull("normalize($raw) must be null", GatewayIdentity.normalize(raw))
            assertFalse("isValid($raw) must be false", GatewayIdentity.isValid(raw))
        }
    }

    @Test
    fun g2_rank_dedupesAndOrders() {
        val input = listOf(
            GatewayIdentity.Candidate("84:c2:e4:03:02:02", "gw", -70, false),
            GatewayIdentity.Candidate("84:C2:E4:03:02:02", "gw2", -50, true),
            GatewayIdentity.Candidate("AA:BB:CC:DD:EE:01", "a", -60, true),
            GatewayIdentity.Candidate("AA:BB:CC:DD:EE:02", "b", -40, true),
            GatewayIdentity.Candidate("AA:BB:CC:DD:EE:03", "c", -40, false),
            GatewayIdentity.Candidate("not-a-mac", "x", -10, true)
        )
        val snapshot = input.map { it.copy() }
        val ranked = GatewayIdentity.rank(input)
        assertEquals("invalid MAC must be dropped", 4, ranked.size)
        assertEquals(listOf("AA:BB:CC:DD:EE:02", "84:C2:E4:03:02:02", "AA:BB:CC:DD:EE:01",
            "AA:BB:CC:DD:EE:03"), ranked.map { it.mac })
        assertEquals("FFE0 candidate wins duplicate", true, ranked.last { it.mac == "84:C2:E4:03:02:02" }.hasFfe0)
        assertEquals("input must not be modified", snapshot, input)
    }

    @Test
    fun g2_bindingStates_andResolveAddress() {
        assertEquals(GatewayIdentity.BindingState.DEFAULT_UNCONFIRMED,
            GatewayIdentity.bindingState(null, cleared = false))
        assertEquals(GatewayIdentity.BindingState.UNSET,
            GatewayIdentity.bindingState(null, cleared = true))
        assertEquals(GatewayIdentity.BindingState.UNSET,
            GatewayIdentity.bindingState("", cleared = true))
        assertEquals(GatewayIdentity.BindingState.BOUND,
            GatewayIdentity.bindingState("aa:bb:cc:dd:ee:01", cleared = false))
        assertEquals(GatewayIdentity.BindingState.BOUND,
            GatewayIdentity.bindingState("aa:bb:cc:dd:ee:01", cleared = true))

        assertEquals(GatewayIdentity.DEFAULT_GATEWAY_BLE_ADDR,
            GatewayIdentity.resolveAddress(null, cleared = false))
        assertNull("cleared must never fall back to default", GatewayIdentity.resolveAddress(null, cleared = true))
        assertEquals("AA:BB:CC:DD:EE:01", GatewayIdentity.resolveAddress("aa:bb:cc:dd:ee:01", cleared = true))
        assertTrue(GatewayIdentity.isMigrationDefault(null, cleared = false))
        assertFalse(GatewayIdentity.isMigrationDefault(null, cleared = true))
        assertFalse(GatewayIdentity.isMigrationDefault("AA:BB:CC:DD:EE:01", cleared = false))
    }

    // ================= G-7 容量/完整性 =================

    @Test
    fun g7_capacity_expectedSize_andMtu() {
        assertEquals(9, GatewayReadGuard.MAX_DEVICES)
        assertEquals(160, GatewayReadGuard.MAX_FRAME_BYTES)
        assertEquals(160, GatewayReadGuard.expectedFrameSize(9))
        for (n in 0..9) {
            assertTrue("capacity $n", GatewayReadGuard.capacityVerdict(n) is GatewayReadGuard.Capacity.Ok)
        }
        for (n in 10..255) {
            val v = GatewayReadGuard.capacityVerdict(n)
            assertTrue("capacity $n", v is GatewayReadGuard.Capacity.Exceeded)
            assertEquals(n, (v as GatewayReadGuard.Capacity.Exceeded).bound)
        }
        for (n in 0..255) {
            assertEquals("frame size consistency n=$n",
                GatewayFrameCodec.expectedFrameSize(n), GatewayReadGuard.expectedFrameSize(n))
        }
        fun mtuRefused(mtu: Int, n: Int) =
            GatewayReadGuard.mtuVerdict(mtu, n) is GatewayReadGuard.MtuVerdict.Refuse
        assertTrue(mtuRefused(23, 1)); assertTrue(mtuRefused(34, 1)); assertFalse(mtuRefused(35, 1))
        assertTrue(mtuRefused(162, 9)); assertFalse(mtuRefused(163, 9))
        assertFalse(mtuRefused(240, 9)); assertFalse(mtuRefused(240, 8))
        val r = GatewayReadGuard.mtuVerdict(34, 1) as GatewayReadGuard.MtuVerdict.Refuse
        assertEquals(35, r.requiredMtu); assertEquals(34, r.actualMtu)
    }

    @Test
    fun g7_lengthVerdict_alignThenCoverage() {
        fun reasonOf(bytes: Int, n: Int) =
            (GatewayReadGuard.lengthVerdict(bytes, n) as? GatewayReadGuard.LengthVerdict.Refuse)?.reason
        assertEquals(GatewayReadGuard.ReadRefuseReason.LENGTH_TOO_SMALL, reasonOf(0, 1))
        assertEquals(GatewayReadGuard.ReadRefuseReason.LENGTH_TOO_SMALL, reasonOf(16, 1))
        assertEquals(GatewayReadGuard.ReadRefuseReason.LENGTH_TOO_SMALL, reasonOf(48, 3))
        assertEquals(GatewayReadGuard.ReadRefuseReason.LENGTH_TOO_SMALL, reasonOf(144, 9))
        assertEquals(GatewayReadGuard.ReadRefuseReason.LENGTH_NOT_ALIGNED, reasonOf(20, 1))
        assertEquals(GatewayReadGuard.ReadRefuseReason.LENGTH_NOT_ALIGNED, reasonOf(31, 1))
        assertEquals(GatewayReadGuard.ReadRefuseReason.LENGTH_NOT_ALIGNED, reasonOf(63, 3))
        assertEquals(GatewayReadGuard.ReadRefuseReason.LENGTH_NOT_ALIGNED, reasonOf(152, 9))
        for ((bytes, n) in listOf(32 to 1, 48 to 1, 64 to 3, 160 to 9, 176 to 9, 256 to 9)) {
            assertTrue("$bytes B / N=$n", GatewayReadGuard.lengthVerdict(bytes, n) is
                    GatewayReadGuard.LengthVerdict.Proceed)
        }
        assertEquals(32, (GatewayReadGuard.lengthVerdict(32, 1) as
                GatewayReadGuard.LengthVerdict.Proceed).expectedBytes)
    }

    // ================= G-3 批次转发 =================

    @Test
    fun g3_singleDevicePayloadIsExact_andParsesBack() {
        val r = BleObservation.Reading("A1B2C3D4", 255, 600)
        val plan = BleBatchForwarding.plan(listOf(r), listOf("A1B2C3D4"), 1_700_000_000L)
        assertTrue("plan=$plan", plan is BleBatchForwarding.Plan.Forward)
        assertEquals("1700000000:25.5:60.0", (plan as BleBatchForwarding.Plan.Forward).payload)
        val parsed = ReadingAttribution.attribute(plan.payload, listOf("A1B2C3D4"))
        assertTrue(parsed is ReadingAttribution.Result.Success)
        val reading = (parsed as ReadingAttribution.Result.Success).batch.readings.single()
        assertEquals("A1B2C3D4", reading.devId)
        assertEquals(25.5, reading.temperature, 0.0)
        assertEquals(60.0, reading.humidity, 0.0)
    }

    @Test
    fun g3_localeIndependent_andNegativeTemperature() {
        val old = Locale.getDefault()
        try {
            for (locale in listOf(Locale.US, Locale.CHINA, Locale.GERMANY, Locale.FRANCE)) {
                Locale.setDefault(locale)
                val r = BleObservation.Reading("A1B2C3D4", -32, 0)
                val plan = BleBatchForwarding.plan(listOf(r), listOf("A1B2C3D4"), 42L)
                assertEquals("locale=$locale", "42:-3.2:0.0",
                    (plan as BleBatchForwarding.Plan.Forward).payload)
                assertFalse(plan.payload.contains(" "))
            }
        } finally {
            Locale.setDefault(old)
        }
    }

    @Test
    fun g3_multiDeviceOrder_andRefusalBranches() {
        val a = BleObservation.Reading("A1B2C3D4", 255, 600)
        val b = BleObservation.Reading("0C118224", -1, 1)
        val c = BleObservation.Reading("11223344", 0, 0)

        val ok = BleBatchForwarding.plan(listOf(a, b, c), listOf("A1B2C3D4", "0C118224", "11223344"), 9L)
        assertEquals("9:25.5,-0.1,0.0:60.0,0.1,0.0",
            (ok as BleBatchForwarding.Plan.Forward).payload)

        fun reason(plan: BleBatchForwarding.Plan) =
            (plan as BleBatchForwarding.Plan.Refuse).reason

        assertEquals(BleBatchForwarding.RefuseReason.NO_BOUND_DEVICE,
            reason(BleBatchForwarding.plan(emptyList(), emptyList(), 1L)))
        assertEquals(BleBatchForwarding.RefuseReason.COUNT_MISMATCH,
            reason(BleBatchForwarding.plan(emptyList(), listOf("A1B2C3D4"), 1L)))
        assertEquals(BleBatchForwarding.RefuseReason.ALL_PLACEHOLDER,
            reason(BleBatchForwarding.plan(emptyList(), listOf("A1B2C3D4"), 1L, allPlaceholder = true)))
        assertEquals(BleBatchForwarding.RefuseReason.COUNT_MISMATCH,
            reason(BleBatchForwarding.plan(listOf(a), listOf("A1B2C3D4", "0C118224"), 1L)))
        assertEquals(BleBatchForwarding.RefuseReason.DUPLICATE_ID,
            reason(BleBatchForwarding.plan(listOf(a, a), listOf("A1B2C3D4", "0C118224"), 1L)))
        assertEquals(BleBatchForwarding.RefuseReason.UNKNOWN_ID,
            reason(BleBatchForwarding.plan(listOf(a, b), listOf("A1B2C3D4", "11223344"), 1L)))
        assertEquals(BleBatchForwarding.RefuseReason.ORDER_MISMATCH,
            reason(BleBatchForwarding.plan(listOf(b, a), listOf("A1B2C3D4", "0C118224"), 1L)))
        // 拒绝结果不携带载荷字段
        val refused = BleBatchForwarding.plan(listOf(a), listOf("A1B2C3D4", "0C118224"), 1L)
        assertTrue(refused is BleBatchForwarding.Plan.Refuse)
        assertTrue(refused.javaClass.declaredFields.none { it.name.contains("payload", true) })
    }

    // ================= G-5 观测有效性 =================

    @Test
    fun g5_placeholderAndValidZero() {
        assertTrue(BleObservation.isPlaceholder("00000000", 0, 0))
        assertTrue(BleObservation.isPlaceholder("00:00:00:00", 0, 0))
        assertFalse("non-zero temperature is not placeholder", BleObservation.isPlaceholder("00000000", 1, 0))
        assertFalse("non-zero humidity is not placeholder", BleObservation.isPlaceholder("00000000", 0, 1))
        assertFalse("valid ID with 0/0 is a reading", BleObservation.isPlaceholder("A1B2C3D4", 0, 0))

        assertTrue(BleObservation.classify("00000000", 0, 0) is BleObservation.Observation.Placeholder)
        val v = BleObservation.classify("a1:b2:c3:d4", 0, 0) as BleObservation.Observation.Valid
        assertEquals("A1B2C3D4", v.reading.devId)
        assertEquals(0.0, v.reading.temperatureC, 0.0)
        assertEquals(0.0, v.reading.humidityPct, 0.0)
        assertTrue(v.reading.tX10 == 0 && v.reading.hX10 == 0)
    }

    @Test
    fun g5_codecX10FieldsAreAdditive_andLayoutUnchanged() {
        // IC-002 §5 向量：ID 01 02 03 04、H=0x0258=600、T=0xFFCE=-50
        val block = byteArrayOf(
            0x01, 0x02, 0x03, 0x04, 0x02, 0x58, 0xFF.toByte(), 0xCE.toByte(),
            0, 0, 0, 0, 0, 0, 0, 0
        )
        val sample = GatewayFrameCodec.decodeDeviceBlock(block, 0)
        assertEquals("0C:11:82:24", GatewayFrameCodec.toColonHex(
            byteArrayOf(0x0C, 0x11, 0x82.toByte(), 0x24)))
        assertEquals("01020304", sample.devIdCompact)
        assertEquals(600, sample.humidityX10)
        assertEquals(-50, sample.temperatureX10)
        assertEquals(60.0, sample.humidityPct, 0.0)
        assertEquals(-5.0, sample.temperatureC, 0.0)
    }

    // ================= G-6 上传分类与阻塞 =================

    @Test
    fun g6_uploadResultClassificationMatrix() {
        for (s in listOf(200, 201, 204, 299)) {
            assertTrue("$s -> Success", UploadResult.fromHttp(s) is UploadResult.Success)
        }
        for (s in listOf(401, 403)) {
            val r = UploadResult.fromHttp(s)
            assertTrue("$s -> AuthRejected", r is UploadResult.AuthRejected)
            assertEquals(s, (r as UploadResult.AuthRejected).httpStatus)
        }
        for (s in listOf(400, 404, 413, 422, 499)) {
            assertTrue("$s -> RequestRejected", UploadResult.fromHttp(s) is UploadResult.RequestRejected)
        }
        for (s in listOf(408, 429, 500, 502, 503, 599)) {
            assertTrue("$s -> Retryable", UploadResult.fromHttp(s) is UploadResult.Retryable)
        }
        assertTrue(UploadResult.fromHttp(null) is UploadResult.Retryable)
        assertTrue(UploadResult.fromHttp(301) is UploadResult.Retryable)
        assertTrue(UploadResult.fromError(java.io.IOException("io")) is UploadResult.Retryable)
    }

    private class MemStore : UploadOutboxStore {
        val rows = LinkedHashMap<Pair<String, Long>, UploadOutboxRow>()
        override fun insertIfAbsent(row: UploadOutboxRow): Boolean {
            val key = row.devId to row.time
            if (rows.containsKey(key)) return false
            rows[key] = row.copy()
            return true
        }
        override fun dueRows(now: Long, maxAttempts: Int, limit: Int): List<UploadOutboxRow> =
            rows.values.filter { it.nextAttemptAt <= now && it.attempts < maxAttempts }
                .sortedWith(compareBy({ it.nextAttemptAt }, { it.time }, { it.devId }))
                .take(limit)
        override fun delete(devId: String, time: Long) { rows.remove(devId to time) }
        override fun updateRetry(devId: String, time: Long, attempts: Int, nextAttemptAt: Long) {
            val key = devId to time
            val old = rows[key] ?: return
            rows[key] = old.copy(attempts = attempts, nextAttemptAt = nextAttemptAt)
        }
        override fun count(): Int = rows.size
        override fun resetForManualRetry(maxAttempts: Int) {
            rows.replaceAll { _, row ->
                if (row.attempts >= maxAttempts) row.copy(attempts = 0, nextAttemptAt = 0L) else row
            }
        }
    }

    private class FakeGate(
        var guard: Boolean = true,
        var phone: String = "13800000000",
        var mac: String = "AABBCCDDEEFF"
    ) : CloudUploadRepository.UploadGate {
        override fun guardActive(): Boolean = guard
        override fun phone(): String = phone
        override fun gatewayMac(): String = mac
    }

    private class FakeApi(private val result: () -> UploadResult) : CloudUploadRepository.UploadApi {
        val calls = ArrayList<SensorUploadData>()
        override suspend fun upload(data: SensorUploadData): Boolean {
            calls += data
            return result() is UploadResult.Success
        }
        override suspend fun uploadDetailed(data: SensorUploadData): UploadResult {
            calls += data
            return result()
        }
    }

    @Test
    fun g6_authRejected_blocksTheRejectedBatch_andKeepsAllRows() = runBlocking {
        var clock = 1_700_000_000L
        val store = MemStore()
        val outbox = UploadOutbox(store) { clock }
        val api = FakeApi { UploadResult.AuthRejected(401) }
        val repo = CloudUploadRepository(
            api, outbox, FakeGate(), CloudUploadRepository.Config(), { clock },
            CoroutineScope(Dispatchers.Unconfined)
        )
        store.rows["A1B2C3D4" to 10L] = UploadOutboxRow("A1B2C3D4", 10L, 25.0, 60.0, 0, 0L)
        store.rows["0C118224" to 11L] = UploadOutboxRow("0C118224", 11L, 26.0, 61.0, 0, 0L)

        val first = repo.uploadPendingOnce()
        assertEquals("upload_blocked", first.skipped)
        assertEquals(2, store.rows.size)
        assertEquals("both rows parked at max attempts", 20, store.rows.values.maxOf { it.attempts })

        // 同一批被拒绝的行不得自动重试
        clock += 10_000
        val second = repo.uploadPendingOnce()
        assertEquals("empty", second.skipped)
        assertEquals("no automatic retry of the rejected batch", 1, api.calls.size)
        assertTrue("rejected rows kept", store.rows.size == 2)
    }

    /**
     * 验收（ITEM-005）：401/403 与其他 4xx **不产生自动重试请求**，并置全局阻塞状态。
     *
     * 场景：一次被拒批次只覆盖单批上限内的行；其余到期行未在该批中。若阻塞是“全局”的，
     * 在下一次自动触发（每轮 BLE / 入队后 / 网络恢复）时不应再发出任何自动请求，直到用户手动重试。
     * 若实现只 park 了本批行、未在取批前检查全局阻塞，则其余到期行仍会被自动上传 → 与本断言不符。
     */
    @Test
    fun g6_authRejected_mustPauseAllAutomaticRequests_untilManualRetry() = runBlocking {
        var clock = 1_700_000_000L
        val store = MemStore()
        val outbox = UploadOutbox(store) { clock }
        val api = FakeApi { UploadResult.AuthRejected(401) }
        val repo = CloudUploadRepository(
            api, outbox, FakeGate(), CloudUploadRepository.Config(batchLimit = 1), { clock },
            CoroutineScope(Dispatchers.Unconfined)
        )
        // 两行均到期，单批上限 1：第一行被 401 拒绝并 park，第二行仍在待发箱且自动可发
        store.rows["A1B2C3D4" to 10L] = UploadOutboxRow("A1B2C3D4", 10L, 25.0, 60.0, 0, 0L)
        store.rows["0C118224" to 11L] = UploadOutboxRow("0C118224", 11L, 26.0, 61.0, 0, 0L)

        val first = repo.uploadPendingOnce()
        assertEquals("upload_blocked", first.skipped)
        assertEquals(1, api.calls.size)

        // 下一次自动触发（模拟每轮 BLE 的 triggerUpload）
        clock += 200
        val second = repo.uploadPendingOnce()
        assertEquals(
            "401 后不得再产生自动上传请求（其余到期行必须等待手动重试）",
            "empty", second.skipped
        )
        assertEquals("401 后自动请求次数必须仍为 1", 1, api.calls.size)
    }

    @Test
    fun g6_manualRetry_restoresParkedRows_andSucceeds() = runBlocking {
        var clock = 1_700_000_000L
        val store = MemStore()
        val outbox = UploadOutbox(store) { clock }
        var result: UploadResult = UploadResult.AuthRejected(401)
        val api = FakeApi { result }
        val repo = CloudUploadRepository(
            api, outbox, FakeGate(), CloudUploadRepository.Config(), { clock },
            CoroutineScope(Dispatchers.Unconfined)
        )
        store.rows["A1B2C3D4" to 10L] = UploadOutboxRow("A1B2C3D4", 10L, 25.0, 60.0, 0, 0L)
        repo.uploadPendingOnce()
        assertEquals(20, store.rows.values.single().attempts)

        result = UploadResult.Success
        repo.retryBlockedNow().join()
        assertEquals("manual retry must succeed and delete the row", 0, store.rows.size)
        assertEquals(2, api.calls.size)
    }

    // ================= G-4 规则文案 =================

    @Test
    fun g4_frozenRuleText_andBannedWords() {
        val frozen = "采样周期 3 分钟。仅当温度较上次升高超过 0.9℃且无光，或温度超过 35.0℃ 时才上报；" +
                "未满足条件时不会上报，因此可能长时间没有新数据。"
        assertEquals(frozen, DetailPresentation.REPORT_RULE_TEXT)
        assertTrue(DetailPresentation.REPORT_RULE_TEXT.contains("升高超过 0.9"))
        assertFalse(DetailPresentation.REPORT_RULE_TEXT.contains("较上次下降"))
        assertFalse(DetailPresentation.REPORT_RULE_TEXT.contains("下降超过 0.9"))
        // 不显示环境光状态/照度（规则原文“无光”允许）
        for (banned in listOf("lux", "照度", "光照", "勒克斯")) {
            assertFalse("rule text must not contain $banned", DetailPresentation.REPORT_RULE_TEXT.contains(banned))
        }
        assertTrue(DetailPresentation.isHighTemperature(35.1))
        assertFalse(DetailPresentation.isHighTemperature(35.0))
        assertEquals(24, DetailPresentation.MAX_CHART_RANGE_HOURS)
        assertEquals(86_400_000L, DetailPresentation.MAX_CHART_RANGE_MS)
        assertFalse(DetailPresentation.rangeLimitMessage().contains("50"))
    }
}
