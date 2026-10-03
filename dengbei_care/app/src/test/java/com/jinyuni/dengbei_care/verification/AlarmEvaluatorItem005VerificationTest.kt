package com.jinyuni.dengbei_care.verification

import com.jinyuni.dengbei_care.telemetry.AlarmEvaluator
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.util.Random

/**
 * 软件测试独立验证（software_tester.software_verification / ITEM-005）。
 *
 * 期望值由本文件按 AA-002 §7 / D-04 / D-05 与任务描述**独立推导**（独立时间窗参考实现），
 * 不复用被测模块或其自检。覆盖：时间窗基准选取与边界、严格比较、开关矩阵与优先级、
 * 紧急阈值、无基准不成立、与"最近第 N 个样本"旧语义的负对照、纯函数性与无伪造数值。
 *
 * 说明：本项只交付纯逻辑模块（`MqtttService` 接线属后续队列项），故验证范围为模块契约，
 * 不声称生产路径已切换。
 */
class AlarmEvaluatorItem005VerificationTest {

    // ---------- 独立参考实现（按设计规则重写） ----------

    private data class Ref(val outcome: String, val message: String, val severity: String, val window: Int?)

    private fun ref(
        now: Long,
        t: Double,
        h: Double,
        p: AlarmEvaluator.Parameters,
        history: List<AlarmEvaluator.Sample>,
        slack: Long = 30L * 60L
    ): Ref {
        if (t > 65.0) return Ref("EMERGENCY", "紧急报警，温度超过65度，谨防火灾", "ALARM", null)
        if (!p.dropEnabled && !p.thresholdEnabled) return Ref("NOT_CONFIGURED", "未设置报警", "NORMAL", null)

        var dropWindow: Int? = null
        if (p.dropEnabled) {
            for (w in listOf(5, 10, 15)) {
                val upper = now - w * 60L
                val lower = upper - slack
                val baseline = history
                    .filter { it.timeSeconds in lower..upper }
                    .filter { it.temperature.isFinite() && it.humidity.isFinite() }
                    .maxByOrNull { it.timeSeconds }
                if (baseline != null &&
                    baseline.temperature - t > p.tempDrop &&
                    baseline.humidity - h > p.humiDrop
                ) {
                    dropWindow = w
                    break
                }
            }
        }
        val thresholdHit = p.thresholdEnabled &&
            (t > p.tempMax || t < p.tempMin || h > p.humiMax || h < p.humiMin)

        return when {
            thresholdHit -> Ref("THRESHOLD", "温湿度超限报警", "WARNING", null)
            dropWindow != null -> Ref("DROP", "温度湿度下降报警", "WARNING", dropWindow)
            else -> Ref("NONE", "无报警", "NORMAL", null)
        }
    }

    private fun call(
        now: Long,
        t: Double,
        h: Double,
        p: AlarmEvaluator.Parameters,
        history: List<AlarmEvaluator.Sample>,
        slack: Long = 30L * 60L
    ) = AlarmEvaluator.evaluate(now, t, h, p, history, slack)

    private fun assertMatchesReference(
        now: Long,
        t: Double,
        h: Double,
        p: AlarmEvaluator.Parameters,
        history: List<AlarmEvaluator.Sample>,
        label: String
    ) {
        val e = ref(now, t, h, p, history)
        val a = call(now, t, h, p, history)
        assertEquals("$label outcome", e.outcome, a.outcome.name)
        assertEquals("$label message", e.message, a.message)
        assertEquals("$label severity", e.severity, a.severity.name)
        assertEquals("$label window", e.window, a.matchedWindowMinutes)
    }

    private val base = AlarmEvaluator.Parameters(
        tempDrop = 2.0, humiDrop = 1.0,
        tempMax = 35.0, tempMin = 29.0, humiMax = 75.0, humiMin = 40.0,
        dropEnabled = true, thresholdEnabled = false
    )

    private fun sample(secAgo: Long, t: Double, h: Double, now: Long) =
        AlarmEvaluator.Sample(now - secAgo, t, h)

    // ---------- 1. 时间窗基准选取与边界（含报告窗口） ----------

    @Test
    fun dropWindowBoundaries_matchSpec() {
        val now = 1_700_000_000L
        // 基准比当前高 3.0℃ / 5.0%RH，两条件都严格超过阈值
        fun at(secAgo: Long) = listOf(sample(secAgo, 30.0, 60.0, now))
        val cur = 27.0 to 55.0

        val cases = mapOf(
            35L * 60 to 5,     // now-35min = 5 分钟窗口下界（含）
            36L * 60 to 10,    // 5 分钟窗口下界之外 → 10 分钟窗口
            41L * 60 to 15,    // 10 分钟窗口下界之外 → 15 分钟窗口
            40L * 60 to 10,
            45L * 60 to 15,    // 15 分钟窗口下界（含）
            46L * 60 to null,  // 超出所有窗口的回溯下界
            4L * 60 to null,   // 过新（晚于 now-5min）
            0L to null         // 当前时刻样本不作为基准
        )
        for ((secAgo, expectedWindow) in cases) {
            val r = call(now, cur.first, cur.second, base, at(secAgo))
            assertEquals("secAgo=$secAgo outcome", if (expectedWindow == null) "NONE" else "DROP", r.outcome.name)
            assertEquals("secAgo=$secAgo window", expectedWindow, r.matchedWindowMinutes)
            assertMatchesReference(now, cur.first, cur.second, base, at(secAgo), "secAgo=$secAgo")
        }
    }

    @Test
    fun latestEligibleSampleIsChosenPerWindow() {
        val now = 1_700_000_000L
        // W=5 窗口内的最新样本是 now-12min；W=10/15 也各自取其窗口内最新
        val history = listOf(
            sample(35L * 60, 40.0, 80.0, now), // 对所有窗口都满足下降，但温度更高（更早）
            sample(12L * 60, 30.0, 60.0, now)  // W=5/W=10 命中，且是最新
        )
        val r = call(now, 27.0, 55.0, base, history)
        assertEquals("DROP", r.outcome.name)
        assertEquals("取最新合格基准 → 5 分钟窗口", 5, r.matchedWindowMinutes)
        assertMatchesReference(now, 27.0, 55.0, base, history, "latest-per-window")

        // 只保留 now-12min：W=5/W=10 均合格 → 仍报 5
        val onlyNew = listOf(sample(12L * 60, 30.0, 60.0, now))
        assertEquals(5, call(now, 27.0, 55.0, base, onlyNew).matchedWindowMinutes)
        // 只保留 now-36min：W=5 不合格（下界 now-35min）→ 报 10
        val onlyOld = listOf(sample(36L * 60, 30.0, 60.0, now))
        assertEquals(10, call(now, 27.0, 55.0, base, onlyOld).matchedWindowMinutes)
        assertMatchesReference(now, 27.0, 55.0, base, onlyOld, "onlyOld")
    }

    // ---------- 2. 无基准 / 无历史 → 下降分支不成立 ----------

    @Test
    fun dropRequiresEligibleBaseline() {
        val now = 1_700_000_000L
        // 三个样本但都在最近 1 分钟内：旧的"最近第 N 个样本"会触发，新实现必须不触发（负对照）
        val recent = listOf(
            sample(30, 30.0, 60.0, now),
            sample(60, 31.0, 65.0, now),
            sample(90, 32.0, 70.0, now)
        )
        val r = call(now, 27.0, 55.0, base, recent)
        assertEquals("样本够但时间窗不够 → 不报警", "NONE", r.outcome.name)
        assertEquals(null, r.matchedWindowMinutes)
        assertMatchesReference(now, 27.0, 55.0, base, recent, "recent-only")

        // 空历史
        assertEquals("NONE", call(now, 27.0, 55.0, base, emptyList()).outcome.name)
        // 无下降（上升）
        assertEquals("NONE", call(now, 33.0, 65.0, base, listOf(sample(20L * 60, 30.0, 60.0, now))).outcome.name)
    }

    // ---------- 3. 严格比较：恰好等于阈值不触发 ----------

    @Test
    fun strictComparisons_exactEqualityDoesNotTrigger() {
        val now = 1_700_000_000L
        // 温度差恰好 == tempDrop（2.0）→ 不触发
        assertEquals("NONE", call(now, 28.0, 55.0, base, listOf(sample(20L * 60, 30.0, 60.0, now))).outcome.name)
        // 湿度差恰好 == humiDrop（1.0）→ 不触发
        assertEquals("NONE", call(now, 27.0, 59.0, base, listOf(sample(20L * 60, 30.0, 60.0, now))).outcome.name)
        // 两点都严格超过 → 触发
        assertEquals("DROP", call(now, 27.9, 58.9, base, listOf(sample(20L * 60, 30.0, 60.0, now))).outcome.name)

        val p = base.copy(dropEnabled = false, thresholdEnabled = true)
        val hist = emptyList<AlarmEvaluator.Sample>()
        // 恰好等于上下限 → 不触发
        assertEquals("NONE", call(now, 35.0, 60.0, p, hist).outcome.name)
        assertEquals("NONE", call(now, 29.0, 60.0, p, hist).outcome.name)
        assertEquals("NONE", call(now, 30.0, 75.0, p, hist).outcome.name)
        assertEquals("NONE", call(now, 30.0, 40.0, p, hist).outcome.name)
        // 略超 → 触发
        assertEquals("THRESHOLD", call(now, 35.1, 60.0, p, hist).outcome.name)
        assertEquals("THRESHOLD", call(now, 28.9, 60.0, p, hist).outcome.name)
        assertEquals("THRESHOLD", call(now, 30.0, 75.1, p, hist).outcome.name)
        assertEquals("THRESHOLD", call(now, 30.0, 39.9, p, hist).outcome.name)
    }

    // ---------- 4. 开关矩阵与优先级 ----------

    @Test
    fun switchMatrix_andPrecedence_matchLegacy() {
        val now = 1_700_000_000L
        // 下降成立且不超限：基准 (40.0, 80.0) → 当前 (30.0, 60.0)（30 在 29..35、60 在 40..75）
        val dropOnlyHist = listOf(sample(20L * 60, 40.0, 80.0, now))
        // 下降与超限同时成立：基准 (40.0, 80.0) → 当前 (36.0, 55.0)（36>35 超限）
        val bothHitHist = listOf(sample(20L * 60, 40.0, 80.0, now))
        val curDrop = 30.0 to 60.0
        val curThreshold = 36.0 to 60.0
        val curBoth = 36.0 to 55.0

        for ((d, th) in listOf(true to true, true to false, false to true, false to false)) {
            val p = base.copy(dropEnabled = d, thresholdEnabled = th)
            val bothOff = !d && !th
            // 无命中
            val rNone = call(now, curDrop.first, curDrop.second, p, emptyList())
            if (bothOff) assertEquals("NOT_CONFIGURED", rNone.outcome.name)
            else assertEquals("d=$d th=$th NONE", "NONE", rNone.outcome.name)
            // 仅下降成立
            val rDrop = call(now, curDrop.first, curDrop.second, p, dropOnlyHist)
            when {
                bothOff -> assertEquals("NOT_CONFIGURED", rDrop.outcome.name)
                d -> assertEquals("DROP", rDrop.outcome.name)
                else -> assertEquals("NONE", rDrop.outcome.name)
            }
            // 仅超限成立
            val rTh = call(now, curThreshold.first, curThreshold.second, p, emptyList())
            when {
                bothOff -> assertEquals("NOT_CONFIGURED", rTh.outcome.name)
                th -> assertEquals("THRESHOLD", rTh.outcome.name)
                else -> assertEquals("NONE", rTh.outcome.name)
            }
            // 两者同时成立：先下降后超限，同时成立时超限文案覆盖下降（与改动前一致）
            val rBoth = call(now, curBoth.first, curBoth.second, p, bothHitHist)
            when {
                bothOff -> assertEquals("NOT_CONFIGURED", rBoth.outcome.name)
                th -> assertEquals("THRESHOLD", rBoth.outcome.name)
                d -> assertEquals("DROP", rBoth.outcome.name)
                else -> assertEquals("NONE", rBoth.outcome.name)
            }
            assertMatchesReference(now, curBoth.first, curBoth.second, p, bothHitHist, "both d=$d th=$th")
            assertMatchesReference(now, curDrop.first, curDrop.second, p, dropOnlyHist, "drop d=$d th=$th")
        }
    }

    // ---------- 5. 紧急阈值（>65.0，且与开关无关） ----------

    @Test
    fun emergencyThreshold_isStrictAndHasPriority() {
        val now = 1_700_000_000L
        val off = base.copy(dropEnabled = false, thresholdEnabled = false)
        // 恰好 65.0 不触发紧急（两开关都关 → 未设置报警）
        assertEquals("NOT_CONFIGURED", call(now, 65.0, 60.0, off, emptyList()).outcome.name)
        assertEquals("NONE", call(now, 64.9, 60.0, base, emptyList()).outcome.name)
        // >65.0 → 紧急，覆盖一切（含开关都关、含同时超限/下降）
        for (t in listOf(65.000001, 65.1, 66.0, 100.0)) {
            for (p in listOf(off, base, base.copy(thresholdEnabled = true))) {
                val r = call(now, t, 90.0, p, listOf(sample(20L * 60, 90.0, 95.0, now)))
                assertEquals("t=$t", "EMERGENCY", r.outcome.name)
                assertEquals("紧急文案", "紧急报警，温度超过65度，谨防火灾", r.message)
                assertEquals("ALARM", r.severity.name)
                assertEquals(null, r.matchedWindowMinutes)
                assertTrue("每次通知", r.notifiesImmediately)
            }
        }
        // 只有紧急才 notifiesImmediately
        assertTrue(!call(now, 27.0, 55.0, base, listOf(sample(20L * 60, 30.0, 60.0, now))).notifiesImmediately)
    }

    // ---------- 6. 文案字面值（兼容面） ----------

    @Test
    fun messageLiterals_areExact() {
        assertEquals("未设置报警", AlarmEvaluator.MSG_NOT_CONFIGURED)
        assertEquals("无报警", AlarmEvaluator.MSG_NONE)
        assertEquals("温度湿度下降报警", AlarmEvaluator.MSG_DROP)
        assertEquals("温湿度超限报警", AlarmEvaluator.MSG_THRESHOLD)
        assertEquals("紧急报警，温度超过65度，谨防火灾", AlarmEvaluator.MSG_EMERGENCY)
        assertEquals(listOf(5, 10, 15), AlarmEvaluator.DROP_WINDOW_MINUTES)
        assertEquals(65.0, AlarmEvaluator.EMERGENCY_TEMP_C, 0.0)
        assertEquals(1800L, AlarmEvaluator.DEFAULT_LOOKBACK_SLACK_SECONDS)
    }

    // ---------- 7. 纯函数性与"不写入伪造数值" ----------

    @Test
    fun evaluate_isPure_andResultCarriesNoMeasurements() {
        val now = 1_700_000_000L
        val history = mutableListOf(
            sample(20L * 60, 30.0, 60.0, now),
            sample(50L * 60, 40.0, 80.0, now)
        )
        val snapshot = history.map { Triple(it.timeSeconds, it.temperature, it.humidity) }
        val first = call(now, 27.0, 55.0, base, history)
        val second = call(now, 27.0, 55.0, base, history)
        assertEquals(first, second)                                   // 同一输入结果相同
        assertEquals(snapshot, history.map { Triple(it.timeSeconds, it.temperature, it.humidity) }) // 不修改入参

        // 结果结构不含任何测量数值字段（不产生伪造温度/湿度）
        val measurementFields = AlarmEvaluator.Result::class.java.declaredFields
            .filter {
                it.type == Double::class.javaPrimitiveType || it.type == Double::class.java ||
                    it.type == Float::class.javaPrimitiveType || it.type == Float::class.java ||
                    it.name.lowercase().contains("temp") || it.name.lowercase().contains("humi") ||
                    it.name.lowercase().contains("reading")
            }
            .map { "${it.name}:${it.type.simpleName}" }
        assertTrue("Result measurement fields=$measurementFields", measurementFields.isEmpty())
    }

    // ---------- 8. 随机属性比对（与独立参考一致） ----------

    @Test
    fun randomCases_matchIndependentReference() {
        val rnd = Random(0x00A1A2A3L)
        val now = 1_700_000_000L
        val offsets = listOf(0L, 60L, 4 * 60L, 5 * 60L, 6 * 60L, 10 * 60L, 12 * 60L, 15 * 60L, 16 * 60L,
            20 * 60L, 30 * 60L, 34 * 60L, 35 * 60L, 36 * 60L, 40 * 60L, 41 * 60L, 45 * 60L, 46 * 60L, 90 * 60L)
        repeat(4000) {
            val p = AlarmEvaluator.Parameters(
                tempDrop = listOf(0.0, 0.9, 2.0, 5.0)[rnd.nextInt(4)],
                humiDrop = listOf(0.0, 1.0, 3.0)[rnd.nextInt(3)],
                tempMax = listOf(30.0, 35.0)[rnd.nextInt(2)],
                tempMin = listOf(25.0, 29.0)[rnd.nextInt(2)],
                humiMax = listOf(70.0, 75.0)[rnd.nextInt(2)],
                humiMin = listOf(40.0, 45.0)[rnd.nextInt(2)],
                dropEnabled = rnd.nextBoolean(),
                thresholdEnabled = rnd.nextBoolean()
            )
            val n = rnd.nextInt(6)
            val history = (0 until n).map {
                AlarmEvaluator.Sample(
                    now - offsets[rnd.nextInt(offsets.size)] - rnd.nextInt(3),
                    -10.0 + rnd.nextInt(60),   // -10.0..49.9
                    rnd.nextInt(100).toDouble()
                )
            }
            val t = -5.0 + rnd.nextInt(750) / 10.0
            val h = rnd.nextInt(110).toDouble()
            assertMatchesReference(now, t, h, p, history, "random#$it")
        }
    }

    // ---------- 9. 异常输入鲁棒性 ----------

    @Test
    fun nonFiniteInputs_neverThrow_andNonFiniteBaselineDoesNotAlarm() {
        val now = 1_700_000_000L
        // 非有限基准不得成为下降依据
        val nanBaseline = listOf(
            AlarmEvaluator.Sample(now - 20 * 60, Double.NaN, Double.NaN),
            AlarmEvaluator.Sample(now - 20 * 60, Double.POSITIVE_INFINITY, 60.0)
        )
        val r = try {
            call(now, 27.0, 55.0, base, nanBaseline)
        } catch (t: Throwable) {
            throw AssertionError("evaluate threw on non-finite baseline: $t", t)
        }
        assertEquals("NONE", r.outcome.name)
        // 非有限当前值不得抛异常
        for (v in listOf(Double.NaN, Double.NEGATIVE_INFINITY)) {
            val rr = try {
                call(now, v, 55.0, base, emptyList())
            } catch (t: Throwable) {
                throw AssertionError("evaluate threw on current=$v: $t", t)
            }
            assertTrue("outcome for current=$v is ${rr.outcome}", rr.outcome.name in
                setOf("NONE", "THRESHOLD", "EMERGENCY"))
        }
    }
}
