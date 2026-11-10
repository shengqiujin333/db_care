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

        /**
         * 详细上传结果（AA-005 D-27）：生产实现按真实 HTTP 状态/异常分类。
         * 默认实现由 [upload] 的布尔结果推导，保持既有伪实现与历史验证用例兼容：
         * true → Success；false → Retryable（与改动前的统一退避一致）。
         */
        suspend fun uploadDetailed(data: SensorUploadData): UploadResult =
            if (upload(data)) UploadResult.Success
            else UploadResult.Retryable(null, "upload_failed")
    }

    /** 门控与身份来源（生产实现见 [PrefsUploadGate]） */
    interface UploadGate {
        fun guardActive(): Boolean
        fun phone(): String
        fun gatewayMac(): String

        /** 401/403/4xx：写入可观测阻塞状态（实现可忽略，便于伪实现） */
        fun markUploadBlocked(result: UploadResult, pendingCount: Int) {}

        /** 手动重试/成功后清除阻塞状态 */
        fun clearUploadBlocked() {}

        /** 可重试失败：记录最近一次结果与待发条数 */
        fun markUploadRetryScheduled(pendingCount: Int) {}

        /** 上传成功：记录最近一次结果 */
        fun markUploadSucceeded(pendingCount: Int) {}
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

            val result = try {
                api.uploadDetailed(payload)
            } catch (e: Exception) {
                Log.w(TAG, "upload failed: ${e.message}")
                UploadResult.fromError(e)
            }

            return when (result) {
                is UploadResult.Success -> {
                    batch.forEach { outbox.ack(it) }
                    val pending = outbox.pendingCount()
                    gate.markUploadSucceeded(pending)
                    Log.i(TAG, "uploaded ${batch.size} reading(s)")
                    UploadOutcome(batch.size, batch.size, 0)
                }
                is UploadResult.Retryable -> {
                    batch.forEach { outbox.markFailed(it, config.backoffSeconds(it.attempts + 1)) }
                    gate.markUploadRetryScheduled(outbox.pendingCount())
                    Log.w(TAG, "upload failed for ${batch.size} reading(s); will retry with backoff (${result.cause})")
                    UploadOutcome(batch.size, 0, batch.size, "upload_failed")
                }
                is UploadResult.AuthRejected, is UploadResult.RequestRejected -> {
                    // 永久拒绝：不进入自动退避队列（park），保留全部行并置全局阻塞状态；
                    // 不影响 BLE/MQTT/告警/入库；用户手动重试时恢复。
                    batch.forEach { outbox.park(it, config.maxAttempts) }
                    gate.markUploadBlocked(result, outbox.pendingCount())
                    Log.w(TAG, "upload rejected for ${batch.size} reading(s); auto-retry paused")
                    UploadOutcome(batch.size, 0, batch.size, "upload_blocked")
                }
            }
        } finally {
            mutex.unlock()
        }
    }

    /**
     * 用户手动“重试上传”：清除阻塞状态、恢复被暂停的到期行（批量 UPDATE，不改表结构）
     * 并立即触发一次上传。
     */
    fun retryBlockedNow(): Job {
        gate.clearUploadBlocked()
        outbox.resetForManualRetry(config.maxAttempts)
        return triggerUpload()
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

    override fun markUploadBlocked(result: UploadResult, pendingCount: Int) {
        val (reason, status) = when (result) {
            is UploadResult.AuthRejected -> KEY_REASON_AUTH to result.httpStatus
            is UploadResult.RequestRejected -> KEY_REASON_REQUEST to result.httpStatus
            else -> "RETRYABLE" to null
        }
        prefs.edit()
            .putString(KEY_UPLOAD_BLOCKED_REASON, reason)
            .putLong(KEY_UPLOAD_BLOCKED_AT, System.currentTimeMillis() / 1000)
            .putString(KEY_UPLOAD_LAST_STATUS, status?.toString() ?: "")
            .putInt(KEY_UPLOAD_PENDING_COUNT, pendingCount)
            .apply()
    }

    override fun clearUploadBlocked() {
        prefs.edit()
            .putString(KEY_UPLOAD_BLOCKED_REASON, "")
            .putLong(KEY_UPLOAD_BLOCKED_AT, 0L)
            .apply()
    }

    override fun markUploadRetryScheduled(pendingCount: Int) {
        prefs.edit()
            .putString(KEY_UPLOAD_LAST_STATUS, KEY_STATUS_RETRYING)
            .putInt(KEY_UPLOAD_PENDING_COUNT, pendingCount)
            .apply()
    }

    override fun markUploadSucceeded(pendingCount: Int) {
        prefs.edit()
            .putString(KEY_UPLOAD_LAST_STATUS, KEY_STATUS_OK)
            .putInt(KEY_UPLOAD_PENDING_COUNT, pendingCount)
            .putString(KEY_UPLOAD_BLOCKED_REASON, "")
            .putLong(KEY_UPLOAD_BLOCKED_AT, 0L)
            .apply()
    }

    companion object {
        const val KEY_MEASURE_STATE = "measure_state"
        const val STATE_GUARD_RUNNING = "stopping"
        const val KEY_REGISTER_PHONE = "registerphone"
        const val KEY_MAC_ADDR = "mac_addr"

        const val KEY_UPLOAD_BLOCKED_REASON = "upload_blocked_reason"
        const val KEY_UPLOAD_BLOCKED_AT = "upload_blocked_at"
        const val KEY_UPLOAD_LAST_STATUS = "upload_last_status"
        const val KEY_UPLOAD_PENDING_COUNT = "upload_pending_count"

        const val KEY_REASON_AUTH = "AUTH_REJECTED"
        const val KEY_REASON_REQUEST = "REQUEST_REJECTED"
        const val KEY_STATUS_RETRYING = "RETRYING"
        const val KEY_STATUS_OK = "OK"
    }
}

/** 仅依赖服务器基址的 Retrofit 客户端（基址集中于 [CloudConfig]） */
object CloudApiClient {

    /**
     * 唯一拦截器注入点（AA-005 D-28）：当前为空实现。
     * 401 的具体机制未定性前，**不得**在这里加入 Authorization/token/签名头；
     * 端到端 D1 定性后由后续任务决定。
     */
    private val interceptorInjectionPoint: okhttp3.Interceptor? = null

    val okHttpClient: okhttp3.OkHttpClient by lazy {
        okhttp3.OkHttpClient.Builder()
            .apply { interceptorInjectionPoint?.let { addInterceptor(it) } }
            .build()
    }

    val apiService: ApiService by lazy {
        Retrofit.Builder()
            .baseUrl(CloudConfig.HTTP_BASE_URL)
            .client(okHttpClient)
            .addConverterFactory(GsonConverterFactory.create())
            .build()
            .create(ApiService::class.java)
    }
}

/**
 * 生产上传实现：`POST /upload_data`。
 * 2xx 视为 HTTP 受理（不代表持久化）；401/403/4xx/5xx/网络异常按 [UploadResult] 分类。
 * 网络/解析异常不在此处吞掉，由 [CloudUploadRepository.uploadPendingOnce] 统一分类。
 */
class RetrofitUploadApi(
    private val apiService: ApiService = CloudApiClient.apiService
) : CloudUploadRepository.UploadApi {

    override suspend fun upload(data: SensorUploadData): Boolean =
        uploadDetailed(data) is UploadResult.Success

    override suspend fun uploadDetailed(data: SensorUploadData): UploadResult {
        val response = try {
            apiService.uploadData(data)
        } catch (e: Exception) {
            return UploadResult.fromError(e)
        }
        return UploadResult.fromHttp(response.code())
    }
}
