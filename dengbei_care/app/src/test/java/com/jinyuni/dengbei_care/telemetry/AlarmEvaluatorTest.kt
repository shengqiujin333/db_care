package com.jinyuni.dengbei_care.telemetry

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * ITEM-005 实现侧自检：报警判定纯逻辑（telemetry/AlarmEvaluator）。
 *
 * 覆盖 TD-SW-002 §4.1：
 * - T-SW-L0-06 未设置报警（两开关都关）；
 * - T-SW-L0-07 时间窗下降（核心改动：含恰好等于阈值、AND 语义、超出回溯容差、样本够但时间不够的负对照、无历史）；
 * - T-SW-L0-08 阈值超限（保持现状：严格比较，恰好等于不触发）；
 * - T-SW-L0-09 优先级与文案（保持现状：先下降后超限，同时成立时超限文案覆盖）；
 * - T-SW-L0-10 紧急（仅 > 65.0 触发，每次通知）。
 */
class AlarmEvaluatorTest {

    private val now = 1_700_000_000L

    /** 与 DashboardFragment/MainActivity 默认一致的参数（仅测试用；模块本身不内置默认值） */
    private fun params(
        tempDrop: Double = 2.0,
        humiDrop: Double = 1.0,
        tempMax: Double = 35.0,
        tempMin: Double = 29.0,
        humiMax: Double = 75.0,
        humiMin: Double = 40.0,
        drop: Boolean = true,
        threshold: Boolean = true
    ) = AlarmEvaluator.Parameters(
        tempDrop = tempDrop,
        humiDrop = humiDrop,
        tempMax = tempMax,
        tempMin = tempMin,
        humiMax = humiMax,
        humiMin = humiMin,
        dropEnabled = drop,
        thresholdEnabled = threshold
    )

    private fun sample(minutesAgo: Long, temperature: Double, humidity: Double) =
        AlarmEvaluator.Sample(now - minutesAgo * 60L, temperature, humidity)

    /** 范围内读数（不触发阈值）：31.0℃ / 55.0 %RH */
    private fun evaluate(
        temperature: Double = 31.0,
        humidity: Double = 55.0,
        parameters: AlarmEvaluator.Parameters = params(),
        history: List<AlarmEvaluator.Sample> = emptyList(),
        slack: Long = AlarmEvaluator.DEFAULT_LOOKBACK_SLACK_SECONDS
    ) = AlarmEvaluator.evaluate(now, temperature, humidity, parameters, history, slack)

    // ---------- L0-06 未设置报警 ----------

    @Test
    fun bothSwitchesOff_isNotConfigured() {
        val r = evaluate(parameters = params(drop = false, threshold = false))
        assertEquals(AlarmEvaluator.Outcome.NOT_CONFIGURED, r.outcome)
        assertEquals("未设置报警", r.message)
        assertEquals(AlarmEvaluator.Severity.NORMAL, r.severity)
        assertNull(r.matchedWindowMinutes)
        assertFalse(r.notifiesImmediately)
    }

    @Test
    fun enabledButNothingMatches_isNoAlarm() {
        val r = evaluate() // 范围内读数、无历史
        assertEquals(AlarmEvaluator.Outcome.NONE, r.outcome)
        assertEquals(AlarmEvaluator.Severity.NORMAL, r.severity)
        assertNull(r.matchedWindowMinutes)
    }

    // ---------- L0-07 时间窗下降 ----------

    @Test
    fun baselineInsideFiveMinuteWindow_triggersDrop() {
        val r = evaluate(history = listOf(sample(5, 34.0, 60.0)))
        assertEquals(AlarmEvaluator.Outcome.DROP, r.outcome)
        assertEquals("温度湿度下降报警", r.message)
        assertEquals(AlarmEvaluator.Severity.WARNING, r.severity)
        assertEquals(5, r.matchedWindowMinutes)
    }

    @Test
    fun baselineInsideWindow_triggersDrop_andSmallestMatchingWindowIsReported() {
        // 窗口选择规则：取“不晚于 now-W”的最新样本；返回**最先满足**的窗口（W 递增）
        assertEquals(5, evaluate(history = listOf(sample(5, 34.0, 60.0))).matchedWindowMinutes)
        assertEquals(5, evaluate(history = listOf(sample(10, 34.0, 60.0))).matchedWindowMinutes)
        assertEquals(5, evaluate(history = listOf(sample(15, 34.0, 60.0))).matchedWindowMinutes)
        assertEquals(5, evaluate(history = listOf(sample(8, 34.0, 60.0))).matchedWindowMinutes)
        // now-36min 已早于 5 分钟窗口下界 now-35min → 由 10 分钟窗口承接
        assertEquals(10, evaluate(history = listOf(sample(36, 34.0, 60.0))).matchedWindowMinutes)
        // now-41min 早于 10 分钟窗口下界 now-40min → 由 15 分钟窗口承接
        assertEquals(15, evaluate(history = listOf(sample(41, 34.0, 60.0))).matchedWindowMinutes)
        // now-46min 早于 15 分钟窗口下界 now-45min → 无窗口成立
        assertNull(evaluate(history = listOf(sample(46, 34.0, 60.0))).matchedWindowMinutes)
    }

    @Test
    fun exactlyAtDropThreshold_doesNotTrigger() {
        // 温度差恰好等于 tempDrop（2.0）→ 不触发
        assertFalse(
            evaluate(history = listOf(sample(5, 33.0, 60.0))).outcome == AlarmEvaluator.Outcome.DROP
        )
        // 湿度差恰好等于 humiDrop（1.0）→ 不触发
        assertFalse(
            evaluate(history = listOf(sample(5, 34.0, 56.0))).outcome == AlarmEvaluator.Outcome.DROP
        )
    }

    @Test
    fun dropRequiresBothTemperatureAndHumidity_strictAnd() {
        // 只有温度满足
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(history = listOf(sample(5, 34.0, 55.0))).outcome)
        // 只有湿度满足
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(history = listOf(sample(5, 31.0, 60.0))).outcome)
        // 湿度上升 → 不满足
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(history = listOf(sample(5, 34.0, 50.0))).outcome)
    }

    @Test
    fun baselineOlderThanAllWindowSlack_doesNotTrigger() {
        // now-50min 早于 now-15min-30min = now-45min → 三个窗口都不成立
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(history = listOf(sample(50, 40.0, 70.0))).outcome)
        // now-35min 恰为 5 分钟窗口下界 → 含边界，命中 5 分钟窗口
        assertEquals(5, evaluate(history = listOf(sample(35, 40.0, 70.0))).matchedWindowMinutes)
    }

    @Test
    fun windowSlackIsInclusiveAtBothBounds() {
        // 基准恰在下界 now-5min-30min → 合格（含边界）
        assertEquals(5, evaluate(history = listOf(sample(35, 34.0, 60.0))).matchedWindowMinutes)
        // 基准恰在上界 now-5min → 合格
        assertEquals(5, evaluate(history = listOf(sample(5, 34.0, 60.0))).matchedWindowMinutes)
    }

    @Test
    fun samplesTooRecent_doNotTrigger_negativeControl() {
        // 3 个样本但都在最近 1 分钟内：旧实现（第 1/2/3 个样本）会触发，时间窗实现不得触发
        val history = listOf(
            AlarmEvaluator.Sample(now - 10, 40.0, 70.0),
            AlarmEvaluator.Sample(now - 30, 40.0, 70.0),
            AlarmEvaluator.Sample(now - 60, 40.0, 70.0)
        )
        val r = evaluate(history = history)
        assertEquals(AlarmEvaluator.Outcome.NONE, r.outcome)
        assertNull(r.matchedWindowMinutes)
    }

    @Test
    fun noHistory_doesNotTriggerAndDoesNotThrow() {
        val r = evaluate(history = emptyList())
        assertEquals(AlarmEvaluator.Outcome.NONE, r.outcome)
        assertNull(r.matchedWindowMinutes)
    }

    @Test
    fun newestSampleInWindowIsUsedAsBaseline() {
        val history = listOf(
            sample(50, 40.0, 70.0), // 早于所有窗口（>45min）→ 不参与
            sample(6, 33.0, 56.0)   // 5 分钟窗口内最新，落差恰好等于阈值 → 不触发
        )
        val r = evaluate(history = history)
        assertEquals(AlarmEvaluator.Outcome.NONE, r.outcome)
        assertNull(r.matchedWindowMinutes)
    }

    @Test
    fun customSlackNarrowsTheWindow() {
        // slack=0 时，now-5min-1s 的样本不在 [now-5min, now-5min] 内 → 不触发
        val history = listOf(AlarmEvaluator.Sample(now - 5 * 60 - 1, 34.0, 60.0))
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(history = history, slack = 0L).outcome)
        assertEquals(5, evaluate(history = history).matchedWindowMinutes)
    }

    // ---------- L0-08 阈值超限（保持现状） ----------

    @Test
    fun thresholdExceeded_triggersWithStrictComparison() {
        // 温度上限 35：36 触发
        assertEquals(AlarmEvaluator.Outcome.THRESHOLD, evaluate(temperature = 36.0).outcome)
        // 温度下限 29：28.9 触发
        assertEquals(AlarmEvaluator.Outcome.THRESHOLD, evaluate(temperature = 28.9).outcome)
        // 湿度上限 75：76 触发
        assertEquals(AlarmEvaluator.Outcome.THRESHOLD, evaluate(humidity = 76.0).outcome)
        // 湿度下限 40：39.9 触发
        assertEquals(AlarmEvaluator.Outcome.THRESHOLD, evaluate(humidity = 39.9).outcome)
    }

    @Test
    fun exactlyAtThreshold_doesNotTrigger() {
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(temperature = 35.0).outcome)
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(temperature = 29.0).outcome)
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(humidity = 75.0).outcome)
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(humidity = 40.0).outcome)
        // 范围内值（34.9 / 74.9）同样不触发
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(temperature = 34.9).outcome)
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(humidity = 74.9).outcome)
    }

    @Test
    fun thresholdOnlyWhenEnabled() {
        // 只开下降：超限值不产生报警
        assertEquals(
            AlarmEvaluator.Outcome.NONE,
            evaluate(temperature = 36.0, parameters = params(drop = true, threshold = false)).outcome
        )
        // 只开超限：下降条件成立也不产生下降报警
        assertEquals(
            AlarmEvaluator.Outcome.THRESHOLD,
            evaluate(temperature = 36.0, parameters = params(drop = false, threshold = true)).outcome
        )
    }

    // ---------- L0-09 优先级与文案（保持现状） ----------

    @Test
    fun bothEnabled_bothHit_thresholdMessageWins() {
        // 下降成立（34.0-27.0=7.0>2、60.0-50.0=10.0>1）且超限成立（27.0 < 29.0）
        val r = evaluate(
            temperature = 27.0,
            humidity = 50.0,
            history = listOf(sample(5, 34.0, 60.0))
        )
        assertEquals(AlarmEvaluator.Outcome.THRESHOLD, r.outcome)
        assertEquals("温湿度超限报警", r.message)
        assertEquals(AlarmEvaluator.Severity.WARNING, r.severity)
    }

    @Test
    fun bothEnabled_dropOnly_dropMessage() {
        val r = evaluate(temperature = 31.0, humidity = 55.0, history = listOf(sample(5, 34.0, 60.0)))
        assertEquals(AlarmEvaluator.Outcome.DROP, r.outcome)
        assertEquals("温度湿度下降报警", r.message)
    }

    @Test
    fun bothEnabled_thresholdOnly_thresholdMessage() {
        val r = evaluate(temperature = 36.0, humidity = 55.0)
        assertEquals(AlarmEvaluator.Outcome.THRESHOLD, r.outcome)
        assertEquals("温湿度超限报警", r.message)
    }

    // ---------- L0-10 紧急（保持现状） ----------

    @Test
    fun emergencyOnlyAboveSixtyFive() {
        // 开关全关以隔离阈值分支，只观察紧急阈值边界（65.0/64.9 不紧急 → 落入未设置报警）
        val off = params(drop = false, threshold = false)
        assertEquals(AlarmEvaluator.Outcome.NOT_CONFIGURED, evaluate(temperature = 65.0, parameters = off).outcome)
        assertEquals(AlarmEvaluator.Outcome.NOT_CONFIGURED, evaluate(temperature = 64.9, parameters = off).outcome)
        assertEquals(AlarmEvaluator.Outcome.EMERGENCY, evaluate(temperature = 65.1, parameters = off).outcome)
        assertEquals(AlarmEvaluator.Outcome.EMERGENCY, evaluate(temperature = 100.0, parameters = off).outcome)
        // 启用阈值时，65.0 因“严格大于”不紧急，但仍受阈值超限分支影响
        assertEquals(AlarmEvaluator.Outcome.THRESHOLD, evaluate(temperature = 65.0).outcome)
    }

    @Test
    fun emergency_overridesEverything_andNotifiesImmediately() {
        val r = evaluate(
            temperature = 66.0,
            humidity = 90.0,
            parameters = params(drop = false, threshold = false), // 开关全关也照样紧急
            history = listOf(sample(5, 90.0, 95.0))
        )
        assertEquals(AlarmEvaluator.Outcome.EMERGENCY, r.outcome)
        assertEquals("紧急报警，温度超过65度，谨防火灾", r.message)
        assertEquals(AlarmEvaluator.Severity.ALARM, r.severity)
        assertTrue(r.notifiesImmediately)
    }

    // ---------- 纯函数性与不伪造数据 ----------

    @Test
    fun evaluate_isPure_doesNotMutateHistory_andIsDeterministic() {
        val history = listOf(sample(5, 34.0, 60.0), sample(20, 33.0, 58.0))
        val snapshot = history.map { Triple(it.timeSeconds, it.temperature, it.humidity) }

        val first = evaluate(history = history)
        val second = evaluate(history = history)

        assertEquals(first, second)
        assertEquals(snapshot, history.map { Triple(it.timeSeconds, it.temperature, it.humidity) })
        assertEquals(2, history.size)
    }

    @Test
    fun result_carriesNoTemperatureOrHumidityValues() {
        val r = evaluate(temperature = 36.0)
        val propertyNames = r::class.java.declaredMethods
            .map { it.name }
            .filter { it.startsWith("get") }
        assertFalse(propertyNames.any { it.contains("Temperature", ignoreCase = true) })
        assertFalse(propertyNames.any { it.contains("Humidity", ignoreCase = true) })
    }

    @Test
    fun nonFiniteHistorySamples_areIgnored() {
        val history = listOf(
            AlarmEvaluator.Sample(now - 5 * 60, Double.NaN, 60.0),
            AlarmEvaluator.Sample(now - 5 * 60, 34.0, Double.POSITIVE_INFINITY)
        )
        assertEquals(AlarmEvaluator.Outcome.NONE, evaluate(history = history).outcome)
    }
}
