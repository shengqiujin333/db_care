package com.jinyuni.dengbei_care.cloud

import android.content.SharedPreferences
import android.util.Log
import com.jinyuni.dengbei_care.ui.zhuce.ApiService
import com.jinyuni.dengbei_care.ui.zhuce.SensorReading
import com.jinyuni.dengbei_care.ui.zhuce.SensorUploadData
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory

/**
 * 云上传编排（AA-002 §5.5/§6、D-07/D-09）。
 *
 * 职责：门控（守护中 + 已注册手机号）→ 幂等入队 → 在**独立作用域**取批上传 → 成功删除 / 失败退避记账。
 * 失败不抛出到调用方，也不阻塞 BLE 与 MQTT 主链路（上传在 [scope] 上执行，与业务循环解耦）。
 *
 * 幂等：本地待发箱以 `(dev_id, time)` 去重，服务端以 `UNIQUE(dev_id, time)` + `INSERT IGNORE` 兜底，
 * 因此重试/补传（含超时后服务端其实已入库的情形）不会产生重复数据。
 *
 * 参数集中在 [Config]（单批条数、退避起止、最大尝试次数）。
 */
class CloudUploadRepository(
    private val api: UploadApi,
    private val outbox: UploadOutbox,
    private val gate: UploadGate,
    private val config: Config = Config(),
    private val now: () -> Long = { System.currentTimeMillis() / 1000 },
    private val scope: CoroutineScope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
) {

    /** 上传接口抽象（生产实现见 [RetrofitUploadApi]；测试可注入伪实现） */
    interface UploadApi {
        /** @return true 表示 HTTP 2xx（视为成功） */
        suspend fun upload(data: SensorUploadData): Boolean
    }

    /** 门控与身份来源（生产实现见 [PrefsUploadGate]） */
    interface UploadGate {
        fun guardActive(): Boolean
        fun phone(): String
        fun gatewayMac(): String
    }

    /** 上传参数（唯一常量来源） */
    data class Config(
        val batchLimit: Int = DEFAULT_BATCH_LIMIT,
        val initialBackoffSeconds: Long = DEFAULT_INITIAL_BACKOFF_SECONDS,
        val maxBackoffSeconds: Long = DEFAULT_MAX_BACKOFF_SECONDS,
        val maxAttempts: Int = DEFAULT_MAX_ATTEMPTS
    ) {
        /** 指数退避：`initial * 2^(attempts-1)`，上限 [maxBackoffSeconds] */
        fun backoffSeconds(attempts: Int): Long {
            if (attempts <= 1) return initialBackoffSeconds
            var seconds = initialBackoffSeconds
            repeat(attempts - 1) {
                if (seconds >= maxBackoffSeconds) return maxBackoffSeconds
                seconds *= 2
            }
            return seconds.coerceAtMost(maxBackoffSeconds)
        }
    }

    /** 一次上传尝试的结果（用于日志/测试断言） */
    data class UploadOutcome(
        val attempted: Int,
        val uploaded: Int,
        val failed: Int,
        val skipped: String? = null
    )

    private val mutex = Mutex()

    /** 门控：守护中且已注册手机号；否则不入队、不发起请求（D-09） */
    fun isGateOpen(): Boolean = gate.guardActive() && gate.phone().isNotBlank()

    /**
     * 业务侧入口：门控通过后幂等入队，并异步触发一次上传（不阻塞调用线程）。
     * @return true 表示本次新入队
     */
    fun enqueueAndUpload(
        devId: String,
        time: Long,
        temperature: Double,
        humidity: Double
    ): Boolean {
        if (!isGateOpen()) {
            Log.i(TAG, "upload gated off (guard=${gate.guardActive()}, phoneRegistered=${gate.phone().isNotBlank()})")
            return false
        }
        val enqueued = outbox.enqueue(devId, time, temperature, humidity)
        if (!enqueued) {
            Log.i(TAG, "duplicate reading ignored by outbox: devId=$devId time=$time")
        }
        triggerUpload()
        return enqueued
    }

    /** 触发一次上传（入队后 / 每轮 BLE 开始 / 网络恢复 / App 冷启动，见 ITEM-010 接线） */
    fun triggerUpload(): Job = scope.launch { uploadPendingOnce() }

    /**
     * 取一批到期行上传一次（串行，同一时刻不产生并发重复请求）。
     * 成功 → 逐行删除；失败 → 逐行 `attempts+1` 并推后退避时间；到 `maxAttempts` 的行保留但不再自动重试。
     */
    suspend fun uploadPendingOnce(): UploadOutcome {
        if (!isGateOpen()) return UploadOutcome(0, 0, 0, "gate_closed")
        if (!mutex.tryLock()) return UploadOutcome(0, 0, 0, "busy")
        try {
            val batch = outbox.dueBatch(config.maxAttempts, config.batchLimit)
            if (batch.isEmpty()) return UploadOutcome(0, 0, 0, "empty")

            val payload = UploadPayload.build(
                phone = gate.phone(),
                mac = gate.gatewayMac(),
                readings = batch.map { SensorReading(it.devId, it.time, it.temperature, it.humidity) }
            ) ?: return UploadOutcome(0, 0, 0, "empty_payload")

            val ok = try {
                api.upload(payload)
            } catch (e: Exception) {
                Log.w(TAG, "upload failed: ${e.message}")
                false
            }

            return if (ok) {
                batch.forEach { outbox.ack(it) }
                Log.i(TAG, "uploaded ${batch.size} reading(s)")
                UploadOutcome(batch.size, batch.size, 0)
            } else {
                batch.forEach { outbox.markFailed(it, config.backoffSeconds(it.attempts + 1)) }
                Log.w(TAG, "upload failed for ${batch.size} reading(s); will retry with backoff")
                UploadOutcome(batch.size, 0, batch.size, "upload_failed")
            }
        } finally {
            mutex.unlock()
        }
    }

    companion object {
        private const val TAG = "CloudUpload"

        /** 单批最多上传条数 */
        const val DEFAULT_BATCH_LIMIT = 50

        /** 首次失败后的退避（秒） */
        const val DEFAULT_INITIAL_BACKOFF_SECONDS = 60L

        /** 退避上限（秒，30 分钟） */
        const val DEFAULT_MAX_BACKOFF_SECONDS = 1800L

        /** 最大尝试次数；达到后保留在待发箱但不再自动重试 */
        const val DEFAULT_MAX_ATTEMPTS = 20
    }
}

/**
 * 生产门控实现：从 `MyPrefs` 读取守护状态与身份。
 *
 * 注意 as-built 约定（`HomeFragment.restoreGuardState`）：`measure_state == "stopping"` 表示守护**运行中**，
 * `"starting"` 表示已停止；`registerphone` 为已注册手机号，`mac_addr` 为网关 MAC。
 */
class PrefsUploadGate(private val prefs: SharedPreferences) : CloudUploadRepository.UploadGate {

    override fun guardActive(): Boolean = prefs.getString(KEY_MEASURE_STATE, "") == STATE_GUARD_RUNNING

    override fun phone(): String = prefs.getString(KEY_REGISTER_PHONE, "").orEmpty()

    override fun gatewayMac(): String = prefs.getString(KEY_MAC_ADDR, "").orEmpty()

    private companion object {
        const val KEY_MEASURE_STATE = "measure_state"
        const val STATE_GUARD_RUNNING = "stopping"
        const val KEY_REGISTER_PHONE = "registerphone"
        const val KEY_MAC_ADDR = "mac_addr"
    }
}

/** 仅依赖服务器基址的 Retrofit 客户端（基址集中于 [CloudConfig]） */
object CloudApiClient {

    val apiService: ApiService by lazy {
        Retrofit.Builder()
            .baseUrl(CloudConfig.HTTP_BASE_URL)
            .addConverterFactory(GsonConverterFactory.create())
            .build()
            .create(ApiService::class.java)
    }
}

/**
 * 生产上传实现：`POST /upload_data`，HTTP 2xx 视为成功（AA-002 §5.5）。
 * 网络/解析异常不在此处吞掉，由 [CloudUploadRepository.uploadPendingOnce] 统一按失败退避处理。
 */
class RetrofitUploadApi(
    private val apiService: ApiService = CloudApiClient.apiService
) : CloudUploadRepository.UploadApi {

    override suspend fun upload(data: SensorUploadData): Boolean =
        apiService.uploadData(data).isSuccessful
}
